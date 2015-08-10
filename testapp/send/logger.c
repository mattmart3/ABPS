
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <json-c/json.h>
#include <sys/time.h>
#include <time.h>

#include "libsend.h"
#include "utils.h"
#include "structs.h"

static struct loglist_s *head = NULL;

static int logid = 0;

static int hopV = 0;


void __add_in_loglist(struct loglist_s *log_elem)
{
	if(head == NULL) {
		head = log_elem;
	} else {
		struct loglist_s *temp = head;
		while (temp->next != NULL)
			temp = temp->next;

		temp->next = log_elem; 
	}
	return;
}


struct loglist_s * __search_in_loglist(int logid)
{
	struct loglist_s *return_value;

	if(head->logid == logid) {
		return_value = head;
		head = head->next;
		return return_value;
	}

	struct loglist_s *temp = head;

	while (temp->next != NULL) {
		if ((temp->next)->logid == logid) {
			return_value = temp->next;
			temp->next = (temp->next)->next;
 
			return_value->next = NULL;
			return return_value;
		}
		temp = temp->next;

	}

	return NULL;
}


void __set_current_time(struct logtime_s *time)
{
	struct timeval current_time;
	gettimeofday(&current_time, NULL);

	time->human_readable_time_and_date = asctime(gmtime(&current_time.tv_sec));

	time->milliseconds_time = ((current_time.tv_sec)*1000000L+current_time.tv_usec)/1000;
}

int logger_get_logid(void)
{
	return ++logid;
}

int logger_init_json(void) 
{
	int len;

	if (!conf.logfile)
		return -1;

	len = 0;
	fseek(conf.logfile, 0, SEEK_END);
	len = (int)ftell(conf.logfile);

	/* control size of file to initialize json format. */
	if (len == 0) { 
		fseek(conf.logfile, 0, SEEK_SET  );
		fputs("{ \"pacchetti\": [ \n \n ]}", conf.logfile);
		hopV = 1;
	}

	return 0;
}

/* Store the just sent packet in the loglist in order to actually log it when
 * TED information for that packet will be received */
void logger_mark_sent_pkt(uint32_t pkt_id, int logid)
{
	struct logtime_s current_time;

	__set_current_time(&current_time);

	struct loglist_s *ptr = (struct loglist_s *)malloc(sizeof(struct loglist_s));
	
	ptr->next = NULL;
	ptr->logtime = current_time;
	ptr->logid = logid;

	__add_in_loglist(ptr);
	
}


int log_notification(struct ted_info_s *ted_info, int logid)
{
	long int time_diff;
	json_object *logobj;
	struct logtime_s current_time;
	struct loglist_s *ptr;
	time_t actual_time;
	char *c_time_string, *str;

	if (!conf.logfile)
		return -1;


	__set_current_time(&current_time);

	ptr = __search_in_loglist(logid);

	time_diff = current_time.milliseconds_time - ptr->logtime.milliseconds_time;
	
	logobj = json_object_new_object();
	json_object *timeJ = json_object_new_int(time_diff);
	json_object *typeJ = json_object_new_int(conf.test_type);
	json_object *testIdJ = json_object_new_int(logid);

	json_object *retrycountJ = json_object_new_int(ted_info->retry_count);
	json_object *ackJ = json_object_new_boolean(ted_info->status);

	json_object *versionJ;

	if (ted_info->ip_vers == IPPROTO_IPV6)
		versionJ = json_object_new_string("ipv6");
	else
		versionJ = json_object_new_string("ipv4");
	
	/* Obtain current time as seconds elapsed since the Epoch. */
	actual_time = time(NULL); 
	/* Convert to local time format. */
	c_time_string = ctime(&actual_time); 
	json_object *dateJ = json_object_new_string(c_time_string);

	json_object_object_add(logobj, "testId", testIdJ);
	json_object_object_add(logobj, "type", typeJ);
	json_object_object_add(logobj, "ack", ackJ);
	json_object_object_add(logobj, "time", timeJ);
	json_object_object_add(logobj, "retrycount", retrycountJ);
	json_object_object_add(logobj, "ipVersion", versionJ);
	json_object_object_add(logobj, "date", dateJ);

	if (hopV == 1) {
		asprintf(&str, "%s\n]}", json_object_to_json_string(logobj));
		fseek(conf.logfile, -4, SEEK_END);
		hopV = 0;
	} else {

		asprintf(&str, ",\n%s\n]}", json_object_to_json_string(logobj));
		fseek(conf.logfile, -3, SEEK_END);
	}

	fputs(str, conf.logfile);
	free(str);
	free(logobj);
	free(ptr);
	
	return 0;
}
