//
//  
//
//  Created by Gabriele Di Bernardo on 13/05/15.
//
//

#ifndef TEDERROR_H
#define TEDERROR_H

#include <stdio.h>

#include "network.h"
#include "structs.h"

int tederror_recv_nowait(ErrMsg **em);
int tederror_check_ted_info(ErrMsg *emsg, struct ted_info_s *ted_info);

#endif 
