
SENSOR=bmp180
FUNCTIONS=functions
C_COMPILER=gcc
#C_COMPILER=/opt/Xilinx/SDK/2017.2/gnu/aarch32/lin/gcc-arm-linux-gnueabi/bin/arm-linux-gnueabihf-gcc

all:
	$(C_COMPILER) -Wall -c $(SENSOR).c -o $(SENSOR).o -lm
	$(C_COMPILER) -Wall -c $(FUNCTIONS).c -o $(FUNCTIONS).o -lm
	$(C_COMPILER) -Wall $(SENSOR).o $(FUNCTIONS).o lcd_bmp.c -o lcd_bmp -lm

clean:
	rm *.o > /dev/null 2>&1 &
