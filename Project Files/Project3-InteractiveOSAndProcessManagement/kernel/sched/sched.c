#include <os/list.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/mm.h>
#include <os/smp.h>
#include <screen.h>
#include <printk.h>
#include <assert.h>

#define SR_SPIE   0x00000020 /* Previous Supervisor IE */
#define ARG_POINTER_SIZE 8
#define ARGS_MAX_LEN 16
#define NAME_LEN_TO_PRINT 16
#define HALF_PAGE 2048

#define READY_QUEUE_NOT_SINGLE(id) (ready_queue[id].prev != NULL)
#define ALIGNED_128(addr) (((addr) / 128) * 128) 
#define IS_READY_QUEUE(queue) (queue == &ready_queue[0] || queue == &ready_queue[1])

extern void ret_from_exception();

pcb_t pcb[NUM_MAX_TASK];
const ptr_t pid0_stack_core0 = INIT_KERNEL_STACK + HALF_PAGE;
const ptr_t pid0_stack_core1 = INIT_KERNEL_STACK + PAGE_SIZE;

pcb_t pid0_pcb_core0 = {
    .pid = 0,
    .name = "wfi_core0",
    .status = TASK_READY,
    .kernel_sp = (ptr_t)pid0_stack_core0,
    .list = {.index = 0, .next = NULL, .prev = NULL},
    .locks.hold_lock_num = 0,
    .end_ticks = 0,
    .core_mask = 0,
    .running_on = 0
};

pcb_t pid0_pcb_core1 = {
    .pid = 1,
    .name = "wfi_core1",
    .status = TASK_READY,
    .kernel_sp = (ptr_t)pid0_stack_core1,
    .list = {.index = 1, .next = NULL, .prev = NULL},
    .locks.hold_lock_num = 0,
    .end_ticks = 0,
    .core_mask = 1,
    .running_on = 1
};

list_head_t ready_queue[CORE_NUM] = {
    {.index = 2, .next = NULL, .prev = NULL, .tail = &(ready_queue[0])}, // shell
    {.index = 1, .next = NULL, .prev = NULL, .tail = &(ready_queue[1])}  // pid0_core1
};
LIST_HEAD(sleep_queue);

/* current running task PCB */
pcb_t * volatile current_running[CORE_NUM];

/* previous running task PCB */
pcb_t * volatile prev_running;

pcb_t * volatile pointer_to_pid0[CORE_NUM] = {&pid0_pcb_core0, &pid0_pcb_core1};

/* global process id */
pid_t process_id = 2;

int core_task_count = 1;

