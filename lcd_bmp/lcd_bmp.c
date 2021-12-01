#include "bmp180.h"
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include "font.h"
#include "functions.h"

#define OLED96_ADDR     0x3c
#define PAGESIZE        32

int fd;
void *bmp;

int main(int argc, char **argv){
	
    inic_disp();
	
	if(bmp != NULL){
		int i;
		Bienvenida(fd,0x32,0x00);
		for(i = 0; i < 25; i++) {
			datos(bmp, fd);
		}
	}
	
	close_disp(bmp,fd);

	return 0;
}

void inic_disp(){
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
	initialize(fd);
	clear_lcd(fd);
	
	bmp = bmp180_init(address, i2c_device);
	
	bmp180_eprom_t eprom;
	bmp180_dump_eprom(bmp, &eprom);
	bmp180_set_oss(bmp, 1);
}