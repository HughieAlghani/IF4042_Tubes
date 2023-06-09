#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libfile/close_dict.c,v 11.0 1992/07/21 18:20:46 chrisb Exp $";
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
#include "dict.h"
#include "io.h"
#include "smart_error.h"

extern _SDICT _Sdict[];


int
close_dict (dict_index)
int dict_index;
{
    int fd;
    register _SDICT *dict_ptr = &_Sdict[dict_index];
    _SDICT *tdict_ptr;

    if (dict_ptr->opened == 0) {
        set_error (SM_ILLMD_ERR, dict_ptr->file_name, "close_dict");
        return (UNDEF);
    }

    if (dict_ptr->next_ovfl_index != UNDEF) {
        if (UNDEF == close_dict (dict_ptr->next_ovfl_index))
            return (UNDEF);
    }

    if ((dict_ptr->mode & SINCORE) && (dict_ptr->mode & (SWRONLY | SRDWR))) {
        if (dict_ptr->mode & SBACKUP) {
            if (NULL == (fd = prepare_backup (dict_ptr->file_name))) {
                return (UNDEF);
            }
        }
        else if (-1 == (fd = open (dict_ptr->file_name, SWRONLY))) {
            set_error (errno, dict_ptr->file_name, "close_dict");
            return (UNDEF);
        }

        if (-1 == write (fd,
                         (char *) &dict_ptr->rh, 
                         (int) (sizeof (REL_HEADER))) ||
            -1 == write (fd,
                         (char *) dict_ptr->hsh_table, 
                       (int) (sizeof (HASH_ENTRY) * dict_ptr->hsh_tab_size))||
            -1 == write (fd,
                         dict_ptr->str_table, 
                         (int) (dict_ptr->str_next_loc - 
                                dict_ptr->str_table)) ||
            -1 ==  close (fd)) {

            set_error (errno, dict_ptr->file_name, "close_dict");
            return (UNDEF);
        }
        if (dict_ptr->mode & SBACKUP) {
            if (UNDEF == make_backup (dict_ptr->file_name)) {
                return (UNDEF);
            }
        }
    }

    /* Free up any memory alloced (except if still in use by another rel) */
    /* BUG: never frees buffers if they were shared at some point */
    if ((dict_ptr->mode & SINCORE) && dict_ptr->shared == 0) {
#ifdef MMAP
        if (dict_ptr->mode & SMMAP) {
            if (-1 == munmap (((char *) dict_ptr->hsh_table) -
                                  sizeof (REL_HEADER),
                              (int) dict_ptr->file_size)) {
                set_error (errno, dict_ptr->file_name, "close_dict");
                return (UNDEF);
            }
        }
        else {
            (void) free ((char *) dict_ptr->str_table);
            (void) free ((char *) dict_ptr->hsh_table);
        }
#else
        (void) free ((char *) dict_ptr->str_table);
        (void) free ((char *) dict_ptr->hsh_table);
#endif MMAP
    }
    dict_ptr->opened --;
    /* Handle shared files.  All descriptors with shared set for a file
       should have the same dir_ptr->shared value (1 less than the number of
       shared descriptors) */
    if (dict_ptr->shared) {
        for (tdict_ptr = &_Sdict[0];
             tdict_ptr < &_Sdict[MAX_NUM_DICT];
             tdict_ptr++) {
            if (dict_ptr->mode == tdict_ptr->mode &&
                0 == strcmp (dict_ptr->file_name, tdict_ptr->file_name)) {
                /* decrement count of relations sharing readonly buffers */
                tdict_ptr->shared--;
            }
        }
    }

    return (0);
}
