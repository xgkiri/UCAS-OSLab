/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#include <asm.h>
#include <csr.h>

#define KERNEL_STACK_CORE0		0xffffffc050500000
#define KERNEL_STACK_CORE1    0xffffffc051000000

ENTRY(_start)
//**************** 3 ****************

  /* Mask all interrupts */
  csrw CSR_SIE, zero
  csrw CSR_SIP, zero

judge_core_id:
  // 判断启动的是主核还是从核，据此分配不同的栈空间
  // 从核直接跳转到分配栈空间，不需要再一次清空bss段
  csrr    t0, CSR_MHARTID
  bnez    t0, core_1_sp

set_bss:
  /* TODO: [p1-task2] clear BSS for flat non-ELF images */
  la t1, __bss_start
  la t2, __BSS_END__

clear_bss:
  bgeu t1, t2, core_0_sp
  sw x0, (t1)
  addi t1, t1, 4
  j clear_bss

  /* TODO: [p1-task2] setup C environment */
core_0_sp:
  // 给主核分配栈空间
  li sp, KERNEL_STACK_CORE0
  call main

core_1_sp:
  // 给从核分配栈空间
  li sp, KERNEL_STACK_CORE1
  call main

loop:
  wfi
  j loop

END(_start)
