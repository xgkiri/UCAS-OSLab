#include <time.h>
#include <mailbox.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void test_str(void *arg){
    char c = (char)arg;
    printf("\nchar = %c\n",c);
    while(1);
}

int main(){
    int id;
    char c = 'h';
    printf("ready to create thread\n");
    pthread_create(&id, test_str, (void *)c);
    while(1);
}