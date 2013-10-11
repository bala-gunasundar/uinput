#include "set_termios.h"
#include <termios.h>
#include <stdint.h>
#include <string.h>

typedef struct struct_speed {
	uint32_t speed;
	uint16_t cflag;
} u_speed;

u_speed select_speed[] = {
	{9600, B9600},
	{19200, B19200},
	{38400, B38400},
	{57600, B57600},
	{115200, B115200},
	{230400, B230400},
	{460800, B460800},
	{500000, B500000},
	{576000, B576000},
	{921600, B921600},
	{1000000, B1000000},
	{0, 0}
};

void uart_set_termios(int fd, uint32_t speed, uint8_t stop_bits,
		      uint8_t data_bits, uint8_t parity, uint8_t flowcontrol)
{
	int i = 0;
	struct termios uart_settings;

	tcgetattr(fd, &uart_settings);
	memset(&uart_settings, 0, sizeof(uart_settings));

	do {
		if (select_speed[i].speed == speed) {
			cfsetispeed(&uart_settings, select_speed[i].cflag);
			cfsetospeed(&uart_settings, select_speed[i].cflag);
			break;
		}
		i++;
	} while (select_speed[i].speed != 0);

	uart_settings.c_cflag |= (CLOCAL | CREAD);

	if (stop_bits == 2)
		uart_settings.c_cflag |= CSTOPB;

	uart_settings.c_cflag &= ~CSIZE;

	switch (data_bits) {
	case 8:
		uart_settings.c_cflag |= CS8;
		break;
	case 7:
		uart_settings.c_cflag |= CS7;
		break;
	case 6:
		uart_settings.c_cflag |= CS6;
		break;
	case 5:
		uart_settings.c_cflag |= CS5;
		break;
	}

	if (parity == 'o') {
		uart_settings.c_cflag |= PARENB;
		uart_settings.c_cflag |= PARODD;
	} else if (parity == 'e') {
		uart_settings.c_cflag |= PARENB;
		uart_settings.c_cflag &= ~PARODD;
	} else {
		uart_settings.c_cflag &= ~PARENB;
	}

	if (flowcontrol == 1) {
		uart_settings.c_cflag |= CRTSCTS;
	} else {
		uart_settings.c_cflag &= ~CRTSCTS;
	}

	uart_settings.c_iflag &=
	    ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	uart_settings.c_oflag &= ~OPOST;
	uart_settings.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

	uart_settings.c_cc[VMIN] = 1;
	uart_settings.c_cc[VTIME] = 0;

	tcflush(fd, TCIOFLUSH);

	tcsetattr(fd, TCSANOW, &uart_settings);
}
