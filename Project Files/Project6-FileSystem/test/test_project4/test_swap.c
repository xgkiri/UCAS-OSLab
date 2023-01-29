#include <time.h>
#include <mailbox.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ADDR_BASE 0x8000000
#define PAGE_SIZE 4096
#define MAX_PAGE_NUM 32
#define TEST_PAGE_NUM 36
#define NUM_MUST_SWAPPED (TEST_PAGE_NUM - MAX_PAGE_NUM)

int main(){
    char buf_number[4];
    uint64_t vaddr;
    sys_move_cursor(0, 2);
    printf("--------------Swap Test Start--------------\n\n");
    /*
    printf("First write something in each page\n");
    printf("The total number of pages is double of max-limit\n");
    printf("Some of pages' infomation will be printed out:\n");
    */
    for(int i = 0; i < MAX_PAGE_NUM; i++){ 
        char str[] = "i am str ";
        vaddr = ADDR_BASE + 2 * PAGE_SIZE * i;
        itoa(i, buf_number, 4, 10);
        strcat(str, buf_number);
        memcpy(vaddr, str, strlen(str));
        if(i <= NUM_MUST_SWAPPED){
            printf("str: %s ", vaddr);
            printf("physical address = 0x%lx\n\n", sys_get_pa(vaddr));
        }
    }
    printf("-----------------------------\n\n");
    /*
    printf("---------------------------------------------------------\n");
    printf("Now print out the infomation of same pages\n");
    printf("They must have been swapped\n");
    */
    for(int i = 0; i <= NUM_MUST_SWAPPED; i ++){
        vaddr = ADDR_BASE + 2 * PAGE_SIZE * i;
        printf("str: %s ", vaddr);
        printf("physical address = 0x%lx\n\n", sys_get_pa(vaddr));
    }

    printf("---------------Swap Test End---------------\n");
}