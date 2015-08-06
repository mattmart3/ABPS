//
//  sendclientapp.c
//  
//
//  Created by Gabriele Di Bernardo on 10/05/15.
//
//

#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <json-c/json.h>
#include <getopt.h>

#include "libsend.h"
#include "testlib.h"
#include "utils.h"

#define NUMBER_OF_PACKETS 5000

#define MESSAGE_LENGTH 1050 /*mtu*/

void exit_error(char *emsg)
{
	fprintf(stderr, "%s", emsg);
	exit(EXIT_FAILURE);
}

FILE *open_logfile(char *path) {
	FILE *f;

	f = fopen(path, "r+");

	if (f == NULL && errno == ENOENT) 
		f = fopen(path, "w+");

	return f;

}

int get_opt(int argc, char **argv) {
	
	for (;;) {

		int option_index = 0;
		int c;

		static struct option long_options[] = {
			{ "help", no_argument, 0, 'h' },
			{ "ipv6", no_argument, 0, '6'},
			{ "path", required_argument, 0, 'p' },
			{ 0, 0, 0, 0 },
		};

		c = getopt_long(argc, argv, "h6p:",
				long_options, &option_index);
		if (c == -1)
			break;
		switch(c) {
			case '6':
				conf.ip_vers = 6;
				break;
			case 'p':
				conf.logfile = open_logfile(optarg);
	
				if (conf.logfile == NULL) 
					fprintf(stderr, "Can't open log file");

				break;
			case 'h':
			default:
				return -1;
		}
	}
	return 0;
}

int main(int argc, char **argv)
{

	char *usage, *address;
	int port, type;

	asprintf(&usage, "Usage: %s [OPTIONS] ADDRESS PORT TYPE\n\n"
	         "OPTIONS:\n"
	         "    -h, --help            print this help\n"
	         "    -6, --ipv6            use IPv6 (IPv4 default)\n"
	         "    -p, --path=logfile    specify a log file path\n",
	         argv[0]);
	
	/* Default configuration */
	conf.ip_vers = 4;
	conf.logfile = NULL;

	if (get_opt(argc, argv) == -1 || argc - optind < 3) {
		exit_error(usage);
	} else {
		address = argv[optind++];
		port = atoi(argv[optind++]);
		type = atoi(argv[optind]);  /* type of connection, ex: with/without hosts, indoor/outdoor */
	}

	int hopV = 0;  /* for json file. If the file is empty, hop comma in the first row */
	int len=0; /* length of file */

	int index;
	/* control size of file to iniziliaze json format. TODO: could make parsing */
	/* TODO: init logger (in the logger file) */
	if (conf.logfile) {
		fseek(conf.logfile, 0, SEEK_END);
		len = (int)ftell(conf.logfile);
		if (len==0)
		{ 
			fseek(conf.logfile, 0, SEEK_SET  );
			fputs("{ \"pacchetti\": [ \n \n ]}", conf.logfile);
			hopV=1;
		}
	}
	/* check for error */
	instantiate_ipv6_shared_instance_by_address_and_port(address, port);


	for(index = 0; index < NUMBER_OF_PACKETS; index++)
	{
		int test_identifier = get_test_identifier();
		char stringaInvio[MESSAGE_LENGTH];
		memset(stringaInvio,'c',MESSAGE_LENGTH-1);
		stringaInvio[MESSAGE_LENGTH-1]='\0';

		json_object *object = json_object_new_object();

		json_object *test_identifier_content = json_object_new_int(test_identifier);
		json_object *message_content = json_object_new_string(stringaInvio);

		json_object_object_add(object,"testIdentifier", test_identifier_content);
		json_object_object_add(object, "messageContent", message_content);

		const char *buffer = json_object_to_json_string(object);

		uint32_t identifier;
		printf("lunghezza: %lu e id:%d  \n", strlen(buffer), test_identifier );	

		send_packet_with_message(buffer, strlen(buffer), &identifier);

		printf("Sent packet with identifier %d.\n", identifier);

		/* Log packet just sent. */
		sent_packet_with_packet_and_test_identifier(identifier, test_identifier);


		ErrMsg *error_message = alloc_init_ErrMsg();

		if(receive_local_error_notify_with_error_message(error_message))
		{
			/* Application does not use local notification info. */
			check_and_log_local_error_notify_with_test_identifier(error_message, test_identifier, type, hopV);
		}

		if (hopV==1)
		{
			hopV=0;
		}        

		free(object);

		free(error_message);
	}

	release_shared_instance();

	return 0;
}
