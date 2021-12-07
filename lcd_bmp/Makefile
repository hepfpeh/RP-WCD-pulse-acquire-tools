
SENSOR=bmp180

all:
	gcc -Wall -c $(SENSOR).c -o $(SENSOR).o -lm
	gcc -Wall $(SENSOR).o lcd_bmp.c -o lcd_bmp -lm

clean:
	rm *.o > /dev/null 2>&1 &
