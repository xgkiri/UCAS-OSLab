#include <syscall.h>
//#include <tiny_libc/include/syscall.h>

#include <stdint.h>

#include <unistd.h>
//#include <tiny_libc/include/unistd.h>

static const long IGNORE = 0L;

static long invoke_syscall(long sysno, long arg0, long arg1, long arg2,
                           long arg3, long arg4)
{
    /* TODO: [p2-task3] implement invoke_syscall via inline assembly */
    //将参数存入寄存器中，SAVE_CONTEXT时将其转存进栈里，之后从栈中访问参数
    long return_value;

    asm volatile(
    "add a7, %[sysno], zero\n\t"
    "add a0, %[arg0], zero\n\t"
    "add a1, %[arg1], zero\n\t"
    "add a2, %[arg2], zero\n\t"
    "add a3, %[arg3], zero\n\t"
    "add a4, %[arg4], zero\n\t"
    "ecall\n\t"
    "nop\n\t"//sepc4 return here
    "add %0, a0, zero\n\t"
    : "=r"(return_value)
    : [sysno] "r" (sysno),
      [arg0]  "r" (arg0),
      [arg1]  "r" (arg1),
      [arg2]  "r" (arg2),
      [arg3]  "r" (arg3),
      [arg4]  "r" (arg4)
    : "a0", "a1", "a2", "a3", "a4", "a7"
    );


    return return_value;
}

