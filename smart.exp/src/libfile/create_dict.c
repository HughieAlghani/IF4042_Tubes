#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libfile/create_dict.c,v 11.0 1992/07/21 18:20:47 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include <stdio.h>
#include "common.h"
#include "param.h"
#include "functions.h"
#include "rel_header.h"
#include "dict.h"
#include "io.h"
#include "smart_error.h"

int
create_dict (file_name, rh)
char *file_name;
REL_HEADER *rh;
{
    int fd;
    register HASH_ENTRY *hash_tab;
    HASH_ENTRY hash_entry;
    register long i;
    long size;
    REL_HEADER real_rh;

    real_rh.max_primary_value = 
                        (rh == (REL_HEADER *) NULL || 
                         rh->max_primary_value == UNDEF) ?
                                    DEFAULT_DICT_SIZE :
                                    rh->max_primary_value;
    real_rh.num_entries = 0;
    real_rh._entry_size = sizeof (HASH_ENTRY);
    real_rh.type = S_RH_DICT;
    real_rh.data_type = S_RD_DICT;
    real_rh._internal = 0;
    real_rh.max_secondary_value = 0;

    size = real_rh.max_primary_value * sizeof (HASH_ENTRY);

    if (-1 == (fd = open (file_name, SWRONLY|SCREATE, 0664))) {
        set_error (errno, file_name, "create_dict");
        return (UNDEF);
    }

    hash_entry.info = 0;
    hash_entry.collision_ptr = 0;
    hash_entry.prefix[0] = '\0';
    hash_entry.prefix[1] = '\0';
    hash_entry.str_tab_off = UNDEF;
    

    if (NULL == (hash_tab = (HASH_ENTRY *) 
                              malloc ((unsigned) size))) {
        set_error (errno, file_name, "create_dict");
        return (UNDEF);
    }

    for (i = 0; i < real_rh.max_primary_value; i++) {
        bcopy ((char *) &hash_entry, 
               (char *) &hash_tab[i], 
               sizeof (HASH_ENTRY));
    }

    if (sizeof (REL_HEADER) != write (fd,
                                      (char *) &real_rh, 
                                      sizeof (REL_HEADER))){
        set_error (errno, file_name, "create_dict");
        return (UNDEF);
    }

    if (size != write (fd, (char *) hash_tab, (int) size)) {
        set_error (errno, file_name, "create_dict");
        return (UNDEF);
    }

    (void) free ((char *) hash_tab);
    (void) close (fd);
    
    return (0);
}
    
