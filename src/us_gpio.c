#include <fcntl.h>
#include <unistd.h>
#include <error.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "us_gpio.h"

#define FILE_GPIO_EXPORT           "/sys/class/gpio/export"
#define FILE_GPIO_UNEXPORT         "/sys/class/gpio/unexport"
#define PIN_MAX_SIZE               3
#define SIZE_NULL_CHAR             1
#define DELAY_SYSFS                50000 /* usecs */
#define MAX_ITOA                   80
#define SIZE_MAX_FILE              80

enum gpio_request {
	GPIO_EXPORT, GPIO_UNEXPORT
};

enum gpio_direction {
	DIR_OUTPUT, DIR_INPUT
};

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

int us_gpio_request(uint16_t gpio, uint8_t operation)
{
	int fd;
	int ret;
	ssize_t wlen;
	char file[SIZE_MAX_FILE];
	char pin[PIN_MAX_SIZE + SIZE_NULL_CHAR];
	
	usleep(DELAY_SYSFS);

	switch (operation) {

	case GPIO_EXPORT:
		strcpy(file, FILE_GPIO_EXPORT);
		break;

	case GPIO_UNEXPORT:
		strcpy(file, FILE_GPIO_UNEXPORT);
		break;

	default:
		puts("Unknown operation");
		return -1;
	}
	
	fd = open(file, O_WRONLY);
	if (fd < 0) {
		error(0, errno, "GPIO Request err: Err while opening");
		return -1;
	}
	
	snprintf(pin, MAX_ITOA, "%i", gpio);
	
	wlen = write_in_loop(fd, pin, strlen(pin) + SIZE_NULL_CHAR);
	if (wlen < 0) {
		error(0, errno, "Write err");
		return -1;
	}
	
	close(fd);
	
	return 0;
}

int us_gpio_export(uint16_t gpio)
{
	int ret;

	ret = us_gpio_request(gpio, GPIO_EXPORT);
	if (ret == -1) {
		printf("Error in exporting pin %d", gpio);
		return -1;
	}
	
	return 0;	
}

int us_gpio_unexport(uint16_t gpio)
{
	int ret;
	
	ret = us_gpio_request(gpio, GPIO_UNEXPORT);
	if (ret == -1) {
		printf("Error in unexporting pin %d", gpio);
		return -1;
	}
	
	return 0;	
}

int us_gpio_set_direction(uint16_t gpio, int dir)
{
	int fd;
	char file[SIZE_MAX_FILE];
	int ret;
	char gpio_dir[SIZE_MAX_FILE];

	usleep(DELAY_SYSFS);
	snprintf(file, MAX_ITOA, "/sys/class/gpio/gpio%d/direction", gpio);

	puts(file);
	fd = open(file, O_WRONLY);
	if (fd < 0) {
		error(0, errno, "Dir err");
		return -1;
	}
	
	switch (dir) {
	case DIR_OUTPUT:
		strcpy(gpio_dir, "out");
		break;

	case DIR_INPUT:
		strcpy(gpio_dir, "in");
		break;
		
	default:
		puts("Unknown second argument");
		return -1;
	}
			
	ret = write_in_loop(fd, gpio_dir, strlen(gpio_dir) + SIZE_NULL_CHAR);
	if (ret < 0)
		puts("Dir err");
	
	close(fd);
}

int us_gpio_set_value(uint16_t gpio, int value)
{
	char file[SIZE_MAX_FILE];
	int fd;
	char val[SIZE_MAX_FILE];
	ssize_t wlen;

	usleep(DELAY_SYSFS);
	snprintf(file, MAX_ITOA, "/sys/class/gpio/gpio%d/value", gpio);
	
	fd = open(file, O_WRONLY);
	if (fd < 0) {
		error(0, errno, "Open err");
		return -1;
	}

	snprintf(val, MAX_ITOA, "%d", value);

	wlen = write(fd, val, strlen(val) + SIZE_NULL_CHAR);
	if (wlen < 0) {
		error(0, errno, "Write err");
		return -1;
	}
}
int us_gpio_direction_input(uint16_t gpio)
{
	us_gpio_set_direction(gpio, DIR_INPUT);
}

int us_gpio_direction_output(uint16_t gpio, int value)
{
	us_gpio_set_direction(gpio, DIR_OUTPUT);
	us_gpio_set_value(gpio, value);
}

int us_gpio_get_value(uint16_t gpio)
{
	char file[SIZE_MAX_FILE];
	int fd;
	char val;
	ssize_t rlen;

	usleep(DELAY_SYSFS);
	snprintf(file, MAX_ITOA, "/sys/class/gpio/gpio%d/value", gpio);
	
	fd = open(file, O_RDONLY);
	if (fd < 0)
		error(0, errno, "Open err");
	
	rlen = read(fd, &val, 1);
	if (rlen < 0) {
		error(0, errno, "Read err");
		return -1;
	}

	close(fd);

	return atoi(&val);
}

