#include <e1000.h>
#include <type.h>
#include <os/string.h>
#include <os/time.h>
#include <os/sched.h>
#include <os/smp.h>
#include <assert.h>
#include <pgtable.h>
#include <printk.h>

// E1000 Registers Base Pointer
volatile uint8_t *e1000;  // use virtual memory address

// E1000 Tx & Rx Descriptors
static struct e1000_tx_desc tx_desc_array[TXDESCS] __attribute__((aligned(16)));
static struct e1000_rx_desc rx_desc_array[RXDESCS] __attribute__((aligned(16)));

// E1000 Tx & Rx packet buffer
static char tx_pkt_buffer[TXDESCS][TX_PKT_SIZE];
static char rx_pkt_buffer[RXDESCS][RX_PKT_SIZE];

// Fixed Ethernet MAC Address of E1000
static const uint8_t enetaddr[6] = {0x00, 0x0a, 0x35, 0x00, 0x1e, 0x53};

/**
 * e1000_reset - Reset Tx and Rx Units; mask and clear all interrupts.
 **/
static void e1000_reset(void)
{
	/* Turn off the ethernet interface */
    e1000_write_reg(e1000, E1000_RCTL, 0);
    e1000_write_reg(e1000, E1000_TCTL, 0);

	/* Clear the transmit ring */
    e1000_write_reg(e1000, E1000_TDH, 0);
    e1000_write_reg(e1000, E1000_TDT, 0);

	/* Clear the receive ring */
    e1000_write_reg(e1000, E1000_RDH, 0);
    e1000_write_reg(e1000, E1000_RDT, 0);

	/**
     * Delay to allow any outstanding PCI transactions to complete before
	 * resetting the device
	 */
    latency(1);

	/* Clear interrupt mask to stop board from generating interrupts */
    e1000_write_reg(e1000, E1000_IMC, 0xffffffff);

    /* Clear any pending interrupt events. */
    while (0 != e1000_read_reg(e1000, E1000_ICR)) ;
}

/**
 * e1000_configure_tx - Configure 8254x Transmit Unit after Reset
 **/
static void e1000_configure_tx(void)
{
    //printl("\n\ninfo in e1000_configure_tx-----------------:\n\n");
    /* TODO: [p5-task1] Initialize tx descriptors */
    for(int i = 0; i < TXDESCS; i++){
        tx_desc_array[i].addr = kva2pa(tx_pkt_buffer[i]);// 填入物理地址
        tx_desc_array[i].length = 0;// 内容长度初始化为0
        tx_desc_array[i].cmd = tx_desc_array[i].cmd & ~E1000_TXD_CMD_DEXT | E1000_TXD_CMD_EOP;
        if(i == 0){
            //printl("CMD = 0x%lx\n", tx_desc_array[i].cmd);
        }
    }

    /* TODO: [p5-task1] Set up the Tx descriptor base address and length */
    // 填入物理地址
    e1000_write_reg(e1000, E1000_TDBAL, (uint32_t)kva2pa(tx_desc_array));
    //uint32_t reg_tdbal = e1000_read_reg(e1000, E1000_TDBAL);
    //printl("E1000_TDBAL = 0x%lx\n", reg_tdbal);

    e1000_write_reg(e1000, E1000_TDBAH, (uint32_t)(kva2pa(tx_desc_array) >> 32));
    //uint32_t reg_tdbah = e1000_read_reg(e1000, E1000_TDBAH);
    //printl("E1000_TDBAH = 0x%lx\n", reg_tdbah);

    e1000_write_reg(e1000, E1000_TDLEN, TXDESCS * sizeof(struct e1000_tx_desc));
    //uint32_t reg_tdlen = e1000_read_reg(e1000, E1000_TDLEN);
    //printl("E1000_TDLEN = 0x%lx\n", reg_tdlen);

	/* TODO: [p5-task1] Set up the HW Tx Head and Tail descriptor pointers */
    // 填入索引
    // 初始化时head = tail = 最高处
    // 之后软件移动tail，硬件移动head，都是向上移动？两者空出来的部分就是硬件待处理的包
    e1000_write_reg(e1000, E1000_TDT, 0);
    //uint32_t reg_tdt = e1000_read_reg(e1000, E1000_TDT);
    //printl("E1000_TDT = 0x%lx\n", reg_tdt);

    e1000_write_reg(e1000, E1000_TDH, 0);
    //uint32_t reg_tdh = e1000_read_reg(e1000, E1000_TDH);
    //printl("E1000_TDH = 0x%lx\n", reg_tdh);

    /* TODO: [p5-task1] Program the Transmit Control Register */
    e1000_write_reg(e1000, E1000_TCTL, 
                    (1 << E1000_TCTL_EN_OFFSET | 1 << E1000_TCTL_PSP_OFFSET |
                    0x10 << E1000_TCTL_CT_OFFSET | 0x40 << E1000_TCTL_COLD_OFFSET));
    //uint32_t reg_tctl = e1000_read_reg(e1000, E1000_TCTL);
    //printl("E1000_TCTL = 0x%lx\n", reg_tctl);
    
    e1000_write_reg(e1000, E1000_IMS, E1000_IMS_TXQE);
    //uint32_t reg_ims = e1000_read_reg(e1000, E1000_IMS);
    //printl("E1000_IMS = 0x%lx\n", reg_ims);

    local_flush_dcache();
}

