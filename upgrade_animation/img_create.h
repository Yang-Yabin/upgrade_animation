#ifndef _IMG_PRINT_H
#define _IMG_PRINT_H

/*****************************************************************************
 Prototype    : img_save
 Description  : ��ͼƬ���ݱ��浽ָ�����ļ���               
 Input        : int                         size   ͼ��ֱ���
                unsigned char*              img    �����Ӧ��diff�ļ����ͼ������
                char*                       path   ָ����ͼ�񱣴�·��
 Output       : None
 Return Value : int                        - �ɹ����� 0,ʧ�������������
*****************************************************************************/
extern int img_save(int size, unsigned char *img, char *path);

/*****************************************************************************
 Prototype    : img_rotate
 Description  : ͼ��������ת90��               
 Input        : unsigned char*              img    �����Ӧ��diff�ļ����ͼ������
                int                         w      ͼ��Ŀ��
                int                         h      ͼ��ĸ߶�
                int                         pixel  ÿ��ͼ��ռ�õ��ֽ���
 Output       : None
 Return Value : int                        - �ɹ����� 0,ʧ�������������
*****************************************************************************/
extern int img_rotate(unsigned char *img, int w, int h, int pixel);

/*****************************************************************************
 Prototype    : png_read
 Description  : ��ȡPNG�ļ����������Ϊͼ������
                ������libpng��������PNG�ļ���RGB��RGBA����               
 Input        : int                         size   ͼ��ֱ���
                char*                       path   ������PNG�ļ�·��
 Output       : None
 Return Value : unsigned char*             - �ɹ����� *img,ʧ�ܷ��ؿ�ָ��
*****************************************************************************/
extern unsigned char *png_read(int size, char *path);

/*****************************************************************************
 Prototype    : img_is_png
 Description  : ������·���ļ��Ƿ���PNG�ļ�               
 Input        : char*              path    �ļ�·��
 Output       : None
 Return Value : int                        - �����ȷ���� 1 ,ʧ�ܷ��� 0
*****************************************************************************/
extern int img_is_png(char *path);

/*****************************************************************************
 Prototype    : img_read
 Description  : ��ΪPNG�ļ��������Ϊͼ�����ݣ���·��Ϊ���򷵻ذ�ɫͼƬ��
                ��ΪDIFF�ļ����ȡ
                              
 Input        : int                         size   ͼ��ֱ���
                char*                       path   ������/��ȡ���ļ�·��
 Output       : None
 Return Value : unsigned char*             - �ɹ����� *img,ʧ�ܷ��ؿ�ָ��
*****************************************************************************/
extern unsigned char *img_read(int size, char *path);

/*****************************************************************************
 Prototype    : img_mk
 Description  : ��ͼƬ���ݱ���Ϊָ���ֱ��ʵ�ͼƬ

 Input        : int                         size   ͼ��ֱ���
                char*                       path   ָ����ͼ�񱣴�·��
 Output       : None
 Return Value : unsigned                     - �ɹ����� *img,ʧ�ܷ��ؿ�ָ��
*****************************************************************************/
extern int img_mk(int size, char *path);

/*****************************************************************************
 Prototype    : img_diff
 Description  : �Ƚ�����ͼ��Ĳ��죬����������Ϣ���浽�ļ���               
 Input        : int                         size   ͼ��ֱ���
                char*                       img_path1   ��һ��ͼƬ·��
                char*                       img_path2   �ڶ���ͼƬ·��
                char*                       diff_path   �����ļ�����·��
 Output       : None
 Return Value : int                        - �ɹ����� 0
*****************************************************************************/
extern int img_diff(int size, char *img_path1, char *img_path2, char *diff_path);

/*****************************************************************************
 Prototype    : diff_apply
 Description  : �������ļ�Ӧ�õ�PNG�ļ���              
 Input        : int                         size        ͼ��ֱ���
                unsigned char*              img         PNGͼƬ·��
                char*                       diff_path   �����ļ�·��
 Output       : None
 Return Value : int                        - �ɹ�����ret(0),ʧ�����������Ϣ
*****************************************************************************/
extern int diff_apply(int size, unsigned char *img, char *diff_path);

/*****************************************************************************
 Prototype    : get_screen_size
 Description  : ��ȡLCD�豸�ֱ���             
 Input        : Noen
 Output       : None
 Return Value : int                        - �ɹ�����size,ʧ�����������Ϣ
*****************************************************************************/
extern int get_screen_size(void);

#endif