void do_scheduler(void)
{ 
    int core_id = get_current_cpu_id();
    int core_mask = current_running[core_id]->core_mask;
    int put_on_which_core;
    if(core_mask != CORE_MASK_DEFAULT){
        put_on_which_core = core_mask;
    }
    else{
        put_on_which_core = allocate_core();
    }

    prev_running = current_running[core_id];
    current_running[core_id] = &(pcb[ready_queue[core_id].index]);

    /* 杀死ZOMBIE */
    if(prev_running->status == TASK_ZOMBIE){
        prev_running->status == TASK_EXITED;
    }

    while(current_running[core_id]->status == TASK_ZOMBIE){
        current_running[core_id]->status == TASK_EXITED;
        queue_head_get_out(&(ready_queue[core_id]));
        current_running[core_id] = &(pcb[ready_queue[core_id].index]);
    }

    /* 将当前运行的进程放回队尾 */
    if(put_on_which_core == core_id){
        // 放在当前核的等待队列上
        if(prev_running->pid != core_id){
            // 非pid0
            if(prev_running->status == TASK_RUNNING && READY_QUEUE_NOT_SINGLE(core_id)){
                // 如果不止ready_queue[core_id]一个结点，则向队尾加入
                prev_running->status = TASK_READY;
                ready_queue[core_id].tail->prev = &(prev_running->list);
                prev_running->list.next = ready_queue[core_id].tail;
                prev_running->list.prev = NULL;
                ready_queue[core_id].tail = &(prev_running->list);
            }
            else if(prev_running->status == TASK_RUNNING && ready_queue[core_id].index != prev_running->pid){
                // 如果只有ready_queue[core_id]一个结点，但不止一个task（两个task）
                // 则在ready_queue[core_id]结点和current_running间切换就可以了
                prev_running->status = TASK_READY;
                ready_queue[core_id].index = prev_running->pid;
            }
            else if(prev_running->status != TASK_RUNNING && ready_queue[core_id].index == prev_running->pid){
                // 空队列，等待调度
                ready_queue[core_id].index = core_id;
                current_running[core_id] = pointer_to_pid0[core_id];
            }
        }
    }
    else{
        // 换核
        if(prev_running->status == TASK_RUNNING){
            prev_running->status = TASK_READY;
            prev_running->running_on = put_on_which_core;
            put_at_tail(&(ready_queue[put_on_which_core]), &(prev_running->list));
        }
        if(ready_queue[core_id].index == prev_running->pid){
            // 空队列，等待调度
            ready_queue[core_id].index = core_id;
            current_running[core_id] = pointer_to_pid0[core_id];
        }
    }
    
    
    /* 新结点进入队头 */
    if(READY_QUEUE_NOT_SINGLE(core_id)){
        // 下一个执行的task进入ready_queue[core_id]结点
        ready_queue[core_id].index = ready_queue[core_id].prev->index;
        // 进入ready_queue[core_id]结点的task出队列
        if(ready_queue[core_id].prev->prev != NULL){
            // 有三个及以上的ready_task
            ready_queue[core_id].prev = ready_queue[core_id].prev->prev;
            ready_queue[core_id].prev->next = &ready_queue[core_id];
        }
        else{
            // 刚好三个ready_task时，可能current_running被堵塞
            // 恢复到两个task的情况
            ready_queue[core_id].tail = &ready_queue[core_id];
            ready_queue[core_id].prev = NULL;
        }
    }

    if(prev_running->pid == core_id && current_running[core_id]->pid != core_id){
        // 重新设置pid0状态
        pcb[core_id].status = TASK_READY;
    }

    current_running[core_id]->status = TASK_RUNNING;
    current_running[core_id]->end_ticks = get_ticks() + RUNNING_TICKS;
    switch_to(prev_running, current_running[core_id]);    
    return;
}

void do_sleep(uint32_t sleep_time)
{
    // TODO: [p2-task3] sleep(seconds)
    // NOTE: you can assume: 1 second = 1 `timebase` ticks
    // 1. block the current_running
    // 2. set the wake up time for the blocked task
    // 3. reschedule because the current_running is blocked.
    int core_id = get_current_cpu_id();
    current_running[core_id]->wakeup_time = get_timer() + sleep_time;
    do_block(&(current_running[core_id]->list), &sleep_queue);
}                                   
                              
void do_block(list_node_t *pcb_node, list_head_t *queue)
{
    // TODO: [p2-task2] block the pcb task into the block queue
    put_at_tail(queue, pcb_node);
    pcb[pcb_node->index].status = TASK_BLOCKED;
    do_scheduler();
}
       
void do_unblock_head(list_head_t *queue, put_pos_t put_pos)
{   
    if(queue->index == -1){
        //如果队列为空，则相当于空操作
        return;
    }
    // 获得队头
    list_node_t *node_to_unblock = &(pcb[queue->index].list);
    // 队头出列
    queue_head_get_out(queue);
    // 设置状态为ready
    if(put_pos != DROP){
        pcb[node_to_unblock->index].status = TASK_READY;
    }
    // 进入等待队列
    int core_mask = pcb[node_to_unblock->index].core_mask;
    int put_on_which_core;
    if(core_mask != CORE_MASK_DEFAULT){
        put_on_which_core = core_mask;
    }
    else{
        put_on_which_core = allocate_core();
    }
    if(put_pos == PUT_AT_TAIL){
        put_at_tail(&ready_queue[put_on_which_core], node_to_unblock);
    }
    else if(put_pos == PUT_AT_HEAD){
        put_at_head(&ready_queue[put_on_which_core], node_to_unblock);
    }
}

