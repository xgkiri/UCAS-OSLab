/* RISC-V kernel boot stage */
#include <pgtable.h>
#include <asm.h>

typedef void (*kernel_entry_t)(unsigned long);

/********* setup memory mapping ***********/
static uintptr_t alloc_page()
{
    // 每次分配4KB，加4KB
    static uintptr_t pg_base = KERNEL_PGDIR_PA;
    pg_base += 0x1000;
    return pg_base;
}

// using 2MB large page
static void map_page(uint64_t va, uint64_t pa, PTE *pgdir)
{
    // mask虚地址，获取vpn2和vpn1
    va &= VA_MASK;
    uint64_t vpn2 =
        va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    uint64_t vpn1 = (vpn2 << PPN_BITS) ^
                    (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    // 给目录中的一项PTE分配物理页框
    if (pgdir[vpn2] == 0) {
        // alloc a new second-level page directory
        // PPN
        set_pfn(&pgdir[vpn2], alloc_page() >> NORMAL_PAGE_SHIFT);
        // 控制位
        set_attribute(&pgdir[vpn2], _PAGE_PRESENT);
        // 注意此时未开启虚存，故使用实地址来清空
        // 清空指向的二级页表
        clear_pgdir(get_pa(pgdir[vpn2]));
    }
    // pmd是二级页表的物理地址
    PTE *pmd = (PTE *)get_pa(pgdir[vpn2]);
    // PPN
    set_pfn(&pmd[vpn1], pa >> NORMAL_PAGE_SHIFT);
    // 控制位
    set_attribute(
        &pmd[vpn1], _PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE |
                        _PAGE_EXEC | _PAGE_ACCESSED | _PAGE_DIRTY);
    // 大页 -> 此时就是叶结点了，所以不存在下一级页表，自然不用清空
    // 12 + 9 = 21 = 2MB
}

static void enable_vm()
{
    // write satp to enable paging
    set_satp(SATP_MODE_SV39, 0, KERNEL_PGDIR_PA >> NORMAL_PAGE_SHIFT);
    // 开启虚存后，全部刷新TLB
    local_flush_tlb_all();
}

/* Sv-39 mode
 * 0x0000_0000_0000_0000-0x0000_003f_ffff_ffff is for user mode
 * 0xffff_ffc0_0000_0000-0xffff_ffff_ffff_ffff is for kernel mode
 */
static void setup_vm()
{
    // 清空内核页表
    clear_pgdir(KERNEL_PGDIR_PA);
    // map kernel virtual address(kva) to kernel physical
    // address(kpa) kva = kpa + 0xffff_ffc0_0000_0000 use 2MB page,
    // map all physical memory
    // 内核页表地址
    PTE *early_pgdir = (PTE *)KERNEL_PGDIR_PA;
    // 内核地址映射
    for (uint64_t kva = 0xffffffc050000000lu;
         kva < 0xffffffc060000000lu; kva += 0x200000lu) {
        map_page(kva, kva2pa(kva), early_pgdir);
    }
    // map boot address
    // 对于boot_kernel所使用的空间，先进行恒同映射，虚地址直接访问实地址
    for (uint64_t pa = 0x50000000lu; pa < 0x51000000lu;
         pa += 0x200000lu) {
        map_page(pa, pa, early_pgdir);
    }
    // 开启虚存
    enable_vm();
}

extern uintptr_t _start[];

/*********** start here **************/
int boot_kernel(unsigned long mhartid)
{
    //**************** 2 ****************

    // 主核初始化内核页表，打开虚存
    // 从核只需要打开虚存
    if (mhartid == 0) {
        setup_vm();
    } else {
        enable_vm();
    }

    /* go to kernel */
    ((kernel_entry_t)pa2kva(_start))(mhartid);

    return 0;
}
