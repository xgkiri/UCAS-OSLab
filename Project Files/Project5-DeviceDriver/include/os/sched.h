/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *        Process scheduling related content, such as: scheduler, process blocking,
 *                 process wakeup, process creation, process kill, etc.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
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

#ifndef INCLUDE_SCHEDULER_H_
#define INCLUDE_SCHEDULER_H_

#include <type.h>
#include <os/list.h>
#include <os/task.h>
#include <os/string.h>

#define NUM_MAX_TASK 32
#define RUNNING_TICKS 450000
#define NAME_LEN 24
#define CORE_NUM 2
#define CORE_MASK_DEFAULT 2 
#define MAX_HOLD_LOCK 8

/* used to save register infomation */
typedef struct regs_context
{
    /* Saved main processor registers.*/
    reg_t regs[32];

    /* Saved special registers. */
    reg_t sstatus;
    reg_t sepc;
    reg_t sbadaddr;
    reg_t scause;
} regs_context_t;

/* used to save register infomation in switch_to */
typedef struct switchto_context
{
    /* Callee saved registers.*/
    /* 14regs: ra, sp, s0 to s11 */
    reg_t regs[14];
} switchto_context_t;

typedef enum {
    TASK_BLOCKED,
    TASK_RUNNING,
    TASK_READY,
    TASK_EXITED,
    TASK_ZOMBIE,
} task_status_t;

typedef enum {
    PUT_AT_HEAD,
    PUT_AT_TAIL,
    DROP,
} put_pos_t;

typedef enum {
    PROCESS,
    THREAD
} task_type_t;

typedef struct hold_lock {
    int hold_lock_num;
    int hold_lock_idx[MAX_HOLD_LOCK];
} hold_lock_t;

/* Process Control Block */
typedef struct pcb
{
    /* register context */
    // NOTE: this order must be preserved, which is defined in regs.h!!
    reg_t kernel_sp;   
    reg_t user_sp;
    ptr_t current_kernel_sp;
    ptr_t kernel_stack_base;
    ptr_t user_stack_base;

    char name[NAME_LEN];
    /* previous, next pointer */
    list_node_t list;

    list_head_t wait_list;
    int waiting_num;
    /* process id */
    pid_t pid;

    /* BLOCK | READY | RUNNING */
    task_status_t status;

    hold_lock_t locks;

    /* pgdir */
    uintptr_t pgdir;

    /* cursor position */
    int cursor_x;
    int cursor_y;

    /* time(seconds) to wake up sleeping PCB */
    uint64_t wakeup_time;
    uint64_t end_ticks;

    /* core mask */
    int core_mask;
    int running_on;

    task_type_t task_type;
    int thread_num;
} pcb_t;

/* the head of ready_queue: a node */
extern list_head_t ready_queue[CORE_NUM];

/* sleep queue to be blocked in */
extern list_head_t sleep_queue;

extern list_head_t net_send_queue;
extern list_head_t net_recv_queue;

/* current running task PCB */
extern pcb_t * volatile current_running[CORE_NUM];

extern pcb_t * volatile pointer_to_pid0[CORE_NUM];

extern int core_task_count;

extern pid_t process_id;

extern int core_id;

extern pcb_t pcb[NUM_MAX_TASK];
extern pcb_t pid0_pcb_core0;
extern pcb_t pid0_pcb_core1;
extern const ptr_t pid0_stack_core0;
extern const ptr_t pid0_stack_core1;

extern void switch_to(pcb_t *prev, pcb_t *next);
void do_scheduler(void);
void do_sleep(uint32_t);

void do_block(list_node_t *pcb_node, list_head_t *queue);
void do_unblock_head(list_head_t *queue, put_pos_t put_pos); // 将指定队列的队头出列，状态设置为ready，放到ready_queue的指定位置

void do_fork(void *func, long arg0, long arg1);

void queue_head_get_out(list_head_t *queue);
void put_at_head(list_head_t *queue, list_node_t *pcb_node);
void put_at_tail(list_head_t *queue, list_node_t *pcb_node);
int queue_is_empty(list_head_t *queue);

/* TODO [P3-TASK1] exec exit kill waitpid ps */
#ifdef S_CORE
extern pid_t do_exec(int id, int argc, uint64_t arg0, uint64_t arg1, uint64_t arg2);
#else
extern pid_t do_exec(char *name, int argc, char *argv[]);
#endif

extern task_info_t task_info[TASK_MAXNUM];

extern void do_exit(void);
extern int do_kill(pid_t pid);
extern int do_waitpid(pid_t pid);
extern void do_process_show();
extern pid_t do_getpid();

extern void do_set_mask(int mask, pid_t pid);
extern pid_t do_task_set(char *name, int mask, int argc, char *argv[]);

int allocate_core();

void do_pthread_create(ptr_t *thread, void (*start_routine)(void*), void *arg);

void check_net_send();
void check_net_recv();
#endif
