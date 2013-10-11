#include "j36.h"

static struct j36_events events[J36_MAX_FINGERS];
extern int fd;

int j36_report_touch_event(uint8_t fingers)
{
	int i;
	uint16_t x;
	uint16_t y;
	uint8_t status = 0;
	struct input_event ev[6];
	uint8_t *bufp;
	uint8_t len;
	ssize_t wlen;
	
	for (i = 0; i < fingers; i++) {

		if (events[i].status & J36_MASK_STATUS_DETECT) {
			status = PRESS;

		} else if ((events[i].status & J36_MASK_STATUS_DETECT) == 0) {
			status = RELEASE;
			
		} else {
			continue;
		}

		x = events[i].x;
		y = events[i].y;

		memset(ev, 0, sizeof(ev));

		if (status == PRESS) {
			ev[0].type = EV_ABS; 
			ev[0].code = ABS_MT_TRACKING_ID;
			ev[0].value = events[i].touch_id - J36_TO_STD_TOUCH_ID;
			
			ev[1].type = EV_ABS;
			ev[1].code = ABS_MT_TOUCH_MAJOR;
			ev[1].value = J36_CROSS_SEC_AREA_PRESS;
			
			ev[2].type = EV_ABS;
			ev[2].code = ABS_MT_POSITION_X;
			ev[2].value = (x * J36_NON_CALIB_VAL_X);
			
			ev[3].type = EV_ABS;
			ev[3].code = ABS_MT_POSITION_Y;
			ev[3].value = (max_y - y) * J36_NON_CALIB_VAL_Y;
			
			ev[4].type = EV_SYN;
			ev[4].code = SYN_MT_REPORT;
			
			ev[5].type = EV_SYN;
			ev[5].code= SYN_REPORT;
			
			bufp = (uint8_t *) ev;
			len = sizeof(ev);

			do {
				wlen = write(fd, bufp, sizeof(ev));
				if (wlen < 0)
					error(1, errno, "Error while inj events");
				bufp += wlen;
				len -= wlen;
				
			} while (len != 0);
				
		} else if (status == RELEASE) {
			/* Yet to fix since J36 not supporting release events */
		}
	}

	return 0;
}

void j36_process_touch_data(uint8_t * dat, uint8_t fingers)
{
	int i, j = 0;

	for (i = 0; i < fingers; i++) {

		events[i].touch_id = dat[j + TOUCH_ID];
		events[i].status = dat[j + TOUCH_STATUS];
		events[i].x =
		    ((dat[j + TOUCH_XY_LOW] & J36_MASK_X_LSB) + (dat[j + TOUCH_X] << J36_SHIFT_MSB));
		events[i].y =
		    (((dat[j + TOUCH_XY_LOW] & J36_MASK_Y_LSB) >> J36_Y_LSB_SHIFT) +
		     (dat[j + TOUCH_Y] << J36_SHIFT_MSB));

		j += J36_TOUCH_DATA_LEN;
	}

	j36_report_touch_event(fingers);
}

int j36_gmsl_validate_header(struct gmsl_header *header)
{
	if (header->sync == GMSL_SYNC_BYTE) {
		if (header->dev_addr == ADDR_SER_UC) {
			if (header->reg_addr == ADDR_REG_DEFAULT) {
				return 0;
			}
		}
	}

	return -1;
}

int j36_read_disp_status(struct gmsl_header *header, uint8_t *buf)
{
	int ret;
	int rlen;
	uint8_t len = 0;

	ret =
	    read_packet_in_loop((uint8_t *) header, sizeof(struct gmsl_header),
				J36_MAX_READ_TIMES);
	if (ret == -1) {
		error (0, 0, "Error while reading GMSL Header");
		goto status_fail;
	}

	len = header->no_bytes + SIZE_CHKSUM;
	
	if (j36_gmsl_validate_header(header) != -1) {
		
		rlen = read_packet_in_loop(buf, len, J36_MAX_READ_TIMES);
		if (rlen == len)  
			return rlen;
		else if (rlen == -1) 
			error(0, 0, "Error while reading disp status\n");
	} else {
		puts("Invalid GMSL Header");
	}

status_fail:
	j36_gmsl_flush_channel();
	return -1;
}

