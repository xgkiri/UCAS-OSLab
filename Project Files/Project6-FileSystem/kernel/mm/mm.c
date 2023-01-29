#include <os/mm.h>
#include <os/string.h>
#include <os/kernel.h>
#include <printk.h>
#include <assert.h>
#include <pgtable.h>

static ptr_t kernMemCurr = FREEMEM_KERNEL;
static ptr_t UserMemCurr = FREEMEM_USER;
static uint64_t sd_pos_base = SD_POS_BASE; // TODO: sd卡中储存数据的初始sector位置

page_list_t page_queue = {
    .head_node = {
        .next = NULL,
        .prev = NULL,
        .page_vaddr = -1,
        .pgdir = -1
    },
    .tail = &(page_queue.head_node)
};

int page_num = 0;

uint32_t malloc_size = 0;
uint64_t malloc_kva_base;

void freePage(ptr_t baseAddr)
{
    // TODO [P4-task1] (design you 'freePage' here if you need):
}

void *kmalloc(size_t size)
{
    // TODO [P4-task1] (design you 'kmalloc' here if you need):
    uint64_t malloc_kva;
    if(malloc_size == 0 || malloc_size + size > PAGE_SIZE){
        malloc_kva_base = pa2kva(alloc_one_page_kernel());
        malloc_size = 0;
    }
    malloc_kva = malloc_kva_base + malloc_size;
    malloc_size += size;
    return (void *)malloc_kva;
}

ptr_t alloc_page_kernel(int numPage)
{
    // align PAGE_SIZE
    // 分配numpage页，返回分配的第一页的物理地址
    ptr_t ret = ROUND(kernMemCurr, PAGE_SIZE);
    kernMemCurr = ret + numPage * PAGE_SIZE;
    return ret;
}

// 返回一页空闲内核页的物理地址
ptr_t alloc_one_page_kernel(){
    return alloc_page_kernel(1);
}

ptr_t alloc_page_user(int numPage)
{
    // align PAGE_SIZE
    // 分配numpage页，返回分配的第一页的物理地址
    // TODO: 在此处加入无空闲物理页的判断机制，并进行换页操作
    uintptr_t pgdir;
    uintptr_t vaddr_to_swap;
    uint64_t sd_pos;
    while(page_num + numPage > MAX_PAGE_NUM){
        pgdir = page_queue.head_node.pgdir;
        vaddr_to_swap = page_queue.head_node.page_vaddr;
        head_out_page_queue();
        sd_pos = alloc_sd();
        //printk("swap page 0x%lx to disk\n", get_pa_of(vaddr_to_swap, pgdir));
        swap_to_disk(vaddr_to_swap, pgdir, sd_pos);
        page_num--;
    }
    ptr_t ret = ROUND(UserMemCurr, PAGE_SIZE);
    UserMemCurr = ret + numPage * PAGE_SIZE;
    return ret;
}

// 返回一页空闲用户页的物理地址
ptr_t alloc_one_page_user(){
    return alloc_page_user(1);
}

uint64_t alloc_sd(){
    uint64_t sd_pos = ROUND(sd_pos_base, SECTOR_ALIGN);
    sd_pos_base = sd_pos + SECTOR_ALIGN;
    return sd_pos;
}