void sys_yield(void)
{
    /* TODO: [p2-task3] call invoke_syscall to implement sys_yield */
    invoke_syscall(SYSCALL_YIELD, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_move_cursor(int x, int y)
{   
    /* TODO: [p2-task3] call invoke_syscall to implement sys_move_cursor */
    invoke_syscall(SYSCALL_CURSOR, (long)x, (long)y, IGNORE, IGNORE, IGNORE);
}

void sys_write(char *buff)
{   
    /* TODO: [p2-task3] call invoke_syscall to implement sys_write */
    invoke_syscall(SYSCALL_WRITE, (long)buff, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_reflush(void)
{   
    /* TODO: [p2-task3] call invoke_syscall to implement sys_reflush */
    invoke_syscall(SYSCALL_REFLUSH, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_mutex_init(int key)
{
    /* TODO: [p2-task3] call invoke_syscall to implement sys_mutex_init */
    return invoke_syscall(SYSCALL_LOCK_INIT, (long)key, IGNORE, IGNORE, IGNORE, IGNORE);
    //return 0;
}

void sys_mutex_acquire(int mutex_idx)
{
    /* TODO: [p2-task3] call invoke_syscall to implement sys_mutex_acquire */
    invoke_syscall(SYSCALL_LOCK_ACQ, (long)mutex_idx, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_mutex_release(int mutex_idx)
{
    /* TODO: [p2-task3] call invoke_syscall to implement sys_mutex_release */
    invoke_syscall(SYSCALL_LOCK_RELEASE, (long)mutex_idx, IGNORE, IGNORE, IGNORE, IGNORE);
}

long sys_get_timebase(void)
{   
    /* TODO: [p2-task3] call invoke_syscall to implement sys_get_timebase */
    return invoke_syscall(SYSCALL_GET_TIMEBASE, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
    //return 0;
}

long sys_get_tick(void)
{
    /* TODO: [p2-task3] call invoke_syscall to implement sys_get_tick */
    return invoke_syscall(SYSCALL_GET_TICK, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
    //return 0;
}

void sys_sleep(uint32_t time)
{
    /* TODO: [p2-task3] call invoke_syscall to implement sys_sleep */
    invoke_syscall(SYSCALL_SLEEP, (long)time, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_fork(void *func, long arg0, long arg1)
{
    invoke_syscall(SYSCALL_FORK, (long)func, arg0, arg1, IGNORE, IGNORE);
}

// A/C-core
pid_t sys_exec(char *name, int argc, char **argv)
{
    /* TODO: [p3-task1] call invoke_syscall to implement sys_exec */
    return invoke_syscall(SYSCALL_EXEC, (long)name, (long)argc, (long)argv, IGNORE, IGNORE);
}


void sys_exit(void)
{
    /* TODO: [p3-task1] call invoke_syscall to implement sys_exit */
    invoke_syscall(SYSCALL_EXIT, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_kill(pid_t pid)
{
    /* TODO: [p3-task1] call invoke_syscall to implement sys_kill */
    return invoke_syscall(SYSCALL_KILL, (long)pid, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_waitpid(pid_t pid)
{
    /* TODO: [p3-task1] call invoke_syscall to implement sys_waitpid */
    return invoke_syscall(SYSCALL_WAITPID, (long)pid, IGNORE, IGNORE, IGNORE, IGNORE);
}


void sys_ps(void)
{
    /* TODO: [p3-task1] call invoke_syscall to implement sys_ps */
    invoke_syscall(SYSCALL_PS, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

pid_t sys_getpid()
{
    /* TODO: [p3-task1] call invoke_syscall to implement sys_getpid */
    return invoke_syscall(SYSCALL_GETPID, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_getchar(void)
{
    /* TODO: [p3-task1] call invoke_syscall to implement sys_getchar */
    char c;
    c = invoke_syscall(SYSCALL_GETCHAR, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
    return c;
}

void sys_screen_clean(void)
{
    invoke_syscall(SYSCALL_SCREEN_CLEAN, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_barrier_init(int key, int goal)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_barrier_init */
    return invoke_syscall(SYSCALL_BARR_INIT, (long)key, (long)goal, IGNORE, IGNORE, IGNORE);
}

void sys_barrier_wait(int bar_idx)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_barrie_wait */
    invoke_syscall(SYSCALL_BARR_WAIT, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_barrier_destroy(int bar_idx)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_barrie_destory */
    invoke_syscall(SYSCALL_BARR_DESTROY, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_condition_init(int key)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_condition_init */
    return invoke_syscall(SYSCALL_COND_INIT, (long)key, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_condition_wait(int cond_idx, int mutex_idx)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_condition_wait */
    invoke_syscall(SYSCALL_COND_WAIT, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_condition_signal(int cond_idx)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_condition_signal */
    invoke_syscall(SYSCALL_COND_SIGNAL, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_condition_broadcast(int cond_idx)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_condition_broadcast */
    invoke_syscall(SYSCALL_COND_BROADCAST, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_condition_destroy(int cond_idx)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_condition_destroy */
    invoke_syscall(SYSCALL_COND_DESTROY, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_mbox_open(char * name)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_mbox_open */
    return invoke_syscall(SYSCALL_MBOX_OPEN, (long)name, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_mbox_close(int mbox_id)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_mbox_close */
    invoke_syscall(SYSCALL_MBOX_CLOSE, (long)mbox_id, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_mbox_send(int mbox_idx, void *msg, int msg_length)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_mbox_send */
    return invoke_syscall(SYSCALL_MBOX_SEND, (long)mbox_idx, (long)msg, (long)msg_length, IGNORE, IGNORE);
}

int sys_mbox_recv(int mbox_idx, void *msg, int msg_length)
{
    /* TODO: [p3-task2] call invoke_syscall to implement sys_mbox_recv */
    return invoke_syscall(SYSCALL_MBOX_RECV, (long)mbox_idx, (long)msg, (long)msg_length, IGNORE, IGNORE);
}

int sys_get_core_id(void){
    return invoke_syscall(SYSCALL_GET_CORE_ID, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_set_mask(int mask, pid_t pid){
    invoke_syscall(SYSCALL_SET_MASK, (long)mask, (long)pid, IGNORE, IGNORE, IGNORE);
}

pid_t sys_set_task(char *name, int mask, int argc, char *argv[]){
    return invoke_syscall(SYSCALL_SET_TASK, (long)name, (long)mask, (long)argc, (long)argv, IGNORE);
}

void* sys_shmpageget(int key)
{
    /* TODO: [p4-task5] call invoke_syscall to implement sys_shmpageget */
    return invoke_syscall(SYSCALL_SHM_GET, (long)key, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_shmpagedt(void *addr)
{
    /* TODO: [p4-task5] call invoke_syscall to implement sys_shmpagedt */
    invoke_syscall(SYSCALL_SHM_DT, (long)addr, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_pthread_create(pthread_t *thread, void (*start_routine)(void*), void *arg){
    invoke_syscall(SYSCALL_PTHREAD_CREATE, (long)thread, (long)start_routine, (long)arg, IGNORE, IGNORE);
}

uint64_t sys_create_sp(uint64_t vaddr_u){
    return invoke_syscall(SYSCALL_CREATE_SP, (long)vaddr_u, IGNORE, IGNORE, IGNORE, IGNORE);
}

uint64_t sys_get_pa(uint64_t vaddr_u){
    return invoke_syscall(SYSCALL_GET_PA, (long)vaddr_u, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_net_send(void *txpacket, int length)
{
    /* TODO: [p5-task1] call invoke_syscall to implement sys_net_send */
    return invoke_syscall(SYSCALL_NET_SEND, (long)txpacket, (long)length, IGNORE, IGNORE, IGNORE);
}

int sys_net_recv(void *rxbuffer, int pkt_num, int *pkt_lens)
{
    /* TODO: [p5-task2] call invoke_syscall to implement sys_net_recv */
    return invoke_syscall(SYSCALL_NET_RECV, (long)rxbuffer, (long)pkt_num, (long)pkt_lens, IGNORE, IGNORE);
}

int sys_mkfs(void)
{
    // TODO [P6-task1]: Implement sys_mkfs
    return invoke_syscall(SYSCALL_FS_MKFS, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_statfs(void)
{
    // TODO [P6-task1]: Implement sys_statfs
    return invoke_syscall(SYSCALL_FS_STATFS, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_cd(char *path)
{
    // TODO [P6-task1]: Implement sys_cd
    return invoke_syscall(SYSCALL_FS_CD, (long)path, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_mkdir(char *path)
{
    // TODO [P6-task1]: Implement sys_mkdir
    return invoke_syscall(SYSCALL_FS_MKDIR, (long)path, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_rmdir(char *path)
{
    // TODO [P6-task1]: Implement sys_rmdir
    return invoke_syscall(SYSCALL_FS_RMDIR, (long)path, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_ls(char *path, int option)
{
    // TODO [P6-task1]: Implement sys_ls
    // Note: argument 'option' serves for 'ls -l' in A-core
    return invoke_syscall(SYSCALL_FS_LS, (long)path, (long)option, IGNORE, IGNORE, IGNORE);
}

int sys_touch(char *path)
{
    // TODO [P6-task2]: Implement sys_touch
    return invoke_syscall(SYSCALL_FS_TOUCH, (long)path, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_cat(char *path)
{
    // TODO [P6-task2]: Implement sys_cat
    return invoke_syscall(SYSCALL_FS_CAT, (long)path, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_fopen(char *path, int mode)
{
    // TODO [P6-task2]: Implement sys_fopen
    return invoke_syscall(SYSCALL_FS_FOPEN, (long)path, (long)mode, IGNORE, IGNORE, IGNORE);
}

int sys_fread(int fd, char *buff, int length)
{
    // TODO [P6-task2]: Implement sys_fread
    return invoke_syscall(SYSCALL_FS_FREAD, (long)fd, (long)buff, (long)length, IGNORE, IGNORE);
}

int sys_fwrite(int fd, char *buff, int length)
{
    // TODO [P6-task2]: Implement sys_fwrite
    return invoke_syscall(SYSCALL_FS_FWRITE, (long)fd, (long)buff, (long)length, IGNORE, IGNORE);
}

int sys_fclose(int fd)
{
    // TODO [P6-task2]: Implement sys_fclose
    return invoke_syscall(SYSCALL_FS_FCLOSE, (long)fd, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_ln(char *src_path, char *dst_path)
{
    // TODO [P6-task2]: Implement sys_ln
    return invoke_syscall(SYSCALL_FS_LN, (long)src_path, (long)dst_path, IGNORE, IGNORE, IGNORE);
}

int sys_rm(char *path)
{
    // TODO [P6-task2]: Implement sys_rm
    return invoke_syscall(SYSCALL_FS_RM, (long)path, IGNORE, IGNORE, IGNORE, IGNORE); 
}

int sys_lseek(int fd, int offset, int whence, int rw_type)
{
    // TODO [P6-task2]: Implement sys_lseek
    return invoke_syscall(SYSCALL_FS_LSEEK, (long)fd, (long)offset, (long)whence, (long)rw_type, IGNORE);
}
