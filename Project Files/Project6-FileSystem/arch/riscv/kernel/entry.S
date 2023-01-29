#include <asm.h>
#include <csr.h>
#include <asm/regs.h>

.macro SAVE_CONTEXT
  /* TODO: [p2-task3] save all general purpose registers here! */
  /* HINT: Pay attention to the function of tp and sp, and save them carefully! */
  //beq tp, zero, PASS_PARAMS

  // 在栈中保存t0
  sd t0, -8(sp)

  ld t0, PCB_KERNEL_SP(tp)//top
  addi t0, t0, -(OFFSET_SIZE)//context

  sd zero, OFFSET_REG_ZERO(t0)

  sd ra, OFFSET_REG_RA(t0)

  sd sp, OFFSET_REG_SP(t0)
  sd gp, OFFSET_REG_GP(t0)
  //sd tp, OFFSET_REG_TP(t0)

  //sd t0, OFFSET_REG_T0(t0)
  sd t1, OFFSET_REG_T1(t0)
  sd t2, OFFSET_REG_T2(t0)

  sd s0, OFFSET_REG_S0(t0)
  sd s1, OFFSET_REG_S1(t0)

  sd a0, OFFSET_REG_A0(t0)
  sd a1, OFFSET_REG_A1(t0)
  sd a2, OFFSET_REG_A2(t0)
  sd a3, OFFSET_REG_A3(t0)
  sd a4, OFFSET_REG_A4(t0)
  sd a5, OFFSET_REG_A5(t0)
  sd a6, OFFSET_REG_A6(t0)
  sd a7, OFFSET_REG_A7(t0)

  sd s2, OFFSET_REG_S2(t0)
  sd s3, OFFSET_REG_S3(t0)
  sd s4, OFFSET_REG_S4(t0)
  sd s5, OFFSET_REG_S5(t0)
  sd s6, OFFSET_REG_S6(t0)
  sd s7, OFFSET_REG_S7(t0)
  sd s8, OFFSET_REG_S8(t0)
  sd s9, OFFSET_REG_S9(t0)
  sd s10, OFFSET_REG_S10(t0)
  sd s11, OFFSET_REG_S11(t0)

  sd t3, OFFSET_REG_T3(t0)
  sd t4, OFFSET_REG_T4(t0)
  sd t5, OFFSET_REG_T5(t0)
  sd t6, OFFSET_REG_T6(t0)
  /*
   * Disable user-mode memory access as it should only be set in the
   * actual user copy routines.
   *
   * Disable the FPU to detect illegal usage of floating point in kernel
   * space.
   */
  
  li t1, SR_FS
  csrc CSR_SSTATUS, t1

  /* TODO: [p2-task3] save sstatus, sepc, stval and scause on kernel stack */
  csrr t1, CSR_SSTATUS
  csrr t2, CSR_SEPC
  csrr t3, CSR_STVAL
  csrr t4, CSR_SCAUSE

  sd   t1, OFFSET_REG_SSTATUS(t0)
  sd   t2, OFFSET_REG_SEPC(t0)
  sd   t3, OFFSET_REG_SBADADDR(t0)
  sd   t4, OFFSET_REG_SCAUSE(t0)

  // 从栈中恢复t0，并保存
  ld   t1, -8(sp)
  sd   t1, OFFSET_REG_T0(t0)

  // 进入内核，切换到内核栈
  ld sp, PCB_CURRENT_KERNEL_SP(tp)
.endm

.macro RESTORE_CONTEXT
  /* TODO: Restore all general purpose registers and sepc, sstatus */
  /* HINT: Pay attention to sp again! */
  
  // 保存内核栈sp，之后切换到用户栈
  sd sp, PCB_CURRENT_KERNEL_SP(tp)

  ld t0, PCB_KERNEL_SP(tp)//top
  addi t0, t0, -(OFFSET_SIZE)//context

  ld t1, OFFSET_REG_SSTATUS(t0)
  ld t2, OFFSET_REG_SEPC(t0)

  csrw CSR_SSTATUS, t1
  csrw CSR_SEPC, t2

  ld zero, OFFSET_REG_ZERO(t0)

  ld ra, OFFSET_REG_RA(t0)

  ld sp, OFFSET_REG_SP(t0)
  ld gp, OFFSET_REG_GP(t0)
  //ld tp, OFFSET_REG_TP(t0)

  //ld t0, OFFSET_REG_T0(t0)
  ld t1, OFFSET_REG_T1(t0)
  ld t2, OFFSET_REG_T2(t0)

  ld s0, OFFSET_REG_S0(t0)
  ld s1, OFFSET_REG_S1(t0)

  ld a0, OFFSET_REG_A0(t0)
  ld a1, OFFSET_REG_A1(t0)
  ld a2, OFFSET_REG_A2(t0)
  ld a3, OFFSET_REG_A3(t0)
  ld a4, OFFSET_REG_A4(t0)
  ld a5, OFFSET_REG_A5(t0)
  ld a6, OFFSET_REG_A6(t0)
  ld a7, OFFSET_REG_A7(t0)

  ld s2, OFFSET_REG_S2(t0)
  ld s3, OFFSET_REG_S3(t0)
  ld s4, OFFSET_REG_S4(t0)
  ld s5, OFFSET_REG_S5(t0)
  ld s6, OFFSET_REG_S6(t0)
  ld s7, OFFSET_REG_S7(t0)
  ld s8, OFFSET_REG_S8(t0)
  ld s9, OFFSET_REG_S9(t0)
  ld s10, OFFSET_REG_S10(t0)
  ld s11, OFFSET_REG_S11(t0)

  ld t3, OFFSET_REG_T3(t0)
  ld t4, OFFSET_REG_T4(t0)
  ld t5, OFFSET_REG_T5(t0)
  ld t6, OFFSET_REG_T6(t0)

  // 最后恢复t0
  ld t0, OFFSET_REG_T0(t0)
