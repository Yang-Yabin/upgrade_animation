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

struct buffer_object
{
    uint32_t width;        //帧缓冲的宽度，以像素为单位
    uint32_t height;       //帧缓冲的高度，以像素为单位
    uint32_t pitch;        //帧缓冲的行距，即每一行像素数据所占用的字节数
    uint32_t handle;       //帧缓冲的句柄
    uint32_t size;         //帧缓冲的大小,字节
    unsigned char *vaddr;  //指向帧缓冲的虚拟地址的指针
    uint32_t fb_id;        //帧缓冲的ID
};

int modeset_create_fb(int fd, struct buffer_object *bo)
{
    int ret;
    struct drm_mode_create_dumb create = {};
    struct drm_mode_map_dumb map = {};
    
    create.width = bo->width;
    create.height = bo->height;
    create.bpp = 32;
    /* create a dumb-buffer */
    ret = drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create);
    if(ret != 0){
        fprintf(stderr, "drmIoctl error: %d\n", ret);
        return ret;
    }

    bo->pitch = create.pitch;
    bo->size = create.size;
    bo->handle = create.handle;
    /*向DRM设备添加一个帧缓冲*/
    ret = drmModeAddFB(fd, bo->width, bo->height, 24, 32, bo->pitch,
                 bo->handle, &bo->fb_id);
    if(ret != 0){
        fprintf(stderr, "drmModeAddFB error: %d\n", ret);
        return ret;
    }

    map.handle = create.handle;
    /*将dumb buffer映射到用户空间*/
    ret = drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map);
    if(ret != 0){
        fprintf(stderr, "drmModeAddFB error: %d\n", ret);
        return ret;
    }
    bo->vaddr = mmap(0, create.size, PROT_READ | PROT_WRITE,
                     MAP_SHARED, fd, map.offset);
    return 0;
}

int draw_img(int size, struct buffer_object *bo, unsigned char *img){
    uint32_t i;
    unsigned char *temp_img = img; 
    uint32_t *vaddr = (uint32_t *)bo->vaddr;
    for(i = 0; i < (bo->size / 4); i++) 
    {
        vaddr[i] = (temp_img[3] << 24) | (temp_img[0] << 16) | (temp_img[1] << 8) | temp_img[2];
        temp_img += 4;
    }
    return 0;
}

void modeset_destroy_fb(int fd, struct buffer_object *bo) {
    struct drm_mode_destroy_dumb destroy ;
    // 从显示器中移除帧缓冲区
    drmModeRmFB(fd, bo->fb_id);
    // 解除内存映射
    munmap(bo->vaddr, bo->size);
    /* delete dumb buffer */
    memset(&destroy, 0, sizeof(destroy));
    // 设置 destroy 结构体的 handle 字段
    destroy.handle = bo->handle;
    // 销毁帧缓冲区
    drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
}