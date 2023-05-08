#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libobsolete/seek_rr.c,v 11.0 1992/07/21 18:22:52 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include "common.h"
#include "param.h"
#include "rel_header.h"
#include "smart_error.h"
#include "rr.h"
#include "io.h"

extern _SRR_FILES _Srel_ranks[];

int
seek_rr (rr_index, rr)
int rr_index;
register RR *rr;
{
    register RR *rr_current;
    register _SRR_FILES *rr_ptr = &(_Srel_ranks[rr_index]);
    RR *rr_end, *rr_beginning;
    
    if (rr_ptr->opened == 0) {
        set_error (SM_ILLPA_ERR, "rr_fd not opened", "seek_rr");
        return (UNDEF);
    }

    if (rr == NULL) {
        /* Seek to beginning of file */
        rr_ptr->current_rr = rr_ptr->beginning_rr;
        rr_ptr->last_seek = 1;
        return (1);
    }
    
    if (rr->qid == UNDEF) {
        /* Insufficient information */
        return (UNDEF);
    }

    rr_current  = rr_ptr->current_rr;
    rr_end      = rr_ptr->end_rr;
    rr_beginning= rr_ptr->beginning_rr;

    if (rr_current >= rr_end) {
        rr_current = rr_end - 1;
    }
    while (rr_current >= rr_beginning && rr_current->qid >= rr->qid) {
        rr_current--;
    }
    rr_current++;
    while (rr_current < rr_end && rr_current->qid < rr->qid) {
        rr_current++;
    }
    if (rr_current < rr_end && rr_current->qid == rr->qid) {
        /* Have matched qid now must worry about did */
        if (rr->did == UNDEF) {
            rr_ptr->current_rr = rr_current;
            rr_ptr->last_seek = 1;
            return (1);
        }
        /* else position to first tuple with >= did */
        while (rr_current < rr_end && 
               rr_current->qid == rr->qid && 
               rr_current->did < rr->did) {
            rr_current++;
        }
        if ( rr_current < rr_end && 
             rr_current->qid == rr->qid && 
             rr_current->did == rr->did) {
            rr_ptr->current_rr = rr_current;
            rr_ptr->last_seek = 1;
            return (1);
        }
    }

    rr_ptr->current_rr = rr_current;
    rr_ptr->last_seek = 0;
    return (0);
}
