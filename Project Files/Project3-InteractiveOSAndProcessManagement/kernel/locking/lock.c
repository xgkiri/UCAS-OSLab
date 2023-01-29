#include <os/lock.h>
#include <os/sched.h>
#include <os/list.h>
#include <atomic.h>

#define MBOX_INIT_NAME "no_name"
#define MBOX_MUTEX_KEY_OFFSET 4900
#define MBOX_FULL_COND_KEY_OFFSET 4396
#define MBOX_DNE_COND_KEY_OFFSET 1557

mutex_lock_t mlocks[LOCK_NUM];
barrier_t barriers[BARRIER_NUM];
condition_t conditions[CONDITION_NUM];
mailbox_t mbox[MBOX_NUM];

void init_locks(void)
{
    /* TODO: [p2-task2] initialize mlocks */
    for(int i = 0; i< LOCK_NUM; i++){
        mlocks[i].lock.status = UNLOCKED;
        mlocks[i].key = -1;
        LIST_INIT(mlocks[i].block_queue);
    }
}

void spin_lock_init(spin_lock_t *lock)
{
    /* TODO: [p2-task2] initialize spin lock */
    lock->status = UNLOCKED;
}

int spin_lock_try_acquire(spin_lock_t *lock)
{
    /* TODO: [p2-task2] try to acquire spin lock */
    if(lock->status == UNLOCKED){
        lock->status = LOCKED;
        return 1;
    }
    else{
        return 0;
    }
}

void spin_lock_acquire(spin_lock_t *lock)
{
    /* TODO: [p2-task2] acquire spin lock */
    while(spin_lock_try_acquire(lock) != 1);
}

void spin_lock_release(spin_lock_t *lock)
{
    /* TODO: [p2-task2] release spin lock */
    lock->status = UNLOCKED;
}

int do_mutex_lock_init(int key)
{
    /* TODO: [p2-task2] initialize mutex lock */
    for(int i = 0; i < LOCK_NUM; i++){
        if(mlocks[i].key == -1){
            mlocks[i].key = key;
            return i;
        }
        else if(mlocks[i].key == key){
            return i;
        }
    }
    return -1;
}

void do_mutex_lock_acquire(int mlock_idx)
{
    int core_id = get_current_cpu_id();
    /* TODO: [p2-task2] acquire mutex lock */
    if(mlocks[mlock_idx].lock.status == UNLOCKED){
        mlocks[mlock_idx].lock.status = LOCKED; 
        current_running[core_id]->locks.hold_lock_idx[current_running[core_id]->locks.hold_lock_num++] = mlock_idx;
    }
    else{
        while(1){
            if(mlocks[mlock_idx].lock.status == UNLOCKED){
                mlocks[mlock_idx].lock.status = LOCKED; 
                current_running[core_id]->locks.hold_lock_idx[current_running[core_id]->locks.hold_lock_num++] = mlock_idx;
                return;
            }
            else{
                do_block(&(current_running[core_id]->list), &(mlocks[mlock_idx].block_queue));
            }
        }
    }
}

void do_mutex_lock_release(int mlock_idx)
{
    /* TODO: [p2-task2] release mutex lock */
    int core_id = get_current_cpu_id();
    if(mlocks[mlock_idx].lock.status == LOCKED){
        mlocks[mlock_idx].lock.status = UNLOCKED;
        current_running[core_id]->locks.hold_lock_num--;
        while(mlocks[mlock_idx].block_queue.index != -1){
            do_unblock_head(&(mlocks[mlock_idx].block_queue), PUT_AT_TAIL);
        }
    }
}

/* TODO: 摧毁锁 */

void init_barriers(void){
    for(int i = 0; i < BARRIER_NUM; i++){
        barriers[i].key = -1;
        barriers[i].num_to_wait = 0;
        barriers[i].goal = 0;
        LIST_INIT(barriers[i].barrier_queue);
    }
}

int do_barrier_init(int key, int goal){
    for(int i = 0; i < BARRIER_NUM; i++){
        if(barriers[i].key == -1){
            barriers[i].key = key;
            barriers[i].num_to_wait = goal;
            barriers[i].goal = goal;
            return i;
        }
        else if(barriers[i].key == key){
            return i;
        }
    }
    return -1;
}

void do_barrier_wait(int bar_idx){
    int core_id = get_current_cpu_id();
    barrier_t *bar = &(barriers[bar_idx]);
    bar->num_to_wait--;
    if(bar->num_to_wait == 0){
        //wake_up_all
        while(bar->barrier_queue.index != -1){
            do_unblock_head(&(bar->barrier_queue), PUT_AT_TAIL);
        }
        bar->num_to_wait = bar->goal;
    }
    else{
        do_block(&(current_running[core_id]->list), &(bar->barrier_queue));
    }
}

void do_barrier_destroy(int bar_idx){
    barrier_t *bar = &(barriers[bar_idx]);
    bar->key = -1;
    bar->num_to_wait = 0;
    bar->goal = 0;
    /* 释放等待队列中的所有结点并重新初始化等待队列 */
    while(bar->barrier_queue.index != -1){
        do_unblock_head(&(bar->barrier_queue), PUT_AT_TAIL);
    }
    LIST_INIT(bar->barrier_queue);
}

void init_conditions(void){
    for(int i = 0; i < CONDITION_NUM; i++){
        conditions[i].key = -1;
        LIST_INIT(conditions[i].cond_queue);
    }
}

int do_condition_init(int key){
    for(int i = 0; i < CONDITION_NUM; i++){
        if(conditions[i].key == -1){
            conditions[i].key = key;
            //conditions[i].mutex_idx = -1;
            return i;
        }
        else if(conditions[i].key == key){
            return i;
        }
    }
    return -1;
}

