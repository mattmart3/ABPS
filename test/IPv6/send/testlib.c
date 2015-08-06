//
//  testlib.c
//  
//
//  Created by Gabriele Di Bernardo on 13/05/15.
//
//

#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <linux/errqueue.h>
#include <json-c/json.h>
#include <string.h>


#include "utils.h"
#include "testlib.h"

static char *log_path;

static int log_descriptor;

static int shared_test_identifier = 0;





typedef struct testlib_list
{
	int id_test;
	testlib_time time_list;

	struct testlib_list * next;

}testlib_list;


static testlib_list *head = NULL;


void add_in_list(testlib_list *list)
{
	if(head == NULL)
		head = list;
	else
	{
		testlib_list *temp=head;
		while(temp->next != NULL)
			temp=temp->next;

		temp->next = list; 
	}
	return;
}


testlib_list* search_in_list(int id)
{
	testlib_list *return_value;

	if(head->id_test == id)
	{
		return_value = head;
		head = head->next;
		return return_value;
	}

	testlib_list * temp = head;

	while(temp->next != NULL)	
	{
		if((temp->next)->id_test == id)
		{
			return_value = temp->next;
			temp->next = (temp->next)->next;

			return_value->next = NULL;
			return return_value;
		}
		temp=temp->next;

	}

	return NULL;
}



int get_test_identifier(void)
{
	return ++shared_test_identifier;
}


int prepare_for_logging(void)
{
	int return_value = lseek(log_descriptor, 0, SEEK_END);
	return return_value;
}



void ipv4_check_and_log_local_error_notify_with_test_identifier(ErrMsg *emsg, int test_id, int type, int hopV)
{

	char *end_string_file = "\n]}";
	for(emsg->c = CMSG_FIRSTHDR(emsg->msg); emsg->c; emsg->c = CMSG_NXTHDR(emsg->msg, emsg->c))
	{
		if((emsg->c->cmsg_level == IPPROTO_IP) && (emsg->c->cmsg_type == IP_RECVERR))
		{
			struct sockaddrin *from;

			emsg->ee = (struct sock_extended_err *) CMSG_DATA(emsg->c);

			from = (struct sockaddrin *) SO_EE_OFFENDER(emsg->ee);

			if((emsg->ee->ee_origin == SO_EE_ORIGIN_LOCAL_NOTIFY) && (emsg->ee->ee_errno == 0))
			{
				/* json for log */	
				json_object *logobj = json_object_new_object();

				uint32_t identifier = emsg->ee->ee_info;


				printf("Received notification for packet %" PRIu32 " \n", identifier);

				uint8_t acked = emsg->ee->ee_type;
				uint8_t retry_count = emsg->ee->ee_retry_count;
				//uint8_t more_frag = emsg->ee->ee_code;
				//uint16_t frag_len = (emsg->ee->ee_data >> 16);
				//uint16_t offset = ((emsg->ee->ee_data << 16) >> 16);

				if(acked)
				{
					printf("packet with identifier %" PRIu32 " is acked \n", identifier);
				}
				else
				{
					printf("packet with identifier %" PRIu32 " is not acked \n", identifier);
				}

				//int return_value;
				/* start time */
				time_t actual_time;
				char* c_time_string;


				actual_time = time(NULL); /* Obtain current time as seconds elapsed since the Epoch. */
				c_time_string = ctime(&actual_time); /* Convert to local time format. */
				/* TODO: could make control on time */

				/* end time */
				testlib_time current_time;

				current_time_with_supplied_time(&current_time);

				testlib_list *ptr = search_in_list(test_id);

				long int difference = current_time.milliseconds_time - ptr->time_list.milliseconds_time;                    


				/* start setting elements of json object, for log */
				json_object *testIdJ = json_object_new_int(test_id);
				json_object *retrycountJ = json_object_new_int(retry_count);
				json_object *timeJ = json_object_new_int(difference);
				json_object *typeJ = json_object_new_int(type);
				json_object *ackJ = json_object_new_boolean(acked);
				json_object *versionJ = json_object_new_string("ipv4");
				json_object *dateJ = json_object_new_string(c_time_string);
				/* end setting elements of json object, for log */



				/* start adding elements into the json */
				json_object_object_add(logobj,"testId", testIdJ);
				json_object_object_add(logobj, "type", typeJ);
				json_object_object_add(logobj,"ack", ackJ);
				json_object_object_add(logobj, "time", timeJ);
				json_object_object_add(logobj,"retrycount", retrycountJ);
				json_object_object_add(logobj,"ipVersion", versionJ);
				json_object_object_add(logobj, "date", dateJ); 
				if (hopV==1) /* check if comma is to insert or not */
				{
					char str[150]; //TODO: make it dinamic
					strcpy(str,json_object_to_json_string(logobj));
					strcat(str,end_string_file);
					strcat(str, "\0");
					if (conf.logfile) {
						fseek( conf.logfile, -4, SEEK_END  );
						fputs(str, conf.logfile);
					}

				}
				else
				{
					char str[150];
					strcpy(str, ",\n");
					strcat(str,json_object_to_json_string(logobj));
					strcat(str,end_string_file);
					strcat(str, "\0");
					if (conf.logfile) {
						fseek( conf.logfile, -3, SEEK_END  );
						fputs(str, conf.logfile);
					}

				}


				free(ptr);
				free(logobj);
			}
		}
	}

}

