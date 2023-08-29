.SUFFIXES :
.SUFFIXES : .s .c .o .a

NAME     = boot_animation
IMG_NAME = img_print

CC :=/usr/local/arm/rk3308_linux_32_app/bin/arm-rockchip-linux-gnueabihf-gcc
export LD_LIBRARY_PATH=/usr/local/arm/rk3308_linux_32_app/lib
S_PATH   = .
C_PATH   = .
O_PATH   = .
I_PATH   =	$(UCFG_SYSTEM_DIR)/../build/libpng-1.6.34 \
			$(UCFG_SYSTEM_DIR)/../build/libdrm-2.4.89 \
			$(UCFG_SYSTEM_DIR)/../build/libdrm-2.4.89/include/drm \
			$(UCFG_SYSTEM_DIR)/usr/local/arm/rk3308_linux_32_app \
			$(UCFG_SYSTEM_DIR)/usr/local/arm/rk3308_linux_32_app/arm-rockchip-linux-gnueabihf/sysroot/usr/include/drm
A_PATH   = .
L_PATH   =	$(UCFG_SYSTEM_DIR)/../build/libdrm-2.4.89/.libs/ \
			/usr/local/arm/rk3308_linux_32_app/lib
VPATH    = .:$(S_PATH):$(C_PATH):$(O_PATH):$(I_PATH):$(A_PATH)
	
S_FILES  =
C_FILES  = 
#C_FILES  = $(wildcard $(C_PATH)/*.c)
CPP_FILES=
O_FILES  =	$(patsubst %.c, %.o, $(notdir $(S_FILES))) \
			$(patsubst %.c, %.o, $(notdir $(C_FILES))) \
			$(patsubst %.c, %.o, $(notdir $(CPP_FILES)))

A_FILES  = z png drm 

LINK_O   = $(addprefix $(O_PATH)/, $(O_FILES))
LINK_P   = $(addprefix -L, $(L_PATH))  
LINK_A   = $(addprefix -l, $(A_FILES))
OTHER_I  = $(addprefix -I, $(I_PATH))

CFLAGS  += $(OTHER_I)

LCD      = 320x240


all: $(NAME) $(IMG_NAME)

$(NAME): $(LINK_O) img_create.o
	$(CC) $(CFLAGS) $(LINK_O) img_create.o boot_animation.c -o $(NAME) $(LINK_P) $(LINK_A)
	chmod +x $(NAME)
	rm -rf img $(LCD); cp -rf imgs/$(LCD) . && mv $(LCD) img

$(IMG_NAME): img_print.o img_create.o img_display.o
	$(CC) $(CFLAGS) $(LINK_O) img_create.o img_display.o img_print.c -o $(IMG_NAME) $(LINK_P) $(LINK_A)
	chmod +x $(IMG_NAME)

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $(O_PATH)/$@

romfs :
	rm -rf $(TARGET_ROMFS_PATH)/img $(TARGET_ROMFS_PATH)/$(LCD)
	cp -rf imgs/$(LCD) $(TARGET_ROMFS_PATH) && mv $(TARGET_ROMFS_PATH)/$(LCD) $(TARGET_ROMFS_PATH)/img
	chmod +x $(NAME)
	chmod +x $(IMG_NAME)
	cp -rf $(NAME) upgrade_ui $(TARGET_ROMFS_PATH)/bin


clean :
	$(RM) -rf $(O_PATH)/*.o
	$(RM) -rf $(NAME) $(IMG_NAME)
	

clean_all : clean
