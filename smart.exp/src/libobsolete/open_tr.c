#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libobsolete/open_tr.c,v 11.0 1992/07/21 18:22:54 chrisb Exp $";
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

static int get_file();
int create_tr(), seek_tr();

int
open_tr (file_name, mode)
char *file_name;
long  mode;
{
    REL_HEADER temp_entry;
    int tr_index;
    
    register _STR_FILES *tr_ptr;

    for ( tr_ptr = &_Stop_ranks[0];
          tr_ptr < &_Stop_ranks[MAX_NUM_TR] && tr_ptr->opened == 1;
          tr_ptr++)
        ;
    if (tr_ptr == &_Stop_ranks[MAX_NUM_TR]) {
        set_error (SM_ILLPA_ERR, file_name, "open_tr");
        return (UNDEF);
    }
    
    tr_index = tr_ptr - &_Stop_ranks[0];

    tr_ptr->mode = mode;
    
    (void) strncpy (tr_ptr->file_name, file_name, PATH_LEN-4);

    if (mode & SCREATE) {
        if (UNDEF == create_tr (tr_ptr->file_name, &temp_entry)) {
            return (UNDEF);
        }
        tr_ptr->mode = mode & ~SCREATE;
    }
    
    if (mode & SINCORE) {            
        if (UNDEF == get_file (tr_index)) {
            return (UNDEF);
        }
    }
    else {
        /* Ignore given mode, and bring in_core */
        tr_ptr->mode = mode | SINCORE;
        if (UNDEF == get_file (tr_index)) {
            return (UNDEF);
        }
    }
            
    tr_ptr->opened = 1;

    /* Position at beginning of file */
    if (UNDEF == seek_tr (tr_index, (TR *) NULL)) {
        return (UNDEF);
    }

    return (tr_index);
}



static int
get_file (tr_index)
int tr_index;
{
    int size;
    register _STR_FILES *tr_ptr = &(_Stop_ranks[tr_index]);


    if (-1 == (tr_ptr->fd = open (tr_ptr->file_name, SRDONLY))) {
        set_error (errno, tr_ptr->file_name, "open_tr");
        return (UNDEF);
    }
    tr_ptr->file_size = lseek (tr_ptr->fd, 0L, 2);
    (void) lseek (tr_ptr->fd, 0L, 0);

    /* Get header at beginning of file */
    if (sizeof (REL_HEADER) != read (tr_ptr->fd,
                                     (char *) &tr_ptr->rh,
                                     sizeof (REL_HEADER))) {
        set_error (errno, tr_ptr->file_name, "open_tr");
        return (UNDEF);
    }

    size = tr_ptr->file_size - sizeof (REL_HEADER);

    if (! (tr_ptr->mode & (SWRONLY | SRDWR))) {
        /* If reading existing file (ie SRDONLY) */
        if (NULL == (tr_ptr->beginning_tr = (TR *)
                             malloc ( (unsigned) size))) {
            set_error (errno, tr_ptr->file_name, "open_tr");
            return (UNDEF);
        }

        /* Read the file into memory and close file */
        if (size != read (tr_ptr->fd, (char *) tr_ptr->beginning_tr, size)) {
            set_error (errno, tr_ptr->file_name, "open_tr");
            return (UNDEF);
        }
        tr_ptr->end_tr = tr_ptr->beginning_tr + (size / sizeof (TR));
        tr_ptr->size_tr = size;
    }
    else if (tr_ptr->mode & SRDWR) {
        if (NULL == (tr_ptr->beginning_tr = (TR *)
                             malloc ( (unsigned) size + 
                                       (DEFAULT_TR_SIZE * sizeof (TR))))) {
           set_error (errno, tr_ptr->file_name, "open_tr");
            return (UNDEF);
        }

        /* Read the file into memory */
        if (size != 0 && size != read (tr_ptr->fd, 
                                       (char *) tr_ptr->beginning_tr, 
                                       size)) {
            set_error (errno, tr_ptr->file_name, "open_tr");
            return (UNDEF);
        }
        tr_ptr->end_tr = tr_ptr->beginning_tr + (size/ sizeof (TR));
        tr_ptr->size_tr = size + (DEFAULT_TR_SIZE * sizeof (TR));
    }
    else if (tr_ptr->mode & SWRONLY) {
        /* Writing a new file */
        tr_ptr->size_tr = DEFAULT_TR_SIZE * sizeof (TR);
        if (NULL == (tr_ptr->beginning_tr = (TR *)
                             malloc ( (unsigned) tr_ptr->size_tr))) {
            set_error (errno, tr_ptr->file_name, "open_tr");
            return (UNDEF);
        }
        tr_ptr->end_tr = tr_ptr->beginning_tr;
    }
    else {
        set_error (SM_ILLPA_ERR, tr_ptr->file_name, "open_tr");
        return (UNDEF);
    }
    
    if (-1 == close (tr_ptr->fd)) {
        set_error (errno, tr_ptr->file_name, "open_tr");
        return (UNDEF);
    }
    return (0);
}
