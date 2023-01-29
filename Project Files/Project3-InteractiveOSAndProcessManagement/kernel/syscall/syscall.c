#include <sys/syscall.h>
//#include <include/sys/syscall.h>

long (*syscall[NUM_SYSCALLS])();

void handle_syscall(regs_context_t *regs, uint64_t interrupt, uint64_t cause)
{
    /* TODO: [p2-task3] handle syscall exception */
    /**
     * HINT: call syscall function like syscall[fn](arg0, arg1, arg2),
     * and pay attention to the return value and sepc
     */
    uint64_t sys_num = regs->regs[17];//syscall number: in a7
    uint64_t arg0 = regs->regs[10];//arg0: in a0
    uint64_t arg1 = regs->regs[11];//arg1: in a1
    uint64_t arg2 = regs->regs[12];//arg2: in a2
    uint64_t arg3 = regs->regs[13];//arg3: in a3
    uint64_t arg4 = regs->regs[14];//arg3: in a4

    regs->sepc = regs->sepc + 4;
    //把返回值存在栈里，恢复上下文时，返回值被load到相应的寄存器中
    regs->regs[10] = syscall[sys_num](arg0, arg1, arg2, arg3, arg4);//save return value in a0
}
