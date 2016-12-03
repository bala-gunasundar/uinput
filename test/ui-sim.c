#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <stdint.h>

int main()
{
	int i;
	int fd;
	int ret;
	uint8_t *bufp;
	ssize_t wlen;
	uint8_t len;
	struct uinput_user_dev uidev;
	struct input_event ev[6];

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


	/* Injecting events */
	while (1) {
		memset(ev, 0, sizeof(ev));

		ev[0].type = EV_ABS;
		ev[0].code = ABS_MT_TRACKING_ID;
		ev[0].value = 0;

		ev[1].type = EV_ABS;
		ev[1].code = ABS_MT_TOUCH_MAJOR;
		ev[1].value = 255;

		ev[2].type = EV_ABS;
		ev[2].code = ABS_MT_POSITION_X;
		ev[2].value = 17920;

		ev[3].type = EV_ABS;
		ev[3].code = ABS_MT_POSITION_Y;
		ev[3].value = 16048;

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

		sleep(1);
	}

	return 0;

ioctl_err:
	puts("ioctl err");
	return -1;
}
