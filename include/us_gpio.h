#ifndef __US_GPIO_H__
#define __US_GPIO_H__

#include <stdint.h>

int us_gpio_export(uint16_t gpio);
int us_gpio_unexport(uint16_t gpio);
int us_gpio_direction_input(uint16_t gpio);
int us_gpio_direction_output(uint16_t gpio, int value);
int us_gpio_set_value(uint16_t gpio, int value);
int us_gpio_get_value(uint16_t gpio);

#endif
