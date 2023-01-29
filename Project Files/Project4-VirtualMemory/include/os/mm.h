/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                                   Memory Management
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */
#ifndef MM_H
#define MM_H

#include <type.h>

#define MEM_SIZE 32
#define PAGE_SIZE 4096 // 4K
#define SECTOR_ALIGN 8
#define SECTOR_NUM_IN_ONE_PAGE 8
#define SH_PG_NUM 16
#define SD_POS_BASE (1lu << 18) // 20-->512MB 16-->32MB
#define MAX_PAGE_NUM 1024 // 在此处调小最大物理页数目，进行换页测试
#define INIT_KERNEL_STACK 0x50500000
#define INIT_USER_STACK 0x52500000
#define INIT_KERNEL_STACK_KVA 0xffffffc050500000
#define INIT_FREE_VA 0xf00100000
/* FREEMEM_KERNEL里的第一个PAGE_SIZE分配给pid0 */
#define FREEMEM_KERNEL (INIT_KERNEL_STACK + 2 * PAGE_SIZE)
#define FREEMEM_USER (INIT_USER_STACK + PAGE_SIZE)

/* Rounding; only works for n = power of two */
// 向上取离a最近的能整除n的数
#define ROUND(a, n)     (((((uint64_t)(a))+(n)-1)) & ~((n)-1))
// 向下取离a最近的能整除n的数
#define ROUNDDOWN(a, n) (((uint64_t)(a)) & ~((n)-1))

static ptr_t kernMemCurr;
static ptr_t UserMemCurr;

typedef struct page_node{
    ptr_t page_vaddr; // 物理页框对应的虚地址
    uintptr_t pgdir;  // 虚地址所在的页表
    struct page_node *next;
    struct page_node *prev;
} page_node_t;

typedef struct page_list
{   
    page_node_t head_node;
    page_node_t *tail;
} page_list_t;

extern page_list_t page_queue;
extern int page_num;

extern uint32_t malloc_size;
extern uint64_t malloc_kva_base;

typedef struct share_page{
    int key;
    int num_using;
    ptr_t paddr;
} share_page_t;

share_page_t sh_pages[SH_PG_NUM];

void *kmalloc(size_t size);

ptr_t alloc_page_kernel(int numPage);
ptr_t alloc_one_page_kernel();

ptr_t alloc_page_user(int numPage);
ptr_t alloc_one_page_user();

uint64_t alloc_sd();

uintptr_t alloc_page_helper(uintptr_t va, uintptr_t pgdir);
void share_pgtable(uintptr_t dest_pgdir, uintptr_t src_pgdir);

void add_to_page_queue(ptr_t va, uintptr_t pgdir);
void head_out_page_queue();
void swap_to_disk(ptr_t va, uintptr_t pgdir, uint64_t sd_pos);

uintptr_t shm_page_get(int key);
void shm_page_dt(uintptr_t addr);
uintptr_t find_free_va(uintptr_t pgdir);
uintptr_t create_map(uintptr_t va, uintptr_t pa, uintptr_t pgdir);
void delete_map(uintptr_t va, uintptr_t pgdir);
void init_sh_pg();

uint64_t do_create_sp(uint64_t vaddr_u);
uint64_t do_get_pa(uint64_t vaddr_u);
#endif /* MM_H */
