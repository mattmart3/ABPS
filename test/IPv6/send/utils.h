#ifndef UTILS_H
#define UTILS_H

#include "structs.h"

extern struct conf_s conf;

void utils_exit_error(char *emsg);
void utils_default_conf(void);
int utils_get_opt(int argc, char **argv);


#endif
