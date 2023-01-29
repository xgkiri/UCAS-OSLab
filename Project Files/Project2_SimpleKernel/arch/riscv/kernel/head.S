/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#include <asm.h>
#include <csr.h>

#define KERNEL_STACK		0x50500000
.section ".entry_function","ax"
ENTRY(_start)
  /* Mask all interrupts */
  csrw CSR_SIE, zero
  csrw CSR_SIP, zero

  /* TODO: [p1-task2] clear BSS for flat non-ELF images */
  la t1, __bss_start
  la t2, __BSS_END__

L1:
  bgeu t1, t2, L2
  sw x0, (t1)
  addi t1, t1, 4
  j L1

  
  /* TODO: [p1-task2] setup C environment */
L2:
  li sp, KERNEL_STACK
  j main

loop:
  wfi
  j loop

END(_start)
