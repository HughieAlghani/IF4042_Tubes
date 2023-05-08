#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libobsolete/open_rr.c,v 11.0 1992/07/21 18:22:51 chrisb Exp $";
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

static int get_file();
int create_rr(), seek_rr();

int
open_rr (file_name, mode)
char *file_name;
long mode;
{
    REL_HEADER temp_entry;
    int rr_index;
    
    register _SRR_FILES *rr_ptr;

    for ( rr_ptr = &_Srel_ranks[0];
          rr_ptr < &_Srel_ranks[MAX_NUM_RR] && rr_ptr->opened == 1;
          rr_ptr++)
        ;
    if (rr_ptr == &_Srel_ranks[MAX_NUM_RR]) {
        set_error (SM_ILLPA_ERR, file_name, "open_rr");
        return (UNDEF);
    }
    
    rr_index = rr_ptr - &_Srel_ranks[0];

    rr_ptr->mode = mode;
    
    (void) strncpy (rr_ptr->file_name, file_name, PATH_LEN-4);

    if (mode & SCREATE) {
        if (UNDEF == create_rr (rr_ptr->file_name, &temp_entry)) {
            return (UNDEF);
        }
        rr_ptr->mode = mode & ~SCREATE;
    }
    
    if (mode & SINCORE) {            
        if (UNDEF == get_file (rr_index)) {
            return (UNDEF);
        }
    }
    else {
        /* Ignore given mode, and bring in_core */
        rr_ptr->mode = mode | SINCORE;
        if (UNDEF == get_file (rr_index)) {
            return (UNDEF);
        }
    }
            
    rr_ptr->opened = 1;

    /* Position at beginning of file */
    if (UNDEF == seek_rr (rr_index, (RR *) NULL)) {
        return (UNDEF);
    }

    return (rr_index);
}



static int
get_file (rr_index)
int rr_index;
{
    int size;
    register _SRR_FILES *rr_ptr = &(_Srel_ranks[rr_index]);


    if (-1 == (rr_ptr->fd = open (rr_ptr->file_name, SRDONLY))) {
        set_error (errno, rr_ptr->file_name, "open_rr");
        return (UNDEF);
    }
    rr_ptr->file_size = lseek (rr_ptr->fd, 0L, 2);
    (void) lseek (rr_ptr->fd, 0L, 0);

    /* Get header at beginning of file */
    if (sizeof (REL_HEADER) != read (rr_ptr->fd,
                                     (char *) &rr_ptr->rh,
                                     sizeof (REL_HEADER))) {
        set_error (errno, rr_ptr->file_name, "open_rr");
        return (UNDEF);
    }

    size = rr_ptr->file_size - sizeof (REL_HEADER);

    if (! (rr_ptr->mode & (SWRONLY | SRDWR))) {
        /* If reading existing file (ie SRDONLY) */
        if (NULL == (rr_ptr->beginning_rr = (RR *)
                             malloc ( (unsigned) size))) {
            set_error (errno, rr_ptr->file_name, "open_rr");
            return (UNDEF);
        }

        /* Read the file into memory and close file */
        if (size != read (rr_ptr->fd, (char *) rr_ptr->beginning_rr, size)) {
            set_error (errno, rr_ptr->file_name, "open_rr");
            return (UNDEF);
        }
        rr_ptr->end_rr = rr_ptr->beginning_rr + (size / sizeof (RR));
        rr_ptr->size_rr = size;
    }
    else if (rr_ptr->mode & SRDWR) {
        if (NULL == (rr_ptr->beginning_rr = (RR *)
                             malloc ( (unsigned) size + 
                                       (DEFAULT_RR_SIZE * sizeof (RR))))) {
           set_error (errno, rr_ptr->file_name, "open_rr");
            return (UNDEF);
        }

        /* Read the file into memory */
        if (size != 0 && size != read (rr_ptr->fd, 
                                       (char *) rr_ptr->beginning_rr, 
                                       size)) {
            set_error (errno, rr_ptr->file_name, "open_rr");
            return (UNDEF);
        }
        rr_ptr->end_rr = rr_ptr->beginning_rr + (size/ sizeof (RR));
        rr_ptr->size_rr = size + (DEFAULT_RR_SIZE * sizeof (RR));
    }
    else if (rr_ptr->mode & SWRONLY) {
        /* Writing a new file */
        rr_ptr->size_rr = DEFAULT_RR_SIZE * sizeof (RR);
        if (NULL == (rr_ptr->beginning_rr = (RR *)
                             malloc ( (unsigned) rr_ptr->size_rr))) {
            set_error (errno, rr_ptr->file_name, "open_rr");
            return (UNDEF);
        }
        rr_ptr->end_rr = rr_ptr->beginning_rr;
    }
    else {
        set_error (SM_ILLPA_ERR, rr_ptr->file_name, "open_rr");
        return (UNDEF);
    }
    
    if (-1 == close (rr_ptr->fd)) {
        set_error (errno, rr_ptr->file_name, "open_rr");
        return (UNDEF);
    }
    return (0);
}