// va 为需要映射的虚拟地址，pgdir 为页表目录，
// 返回值为为 va 映射的物理地址对应的内核虚地址
uintptr_t alloc_page_helper(uintptr_t va, uintptr_t pgdir)
{
    // TODO [P4-task1] alloc_page_helper:
    // 一级页表：目录
    PTE *pgtab_level3_vaddr; 
    // 二级页表
    PTE *pgtab_level2_vaddr; 
    PTE *pgtab_level2_paddr;
    // 三级页表
    PTE *pgtab_level1_vaddr;
    PTE *pgtab_level1_paddr;
    // 物理页
    uintptr_t page_paddr;    

    uint64_t vpn2 = (va >> (NORMAL_PAGE_SHIFT + VPN_LEN + VPN_LEN)) & VPN_MASK;
    uint64_t vpn1 = (va >> (NORMAL_PAGE_SHIFT + VPN_LEN)) & VPN_MASK;
    uint64_t vpn0 = (va >> NORMAL_PAGE_SHIFT) & VPN_MASK;

    uint64_t sd_pos;
    uintptr_t paddr_to_copy;

    // 实地址用来填入PTE
    // 虚地址用来访问PTE
    pgtab_level3_vaddr = pgdir; // pgdir是目录的内核虚地址
    
    if(pgtab_level3_vaddr[vpn2] & _PAGE_PRESENT){
        pgtab_level2_paddr = get_pa(pgtab_level3_vaddr[vpn2]);
        pgtab_level2_vaddr = pa2kva(pgtab_level2_paddr);
    }
    else{
        pgtab_level2_paddr = alloc_one_page_kernel();
        pgtab_level2_vaddr = pa2kva(pgtab_level2_paddr);
        clear_pgdir(pgtab_level2_vaddr);
        set_pfn(&(pgtab_level3_vaddr[vpn2]), ((uint64_t)pgtab_level2_paddr >> NORMAL_PAGE_SHIFT));
        set_attribute(&(pgtab_level3_vaddr[vpn2]), _PAGE_PRESENT);
    }
    
    if(pgtab_level2_vaddr[vpn1] & _PAGE_PRESENT){
        pgtab_level1_paddr = get_pa(pgtab_level2_vaddr[vpn1]);
        pgtab_level1_vaddr = pa2kva(pgtab_level1_paddr);
    }
    else{
        pgtab_level1_paddr = alloc_one_page_kernel();
        pgtab_level1_vaddr = pa2kva(pgtab_level1_paddr);
        clear_pgdir(pgtab_level1_vaddr);
        set_pfn(&(pgtab_level2_vaddr[vpn1]), ((uint64_t)pgtab_level1_paddr >> NORMAL_PAGE_SHIFT));
        set_attribute(&(pgtab_level2_vaddr[vpn1]), _PAGE_PRESENT);
    }
    
    if(pgtab_level1_vaddr[vpn0] == 0){
    // 还没有分配物理页
        page_paddr = alloc_one_page_user(); // 以实地址填入叶子级页表中
        clear_pgdir(pa2kva(page_paddr));

        set_pfn(&(pgtab_level1_vaddr[vpn0]), (page_paddr >> NORMAL_PAGE_SHIFT));
        set_attribute(&(pgtab_level1_vaddr[vpn0]), _PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE |
                            _PAGE_EXEC | _PAGE_USER | _PAGE_ACCESSED | _PAGE_DIRTY);
        // 将对应的虚地址加入物理页管理队列中
        add_to_page_queue(va, pgdir);
    }
    else if((pgtab_level1_vaddr[vpn0] & _PAGE_PRESENT) == 0){
    // 物理页被换入sd卡，分配新的物理页，并把sd卡中的数据拷贝入该物理页
        page_paddr = alloc_one_page_user();
        clear_pgdir(pa2kva(page_paddr));
        // 拷贝数据
        sd_pos = get_pfn(pgtab_level1_vaddr[vpn0]);
        bios_sdread(page_paddr, SECTOR_NUM_IN_ONE_PAGE, sd_pos);
        // 设置新的pfn和控制位
        set_pfn(&(pgtab_level1_vaddr[vpn0]), (page_paddr >> NORMAL_PAGE_SHIFT));
        set_attribute(&(pgtab_level1_vaddr[vpn0]), _PAGE_PRESENT);
        // 将对应的虚地址重新加入物理页管理队列中
        add_to_page_queue(va, pgdir);
    }
    else if((pgtab_level1_vaddr[vpn0] & _PAGE_WRITE) == 0){
    // copy-on-write
        paddr_to_copy = get_pa_of(va, pgdir);
        // 分配新的物理页
        page_paddr = alloc_one_page_user();
        clear_pgdir(pa2kva(page_paddr));
        set_pfn(&(pgtab_level1_vaddr[vpn0]), (page_paddr >> NORMAL_PAGE_SHIFT));
        set_attribute(&(pgtab_level1_vaddr[vpn0]), _PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE |
                            _PAGE_EXEC | _PAGE_USER | _PAGE_ACCESSED | _PAGE_DIRTY);
        // 将数据拷贝到新的物理页
        memcpy(pa2kva(paddr_to_copy), pa2kva(page_paddr), PAGE_SIZE);
        // 将对应的虚地址重新加入物理页管理队列中
        add_to_page_queue(va, pgdir);
    }

    local_flush_tlb_all();
    return pa2kva(page_paddr);
}

