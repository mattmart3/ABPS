//
//  testlib.h
//  
//
//  Created by Gabriele Di Bernardo on 13/05/15.
//
//

#ifndef ____testlib__
#define ____testlib__

#include <stdio.h>

#include "sendrecvUDP.h"
#include "structs.h"


int get_test_identifier(void);

void sent_packet_with_packet_and_test_identifier(uint32_t packet_identifier, int test_identifier);

void check_and_log_local_error_notify_with_test_identifier(ErrMsg *error_message, int test_identifier, int hopV);


/* Set current time at the supplied testlib structure. It's not thread-safe. */
void current_time_with_supplied_time(testlib_time *time_lib);


#endif /* defined(____testlib__) */
