#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <error.h>
#include <errno.h>
#include "gmsl_uart.h"
#include <termios.h>
#include <stdint.h>
#include <sys/select.h>
#include "us_gpio.h"
#include "maxim.h"

enum i2c_states {
	I2C_STATE_ACK, I2C_STATE_DATA
};

#define PORT_TO_NUMBER(port, pin)  (((port - 1) * 32) + pin)

#define SYSFS_CMD_DELAY            2000

#define MAX_PIN_PWDN               PORT_TO_NUMBER(3, 7)
#define MAX_PIN_MS                 PORT_TO_NUMBER(3, 14)
#define MAX_PIN_AUTOS              PORT_TO_NUMBER(3, 6)
#define MAX_PIN_CDS                PORT_TO_NUMBER(6, 6)
#define MAX_PIN_BWS                PORT_TO_NUMBER(5, 12)
#define MAX_PIN_CONF0              PORT_TO_NUMBER(4, 15)
#define MAX_PIN_CONF1              PORT_TO_NUMBER(3, 30)


static int write_in_loop(int fd, uint8_t * buf, uint8_t wsize)
{
	ssize_t wlen;
	uint8_t len;
	uint8_t *bufp;

	bufp = buf;
	len = wsize;

	do {
		wlen = write(fd, bufp, len);
		if (wlen == -1) {
			error(0, errno, "Write error");
			return -1;
		}
		bufp += wlen;
		len -= wlen;
	} while (len != 0);

	return 0;
}

int maxim_write(int fd, uint8_t slave_add, uint8_t reg_add, int len, uint8_t *buf)
{
	uint8_t cmd[SIZE_GMSL_HEADER];
	fd_set rfds;
	struct timeval tv;
	char ack;
	ssize_t rlen;
	ssize_t wlen;

	tcdrain(fd);
	usleep(139);		/* wait 16 bit times (assuming 115200 baud) */

	cmd[0] =  GMSL_SYNC_BYTE;
	cmd[1] = (slave_add << 1) | 0;
	cmd[2] = reg_add;
	cmd[3] = len;

	wlen = write_in_loop(fd, cmd, SIZE_GMSL_HEADER);
	if (wlen < 0)
		goto write_err;
	wlen = write_in_loop(fd, buf, len);
	if (wlen < 0)
		goto write_err;

	tcdrain(fd);

	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	tv.tv_sec = 0;
	tv.tv_usec = 1000;

	if (select(fd + 1, &rfds, NULL, NULL, &tv)) {
		rlen = read(fd, &ack, 1);
		if (ack != ACK_SERDES) {
			puts("Invalid serdes ack");
			goto write_err;
		}
	} else {
		puts("Read ack time out");
		return -1;
	}
	
	return 0;

write_err:
	tcflush(fd, TCIOFLUSH);
	return -1;
}

int maxim_read(int fd, uint8_t slave_add, uint8_t reg_add, int len, uint8_t *buf)
{
	uint8_t cmd[SIZE_GMSL_HEADER];
	ssize_t wlen;
	ssize_t rlen;
	fd_set rfds;
	struct timeval tv;
	int i;
	uint8_t ch;
	uint8_t state;
	int count;
	
	tcdrain(fd);
	usleep(10 * 139);	/* wait 16 bit times (assuming 115200 baud) - actually times five */
	
	cmd[0] = GMSL_SYNC_BYTE;;
	cmd[1] = (slave_add << 1) | 1;
	cmd[2] = reg_add;
	cmd[3] = len;

	wlen = write_in_loop(fd, cmd, 4);
	if (wlen < 0) {
		error(0, errno, "Write error");
		goto read_err;
	}

	state = I2C_STATE_ACK;

	for (i = 0; i < len + 1; i++) {
		
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		tv.tv_sec = 0;
		tv.tv_usec = 1000;
		
		if (select(fd + 1, &rfds, NULL, NULL, &tv)) {
			rlen = read(fd, &ch, 1);
			if (rlen < 0) {
				error(0, errno, "Read err");
				goto read_err;
			}
			
			switch (state) {
				
			case I2C_STATE_ACK:
				if (ch == ACK_SERDES) {
					state = I2C_STATE_DATA;
					count = 0;
				} else {
					puts("Invalid ack");
					goto read_err;
				}
				break;

			case I2C_STATE_DATA:
				buf[count++] = ch;
				break;
			};

		} else {
			puts("Read time out");
			goto read_err;
		};
	};

	return 0;

read_err:
	tcflush(fd, TCIOFLUSH);
	return -1;	
}

void serdes_config_gpio(uint16_t gpio, int val)
{
	us_gpio_export(gpio);
	usleep(SYSFS_CMD_DELAY);
	us_gpio_direction_output(gpio, val);
	usleep(SYSFS_CMD_DELAY);
	us_gpio_set_value(gpio, val);
	usleep(SYSFS_CMD_DELAY);
}

void serdes_power_on(void)
{
	serdes_config_gpio(MAX_PIN_AUTOS, 0);
	serdes_config_gpio(MAX_PIN_CDS, 0);
	serdes_config_gpio(MAX_PIN_BWS, 1);
	serdes_config_gpio(MAX_PIN_CONF0, 0);
	serdes_config_gpio(MAX_PIN_CONF1,0);
	serdes_config_gpio(MAX_PIN_MS, 0);
	serdes_config_gpio(MAX_PIN_PWDN, 0);

	usleep(SYSFS_CMD_DELAY);
	us_gpio_set_value(MAX_PIN_PWDN, 1);


	puts("maxim powered on");
}

int maxim_init(uint8_t *gmsl_uart, uint32_t speed, uint8_t stop_bits,
	       uint8_t data_bits, uint8_t parity, uint8_t flow_ctl)
{
	int fd;

	serdes_power_on();

	fd = open(GMSL_UART, (O_RDWR | O_NOCTTY));
	if (fd < 0) {
		error(0, errno, "Error while opening GMSL UART");
		return -1;
	}
	
	uart_set_termios(fd, speed, stop_bits, data_bits, parity, flow_ctl);

	/* Maxim registers should be configured to select parameters
	 * like UART/I2C, Base mode/Bypassmode etc . Since by default
	 * it is configured for UART Base mode it is left as it is.
	 */
 
	return fd;
}