// 在指定的虚实地址之间建立映射关系
// 返回值为为 va 映射的物理地址对应的内核虚地址
uintptr_t create_map(uintptr_t va, uintptr_t pa, uintptr_t pgdir)
{
    // 一级页表：目录
    PTE *pgtab_level3_vaddr; 
    // 二级页表
    PTE *pgtab_level2_vaddr; 
    PTE *pgtab_level2_paddr;
    // 三级页表
    PTE *pgtab_level1_vaddr;
    PTE *pgtab_level1_paddr;
    // 物理页
    uintptr_t page_paddr;    

    uint64_t vpn2 = (va >> (NORMAL_PAGE_SHIFT + VPN_LEN + VPN_LEN)) & VPN_MASK;
    uint64_t vpn1 = (va >> (NORMAL_PAGE_SHIFT + VPN_LEN)) & VPN_MASK;
    uint64_t vpn0 = (va >> NORMAL_PAGE_SHIFT) & VPN_MASK;

    // 实地址用来填入PTE
    // 虚地址用来访问PTE
    pgtab_level3_vaddr = pgdir; // pgdir是目录的内核虚地址
    
    if(pgtab_level3_vaddr[vpn2] & _PAGE_PRESENT){
        pgtab_level2_paddr = get_pa(pgtab_level3_vaddr[vpn2]);
        pgtab_level2_vaddr = pa2kva(pgtab_level2_paddr);
    }
    else{
        pgtab_level2_paddr = alloc_one_page_kernel();
        pgtab_level2_vaddr = pa2kva(pgtab_level2_paddr);
        clear_pgdir(pgtab_level2_vaddr);
        set_pfn(&(pgtab_level3_vaddr[vpn2]), ((uint64_t)pgtab_level2_paddr >> NORMAL_PAGE_SHIFT));
        set_attribute(&(pgtab_level3_vaddr[vpn2]), _PAGE_PRESENT);
    }
    
    if(pgtab_level2_vaddr[vpn1] & _PAGE_PRESENT){
        pgtab_level1_paddr = get_pa(pgtab_level2_vaddr[vpn1]);
        pgtab_level1_vaddr = pa2kva(pgtab_level1_paddr);
    }
    else{
        pgtab_level1_paddr = alloc_one_page_kernel();
        pgtab_level1_vaddr = pa2kva(pgtab_level1_paddr);
        clear_pgdir(pgtab_level1_vaddr);
        set_pfn(&(pgtab_level2_vaddr[vpn1]), ((uint64_t)pgtab_level1_paddr >> NORMAL_PAGE_SHIFT));
        set_attribute(&(pgtab_level2_vaddr[vpn1]), _PAGE_PRESENT);
    }
    
    // 第一次建立映射，必然还没有分配物理页
    // 使用给定的物理地址
    page_paddr = pa;
    // 只建立映射，不在此处清空物理页
    set_pfn(&(pgtab_level1_vaddr[vpn0]), (page_paddr >> NORMAL_PAGE_SHIFT));
    set_attribute(&(pgtab_level1_vaddr[vpn0]), _PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE |
                        _PAGE_EXEC | _PAGE_USER | _PAGE_ACCESSED | _PAGE_DIRTY);
    // 将对应的虚地址加入物理页管理队列中
    //add_to_page_queue(va, pgdir);
    
    local_flush_tlb_all();
    return pa2kva(page_paddr);
}

