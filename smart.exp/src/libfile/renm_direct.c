#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libfile/renm_direct.c,v 11.0 1992/07/21 18:21:00 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "direct.h"
#include "smart_error.h"

/* Rename an instance of a 'direct' relational object. */
int
rename_direct (in_file_name, out_file_name)
char *in_file_name;
char *out_file_name;
{
    char in_buf[PATH_LEN+4];
    char out_buf[PATH_LEN+4];

    /* Name the var file */
    (void) sprintf (in_buf, "%s.var", in_file_name);
    (void) sprintf (out_buf, "%s.var", out_file_name);

    /* Link out_file_name to the existing in_file_name relation */
    /* Maintain invariant that if the main file_name of a relation
       exists, it is a complete relation */
    if (-1 == link (in_buf, out_buf)) {
        set_error (errno, in_buf, "rename_direct");
        return (UNDEF);
    }
    if (-1 == link (in_file_name, out_file_name)) {
        set_error (errno, in_file_name, "rename_direct");
        return (UNDEF);
    }

    /* Unlink the old filenames */
    if (-1 == unlink (in_file_name)) {
        set_error (errno, in_file_name, "rename_direct");
        return (UNDEF);
    }

    if (-1 == unlink (in_buf)) {
        set_error (errno, in_buf, "rename_direct");
        return (UNDEF);
    }
    return (0);
}
