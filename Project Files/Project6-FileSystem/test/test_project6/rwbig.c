#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define OFFSET_1MB (1024 * 1024)

static char buff[64];

int main(void)
{
    int fd = sys_fopen("2.txt", O_RDWR);

    // write 'hello world!' * 10
    for (int i = 0; i < 10; i++)
    {
        sys_lseek(fd, OFFSET_1MB * i, SEEK_SET, WRITE_P);
        sys_fwrite(fd, "hello world!\n", 13);
    }

    sys_lseek(fd, 0, SEEK_SET, READ_P);

    // read
    for (int i = 0; i < 10; i++)
    {
        sys_lseek(fd, OFFSET_1MB * i, SEEK_SET, READ_P);
        sys_fread(fd, buff, 13);
        for (int j = 0; j < 13; j++)
        {
            printf("%c", buff[j]);
        }
    }

    sys_fclose(fd);

    return 0;
}