// 取消va的映射
// 只是取消映射，不在此处进行物理页的释放
void delete_map(uintptr_t va, uintptr_t pgdir){
    // VPN2 VPN1 VPN0 控制位
    uint64_t vpn2 = (va >> (NORMAL_PAGE_SHIFT + VPN_LEN + VPN_LEN)) & VPN_MASK;
    uint64_t vpn1 = (va >> (NORMAL_PAGE_SHIFT + VPN_LEN)) & VPN_MASK;
    uint64_t vpn0 = (va >> NORMAL_PAGE_SHIFT) & VPN_MASK;
    
    // 3 -> 2 -> 1
    PTE *pgtab_level_3;
    PTE *pgtab_level_2;
    PTE *pgtab_level_1;

    pgtab_level_3 = (PTE *)pgdir;
    pgtab_level_2 = (PTE *)pa2kva(get_pa(pgtab_level_3[vpn2]));
    pgtab_level_1 = (PTE *)pa2kva(get_pa(pgtab_level_2[vpn1]));

    pgtab_level_1[vpn0] = 0;
}

// 用于把内核页表拷贝到用户程序的页表
void share_pgtable(uintptr_t dest_pgdir, uintptr_t src_pgdir)
{
    // TODO [P4-task1] share_pgtable:
    PTE *src = src_pgdir;
    PTE *dest = dest_pgdir;
    for(int i = 0; i < PTE_NUM; i++){
        if((src[i] & _PAGE_PRESENT) == 1){
            memcpy((uint8_t *)(dest + i), (uint8_t *)(src + i), sizeof(dest[i]));
        }
    }
    local_flush_tlb_all();
}

void add_to_page_queue(ptr_t va, uintptr_t pgdir){
    if(va == USER_STACK_VA - PAGE_SIZE){
        // 用户栈不换页
        return;
    }
    page_node_t *new_page_node;
    if(page_queue.head_node.page_vaddr == -1){
        page_queue.head_node.page_vaddr = va;
        page_queue.head_node.pgdir = pgdir;
    }
    else{
        new_page_node = kmalloc(sizeof(page_node_t));
        new_page_node->page_vaddr = va;
        new_page_node->pgdir = pgdir;
        new_page_node->next = page_queue.tail;
        new_page_node->prev = NULL;
        page_queue.tail->prev = new_page_node;
        page_queue.tail = new_page_node;
    }
    page_num++;
    return;
}

// FIFO
void head_out_page_queue(){
    if(page_queue.tail == &(page_queue.head_node)){
        // 单节点，置空
        page_queue.head_node.page_vaddr = -1;
        page_queue.head_node.pgdir = -1;
    }
    else{
        // 非单节点，新数据进入队头
        page_queue.head_node.page_vaddr = page_queue.head_node.prev->page_vaddr;
        page_queue.head_node.pgdir = page_queue.head_node.prev->pgdir;

        if(page_queue.head_node.prev->prev == NULL){
            // 仅两个结点，丢弃后继，tail指向头结点
            page_queue.head_node.prev = NULL;
            page_queue.tail = &(page_queue.head_node);
        }
        else{
            // 三个及以上结点，tail不变，丢弃后继即可
            page_queue.head_node.prev->prev->next = &(page_queue.head_node);
            page_queue.head_node.prev = page_queue.head_node.prev->prev;
        }
    }
}

