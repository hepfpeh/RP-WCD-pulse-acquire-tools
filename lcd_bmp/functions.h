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

////////////////////////////////////////////////////////////////////////////////

void page_data(int fd, int page_a, int column_a){
    i2c_smbus_write_byte_data(fd, 0x00, SET_PAGE_ADDR);
    i2c_smbus_write_byte_data(fd, 0x00, page_a);
    i2c_smbus_write_byte_data(fd, 0x00, INIT_STATUS);
    i2c_smbus_write_byte_data(fd, 0x00, SET_COLUMN_ADDR);
    i2c_smbus_write_byte_data(fd, 0x00, column_a);
    i2c_smbus_write_byte_data(fd, 0x00, 0x7f);
}

void initialize(int fd){
    i2c_smbus_write_byte_data(fd, 0x00, SET_OSC_FREQ);      
    i2c_smbus_write_byte_data(fd, 0x00, 0x80);
    i2c_smbus_write_byte_data(fd, 0x00, SET_MUX_RATIO);
    i2c_smbus_write_byte_data(fd, 0x00, 0x1f);
    i2c_smbus_write_byte_data(fd, 0x00, SET_DISP_OFFSET);
    i2c_smbus_write_byte_data(fd, 0x00, 0x00);
    i2c_smbus_write_byte_data(fd, 0x00, SET_START_LINE | 0x0 );
    i2c_smbus_write_byte_data(fd, 0x00, SET_CHAR_REG);
    i2c_smbus_write_byte_data(fd, 0x00, 0x14);
    i2c_smbus_write_byte_data(fd, 0x00, MEMORY_ADDR_MODE);
    i2c_smbus_write_byte_data(fd, 0x00, 0x00);
    i2c_smbus_write_byte_data(fd, 0x00, SET_SEG_REMAP | 0x1 );
    i2c_smbus_write_byte_data(fd, 0x00, COM_SCAN_DIR_OP);
    i2c_smbus_write_byte_data(fd, 0x00, SET_COM_PIN_CFG);
    i2c_smbus_write_byte_data(fd, 0x00, 0x02); // x12 **
    i2c_smbus_write_byte_data(fd, 0x00, SET_CONTRAST);
    i2c_smbus_write_byte_data(fd, 0x00, 0x7f);
    i2c_smbus_write_byte_data(fd, 0x00, SET_PRECHARGE);
    i2c_smbus_write_byte_data(fd, 0x00, 0xc2); //xc2
    i2c_smbus_write_byte_data(fd, 0x00, 0xd8);
    i2c_smbus_write_byte_data(fd, 0x00, SET_START_LINE);
    i2c_smbus_write_byte_data(fd, 0x00, SET_ENTIRE_ON);
    i2c_smbus_write_byte_data(fd, 0x00, DISPLAY_NORMAL);
    i2c_smbus_write_byte_data(fd, 0x00, DEACT_SCROLL);
    i2c_smbus_write_byte_data(fd, 0x00, DISPLAY_ON);
}

void numeros(int fd, int page, int column){
    page_data(fd, page, column);
    N_1(fd);
    N_2(fd);
    N_3(fd);
    N_4(fd);
    N_5(fd);
    N_6(fd);
    N_7(fd);
    N_8(fd);
    N_9(fd);
    N_0(fd);
}

void abecedario(int fd, int page, int column){
    page_data(fd, page, column);
    A_M(fd);
    B_M(fd);
    C_M(fd);
    D_M(fd);
    E_M(fd);
    F_M(fd);
    G_M(fd);
    H_M(fd);
    I_M(fd);
    J_M(fd);
    K_M(fd);
    L_M(fd);
    M_M(fd);
    N_M(fd);
    O_M(fd);
    P_M(fd);
    V_M(fd);
    W_M(fd);
    X_M(fd);
    Y_M(fd);
    Z_M(fd);
}

void clear_lcd(int fd){
    page_data(fd, 0x00, 0x00);
    for( int i = 0; i< 512; i++){
    i2c_smbus_write_byte_data(fd, 0x40, 0x00);
    }
}

void digitos(int fd,int page, int column, int num, int caso){
	int lista[10];
	int j=0;
	int i;
	page_data(fd, page, column);
	while(num > 0) //do till num greater than  0
    {
        int mod = num % 10;  //split last digit from number
		lista[j]=mod;
        num = num / 10;    //divide num by 10. num /= 10 also a valid one 
    	j++;
    }
	for(i=j-1;i>=0;i--){
		if(i==0){
			dot(fd);
			lcd_num(fd, lista[i]);
		}
		lcd_num(fd, lista[i]);
	}
	if(caso==1){
		space(fd);
		C_M(fd);
	}
	if(caso==2){
		space(fd);
		P_M(fd);
		A_M(fd);
	}
	if(caso==3){
		space(fd);
		M_M(fd);
	}
}

void Text_temp(int fd,int page, int column){
	page_data(fd, page, column);
	T_M(fd);
	E_M(fd);
	M_M(fd);
	P_M(fd);
	equal(fd);
}

void Text_alt(int fd,int page, int column){
	page_data(fd, page, column);
	A_M(fd);
	L_M(fd);
	T_M(fd);
	I_M(fd);
	T_M(fd);
	U_M(fd);
	D_M(fd);
	equal(fd);
}

void Text_Pres(int fd,int page, int column){
	page_data(fd, page, column);
	P_M(fd);
	R_M(fd);
	E_M(fd);
	S_M(fd);
	I_M(fd);
	O_M(fd);
	N_M(fd);
	equal(fd);
}

void lcd_num(int fd, int a){
	if(a==1){
		N_1(fd);
	}
	if(a==2){
		N_2(fd);
	}
	if(a==3){
		N_3(fd);
	}
	if(a==4){
		N_4(fd);
	}
	if(a==5){
		N_5(fd);
	}
	if(a==6){
		N_6(fd);
	}
	if(a==7){
		N_7(fd);
	}
	if(a==8){
		N_8(fd);
	}
	if(a==9){
		N_8(fd);
	}
	if(a==0){
		N_0(fd);
	}	
}

void Bienvenida(int fd,int page, int column){
    page_data(fd, page, column);
    B_M(fd);
    i_MI(fd);
    e_MI(fd);
    n_MI(fd);
    v_MI(fd);
    e_MI(fd);
    n_MI(fd);
    i_MI(fd);
    d_MI(fd);
    o_MI(fd);
    s_MI(fd);
    sleep(3);
    clear_lcd(fd);
}


void close_disp(void *bmp, int fd){
    bmp180_close(bmp);
    close(fd);
}

void datos(void *bmp, int fd){
	float t;
	long p;
	float alt;
    int tempera;
    int altu;
    
	t = bmp180_temperature(bmp);
	p = bmp180_pressure(bmp);
	alt = bmp180_altitude(bmp);
	printf("Temperature = %.1f, Pressure = %lu, Altitude= %.1f\n", t, p, alt);
	//usleep(2 * 1000 * 1000);
	t=t*10;
	alt=alt*10;
	altu=(int)alt;
	tempera=(int)t;
	p=p*10;
	Text_temp(fd, 0x00, 0x00);
	digitos(fd, 0x00, 0x32, tempera, 1);
	Text_alt(fd, 0x32, 0x00);
	digitos(fd, 0x32, 0x32, altu, 3);
	sleep(2);
	clear_lcd(fd);
	Text_Pres(fd, 0x00, 0x00);
	digitos(fd, 0x32, 0x00, p, 2);
	sleep(2);
	clear_lcd(fd);
}

#endif /*FUNCTIONS_H*/