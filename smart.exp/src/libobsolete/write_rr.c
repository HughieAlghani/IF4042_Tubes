#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libobsolete/write_rr.c,v 11.0 1992/07/21 18:22:53 chrisb Exp $";
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
#include "smart_error.h"

extern _SRR_FILES _Srel_ranks[];

int
write_rr (rr_index, rr)
int rr_index;
RR * rr;
{
    register _SRR_FILES *rr_ptr = &(_Srel_ranks[rr_index]);
    register RR *rr_entry;
    register RR *temp_rr;

    if (rr->qid == UNDEF) {
        if (rr_ptr->last_seek == 1) {
            /* Must delete current_rr tuple */
            for ( rr_entry = rr_ptr->current_rr+1;
                  rr_entry < rr_ptr->end_rr;
                  rr_entry++) {
                bcopy ((char *) rr_entry, (char *) (rr_entry-1), sizeof (RR));
            }
            rr_ptr->end_rr --;
            rr_ptr->rh.num_entries --;
            return (1);
        }
        else {
            return (UNDEF);
        }
    }

    /* Allocate space (if needed) for new tuple */
    if (rr_ptr->end_rr >=
           rr_ptr->beginning_rr + (rr_ptr->size_rr / sizeof (RR))) {
        if (NULL == (temp_rr = (RR *)
                      realloc ((char *) rr_ptr->beginning_rr,
                                (unsigned) rr_ptr->size_rr + 
                                        (DEFAULT_RR_SIZE * sizeof (RR))))) {
            set_error (errno, rr_ptr->file_name, "write_rr");
            return (UNDEF);
        }
        rr_ptr->end_rr = temp_rr + (rr_ptr->end_rr - rr_ptr->beginning_rr);
        rr_ptr->current_rr = temp_rr + 
                                  (rr_ptr->current_rr - rr_ptr->beginning_rr);
        rr_ptr->beginning_rr = temp_rr;
        rr_ptr->size_rr += DEFAULT_RR_SIZE * sizeof (RR);
    }
    
    if (rr_ptr->last_seek == 0 && rr_ptr->current_rr < rr_ptr->end_rr) {
        /* Must move from current_rr to end_rr up one spot to leave room to */
	/* insert new entry */
        for ( rr_entry = rr_ptr->end_rr; 
              rr_entry > rr_ptr->current_rr; 
              rr_entry--) {
            bcopy ((char *) (rr_entry-1), (char *) rr_entry, sizeof (RR));
        }
    }

    bcopy ((char *) rr, (char *) rr_ptr->current_rr, sizeof (RR));
    
    if (rr_ptr->last_seek == 0) {
        rr_ptr->rh.num_entries++;
        rr_ptr->end_rr++;
    }
    
    /* Revalidate rest of REL_HEADER info */
    if (rr->qid > rr_ptr->rh.max_primary_value) {
        rr_ptr->rh.max_primary_value = rr->qid;
    }
    if (rr->did > rr_ptr->rh.max_secondary_value) {
        rr_ptr->rh.max_secondary_value = rr->did;
    }
        
    rr_ptr->current_rr++;
    rr_ptr->last_seek = 0;
    

    return (1);
}
