#include <stdio.h>
#include <unistd.h> 
#include <kernel.h>

#define MAX_NUM 10000
#define HALF_NUM 5000

int data[MAX_NUM];
int done0 = 0;
int done1 = 0;
int result0 = 0;
int result1 = 0;
int check_result = 0;

void *func0(int start_idx, int end_idx){
    int print_location = 6;
    for(int i = start_idx; i < end_idx; i++){
        result0 += data[i];
        if(i % 1000 == 0){
            sys_move_cursor(0, print_location);
            printf("sum result0 = %d\n", result0);
            sys_sleep(1);
        }
    }
    sys_move_cursor(0, print_location);
    printf("sum result0 = %d\n", result0);
    done0 = 1;
    while(1);
}

void *func1(int start_idx, int end_idx){
    int print_location = 7;
    for(int i = start_idx; i < end_idx; i++){
        result1 += data[i];
        if(i % 1000 == 0){
            sys_move_cursor(0, print_location);
            printf("sum result1 = %d\n", result1);
        }
    }
    sys_move_cursor(0, print_location);
    printf("sum result1 = %d\n", result1);
    done1 = 1;
    while(1);
}

int main(){
    int print_location = 8;
    int result;
    for(int i = 0; i < MAX_NUM; i++){
        data[i] = i;
        check_result += i;
    }
    sys_move_cursor(0, print_location);
    printf("check result = %d\n", check_result);

    sys_fork(func0, 0, HALF_NUM);
    sys_fork(func1, HALF_NUM, MAX_NUM);
 
    while(!done0 || !done1);

    result = result0 + result1;
    print_location = 9;
    sys_move_cursor(0, print_location);
    if(result != check_result){
        printf("result check failed!\n");
    }
    else{
        printf("result check pass!\n");
    }

    while(1);
}