#ifndef UTILS_H
#define UTILS_H

/* Configuration data */
struct conf_s {
	int ip_vers;
	FILE *logfile;
};

extern struct conf_s conf;

#endif