/**
 * e1000_configure_rx - Configure 8254x Receive Unit after Reset
 **/
static void e1000_configure_rx(void)
{
    //printl("\n\ninfo in e1000_configure_rx-----------------:\n\n");
    /* TODO: [p5-task2] Set e1000 MAC Address to RAR[0] */
    e1000_write_reg_array(e1000, E1000_RA, 0, 
                            enetaddr[0] | enetaddr[1] << 8 * 1 | enetaddr[2] << 8 * 2 | enetaddr[3] << 8 * 3);
    //uint32_t reg_ral = e1000_read_reg_array(e1000, E1000_RA, 0);
    //printl("E1000_RAL = 0x%x\n", reg_ral);

    e1000_write_reg_array(e1000, E1000_RA, 1, E1000_RAH_AV | enetaddr[4] | enetaddr[5] << 8 * 1);
    //uint32_t reg_rah = e1000_read_reg_array(e1000, E1000_RA, 1);
    //printl("E1000_RAH = 0x%x\n", reg_rah);

    /* TODO: [p5-task2] Initialize rx descriptors */
    for(int i = 0; i < TXDESCS; i++){
        rx_desc_array[i].addr = kva2pa(rx_pkt_buffer[i]);// 填入物理地址
    }

    /* TODO: [p5-task2] Set up the Rx descriptor base address and length */
    e1000_write_reg(e1000, E1000_RDBAL, (uint32_t)kva2pa(rx_desc_array));
    //uint32_t reg_rdbal = e1000_read_reg(e1000, E1000_RDBAL);
    //printl("E1000_RDBAL = 0x%x\n", reg_rdbal);

    e1000_write_reg(e1000, E1000_RDBAH, (uint32_t)(kva2pa(rx_desc_array) >> 32));
    //uint32_t reg_rdbah = e1000_read_reg(e1000, E1000_RDBAH);
    //printl("E1000_RDBAH = 0x%x\n", reg_rdbah);

    e1000_write_reg(e1000, E1000_RDLEN, RXDESCS * sizeof(struct e1000_rx_desc));
    //uint32_t reg_rdlen = e1000_read_reg(e1000, E1000_RDLEN);
    //printl("E1000_RDLEN = 0x%x\n", reg_rdlen);

    /* TODO: [p5-task2] Set up the HW Rx Head and Tail descriptor pointers */
    e1000_write_reg(e1000, E1000_RDT, 0);
    e1000_write_reg(e1000, E1000_RDH, 1);

    /* TODO: [p5-task2] Program the Receive Control Register */
    e1000_write_reg(e1000, E1000_RCTL, E1000_RCTL_EN | E1000_RCTL_BAM);
    //uint32_t reg_rctl = e1000_read_reg(e1000, E1000_RCTL);
    //printl("E1000_RCTL = 0x%x\n", reg_rctl);

    /* TODO: [p5-task4] Enable RXDMT0 Interrupt */
    e1000_write_reg(e1000, E1000_IMS, E1000_IMS_RXDMT0);
    //uint32_t reg_ims = e1000_read_reg(e1000, E1000_IMS);
    //printl("E1000_IMS = 0x%lx\n", reg_ims);

    local_flush_dcache();
}

/**
 * e1000_init - Initialize e1000 device and descriptors
 **/
void e1000_init(void)
{
    /* Reset E1000 Tx & Rx Units; mask & clear all interrupts */
    e1000_reset();

    /* Configure E1000 Tx Unit */
    e1000_configure_tx();

    /* Configure E1000 Rx Unit */
    e1000_configure_rx();
}

/**
 * e1000_transmit - Transmit packet through e1000 net device
 * @param txpacket - The buffer address of packet to be transmitted
 * @param length - Length of this packet
 * @return - Number of bytes that are transmitted successfully
 **/
