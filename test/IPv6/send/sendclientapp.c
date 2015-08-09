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

#include "libsend.h"
#include "testlib.h"
#include "utils.h"
#include "structs.h"


int main(int argc, char **argv)
{

	char *usage, *address;
	int port;

	asprintf(&usage, "Usage: %s [OPTIONS] ADDRESS PORT\n\n"
	         "OPTIONS:\n"
	         "    -h, --help            print this help\n"
	         "    -6, --ipv6            use IPv6 (IPv4 default)\n"
	         "    -p, --path=logfile    specify a log file path\n"
		 "    -t, --type=TEST_TYPE  declare the type of the "
		                           "running test (default 0)\n",
	         argv[0]);
	
	/* Initialize configuration */
	utils_default_conf();

	if (utils_get_opt(argc, argv) == -1 || argc - optind < 2) {
		utils_exit_error(usage);
	} else {
		address = argv[optind++];
		port = atoi(argv[optind++]);
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
	if (conf.ip_vers == 6) 
		instantiate_ipv6_shared_instance(address, port);
	else 
		instantiate_ipv4_shared_instance(address, port);

	for(index = 0; index < conf.n_packets; index++)
	{
		int test_identifier = get_test_identifier();

		char *content;

		content = (char *)malloc(conf.msg_length * sizeof(char));
		memset(content, 'c', conf.msg_length - 1);
		content[conf.msg_length - 1]='\0';

		json_object *object = json_object_new_object();

		json_object *test_identifier_content = json_object_new_int(test_identifier);
		json_object *message_content = json_object_new_string(content);

		json_object_object_add(object,"testIdentifier", test_identifier_content);
		json_object_object_add(object, "messageContent", message_content);

		const char *buffer = json_object_to_json_string(object);

		uint32_t identifier;
		printf("\nlunghezza: %lu e id:%d  \n", strlen(buffer), test_identifier );	

		send_packet_with_message(buffer, strlen(buffer), &identifier);

		printf("Sent packet with identifier %d.\n", identifier);

		/* Log packet just sent. */
		sent_packet_with_packet_and_test_identifier(identifier, test_identifier);


		ErrMsg *error_message = alloc_init_ErrMsg();
		if(receive_local_error_notify_with_error_message(error_message))
		{
			/* Application does not use local notification info. */
			check_and_log_local_error_notify_with_test_identifier(error_message, test_identifier, hopV);
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
