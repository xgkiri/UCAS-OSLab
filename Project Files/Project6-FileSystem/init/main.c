/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *         The kernel's entry, where most of the initialization work is done.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#include <common.h>
#include <asm.h>

#include <asm/unistd.h>
//#include <arch/riscv/include/asm/unistd.h>

#include <os/loader.h>
#include <os/irq.h>
#include <os/sched.h>
#include <os/lock.h>
#include <os/kernel.h>
#include <os/task.h>
#include <os/string.h>
#include <os/mm.h>
#include <os/time.h>
#include <os/smp.h>
#include <os/ioremap.h>
#include <os/net.h>
#include <os/fs.h>

#include <sys/syscall.h>
//#include <include/sys/syscall.h>

#include <screen.h>
#include <printk.h>
#include <assert.h>
#include <type.h>
#include <csr.h>
#include <e1000.h>

extern void ret_from_exception();

int task_num;
task_info_t task_info[TASK_MAXNUM];

static void init_jmptab(void)
{
    volatile long (*(*jmptab))() = (volatile long (*(*))())KERNEL_JMPTAB_BASE;

    jmptab[CONSOLE_PUTSTR]  = (long (*)())port_write;
    jmptab[CONSOLE_PUTCHAR] = (long (*)())port_write_ch;
    jmptab[CONSOLE_GETCHAR] = (long (*)())port_read_ch;
    jmptab[SD_READ]         = (long (*)())sd_read;
    jmptab[SD_WRITE]        = (long (*)())sd_write;
    jmptab[QEMU_LOGGING]    = (long (*)())qemu_logging;
    jmptab[SET_TIMER]       = (long (*)())set_timer;
    jmptab[READ_FDT]        = (long (*)())read_fdt;
    jmptab[MOVE_CURSOR]     = (long (*)())screen_move_cursor;
    jmptab[PRINT]           = (long (*)())printk;
    jmptab[YIELD]           = (long (*)())do_scheduler;
    jmptab[MUTEX_INIT]      = (long (*)())do_mutex_lock_init;
    jmptab[MUTEX_ACQ]       = (long (*)())do_mutex_lock_acquire;
    jmptab[MUTEX_RELEASE]   = (long (*)())do_mutex_lock_release;
}

static void init_task_info(void)
{
    // TODO: [p1-task4] Init 'tasks' array via reading app-info sector
    // NOTE: You need to get some related arguments from bootblock first
    get_task_info(task_info);
}

