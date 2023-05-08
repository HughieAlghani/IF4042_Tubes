#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libfile/creat_direct.c,v 11.0 1992/07/21 18:20:53 chrisb Exp $";
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
#include "direct.h"
#include "io.h"
#include "smart_error.h"

extern DIRECT_TYPES direct_object_type[];

/* Create an instance of a 'direct' relational object.  The only field of */
/* the input rel_header that is looked at is 'rh->type'. The rest are */
/* ignored */
int
create_direct (file_name, rh)
char *file_name;
REL_HEADER *rh;
{
    int fd;
    REL_HEADER real_rh;
    char buf[PATH_LEN];
    register DIRECT_TYPES *dir_type_ptr;

    if (-1 != (fd = open (file_name, 0))) {
        (void) close (fd);
        set_error (EEXIST, file_name, "create_direct");
        return (UNDEF);
    }

    /* Open and close the var file */
    (void) sprintf (buf, "%s.var", file_name);

    if (-1 == (fd = open (buf, SWRONLY | SCREATE, 0664))) {
        set_error (errno, buf, "create_direct");
        return (UNDEF);
    }
    (void) close (fd);

    if (-1 == (fd = open (file_name, SWRONLY | SCREATE, 0664))) {
        set_error (errno, file_name, "create_direct");
        return (UNDEF);
    }

#ifdef notdef
    /* Alternative in case some ancient version of UNIX can't give
       mode on open line */
    if ( -1 == (fd = creat (buf, 0664))) {
        set_error (errno, buf, "create_direct");
        return (UNDEF);
    }
    (void) close (fd);

    if ( -1 == (fd = creat (file_name, 0664))) {
        set_error (errno, file_name, "create_direct");
        return (UNDEF);
    }
#endif

    /* Calculate the length of the fixed size part of a tuple */
    dir_type_ptr = &direct_object_type[rh->type];
    real_rh._entry_size = dir_type_ptr->atts
                             [dir_type_ptr->num_fixed_fields - 1].field_offset
                          + dir_type_ptr->atts
                             [dir_type_ptr->num_fixed_fields - 1].field_size
                          + sizeof (long);


    real_rh.max_primary_value = -1;
    real_rh.max_secondary_value = -1;
    real_rh.num_entries = 0;
    real_rh.data_type = S_RD_DIRECT;
    real_rh.type = rh->type;
    real_rh._internal = 0;

    /* Write the relation header */
    if (sizeof (REL_HEADER) != write (fd,
                                      (char *) &real_rh, 
                                      sizeof (REL_HEADER))){
        set_error (errno, file_name, "create_direct");
        return (UNDEF);
    }

    (void) close (fd);
    return (0);
}