void swap_to_disk(ptr_t va, uintptr_t pgdir, uint64_t sd_pos){
    // 调用此函数时说明空闲物理页已耗尽，需要将一页物理页的数据换入sd卡中，同时标记其为空闲
    ptr_t pa = get_pa_of(va, pgdir);
    set_swapped(va, pgdir, sd_pos);
    // TODO: 拷贝数据
    bios_sdwrite(pa, SECTOR_NUM_IN_ONE_PAGE, sd_pos);
    // TODO: 标记空闲
    clear_pgdir(pa2kva(pa));
}

void init_sh_pg(){
    for(int i = 0; i < SH_PG_NUM; i++){
        sh_pages[i].key = -1;
        sh_pages[i].num_using = 0;
    }
}

// 返回空闲用户虚地址
uintptr_t shm_page_get(int key)
{
    // TODO [P4-task4] shm_page_get:
    core_id = get_current_cpu_id();
    uintptr_t pgdir = current_running[core_id]->pgdir;
    uintptr_t sh_paddr;
    uintptr_t local_vaddr;
    for(int i = 0; i < SH_PG_NUM; i++){
        if(sh_pages[i].key == -1){
            sh_pages[i].key = key;
            sh_pages[i].num_using++;
            // 仅首次分配并清空共享页
            sh_pages[i].paddr = alloc_one_page_user();
            sh_paddr = sh_pages[i].paddr;
            clear_pgdir(pa2kva(sh_paddr));
            break;
        }
        else if(sh_pages[i].key == key){
            sh_pages[i].num_using++;
            sh_paddr = sh_pages[i].paddr;
            break;
        }
    }
    local_vaddr = find_free_va(pgdir);
    create_map(local_vaddr, sh_paddr, pgdir);
    local_flush_tlb_all();
    return local_vaddr;
}

void shm_page_dt(uintptr_t addr)
{
    // TODO [P4-task4] shm_page_dt:
    core_id = get_current_cpu_id();
    uintptr_t pgdir = current_running[core_id]->pgdir;
    uintptr_t sh_paddr;
    uintptr_t local_vaddr = addr;
    sh_paddr = get_pa_of(local_vaddr, pgdir);
    for(int i = 0; i < SH_PG_NUM; i++){
        if(sh_pages[i].paddr == sh_paddr){
            sh_pages[i].num_using--;
            delete_map(local_vaddr, pgdir);
            if(sh_pages[i].num_using == 0){
                sh_pages[i].key = -1;
                clear_pgdir(pa2kva(sh_paddr));
                // TODO: 标记物理页为空闲
            }
        }
    }
    local_flush_tlb_all();
}

uintptr_t find_free_va(uintptr_t pgdir){
    uintptr_t va = INIT_FREE_VA;
    for(int i = 0; ;i++){
        va += i * PAGE_SIZE;
        if(query_status(va, pgdir) == NO_MAPPING){
            return va;
        }
    }
}

uint64_t do_create_sp(uint64_t vaddr_u){
    core_id = get_current_cpu_id();
    uintptr_t pgdir = current_running[core_id]->pgdir;
    uintptr_t origin_va = vaddr_u; 
    // 给正本分配物理页
    if(query_status(origin_va, pgdir) == NO_MAPPING){
        alloc_page_helper(origin_va, pgdir);
        clear_attribute_out(origin_va, pgdir, _PAGE_WRITE);
    }
    uintptr_t origin_pa = get_pa_of(origin_va, pgdir);
    uintptr_t copy_va = find_free_va(pgdir);
    // 建立映射
    create_map(copy_va, origin_pa, pgdir);
    // 设置页只读
    clear_attribute_out(copy_va, pgdir, _PAGE_WRITE);
    return copy_va;
}

uint64_t do_get_pa(uint64_t vaddr_u){
    core_id = get_current_cpu_id();
    uintptr_t pgdir = current_running[core_id]->pgdir;
    return get_pa_of(vaddr_u, pgdir);
}