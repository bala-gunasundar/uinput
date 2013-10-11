#include <stdio.h>
#include <maxim.h>
#include "set_termios.h"
#include <stdint.h>

void putx(char *str, uint8_t * buf, uint8_t size)
{
	int i;
	printf("\n%s: ", str);

	for (i = 0; i < size; i++) {
		printf("0x%x ", buf[i]);
	}

	printf("\n");
}

int main()
{
	int fd;
	uint8_t buf[10];
	
	fd = maxim_init("/dev/ttymxc0", 230400, 1, 8, 'e', 0);
	maxim_read(fd, 0x40, 0x00, 10, buf);

	putx("R", buf, 10);
}
