#ifndef  _IMG_DISPLAY_H
#define  _IMG_DISPLAY_H
#include <stdint.h>

struct buffer_object{
    uint32_t width;        //帧缓冲的宽度，以像素为单位
    uint32_t height;       //帧缓冲的高度，以像素为单位
    uint32_t pitch;        //帧缓冲的行距，即每一行像素数据所占用的字节数
    uint32_t handle;       //帧缓冲的句柄
    uint32_t size;         //帧缓冲的大小,字节
    unsigned char *vaddr;  //指向帧缓冲的虚拟地址的指针
    uint32_t fb_id;        //帧缓冲的ID
};

/*****************************************************************************
 Prototype    : modeset_create_fb
 Description  : 在DRM设备上创建帧缓冲(内核空间)并将其映射到用户空间               
 Input        : int                              fd     DRM设备文件描述符
                struct buffer_object*            bo     framebuffer结构体指针
 Output       : None
 Return Value : int                        - 成功返回ret(0),失败则输出错误码
*****************************************************************************/
extern int modeset_create_fb(int fd, struct buffer_object *bo);

/*****************************************************************************
 Prototype    : draw_img
 Description  : 将图像数据绘制到用户空间的帧缓冲映射区中
                使用之前需要对struct buffer_object初始化：需要在
                struct buffer_object中存储LCD设备分辨率信息和帧缓冲的地址信息               
 Input        : int                              size     LCD设备的分辨率
                struct buffer_object*            bo     framebuffer结构体指针
                unsigned char*                   img    存储解码后png图像数据
 Output       : None
 Return Value : int                        - 成功返回0
*****************************************************************************/
extern int draw_img(int size, struct buffer_object *bo, unsigned char *img);

/*****************************************************************************
 Prototype    : modeset_destroy_fb
 Description  : 从DRM设备移除帧缓冲，取消内存映射并销毁帧缓冲              
 Input        : int                              fd     DRM设备文件描述符
                struct buffer_object*            bo     framebuffer结构体指针
 Output       : None
*****************************************************************************/
extern void modeset_destroy_fb(int fd, struct buffer_object *bo);

#endif 