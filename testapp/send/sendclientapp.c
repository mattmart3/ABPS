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
#include <netinet/in.h>
#include <json-c/json.h>

#include "libsend.h"
#include "tederror.h"
#include "utils.h"
#include "structs.h"
#include "consts.h"
#include "logger.h"


#define USAGE "[OPTIONS] ADDRESS PORT\n\n\
OPTIONS:\n\
    -h, --help                Print this help.\n\
    -6, --ipv6                Use IPv6 (IPv4 default).\n\
                                  Interface argument MUST be specified (-i).\n\
    -i, --iface=NIC_NAME      Declare the network interface associated\n\
                                  to the specified ip address.\n\
    -n, --npkts=N_PKTS        Specify how many packets must be sent.\n\
    -s, --size=PKT_SIZE       Specify how big (bytes) a packet must be.\n\
    -p, --path=logfile        Specify a log file path.\n\
    -t, --type=TEST_TYPE      Declare the type of the \n\
                                  running test (default 0).\n"

const char * new_json_msg(int logid, char *msg_content)
{
	const char *str;
	json_object *object = json_object_new_object();

	json_object *test_identifier = json_object_new_int(logid);
	json_object *message_content = json_object_new_string(msg_content);

	json_object_object_add(object,"testIdentifier", test_identifier);
	json_object_object_add(object, "messageContent", message_content);

	str = json_object_to_json_string(object);
	free(object);

	return str;
}

int main(int argc, char **argv)
{

	char *usage, *address;
	int port;

	asprintf(&usage, "Usage: %s %s", basename(argv[0]), USAGE);
	
	/* Initialize configuration */
	utils_default_conf();

	/* Arguments validation */
	if (utils_get_opt(argc, argv) == -1 || argc - optind < 2) {

		utils_exit_error("Some required argument is missing.\n\n" 
		                 "%s", usage);
	} else if (conf.ip_vers == IPPROTO_IPV6 && conf.iface_name == NULL) { 

		utils_exit_error("Interface MUST be specified while using IPv6 "
		                 "(see '-i' option).\n\n%s", usage);
	} else {
		address = argv[optind++];
		port = atoi(argv[optind++]);
	}

	free(usage);

	int index;

	if (conf.ip_vers == IPPROTO_IPV6) 
		net_init_ipv6_shared_instance(conf.iface_name, address, port);
	else 
		net_init_ipv4_shared_instance(address, port);

	logger_init_json();

	for(index = 0; index < conf.n_packets; index++)
	{
		int logid; 
		uint32_t identifier;
		char *content;
		const char *buffer;

		logid = logger_get_logid();
		
		/* Init the message content space */
		content = (char *)malloc(conf.msg_length * sizeof(char));
		if (content == NULL) 
			utils_exit_error("%s: malloc failed", __func__);

		/* Fill the message content */
		memset(content, 'c', conf.msg_length - 1);
		content[conf.msg_length - 1] = '\0';
		
		/* Wrap the message in a json structure */
		buffer = new_json_msg(logid, content);

		printf("\nlength: %lu, logid: %d  \n", strlen(buffer), logid);	

		net_sendmsg(buffer, strlen(buffer), &identifier);

		printf("Sent packet with identifier %d.\n", identifier);

		/* Mark the packet just sent to be logged later. */
		logger_mark_sent_pkt(identifier, logid);


		ErrMsg *error_message = alloc_init_ErrMsg();
		if (tederror_recv_wait(error_message)) {
			struct ted_info_s ted_info;
			
			/* Application does not use local notification info. */
			tederror_check_ted_info(error_message, &ted_info);

			printf("Received notification for packet %d \n",
			       ted_info.msg_id);

			printf("msg_id: %d, retry_count: %d, acked: %d\n"
			       "more_frag: %d, frag_length: %d, frag_offset: %d\n",
			       ted_info.msg_id, ted_info.retry_count, ted_info.status,
			       ted_info.more_frag, ted_info.frag_length, ted_info.frag_offset);

			log_notification(&ted_info, logid);
		}

		free(content);
		free(error_message);
	}

	net_release_shared_instance();

	return 0;
}
