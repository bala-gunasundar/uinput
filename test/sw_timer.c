#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>

#define SIG SIGRTMIN
#define CLOCKID CLOCK_REALTIME

#define err_exit(msg)    do { perror(msg); exit(EXIT_FAILURE); } while (0)


static void handler(int sig, siginfo_t *si, void *uc)
{
	printf("Handler function\n");
}

void timer_init(void)
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
	timer_init();
	while (1);
}

