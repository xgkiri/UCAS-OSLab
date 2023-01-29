#ifndef __INCLUDE_TASK_H__
#define __INCLUDE_TASK_H__

#include <type.h>

#define TASK_MEM_BASE    0x52000000
#define TASK_MAXNUM      32
#define TASK_SIZE        0x10000
#define NAME_LEN         24

/* TODO: [p1-task4] implement your own task_info_t! */
/*
typedef struct task{
    char task_name[NAME_LEN];
    unsigned task_entry;
}task_info_t;
*/

// 引入虚存机制后，不再需要task_entry
// 改为需要所有相关信息
// 由loader利用这些信息把指定的task load进内存中，并分配页
typedef struct {
    char task_name[24];
    uint32_t filesz;    
    uint32_t memsz;
    int offset;   
} task_info_t;  

#endif