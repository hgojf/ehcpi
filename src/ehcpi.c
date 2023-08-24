#include <stdio.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <err.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <signal.h>
#include <assert.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

#include "input.h"

static void populate_poll(int, int [32]);
static void epoll_loop(int, int);
void sigint_handler(int);

int main(int argc, char **argv)
{
	if (daemon(0, 0) == -1)
		err(1, "daemon");
	int debug_mode = 0;
	char *cfg = "/etc/ehcpi";
	{
	int ch;
	while ( (ch = getopt(argc, argv, "")) != EOF)
	{
		switch (ch)
		{
			default:
				break;
		}
	}
	}

	FILE *fp = fopen(cfg, "r");
	if (!fp)
		err(1, "fopen");

	parse_rules(fp);
	if (fclose(fp) == -1)
		err(1, "fclose");

	int efd = epoll_create(32);
	if (efd == -1)
		err(1, "epoll_create");

	//im not sure about the limits imposed on epoll (number of fds, etc)
	int fds[32];

	populate_poll(efd, fds);

	(void) signal(SIGINT, sigint_handler);
	(void) signal(SIGTERM, sigint_handler);
	epoll_loop(efd, debug_mode);

	free_rules();

	for (int i = 0; i < 32 && fds[i] != -1; i++)
	{
		if (close(fds[i]) == -1)
			err(1, "close");
	}

	if (close(efd) == -1)
		err(1, "close");
}

static void populate_poll(int efd, int fds[32])
{
	//maybe implement something like in acpid where you just check every input device
       //and if they have an event you need add them
	int fdi = 0;
	DIR *dir;
	struct dirent *dp;
	int dfd;
	if ((dir = opendir("/dev/input")) == NULL)
		err(1, "opendir");
	dfd = dirfd(dir);
	for (int i = 0; (dp = readdir(dir)) != NULL && i < 32; )
	{
		struct stat sb;
		if (fstatat(dfd, dp->d_name, &sb, 0) == -1)
			err(1, "stat %s", dp->d_name);
		if (S_ISDIR(sb.st_mode))
			continue;
		if (strncmp(dp->d_name, "event", strlen("event")) != 0)
			continue;
		int fd = openat(dfd, dp->d_name, O_RDONLY | O_NONBLOCK);
		if (fd == -1)
			err(1, "open %s", dp->d_name);
		fds[fdi] = fd;
		struct epoll_event event_hints = {
			.events = EPOLLIN,
			.data.fd = fd,
		};
		if (epoll_ctl(efd, EPOLL_CTL_ADD, fd, &event_hints) == -1)
			err(1, "epollctl");
		++i;
		++fdi;
	}
	closedir(dir);
	fds[fdi] = -1;
}

void sigint_handler(int signum)
{
	assert (signum == SIGINT);
}

static void epoll_loop(int efd, int debug_mode)
{
	//too small? too big?
	struct epoll_event events[12];

	//we only do this now because we want epoll_wait to return -1 on a SIGINT,
	//rather than the program dying immediately

	int events_read;
	while ( (events_read = epoll_wait(efd, events, 12, -1)) != -1)
	{
		struct input_event ev;
		for (int i = 0; i < events_read; i++)
		{
			if (read(events[i].data.fd, &ev, sizeof(ev)) != sizeof(ev))
				err(1, "read");
			else
			{
				const char *name = input_string(ev);
				if (!name)
					continue;
				//maybe we should just fork and exec ourselves 
				//so that we can close stdout and stderr
				//(by redirecting them to /dev/null)
				if (system(name) == -1)
					err(1, "system(%s) failed", name);
			}
		}
	}
}
