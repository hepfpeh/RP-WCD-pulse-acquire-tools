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

#define OLED96_ADDR     0x3c

#define PAGESIZE        32
/* eeprom size on a redpitaya */

int fd;

void digitos(int, int, int, int, int);
void Text_temp(int, int, int);
void Text_alt(int, int, int);
void Text_Pres(int, int, int);
int tempera;
int altu;
int pres;
float t;
long p;
float alt;

int main(int argc, char **argv){
	int status;
    fd = open("/dev/i2c-0", O_RDWR);

    if(fd < 0)
    {
        printf("Cannot open the IIC device\n");
        return 1;
    }

    status = ioctl(fd, I2C_SLAVE, OLED96_ADDR);
    if(status < 0)
    {
        printf("Unable to set the OLED96 address\n");
        return -1;
    }
    if ( i2c_smbus_write_byte_data(fd, 0x00, DISPLAY_OFF) < 0 )
    {
        printf("Unable to send commands\n");
        printf("errno: %i %s\n",errno,strerror(errno));
        return -1;
    }
	
	initialize(fd);
    clear_lcd(fd);

	char *i2c_device = "/dev/i2c-0";
	int address = 0x77;
	void *bmp = bmp180_init(address, i2c_device);
	
	bmp180_eprom_t eprom;
	bmp180_dump_eprom(bmp, &eprom);
	bmp180_set_oss(bmp, 1);

	if(bmp != NULL){
		int i;
		for(i = 0; i < 25; i++) {
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
			digitos(fd, 0x00, 0x32, tempera,1);
			Text_alt(fd, 0x32, 0x00);
			digitos(fd, 0x32, 0x32, altu,3);
			sleep(2);
			clear_lcd(fd);
			Text_Pres(fd, 0x00, 0x00);
			digitos(fd, 0x32, 0x00, p,2);
			sleep(2);
			clear_lcd(fd);
		}
	bmp180_close(bmp);
	}
	close(fd);
	return 0;
}