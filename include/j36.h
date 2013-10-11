#ifndef __J36_H__
#define __J36_H__

#include <stdint.h>
#include "gmsl_uart.h"
#include <linux/input.h>
#include <linux/uinput.h>
#include <stdbool.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>

#define J36_BAUD                   230400
#define J36_MAX_READ_TIMES         30
#define BRIGHTNESS_VAL             100	/* value to increment 1% */
#define BRIGHTNESS_NULL            0
#define BRIGHTNESS_DEFAULT         25	/* Percentage 0 - 100 */
#define J36_MAX_RESP_SIZE          50

#define J36_NON_CALIB_VAL_X        (kanzi_calib / max_x) 
#define J36_NON_CALIB_VAL_Y        (kanzi_calib / max_y) 

#define J36_TOUCH_COORD_MIN_X      0
#define J36_TOUCH_COORD_MIN_Y      0
#define J36_MT_TOUCH_MAJOR         255
#define J36_MT_WIDTH_MAJOR         255
#define J36_MAX_FINGERS            4
#define J36_MT_TRACKING_ID         (J36_MAX_FINGERS - 1)

static long kanzi_calib = 32767;
static long max_x = 800;
static long max_y = 480;

int j36_report_touch_event(uint8_t fingers);
void j36_process_touch_data(uint8_t * dat, uint8_t fingers);
int j36_gmsl_validate_header(struct gmsl_header *header);
int j36_read_disp_status(struct gmsl_header *header, uint8_t *buf);
int j36_write_cmu_request(uint8_t status, uint8_t cmd,
			  uint8_t mode_change_req, uint16_t brightness);
int j36_cmd_set_brightness(uint8_t brightness);
bool is_scr_touched(uint8_t * resp);
int j36_uinput_init(void);
void j36_gmsl_flush_channel(void);

#endif
