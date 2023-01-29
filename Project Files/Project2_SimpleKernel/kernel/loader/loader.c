#include <os/task.h>
#include <os/string.h>
#include <os/kernel.h>
#include <type.h>

#define SECTOR_SIZE 512
#define TASK_INFO_ADDR 0x502001f8
#define TASK_NUM_ADDR 0x502001f4
#define ONE_INFO_SIZE 16
#define BUF_START 0x55000000
#define BUF_SIZE 0x10000
void load_task_img(int *task_num, unsigned *start_addr)
{
    int app_num = (int)*((short *)TASK_NUM_ADDR);
    *task_num = app_num;
    int app_offset = app_num * TASK_SIZE;
    int app_info_offset = (*((int *)TASK_INFO_ADDR)) % SECTOR_SIZE;

    for(int taskid = 0; taskid < app_num; taskid++){
        int *info_pos = (int *)(TASK_MEM_BASE + app_offset + app_info_offset + taskid * ONE_INFO_SIZE);    
        int offset = *(info_pos + 3);
        int size = *(info_pos + 2);
        unsigned sector_start = offset / SECTOR_SIZE;
        unsigned sector_num = (size + offset) / SECTOR_SIZE - sector_start + 1;
        unsigned offset_in_sector = offset - (sector_start * SECTOR_SIZE);
        /*
        unsigned mem_address = 0x52000000 + taskid * 0x10000;
        unsigned start_address = mem_address - offset_in_sector;
        bios_sdread(mem_address, sector_num, sector_start);
        memcpy(start_address, mem_address, size);
        */
        unsigned mem_address = 0x52000000 + taskid * TASK_SIZE;
        unsigned buf_addr = BUF_START + BUF_SIZE * taskid;
        unsigned start_address = buf_addr + offset_in_sector;
        bios_sdread(buf_addr, sector_num, sector_start);
        memcpy((uint8_t *)mem_address, (uint8_t *)start_address, (uint32_t)size);
        *(start_addr + taskid) = mem_address;
    }

    return;
}