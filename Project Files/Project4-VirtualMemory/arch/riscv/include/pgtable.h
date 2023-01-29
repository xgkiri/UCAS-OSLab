#ifndef PGTABLE_H
#define PGTABLE_H

#include <type.h>

#define SATP_MODE_SV39 8
#define SATP_MODE_SV48 9

#define SATP_ASID_SHIFT 44lu
#define SATP_MODE_SHIFT 60lu

#define NORMAL_PAGE_SHIFT 12lu
#define NORMAL_PAGE_SIZE (1lu << NORMAL_PAGE_SHIFT) // 4KB
#define LARGE_PAGE_SHIFT 21lu
#define LARGE_PAGE_SIZE (1lu << LARGE_PAGE_SHIFT) // 2MB

#define PTE_SIZE 4 // 4B
#define PTE_NUM (NORMAL_PAGE_SIZE / PTE_SIZE) // 1024
#define KERNEL_PTE_NUM 128 // (0xffffffc050000000lu - 0xffffffc060000000lu) / 0x200000lu

typedef enum vpage_status {
    NO_MAPPING,
    SWAPPED,
    READ_ONLY
} vpage_status_t;

/*
 * Flush entire local TLB.  'sfence.vma' implicitly fences with the instruction
 * cache as well, so a 'fence.i' is not necessary.
 */
static inline void local_flush_tlb_all(void)
{
    __asm__ __volatile__ ("sfence.vma" : : : "memory");
}

/* Flush one page from local TLB */
static inline void local_flush_tlb_page(unsigned long addr)
{
    __asm__ __volatile__ ("sfence.vma %0" : : "r" (addr) : "memory");
}

static inline void local_flush_icache_all(void)
{
    asm volatile ("fence.i" ::: "memory");
}

static inline void set_satp(
    unsigned mode, unsigned asid, unsigned long ppn)
{
    unsigned long __v =
        (unsigned long)(((unsigned long)mode << SATP_MODE_SHIFT) | ((unsigned long)asid << SATP_ASID_SHIFT) | ppn);
    __asm__ __volatile__("sfence.vma\ncsrw satp, %0" : : "rK"(__v) : "memory");
}

#define KERNEL_PGDIR_PA 0x51000000lu  // use 51000000 page as PGDIR
#define KERNEL_PGDIR_VA 0xffffffc051000000lu

#define USER_STACK_VA 0xf00010000
#define USER_VA_BASE 0x10000

/*
 * PTE format:
 * | XLEN-1  10 | 9             8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0
 *       PFN      reserved for SW   D   A   G   U   X   W   R   V
 */

#define _PAGE_ACCESSED_OFFSET 6

#define _PAGE_PRESENT (1 << 0)
#define _PAGE_READ (1 << 1)     /* Readable */
#define _PAGE_WRITE (1 << 2)    /* Writable */
#define _PAGE_EXEC (1 << 3)     /* Executable */
#define _PAGE_USER (1 << 4)     /* User */
#define _PAGE_GLOBAL (1 << 5)   /* Global */
#define _PAGE_ACCESSED (1 << 6) /* Set by hardware on any access \
                                 */
#define _PAGE_DIRTY (1 << 7)    /* Set by hardware on any write */
#define _PAGE_SOFT (1 << 8)     /* Reserved for software */

#define _PAGE_PFN_SHIFT 10lu
#define ATTRIBUTE_MASK ((1lu << _PAGE_PFN_SHIFT) - 1) // 10个1，PTE的控制位

#define VA_MASK ((1lu << 39) - 1) // 39个1，虚地址的有效位数
#define PTE_MASK ((1lu << 56) - 1) // 56个1，实地址和PTE的有效位数

#define PPN_BITS 9lu

#define PPN_MASK_LOW ((1lu << 44) - 1) // 44个1，PPN的总长度
#define PPN_MASK_HIGH (PPN_MASK_LOW << _PAGE_PFN_SHIFT) // 高44位全1，低10位全0

#define VPN_LEN 9
#define VPN_MASK ((1lu << 9) - 1) // 9个1，vpn的长度

#define OFFSET_MASK ((1lu << 12) - 1)

#define NUM_PTE_ENTRY (1 << PPN_BITS)

#define KERNEL_VA_BASE 0xffffffc000000000

typedef uint64_t PTE;

/*
 * PTE format:
 * | XLEN-1  10 | 9             8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0
 *       PFN      reserved for SW   D   A   G   U   X   W   R   V
 */

/* Translation between physical addr and kernel virtual addr */
static inline uintptr_t kva2pa(uintptr_t kva)
{
    /* TODO: [P4-task1] */
    // 内核虚拟地址映射到物理地址：减去前缀即可
    return kva - KERNEL_VA_BASE;
}

