#include <os/task.h>
#include <os/string.h>
#include <os/kernel.h>
#include <os/mm.h>
#include <type.h>

#define SECTOR_SIZE 512
#define TASK_INFO_ADDR 0x502001f8
#define TASK_NUM_ADDR 0x502001f4
#define BUF_START 0x55000000
#define BUF_SIZE 0x10000
#define NAME_LEN 24
#define SIZE_POS 6
#define OFFSET_POS 7

// 原本loader的作用是：获取task_info并加载所有task，返回入口地址即可

// 现在loader的作用被拆分：由一个新的函数获取所有task_info，并在需要启动一个task时，把信息传给loader
// 然后loader利用one task_info加载task，并建立虚存映射

void get_task_info(task_info_t *task_info){
    uint64_t task_info_va = (pa2kva(TASK_NUM_ADDR));
    short task_num = *(short *)task_info_va;
    int image_offset = task_num * TASK_SIZE + (*((int *)TASK_INFO_ADDR)) % SECTOR_SIZE;
    task_info_t *task_info_base = TASK_MEM_BASE + image_offset;
    task_info_t *task_info_pos;

    for(int taskidx = 0; taskidx < task_num; taskidx++){
        task_info_pos = (task_info_t *)(pa2kva(task_info_base + taskidx));
        memcpy((uint8_t *)(task_info + taskidx), (uint8_t *)(task_info_pos), sizeof(task_info_t));
    }
}

// 根据task_info建立页表
// 将task加载进内存
// 返回页表目录的内核虚地址
uintptr_t load_task(task_info_t *task_info){
    int va_base;
    int va;
    int page_num;
    uintptr_t pgdir_va;
    uintptr_t vaddr_to_load_task; 

// 加载task到buf处，之后一边分配页表，一边将4k的数据拷贝到对应物理地址
    int sector_start = task_info->offset / SECTOR_SIZE;
    int sector_num = (task_info->filesz + task_info->offset) / SECTOR_SIZE - sector_start + 1;
    int offset_in_sector = task_info->offset - (sector_start * SECTOR_SIZE);
    static uintptr_t buf_paddr = BUF_START;
    uintptr_t start_vaddr = pa2kva(buf_paddr + offset_in_sector);
    bios_sdread(buf_paddr, sector_num, sector_start);
    bzero((uint8_t *)(start_vaddr + task_info->filesz), (uint32_t)(task_info->memsz - task_info->filesz));
    buf_paddr += BUF_SIZE;

// 建立页表
    va_base = USER_VA_BASE;
    page_num = task_info->memsz / PAGE_SIZE + 1;
    pgdir_va = pa2kva(alloc_one_page_kernel());
    clear_pgdir(pgdir_va);
    // 先拷贝内核页表
    share_pgtable(pgdir_va, KERNEL_PGDIR_VA);
    // 再建立映射
    for(int i = 0; i < page_num; i++){
        va = va_base + i * PAGE_SIZE;
        if(i != page_num - 1){
            vaddr_to_load_task = alloc_page_helper(va, pgdir_va);
            // 每次拷贝4KB数据到物理地址
            memcpy((uint8_t *)vaddr_to_load_task, (uint8_t *)start_vaddr, PAGE_SIZE);
            start_vaddr += PAGE_SIZE;
        }
        else{
            vaddr_to_load_task = alloc_page_helper(va, pgdir_va);
            memcpy((uint8_t *)vaddr_to_load_task, (uint8_t *)start_vaddr, (uint32_t)(task_info->memsz - (page_num - 1) * PAGE_SIZE));
        }
    }
    
    local_flush_tlb_all();
    return pgdir_va;
}