static void init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point,
    pcb_t *pcb)
{
    /* context */
    regs_context_t *pt_regs =
        (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    
    reg_t *reg_start1 = pt_regs->regs;
    reg_start1[0] = 0;//zero
    reg_start1[2] = user_stack;//sp 
    pt_regs->sstatus = SR_SPIE | SR_SUM;
    pt_regs->sepc = (reg_t)entry_point;
    /* 14regs */
    switchto_context_t *pt_switchto =
        (switchto_context_t *)((ptr_t)pt_regs - sizeof(switchto_context_t));

    reg_t *reg_start2 = pt_switchto->regs;
    reg_start2[0] = (reg_t)ret_from_exception;//ra
    reg_start2[1] = (reg_t)pt_switchto;//sp
}


static void init_pcb(void)
{
    task_info_t *task_to_exec;
    uintptr_t pgdir_va;

    /* TODO: [p2-task1] load needed tasks and init their corresponding PCB */
    pcb[0] = pid0_pcb_core0;
    pcb[1] = pid0_pcb_core1;
    /* TODO: [p2-task1] remember to initialize 'current_running' */
    current_running[0] = &pcb[0];
    current_running[1] = &pcb[1];
    
    for(int i = 0; i < TASK_MAXNUM; i++){
        if(strcmp(task_info[i].task_name, "shell") == 0){
            task_to_exec = &(task_info[i]);
            break;
        }
    }
    
    assert(process_id == 2);
    assert(strcmp(task_to_exec->task_name, "shell") == 0);

    pgdir_va = load_task(task_to_exec);

    ptr_t kernel_stack_pcb = pa2kva(alloc_one_page_kernel()); // 使用内核页表中的虚地址
    ptr_t user_stack_pcb = alloc_page_helper(USER_STACK_VA - PAGE_SIZE, pgdir_va); // 为用户栈分配一页
    // TODO: 使用分配的虚拟地址入口
    init_pcb_stack(kernel_stack_pcb, USER_STACK_VA, USER_VA_BASE, &(pcb[process_id]));
    
    pcb[process_id].pid = process_id;
    
    strcpy(pcb[process_id].name, task_to_exec->task_name);

    pcb[process_id].kernel_sp = kernel_stack_pcb;
    pcb[process_id].current_kernel_sp = kernel_stack_pcb - sizeof(regs_context_t) - sizeof(switchto_context_t);
    pcb[process_id].user_sp = user_stack_pcb;
    
    pcb[process_id].list.index = process_id;

    LIST_INIT(pcb[process_id].wait_list);
    pcb[process_id].waiting_num = 0;
    pcb[process_id].pgdir = pgdir_va;
    pcb[process_id].status = TASK_READY;
    pcb[process_id].locks.hold_lock_num = 0;

    pcb[process_id].cursor_x = 0;
    pcb[process_id].cursor_y = 0;
    pcb[process_id].wakeup_time = 0;
    pcb[process_id].end_ticks = 0;

    pcb[process_id].core_mask = 0; // NOTE: shell绑定主核
    pcb[process_id].running_on = 0;
    pcb[process_id].task_type = PROCESS;
    process_id++;
}

static void init_syscall(void)
{
    // TODO: [p2-task3] initialize system call table.
    syscall[SYSCALL_WRITE]          = (long (*)())screen_write;
    syscall[SYSCALL_CURSOR]         = (long (*)())screen_move_cursor;
    syscall[SYSCALL_REFLUSH]        = (long (*)())screen_reflush;
    syscall[SYSCALL_YIELD]          = (long (*)())do_scheduler;
    syscall[SYSCALL_LOCK_INIT]      = (long (*)())do_mutex_lock_init;
    syscall[SYSCALL_LOCK_ACQ]       = (long (*)())do_mutex_lock_acquire;
    syscall[SYSCALL_LOCK_RELEASE]   = (long (*)())do_mutex_lock_release;
    syscall[SYSCALL_SLEEP]          = (long (*)())do_sleep;
    syscall[SYSCALL_GET_TICK]       = (long (*)())get_ticks;
    syscall[SYSCALL_GET_TIMEBASE]   = (long (*)())get_time_base;
    syscall[SYSCALL_FORK]           = (long (*)())do_fork;
    syscall[SYSCALL_SCREEN_CLEAN]   = (long (*)())screen_clear;
    syscall[SYSCALL_GETCHAR]        = (long (*)())bios_getchar;
    syscall[SYSCALL_PS]             = (long (*)())do_process_show;
    syscall[SYSCALL_EXEC]           = (long (*)())do_exec;
    syscall[SYSCALL_WAITPID]        = (long (*)())do_waitpid;
    syscall[SYSCALL_EXIT]           = (long (*)())do_exit;
    syscall[SYSCALL_KILL]           = (long (*)())do_kill;
    syscall[SYSCALL_GETPID]         = (long (*)())do_getpid;
    syscall[SYSCALL_BARR_INIT]      = (long (*)())do_barrier_init;
    syscall[SYSCALL_BARR_WAIT]      = (long (*)())do_barrier_wait;
    syscall[SYSCALL_BARR_DESTROY]   = (long (*)())do_barrier_destroy;
    syscall[SYSCALL_COND_INIT]      = (long (*)())do_condition_init;
    syscall[SYSCALL_COND_WAIT]      = (long (*)())do_condition_wait;
    syscall[SYSCALL_COND_SIGNAL]    = (long (*)())do_condition_signal;
    syscall[SYSCALL_COND_BROADCAST] = (long (*)())do_condition_broadcast;
    syscall[SYSCALL_COND_DESTROY]   = (long (*)())do_condition_destroy;
    syscall[SYSCALL_MBOX_OPEN]      = (long (*)())do_mbox_open;
    syscall[SYSCALL_MBOX_CLOSE]     = (long (*)())do_mbox_close;
    syscall[SYSCALL_MBOX_SEND]      = (long (*)())do_mbox_send;
    syscall[SYSCALL_MBOX_RECV]      = (long (*)())do_mbox_recv;
    syscall[SYSCALL_GET_CORE_ID]    = (long (*)())get_current_cpu_id;
    syscall[SYSCALL_SET_MASK]       = (long (*)())do_set_mask;
    syscall[SYSCALL_SET_TASK]       = (long (*)())do_task_set;
    syscall[SYSCALL_SHM_GET]        = (long (*)())shm_page_get;
    syscall[SYSCALL_SHM_DT]         = (long (*)())shm_page_dt;
    syscall[SYSCALL_PTHREAD_CREATE] = (long (*)())do_pthread_create;
    syscall[SYSCALL_CREATE_SP]      = (long (*)())do_create_sp;
    syscall[SYSCALL_GET_PA]         = (long (*)())do_get_pa;
    syscall[SYSCALL_NET_SEND]       = (long (*)())do_net_send;
    syscall[SYSCALL_NET_RECV]       = (long (*)())do_net_recv;
    syscall[SYSCALL_FS_MKFS]        = (long (*)())do_mkfs;
    syscall[SYSCALL_FS_STATFS]      = (long (*)())do_statfs;
    syscall[SYSCALL_FS_CD]          = (long (*)())do_cd;
    syscall[SYSCALL_FS_MKDIR]       = (long (*)())do_mkdir;
    syscall[SYSCALL_FS_RMDIR]       = (long (*)())do_rmdir;
    syscall[SYSCALL_FS_LS]          = (long (*)())do_ls;
    syscall[SYSCALL_FS_TOUCH]       = (long (*)())do_touch;
    syscall[SYSCALL_FS_CAT]         = (long (*)())do_cat;
    syscall[SYSCALL_FS_FOPEN]       = (long (*)())do_fopen;
    syscall[SYSCALL_FS_FREAD]       = (long (*)())do_fread;
    syscall[SYSCALL_FS_FWRITE]      = (long (*)())do_fwrite;
    syscall[SYSCALL_FS_FCLOSE]      = (long (*)())do_fclose;
    syscall[SYSCALL_FS_LN]          = (long (*)())do_ln;
    syscall[SYSCALL_FS_RM]          = (long (*)())do_rm;
    syscall[SYSCALL_FS_LSEEK]       = (long (*)())do_lseek;
}

int main(void)
{   
    /* 从核main函数执行部分 */
    if(get_current_cpu_id() == 1){
        printk("\nCore_1 is ready...\n");
        // 重新设置中断入口，打开中断
        setup_exception();
        // 设置时钟
        bios_set_timer(get_ticks() + 2 * TIMER_INTERVAL);
        // 从核在此处等待调度，不再执行之后的初始化操作
        asm volatile(
            "add tp, %[pid0], zero\n\t"
            :
            : [pid0] "r"(pointer_to_pid0[1])
        );
        while(1){
            enable_preempt();
            asm volatile("wfi");
        }
    }

    /* 主核main函数执行部分 */

    // Init jump table provided by kernel and bios(ΦωΦ)
    init_jmptab();

    // Init task information (〃'▽'〃)
    init_task_info();

    // 取消映射
    /*
    for (uint64_t pa = 0x50000000lu; pa < 0x51000000lu; pa += NORMAL_PAGE_SIZE) {
        delete_map(pa, KERNEL_PGDIR_VA);
    }
    */

    // Read Flatten Device Tree (｡•ᴗ-)_
    time_base = bios_read_fdt(TIMEBASE);
    e1000 = (volatile uint8_t *)bios_read_fdt(EHTERNET_ADDR);
    uint64_t plic_addr = bios_read_fdt(PLIC_ADDR);
    uint32_t nr_irqs = (uint32_t)bios_read_fdt(NR_IRQS);
    //printk("> [INIT] e1000: 0x%lx, plic_addr: 0x%lx, nr_irqs: 0x%lx.\n", e1000, plic_addr, nr_irqs);

    // IOremap
    plic_addr = (uintptr_t)ioremap((uint64_t)plic_addr, 0x4000 * NORMAL_PAGE_SIZE);
    //int test_id = plic_claim();
    //plic_complete(test_id);

    e1000 = (uint8_t *)ioremap((uint64_t)e1000, 8 * NORMAL_PAGE_SIZE);

    // Init Process Control Blocks |•'-'•) ✧
    init_pcb();
    // now do it in syscall exec

    //printl("e1000_paddr = 0x%lx\n", get_pa_of(e1000, KERNEL_PGDIR_VA));
    printk("> [INIT] IOremap initialization succeeded.\n");

    printk("> [INIT] PCB initialization succeeded.\n");

    // Init lock mechanism o(´^｀)o
    init_locks();

    init_barriers();

    init_conditions();

    init_mbox();

    smp_init();

    init_sh_pg();

    printk("> [INIT] Lock mechanism initialization succeeded.\n");

    // Init interrupt (^_^)
    printk("> [INIT] Interrupt processing initialization succeeded.\n");

    /*
    // TODO: [p5-task4] Init plic
    plic_init(plic_addr, nr_irqs);
    printk("> [INIT] PLIC initialized successfully. addr = 0x%lx, nr_irqs=0x%x\n", plic_addr, nr_irqs);

    // Init network device
    e1000_init();
    printk("> [INIT] E1000 device initialized successfully.\n");
    */

    // Init system call table (0_0)
    init_syscall();
    printk("> [INIT] System call initialized successfully.\n");

    // Init screen (QAQ)
    init_screen();
    printk("> [INIT] SCREEN initialization succeeded.\n");
    
    init_fs();
    printk("> [INIT] File System initialized successfully.\n");

    // TODO: [p2-task4] Setup timer interrupt and enable all interrupt globally
    // NOTE: The function of sstatus.sie is different from sie's

    /* 主核完成其余初始化操作
     * 先发送核间中断唤醒从核，再初始化例外
     */

    //wakeup_other_hart();
    printk("\nCore_0 is ready...\n");
    // 主核等待调度
    bios_set_timer(get_ticks() + TIMER_INTERVAL);
    asm volatile(
        "add tp, %[pid0], zero\n\t"
        :
        : [pid0] "r"(pointer_to_pid0[0])
    );
    init_exception();
    while(1){
        enable_preempt();
        asm volatile("wfi");
    }
    /*
    unsigned mem_address = task_entry[1];
    ((void(*)(void))mem_address)();
    */
    // Infinite while loop, where CPU stays in a low-power state (QAQQQQQQQQQQQ)
    /*
    while (1)
    {
        // If you do non-preemptive scheduling, it's used to surrender control
        //do_scheduler();

        // If you do preemptive scheduling, they're used to enable CSR_SIE and wfi
        enable_preempt();
        asm volatile("wfi");
    }
    */
    return 0;
}
