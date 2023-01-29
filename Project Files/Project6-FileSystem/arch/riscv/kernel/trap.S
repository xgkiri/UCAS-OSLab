#include <asm.h>
#include <csr.h>

ENTRY(setup_exception)

  // 暂时关闭外部中断
  /*
  li t0, SIE_SEIE
  csrc CSR_SIE, t0
  */

  /* TODO: [p2-task3] save exception_handler_entry into STVEC */

  la t0, exception_handler_entry
  csrw CSR_STVEC, t0

  // 屏蔽中断，在开中断后也不会对关中断期间收到的软件中断进行响应
  csrw    CSR_SIP, zero
  
  li t0, SIE_SSIE | SIE_STIE | SIE_SEIE
  csrs CSR_SIE, t0

  /* TODO: [p2-task4] enable interrupts globally */
  li t0, SR_SIE
  csrs CSR_SSTATUS, t0

  jr ra
ENDPROC(setup_exception)
