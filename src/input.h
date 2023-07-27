#include <linux/input.h>

const char *input_string(const struct input_event ev);
void parse_rules(FILE *stream);
void free_rules(void);
