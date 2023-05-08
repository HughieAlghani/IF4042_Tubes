#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libobsolete/write_tr.c,v 11.0 1992/07/21 18:22:56 chrisb Exp $";
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
#include "tr.h"
#include "io.h"
#include "smart_error.h"

extern _STR_FILES _Stop_ranks[];

int
write_tr (tr_index, tr)
int tr_index;
TR * tr;
{
    register _STR_FILES *tr_ptr = &(_Stop_ranks[tr_index]);
    register TR *tr_entry;
    register TR *temp_tr;

    if (tr->qid == UNDEF) {
        if (tr_ptr->last_seek == 1) {
            /* Must delete current_tr tuple */
            for ( tr_entry = tr_ptr->current_tr+1;
                  tr_entry < tr_ptr->end_tr;
                  tr_entry++) {
                bcopy ((char *) tr_entry, (char *) (tr_entry-1), sizeof (TR));
            }
            tr_ptr->end_tr --;
            tr_ptr->rh.num_entries --;
            return (1);
        }
        else {
            return (UNDEF);
        }
    }

    /* Allocate space (if needed) for new tuple */
    if (tr_ptr->end_tr >=
           tr_ptr->beginning_tr + (tr_ptr->size_tr / sizeof (TR))) {
        if (NULL == (temp_tr = (TR *)
                      realloc ((char *) tr_ptr->beginning_tr,
                                (unsigned) tr_ptr->size_tr + 
                                        (DEFAULT_TR_SIZE * sizeof (TR))))) {
            set_error (errno, tr_ptr->file_name, "write_tr");
            return (UNDEF);
        }
        tr_ptr->end_tr = temp_tr + (tr_ptr->end_tr - tr_ptr->beginning_tr);
        tr_ptr->current_tr = temp_tr + 
                                  (tr_ptr->current_tr - tr_ptr->beginning_tr);
        tr_ptr->beginning_tr = temp_tr;
        tr_ptr->size_tr += DEFAULT_TR_SIZE * sizeof (TR);
    }
    
    if (tr_ptr->last_seek == 0 && tr_ptr->current_tr < tr_ptr->end_tr) {
        /* Must move from current_tr to end_tr up one spot to leave room to */
	/* insert new entry */
        for ( tr_entry = tr_ptr->end_tr; 
              tr_entry > tr_ptr->current_tr; 
              tr_entry--) {
            bcopy ((char *) (tr_entry-1), (char *) tr_entry, sizeof (TR));
        }
    }

    bcopy ((char *) tr, (char *) tr_ptr->current_tr, sizeof (TR));
    
    if (tr_ptr->last_seek == 0) {
        tr_ptr->rh.num_entries++;
        tr_ptr->end_tr++;
    }
    
    /* Revalidate rest of REL_HEADER info */
    if (tr->qid > tr_ptr->rh.max_primary_value) {
        tr_ptr->rh.max_primary_value = tr->qid;
    }
        
    tr_ptr->current_tr++;
    tr_ptr->last_seek = 0;
    

    return (1);
}
