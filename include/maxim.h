#ifdef __MAXIM_H__
#define __MAXIM_H__

#include <stdint.h>

int maxim_init(uint8_t *gmsl_uart, uint32_t speed, uint8_t stop_bits,
	       uint8_t data_bits, uint8_t parity, uint8_t flow_ctl);
int maxim_write(int fd, uint8_t slave_add, uint8_t reg_add, int len, uint8_t *buf);
int maxim_read(int fd, uint8_t slave_add, uint8_t reg_add, int len, uint8_t *buf);

#endif
