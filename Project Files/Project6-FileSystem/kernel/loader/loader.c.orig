#include <os/task.h>
#include <os/string.h>
#include <os/bios.h>
#include <type.h>

#define SECTOR_SIZE 512
#define APP_START_ADDR 0x52000000
#define APP_INFO_ADDR 0x502001f8
#define APP_NUM_ADDR 0x502001f4
#define ONE_INFO_SIZE 16
uint64_t load_task_img(char *task_name)
{
    /**
     * TODO:
     * 1. [p1-task3] load task from image via task id, and return its entrypoint
     * 2. [p1-task4] load task via task name, thus the arg should be 'char *taskname'
     */
    
    int taskid = strcmp(task_name, "bss")   == 0 ? 1 :
                 strcmp(task_name, "auipc") == 0 ? 2 :
                 strcmp(task_name, "data")  == 0 ? 3 :
                 strcmp(task_name, "2048")  == 0 ? 4 : 4;
    int app_num = (int)*((short *)APP_NUM_ADDR);
    int app_offset = app_num * 0x10000;
    int app_info_offset = (*((int *)APP_INFO_ADDR)) % SECTOR_SIZE;
    int *info_pos = (int *)(APP_START_ADDR + app_offset + app_info_offset + (taskid - 1) * ONE_INFO_SIZE);    
    int offset = *(info_pos + 3);
    int size = *(info_pos + 2);
    unsigned mem_address = 0x52000000 + (taskid - 1) * 0x10000;
    unsigned sector_start = offset / SECTOR_SIZE;
    unsigned sector_num = (size + offset) / SECTOR_SIZE - sector_start + 1;
    unsigned start_address = mem_address + offset - (sector_start * SECTOR_SIZE);

    bios_sdread(mem_address, sector_num, sector_start);
    return start_address;
}