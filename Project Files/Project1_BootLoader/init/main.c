#include <common.h>
#include <asm.h>
#include <os/bios.h>
#include <os/task.h>
#include <os/string.h>
#include <os/loader.h>
#include <type.h>

#define APP_START_ADDR 0x52000000
#define APP_INFO_ADDR 0x502001f8
#define APP_NUM_ADDR 0x502001f4
#define TASK_COUNT_ADDR 0x502001f0
#define ONE_INFO_SIZE 16
#define SECTOR_SIZE 512

#define VERSION_BUF 50
#define TASK_MAXNUM 16

int version = 2; // version must between 0 and 9
char buf[VERSION_BUF];

// Task info array
task_info_t tasks[TASK_MAXNUM];

static int bss_check(void)
{
    for (int i = 0; i < VERSION_BUF; ++i)
    {
        if (buf[i] != 0)
        {
            return 0;
        }
    }
    return 1;
}

static void init_bios(void)
{
    volatile long (*(*jmptab))() = (volatile long (*(*))())BIOS_JMPTAB_BASE;

    jmptab[CONSOLE_PUTSTR]  = (long (*)())port_write;
    jmptab[CONSOLE_PUTCHAR] = (long (*)())port_write_ch;
    jmptab[CONSOLE_GETCHAR] = (long (*)())port_read_ch;
    jmptab[SD_READ]         = (long (*)())sd_read;
}

static void init_task_info(void)
{
    // TODO: [p1-task4] Init 'tasks' array via reading app-info sector
    // NOTE: You need to get some related arguments from bootblock first
}

int *task_count = (int *)TASK_COUNT_ADDR;

int main(void)
{
    // Check whether .bss section is set to zero
    int check = bss_check();

    // Init jump table provided by BIOS (ΦωΦ)
    init_bios();

    // Init task information (〃'▽'〃)
    init_task_info();

    // Output 'Hello OS!', bss check result and OS version
    char output_str[] = "bss check: _ version: _\n\r";
    char output_val[2] = {0};
    int i, output_val_pos = 0;

    output_val[0] = check ? 't' : 'f';
    output_val[1] = version + '0';
    for (i = 0; i < sizeof(output_str); ++i)
    {
        buf[i] = output_str[i];
        if (buf[i] == '_')
        {
            buf[i] = output_val[output_val_pos++];
        }
    }

    bios_putstr("Hello OS!\n\r");
    bios_putstr(buf);

    char c;
    /*
    while(1){
        c = bios_getchar();
        if(c != 255){
            bios_putchar(c);
        }
    }
    */
    
    // TODO: Load tasks by either task id [p1-task3] or task name [p1-task4],
    //   and then execute them.
    int app_num = (int)*((short *)APP_NUM_ADDR);
    int app_offset = app_num * 0x10000;
    int app_info_offset = (*((int *)APP_INFO_ADDR)) % SECTOR_SIZE;
    int *task_num = (int *)(APP_START_ADDR + app_offset + app_info_offset + app_num * ONE_INFO_SIZE); 
    int task_id;
    unsigned mem_address;
    if(*task_count >= *task_num){
        while(1){
            asm volatile("wfi");
        }
    }
    else{
        (*task_count)++;
        task_id = *(task_num + *task_count);
        if(task_id == 1){
            mem_address = load_task_img("bss");
        }
        else if(task_id == 2){
            mem_address = load_task_img("auipc");
        }
        else if(task_id == 3){
            mem_address = load_task_img("data");
        }
        else if(task_id == 4){
            mem_address = load_task_img("2048");
        }
    }

    /*
    char task_name[10];
    unsigned mem_address;
    i = 0;
    while(1){
        c = bios_getchar();
        if(c != 255 && c != '\r'){
            bios_putchar(c);
            task_name[i++] = c;
        }
        else if(c == '\r'){
            task_name[i] = '\0';
            break;
        }
    }
    mem_address = load_task_img(task_name);
    */
    ((void(*)(void))mem_address)();
    
    // Infinite while loop, where CPU stays in a low-power state (QAQQQQQQQQQQQ)
    while (1)
    {
        asm volatile("wfi");
    }

    return 0;
}
