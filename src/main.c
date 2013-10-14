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
#include "gmsl_uart.h"
#include <termios.h>
#include <stdbool.h>
#include "j36.h"
#include <signal.h>
#include <time.h>
#include "set_termios.h"

#pragma pack(1)

int fd;
int gmsl_fd;

#define SIG SIGRTMIN
#define CLOCKID CLOCK_REALTIME

static char *gmsl_uart = "/dev/ttymxc0";

#define err_exit(msg)    do { perror(msg); exit(EXIT_FAILURE); } while (0)

void putx(char *str, uint8_t * buf, uint8_t size)
{
	int i;
	printf("\n%s: ", str);

	for (i = 0; i < size; i++) {
		printf("0x%x ", buf[i]);
	}

	printf("\n");
}


uint8_t calculate_chksum(uint8_t * buf, int size, uint8_t base)
{
	int i;

	for (i = 0; i < size; i++) {
		base += buf[i];
	}

	return base;
}

int write_packet(uint8_t *buf, uint8_t wsize)
{
	ssize_t wlen;
	uint8_t len;
	uint8_t *bufp;

	bufp = buf;
	len = wsize;

	do {
		wlen = write(gmsl_fd, bufp, len);
		if (wlen == -1)
			error(1, errno, "Error while writing to GMSL-UART");
		bufp += wlen;
		len -= wlen;
	} while (len != 0);

	tcdrain(fd);

	return 0;
}

int read_packet_in_loop(uint8_t *buf, uint8_t rsize, uint8_t max_try)
{
	int i;
	ssize_t rlen;
	uint8_t *bufp;
	uint8_t len;
	uint8_t count;

	bufp = buf;
	len = rsize;
	count = 0;

	for (i = 0; i < max_try; i++) {
		rlen = read(gmsl_fd, bufp, len);
		if (rlen == -1) {
			error(0, errno, "Error while reading GMSL-UART");
			return -1;
		}		
		count += rlen;
		len -= rlen;
		if (len == 0)
			break;
		bufp += rlen;
	}
	
	if (i > max_try)
		error(0, 0, "J36: Read timeout");
	
	if (count == rsize)
		return count;
	else 
		return -1;
}

/* int simulate_touch_event(void) */
/* { */
/* 	struct input_event ev[6]; */
/* 	int i; */
/* 	int ret; */
/* 	uint8_t *bufp; */
/* 	ssize_t wlen; */
/* 	uint8_t len; */
	
/* 	memset(ev, 0, sizeof(ev)); */

/* 	ev[0].type = EV_ABS; */
/* 	ev[0].code = ABS_MT_TRACKING_ID; */
/* 	ev[0].value = 0; */
	
/* 	ev[1].type = EV_ABS; */
/* 	ev[1].code = ABS_MT_TOUCH_MAJOR; */
/* 	ev[1].value = 255; */
		
/* 	ev[2].type = EV_ABS; */
/* 	ev[2].code = ABS_MT_POSITION_X; */
/* 	ev[2].value = 17920; */
		
/* 	ev[3].type = EV_ABS; */
/* 	ev[3].code = ABS_MT_POSITION_Y; */
/* 	ev[3].value = 16048; */
		
/* 	ev[4].type = EV_SYN; */
/* 	ev[4].code = SYN_MT_REPORT; */
		
/* 	ev[5].type = EV_SYN; */
/* 	ev[5].code= SYN_REPORT; */

/* 	bufp = (uint8_t *) ev; */
/* 	len = sizeof(ev); */

/* 	do { */
/* 		wlen = write(fd, bufp, sizeof(ev)); */
/* 		if (wlen < 0) { */
/* 			error(0, errno, "Error while inj events"); */
/* 			return -1; */
/* 		} */
		
/* 		bufp += wlen; */
/* 		len -= wlen; */
			
/* 	} while (len != 0); */
	
/* 	return 0; */
/* } */

