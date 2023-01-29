#include <asm.h>

.section ".entry_function","ax"

ENTRY(_start)

    /* TODO: [p1-task3] setup C runtime environment for the user program */
  la t1, __bss_start
  la t2, __BSS_END__

L1:
  bgeu t1, t2, L2
  sw x0, (t1)
  addi t1, t1, 4
  j L1

L2:
  call main
  
  call sys_exit
  
// while(1) loop, unreachable here
loop:
    wfi
    j loop

END(_start)
