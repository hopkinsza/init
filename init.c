/*
 * We are always in one of two states: SINGLEUSER or MULTIUSER.
 *   SINGLEUSER -> an emergency shell /bin/sh is running.
 *   MULTIUSER  -> /etc/rc is running. This is the default unless
 *                 argv[1] == "-s".
 * Either way, we keep track of the PID of that process in `cpid'.
 *
 * If the `cpid' process exits, switch state to the opposite mode.
 *
 * Only respond to the following signals:
 *   SIGALRM || SIGCHLD -> wait() on any children. A SIGALRM is scheduled to be
 *                         sent every 30 seconds, to make absolutely certain
 *                         that every child process is wait()ed on.
 *   SIGINT  || SIGTERM -> If we are in MULTIUSER state, send a SIGTERM to every
 *                         process in the system every 0.5 seconds for 10
 *                         seconds. (If any processes remain alive after this,
 *                         print a warning.) Then switch to SINGLEUSER state.
 */

#include <sys/ioctl.h>
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "debug.h"

enum states {
	SINGLEUSER,
	MULTIUSER
} state;

int  fork_rc();
int  fork_shell();
int  wait_for_upto(int);
void sleep_err(int, char *, ...);
void start_timer();
void swap_state(int *, enum states *);

const struct timespec one_sec = {
	1,
	0
};

/* signal block mask -- full */
sigset_t bmask;

int
main(int argc, char *argv[])
{
	int cons_fd;
	int sig;
	int status;
	pid_t cpid;
	pid_t pid;

	if (getpid() != 1)
		return 1;
	if (getuid() != 0)
		return 1;

	/*
	 * Call setsid().
	 * Make /dev/console our ctty.
	 * Make sure STD{IN,OUT,ERR} use it.
	 */

	setsid();

	cons_fd = open("/dev/console", O_RDWR);
	if (cons_fd == -1)
		sleep_err(1, "open /dev/console failed");
	if (ioctl(cons_fd, TIOCSCTTY) == -1)
		sleep_err(1, "ioctl to make /dev/console my ctty failed");

	dup2(cons_fd, 0);
	dup2(cons_fd, 1);
	dup2(cons_fd, 2);

	close(cons_fd);


	sigfillset(&bmask);
	sigprocmask(SIG_BLOCK, &bmask, NULL);

	/* Set initial state. */
	state = SINGLEUSER;
	if (argc == 2 && (strncmp(argv[1], "-s", 2)) == 0) {
		debug("got -s\n");
		state = MULTIUSER;
	}
	swap_state(&cpid, &state);


	start_timer();

	while (sigwait(&bmask, &sig) == 0)
	switch (sig) {
	case SIGALRM:
	case SIGCHLD:
		if (DEBUG) {
			if (sig == SIGALRM)
				debug("got alrm\n");
			if (sig == SIGCHLD)
				debug("got chld\n");
		}

		while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
			if (pid == cpid) {
				debug("cpid exited, running swap_state()\n");
				swap_state(&cpid, &state);
			}
		}
		break;
	case SIGINT:
	case SIGTERM:
		debug("got int or term\n");
		if (state != MULTIUSER)
			break;

		warnx("waiting up to 15 seconds for processes to exit...");
		kill(-1, SIGTERM);
		kill(-1, SIGHUP);
		kill(-1, SIGCONT);
		if (wait_for_upto(10) != 0) {
			kill(-1, SIGKILL);
			if (wait_for_upto(5) != 0) {
				warnx("some processes would not die; ps axl advised");
			}
		}
		swap_state(&cpid, &state);

		break;
	}
}

int
fork_rc()
{
	int pid;
	if ((pid = fork()) == 0) {
		sigprocmask(SIG_UNBLOCK, &bmask, NULL);
		execl("/etc/rc", "rc", NULL);
	}
	return pid;
}

int
fork_shell()
{
	int pid;
	if ((pid = fork()) == 0) {
		sigprocmask(SIG_UNBLOCK, &bmask, NULL);
		execl("/bin/sh", "sh", NULL);
	}
	return pid;
}

int
wait_for_upto(int secs)
{
	int pid;
	int status;

	for (int i=0; i<secs; i++) {
		while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
			;

		if (pid == -1) {
			/* All children successfully waited for. */
			return 0;
		}

		nanosleep(&one_sec, NULL);
	}

	/* Unwaited children remain. */
	return 1;
}

/* Print error, sleep for a few seconds, then exit. */
void
sleep_err(int status, char *fmt, ...)
{
	va_list ap;

	fprintf(stderr, "\n");

	va_start(ap, fmt);
	vwarn(fmt, ap);
	va_end(ap);

	for (int i=0; i<6; i++) {
		nanosleep(&one_sec, NULL);
	}

	exit(status);
}

void
start_timer()
{
	timer_t tid;
	struct itimerspec its;

	timer_create(CLOCK_MONOTONIC, NULL, &tid);

	its = (struct itimerspec){
		{ 30, 0 },
		{ 30, 0 }
	};
	timer_settime(tid, TIMER_ABSTIME, &its, NULL);
}

void
swap_state(int *cpid, enum states *state)
{
	if (*state == MULTIUSER) {
		warnx("entering single-user mode");
		*state = SINGLEUSER;
		*cpid = fork_shell();
	} else if (*state == SINGLEUSER) {
		warnx("entering multi-user mode");
		*state = MULTIUSER;
		*cpid = fork_rc();
	}

	if (*cpid == -1)
		sleep_err(1, "fork(2) failed");
}
