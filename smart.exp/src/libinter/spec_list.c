#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libinter/spec_list.c,v 11.0 1992/07/21 18:21:55 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include <ctype.h>
#include "common.h"
#include "param.h"
#include "functions.h"
#include "io.h"
#include "smart_error.h"
#include "spec.h"
#include "tr_vec.h"
#include "vector.h"
#include "docindex.h"
#include "inter.h"

static char *spec_list;            /* Whitespace separated list of spec files*/

static SPEC_PARAM spec_args[] = {
    "inter.spec_list",           getspec_string,   (char *) &spec_list,
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

static int num_init = 0;
static int add_spec();


int
init_spec_list (spec, is)
SPEC *spec;
INTER_STATE *is;
{
    char *ptr, *start_spec;

    /* Only initialize the very first time this is called. */
    if (num_init++)
        return (0);

    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    if (NULL == (is->spec_list.spec = (SPEC **) malloc (sizeof (SPEC *))) ||
        NULL == (is->spec_list.spec_name = (char **) malloc (sizeof (char *))))
        return (UNDEF);
    is->spec_list.num_spec = 0;

    /* Copy spec_list to is->spec_list */
    ptr = spec_list;
    while (*ptr) {
        while (*ptr && isspace (*ptr))
            ptr++;
        start_spec = ptr;
        while (*ptr && ! isspace (*ptr))
            ptr++;
        if (UNDEF == add_spec (is, start_spec, ptr - start_spec))
            return (UNDEF);
    }

    return (1);
}


static int
add_spec (is, spec_file, len)
INTER_STATE *is;
char *spec_file;
int len;
{
    char *spec_file_ptr;
    SPEC *spec_ptr;
    if (len <= 0)
        return (0);

    if (NULL == (spec_file_ptr = malloc ((unsigned) len + 1)) ||
        NULL == (spec_ptr = (SPEC *) malloc (sizeof (SPEC))))
        return (UNDEF);

    (void) strncpy (spec_file_ptr, spec_file, len);
    spec_file_ptr[len] = '\0';

    if (NULL == (is->spec_list.spec = (SPEC **)
                 realloc ((char *) is->spec_list.spec,
                          (unsigned) (is->spec_list.num_spec + 1)
                          * sizeof (SPEC *))))
        return (UNDEF);
    if (NULL == (is->spec_list.spec_name = (char **)
                 realloc ((char *) is->spec_list.spec_name,
                          (unsigned) (is->spec_list.num_spec + 1)
                          * sizeof (char *))))
        return (UNDEF);

    if (UNDEF == get_spec (spec_file_ptr, spec_ptr))
        return (UNDEF);

    is->spec_list.spec_name[is->spec_list.num_spec] = spec_file_ptr;
    is->spec_list.spec[is->spec_list.num_spec] = spec_ptr;
    is->spec_list.num_spec++;

    return (1);
}


int
show_spec_list (is, unused)
INTER_STATE *is;
char *unused;
{
    long i;
    char temp_buf[PATH_LEN];

    for (i = 0; i < is->spec_list.num_spec; i++) {
        (void) sprintf (temp_buf, "%3d %s\n", i, is->spec_list.spec_name[i]);
        if (UNDEF == add_buf_string (temp_buf, &is->output_buf))
            return (UNDEF);
    }

    return (1);
}

int
add_spec_list (is, unused)
INTER_STATE *is;
char *unused;
{
    if (is->num_command_line < 2) {
        if (UNDEF == add_buf_string ("No spec_file specified\n", &is->err_buf))
            return (UNDEF);
        return (0);
    }

    if (UNDEF == add_spec (is, 
                           is->command_line[1],
                           strlen (is->command_line[1]))) {
        if (UNDEF == add_buf_string ("Couldn't read spec file\n",
                                     &is->err_buf))
            return (UNDEF);
        return (0);
    }

    return (show_spec_list (is, (char *) NULL));
}

int
use_spec_list (is, unused)
INTER_STATE *is;
char *unused;
{
    int spec_num;

    if (is->num_command_line < 2 || !isdigit (*is->command_line[1])) {
        if (UNDEF == add_buf_string ("No spec_file number specified\n",
                                     &is->err_buf))
            return (UNDEF);
        return (0);
    }
       
    spec_num = atoi (is->command_line[1]);
    if (spec_num < 0 || spec_num >= is->spec_list.num_spec) {
        if (UNDEF == add_buf_string ("Illegal spec_file number specified\n",
                                     &is->err_buf))
            return (UNDEF);
        return (0);
    }

    if (UNDEF == reset_is (is->spec_list.spec[spec_num], is))
        return (UNDEF);

    return (1);
}

        
        

