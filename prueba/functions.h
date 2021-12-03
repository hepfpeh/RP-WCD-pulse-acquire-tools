#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include "font.h"

#define OLED96_ADDR     0x3c
#define PAGESIZE        32

int fd;
void *bmp;

void page_data(int, int, int);
void initialize(int);
void clear_lcd(int);
void lcd_num(int, int);

void abecedario(int, int, int);
void numeros(int, int, int);
void digitos(int, int, int, int ,int);
void Text_alt(int, int, int);
void Text_Pres(int, int, int);
void Text_temp(int, int, int);
void Bienvenida(int, int, int);
void datos(void *, int);
void inic_disp();
void close_disp(void *, int);

#endif /*FUNCTIONS_H*/