#include <os/list.h>
#include <os/sched.h>
#include <type.h>

uint64_t time_elapsed = 0;
uint64_t time_base = 0;

uint64_t get_ticks()
{
    __asm__ __volatile__(
        "rdtime %0"
        : "=r"(time_elapsed));
    return time_elapsed;
}

uint64_t get_timer()
{
    return get_ticks() / time_base;
}

uint64_t get_time_base()
{
    return time_base;
}

void latency(uint64_t time)
{
    uint64_t begin_time = get_timer();

    while (get_timer() - begin_time < time);
    return;
}

void check_sleeping(void)
{
    // TODO: [p2-task3] Pick out tasks that should wake up from the sleep queue
    list_node_t *pcb_node;//被唤醒的pcb
    uint64_t current_time;
    uint64_t wakeup_time;

    // TODO: 完善唤醒机制，每次将所有可唤醒的进程全部唤醒
    if(sleep_queue.index != -1){
        pcb_node = &(pcb[sleep_queue.index].list);
        current_time = get_timer();
        wakeup_time = pcb[sleep_queue.index].wakeup_time;
        if(wakeup_time < current_time){
            do_unblock_head(&sleep_queue, PUT_AT_HEAD);
        }
    }
}