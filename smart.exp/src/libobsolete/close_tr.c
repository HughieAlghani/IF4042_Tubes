#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libobsolete/close_tr.c,v 11.0 1992/07/21 18:22:53 chrisb Exp $";
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
close_tr (tr_index)
int tr_index;
{
    register _STR_FILES *tr_ptr = &(_Stop_ranks[tr_index]);
    int fd;
    int num_tr;
    
    tr_ptr->opened = 0;
    if (tr_ptr->mode & (SWRONLY | SRDWR)) {
        if (tr_ptr->mode & SBACKUP) {
           if (UNDEF == backup (tr_ptr->file_name)) {
               return (UNDEF);
           }
        }

        if (-1 == (fd = open (tr_ptr->file_name, SWRONLY))) {
            set_error (errno, tr_ptr->file_name, "close_tr");
            return (UNDEF);
        }

        if (sizeof (REL_HEADER) != write (fd,
                                          (char *) &tr_ptr->rh, 
                                          sizeof (REL_HEADER))) {
            set_error (errno, tr_ptr->file_name, "close_tr");
            return (UNDEF);
        }
        
        num_tr = tr_ptr->end_tr - tr_ptr->beginning_tr;
        if (num_tr != tr_ptr->rh.num_entries) {
            set_error (SM_INCON_ERR, tr_ptr->file_name, "close_tr");
            return (UNDEF);
        }
        
        if (num_tr * sizeof (TR) != write (fd, 
                                           (char *) tr_ptr->beginning_tr, 
                                           num_tr * sizeof (TR))) {
            set_error (errno, tr_ptr->file_name, "close_tr");
            return (UNDEF);
        }
        if (-1 == close (fd)) {
            set_error (errno, tr_ptr->file_name, "close_tr");
            return (UNDEF);
        }
    }
    if (tr_ptr->mode & SINCORE) {
        (void) free ((char *) tr_ptr->beginning_tr);
    }

    return (0);
}
