#include <asm/biosdef.h>
#include <csr.h>
// kernel address (move kernel to here, and jmp here to start kernel)
.equ kernel, 0x50202000

// BIOS function entry (jump here to use BIOS APIs)
.equ bios_func_entry, 0x50150000

// start addr
.equ task_start_addr, 0x52000000

/* 以下信息createimage时被存放在bootblock所在sector的末尾
 * 随bootblock在loadboot时一起被载入内存中
 * bootblock的sector被放在了0x50200000 ~ 0x50200200
 * 据此推算出储存信息在内存中的地址
 * 且此时未开启虚存，直接使用实地址进行访问即可
 */
// os size location (os_size could be sectors [p1-task3] or bytes [p1-task4])
.equ os_size_loc, 0x502001fc

// task counter addr
.equ task_count_addr, 0x502001f0

// task number address
.equ task_num_addr, 0x502001f4

// task info 在img中的offset
// 利用offset在img中找到task info，将其load进内存的某个位置
// 在将该位置存入task_info_addr中
.equ task_info_addr, 0x502001f8

.text
.global main

main:
	// fence on all memory and I/O
	fence

	// 打开sum位，使内核态可以访问用户页
	li		t0, SR_SUM
	csrs	CSR_SSTATUS, t0

	// a0 is mhartid
	bnez a0, secondary

	//****************************NOTE: call_bis(a0: bios_number, a1: arg1, a2: arg2 ...)****************************
	//***************************************sd_read/write(a0: bios_num, a1: addr, a2: sector_num, a3: start_sector)********************

//*********************************************************************** 1. 打印
	// TODO: [p1-task1] call BIOS to print string "It's bootblock!"
	li a0, BIOS_PUTSTR
	la a1, msg
	call bios_func_entry
//*********************************************************************** 2. 载入内核
	// TODO: [p1-task2] call BIOS to read kernel in SD card

	// 分两次次读入内核
	// 第一次读入64个sector
	li a0, BIOS_SDREAD

	lui a1, %hi(kernel)
	addi a1, a1, %lo(kernel)

	li a2, 64
	li a3, 1

	call bios_func_entry

	// 第二次读入剩余部分
	li a0, BIOS_SDREAD

	/* 获取还需要读入的内核大小，存放在a4中 */
	la a3, os_size_loc
	lhu a4, (a3) 
	li a3, 0x8000
	sub a4, a4, a3

	/* 读入位置加0x8000 */
	lui a1, %hi(kernel)
	addi a1, a1, %lo(kernel)
	add a1, a1, a3

	/* 由大小转化为sector_num */
	li a2, 0
	li a3, 512
	
get_kernel_sector_num:
	addi a2, a2, 1
	sub a4, a4, a3
	bgtz a4, get_kernel_sector_num
	
	/* 此时start_sector为65 */
	li a3, 65
	call bios_func_entry

//*********************************************************************** 3. 载入task_info

	// TODO: [p1-task4] load task-related arguments and pass them to kernel
	li a0, BIOS_SDREAD

	//*******************************确定载入位置
	//set a1
	la a2, task_num_addr
	lhu a3, (a2)
	li a2, 1
	lui a1, %hi(task_start_addr)
	addi a1, a1, %lo(task_start_addr)
	li a4, 0x10000
L1:
	add a1, a1, a4
	sub a3, a3, a2
	bgtz a3, L1

	//*******************************确定在img中所处的sector
	//get offset
	la a3, task_info_addr
	lw a4, (a3)
	//set a3
	li a3, 0
	li a2, 512

get_task_info_sector_size:
	addi a3, a3, 1
	sub a4, a4, a2
	bgtz a4, get_task_info_sector_size
	//set a2
	li a2, 1
	call bios_func_entry

//*********************************************************************** 4. 跳转到内核开始执行
	// TODO: [p1-task2] jump to kernel to start UCAS-OS
	la a1, kernel
	jr a1
	//jr a1


secondary:
	/* TODO [P3-task3]: 
	 * 1. Mask all interrupts
	 * 2. let stvec pointer to kernel_main
	 * 3. enable software interrupt for ipi
	 */
	
	// 关闭所有中断使能
	csrw	CSR_SIE, zero
	
	// 将内核入口地址写入CSR_STVEC，触发核间中断后跳转到main执行
	la		t0, kernel
	csrw	CSR_STVEC, t0

	// 打开内核态全局中断使能
	li		t0, SR_SIE
	csrs	CSR_SSTATUS, t0

	// 仅使能软件中断（核间中断是软件中断）
	li		t0, SIE_SSIE
	csrs 	CSR_SIE, t0
	
wait_for_wakeup:
	// 在此处等待主核唤醒
	wfi
	j wait_for_wakeup

// while(1) --> stop here
stop:
	j stop
	nop

.data

msg: .string "It's a bootloader...\n\r"

