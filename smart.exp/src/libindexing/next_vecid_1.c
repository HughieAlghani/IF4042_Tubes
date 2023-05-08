#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libindexing/next_vecid_1.c,v 11.0 1992/07/21 18:21:18 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Give next id for a new vector, starting at 1
 *1 index.next_vecid.next_vecid_1
 *2 next_vecid_1 (unused, id, inst)
 *3   char *unused;
 *3   long *id;
 *3   int inst;
 *4 init_next_vecid_1 (spec, unused)
 *5   "index.next_vecid.trace"
 *4 close_next_vecid (inst)
 *8 First return 1, and then increment by 1 with each call
***********************************************************************/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "smart_error.h"
#include "spec.h"
#include "trace.h"


static SPEC_PARAM spec_args[] = {
    TRACE_PARAM ("index.next_vecid.trace")
    };
static int num_spec_args = sizeof (spec_args) / sizeof (spec_args[0]);


static long next_vecid = 0;

int
init_next_vecid_1 (spec, unused)
SPEC *spec;
char *unused;
{
    /* Lookup the values of the relevant parameters */
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args)) {
        return (UNDEF);
    }

    PRINT_TRACE (2, print_string, "Trace: entering init_next_vecid_1");

    if (next_vecid > 0) {
        set_error (SM_ILLPA_ERR, "Attempt to re-initialize","init_next_vecid_1");
        return (UNDEF);
    }

    next_vecid = 1;
    PRINT_TRACE (2, print_string, "Trace: leaving init_next_vecid_1");

return (0);
}

int
next_vecid_1 (unused, id, inst)
char *unused;
long *id;
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering next_vecid_1");

    *id = next_vecid++;

    PRINT_TRACE (4, print_long, id);
    PRINT_TRACE (2, print_string, "Trace: leaving next_vecid_1");

    return (0);
}

int
close_next_vecid_1 (inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_next_vecid_1");

    next_vecid = 0;

    PRINT_TRACE (2, print_string, "Trace: leaving close_next_vecid_1");

    return (0);
}