int j36_write_cmu_request(uint8_t status, uint8_t cmd,
				 uint8_t mode_change_req, uint16_t brightness)
{
	int ret;
	struct cmu_request request;

	memset(&request, 0, sizeof(request));

	request.header.sync = GMSL_SYNC_BYTE;
	request.header.dev_addr = ADDR_DESER_UC;
	request.header.reg_addr = ADDR_REG_DEFAULT;
	request.header.no_bytes =
	    (sizeof(struct cmu_request) - sizeof(struct gmsl_header)) -
	    SIZE_CHKSUM;
	request.msgid = MSGID_CMU_REQUEST;
	request.status = status;
	request.cmd = cmd;
	request.mode_change_req = mode_change_req;
	request.brightness = brightness;
	request.chksum = calculate_chksum((uint8_t *) & request,
					  (sizeof(struct cmu_request) -
					   SIZE_CHKSUM), 0);
	
	ret = write_packet((uint8_t *) & request, sizeof(struct cmu_request));
	if (ret == -1) {
		error(0, 0, "Writing cmu request failed");
		return -1;
	}

	return 0;
}


int j36_cmd_set_brightness(uint8_t brightness)
{
	int ret;
	int rlen;
	struct gmsl_header head;
	uint8_t buf[J36_RESP_SIZE_MAX];

	ret =
	    j36_write_cmu_request(STATUS_NORMAL_W_O_HDCP, CMD_CHANGE_BRIGHTNESS,
				  MODE_NORMAL, (brightness * BRIGHTNESS_VAL));
	if (ret == -1) {
		error(0, 0, "Error while writing `change brightness` cmd");
		return -1;
	}
	
	usleep(500);

	rlen = j36_read_disp_status(&head, buf);
	if (rlen == -1) {
		error(0, 0, "Error while reading `brightness cmd` response");
		return -1;
	}
	
	return 0;
}

bool is_scr_touched(uint8_t * resp)
{
	if ((resp[BYTE_MSGID] == MSGID_TOUCH_POS) && (resp[TOUCH_ID] != 0))

		return true;
	else
		return false;
}

int j36_uinput_init(void) 
{
	int ret;
	struct uinput_user_dev uidev;

	fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	if (fd < 0) 
		error(1, errno, "Open err");
	
	ret = ioctl(fd, UI_SET_EVBIT, EV_ABS);
	if (ret < 0)
		goto ioctl_err;

	ret = ioctl(fd, UI_SET_EVBIT, EV_SYN);
	if (ret < 0)
		goto ioctl_err;

	
	ret = ioctl(fd, UI_SET_ABSBIT, ABS_MT_POSITION_X);
	if (ret < 0)
		goto ioctl_err;

	ret = ioctl(fd, UI_SET_ABSBIT, ABS_MT_POSITION_Y);
	if (ret < 0)
		goto ioctl_err;

	ret = ioctl(fd, UI_SET_ABSBIT, ABS_MT_TOUCH_MAJOR);	
	if (ret < 0)
		goto ioctl_err;
		

	ret = ioctl(fd, UI_SET_ABSBIT, ABS_MT_WIDTH_MAJOR);
	if (ret < 0)
		goto ioctl_err;

	ret = ioctl(fd, UI_SET_ABSBIT, ABS_MT_TRACKING_ID);
	if (ret < 0)
		goto ioctl_err;


	memset(&uidev, 0, sizeof(uidev));
	snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "j36-test");
	uidev.id.bustype = BUS_USB;
	uidev.id.vendor  = 0x1234;
	uidev.id.product = 0xfedc;
	uidev.id.version = 1;

	
	uidev.absmin[ABS_MT_POSITION_X] = 0;
	uidev.absmax[ABS_MT_POSITION_X] = 800;

	uidev.absmin[ABS_MT_TOUCH_MAJOR] = 0; 
	uidev.absmax[ABS_MT_TOUCH_MAJOR] = 255;
	
	uidev.absmin[ABS_MT_WIDTH_MAJOR] = 0; 
	uidev.absmax[ABS_MT_WIDTH_MAJOR] = 255;
	
	uidev.absmin[ABS_MT_POSITION_Y] = 0;
	uidev.absmax[ABS_MT_POSITION_Y] = 480;
	
	uidev.absmin[ABS_MT_TRACKING_ID] = 0;
	uidev.absmax[ABS_MT_TRACKING_ID] = 3;

	
	ret = write(fd, &uidev, sizeof(uidev));
	if (ret < 0)
		goto ioctl_err;

	ret = ioctl(fd, UI_DEV_CREATE);
	if (ret < 0)
		goto ioctl_err;

	return 0;

ioctl_err:
	puts("j36 init err");
	return -1;
}

void j36_gmsl_flush_channel(void) 
{
	tcflush(fd, TCIOFLUSH);
}