void do_fork(void *func, long arg0, long arg1){
    /*
    创建新线程：
    1. 入口地址为函数地址
    2. 分配新的栈
    3. 参数在初始化栈时放在栈里, restore时被load进寄存器里，然后func从寄存器中拿取参数
    */
    ptr_t entry_point = (ptr_t)func;
    ptr_t kernel_stack_pcb = allocKernelPage(process_id);
    ptr_t user_stack_pcb = allocUserPage(process_id);

    // NOTE: 此处可能需要进一步修改
    int core_id = allocate_core();

    /* init pcb_stack start*/
    regs_context_t *pt_regs =
        (regs_context_t *)(kernel_stack_pcb - sizeof(regs_context_t));
    
    reg_t *reg_start1 = pt_regs->regs;
    reg_start1[0] = 0;//zero
    reg_start1[2] = user_stack_pcb;//sp
    reg_start1[10] = arg0;//a0
    reg_start1[11] = arg1;//a1

    pt_regs->sstatus = SR_SPIE;
    pt_regs->sepc = (reg_t)entry_point;
    
    switchto_context_t *pt_switchto =
        (switchto_context_t *)((ptr_t)pt_regs - sizeof(switchto_context_t));

    reg_t *reg_start2 = pt_switchto->regs;
    
    reg_start2[0] = (reg_t)ret_from_exception;//ra
    reg_start2[1] = (reg_t)pt_switchto;//sp
    /* init pcb_stack end*/

    /* init pcb start*/
    pcb[process_id].pid = process_id;

    pcb[process_id].kernel_sp = kernel_stack_pcb;
    pcb[process_id].user_sp = user_stack_pcb;

    pcb[process_id].list.index = process_id;

    if(process_id != 1){
        ready_queue[core_id].tail->prev = &(pcb[process_id].list);
        pcb[process_id].list.next = ready_queue[core_id].tail;
        pcb[process_id].list.prev = NULL;
        ready_queue[core_id].tail = &(pcb[process_id].list); 
    }

    pcb[process_id].status = TASK_READY;

    pcb[process_id].cursor_x = 0;
    pcb[process_id].cursor_y = 0;
    pcb[process_id].wakeup_time = 0;
    pcb[process_id].end_ticks = 0;
    /* init pcb end */
    process_id++;
}

void do_process_show(){
    printk("[Process Table]:\n");
    for(int i = 0; i < process_id; i++){
        printk("[%d] PID : %d", i, pcb[i].pid);
        // padding
        if(pcb[i].pid >= 10){
            printk(" ");
        }
        else{
            printk("  ");
        }
        printk("%s", pcb[i].name);

        // padding
        for(int j = 0; j < NAME_LEN_TO_PRINT- strlen(pcb[i].name); j++){
            printk(" ");
        }
        switch(pcb[i].status){
            case TASK_READY:
                printk("STATUS : READY  ");
                break;
            
            case TASK_BLOCKED:
                printk("STATUS : BLOCKED");
                break;

            case TASK_RUNNING:
                printk("STATUS : RUNNING");
                break;

            case TASK_EXITED:
                printk("STATUS : EXITED ");
                break;

            case TASK_ZOMBIE:
                printk("STATUS : ZOMBIE ");
                break;

            default:
                printk("STATUS : DEFAULT");
                break;
        }
        printk(" MASK = %d", pcb[i].core_mask);
        if(pcb[i].status == TASK_RUNNING){
            printk(" RUNNING ON CORE %d\n", pcb[i].running_on);
        }
        else{
            printk("\n");
        }
    }
}

