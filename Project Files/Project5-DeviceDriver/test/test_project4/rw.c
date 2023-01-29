#include <stdio.h>
#include <stdint.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

int main(int argc, char* argv[])
{
	assert(argc >= 0);
	srand(clock());
	long mem2 = 0;
	uintptr_t mem1 = 0;
	int curs = 0;
	int i;
	sys_move_cursor(1, 2);
	/*
	// 测试参数
	printf("arg number = %d:\n", argc);
    for(int i = 0; i < argc; i++){
        printf("arg[%d]: %s\n", i, argv[i]);
    }
	while(1);
	*/
	for (i = 0; i < argc; i++)
	{
		if(strcmp(argv[i], "&") == 0){
			continue;
		}
		mem1 = atol(argv[i]);
		// sys_move_cursor(2, curs+i);
		mem2 = rand();
		*(long*)mem1 = mem2;
		printf("0x%lx, %ld\n", mem1, mem2);
		if (*(long*)mem1 != mem2) {
			printf("Error!\n");
		}
	}
	//Only input address.
	//Achieving input r/w command is recommended but not required.
	printf("Success!\n");
	return 0;
}
