#include <os/task.h>
#include <os/string.h>
#include <os/kernel.h>
#include <type.h>

#define SECTOR_SIZE 512
#define TASK_INFO_ADDR 0x502001f8
#define TASK_NUM_ADDR 0x502001f4
#define ONE_INFO_SIZE 32
#define BUF_START 0x55000000
#define BUF_SIZE 0x10000
#define NAME_LEN 24
#define SIZE_POS 6
#define OFFSET_POS 7
void load_task_img(int *task_num, task_info_t *task_info)
{
    int task_num_loader = (int)*((short *)TASK_NUM_ADDR);
    *task_num = task_num_loader;
    int app_offset = task_num_loader * TASK_SIZE;
    int app_info_offset = (*((int *)TASK_INFO_ADDR)) % SECTOR_SIZE;
    task_info_t *task_info_loader;

    for(int taskid = 0; taskid < task_num_loader; taskid++){
        task_info_loader = task_info + taskid;
        int *info_pos = (int *)(TASK_MEM_BASE + app_offset + app_info_offset + taskid * ONE_INFO_SIZE);    
        strcpy(task_info_loader->task_name, (char *)info_pos);
        int offset = *(info_pos + OFFSET_POS);
        int size = *(info_pos + SIZE_POS);
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
        
        task_info_loader->task_entry = mem_address;
    }

    return;
}