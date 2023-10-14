#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/queue.h>
#include <sys/wait.h>
#include <sys/signalfd.h>

#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <fcntl.h>
#include <dirent.h>

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <assert.h>

#include "input.h"
#include "pathnames.h"

struct entry
{
	int fd;
	SLIST_ENTRY(entry) entries;
};

SLIST_HEAD(listhead, entry) head;

static void populate_poll(int, struct listhead *);
static void epoll_loop(int);
void free_slist(struct listhead *);

static bool debug_mode = false;

int 
main(int argc, char **argv)
{
	bool daemonize = false;
	int ch;
	FILE *cfg;
	int efd;
	while ((ch = getopt(argc, argv, "dt")) != EOF)
	{
		switch (ch)
		{
			case 'd':
				daemonize = true;
				break;
			case 't':
				debug_mode = true;
				break;
			default:
				break;
		}
	}

	if (daemonize)
	{
		if (daemon(0, 0) == -1)
			err(1, "daemon");
	}

	if ((cfg = fopen(CFG_PATH, "r")) == NULL)
		err(1, "fopen");

	parse_rules(cfg);
	if (fclose(cfg) == -1)
		err(1, "fclose");

	if ((efd = epoll_create(32)) == -1)
		err(1, "epoll_create");

	SLIST_INIT(&head);

	populate_poll(efd, &head);

	epoll_loop(efd);

	free_rules();

	free_slist(&head);

	if (close(efd) == -1)
		err(1, "close");
}

void
free_slist(struct listhead *head)
{
	struct entry *i, *i2;
	i = SLIST_FIRST(head);
	while (i != NULL)
	{
		if (close(i->fd) == -1)
			err(1, "close");
		i2 = SLIST_NEXT(i, entries);
		free(i);
		i = i2;
	}
}

static void 
populate_poll(int efd, struct listhead *head)
{
	assert(head != NULL);

	DIR *dir;
	struct dirent *dp;
	int dfd;
	if ((dir = opendir("/dev/input")) == NULL)
		err(1, "opendir");
	/* EINVAL will never happen, but ENOTSUP might (on some libc?) */
	if ((dfd = dirfd(dir)) == -1)
		err(1, "dirfd");
	while ((dp = readdir(dir)) != NULL)
	{
		struct stat sb;
		struct epoll_event event_hints;
		int fd;
		if (fstatat(dfd, dp->d_name, &sb, 0) == -1)
			err(1, "stat %s", dp->d_name);
		if (S_ISDIR(sb.st_mode))
			continue;
		if (strncmp(dp->d_name, "event", strlen("event")) != 0)
			continue;
		if ((fd = openat(dfd, dp->d_name, O_RDONLY | O_NONBLOCK)) == -1)
			err(1, "open %s", dp->d_name);
		if (ev_needed(fd))
		{
			struct entry *e;
			if ((e = malloc(sizeof(struct entry))) == NULL)
				err(1, "malloc");
			e->fd = fd;
			SLIST_INSERT_HEAD(head, e, entries);
		}
		else
		{
			if (close(fd) == -1)
				err(1, "close");
			continue;
		}

		bzero(&event_hints, sizeof(struct epoll_event));
		event_hints.events = EPOLLIN;
		event_hints.data.fd = fd;

		if (epoll_ctl(efd, EPOLL_CTL_ADD, fd, &event_hints) == -1)
			err(1, "epollctl");
	}
	if (closedir(dir) == -1)
		err(1, "closedir");
}

static void 
epoll_loop(int efd)
{
	struct epoll_event events[12];
	int sigfd;
	int events_read;
	struct epoll_event sigfd_event;
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGTERM);
	if ((sigfd = signalfd(-1, &set, 0)) == -1)
		return;
	bzero(&sigfd_event, sizeof(struct epoll_event));
	sigfd_event.events = EPOLLIN;
	sigfd_event.data.fd = sigfd;
	if (epoll_ctl(efd, EPOLL_CTL_ADD, sigfd, &sigfd_event) == -1)
		return;
	if (sigprocmask(SIG_BLOCK, &set, NULL) == -1)
		return;

	while ((events_read = epoll_wait(efd, events, 12, -1)) != -1)
	{
		struct input_event ev;
		for (int i = 0; i < events_read; i++)
		{
			const char *name;
			if (events[i].data.fd == sigfd)
				goto done;
			if (read(events[i].data.fd, &ev, sizeof(ev)) != sizeof(ev))
				err(1, "read");
			if (debug_mode)
			{
				if (fprintf(stderr, "type:%u code:%u value:%u\n", ev.type, ev.code, ev.value) == -1)
					err(1, "fprintf");
				continue;
			}
			if ((name = input_string(&ev)) == NULL)
				continue;
			if (system(name) == -1)
				err(1, "system(%s) failed", name);
		}
	}
	done:
	close(sigfd);
}
