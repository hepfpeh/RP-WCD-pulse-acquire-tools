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
#include "functions.h"
#include "bmp180.h"

char abecedary_lower[]="abcdefghijklmnopqrstuvwxyz";
char abecedary_upper[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ";
char special_signs[]=" :*=.";

void fun_page_data(int fd, int page_a, int column_a){
    i2c_smbus_write_byte_data(fd, 0x00, SET_PAGE_ADDR);
    i2c_smbus_write_byte_data(fd, 0x00, page_a);
    i2c_smbus_write_byte_data(fd, 0x00, INIT_STATUS);
    i2c_smbus_write_byte_data(fd, 0x00, SET_COLUMN_ADDR);
    i2c_smbus_write_byte_data(fd, 0x00, column_a);
    i2c_smbus_write_byte_data(fd, 0x00, 0x7f);
}

void fun_initialize(int fd){
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


void fun_clear_lcd(int fd){
    fun_page_data(fd, 0x00, 0x00);
    for( int i = 0; i< 512; i++){
    i2c_smbus_write_byte_data(fd, 0x40, 0x00);
    }
}

void fun_digits(int fd, int num){
	int lista[10];
	int j=0;
	int i;
	while(num > 0){
        int mod = num % 10;
		lista[j]=mod;
        num = num / 10;
    	j++;
    }
	for(i=j-1;i>=0;i--){
		if(i==0){
			dot(fd);
			fun_lcd_num(fd, lista[i]);
		}
		fun_lcd_num(fd, lista[i]);
	}
}

void fun_bienv(int fd){
    fun_page_data(fd, 0x00, 0x00);
    fun_println(fd, "Bienvenidos");
	fun_page_data(fd, 0x32, 0x00);
	fun_println(fd, "LAGO GT");
    sleep(3);
    fun_clear_lcd(fd);
}


void fun_close_disp(void *bmp, int fd){
    bmp180_close(bmp);
    close(fd);
}

void fun_data(void *bmp, int fd){
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
	fun_page_data( fd, 0x00, 0x00);
	fun_println(fd, "Temperatura = ");
	fun_digits(fd, tempera);
	fun_println(fd, " C");

	fun_page_data( fd, 0x32, 0x00);
	fun_println(fd, "Altura = ");
	fun_digits(fd, altu);
	fun_println(fd, " m");

	sleep(2);

	fun_clear_lcd(fd);
	fun_page_data( fd, 0x00, 0x00);
	fun_println(fd, "Presion = ");
	fun_digits(fd, p);
	fun_println(fd, " Pa");

	sleep(2);
	fun_clear_lcd(fd);
}

void fun_inic_disp(){
	int status;
	char *i2c_device = "/dev/i2c-0";
    int address = 0x77;

	fd = open("/dev/i2c-0", O_RDWR);
	if(fd < 0)
    {
        printf("Cannot open the IIC device\n");
    }

    status = ioctl(fd, I2C_SLAVE, OLED96_ADDR);
    if(status < 0)
    {
        printf("Unable to set the OLED96 address\n");
    }
    if ( i2c_smbus_write_byte_data(fd, 0x00, DISPLAY_OFF) < 0 )
    {
        printf("Unable to send commands\n");
        printf("errno: %i %s\n",errno,strerror(errno));
    }
	fun_initialize(fd);
	fun_clear_lcd(fd);
	
	bmp = bmp180_init(address, i2c_device);
	
	bmp180_eprom_t eprom;
	bmp180_dump_eprom(bmp, &eprom);
	bmp180_set_oss(bmp, 1);
}

void fun_lcd_num(int fd, int a){
	if(a==1){N_1(fd);}
	if(a==2){N_2(fd);}
	if(a==3){N_3(fd);}
	if(a==4){N_4(fd);}
	if(a==5){N_5(fd);}
	if(a==6){N_6(fd);}
	if(a==7){N_7(fd);}
	if(a==8){N_8(fd);}
	if(a==9){N_8(fd);}
	if(a==0){N_0(fd);}	
}

void fun_println(int fd, char arr[]){
	int i;
	for(i=0 ; i<strlen(arr); i++){
		if(arr[i]==abecedary_upper[0]){A_M(fd);}
		if(arr[i]==abecedary_upper[1]){B_M(fd);}
		if(arr[i]==abecedary_upper[2]){C_M(fd);}
		if(arr[i]==abecedary_upper[3]){D_M(fd);}
		if(arr[i]==abecedary_upper[4]){E_M(fd);}
		if(arr[i]==abecedary_upper[5]){F_M(fd);}
		if(arr[i]==abecedary_upper[6]){G_M(fd);}
		if(arr[i]==abecedary_upper[7]){H_M(fd);}
		if(arr[i]==abecedary_upper[8]){I_M(fd);}
		if(arr[i]==abecedary_upper[9]){J_M(fd);}
		if(arr[i]==abecedary_upper[10]){K_M(fd);}
		if(arr[i]==abecedary_upper[11]){L_M(fd);}
		if(arr[i]==abecedary_upper[12]){M_M(fd);}
		if(arr[i]==abecedary_upper[13]){N_M(fd);}
		if(arr[i]==abecedary_upper[14]){O_M(fd);}
		if(arr[i]==abecedary_upper[15]){P_M(fd);}
		if(arr[i]==abecedary_upper[16]){Q_M(fd);}
		if(arr[i]==abecedary_upper[17]){R_M(fd);}
		if(arr[i]==abecedary_upper[18]){S_M(fd);}
		if(arr[i]==abecedary_upper[19]){T_M(fd);}
		if(arr[i]==abecedary_upper[20]){U_M(fd);}
		if(arr[i]==abecedary_upper[21]){V_M(fd);}
		if(arr[i]==abecedary_upper[22]){W_M(fd);}
		if(arr[i]==abecedary_upper[23]){X_M(fd);}
		if(arr[i]==abecedary_upper[24]){Y_M(fd);}
		if(arr[i]==abecedary_upper[25]){Z_M(fd);}
		if(arr[i]==abecedary_lower[0]){a_MI(fd);}
		if(arr[i]==abecedary_lower[1]){b_MI(fd);}
		if(arr[i]==abecedary_lower[2]){c_MI(fd);}
		if(arr[i]==abecedary_lower[3]){d_MI(fd);}
		if(arr[i]==abecedary_lower[4]){e_MI(fd);}
		if(arr[i]==abecedary_lower[5]){f_MI(fd);}
		if(arr[i]==abecedary_lower[6]){g_MI(fd);}
		if(arr[i]==abecedary_lower[7]){h_MI(fd);}
		if(arr[i]==abecedary_lower[8]){i_MI(fd);}
		if(arr[i]==abecedary_lower[9]){j_MI(fd);}
		if(arr[i]==abecedary_lower[10]){k_MI(fd);}
		if(arr[i]==abecedary_lower[11]){l_MI(fd);}
		if(arr[i]==abecedary_lower[12]){m_MI(fd);}
		if(arr[i]==abecedary_lower[13]){n_MI(fd);}
		if(arr[i]==abecedary_lower[14]){o_MI(fd);}
		if(arr[i]==abecedary_lower[15]){p_MI(fd);}
		if(arr[i]==abecedary_lower[16]){q_MI(fd);}
		if(arr[i]==abecedary_lower[17]){r_MI(fd);}
		if(arr[i]==abecedary_lower[18]){s_MI(fd);}
		if(arr[i]==abecedary_lower[19]){t_MI(fd);}
		if(arr[i]==abecedary_lower[20]){u_MI(fd);}
		if(arr[i]==abecedary_lower[21]){v_MI(fd);}
		if(arr[i]==abecedary_lower[22]){w_MI(fd);}
		if(arr[i]==abecedary_lower[23]){x_MI(fd);}
		if(arr[i]==abecedary_lower[24]){y_MI(fd);}
		if(arr[i]==abecedary_lower[25]){z_MI(fd);}
		if(arr[i]==special_signs[0]){space(fd);}
		if(arr[i]==special_signs[1]){colon(fd);}
		if(arr[i]==special_signs[2]){asterisk(fd);}
		if(arr[i]==special_signs[3]){equal(fd);}
		if(arr[i]==special_signs[4]){dot(fd);}
	}
}