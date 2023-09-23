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

struct input_cat {
	size_t len;
	struct evtab_entry ***codes; //i.e *[][]
};

static struct input_cat cats[EV_MAX];
static struct input_cat keys;
static struct input_cat switches;

struct evtab_entry ***
make_code(size_t max)
{
	struct evtab_entry ***ret;
	if ((ret = reallocarray(NULL, max, sizeof(struct evtab_entry ***))) == NULL)
		err(1, "ha");
	for (size_t i = 0; i < max; i++)
	{
		if ((ret[i] = calloc(2, sizeof(struct evtab_entry **))) == NULL)
			err(1, "baha");
		/* We actually need to calloc here. */
	}
	return ret;
}

void 
free_codes(struct evtab_entry ***code, size_t len)
{
	for (size_t i = 0; i < len; i++)
	{
		free(code[i]);
	}
	free(code);
}

void 
evtab2_init(void)
{
	keys.codes = make_code(KEY_MAX);
	keys.len = KEY_MAX;
	switches.codes = make_code(SW_MAX);
	switches.len = SW_MAX;
	cats[EV_KEY] = keys;
	cats[EV_SW] = switches;
}

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
	struct evtab_entry *r;
	switch (ev.type)
	{
		case EV_KEY:
		case EV_SW:
			break;
		default:
			return NULL;
	}
	if (ev.code > cats[ev.type].len)
		return NULL;
	if ((r = cats[ev.type].codes[ev.code][ev.value]) != NULL)
		return r->cmd;
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
	free_codes(keys.codes, keys.len);
	free_codes(switches.codes, switches.len);
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
		cats[evtab[i].type].codes[evtab[i].code][evtab[i].value] = &evtab[i];
		return 0;
	}
	return -1;
}

void
parse_rules(FILE *stream)
{
	assert(stream != NULL);

	evtab2_init();
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
