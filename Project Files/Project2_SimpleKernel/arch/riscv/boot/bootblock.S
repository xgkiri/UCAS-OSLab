#include <asm/biosdef.h>

// os size location (os_size could be sectors [p1-task3] or bytes [p1-task4])
.equ os_size_loc, 0x502001fc

// kernel address (move kernel to here, and jmp here to start kernel)
.equ kernel, 0x50201000

// BIOS function entry (jump here to use BIOS APIs)
.equ bios_func_entry, 0x50150000

// task info address
.equ task_info_addr, 0x502001f8

// task number address
.equ task_num_addr, 0x502001f4

// start addr
.equ task_start_addr, 0x52000000

//task counter addr
.equ task_count_addr, 0x502001f0

.text
.global main

main:
	// fence on all memory and I/O
	fence

	// TODO: [p1-task1] call BIOS to print string "It's bootblock!"
	li a0, BIOS_PUTSTR
	la a1, msg
	call bios_func_entry

	// TODO: [p1-task2] call BIOS to read kernel in SD card
	li a0, BIOS_SDREAD

	lui a1, %hi(kernel)
	addi a1, a1, %lo(kernel)

	la a3, os_size_loc
	lh a4, (a3) 
	li a2, 0
	li a3, 512
	
get_kernel_sector_num:
	addi a2, a2, 1
	sub a4, a4, a3
	bgtz a4, get_kernel_sector_num
	/*lui a4, %hi(os_size_loc)
	addi a4, a4, %lo(os_size_loc)
	lhu a2, (a4)*/
	//li a2, 4

	li a3, 1
	call bios_func_entry

	// TODO: [p1-task4] load task-related arguments and pass them to kernel
	li a0, BIOS_SDREAD
	
	//set a1
	la a2, task_num_addr
	lh a3, (a2)
	li a2, 1
	lui a1, %hi(task_start_addr)
	addi a1, a1, %lo(task_start_addr)
	li a4, 0x10000
L1:
	add a1, a1, a4
	sub a3, a3, a2
	bgtz a3, L1

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

	//init task_counter
	la a1, task_count_addr
	sw x0, (a1)

	// TODO: [p1-task2] jump to kernel to start UCAS-OS
	la a1, kernel
	jr a1
	//jr a1

// while(1) --> stop here
stop:
	j stop
	nop

.data

msg: .string "It's a bootloader...\n\r"

