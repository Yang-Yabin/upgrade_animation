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

#define PNG_BYTES_TO_CHECK 4
#define COLOR_WHITE 0xff
#define DEV_FB "/dev/fb0"

typedef struct 
{
    int id;
    unsigned char len;
    unsigned char rgba;
} rgba_info;

struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;

int img_save(int size, unsigned char *img, char *path)
{
    FILE *file;

    if (img == NULL || path == NULL) {
        return -1;
    }

    file = fopen(path, "wb+");
    if (file == NULL) {
        printf("Error: failed to create a img [%s].\n", path);
        return -1;
    }

    fwrite(img, size, 1, file);
    fclose(file);
    printf("create a img [%s].\n", path);

    return 0;
}

int img_rotate(unsigned char *img, int w, int h, int pixel)
{
    int n, i, j, size, line_size;
    unsigned char *output;

    size = w * h * pixel;
    output = malloc(size);
    if (output == NULL)
    {
        printf("Error: failed to get a [%d] byte memory.\n", size);
        return -1;
    }

    memset(output, COLOR_WHITE, size);
    line_size = w * pixel;

    for (n = 0, j = w; j > 0; j--)
    {
        for (i = 0; i < h; i++)
        {
            memcpy(&output[n], &img[line_size * i + (j - 1) * pixel], pixel);
            n += pixel;
        }
    }

    memcpy(img, output, size);
    free(output);

    return 0;
}

unsigned char *png_read(int size, char *path)
{
    int ret = -1;
    FILE *fp = NULL;
    png_structp png = NULL;
    png_infop info = NULL;
    png_bytep *rows;
    unsigned char *img = NULL;
    int w, h, color, pixel_size;
    int x, y, i, row_size;

    if (path == NULL)
    {
        return NULL;
    }

    img = (unsigned char*)malloc(size);
    if (img == NULL)
    {
        printf("Error: failed to get a [%d] byte memory.\n", size);
        return NULL;
    }

    fp = fopen(path, "rb");
    if (fp == NULL)
    {
        printf("Error: failed to open [%s].\n", path);
        goto EXIT;
    }

    /* 读取PNG校验数据 */
    ret = fread(img, 1, PNG_BYTES_TO_CHECK, fp);
    if (ret < PNG_BYTES_TO_CHECK)
    {
        printf("Error: failed to read png head. (%s: [%d/%d] byte)\n",
               path, ret, PNG_BYTES_TO_CHECK);
        ret = -1;
        goto EXIT;
    }

    /* 检测PNG签名 */
    ret = png_sig_cmp((png_bytep)img, (png_size_t)0, PNG_BYTES_TO_CHECK);
    if (ret != 0)
    {
        printf("Error: failed to check the png sig. (%s: ret [%d])\n", path, ret);
        goto EXIT;
    }

    /* 复位文件指针 */
    rewind(fp);
    memset(img, COLOR_WHITE, size);

    png = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    info = png_create_info_struct(png);
    setjmp(png_jmpbuf(png));

    /* 开始读文件 */
    png_init_io(png, fp);

    /* 读取PNG图片信息和像素数据 */
    png_read_png(png, info, PNG_TRANSFORM_EXPAND, 0);

    /* 获取色彩类型 */
    color = png_get_color_type(png, info);
    if (color != PNG_COLOR_TYPE_RGB && color != PNG_COLOR_TYPE_RGB_ALPHA)
    {
        printf("Error: unsupport png color [%d]. (%s)\n", color, path);
        ret = -1;
        goto EXIT;
    }

    /* 获取宽高信息 */
    w = png_get_image_width(png, info);
    h = png_get_image_height(png, info);

    pixel_size = (color == PNG_COLOR_TYPE_RGB ? 3 : 4);
    x = w * h * 4;
    if (x > size)
    {
        printf("Error: unsupport PNG image data, %d x %d, size [%d/%d], color %s. (%s)\n",
               w, h, x, size, color == PNG_COLOR_TYPE_RGB ? "RGB" : "RGBA", path);
        ret = -1;
        goto EXIT;
    }

    /* 获取行像素数据，row是rgb/rgba数据 */
    rows = png_get_rows(png, info);
    for (i = 0, y = 0; y < h; ++y)
    {
        row_size = w * pixel_size;
        for (x = 0; x < row_size; x += pixel_size, i += 4)
        {
            memcpy(img + i, rows[y] + x, pixel_size);
        }
    }

    /* 图片旋转90度 */
    if (vinfo.xres == h && vinfo.yres == w)
    {
        // printf("Rotate png 90 degrees.(size: screen %dx%d, png %dx%d)\n",
        //     vinfo.xres, vinfo.yres, w, h);
        img_rotate(img, w, h, 4);
    }

EXIT:
    if (fp)
    {
        fclose(fp);
    }

    if (png && info)
    {
        png_destroy_read_struct(&png, &info, 0);
    }

    if (ret && img)
    {
        free(img);
        return NULL;
    }

    return img;
}