static void init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point,
    pcb_t *pcb, int argc, char **argv_base)
{
    /* init context */
    regs_context_t *pt_regs =
        (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    
    reg_t *reg_start1 = pt_regs->regs;
    reg_start1[0] = 0;//zero
    reg_start1[2] = user_stack;//sp
    reg_start1[10] = (reg_t)argc;//a0
    reg_start1[11] = (reg_t)argv_base;//a1

    pt_regs->sstatus = SR_SPIE;
    pt_regs->sepc = (reg_t)entry_point;
    
    /* init 14 regs */
    switchto_context_t *pt_switchto =
        (switchto_context_t *)((ptr_t)pt_regs - sizeof(switchto_context_t));

    reg_t *reg_start2 = pt_switchto->regs;
    reg_start2[0] = (reg_t)ret_from_exception;//ra
    reg_start2[1] = (reg_t)pt_switchto;//sp
}

static int init_pcb(task_info_t *task_to_exec, int mask, int argc, char *argv[])
{
    // PCB数量已达到上限
    if(process_id == NUM_MAX_TASK){
        return 0;
    }
    
    pid_t task_to_exec_pid;
    ptr_t kernel_stack_pcb = allocKernelPage(process_id);
    ptr_t user_stack_base = allocUserPage(process_id);
    ptr_t user_stack_pcb = ALIGNED_128(user_stack_base - argc * ARG_POINTER_SIZE - argc * ARGS_MAX_LEN);

    ptr_t argv_base = user_stack_base - argc * ARG_POINTER_SIZE;
    list_node_t *init_pcb_node;//当前初始化的pcb
    list_node_t *next_pcb_node;//当前ready_queue[core_id]的队头pcb

    // NOTE: 此处设置put_on_which_core恒为0则可模拟单核运行
    int put_on_which_core;
    if(mask != CORE_MASK_DEFAULT){
        put_on_which_core = mask;
    }
    else{
        put_on_which_core = allocate_core();
    }

    // put_on_which_core = 0;

    for(int i = 0; i < argc; i++){
        *(uint64_t *)(argv_base + i * ARG_POINTER_SIZE) = user_stack_pcb + i * ARGS_MAX_LEN;
        memcpy((uint8_t *)(user_stack_pcb + i * ARGS_MAX_LEN), (uint8_t *)(argv[i]), ARGS_MAX_LEN);
    }
    
    init_pcb_stack(kernel_stack_pcb, user_stack_pcb, task_to_exec->task_entry, &(pcb[process_id]), argc, (char **)argv_base);
    
    pcb[process_id].pid = process_id;
    
    task_to_exec_pid = pcb[process_id].pid;
    strcpy(pcb[process_id].name, task_to_exec->task_name);

    pcb[process_id].kernel_sp = kernel_stack_pcb;
    pcb[process_id].user_sp = user_stack_pcb;
    pcb[process_id].user_stack_base = user_stack_base;

    pcb[process_id].list.index = process_id;

    LIST_INIT(pcb[process_id].wait_list);
    pcb[process_id].waiting_num = 0;

    pcb[process_id].status = TASK_READY;

    pcb[process_id].locks.hold_lock_num = 0;

    pcb[process_id].cursor_x = 0;
    pcb[process_id].cursor_y = 0;
    pcb[process_id].wakeup_time = 0;
    pcb[process_id].end_ticks = 0;

    pcb[process_id].core_mask = mask;
    pcb[process_id].running_on = put_on_which_core;

    // 将其放在准备队列的队首，当前队首放在其之后。
    init_pcb_node = &(pcb[process_id].list);
    put_at_head(&ready_queue[put_on_which_core], init_pcb_node);
    
    process_id++;
    return task_to_exec_pid;
}

pid_t do_exec(char *name, int argc, char *argv[]){
    task_info_t *task_to_exec;
    pid_t task_to_exec_pid;
    int success_flag = 0;
    //printk("\nshed:\n");
    //printk("task name = %s\n", name);
    for(int i = 0; i < TASK_MAXNUM; i++){
        if(strcmp(task_info[i].task_name, name) == 0){
            task_to_exec = &(task_info[i]);
            /*
            // TEST EXEC
            printk("%s", task_to_exec->task_name);
            return i;
            printk("matched! task name = %s\n", task_to_exec->task_name);
            */
            success_flag = 1;
            break;
        }
    }
    if(success_flag == 0){
        return 0;
    }
    else{
        task_to_exec_pid = init_pcb(task_to_exec, CORE_MASK_DEFAULT, argc, argv);
        return task_to_exec_pid;
    }
    /*
    printk("task name = %s\n", task_to_exec->task_name);
    // TEST ARGS
    printk("arg number = %d:\n", argc);
    for(int i = 0; i < argc; i++){
        printk("arg[%d]: %s\n", i, argv[i]);
    }
    */
}

int do_waitpid(pid_t pid){
    int core_id = get_current_cpu_id();
    pcb_t *task_to_wait;
    list_node_t node_copy = {
        .index = current_running[core_id]->list.index,
        .next = NULL,
        .prev = NULL
    };
    /* 暂时使用这种判断方式，后续考虑复用pcb数组 */
    if(pid >= NUM_MAX_TASK){
        return 0;
    }
    else{
        task_to_wait = &pcb[pid];
        if(current_running[core_id]->waiting_num == 0){
            current_running[core_id]->waiting_num++;
            do_block(&(current_running[core_id]->list), &(task_to_wait->wait_list));
        }
        else{
            current_running[core_id]->waiting_num++;
            do_block(&node_copy, &(task_to_wait->wait_list));
        }
        return pid;
    }
}

void do_exit(void){
    int core_id = get_current_cpu_id();
    pcb_t *task_to_exit = current_running[core_id];
    pcb_t *task_to_wake_up;
    /* 唤醒等待本进程的进程 */
    while(task_to_exit->wait_list.index != -1){
        task_to_wake_up = &(pcb[task_to_exit->wait_list.index]);
        task_to_wake_up->waiting_num--;
        if(task_to_wake_up->waiting_num == 0){
            do_unblock_head(&(task_to_exit->wait_list), PUT_AT_HEAD);
        }
        else{
            do_unblock_head(&(task_to_exit->wait_list), DROP);
        }
    }

    /* 释放持有的锁 
    for(int i = 0; i < task_to_exit->locks.hold_lock_num; i++){
        do_mutex_lock_release(task_to_exit->locks.hold_lock_idx[i]);
    }
    */
   
    // 进程在do_scheduler时被移出ready队列
    task_to_exit->status = TASK_EXITED;
    do_scheduler();
}

int do_kill(pid_t pid){
    pcb_t *task_to_kill;
    pcb_t *task_to_wake_up;
    /* 暂时使用这种判断方式，后续考虑复用pcb数组 */
    if(pid >= NUM_MAX_TASK){
        return 0;
    }
    else{
        task_to_kill = &pcb[pid];
        /* 唤醒等待本进程的进程 */
        while(task_to_kill->wait_list.index != -1){
            task_to_wake_up = &(pcb[task_to_kill->wait_list.index]);
            task_to_wake_up->waiting_num--;
            if(task_to_wake_up->waiting_num == 0){
                do_unblock_head(&(task_to_kill->wait_list), PUT_AT_HEAD);
            }
            else{
                do_unblock_head(&(task_to_kill->wait_list), DROP);
            }
        }
        
        /* 释放持有的锁 */
        for(int i = 0; i < task_to_kill->locks.hold_lock_num; i++){
            do_mutex_lock_release(task_to_kill->locks.hold_lock_idx[i]);
        }

        task_to_kill->status = TASK_ZOMBIE;
        return 1;
    }
}

pid_t do_getpid(){
    int core_id = get_current_cpu_id();
    return current_running[core_id]->pid;
}

void queue_head_get_out(list_head_t *queue){
    if(queue->prev != NULL){
        queue->index = queue->prev->index;
        if(queue->prev->prev != NULL){
            queue->prev->prev->next = queue;
            queue->prev = queue->prev->prev;
        }
        else{
            queue->tail = queue;
            queue->prev = NULL;
        }
    }
    else{
        set_queue_empty(queue);
        // TODO: 考虑泛化置空队列的方法 
    }
}

void put_at_head(list_head_t *queue, list_node_t *pcb_node){
    int core_id;
    list_node_t *next_pcb_node;
    //将pcb_node放在queue的队头
    //将queue原本的队头放在pcb_node的后面一位
    if(queue_is_empty(queue)){
        if(IS_READY_QUEUE(queue)){
            core_id = (queue == &(ready_queue[1]));
            pcb[pcb_node->index].running_on = core_id;
        }
        queue->index = pcb_node->index;
        return;
    }
    else{
        next_pcb_node = &(pcb[queue->index].list);
        if(IS_READY_QUEUE(queue)){    
            core_id = (queue == &(ready_queue[1]));
            pcb[pcb_node->index].running_on = core_id;
            if(queue->prev != NULL){
                // 有三个及以上task
                queue->prev->next = next_pcb_node;
                next_pcb_node->prev = queue->prev;
                queue->prev = next_pcb_node;
                next_pcb_node->next = queue;
            }
            else if(current_running[core_id]->list.index != next_pcb_node->index){
                // 仅有两个task
                next_pcb_node->next = queue;
                next_pcb_node->prev = NULL;
                queue->prev = next_pcb_node;
                queue->tail = next_pcb_node;
            }
        }
        else{
            if(queue->prev != NULL){
                queue->prev->next = next_pcb_node;
                next_pcb_node->prev = queue->prev;
                queue->prev = next_pcb_node;
                next_pcb_node->next = queue;
            }
            else{
                next_pcb_node->next = queue;
                next_pcb_node->prev = NULL;
                queue->prev = next_pcb_node;
                queue->tail = next_pcb_node;
            }
        }
        queue->index = pcb_node->index;
        return;
    }
}

void put_at_tail(list_head_t *queue, list_node_t *pcb_node){
    int core_id;
    //将pcb_node放在queue的队尾
    if(queue_is_empty(queue)){
        if(IS_READY_QUEUE(queue)){
            core_id = (queue == &(ready_queue[1]));
            pcb[pcb_node->index].running_on = core_id;
        }
        queue->index = pcb_node->index;
        return;
    }
    else{
        if(IS_READY_QUEUE(queue)){
            core_id = (queue == &(ready_queue[1]));
            pcb[pcb_node->index].running_on = core_id;
            if(current_running[core_id]->list.index != queue->index){
                // 两个task
                queue->tail->prev = pcb_node;
                pcb_node->next = queue->tail;
                pcb_node->prev = NULL;
                queue->tail = pcb_node;
            }
            else{
                // 一个task
                queue->index = pcb_node->index;
            }
        }
        else{
            queue->tail->prev = pcb_node;
            pcb_node->next = queue->tail;
            pcb_node->prev = NULL;
            queue->tail = pcb_node;
        }
        return;
    }
}

int queue_is_empty(list_head_t *queue){
    if(queue == &ready_queue[0] && ready_queue[0].index == 0){
        return 1;
    }
    else if(queue == &ready_queue[1] && ready_queue[1].index == 1){
        return 1;
    }
    else if(queue->index == -1){
        return 1;
    }
    else{
        return 0;
    }
}

void set_queue_empty(list_head_t *queue){
    if(queue == &ready_queue[0]){
        queue->index = 0;
    }
    else if(queue == &ready_queue[1]){
        queue->index = 1;
    }
    else{
        queue->index = -1;
    }
}

void do_set_mask(int mask, pid_t pid){
    pcb[pid].core_mask = mask;
    //printk("pid = %d, mask = %d\n", pid, mask);
}

pid_t do_task_set(char *name, int mask, int argc, char *argv[]){
    task_info_t *task_to_exec;
    pid_t task_to_exec_pid;
    int success_flag = 0;
    //printk("\nshed:\n");
    //printk("task name = %s\n", name);
    for(int i = 0; i < TASK_MAXNUM; i++){
        if(strcmp(task_info[i].task_name, name) == 0){
            task_to_exec = &(task_info[i]);
            /*
            // TEST EXEC
            printk("%s", task_to_exec->task_name);
            return i;
            printk("matched! task name = %s\n", task_to_exec->task_name);
            */
            success_flag = 1;
            break;
        }
    }
    if(success_flag == 0){
        return 0;
    }
    else{
        task_to_exec_pid = init_pcb(task_to_exec, mask, argc, argv);
        return task_to_exec_pid;
    }
    /*
    printk("task name = %s\n", task_to_exec->task_name);
    printk("mask = %d\n", mask);
    // TEST ARGS
    printk("arg number = %d:\n", argc);
    for(int i = 0; i < argc; i++){
        printk("arg[%d]: %s\n", i, argv[i]);
    }
    */
}

int allocate_core(){
    if(ready_queue[1].index == 0){
        return 1;
    }
    else if(ready_queue[0].index == 0){
        return 0;
    }
    else{
        return core_task_count++ % 2;
    }
}
