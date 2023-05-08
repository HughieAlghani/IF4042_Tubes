#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libobsolete/read_rr.c,v 11.0 1992/07/21 18:22:51 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "rel_header.h"
#include "rr.h"
#include "io.h"

extern _SRR_FILES _Srel_ranks[];


int
read_rr (rr_index, rr)
int rr_index;
register RR * rr;
{
    register RR * rr_entry;
    register _SRR_FILES *rr_ptr = &(_Srel_ranks[rr_index]);

    rr_entry = rr_ptr->current_rr;
    
    if (rr_entry == NULL || rr_entry >= rr_ptr->end_rr) {
        return (0);
    }

    bcopy ((char *) rr_entry, (char *) rr, sizeof (RR));

    rr_ptr->current_rr++;
    rr_ptr->last_seek = 0;
    return (1);
}

