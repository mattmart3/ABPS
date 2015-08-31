//
//  
//
//  Created by Gabriele Di Bernardo on 13/05/15.
//
//

#ifndef TEDERROR_H
#define TEDERROR_H

#include <stdio.h>

#include "structs.h"

int tederror_recv_nowait(struct err_msg_s **em);
int tederror_check_ted_info(struct err_msg_s *emsg, struct ted_info_s *ted_info);

#endif 
