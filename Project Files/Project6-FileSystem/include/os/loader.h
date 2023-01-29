#ifndef __INCLUDE_LOADER_H__
#define __INCLUDE_LOADER_H__

#include <type.h>
#include <os/task.h>

uintptr_t load_task(task_info_t *task_info);
void get_task_info(task_info_t *task_info);

#endif