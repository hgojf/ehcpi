#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <sys/wait.h>
#include <event.h>

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
	struct event ev;
	SLIST_ENTRY(entry) entries;
};

SLIST_HEAD(listhead, entry) head;

static void populate_poll(struct listhead *);
void free_slist(struct listhead *);
void event_handler(int, short, void *);

static bool debug_mode = false;

int 
main(int argc, char **argv)
{
	bool daemonize = false;
	struct event_base *ep;
	struct event sigint, sigterm;
	int ch;
	FILE *cfg;
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

	if ((ep = event_init()) == NULL)
		err(1, "event_init");
	signal_set(&sigint, SIGINT, event_handler, NULL);
	signal_add(&sigint, NULL);
	signal_set(&sigterm, SIGTERM, event_handler, NULL);
	signal_add(&sigterm, NULL);

	SLIST_INIT(&head);

	populate_poll(&head);

	event_loop(0);

	event_base_free(ep);
	free_rules();
	free_slist(&head);
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

void
event_handler(int fd, short type, void *data)
{
	struct input_event ev;
	const char *name;
	if (type == EV_SIGNAL)
	{
		event_loopexit(NULL);
		return;
	}
	if (read(fd, &ev, sizeof(ev)) != sizeof(ev))
		err(1, "read");
	if (debug_mode)
	{
		if (fprintf(stderr, "type:%u code:%u value:%u\n", ev.type, ev.code, ev.value) == -1)
			err(1, "fprintf");
		return;
	}
	if ((name = input_string(&ev)) == NULL)
		return;
	if (system(name) == -1)
		err(1, "system(%s) failed", name);
}

static void 
populate_poll(struct listhead *head)
{
	DIR *dir;
	struct dirent *dp;
	int dfd;
	assert(head != NULL);
	if ((dir = opendir("/dev/input")) == NULL)
		err(1, "opendir");
	/* EINVAL will never happen, but ENOTSUP might (on some libc?) */
	if ((dfd = dirfd(dir)) == -1)
		err(1, "dirfd");
	while ((dp = readdir(dir)) != NULL)
	{
		struct stat sb;
		struct entry *e;
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

		event_set(&e->ev, e->fd, EV_READ | EV_PERSIST, event_handler, NULL);
		event_add(&e->ev, NULL);

	}
	if (closedir(dir) == -1)
		err(1, "closedir");
}
