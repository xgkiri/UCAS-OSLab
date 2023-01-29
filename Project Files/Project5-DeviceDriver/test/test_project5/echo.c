#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#define PKT_NUM 128
#define BUF_SIZE 256
#define NULL (void *)0x0

static char recv_buffer[PKT_NUM][BUF_SIZE];
static char send_buffer[PKT_NUM][BUF_SIZE];
static uint32_t length[PKT_NUM];
static uint32_t count = 0;

void thread_recv(){
    sys_move_cursor(0, 0);
    printf("thread recv start\n");
    for(int i = 0; i < PKT_NUM; i++){
        sys_net_recv(recv_buffer[i], PKT_NUM, &(length[i]));
        count++;
    }
    while(1);
}

void thread_send(){
    sys_move_cursor(0, 1);
    uint8_t replace1[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    char replace2[] = "Response:";
    printf("thread send start\n");
    //修改MAC地址为广播地址
    //将"Request:"更换为"Response:"
    while(count != PKT_NUM);
    for(int i = 0; i < PKT_NUM; i++){
        memcpy(send_buffer[i], recv_buffer[i], length[i]);
        memcpy(send_buffer[i], replace1, 6);
        memcpy(send_buffer[i] + 54, replace2, 9);
        sys_net_send(send_buffer[i], length[i]);
    }
    /*
    printf("length[0] = %d\n", length[0]);
    for(int i = 0; i < length[0]; i++){
        printf("%lx ", send_buffer[0][i]);
        if(i % 8 == 0 && i!= 0){
            printf("\n");
        }
    }
    */
    while(1);
}

int main(){
    int32_t recv_tid;
    int32_t send_tid;
    sys_pthread_create(&recv_tid, thread_recv, NULL);
    sys_pthread_create(&send_tid, thread_send, NULL);
    return 0;
}