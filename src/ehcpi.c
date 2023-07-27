#include <stdio.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <err.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <stdbool.h>
#include <sys/timerfd.h>
#include <signal.h>
#include <assert.h>
#include <stdlib.h>

#include "input.h"

static void sigint_handler(int signal, siginfo_t *siginfo, void *data);
static void setup_signals(void);

int main(int argc, char **argv)
{
	int debug_mode = 0;
	{
	int ch;
	while ( (ch = getopt(argc, argv, "d")) != EOF)
	{
		switch (ch)
		{
			case 'd':
				debug_mode = 1;
				break;
			default:
				break;
		}
	}
	}

	parse_rules(stdin);

	int efd = epoll_create(1);
	if (efd == -1)
		err(1, "epoll_create");

	//im not sure about the limits imposed on epoll (number of fds, etc)
	int fds[32];

	{

	//maybe implement something like in acpid where you just check every input device
	//and if they have an event you need add them
	
	int fdi = 0;
	for (int i = optind; i < argc && i < 32; i++, fdi++)
	{
		int fd = open(argv[i], O_RDONLY | O_NONBLOCK);
		if (fd == -1)
			err(1, "open");
		fds[fdi] = fd;
		struct epoll_event event_hints = {
			.events = EPOLLIN,
			.data.fd = fd,
		};
		if (epoll_ctl(efd, EPOLL_CTL_ADD, fd, &event_hints) == -1)
			err(1, "epollctl");
	}
	fds[fdi] = -1;

	}

	//too small? too big?
	struct epoll_event events[12];

	{

	//we only do this now because we want epoll_wait to return -1 on a SIGINT,
	//rather than the program dying immediately
	setup_signals();

	int events_read;
	while ( (events_read = epoll_wait(efd, events, 12, -1)) != -1)
	{
		struct input_event ev;
		for (int i = 0; i < events_read; i++)
		{
			if (read(events[i].data.fd, &ev, sizeof(ev)) != sizeof(ev))
				err(1, "read");
			if (debug_mode)
			{
				if (fprintf(stderr, "%u %u %u\n", ev.type, ev.code, ev.value) == -1)
					err(1, "printf");
			}
			else
			{
				const char *name = input_string(ev);
				if (!name)
					continue;
				//maybe we should just fork and exec ourselves 
				//so that we can close stdout and stderr
				//(by redirecting them to /dev/null)
				if (system(name) != 0)
					err(1, "system(%s) failed", name);
			}
		}
	}
	}

	free_rules();

	for (int i = 0; i < 32 && fds[i] != -1; i++)
	{
		if (close(fds[i]) == -1)
			err(1, "close");
	}

	if (close(efd) == -1)
		err(1, "close");
}

static void setup_signals(void)
{
	struct sigaction sigint = {
		.sa_sigaction = sigint_handler,
	};
	
	if (sigaction(SIGINT, &sigint, NULL) == -1)
		err(1, "sigaction");

}

static void sigint_handler(int signal, siginfo_t *siginfo, void *data)
{
	assert(signal == SIGINT);
}
