#include <time.h>
#include <mailbox.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(){
    for(int i = 0; i < 100; i++){
        printf("sleep 1s\n");
        sys_sleep(1);
        printf("wake up\n");
        sys_sleep(1);
    }
}