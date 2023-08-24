#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <sys/stat.h>
#include <sys/epoll.h>

#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <fcntl.h>
#include <dirent.h>

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <assert.h>

#include <libevdev/libevdev.h>

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
	int ch;
	while ((ch = getopt(argc, argv, "")) != EOF)
	{
		switch (ch)
		{
			default:
				break;
		}
	}

	FILE *fp;
	if ((fp = fopen(cfg, "r")) == NULL)
		err(1, "fopen");

	parse_rules(fp);
	if (fclose(fp) == -1)
		err(1, "fclose");

	int efd;
	if ((efd = epoll_create(32)) == -1)
		err(1, "epoll_create");

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
	size_t fdi = 0;
	DIR *dir;
	struct dirent *dp;
	int dfd;
	if ((dir = opendir("/dev/input")) == NULL)
		err(1, "opendir");
	/* EINVAL will never happen, but ENOTSUP might (on some libc?) */
	if ((dfd = dirfd(dir)) == -1)
		err(1, "dirfd");
	for (int i = 0; (dp = readdir(dir)) != NULL && i < 32; )
	{
		struct stat sb;
		if (fstatat(dfd, dp->d_name, &sb, 0) == -1)
			err(1, "stat %s", dp->d_name);
		if (S_ISDIR(sb.st_mode))
			continue;
		if (strncmp(dp->d_name, "event", strlen("event")) != 0)
			continue;
		int fd;
		if ((fd = openat(dfd, dp->d_name, O_RDONLY | O_NONBLOCK)) == -1)
			err(1, "open %s", dp->d_name);
		if (key_valid(fd))
			fds[fdi] = fd;
		else
		{
			if (close(fd) == -1)
				err(1, "close");
			continue;
		}

		struct epoll_event event_hints;
		memset(&event_hints, 0, sizeof(struct epoll_event));
		event_hints.events = EPOLLIN;
		event_hints.data.fd = fd;

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
}

static void epoll_loop(int efd, int debug_mode)
{
	struct epoll_event events[12];

	int events_read;
	while ((events_read = epoll_wait(efd, events, 12, -1)) != -1)
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
				if (system(name) == -1)
					err(1, "system(%s) failed", name);
			}
		}
	}
}
