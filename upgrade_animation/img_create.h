#ifndef _IMG_PRINT_H
#define _IMG_PRINT_H

/*****************************************************************************
 Prototype    : img_save
 Description  : 将图片数据保存到指定的文件中               
 Input        : int                         size   图像分辨率
                unsigned char*              img    解码或应用diff文件后的图像数据
                char*                       path   指定的图像保存路径
 Output       : None
 Return Value : int                        - 成功返回 0,失败则输出错误码
*****************************************************************************/
extern int img_save(int size, unsigned char *img, char *path);

/*****************************************************************************
 Prototype    : img_rotate
 Description  : 图像数据旋转90°               
 Input        : unsigned char*              img    解码或应用diff文件后的图像数据
                int                         w      图像的宽度
                int                         h      图像的高度
                int                         pixel  每个图像占用的字节数
 Output       : None
 Return Value : int                        - 成功返回 0,失败则输出错误码
*****************************************************************************/
extern int img_rotate(unsigned char *img, int w, int h, int pixel);

/*****************************************************************************
 Prototype    : png_read
 Description  : 读取PNG文件并将其解码为图像数据
                依赖于libpng库来解码PNG文件，RGB或RGBA类型               
 Input        : int                         size   图像分辨率
                char*                       path   待解析PNG文件路径
 Output       : None
 Return Value : unsigned char*             - 成功返回 *img,失败返回空指针
*****************************************************************************/
extern unsigned char *png_read(int size, char *path);

/*****************************************************************************
 Prototype    : img_is_png
 Description  : 检查给定路径文件是否是PNG文件               
 Input        : char*              path    文件路径
 Output       : None
 Return Value : int                        - 检查正确返回 1 ,失败返回 0
*****************************************************************************/
extern int img_is_png(char *path);

/*****************************************************************************
 Prototype    : img_read
 Description  : 若为PNG文件则将其解码为图像数据，若路径为空则返回白色图片，
                若为DIFF文件则读取
                              
 Input        : int                         size   图像分辨率
                char*                       path   待解析/读取的文件路径
 Output       : None
 Return Value : unsigned char*             - 成功返回 *img,失败返回空指针
*****************************************************************************/
extern unsigned char *img_read(int size, char *path);

/*****************************************************************************
 Prototype    : img_mk
 Description  : 将图片数据保存为指定分辨率的图片

 Input        : int                         size   图像分辨率
                char*                       path   指定的图像保存路径
 Output       : None
 Return Value : unsigned                     - 成功返回 *img,失败返回空指针
*****************************************************************************/
extern int img_mk(int size, char *path);

/*****************************************************************************
 Prototype    : img_diff
 Description  : 比较两个图像的差异，并将差异信息保存到文件中               
 Input        : int                         size   图像分辨率
                char*                       img_path1   第一张图片路径
                char*                       img_path2   第二张图片路径
                char*                       diff_path   差异文件保存路径
 Output       : None
 Return Value : int                        - 成功返回 0
*****************************************************************************/
extern int img_diff(int size, char *img_path1, char *img_path2, char *diff_path);

/*****************************************************************************
 Prototype    : diff_apply
 Description  : 将差异文件应用到PNG文件中              
 Input        : int                         size        图像分辨率
                unsigned char*              img         PNG图片路径
                char*                       diff_path   差异文件路径
 Output       : None
 Return Value : int                        - 成功返回ret(0),失败输出错误信息
*****************************************************************************/
extern int diff_apply(int size, unsigned char *img, char *diff_path);

/*****************************************************************************
 Prototype    : get_screen_size
 Description  : 获取LCD设备分辨率             
 Input        : Noen
 Output       : None
 Return Value : int                        - 成功返回size,失败输出错误信息
*****************************************************************************/
extern int get_screen_size(void);

#endif

