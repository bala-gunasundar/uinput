#ifdef __SET_TERMIOS_H__
#define __SET_TERMIOS_H__

#include <stdint.h>

void uart_set_termios(int fd, uint32_t speed, uint8_t stop_bits,
		      uint8_t data_bits, uint8_t parity, uint8_t flowcontrol)

#endif