int e1000_transmit(void *txpacket, int length)
{
    /* 先填入再发送 --> HEAD追TAIL 
     * TAIL反超HEAD：轮询等待（硬件处理）
     * HEAD追上TAIL：硬件等软件处理
     */

    //printl("\n\ninfo in e1000_transmit-----------------:\n\n");
    /* TODO: [p5-task1] Transmit one packet from txpacket */

    // 只需要移动tail，并填充好数据
    // 发送使能后，当head不等于tail时，硬件就会自动发送数据，然后移动head
    uint32_t tail_idx = e1000_read_reg(e1000, E1000_TDT);
    //printk("\n\nE1000_TDT = 0x%lx\n", tail_idx);
    uint32_t head_idx = e1000_read_reg(e1000, E1000_TDH);
    //printk("E1000_TDH = 0x%lx\n", head_idx);

    local_flush_dcache();
    // TODO: 阻塞当前发送进程
    if(tail_idx + 1 == head_idx || tail_idx == TXDESCS - 1 && head_idx == 0){
        core_id = get_current_cpu_id();
        printk("\nblock send process\n");
        do_block(&(current_running[core_id]->list), &net_send_queue);
    }
    /*
    if(tail_idx == head_idx){
        // CMD.RS位置1
        tx_desc_array[tail_idx].cmd = tx_desc_array[tail_idx].cmd | E1000_TXD_CMD_RS;
        local_flush_dcache();
        // 轮询STA.DD
        while(tx_desc_array[tail_idx].status & E1000_TXD_STAT_DD == 0){
            local_flush_dcache();
        }
    }
    */

    /*
    printl("\npkt info:\n");
    for(int i = 0; i < length / sizeof(uint32_t); i++){
        printl("0x%x ", *((uint32_t *)txpacket + i));
        if(i % 4 == 0){
            printl("\n");
        }
    }
    */
    memcpy(tx_pkt_buffer[tail_idx], txpacket, length);
    tx_desc_array[tail_idx].length = length;

    //printl("\naddr = 0x%lx\n", tx_desc_array[tail_idx].addr);
    //printl("length = 0x%lx\n", tx_desc_array[tail_idx].length);

    //printl("\npkt info:");
    /*
    for(int i = 0; i < length / sizeof(uint32_t); i++){
        if(i % 4 == 0){
            printl("\n");
        }
        printl("0x%x ", *((uint32_t *)tx_desc_array[tail_idx].addr + i));
    }
    */

    // 移动尾指针
    if(tail_idx == TXDESCS - 1){
        // 循环数组
        tail_idx = 0;
    }
    else{
        tail_idx++;
    }

    e1000_write_reg(e1000, E1000_TDT, tail_idx);
    //tail_idx = e1000_read_reg(e1000, E1000_TDT);
    //printk("\nE1000_TDT(new) = 0x%lx\n", tail_idx);
    local_flush_dcache();
    //static int pkt_num = 0;
    //pkt_num++;
    //printl("pkt_num = %d\n", pkt_num);
    return length;
}

/**
 * e1000_poll - Receive packet through e1000 net device
 * @param rxbuffer - The address of buffer to store received packet
 * @return - Length of received packet
 **/
int e1000_poll(void *rxbuffer)
{
    /* 先接收再取出 --> TAIL追HEAD 
     * TAIL（差一位）追上HEAD：轮询等待（硬件处理）
     * HEAD反超TAIL：硬件等软件处理
     */
    
    //printl("\n\ninfo in e1000_poll-----------------:\n\n");
    /* TODO: [p5-task2] Receive one packet and put it into rxbuffer */
    int length;
    uint32_t tail_idx = e1000_read_reg(e1000, E1000_RDT);
    //printl("E1000_RDT = 0x%lx\n", tail_idx);
    uint32_t head_idx = e1000_read_reg(e1000, E1000_RDH);
    //printl("E1000_RDH = 0x%lx\n", head_idx);
    local_flush_dcache();

    //TODO: 阻塞当前接收进程
    if(tail_idx + 1 == head_idx || tail_idx == TXDESCS - 1 && head_idx == 0){
        core_id = get_current_cpu_id();
        do_block(&(current_running[core_id]->list), &net_recv_queue);
    }

    // 移动尾指针
    if(tail_idx == TXDESCS - 1){
        // 循环数组
        tail_idx = 0;
    }
    else{
        tail_idx++;
    }

    length = rx_desc_array[tail_idx].length;
    //printl("length = 0x%lx\n", length);
    //printl("\npkt info:");
    /*
    for(int i = 0; i < length / sizeof(uint32_t); i++){
        if(i % 4 == 0){
            printl("\n");
        }
        printl("0x%x ", *((uint32_t *)rx_desc_array[tail_idx].addr + i));
    }
    */
    memcpy(rxbuffer, rx_pkt_buffer[tail_idx], length);

    // 清空控制位
    rx_desc_array[tail_idx].csum = 0;
    rx_desc_array[tail_idx].errors = 0;
    rx_desc_array[tail_idx].length = 0;
    rx_desc_array[tail_idx].special = 0;
    rx_desc_array[tail_idx].status = 0;

    e1000_write_reg(e1000, E1000_RDT, tail_idx);
    tail_idx = e1000_read_reg(e1000, E1000_RDT);
    //printl("\nE1000_RDT(new) = 0x%lx\n", tail_idx);
    local_flush_dcache();

    return length;
}

void e1000_handle_txqe(void){
    do_unblock_head(&net_send_queue, PUT_AT_HEAD);
}

void e1000_handle_rxdmt0(void){
    do_unblock_head(&net_recv_queue, PUT_AT_HEAD);
}