static void handler(int sig, siginfo_t *si, void *uc)
{
	int ret;
	int finger;
	int rlen;
	struct gmsl_header header;
	uint8_t response[J36_RESP_SIZE_MAX];

	ret = j36_write_cmu_request(STATUS_NORMAL_W_HDCP, CMD_SEND_TOUCH_INFO,
				    MODE_DIAG, BRIGHTNESS_NULL);
	if (ret == -1)
		goto read_fail;
		
	rlen = j36_read_disp_status(&header, response);
	if (rlen == -1) {
		printf("Error while reading touch response\n");
		goto read_fail;
	}
	
	if (is_scr_touched(response)) {
		switch (header.no_bytes) {
		case ONE_FINGER:
			finger = 1;
			break;
		case TWO_FINGER:
			finger = 2;
			break;
		case THREE_FINGER:
			finger = 3;
			break;
		case FOUR_FINGER:
			finger = 4;
			break;
		}
		
		j36_process_touch_data(response, finger);
		return;
	}
	
read_fail:
	j36_gmsl_flush_channel();
}

void j36_timer_init(void)
{
	timer_t timerid;
	struct sigevent sev;
	struct itimerspec its;
	sigset_t mask;
	struct sigaction sa;
	
	
	/* Establish handler for timer signal */

	printf("Establishing handler for signal %d\n", SIG);
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = handler;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIG, &sa, NULL) == -1)
		err_exit("sigaction");
	
	/* Block timer signal temporarily */
	
	sigemptyset(&mask);
	sigaddset(&mask, SIG);
	if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
		err_exit("sigprocmask");
	
	
	/* Create the timer */

	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = SIG;
	sev.sigev_value.sival_ptr = &timerid;
	if (timer_create(CLOCKID, &sev, &timerid) == -1)
		err_exit("timer_create");
	printf("timer ID is 0x%lx\n", (long) timerid);

	/* Start the timer */
	
	its.it_value.tv_sec = 0;
	its.it_value.tv_nsec = 20000000;
	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 20000000;
	
	if (timer_settime(timerid, 0, &its, NULL) == -1)
                err_exit("timer_settime");
	
	printf("Unblocking signal %d\n", SIG);
	if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
               err_exit("sigprocmask");
}

int main()
{
	int ret;
	int rlen;
	struct gmsl_header header;
	uint8_t response[J36_MAX_RESP_SIZE];
	int finger;
	int i;

	ret = j36_uinput_init();
	if (ret < 0) {
		puts("J36 Init err");
		return -1;
	} 
	puts("UINPUT registered");
		
	gmsl_fd = maxim_init(GMSL_UART, 230400, 1, 8, 'e', 0);
	if (gmsl_fd < 0)
		error(1, 0, "Maxim init err");

	sleep(1);
	
	ret = j36_cmd_set_brightness(25);
	if (ret < 0)
		error(1, 0, "Brightness set failed");
	
	j36_timer_init();
	
	while (1) 
		sleep(1);
	

	/* while (1) { */
		
	/* 	ret = j36_write_cmu_request(STATUS_NORMAL_W_HDCP, CMD_SEND_TOUCH_INFO, */
	/* 				    MODE_DIAG, BRIGHTNESS_NULL); */
	/* 	if (ret == -1) */
	/* 		goto read_fail; */
		
	/* 	rlen = j36_read_disp_status(&header, response); */
	/* 	if (rlen == -1) { */
	/* 		printf("Error while reading touch response\n"); */
	/* 		goto read_fail; */
	/* 	} */

	/* 	if (is_scr_touched(response)) { */
	/* 		switch (header.no_bytes) { */
	/* 		case ONE_FINGER: */
	/* 			finger = 1; */
	/* 			break; */
	/* 		case TWO_FINGER: */
	/* 			finger = 2; */
	/* 			break; */
	/* 		case THREE_FINGER: */
	/* 			finger = 3; */
	/* 			break; */
	/* 		case FOUR_FINGER: */
	/* 			finger = 4; */
	/* 			break; */
	/* 		} */
			
	/* 		j36_process_touch_data(response, finger); */
	/* 	} */

		
	/* 	usleep(12000); */
	/* 	continue; */
		
	/* read_fail: */
	/* 	j36_gmsl_flush_channel(); */
	/* 	continue; */
	/* } */
}
