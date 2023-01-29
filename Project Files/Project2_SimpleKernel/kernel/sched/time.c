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
    list_node_t *next_pcb_node;//当前ready_queue的队头pcb
    uint64_t current_time;
    uint64_t wakeup_time;

    if(sleep_queue.index != -1){
        pcb_node = &(pcb[sleep_queue.index].list);
        next_pcb_node = &(pcb[ready_queue.index].list);
        current_time = get_timer();
        wakeup_time = pcb[sleep_queue.index].wakeup_time;
        if(wakeup_time < current_time){
            if(sleep_queue.prev != NULL){
                sleep_queue.index = sleep_queue.prev->index;
                if(sleep_queue.prev->prev != NULL){
                    sleep_queue.prev->prev->next = &sleep_queue;
                    sleep_queue.prev = sleep_queue.prev->prev;
                }
                else{
                    sleep_queue_tail = &sleep_queue;
                    sleep_queue.prev = NULL;
                }
            }
            else{
                sleep_queue.index = -1;
            }
            pcb[pcb_node->index].status = TASK_READY;
            //将唤醒的pcb放在ready_queue的队头
            ready_queue.index = pcb_node->index;
            //将原本的队头放在被唤醒pcb的后面一位
            ready_queue.prev->next = next_pcb_node;
            next_pcb_node->prev = ready_queue.prev;
            ready_queue.prev = next_pcb_node;
            next_pcb_node->next = &ready_queue;
        }
    }
}