/* Get TED information of an error message */
void get_ted_info(ErrMsg *emsg, int test_id, int type, int hopV) 
{
	char *end_string_file = "\n]}";
	uint32_t identifier = ntohl(emsg->ee->ee_info);
	/* TODO: try with ted_message_identifier_from_notification uapi */
	uint8_t retry_count = emsg->ee->ee_retry_count;
	//uint8_t more_frag = emsg->ee->ee_code;
	//uint16_t frag_len = (emsg->ee->ee_data >> 16);
	//uint16_t offset = ((emsg->ee->ee_data << 16) >> 16);
	//
	/* TODO: check fragment_offset and more_fragment */

	printf("Received notification for packet %d \n", identifier);
	
	uint8_t acked = emsg->ee->ee_type;

	if (acked)
		printf("packet with identifier %d is acked \n", identifier);
	
	else
		printf("packet with identifier %d is not acked \n", identifier);

	json_object *logobj = json_object_new_object();
	testlib_time current_time;
	current_time_with_supplied_time(&current_time);

	testlib_list *ptr = search_in_list(test_id);

	long int difference = current_time.milliseconds_time - ptr->time_list.milliseconds_time;

	json_object *testIdJ = json_object_new_int(test_id);
	json_object *retrycountJ = json_object_new_int(retry_count);
	json_object *timeJ = json_object_new_int(difference);
	json_object *typeJ = json_object_new_int(type);
	json_object *ackJ = json_object_new_boolean(acked);

	json_object *versionJ = json_object_new_string("ipv6");
	//json_object *date = json_object_new_string("hello from client app!\n");

	json_object_object_add(logobj,"testId", testIdJ);
	json_object_object_add(logobj, "type", typeJ);
	json_object_object_add(logobj,"ack", ackJ);
	json_object_object_add(logobj, "time", timeJ);
	json_object_object_add(logobj,"retrycount", retrycountJ);
	json_object_object_add(logobj,"ipVersion", versionJ);

	if (hopV == 1) {
		char str[150];
		strcpy(str,json_object_to_json_string(logobj));
		strcat(str,end_string_file);
		strcat(str, "\0");
		if (conf.logfile) {
			fseek(conf.logfile, -4, SEEK_END  );
			fputs(str, conf.logfile);
		}

	} else {
		char str[150];
		strcpy(str, ",\n");
		strcat(str,json_object_to_json_string(logobj));
		strcat(str,end_string_file);
		strcat(str, "\0");
		if (conf.logfile) {
			fseek(conf.logfile, -3, SEEK_END);
			fputs(str, conf.logfile);
		}

	}
	free(logobj);
	free(ptr);
	
}

void ipv6_check_and_log_local_error_notify_with_test_identifier(ErrMsg *emsg, int test_id, int type, int hopV)
{

	/* For each error header of the previuosly sent message, 
	 * look for TED notifications. */
	for (emsg->c = CMSG_FIRSTHDR(emsg->msg); 
	     emsg->c; 
	     emsg->c = CMSG_NXTHDR(emsg->msg, emsg->c)) {
			
		if ((emsg->c->cmsg_level == IPPROTO_IPV6) && 
		    (emsg->c->cmsg_type == IPV6_RECVERR)) {
			
			struct sockaddrin_6 *from;

			emsg->ee = (struct sock_extended_err *)CMSG_DATA(emsg->c);

			from = (struct sockaddrin_6 *)SO_EE_OFFENDER(emsg->ee);

			switch (emsg->ee->ee_origin) {
				
				/* TED notification */
				case SO_EE_ORIGIN_LOCAL_NOTIFY: 
					if(emsg->ee->ee_errno == 0) {
						get_ted_info(emsg, test_id, type, hopV);
					} else {
						//TODO: handle errno for local notify
					}
					break;
				default:
					if(emsg->ee->ee_errno != 0)
						printf(strerror(emsg->ee->ee_errno));

					break;
			}
		}
	}
}



void check_and_log_local_error_notify_with_test_identifier(ErrMsg *emsg, int test_id,  int type, int hopV)
{
	if(emsg->is_ipv6)
	{
		ipv6_check_and_log_local_error_notify_with_test_identifier(emsg, test_id,type, hopV);
	}
	else
	{
		ipv4_check_and_log_local_error_notify_with_test_identifier(emsg, test_id, type, hopV);
	}
}


void sent_packet_with_packet_and_test_identifier(uint32_t packet_identifier, int test_id)
{
	testlib_time current_time;

	current_time_with_supplied_time(&current_time);

	testlib_list *ptr = (testlib_list *) malloc(sizeof(testlib_list));
	ptr->next = NULL;
	ptr->time_list = current_time;
	ptr->id_test = test_id;

	add_in_list(ptr);
	
}


void current_time_with_supplied_time(testlib_time *time_lib)
{
	struct timeval current_time;
	gettimeofday(&current_time, NULL);

	time_lib->human_readable_time_and_date = asctime(gmtime(&current_time.tv_sec));

	time_lib->milliseconds_time =((current_time.tv_sec)*1000000L+current_time.tv_usec)/1000;
}
