#include <linux/input-event-codes.h>
#include <linux/input.h>

#include <err.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

struct evtab_entry {
	uint16_t type;
	uint16_t code;
	int32_t value;
	const char *str;
	char *cmd;
};

#define TAB_ENTRY(c, k, v, s) { c, k, v, s, NULL }

static struct evtab_entry evtab[] = {
	 TAB_ENTRY(EV_KEY, KEY_POWER, 1, "power"),
	 TAB_ENTRY(EV_KEY, KEY_BRIGHTNESSDOWN, 1, "brightnessdown"),
	 TAB_ENTRY(EV_KEY, KEY_BRIGHTNESSUP, 1, "brightnessup"),
	 TAB_ENTRY(EV_KEY, KEY_VOLUMEDOWN, 1, "volumedown"),
	 TAB_ENTRY(EV_KEY, KEY_VOLUMEUP, 1, "volumeup"),
	 TAB_ENTRY(EV_KEY, KEY_MUTE, 1, "mute"),
	 TAB_ENTRY(EV_SW, SW_LID, 0, "lid-open"),
	 TAB_ENTRY(EV_SW, SW_LID, 1, "lid-close"),
};

#define EV_VREP 2

#define EVTAB_LEN (sizeof(evtab) / sizeof(*evtab))

bool 
ev_needed(int fd)
{
	unsigned long evbit = 0;
	if (ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), &evbit) == -1)
		err(1, "ioctl EVIOCGBIT");
	for (size_t i = 0; i < EVTAB_LEN; i++)
	{
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

const char *
input_string(const struct input_event ev)
{
	/* Hey inneffecient */
	for (size_t i = 0; i < EVTAB_LEN; i++)
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

void 
free_rules(void)
{
	for (size_t i = 0; i < EVTAB_LEN; i++)
	{
		if (evtab[i].cmd != NULL)
			free(evtab[i].cmd);
	}
}

/* Maybe instead of instantly crashing if we cant find a matching event
 * we should simply output a warning that an unknown was passed
 */

int
add_rule(const char *event, const char *action)
{
	assert(event != NULL);
	assert(action != NULL);

	for (size_t i = 0; i < EVTAB_LEN; i++)
	{
		if (strcmp(evtab[i].str, event) != 0)
			continue;
		if ((evtab[i].cmd = strdup(action)) == NULL)
			err(1, "strdup");
		return 0;
	}
	return -1;
}

void
parse_rules(FILE *stream)
{
	assert(stream != NULL);

	char *line = NULL;
	size_t size;
	while (getline(&line, &size, stream) != -1)
	{
		if (!strcmp(line, "\n"))
			continue;
		char *roll = line;
		char *event;
		char *action;
		if (strncmp(line, "event ", strlen("event ")) != 0)
			errx(1, "failed to parse rule %s", line);
		roll += strlen("event ");
		event = roll;
		if ((action = strstr(event, " do ")) == NULL)
			errx(1, "failed to parse rule %s", line);
		*action = '\0';
		action += strlen(" do ");
		if (add_rule(event, action) != 0)
			err(1, "add_rule");
	}
	if (line != NULL)
		free(line);
}
