/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                  The shell acts as a task running in user mode.
 *       The main function is to make system calls through the user's output.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#include <stdio.h>

#include <stdint.h>
//#include <tiny_libc/include/string.h>

#include <unistd.h>
//#include <tiny_libc/include/unistd.h>

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define SHELL_BEGIN 20
#define BUF_LEN 32
#define NAME_LEN 24
#define PID_LEN 4
#define MASK_LEN 2
#define ARGS_MAX_NUM 8
#define ARGS_MAX_LEN 16
#define NAME_LEN_TO_PRINT 16
#define CORE_MASK_DEFAULT 3
#define FILE_NAME_LEN_MAX 32
#define MAX_PATH_LEN 128

void init_buf();

//命令行buf
char buf[BUF_LEN];
int buf_pos;
int buf_offset;

//命令行参数
char arg_holder[ARGS_MAX_NUM][ARGS_MAX_LEN];
char *argv[ARGS_MAX_NUM];
int argc;

int main(void)
{
    char task_name[NAME_LEN];
    char task_pid[PID_LEN];
    char mask_char[MASK_LEN];
    char file_name[FILE_NAME_LEN_MAX];
    char single_path[FILE_NAME_LEN_MAX];
    char current_path[MAX_PATH_LEN];
    char c;

    /*
    int core_id;

    core_id = sys_get_core_id();
    // shell只在主核上运行，从核在此处等待调度
    while(core_id == 1);
    */
    sys_move_cursor(0, SHELL_BEGIN);
    printf("------------------- COMMAND -------------------\n");
    strcpy(current_path, "\\root");
    while (1)
    {
        // TODO [P3-task1]: call syscall to read UART port
        init_buf();
        printf("> @UCAS_OS:");
        printf("%s", current_path);
        printf(": ");
        while(1){
            while((c = sys_getchar()) == (char)-1);
            if(c == '\b' || c == 127){
                if(buf_pos > 0){
                    buf_pos--;
                    printf("%c", c);
                }
            }
            else if(c == '\r' || c == '\n'){
                buf[buf_pos] = '\0';
                printf("\n");
                /*
                printf("buf:\n%s", buf);
                while(1);
                */
                break;
            }
            else{
                buf[buf_pos++] = c;
                printf("%c", c);
            }
        }
        // TODO [P3-task1]: parse input
        // note: backspace maybe 8('\b') or 127(delete)
        // TODO [P3-task1]: ps, exec, kill, clear
        if(strcmp(buf, "clear") == 0){
            sys_screen_clean();
            sys_move_cursor(0, SHELL_BEGIN);
            printf("------------------- COMMAND -------------------\n");
        }
        else if(strcmp(buf, "ps") == 0){
            sys_ps();
        }
        else if(strncmp(buf, "exec ", 5) == 0){
            pid_t task_to_exec_pid;

            buf_offset += strlen("exec");
            // get name
            my_copy_word(task_name, buf, &buf_offset);
            // 获取命令行参数
            while(my_copy_word(arg_holder[argc], buf, &buf_offset) != -1){
                argc++;
            }
            for(int i = 0; i < argc; i++){
                argv[i] = arg_holder[i];
            }
            // exec task
            task_to_exec_pid = sys_exec(task_name, argc, argv);
            if(task_to_exec_pid != 0){
                printf("Info: execute %s", task_name);
                // padding
                for(int j = 0; j < NAME_LEN_TO_PRINT- strlen(task_name); j++){
                    printf(" ");
                }
                printf("successfully, pid = %d ...\n", task_to_exec_pid);
                if(strcmp(argv[argc - 1], "&") == 0){
                    /* 不需要等待进程结束 */
                    sys_yield();
                }
                else{
                    /* 需要等待进程结束 */
                    printf("waiting ...\n");
                    sys_waitpid(task_to_exec_pid);
                    printf("task done\n");
                }
            }
            else{
                printf("Info: task do not found\n");
            }
        }
        else if(strncmp(buf, "kill ", 5) == 0){
            /* TODO: 杀死进程 */
            int task_to_kill_pid;
            buf_offset += strlen("kill");
            my_copy_word(task_pid, buf, &buf_offset);
            task_to_kill_pid = atoi(task_pid);
            //printf("task to kill pid = %d\n", task_to_kill_pid);
            if(sys_kill(task_to_kill_pid) == 1){
                printf("Info: kill task with pid %d successfully\n", task_to_kill_pid);
            }
            else{
                printf("Info: task do not found\n");
            }
        }
        else if(strncmp(buf, "taskset -p ", 11) == 0){
            // TODO: 完成taskset
            pid_t task_to_set_mask_pid;
            int mask;
            buf_offset += strlen("taskset -p");
            // get mask
            my_copy_word(mask_char, buf, &buf_offset);
            mask = atoi(mask_char);
            // get pid
            my_copy_word(task_pid, buf, &buf_offset);
            task_to_set_mask_pid = atoi(task_pid);
            
            sys_set_mask(mask, task_to_set_mask_pid);
            //printf("pid = %d, mask = %d\n", task_to_set_mask_pid, mask);
        }
        else if(strncmp(buf, "taskset ", 8) == 0){
            pid_t task_to_exec_pid;
            int mask;
            buf_offset += strlen("taskset");
            // get mask
            my_copy_word(mask_char, buf, &buf_offset);
            mask = atoi(mask_char);
            // get name
            my_copy_word(task_name, buf, &buf_offset);
            // 获取命令行参数
            while(my_copy_word(arg_holder[argc], buf, &buf_offset) != -1){
                argc++;
            }
            for(int i = 0; i < argc; i++){
                argv[i] = arg_holder[i];
            }
            // exec task
            task_to_exec_pid = sys_set_task(task_name, mask, argc, argv);
            if(task_to_exec_pid != 0){
                printf("Info: execute %s", task_name);
                // padding
                for(int j = 0; j < NAME_LEN_TO_PRINT- strlen(task_name); j++){
                    printf(" ");
                }
                printf("successfully, pid = %d ...\n", task_to_exec_pid);
                if(strcmp(argv[argc - 1], "&") == 0){
                    /* 不需要等待进程结束 */
                    sys_yield();
                }
                else{
                    /* 需要等待进程结束 */
                    printf("waiting ...\n");
                    sys_waitpid(task_to_exec_pid);
                    printf("task done\n");
                }
            }
            else{
                printf("Info: task do not found\n");
            }
            // set mask
            sys_set_mask(mask, task_to_exec_pid);
        }
        else if(strcmp(buf, "mkfs") == 0){
            // make fs
            sys_mkfs();
        }
        else if(strcmp(buf, "statfs") == 0){
            // status
            sys_statfs();
        }
        else if(strncmp(buf, "mkdir ", 6) == 0){
            // make dir
            buf_offset += strlen("mkdir");
            my_copy_word(file_name, buf, &buf_offset);
            if(sys_mkdir(file_name) == -1){
                printf("Info: path already exist\n");
            }
        }
        else if(strncmp(buf, "rmdir ", 6) == 0){
            // remove dir
            buf_offset += strlen("rmdir");
            my_copy_word(file_name, buf, &buf_offset);
            if(sys_rmdir(file_name) == -1){
                printf("Info: path not found\n");
            }
        }
        else if(strncmp(buf, "cd ", 3) == 0){
            // cd
            buf_offset += strlen("cd");
            my_copy_word(file_name, buf, &buf_offset);
            //printf("file name = %s\n", file_name);
            if(sys_cd(file_name) == 0){
                for(int i = 0; ; i++){
                    if(get_single_path(single_path, file_name, i) == -1){
                        break;
                    }
                    else{
                        if(strcmp(single_path, ".") == 0){
                            // do nothing
                        }   
                        else if(strcmp(single_path, "..") == 0){
                            del_single_path(current_path);
                        }
                        else{
                            add_single_path(current_path, single_path);
                        }
                        
                    }
                }
            }
            else{
                printf("Info: path not found\n");
            }
        }
        else if(strncmp(buf, "ls -l ", 6) == 0){ 
            // ls -l
            buf_offset += strlen("ls -l");
            my_copy_word(file_name, buf, &buf_offset);
            if(sys_ls(file_name, 1) == -1){
                printf("Info: path not found\n");
            }
        }
        else if(strcmp(buf, "ls -l") == 0){
            // ls -l current dir
            if(sys_ls(".", 1) == -1){
                printf("Info: path not found\n");
            }
        }
        else if(strncmp(buf, "ls ", 3) == 0){
            // ls
            buf_offset += strlen("ls");
            my_copy_word(file_name, buf, &buf_offset);
            //printf("file name = %s\n", file_name);
            if(sys_ls(file_name, 0) == -1){
                printf("Info: path not found\n");
            }
        }
        else if(strcmp(buf, "ls") == 0){
            // ls current dir
            if(sys_ls(".", 0) == -1){
                printf("Info: path not found\n");
            }
        }
        else if(strncmp(buf, "touch ", 6) == 0){
            buf_offset += strlen("touch");
            my_copy_word(file_name, buf, &buf_offset);
            if(sys_touch(file_name) == -1){
                printf("Info: path already exist\n");
            }
        }
        else if(strncmp(buf, "cat ", 4) == 0){
            buf_offset += strlen("cat");
            my_copy_word(file_name, buf, &buf_offset);
            if(sys_cat(file_name) == -1){
                printf("Info: path not found\n");
            }
        }
        else if(strncmp(buf, "ln ", 3) == 0){
            int result;
            char dst_path[FILE_NAME_LEN_MAX];
            buf_offset += strlen("ln");
            my_copy_word(file_name, buf, &buf_offset);
            my_copy_word(dst_path, buf, &buf_offset);
            result = sys_ln(file_name, dst_path);
            if(result == -1){
                printf("Info: file type error\n");
            }
            else if(result == -2){
                printf("Info: link already exist\n");
            }
        }
        else if(strncmp(buf, "rm ", 3) == 0){
            buf_offset += strlen("rm");
            my_copy_word(file_name, buf, &buf_offset);
            if(sys_rm(file_name) == -1){
                printf("Info: path not found\n");
            }
        }
        else{
            printf("Unknown command\n");
        }
        // TODO [P6-task1]: mkfs, statfs, cd, mkdir, rmdir, ls

        // TODO [P6-task2]: touch, cat, ln, ls -l, rm
    }

    return 0;
}

void init_buf(){
    buf_pos = 0;
    buf_offset = 0;
    argc = 0;
    for(int i = 0; i < BUF_LEN; i++){
        buf[i] = '\0';
    }
}

int get_single_path(char *single_path, char *path, int level){
    char *p_s = single_path;
    char *p_p = path;
    for(int i = 0; i < level; i++, p_p++){
        while(*p_p != '\0' && *p_p != '/'){
            p_p++;
        }
        if(*p_p == '\0'){
            return -1;
        }
    }
    while(*p_p != '\0' && *p_p != '/'){
        *p_s = *p_p;
        p_s++;
        p_p++;
    }
    *p_s = '\0';
    return 0;
}

void del_single_path(char *path){
    while(*path != '\0'){
        path++;
    }
    while(*path != '\\'){
        path--;
    }
    *path = '\0';
}

void add_single_path(char *path, char *single_path){
    strcat(path, "\\");
    strcat(path, single_path);
}