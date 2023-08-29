#ifndef  _IMG_DISPLAY_H
#define  _IMG_DISPLAY_H
#include <stdint.h>

struct buffer_object{
    uint32_t width;        //֡����Ŀ�ȣ�������Ϊ��λ
    uint32_t height;       //֡����ĸ߶ȣ�������Ϊ��λ
    uint32_t pitch;        //֡������о࣬��ÿһ������������ռ�õ��ֽ���
    uint32_t handle;       //֡����ľ��
    uint32_t size;         //֡����Ĵ�С,�ֽ�
    unsigned char *vaddr;  //ָ��֡����������ַ��ָ��
    uint32_t fb_id;        //֡�����ID
};

/*****************************************************************************
 Prototype    : modeset_create_fb
 Description  : ��DRM�豸�ϴ���֡����(�ں˿ռ�)������ӳ�䵽�û��ռ�               
 Input        : int                              fd     DRM�豸�ļ�������
                struct buffer_object*            bo     framebuffer�ṹ��ָ��
 Output       : None
 Return Value : int                        - �ɹ�����ret(0),ʧ�������������
*****************************************************************************/
extern int modeset_create_fb(int fd, struct buffer_object *bo);

/*****************************************************************************
 Prototype    : draw_img
 Description  : ��ͼ�����ݻ��Ƶ��û��ռ��֡����ӳ������
                ʹ��֮ǰ��Ҫ��struct buffer_object��ʼ������Ҫ��
                struct buffer_object�д洢LCD�豸�ֱ�����Ϣ��֡����ĵ�ַ��Ϣ               
 Input        : int                              size     LCD�豸�ķֱ���
                struct buffer_object*            bo     framebuffer�ṹ��ָ��
                unsigned char*                   img    �洢�����pngͼ������
 Output       : None
 Return Value : int                        - �ɹ�����0
*****************************************************************************/
extern int draw_img(int size, struct buffer_object *bo, unsigned char *img);

/*****************************************************************************
 Prototype    : modeset_destroy_fb
 Description  : ��DRM�豸�Ƴ�֡���壬ȡ���ڴ�ӳ�䲢����֡����              
 Input        : int                              fd     DRM�豸�ļ�������
                struct buffer_object*            bo     framebuffer�ṹ��ָ��
 Output       : None
*****************************************************************************/
extern void modeset_destroy_fb(int fd, struct buffer_object *bo);

#endif 