int img_is_png(char *path)
{
    int len;

    if (path == NULL)
    {
        return 0;
    }

    len = strlen(path);
    if (len > 4 && !strcmp((path + len - 4), ".png"))
    {
        return 1;
    }

    return 0;
}

unsigned char *img_read(int size, char *path)
{
    FILE *file;
    unsigned char *img;

    if (img_is_png(path))
    {
        return png_read(size, path);
    }

    img = (char*)malloc(size);
    if (img == NULL)
    {
        printf("Error: failed to get a [%d] byte memory.\n", size);
        return NULL;
    }

    memset(img, COLOR_WHITE, size);
    if (!path)
    {
        return img;
    }

    file = fopen(path, "rb");
    if (file == NULL)
    {
        printf("Error: failed to open [%s].\n", path);
        free(img);
        return NULL;
    }

    fread(img, size, 1, file);
    fclose(file);
    return img;
}

int img_mk(int size, char *path)
{
    int ret;
    unsigned char *img = img_read(size, path);

    if (img == NULL) {
        return -1;
    }

    if (path == NULL) {
        path = "img.rgba";
    }

    ret = img_save(size, img, path);
    free(img);

    return ret;
}

int img_diff(int size, char *img_path1, char *img_path2, char *diff_path)
{
    int ret = -1;
    int i;
    FILE *diff;
    unsigned char *img1;
    unsigned char *img2;

    rgba_info info = {0, 0, 0};
    int info_size = sizeof(rgba_info);

    printf("diff: (size %d) %s, %s => %s\n", size,
        img_path1 ? img_path1 : "blank white",
        img_path2 ? img_path2 : "img.rgba",
        diff_path ? diff_path : "img.diff");

    img1 = img_read(size, img_path1);
    if (img1 == NULL) {
        return -1;
    }

    if (img_path2 == NULL) {
        img_path2 = "img.rgba";
    }

    img2 = img_read(size, img_path2);
    if (img2 == NULL) {
        goto EXIT;
    }

    if (diff_path == NULL) {
        diff_path = "img.diff";
    }

    diff = fopen(diff_path, "wb+");
    if (diff == NULL) {
        goto EXIT;
    }

    for (i = 0; i < size; i++) {
        if (img1[i] != img2[i] || (i == (size - 1))) {
            if (info.len) {
                if (info.rgba == img2[i] && i == (info.id + info.len)) {
                    info.len++;
                    continue;
                }
                fwrite(&info, info_size, 1, diff);
            }
            info.id   = i;
            info.len  = 1;
            info.rgba = img2[i];
        }
    }
    printf("create a diff [%s].\n", diff_path);
    ret = 0;

EXIT:
    if (img1) {
        free(img1);
    }
    if (img2) {
        free(img2);
    }
    fclose(diff);

    return ret;
}

//将差异数据应用到给定的图像数据中
int diff_apply(int size, unsigned char *img, char *diff_path)
{
    int ret = -1;
    int info_size = sizeof(rgba_info);
    rgba_info info = {0, 0, 0};
    FILE *diff;

    diff = fopen(diff_path, "rb");
    if (diff == NULL)
    {
        return -1;
    }

    while (fread(&info, info_size, 1, diff) > 0)
    {
        if (info.id > size)
        {
            printf("Error: invalid diff [%s]. (max size: %d, current id: %d, color: 0x%x)\n",
                   diff_path, size, info.id, info.rgba);
            goto EXIT;
        }
        memset(&img[info.id], info.rgba, info.len);
    }

    ret = 0;

EXIT:
    fclose(diff);
    return ret;
}

int get_screen_size(void)
{
    int fd;
    int size = 307200;   // (240x320) 300K

    fd = open(DEV_FB, O_RDWR);
    if (fd == -1)
    {
        printf("Error: failed to open %s\n", DEV_FB);
        return size;
    }

    if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo))
    {
        printf("Error: reading fixed information.\n");
        close(fd);
        return size;
    }

    if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo))
    {
        printf("Error: reading variable information.\n");
        close(fd);
        return size;
    }
    close(fd);
    size = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
    printf("screen_size:%d  vinfo.xres:%d  vinfo.yres:%d  vinfo.bits_per_pixel:%d\n",
           size, vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);
    return size;
}