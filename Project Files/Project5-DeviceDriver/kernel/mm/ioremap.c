#include <os/ioremap.h>
#include <os/mm.h>
#include <pgtable.h>
#include <type.h>
#include <printk.h>

// maybe you can map it to IO_ADDR_START ?
static uintptr_t io_base = IO_ADDR_START;

// 分配numpage页，返回分配的第一页的内核虚地址
ptr_t alloc_vpage_io(int numPage)
{
    ptr_t ret = ROUND(io_base, PAGE_SIZE);
    io_base = ret + numPage * PAGE_SIZE;
    return ret;
}

// 动态分配空闲内核虚地址完成映射
// 返回起始内核虚地址
void *ioremap(unsigned long phys_addr, unsigned long size)
{
    // TODO: [p5-task1] map one specific physical region to virtual address
    uintptr_t pa = phys_addr;
    uintptr_t pgdir = KERNEL_PGDIR_VA; // 内核页表
    int page_num = size / NORMAL_PAGE_SIZE;
    uintptr_t va_base = alloc_vpage_io(page_num);
    uintptr_t va = va_base;
    for(int i = 0; i < page_num; i++){
        create_map(va, pa, pgdir);
        pa += NORMAL_PAGE_SIZE;
        va += NORMAL_PAGE_SIZE;
    }
    local_flush_tlb_all();
    return va_base;
}

void iounmap(void *io_addr)
{
    // TODO: [p5-task1] a very naive iounmap() is OK
    // maybe no one would call this function?
}
