#include <time.h>
#include <mailbox.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ADDR_DEFAULT 0x8000000
#define OFFSET_IN_PAGE 0x500

int main(int argc, char *argv[]){
    uint64_t vaddr_to_copy;
    uint64_t vaddr_copy1;
    uint64_t vaddr_copy2;
    if(argc >= 1){
        vaddr_to_copy = atoi(argv[0]);
    }
    else{
        vaddr_to_copy = ADDR_DEFAULT;
    }
    sys_move_cursor(0, 2);
    printf("--------------Snapshot Test Start--------------\n\n");
    // 创建快照，打印正本和快照的虚地址
    printf("addr to make snapshot = 0x%lx\n", vaddr_to_copy);
    vaddr_copy1 = sys_create_sp(vaddr_to_copy);
    vaddr_copy2 = sys_create_sp(vaddr_to_copy);
    printf("addr of snapshot1 = 0x%lx\n", vaddr_copy1);
    printf("addr of snapshot2 = 0x%lx\n", vaddr_copy2);
    // 首先检验三者是否拥有相同的物理地址
    if(sys_get_pa(vaddr_copy1) != sys_get_pa(vaddr_to_copy) || sys_get_pa(vaddr_copy2) != sys_get_pa(vaddr_to_copy)){
        printf("\npaddr test fail!\n");
    }
    else{
        printf("\npaddr test pass!\n");
        printf("same paddr = 0x%lx\n", sys_get_pa(vaddr_to_copy));
    }
    // 向快照1中写入数据
    // 验证正本和快照2的物理地址不变，快照1的物理地址改变
    // 验证能够正确读取出写入快照1中的数据
    printf("\nnow write something into snapshot1\n");
    char str1[] = "i am snapshot1";
    memcpy(vaddr_copy1 + OFFSET_IN_PAGE, str1, strlen(str1));
    if(sys_get_pa(vaddr_copy2) != sys_get_pa(vaddr_to_copy)){
        printf("\npaddr test fail!\n");
    }
    else{
        printf("\npaddr test pass!\n");
        printf("same paddr = 0x%lx\n", sys_get_pa(vaddr_to_copy));
    }
    printf("now paddr of snapshot1 = 0x%lx\n", sys_get_pa(vaddr_copy1));
    printf("str in snapshot1 = %s\n", vaddr_copy1 + OFFSET_IN_PAGE);
    // 向正本中写入数据
    // 验证快照1和快照2的物理地址不变，正本的物理地址改变
    // 验证能正确读取出写入正本中的数据
    printf("\nnow write something into original page\n\n");
    char str2[] = "i am original page";
    memcpy(vaddr_to_copy + OFFSET_IN_PAGE, str2, strlen(str1));
    printf("now paddr of original page = 0x%lx\n", sys_get_pa(vaddr_to_copy));
    printf("now paddr of snapshot1     = 0x%lx\n", sys_get_pa(vaddr_copy1));
    printf("now paddr of snapshot2     = 0x%lx\n", sys_get_pa(vaddr_copy2));
    printf("str in original page = %s\n\n", vaddr_to_copy + OFFSET_IN_PAGE);

    printf("---------------Snapshot Test End---------------\n");
}