static inline uintptr_t pa2kva(uintptr_t pa)
{
    /* TODO: [P4-task1] */
    // 物理地址映射到内核虚拟地址：加上前缀即可
    return pa + KERNEL_VA_BASE;
}

/* get physical page addr from PTE 'entry' */
static inline uint64_t get_pa(PTE entry)
{
    /* TODO: [P4-task1] */
    return (((entry >> _PAGE_PFN_SHIFT) & PPN_MASK_LOW) << NORMAL_PAGE_SHIFT);
}

/* Get/Set page frame number of the `entry` */
static inline long get_pfn(PTE entry)
{
    /* TODO: [P4-task1] */
    return (entry >> _PAGE_PFN_SHIFT);
}
static inline void set_pfn(PTE *entry, uint64_t pfn)
{
    /* TODO: [P4-task1] */
    *entry = (pfn << _PAGE_PFN_SHIFT) | (*entry & ATTRIBUTE_MASK);
}

/* Get/Set attribute(s) of the `entry` */
static inline long get_attribute(PTE entry, uint64_t mask)
{
    /* TODO: [P4-task1] */
    return (entry & mask);
}
static inline void set_attribute(PTE *entry, uint64_t bits)
{
    /* TODO: [P4-task1] */
    *entry = *entry | bits;
}

static inline void clear_attribute(PTE *entry, uint64_t bits)
{
    /* TODO: [P4-task1] */
    *entry = *entry & (~bits);
}

static inline void clear_pgdir(uintptr_t pgdir_addr)
{
    /* TODO: [P4-task1] */
    bzero(pgdir_addr, NORMAL_PAGE_SIZE);
}

/* 
 * 用目录的内核虚地址pgdir_va查询va的实地址
 * 返回其内核虚地址
 */
static inline uintptr_t get_kva_of(uintptr_t va, uintptr_t pgdir_va)
{
    // TODO: [P4-task1] (todo if you need)

    // VPN2 VPN1 VPN0 控制位
    uint64_t vpn2 = (va >> (NORMAL_PAGE_SHIFT + VPN_LEN + VPN_LEN)) & VPN_MASK;
    uint64_t vpn1 = (va >> (NORMAL_PAGE_SHIFT + VPN_LEN)) & VPN_MASK;
    uint64_t vpn0 = (va >> NORMAL_PAGE_SHIFT) & VPN_MASK;
    
    // 3 -> 2 -> 1
    PTE *pgtab_level_3;
    PTE *pgtab_level_2;
    PTE *pgtab_level_1;

    pgtab_level_3 = (PTE *)pgdir_va;
    pgtab_level_2 = (PTE *)pa2kva(get_pa(pgtab_level_3[vpn2]));
    pgtab_level_1 = (PTE *)pa2kva(get_pa(pgtab_level_2[vpn1]));

    return (uintptr_t)pa2kva(get_pa(pgtab_level_3[vpn0]));
}

/*
 * 用目录的内核虚地址pgdir_va查询va的实地址
 * 返回查询结果实地址
 */ 
static inline uintptr_t get_pa_of(uintptr_t va, uintptr_t pgdir_va)
{
    // VPN2 VPN1 VPN0 控制位
    uint64_t vpn2 = (va >> (NORMAL_PAGE_SHIFT + VPN_LEN + VPN_LEN)) & VPN_MASK;
    uint64_t vpn1 = (va >> (NORMAL_PAGE_SHIFT + VPN_LEN)) & VPN_MASK;
    uint64_t vpn0 = (va >> NORMAL_PAGE_SHIFT) & VPN_MASK;
    uint64_t offset = va & OFFSET_MASK;
    
    // 3 -> 2 -> 1
    PTE *pgtab_level_3;
    PTE *pgtab_level_2;
    PTE *pgtab_level_1;

    pgtab_level_3 = (PTE *)pgdir_va;
    pgtab_level_2 = (PTE *)pa2kva(get_pa(pgtab_level_3[vpn2]));
    pgtab_level_1 = (PTE *)pa2kva(get_pa(pgtab_level_2[vpn1]));

    return (uintptr_t)(get_pa(pgtab_level_1[vpn0]) + offset);
}

