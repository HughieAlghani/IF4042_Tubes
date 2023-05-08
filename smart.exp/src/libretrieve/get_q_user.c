#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libretrieve/get_q_user.c,v 11.0 1992/07/21 18:23:45 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Get a query from the user, index it, and return result in query_vec.
 *1 retrieve.get_query.get_q_user
 *2 get_q_user (unused, query_vec, inst)
 *3   char *unused;
 *3   QUERY_VECTOR *query_vec;
 *3   int inst;
 *4 init_get_q_user (spec, unused)
 *5   "retrieve.user_qdisp"
 *7   "retrieve.qdisp_query"
 *7   "retrieve.get_query.trace"
 *4 close_get_q_user (inst)
 *7 get_q_user gets a query from the user with the procedure given by
 *7 parameter user_qdisp, and then indexes that query using the
 *7 procedure given by qdisp_query.
***********************************************************************/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "smart_error.h"
#include "proc.h"
#include "spec.h"
#include "trace.h"
#include "context.h"
#include "retrieve.h"
#include "vector.h"
#include "textloc.h"

/* Get the next query from the user, then index it and return it */

static PROC_INST user_qdisp;
static PROC_INST qdisp_query;


static SPEC_PARAM spec_args[] = {
    "retrieve.user_qdisp",       getspec_func,    (char *) &user_qdisp.ptab,
    "retrieve.qdisp_query",      getspec_func,    (char *) &qdisp_query.ptab,
    TRACE_PARAM ("retrieve.get_query.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

static int num_inst = 0;

int
init_get_q_user (spec, unused)
SPEC *spec;
char *unused;
{
    if (num_inst++ > 0) {
        PRINT_TRACE (2, print_string,
                     "Trace: entering/leaving init_get_q_user");
        return (num_inst);
    }

    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_get_q_user");

    if (UNDEF == (user_qdisp.inst = user_qdisp.ptab->init_proc (spec, NULL)) ||
        UNDEF == (qdisp_query.inst = qdisp_query.ptab->init_proc (spec, NULL)))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: leaving init_get_q_user");
    return (0);
}

int
get_q_user (unused, query_vec, inst)
char *unused;
QUERY_VECTOR *query_vec;
int inst;
{
    TEXTLOC textloc;
    int status;

    if (1 != (status = user_qdisp.ptab->proc ((char *) NULL,
                                              &textloc,
                                              user_qdisp.inst)))
        return (status);

    if (1 != (status = qdisp_query.ptab->proc (&textloc,
                                               query_vec,
                                               qdisp_query.inst)))
        return (status);

    return (1);
}

int
close_get_q_user (inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_get_q_user");

    if (--num_inst == 0) {
        if (UNDEF == user_qdisp.ptab->close_proc (user_qdisp.inst) ||
            UNDEF == qdisp_query.ptab->close_proc (qdisp_query.inst))
            return (UNDEF);
    }

    PRINT_TRACE (2, print_string, "Trace: leaving close_get_q_user");
    return (0);
}
