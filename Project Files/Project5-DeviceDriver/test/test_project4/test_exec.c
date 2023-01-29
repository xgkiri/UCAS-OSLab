#include <time.h>
#include <mailbox.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]){
    int pl = atoi(argv[0]);
    char buf_location[10];
    sys_move_cursor(0, pl);
    printf("task[%d]", pl);
    pl++;
    itoa(pl, buf_location, 10, 10);
    if(pl < 5){
        char *argv[1] = {buf_location};
        sys_exec("test_exec", 1, argv);
    }
}