// 将被换出的物理页的valid位置0
// 将叶页表的ppn更换为sd卡中的储存位置
static inline void set_swapped(uintptr_t va, uintptr_t pgdir_va, uint64_t sd_pos)
{
    // VPN2 VPN1 VPN0 控制位
    uint64_t vpn2 = (va >> (NORMAL_PAGE_SHIFT + VPN_LEN + VPN_LEN)) & VPN_MASK;
    uint64_t vpn1 = (va >> (NORMAL_PAGE_SHIFT + VPN_LEN)) & VPN_MASK;
    uint64_t vpn0 = (va >> NORMAL_PAGE_SHIFT) & VPN_MASK;
    
    // 3 -> 2 -> 1
    PTE *pgtab_level_3;
    PTE *pgtab_level_2;
    PTE *pgtab_level_1;

    pgtab_level_3 = (PTE *)pgdir_va;
    pgtab_level_2 = (PTE *)pa2kva(get_pa(pgtab_level_3[vpn2]));
    pgtab_level_1 = (PTE *)pa2kva(get_pa(pgtab_level_2[vpn1]));

    clear_attribute(&(pgtab_level_1[vpn0]), _PAGE_PRESENT);
    set_pfn(&(pgtab_level_1[vpn0]), sd_pos);
}

// 查询虚地址状态
static inline int query_status(uintptr_t va, uintptr_t pgdir_va)
{
    // VPN2 VPN1 VPN0 控制位
    uint64_t vpn2 = (va >> (NORMAL_PAGE_SHIFT + VPN_LEN + VPN_LEN)) & VPN_MASK;
    uint64_t vpn1 = (va >> (NORMAL_PAGE_SHIFT + VPN_LEN)) & VPN_MASK;
    uint64_t vpn0 = (va >> NORMAL_PAGE_SHIFT) & VPN_MASK;
    
    // 3 -> 2 -> 1
    PTE *pgtab_level_3;
    PTE *pgtab_level_2;
    PTE *pgtab_level_1;

    pgtab_level_3 = (PTE *)pgdir_va;
    
    if(pgtab_level_3[vpn2] == 0){
        return NO_MAPPING;
    }
    else{
        pgtab_level_2 = (PTE *)pa2kva(get_pa(pgtab_level_3[vpn2]));
    }

    if(pgtab_level_2[vpn1] == 0){
        return NO_MAPPING;
    }
    else{
        pgtab_level_1 = (PTE *)pa2kva(get_pa(pgtab_level_2[vpn1]));
    }

    if(pgtab_level_1[vpn0] == 0){
        return NO_MAPPING;
    }
    else if((pgtab_level_1[vpn0] & _PAGE_PRESENT) == 0){
        return SWAPPED;
    }
    else if((pgtab_level_1[vpn0] & _PAGE_WRITE) == 0){
        return READ_ONLY;
    }
}

// 用于直接设置虚地址在页表中对应pte的控制位
static inline int set_attribute_out(uintptr_t va, uintptr_t pgdir_va, uint64_t bits)
{
    // VPN2 VPN1 VPN0 控制位
    uint64_t vpn2 = (va >> (NORMAL_PAGE_SHIFT + VPN_LEN + VPN_LEN)) & VPN_MASK;
    uint64_t vpn1 = (va >> (NORMAL_PAGE_SHIFT + VPN_LEN)) & VPN_MASK;
    uint64_t vpn0 = (va >> NORMAL_PAGE_SHIFT) & VPN_MASK;
    
    // 3 -> 2 -> 1
    PTE *pgtab_level_3;
    PTE *pgtab_level_2;
    PTE *pgtab_level_1;

    pgtab_level_3 = (PTE *)pgdir_va;
    pgtab_level_2 = (PTE *)pa2kva(get_pa(pgtab_level_3[vpn2]));
    pgtab_level_1 = (PTE *)pa2kva(get_pa(pgtab_level_2[vpn1]));

    set_attribute(&(pgtab_level_1[vpn0]), bits);
}

// 用于直接设置虚地址在页表中对应pte的控制位
static inline int clear_attribute_out(uintptr_t va, uintptr_t pgdir_va, uint64_t bits)
{
    // VPN2 VPN1 VPN0 控制位
    uint64_t vpn2 = (va >> (NORMAL_PAGE_SHIFT + VPN_LEN + VPN_LEN)) & VPN_MASK;
    uint64_t vpn1 = (va >> (NORMAL_PAGE_SHIFT + VPN_LEN)) & VPN_MASK;
    uint64_t vpn0 = (va >> NORMAL_PAGE_SHIFT) & VPN_MASK;
    
    // 3 -> 2 -> 1
    PTE *pgtab_level_3;
    PTE *pgtab_level_2;
    PTE *pgtab_level_1;

    pgtab_level_3 = (PTE *)pgdir_va;
    pgtab_level_2 = (PTE *)pa2kva(get_pa(pgtab_level_3[vpn2]));
    pgtab_level_1 = (PTE *)pa2kva(get_pa(pgtab_level_2[vpn1]));

    clear_attribute(&(pgtab_level_1[vpn0]), bits);
}
#endif  // PGTABLE_H
