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

#include <sys/wait.h>

#include <err.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
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
void swap_state(int *, enum states *);
void start_timer();

const struct timespec half_sec = {
	0,
	500000000
};

/* signal block mask -- full */
sigset_t bmask;

int
main(int argc, char *argv[])
{
	int n;
	int sig;
	int status;
	pid_t cpid;
	pid_t pid;

	if (DEBUG) {
		if (getpid() != 1)
			return 1;
		if (getuid() != 0)
			return 1;
	}

	sigfillset(&bmask);
	sigprocmask(SIG_BLOCK, &bmask, NULL);

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
		if (sig == SIGALRM)
			debug("got alrm\n");
		if (sig == SIGCHLD)
			debug("got chld\n");

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
		if (state == MULTIUSER) {
			debug("changing to singleuser\n");
			for (n=0; n<20; n++) {
				debug(".");
				fflush(stdout);
				if (!DEBUG) {
					if (kill(-1, SIGTERM) == -1)
						break;
				}
				nanosleep(&half_sec, NULL);
			}
			if (n == 20) {
				debug("\n");
				warnx("some processes would not die; ps axl advised");
			}
			swap_state(&cpid, &state);
		}
		break;
	}
}

int
fork_rc()
{
	int pid;
	if ((pid = fork()) == 0) {
		sigprocmask(SIG_UNBLOCK, &bmask, NULL);
		execl("/home/frey/bin/rc", "rc", NULL);
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

void
swap_state(int *cpid, enum states *state)
{
	if (*state == MULTIUSER) {
		debug("starting singleuser\n");
		*state = SINGLEUSER;
		*cpid = fork_shell();
	} else if (*state == SINGLEUSER) {
		debug("starting multiuser\n");
		*state = MULTIUSER;
		*cpid = fork_rc();
	}
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