.endm

ENTRY(enable_preempt)
  not t0, x0
  csrs CSR_SIE, t0

  // 暂时关闭外部中断
  /*
  li t0, SIE_SEIE
  csrc CSR_SIE, t0
  */
  
  jr ra
ENDPROC(enable_preempt)

ENTRY(disable_preempt)
  csrw CSR_SIE, zero
  jr ra
ENDPROC(disable_preempt)

ENTRY(enable_interrupt)
  // 打开内核态的中断使能
  li t0, SR_SIE
  csrs CSR_SSTATUS, t0
  jr ra
ENDPROC(enable_interrupt)

ENTRY(disable_interrupt)
  // 关闭内核态的中断使能
  li t0, SR_SIE
  csrc CSR_SSTATUS, t0
  jr ra
ENDPROC(disable_interrupt)

// NOTE: the address of previous pcb in a0
// NOTE: the address of next pcb in a1
ENTRY(switch_to)

  ld t0, PCB_KERNEL_SP(a0)//top
  addi t0, t0, -(SWITCH_TO_SIZE + OFFSET_SIZE)//14regs
  
  sd sp, PCB_CURRENT_KERNEL_SP(a0)

  sd ra, SWITCH_TO_RA(t0)
  sd s0, SWITCH_TO_S0(t0)
  sd s1, SWITCH_TO_S1(t0)
  sd s2, SWITCH_TO_S2(t0)
  sd s3, SWITCH_TO_S3(t0)
  sd s4, SWITCH_TO_S4(t0)
  sd s5, SWITCH_TO_S5(t0)
  sd s6, SWITCH_TO_S6(t0)
  sd s7, SWITCH_TO_S7(t0)
  sd s8, SWITCH_TO_S8(t0)
  sd s9, SWITCH_TO_S9(t0)
  sd s10, SWITCH_TO_S10(t0)
  sd s11, SWITCH_TO_S11(t0)
  /* TODO: [p2-task1] restore all callee save registers from kernel stack,
   * see the definition of `struct switchto_context` in sched.h*/
  ld t0, PCB_KERNEL_SP(a1)//top
  addi t0, t0, -(SWITCH_TO_SIZE + OFFSET_SIZE)//14regs

  ld sp, PCB_CURRENT_KERNEL_SP(a1)

  ld ra, SWITCH_TO_RA(t0)
  ld s0, SWITCH_TO_S0(t0)  
  ld s1, SWITCH_TO_S1(t0)
  ld s2, SWITCH_TO_S2(t0)
  ld s3, SWITCH_TO_S3(t0)
  ld s4, SWITCH_TO_S4(t0)
  ld s5, SWITCH_TO_S5(t0)
  ld s6, SWITCH_TO_S6(t0)
  ld s7, SWITCH_TO_S7(t0)
  ld s8, SWITCH_TO_S8(t0)
  ld s9, SWITCH_TO_S9(t0)
  ld s10, SWITCH_TO_S10(t0)
  ld s11, SWITCH_TO_S11(t0)

  //addi sp, sp, SWITCH_TO_SIZE

  /* save current_running in tp */
  add tp, a1, zero
  /* set ra as entry addr of each task when init */
  jr ra
ENDPROC(switch_to)

ENTRY(ret_from_exception)
  /* TODO: [p2-task3] restore context via provided macro and return to sepc */
  /* HINT: remember to check your sp, does it point to the right address? */

  /* can jump to init ra by loading init info when restoring context */

  // 先释放内核锁，再恢复上下文
  call unlock_kernel

  RESTORE_CONTEXT
  
  sret
ENDPROC(ret_from_exception)

ENTRY(exception_handler_entry)
  
  /* TODO: [p2-task3] save context via the provided macro */
  SAVE_CONTEXT

  /* TODO: [p2-task3] load ret_from_exception into $ra so that we can return to
   * ret_from_exception when interrupt_help complete.
   */

  /* TODO: [p2-task3] call interrupt_helper
   * NOTE: don't forget to pass parameters for it.
   */

PASS_PARAMS:
  // 先保存上下文，再抢内核锁
  call lock_kernel

  ld    a0, PCB_KERNEL_SP(tp)//top
  addi  a0, a0, -(OFFSET_SIZE)//context
  csrr  a1, CSR_STVAL
  csrr  a2, CSR_SCAUSE

  call interrupt_helper
  
  j ret_from_exception
ENDPROC(exception_handler_entry)
