#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libinter/show_inv.c,v 11.0 1992/07/21 18:21:50 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "io.h"
#include "spec.h"
#include "inv.h"
#include "inter.h"

static long num_ctypes;

static SPEC_PARAM spec_args[] = {
    "num_ctypes",          getspec_long,    (char *) &num_ctypes,
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

static int num_inst = 0;
static int *ctype_inst;

int
init_show_inv (spec, unused)
SPEC *spec;
char *unused;
{
    long i;
    char param_prefix[PATH_LEN];

    if (num_inst++ > 0) {
        return (num_inst);
    }
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    if (NULL == (ctype_inst = (int *)
                 malloc ((unsigned) num_ctypes * sizeof (int))))
        return (UNDEF);

    for (i = 0; i < num_ctypes; i++) {
        (void) sprintf (param_prefix, "inter.ctype.%ld.", i);
        ctype_inst[i] = init_con_inv (spec, param_prefix);
    }

    return (num_inst);
}


int
close_show_inv (inst)
int inst;
{
    long i;

    if (--num_inst > 0)
        return (0);


    for (i = 0; i < num_ctypes; i++) {
        if (ctype_inst[i]!= UNDEF && UNDEF == close_con_inv (ctype_inst[i]))
            return (UNDEF);
    }

    return (1);
}

int
show_inv (is, unused)
INTER_STATE *is;
char *unused;
{
    long con, ctype;
    INV inv;
    char temp_buf[PATH_LEN];

    if (is->num_command_line < 2) {
        if (UNDEF == add_buf_string ("No con specified\n", &is->err_buf))
            return (UNDEF);
        return (0);
    }
    con = atol (is->command_line[1]);

    if (is->num_command_line == 2)
        ctype = 0;
    else
        ctype = atol (is->command_line[2]);
    
    if (ctype < 0 || ctype >= num_ctypes) {
        if (UNDEF == add_buf_string ("Illegal ctype specified\n",&is->err_buf))
            return (UNDEF);
        return (0);
    }

    if (UNDEF == con_inv (&con, &inv, ctype_inst[ctype])) {
        (void) sprintf (temp_buf, "Can't find con %ld\n", con);
        if (UNDEF == add_buf_string (temp_buf, &is->err_buf))
            return (UNDEF);
        return (0);
    }

    print_inv (&inv, &is->output_buf);

    return (1);
}

