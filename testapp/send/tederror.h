//
//  
//
//  Created by Gabriele Di Bernardo on 13/05/15.
//
//

#ifndef TEDERROR_H
#define TEDERROR_H

#include <stdio.h>

#include "libsend.h"
#include "structs.h"

int tederror_recv_wait(ErrMsg *em);
int tederror_check_ted_info(ErrMsg *emsg, struct ted_info_s *ted_info);

#endif 
