#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <png.h>
#include "drm.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "img_create.h"
#include "img_display.h"

#define DEV_DRM "/dev/dri/card0"
#define FIFO_FILE "/userdata/img_fifo"
#define BUF_SIZE  300

struct buffer_object buf[2];
int flag = 0;

static int split_str(char* read_path, char* img_path[]) {
    const char* delimiter = " ";
    char* token;
    int i = 0;
    int count;
    token = strtok(read_path, delimiter);
    while (token != NULL) {
        img_path[i] = token;
        i++;
        token = strtok(NULL, delimiter);
    }
    count = i;
    return count;
}

static void comp_img(int size, int path_num, char *img_path[]) {
    unsigned char* img;
    if (path_num >= 1) {
        if (path_num == 3) {
            img = img_read(size, img_path[0]);
            diff_apply(size, img, img_path[1]);
            diff_apply(size, img, img_path[2]);
        } else if (path_num == 2) {
            img = img_read(size, img_path[0]);
            diff_apply(size, img, img_path[1]);
        } else if (path_num == 1) {
            img = img_read(size, img_path[0]);
        }
    } else {
        printf("Error: missing parameter\n");
    }
    printf("img pointer(comp_img): %p\n", img);
    if(flag == 0){ 
        printf("comp_img: This is buf[0].flag = 0\n");
        draw_img(size, &buf[0], img);
    } else{
        printf("comp_img: This is buf[1].flag = 1\n");
        draw_img(size, &buf[1], img);
    }
    free(img);
}

// 检查文件是否存在并可读
static void access_file(char *filename){
    if (access(filename, F_OK) != -1 && access(filename, R_OK) != -1){
        printf("The file exists and is readable\n");
    }
    else{
        printf("The file not exists or is disable to read\n");
    }
}

int main(int argc, char *argv[])
{
    int fd;
    int fd_fifo;
    int size;
    int ret;
    int path_num ;

    drmModeCrtc *saved_crtc;
    drmModeConnector *conn;
    drmModeRes *res;
    uint32_t conn_id;
    uint32_t crtc_id;
    char *img_path[4] = {0};
    char *img1 = NULL;
    char *img2 = NULL;
    unsigned char *img = NULL;

    size = get_screen_size();
    img1 = (char*)malloc(BUF_SIZE);
    img2 = (char*)malloc(BUF_SIZE);

    fd = open(DEV_DRM, O_RDWR | O_CLOEXEC);

    res = drmModeGetResources(fd);
    crtc_id = res->crtcs[0];
    conn_id = res->connectors[0];
    conn = drmModeGetConnector(fd, conn_id);
    buf[0].width = conn->modes[0].hdisplay;
    buf[0].height = conn->modes[0].vdisplay;
    buf[1].width = conn->modes[0].hdisplay;
    buf[1].height = conn->modes[0].vdisplay;

    saved_crtc = drmModeGetCrtc(fd, conn_id); 

    modeset_create_fb(fd, &buf[0]);
    modeset_create_fb(fd, &buf[1]);

    if (access(FIFO_FILE, F_OK) == -1) {
        if (mkfifo(FIFO_FILE, 0666) == -1) {
            printf("img_print:failed to create FIFO\n");
            return 1;
        }
        printf("img_print:FIFO created\n");
    } 

    while (true)
    {
        if(flag == 0){
            printf("This is buf[0]\n");
            
            memset(img1, 0, sizeof(img1));
            fd_fifo = open(FIFO_FILE,O_RDONLY);
            ret = read(fd_fifo, img1, BUF_SIZE);
            if(ret <= 0){
                strcpy(img1, img2);
            }
            printf("img1:%s\n", img1);

            ret = strcmp(img1, "exit");
            if(ret == 0){
                break;
            }
            if(strlen(img1) == 0){
                continue;
            }

            path_num = split_str(img1, img_path);
            printf("path_num:%d\n", path_num);
            for(int i = 0; i < path_num; i++){
                access_file(img_path[i]);
            }

            comp_img(size, path_num, img_path);
            system("echo 200 > /sys/class/backlight/backlight/brightness;echo 200 > /sys/class/backlight/backlight1/brightness");
            ret = drmModeSetCrtc(fd, crtc_id, buf[0].fb_id,
                       0, 0, &conn_id, 1, &conn->modes[0]);
            if (ret == 0){
                printf("drmModeSetCrtc executed successfully.\n");
            }
            else{
                printf("drmModeSetCrtc failed with error code %d.\n", ret);
            }

            flag = 1;
            close(fd_fifo);

        } else{
            printf("This is buf[1]\n");

            memset(img2, 0, sizeof(img2));
            fd_fifo = open(FIFO_FILE,O_RDONLY);
            ret = read(fd_fifo, img2, BUF_SIZE);
            if(ret <= 0){
                strcpy(img2, img1);
            }
            printf("img2:%s\n", img2);
            ret = strcmp(img2, "exit");
            if(ret == 0){
                break;
            }
            if(strlen(img2) == 0){
                continue;
            }
            
            path_num = split_str(img2, img_path);
            for(int i = 0; i < path_num; i++){
                access_file(img_path[i]);
            }
            comp_img(size, path_num, img_path);
            system("echo 200 > /sys/class/backlight/backlight/brightness;echo 200 > /sys/class/backlight/backlight1/brightness");
            ret = drmModeSetCrtc(fd, crtc_id, buf[1].fb_id,
                       0, 0, &conn_id, 1, &conn->modes[0]);
            if (ret == 0){
                printf("drmModeSetCrtc executed successfully.\n");
            }
            else{
                printf("drmModeSetCrtc failed with error code %d.\n", ret);
            }

            flag = 0;
            close(fd_fifo);
        }
    }
    
    if (flag == 0){
        comp_img(size, path_num, img_path);
        system("echo 200 > /sys/class/backlight/backlight/brightness;echo 200 > /sys/class/backlight/backlight1/brightness");
        drmModeSetCrtc(fd, crtc_id, buf[0].fb_id,
                       0, 0, &conn_id, 1, &conn->modes[0]);
    }
    
    modeset_destroy_fb(fd, &buf[0]);
    modeset_destroy_fb(fd, &buf[1]);
   
    drmModeFreeConnector(conn);
    drmModeFreeResources(res);
    
    close(fd);
    if(fd_fifo){
        close(fd_fifo);
        system("rm -f /userdata/img_fifo");
        }
    free(img1);
    free(img2);

    return 0;
}

