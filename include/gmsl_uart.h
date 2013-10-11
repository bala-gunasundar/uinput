#ifndef __GMSL_UART_H__
#define __GMSL_UART_H__

#pragma pack(1)
#include <stdint.h>

#define GMSL_UART                    "/dev/ttymxc0"
#define GMSL_SYNC_BYTE               0x79
#define ADDR_SERIALIZER              0x80
#define ADDR_DESERIALIZER            0x90
#define ADDR_SER_UC                  0x02
#define ADDR_DESER_UC                0x04
#define ADDR_REG_DEFAULT             0x00
#define ACK_SERDES                   0xC3
#define GMSL_UART_READ               0x01
#define GMSL_UART_WRITE              0x00

#define LENGTH_GMSL_ACK              1

#define J36_TO_STD_TOUCH_ID          2
#define J36_CROSS_SEC_AREA_PRESS     1
#define J36_CROSS_SEC_AREA_RELEASE   0

#define SIZE_SW_VERSION_INFO         24
#define SIZE_CMU_REQUEST             0x06
#define SIZE_CHKSUM                  1
#define SIZE_SERDES_ACK              1
#define J36_RESP_SIZE_MAX            100
#define GMSL_NOT_APP                 0
#define SIZE_GMSL_HEADER             4
#define DEFAULT_BRIGHTNESS           60	/* percentage */

#define J36_Y_LSB_SHIFT              4
#define J36_SHIFT_MSB                2 
#define J36_TOUCH_DATA_LEN           7

enum j36_mask_vals {
	J36_MASK_STATUS_DETECT = 0x80,
	J36_MASK_X_LSB         = 0x03, 
	J36_MASK_Y_LSB         = 0x30,
};

enum touch_response {
	BYTE_MSGID, TOUCH_ID, TOUCH_STATUS, TOUCH_X, TOUCH_Y, TOUCH_XY_LOW, 
	TOUCH_AREA, TOUCH_AMP
};

enum finger_count {
	ONE_FINGER = 0x08,
	TWO_FINGER = 0x0F,
	THREE_FINGER = 0x16,
	FOUR_FINGER = 0x1D
};

enum finger_status {
	PRESS = 1,
	DETECT,
	RELEASE
};
enum cmu_request_cmds {
	CMD_GIVE_CURRENT_STATUS = 0x00,
	CMD_CHANGE_MODE,
	CMD_CHANGE_BRIGHTNESS,
	CMD_SEND_TOUCH_INFO,
	CMD_GIVE_TP_SW_VER,
	CMD_GIVE_DISP_SW_VER
};

enum cmd_sw_versions {
	CMD_TOUCH_PANEL_CONFIG_VER,
	CMD_DISPLAY_SW_VER
};

enum disp_brightness {
	DISP_BRIGHTNESS_MIN = 0x0000,
	DISP_BRIGHTNESS_MAX = 0x2710
};

enum mode_change_req {
	MODE_DIAG = 0x00,
	MODE_SLEEP,
	MODE_NORMAL,
	MODE_EXTEND,
	MODE_TEST
};

enum cmd_request_status {
	STATUS_NORMAL_W_O_HDCP,
	STATUS_NORMAL_W_HDCP,
	STATUS_ABNORMAL
};

enum disp_status {
	STATUS_DISP_NORMAL,
	STATUS_DISP_ACC_TOUT_SHUTDOWN
};

enum tft_status {
	STAT_TFT_NORMAL,
	STAT_TFT_BUSY,
	STAT_TFT_ABNORMAL
};

enum tft_vol_status {
	TFT_VOL_NORMAL,
	TFT_VOL_ABNORMAL
};

enum blight_status {
	STAT_BLIGHT_NORMAL,
	STAT_BLIGHT_UNDER_LUMINOSITY,
	STAT_BLIGHT_ABNORMAL,
};

enum blight_vol_status {
	STAT_BLIGHT_VOL_NORMAL,
	STAT_BLIGHT_VOL_ABNORMAL
};

enum touch_status {
	TOUCH_STATUS_NORMAL,
	TOUCH_STATUS_BUSY,
	TOUCH_STATUS_ABNORMAL
};

enum touch_vol_status {
	TOUCH_VOL_STATUS_NORMAL,
	TOUCH_VOL_STATUS_ABNORMAL,
};

enum gmsl_msgids {
	MSGID_CMU_REQUEST = 0x01,
	MSGID_DISP_STATUS,
	MSGID_TOUCH_POS,
	MSGID_DIAG_MODE_STATUS,
	MSGID_DISP_SW_VERSION
};

enum screen_type {
	SCRN_TYPE_NONE,
	SCRN_TYPE_HORIZON_LINES,
	SCRN_TYPE_VERTICAL_LINES,
	SCRN_TYPE_CHECKED,
	SCRN_TYPE_PLAIN
};

enum screen_color {
	SCRN_CLR_NONE, SCRN_CLR_RED, SCRN_CLR_GREEN, SCRN_CLR_BLUE,
	    SCRN_CLR_WHITE, SCRN_CLR_BLACK
};

enum current_touch_cursor {
	CURSOR_NONE, CURSOR_CROSS_HAIR
};

struct gmsl_header {
	uint8_t sync;
	uint8_t dev_addr;
	uint8_t reg_addr;
	uint8_t no_bytes;
};

struct j36_events {
	uint8_t touch_id;
	uint8_t status;
	uint16_t x;
	uint16_t y;
};

struct cmu_request {		/* 11 bytes */
	struct gmsl_header header;
	uint8_t msgid;
	uint8_t status;
	uint8_t cmd;
	uint8_t mode_change_req;
	uint16_t brightness;
	uint8_t chksum;
};

#endif
