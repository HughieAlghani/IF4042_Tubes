#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libobsolete/read_tr.c,v 11.0 1992/07/21 18:22:55 chrisb Exp $";
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

extern _STR_FILES _Stop_ranks[];


int
read_tr (tr_index, tr)
int tr_index;
register TR * tr;
{
    register TR * tr_entry;
    register _STR_FILES *tr_ptr = &(_Stop_ranks[tr_index]);

    tr_entry = tr_ptr->current_tr;
    
    if (tr_entry == NULL || tr_entry >= tr_ptr->end_tr) {
        return (0);
    }

    bcopy ((char *) tr_entry, (char *) tr, sizeof (TR));

    tr_ptr->current_tr++;
    tr_ptr->last_seek = 0;
    return (1);
}
