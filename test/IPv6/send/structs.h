#ifndef STRUCTS_H
#define STRUCTS_H

/* Configuration data */
struct conf_s {
	int ip_vers;
	int n_packets;
	int msg_length;
	int test_type;
	FILE *logfile;
};

typedef struct testlib_time_s {
    long int milliseconds_time;    
    char *human_readable_time_and_date;
    
} testlib_time;

typedef struct testlib_list_s
{
	int id_test;
	testlib_time time_list;

	struct testlib_list_s * next;

} testlib_list;


#endif
