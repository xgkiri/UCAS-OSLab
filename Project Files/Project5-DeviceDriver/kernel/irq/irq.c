#include <os/irq.h>
#include <os/time.h>
#include <os/sched.h>
#include <os/string.h>
#include <os/kernel.h>
#include <os/mm.h>
#include <printk.h>
#include <assert.h>
#include <screen.h>
#include <e1000.h>
#include <plic.h>
#include <pgtable.h>

#define SCAUSE_MASK 0x7FFFFFFFFFFFFFFF
#define SCAUSE_IRQ_FLAG (1UL << 63)

handler_t irq_table[IRQC_COUNT];
handler_t exc_table[EXCC_COUNT];

void interrupt_helper(regs_context_t *regs, uint64_t stval, uint64_t scause)
{
    //printk("\nenter int helper\n");
    //printk("scause = 0x%lx\n", scause);
    // TODO: [p2-task3] & [p2-task4] interrupt handler.
    // call corresponding handler by the value of `scause`
    if(scause & SCAUSE_IRQ_FLAG){
        irq_table[scause & SCAUSE_MASK](regs, stval, scause);
    }
    else{
        exc_table[scause & SCAUSE_MASK](regs, stval, scause);
    }
}

void handle_irq_timer(regs_context_t *regs, uint64_t stval, uint64_t scause)
{
    //printk("\nenter irq time handler\n");
    core_id = get_current_cpu_id();
    // TODO: [p2-task4] clock interrupt handler.
    // Note: use bios_set_timer to reset the timer and remember to reschedule
    check_sleeping();
    //check_net_send();
    //check_net_recv();
    uint64_t current_ticks = get_ticks();
    uint64_t next_time_int_ticks = current_ticks + TIMER_INTERVAL;
    bios_set_timer(next_time_int_ticks);
    if(current_ticks > current_running[core_id]->end_ticks){
        do_scheduler();
    }
}  

void handle_page_fault(regs_context_t *regs, uint64_t stval, uint64_t scause){
    // 直接调用alloc_page_helper即可
    // 具体例外情况由alloc_page_helper判断并加以处理
    core_id = get_current_cpu_id();
    uintptr_t va = (uintptr_t)stval;
    uintptr_t pgdir = current_running[core_id]->pgdir;
    alloc_page_helper(va, pgdir);
    return;
}

void handle_irq_ext(regs_context_t *regs, uint64_t stval, uint64_t scause)
{
    //printk("\nenter handle irq ext\n");
    // TODO: [p5-task4] external interrupt handler.
    // Note: plic_claim and plic_complete will be helpful ...
    int dev_id = plic_claim();
    //printk("dev id = %d\n", dev_id);
    if(dev_id != PLIC_E1000_PYNQ_IRQ){
        plic_complete(dev_id);
        return;
    }
    int reg_icr = e1000_read_reg(e1000, E1000_ICR);
    //printk("reg_icr = %d\n", reg_icr);
    int reg_ims = e1000_read_reg(e1000, E1000_IMS);
    //printk("reg_ims = %d\n", reg_ims);
    if((reg_icr & reg_ims) == E1000_ICR_TXQE){
        //printk("handle_txqe\n");
        e1000_handle_txqe();
    }
    else if((reg_icr & reg_ims) == E1000_ICR_RXDMT0){
        //printk("handle_rxdmt0\n");
        e1000_handle_rxdmt0();
    }
    plic_complete(dev_id);
}

void init_exception()
{
    /* TODO: [p2-task3] initialize exc_table */
    /* NOTE: handle_syscall, handle_other, etc.*/
    exc_table[EXCC_INST_MISALIGNED]     = handle_other;
    exc_table[EXCC_INST_ACCESS]         = handle_other;
    exc_table[EXCC_BREAKPOINT]          = handle_other;
    exc_table[EXCC_LOAD_ACCESS]         = handle_other;
    exc_table[EXCC_STORE_ACCESS]        = handle_other;

    exc_table[EXCC_SYSCALL]             = handle_syscall;

    exc_table[EXCC_INST_PAGE_FAULT]     = handle_page_fault;
    exc_table[EXCC_LOAD_PAGE_FAULT]     = handle_page_fault;
    exc_table[EXCC_STORE_PAGE_FAULT]    = handle_page_fault;
    
    /* TODO: [p2-task4] initialize irq_table */
    /* NOTE: handle_int, handle_other, etc.*/
    irq_table[IRQC_U_SOFT]              = handle_other;
    irq_table[IRQC_S_SOFT]              = handle_other;
    irq_table[IRQC_M_SOFT]              = handle_other;
    irq_table[IRQC_U_TIMER]             = handle_other;
    irq_table[IRQC_S_TIMER]             = handle_irq_timer;
    irq_table[IRQC_M_TIMER]             = handle_other;
    irq_table[IRQC_U_EXT]               = handle_other;
    irq_table[IRQC_S_EXT]               = handle_irq_ext;
    irq_table[IRQC_M_EXT]               = handle_irq_ext;
    /* TODO: [p2-task3] set up the entrypoint of exceptions */
    /* In trap.S */
    setup_exception();
}

void handle_other(regs_context_t *regs, uint64_t stval, uint64_t scause)
{
    //printk("\nenter handle other\n");
    return;
    /*
    char* reg_name[] = {
        "zero "," ra  "," sp  "," gp  "," tp  ",
        " t0  "," t1  "," t2  ","s0/fp"," s1  ",
        " a0  "," a1  "," a2  "," a3  "," a4  ",
        " a5  "," a6  "," a7  "," s2  "," s3  ",
        " s4  "," s5  "," s6  "," s7  "," s8  ",
        " s9  "," s10 "," s11 "," t3  "," t4  ",
        " t5  "," t6  "
    };
    for (int i = 0; i < 32; i += 3) {
        for (int j = 0; j < 3 && i + j < 32; ++j) {
            //printk("%s : %016lx ",reg_name[i+j], regs->regs[i+j]);
        }
        //printk("\n\r");
    }
    //printk("sstatus: 0x%lx sbadaddr: 0x%lx scause: %lu\n\r",
           regs->sstatus, regs->sbadaddr, regs->scause);
    //printk("sepc: 0x%lx\n\r", regs->sepc);
    //printk("tval: 0x%lx cause: 0x%lx\n", stval, scause);
    assert(0);
    */
}
