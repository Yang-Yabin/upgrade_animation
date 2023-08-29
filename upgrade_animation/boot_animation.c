
#include<sys/types.h>
#include<sys/stat.h>
#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#include<stdlib.h>
#include <string.h>
#include <errno.h>

#include "img_create.h"

#define LOGO            "/userdata/img/logo.png"
#define FIFO_FILE       "/userdata/img_fifo"
#define IMG_PRINT       "/userdata/img_print"
#define BUF_SIZE        300
#define EXIT            "exit"

//将argv[]合成一个字符串，每个元素以空格分开
static char* merge_argv_strings(int num, char* parm[]) {
    if (num < 2) {
        return NULL;                               // 输入参数不足，返回空指针
    }
    int total_length = 0;
    for (int i = 1; i < num; i++) {
        total_length += strlen(parm[i]) + 1;       // +1 用于空格分隔符
    }

    char* result = (char*)malloc(total_length);    // 分配足够的内存来存储合并后的字符串
    result[0] = '\0';                              // 确保初始字符串为空

    for (int i = 1; i < num; i++) {
        strcat(result, parm[i]);
        if (i != num - 1) {
            strcat(result, " ");                   // 在每个字符串后面添加空格分隔符
        }
    }
    return result;
}

static void check_img_print() {
    if(access(IMG_PRINT, F_OK) == -1) {
        printf("[IMG_PRINT] : not found [/userdata/img_print]\n");
        exit(1);
    }
    const char* process_name = "img_print";
    const int command_max_length = 100;
    char command[command_max_length];
    snprintf(command, command_max_length, "ps -ef | grep -v grep | grep -q %s", process_name);

    int status = system(command);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        printf("boot_animation:img_print is already running.\n");
    } else {
        system("/userdata/img_print &");
        printf("=====POWER-ON=====POWER-ON=====POWER-ON=====POWER-ON=====\n");
        printf("boot_animation:The process is not running, start running.\n");
    }
}

int main(int argc, char *argv[])
{
    int size ;
    int ret;
    int fd;
    char *path = NULL;
    char *img = NULL;
    
    if(argc > 1){
        if (!strcmp(argv[1], "mk") || !strcmp(argv[1], "-m")) {
            if (argc > 2) {
                path = argv[2];
            }
            return img_mk(size, path);
        }
        else if (!strcmp(argv[1], "diff") || !strcmp(argv[1], "-d")) {
            char *img1 = NULL;
            char *img2 = NULL;

            if (argc > 4) {
                img1 = argv[2];
                img2 = argv[3];
                path = argv[4];
            }
            else if (argc > 3) {
                if (strstr(argv[3], "diff")) {
                    img2 = argv[2];
                    path = argv[3];
                } else {
                    img1 = argv[2];
                    img2 = argv[3];
                }
            }
            else if (argc > 2) {
                img2 = argv[2];
            }
            else {
                printf("Usage: %s {diff|-d} [img1] img1 [img.diff]\n", argv[0]);
                return -1;
            }
            return img_diff(size, img1, img2, path);
        }
        else{
            img = merge_argv_strings(argc, argv);
        }   
    } 
    else{
        img = LOGO;
    }

    if (access(FIFO_FILE, F_OK) == -1) {
        if (mkfifo(FIFO_FILE, 0666) == -1) {
            printf("boot_animation:img_send failed to create FIFO\n");
            return -1;
        }
        printf("boot_animation:img_send FIFO created\n");
    } 

    check_img_print();
    
    fd = open(FIFO_FILE, O_WRONLY);     
    if(fd < 0){
        printf("Error: Cannot open FIFO_FILE\n");
        return -1;
    }
    
    ret = write(fd, img, BUF_SIZE);
    if(ret <= 0){
        printf("Error: cannot write to FIFO\n");
    }
    else {
        printf("send_img: %s\n", img);
    }

    ret = strcmp(img, LOGO);
    if (ret != 0){
        free(img);
    }
    else {
        img = EXIT;
        usleep(100000);
        write(fd, img, BUF_SIZE);
    }
        
    close(fd);

    return 0;
}