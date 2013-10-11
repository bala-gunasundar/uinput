CC = arm-oe-linux-gnueabi-gcc 
OBJS = main.o maxim.o set_termios.o us_gpio.o j36.o 
INCLUDE_PATH = ../include

j36 : $(OBJS)
	$(CC) $(OBJS) -I$(INCLUDE_PATH) -lrt -o $@ 

%.o : %.c
	$(CC) -I$(INCLUDE_PATH) -c $<

clean:
	rm -rf j36
	rm -rf *.o

dist-clean:
	rm -rf *~
