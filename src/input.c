#include <linux/input-event-codes.h>
#include <linux/input.h>

#include <err.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

struct evtab_entry {
	unsigned type;
	unsigned code;
	unsigned value;
	const char *str;
	char *cmd;
};

static struct evtab_entry evtab[] = {
	 { EV_KEY, KEY_POWER, 1, "button/power", NULL },

	 { EV_KEY, KEY_BRIGHTNESSDOWN, 1, "button/brightnessdown", NULL },
	 { EV_KEY, KEY_BRIGHTNESSUP, 1, "button/brightnessup", NULL },

	 { EV_KEY, KEY_VOLUMEDOWN, 1, "button/volumedown", NULL },
	 { EV_KEY, KEY_VOLUMEUP, 1, "button/volumeup", NULL },
	 { EV_KEY, KEY_MUTE, 1, "button/mute", NULL },

	 { EV_SW,  SW_LID,  0, "lid-open", NULL },
	 { EV_SW,  SW_LID,  1, "button/lid LID close", NULL },

	 { EV_KEY, KEY_CONFIG, 1, "button/config", NULL },
};

void evtab_init(void)
{
	
}

#define EV_VREP 2

#define EVTAB_LEN (sizeof(evtab) / sizeof(*evtab))

bool ev_needed(int fd)
{
	unsigned long evbit = 0;
	for (size_t i = 0; i < EVTAB_LEN; i++)
	{
		if (ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), &evbit) == -1)
			err(1, "ioctl EVIOCGBIT");
		if (!(evbit & (1 << evtab[i].type)))
			continue;
		if (evtab[i].cmd == NULL)
			continue;
		size_t nchar = KEY_MAX/8 + 1;
		unsigned char bits[nchar];
		if (ioctl(fd, EVIOCGBIT(evtab[i].type, sizeof(bits)), &bits) == -1)
			err(1, "ioctl");
		int key = evtab[i].code;
		if (bits[key/8] & (1 << (key % 8)))
			return true;
	}
	return false;
}

const char *input_string(const struct input_event ev)
{
	for (int i = 0; i < EVTAB_LEN; i++)
	{
		if (evtab[i].type != ev.type || evtab[i].code != ev.code)
			continue;
		if (ev.type == EV_KEY)
		{
			if (ev.value != EV_VREP && evtab[i].value != ev.value)
				continue;
		}
		else if (evtab[i].value != ev.value)
			continue;
		return evtab[i].cmd;
	}
	return NULL;
}

static int add_event(const char *s, FILE *stream)
{
	for (int i = 0; i < EVTAB_LEN; i++)
	{
		if (!strncmp(s, evtab[i].str, strlen(evtab[i].str)))
		{
			if (evtab[i].cmd != NULL)
				errx(1, "rule for %s already exists", evtab[i].str); 
			/* Maybe we should just free it */
			char buf[4192];
			if (fgets(buf, 4192, stream) == NULL)
				err(1, "fgets");
			if (strncmp(buf, "action=", strlen("action=")) != 0)
				return -1;
			char *roll = buf + strlen("action=");
			evtab[i].cmd = strdup(roll);
			if (!evtab[i].cmd)
				err(1, "stdup failed");
			return 0;
		}
	}
	return -1;
}

void parse_rules(FILE *stream)
{
	char buf[4192];
	while (fgets(buf, 4192, stream) != NULL)
	{
		if (!strcmp(buf, "\n"))
			continue;
		if (!strncmp(buf, "event=", strlen("event=")))
		{
			if (add_event(buf + strlen("event="), stream) != 0)
				warn("couldnt parse rule %s", buf);
		}
	}
}

void free_rules(void)
{
	for (int i = 0; i< EVTAB_LEN; i++)
	{
		if (evtab[i].cmd != NULL)
			free(evtab[i].cmd);
	}
}
