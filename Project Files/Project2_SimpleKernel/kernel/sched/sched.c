#include <os/list.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/mm.h>
#include <screen.h>
#include <printk.h>
#include <assert.h>

#define SWITCH_TO_SIZE   112
#define SR_SPIE   0x00000020 /* Previous Supervisor IE */

extern void ret_from_exception();

pcb_t pcb[NUM_MAX_TASK];
const ptr_t pid0_stack = INIT_KERNEL_STACK + SWITCH_TO_SIZE;
pcb_t pid0_pcb = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack,
    .user_sp = (ptr_t)pid0_stack,
    .end_ticks = 0
};

LIST_HEAD(ready_queue);
LIST_HEAD(sleep_queue);

/* current running task PCB */
pcb_t * volatile current_running;
/* previous running task PCB */
pcb_t * volatile prev_running;

/* global process id */
pid_t process_id = 1;

void do_scheduler(void)
{
    // TODO: [p2-task3] Check sleep queue to wake up PCBs
    // check_sleeping();

    // TODO: [p2-task1] Modify the current_running pointer.
    prev_running = current_running;
    current_running = &(pcb[ready_queue.index]);

    /* put previous task at the tail of ready queue despite of pid0 */
    if(prev_running->pid != 0 & prev_running->status == TASK_RUNNING){
        prev_running->status = TASK_READY;
        ready_queue_tail->prev = &(prev_running->list);
        prev_running->list.next = ready_queue_tail;
        prev_running->list.prev = NULL;
        ready_queue_tail = &(prev_running->list);
    }

    /* let ready_queue store the index of next ready task */
    ready_queue.index = ready_queue.prev->index;
    
    /* new running task get out of ready queue */
    ready_queue.prev = ready_queue.prev->prev;
    ready_queue.prev->next = &ready_queue;

    current_running->status = TASK_RUNNING;
    current_running->end_ticks = get_ticks() + RUNNING_TICKS;
    // TODO: [p2-task1] switch_to current_running
    switch_to(prev_running, current_running);    
}

void do_sleep(uint32_t sleep_time)
{
    // TODO: [p2-task3] sleep(seconds)
    // NOTE: you can assume: 1 second = 1 `timebase` ticks
    // 1. block the current_running
    // 2. set the wake up time for the blocked task
    // 3. reschedule because the current_running is blocked.
    current_running->wakeup_time = get_timer() + sleep_time;
    do_block(&(current_running->list), &sleep_queue, sleep_queue_tail);
}                                   
                              
void do_block(list_node_t *pcb_node, list_head *queue, list_node_t *queue_tail)
{
    // TODO: [p2-task2] block the pcb task into the block queue
    if(queue->index == -1){
        queue->index = pcb_node->index;
    }
    else{
        queue_tail->prev = pcb_node;
        pcb_node->next = queue_tail;
        pcb_node->prev = NULL;
        queue_tail = pcb_node;
    }
    pcb[pcb_node->index].status = TASK_BLOCKED;
    do_scheduler();
}
       
void do_unblock(list_node_t *pcb_node)
{
    // TODO: [p2-task2] unblock the `pcb` from the block queue
    pcb[pcb_node->index].status = TASK_READY;
    ready_queue_tail->prev = pcb_node;
    pcb_node->next = ready_queue_tail;
    pcb_node->prev = NULL;
    ready_queue_tail = pcb_node;
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
        ready_queue_tail->prev = &(pcb[process_id].list);
        pcb[process_id].list.next = ready_queue_tail;
        pcb[process_id].list.prev = NULL;
        ready_queue_tail = &(pcb[process_id].list); 
    }

    pcb[process_id].status = TASK_READY;

    pcb[process_id].cursor_x = 0;
    pcb[process_id].cursor_y = 0;
    pcb[process_id].wakeup_time = 0;
    pcb[process_id].end_ticks = 0;
    /* init pcb end */
    process_id++;
}