void do_condition_wait(int cond_idx, int mutex_idx){
    int core_id = get_current_cpu_id();
    condition_t *cond = &(conditions[cond_idx]);
    //cond->mutex_idx = mutex_idx;
    do_mutex_lock_release(mutex_idx);
    do_block(&(current_running[core_id]->list), &(cond->cond_queue));
    do_mutex_lock_acquire(mutex_idx);
}

void do_condition_signal(int cond_idx){
    condition_t *cond = &(conditions[cond_idx]);
    //do_mutex_lock_acquire(cond->mutex_idx);
    do_unblock_head(&(cond->cond_queue), PUT_AT_TAIL);
}

void do_condition_broadcast(int cond_idx){
    condition_t *cond = &(conditions[cond_idx]);
    while(cond->cond_queue.index != -1){
        do_unblock_head(&(cond->cond_queue), PUT_AT_TAIL);
    }
}

void do_condition_destroy(int cond_idx){
    condition_t *cond = &(conditions[cond_idx]);
    cond->key = -1;
    while(cond->cond_queue.index != -1){
        do_unblock_head(&(cond->cond_queue), PUT_AT_TAIL);
    }
    LIST_INIT(cond->cond_queue);
}

void init_mbox(){
    for(int i = 0; i < MBOX_NUM; i++){
        strcpy(mbox[i].name, MBOX_INIT_NAME);
        mbox[i].have_data_len = 0;
        mbox[i].need_data_len = 0;
        mbox[i].using_num = 0;
        mbox[i].mutex_idx = -1;
        mbox[i].full_cond_idx = -1;
        mbox[i].dne_cond_idx = -1;
    }
}

int do_mbox_open(char *name){
    // 第一次使用信箱时再去申请锁和条件变量，避免资源浪费
    for(int i = 0; i < MBOX_NUM; i++){
        if(strcmp(mbox[i].name, MBOX_INIT_NAME) == 0){
            strcpy(mbox[i].name, name);
            mbox[i].using_num++;
            mbox[i].mutex_idx = do_mutex_lock_init(MBOX_MUTEX_KEY_OFFSET + i);
            mbox[i].full_cond_idx = do_condition_init(MBOX_FULL_COND_KEY_OFFSET + i);
            mbox[i].dne_cond_idx = do_condition_init(MBOX_DNE_COND_KEY_OFFSET + i);
            return i;
        }
        else if(strcmp(mbox[i].name, name) == 0){
            mbox[i].using_num++;
            return i;
        }
    }
    return -1;
}

void do_mbox_close(int mbox_idx){
    mbox[mbox_idx].using_num--;
    if(mbox[mbox_idx].using_num == 0){
        // 摧毁申请的锁和条件变量
        do_condition_destroy(mbox[mbox_idx].full_cond_idx);
        do_condition_destroy(mbox[mbox_idx].dne_cond_idx);
        // 重新初始化该信箱
        strcpy(mbox[mbox_idx].name, MBOX_INIT_NAME);
        mbox[mbox_idx].have_data_len = 0;
        mbox[mbox_idx].need_data_len = 0;
        mbox[mbox_idx].using_num = 0;
        mbox[mbox_idx].mutex_idx = -1;
        mbox[mbox_idx].full_cond_idx = -1;
        mbox[mbox_idx].dne_cond_idx = -1;
    }
}

int do_mbox_send(int mbox_idx, void * msg, int msg_length){
    mailbox_t *mbox_to_send = &(mbox[mbox_idx]);

    do_mutex_lock_acquire(mbox_to_send->mutex_idx);

    while(mbox_to_send->have_data_len + msg_length > MAX_MBOX_LENGTH){
        do_condition_wait(mbox_to_send->full_cond_idx, mbox_to_send->mutex_idx);
    }

    strncpy((mbox_to_send->buf + mbox_to_send->have_data_len), (const char *)msg, msg_length);
    mbox_to_send->have_data_len += msg_length;
    
    if(mbox_to_send->have_data_len >= mbox_to_send->need_data_len){
        do_condition_signal(mbox_to_send->dne_cond_idx);
    }

    do_mutex_lock_release(mbox_to_send->mutex_idx);
    return 1;
}

int do_mbox_recv(int mbox_idx, void * msg, int msg_length){
    mailbox_t *mbox_to_recv = &(mbox[mbox_idx]);

    do_mutex_lock_acquire(mbox_to_recv->mutex_idx);

    mbox_to_recv->need_data_len = msg_length;

    while(mbox_to_recv->need_data_len > mbox_to_recv->have_data_len){
        do_condition_wait(mbox_to_recv->dne_cond_idx, mbox_to_recv->mutex_idx);
    }

    strncpy((const char *)msg, mbox_to_recv->buf, msg_length);
    mbox_to_recv->need_data_len = 0;
    mbox_to_recv->have_data_len -= msg_length;
    // 将信箱中被读取出的信息清除，后面的信息前移至0位置
    strncpy(mbox_to_recv->buf, mbox_to_recv->buf + msg_length, mbox_to_recv->have_data_len);

    if(mbox_to_recv->have_data_len <= MAX_MBOX_LENGTH){
        // NOTE: 
        // 双核情况下由于主核运行shell，很难抢到锁
        // 使用signal后表现上更加公平
        // 可能需要改进抢锁策略

        do_condition_broadcast(mbox_to_recv->full_cond_idx);
        //do_condition_signal(mbox_to_recv->full_cond_idx);
    }

    do_mutex_lock_release(mbox_to_recv->mutex_idx);
    return 1;
}