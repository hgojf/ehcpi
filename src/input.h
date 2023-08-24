#include <linux/input.h>

#include <stdbool.h>

const char *input_string(const struct input_event ev);
void parse_rules(FILE *stream);
void free_rules(void);
bool key_valid(int);
