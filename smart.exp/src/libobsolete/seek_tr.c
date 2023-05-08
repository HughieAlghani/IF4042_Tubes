#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libobsolete/seek_tr.c,v 11.0 1992/07/21 18:22:55 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include "common.h"
#include "param.h"
#include "rel_header.h"
#include "tr.h"
#include "io.h"

extern _STR_FILES _Stop_ranks[];

int
seek_tr (tr_index, tr)
int tr_index;
register TR *tr;
{
    register TR *tr_current;
    register _STR_FILES *tr_ptr = &(_Stop_ranks[tr_index]);
    TR *tr_end, *tr_beginning;
    
    if (tr == NULL) {
        /* Seek to beginning of file */
        tr_ptr->current_tr = tr_ptr->beginning_tr;
        tr_ptr->last_seek = 1;
        return (1);
    }
    
    if (tr->qid == UNDEF) {
        /* Insufficient information */
        return (UNDEF);
    }

    tr_current  = tr_ptr->current_tr;
    tr_end      = tr_ptr->end_tr;
    tr_beginning= tr_ptr->beginning_tr;

    if (tr_current >= tr_end) {
        tr_current = tr_end - 1;
    }
    while (tr_current >= tr_beginning && tr_current->qid >= tr->qid) {
        tr_current--;
    }
    tr_current++;
    while (tr_current < tr_end && tr_current->qid < tr->qid) {
        tr_current++;
    }
    if (tr_current < tr_end && tr_current->qid == tr->qid) {
        /* Have matched qid now must worry about did */
        if (tr->did == UNDEF) {
            tr_ptr->current_tr = tr_current;
            tr_ptr->last_seek = 1;
            return (1);
        }
        /* else position to first tuple with >= did */
        while (tr_current < tr_end && 
               tr_current->qid == tr->qid && 
               tr_current->did < tr->did) {
            tr_current++;
        }
        if ( tr_current < tr_end && 
             tr_current->qid == tr->qid && 
             tr_current->did == tr->did) {
            tr_ptr->current_tr = tr_current;
            tr_ptr->last_seek = 1;
            return (1);
        }
    }

    tr_ptr->current_tr = tr_current;
    tr_ptr->last_seek = 0;
    return (0);
}
