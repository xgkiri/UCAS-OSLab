#ifndef IOREMAP_H
#define IOREMAP_H

#include <type.h>

// using this as IO address space (at most using 1 GB, so that it can be store in one pgdir entry)
#define IO_ADDR_START 0xffffffe000000000lu

ptr_t alloc_vpage_io(int numPage);
extern void * ioremap(unsigned long phys_addr, unsigned long size);
extern void iounmap(void *io_addr);

#endif // IOREMAP_H