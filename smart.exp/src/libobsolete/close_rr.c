#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libobsolete/close_rr.c,v 11.0 1992/07/21 18:22:49 chrisb Exp $";
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
close_rr (rr_index)
int rr_index;
{
    register _SRR_FILES *rr_ptr = &(_Srel_ranks[rr_index]);
    int fd;
    int num_rr;
    
    rr_ptr->opened = 0;
    if (rr_ptr->mode & (SWRONLY | SRDWR)) {
        if (rr_ptr->mode & SBACKUP) {
           if (UNDEF == backup (rr_ptr->file_name)) {
               return (UNDEF);
           }
        }

        if (-1 == (fd = open (rr_ptr->file_name, SWRONLY))) {
            set_error (errno, rr_ptr->file_name, "close_rr");
            return (UNDEF);
        }

        if (sizeof (REL_HEADER) != write (fd,
                                          (char *) &rr_ptr->rh, 
                                          sizeof (REL_HEADER))) {
            set_error (errno, rr_ptr->file_name, "close_rr");
            return (UNDEF);
        }
        
        num_rr = rr_ptr->end_rr - rr_ptr->beginning_rr;
        if (num_rr != rr_ptr->rh.num_entries) {
            set_error (SM_INCON_ERR, rr_ptr->file_name, "close_rr");
            return (UNDEF);
        }
        
        if (num_rr * sizeof (RR) != write (fd, 
                                           (char *) rr_ptr->beginning_rr, 
                                           num_rr * sizeof (RR))) {
            set_error (errno, rr_ptr->file_name, "close_rr");
            return (UNDEF);
        }
        if (-1 == close (fd)) {
            set_error (errno, rr_ptr->file_name, "close_rr");
            return (UNDEF);
        }
    }
    if (rr_ptr->mode & SINCORE) {
        (void) free ((char *) rr_ptr->beginning_rr);
    }

    return (0);
}
