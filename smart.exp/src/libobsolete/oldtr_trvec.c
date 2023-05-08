#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libobsolete/oldtr_trvec.c,v 11.0 1992/07/21 18:22:46 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 convert old rel-rank (tr) file object into current format (tr_vec)
 *1 convert.obj.oldtr_trvec
 *2 oldtr_trvec (oldtr_file, tr_file, inst)
 *3 char *oldtr_file;
 *3 char *tr_file;
 *3 int inst;
 *4 init_oldtr_trvec (spec, unused)
 *5   "convert.oldtr_trvec.tr_file"
 *5   "convert.oldtr_trvec.tr_file.rmode"
 *5   "convert.oldtr_trvec.tr_file.rwcmode"
 *5   "oldtr_trvec.trace"
 *7 Convert old implementation of inverted file to new implementation.
 *7 Only difference between the two is that inv.num_list is now a long
 *7 instead of an unsigned short
 *9 Will disappear shortly.
***********************************************************************/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "smart_error.h"
#include "spec.h"
#include "trace.h"
#include "context.h"
#include "proc.h"
#include "vector.h"
#include "tr.h"
#include "tr_vec.h"
#include "io.h"

static char *default_oldtr_file;
static long oldtr_mode;
static long tr_mode;


static SPEC_PARAM spec_args[] = {
    "convert.oldtr_trvec.tr_file",  getspec_dbfile, (char *) &default_oldtr_file,
    "convert.oldtr_trvec.tr_file.rmode",getspec_filemode, (char *) &oldtr_mode,
    "convert.oldtr_trvec.tr_file.rwcmode",getspec_filemode, (char *) &tr_mode,
    TRACE_PARAM ("oldtr_trvec.trace")
    };
static int num_spec_args = sizeof (spec_args) / sizeof (spec_args[0]);


static SPEC save_spec;

extern int open_tr(), seek_tr(), read_tr(), close_tr();

static int init_get_trvec();
static int get_trvec();

int
init_oldtr_trvec (spec, unused)
SPEC *spec;
char *unused;
{
    if (UNDEF == lookup_spec (spec,
                              &spec_args[0],
                              num_spec_args)) {
        return (UNDEF);
    }

    PRINT_TRACE (2, print_string, "Trace: entering init_oldtr_trvec");

    (void) copy_spec (&save_spec, spec);

    PRINT_TRACE (2, print_string, "Trace: leaving init_oldtr_trvec");

    return (0);
}
int
oldtr_trvec (oldtr_file, tr_file, inst)
char *oldtr_file;
char *tr_file;
int inst;
{
    TR oldtr;
    TR_VEC tr_vec;
    int status;
    int oldtr_index, tr_index;

    PRINT_TRACE (2, print_string, "Trace: entering oldtr_trvec");

    if (oldtr_file == NULL)
        oldtr_file = default_oldtr_file;
    if (UNDEF == (oldtr_index = open_tr (oldtr_file, oldtr_mode))){
        return (UNDEF);
    }

    if (UNDEF == (tr_index = open_tr_vec (tr_file, tr_mode))){
        return (UNDEF);
    }

    /* Convert each oldtr in turn */
    if (global_start > 0) {
        oldtr.qid = global_start;
        oldtr.did = UNDEF;
        if (UNDEF == seek_tr (oldtr_index, &oldtr)) {
            return (UNDEF);
        }
    }
    if (UNDEF == init_get_trvec (oldtr_index))
        return (UNDEF);

    while (1 == (status = get_trvec (oldtr_index, &tr_vec)) &&
           tr_vec.qid <= global_end) {
        if (0 != seek_tr_vec (tr_index, &tr_vec) ||
            1 != write_tr_vec (tr_index, &tr_vec))
            return (UNDEF);
    }

    if (0 != seek_tr_vec (tr_index, &tr_vec) ||
        1 != write_tr_vec (tr_index, &tr_vec))
        return (UNDEF);

    if (UNDEF == close_tr_vec (tr_index))
        return (UNDEF);

    if (UNDEF == close_tr (oldtr_index))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: leaving oldtr_trvec");
    return (status);
}

int
close_oldtr_trvec(inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_oldtr_trvec");

    PRINT_TRACE (2, print_string, "Trace: entering close_oldtr_trvec");
    return (0);
}

/* Temporary routine for reading a TR relation by qid, but reporting
   a TR_VEC object */

static TR_TUP *query_tr_by_qid;
static int num_tr = 0;

static TR temp_tr;


static int
init_get_trvec (tr_fd)
int tr_fd;
{
    if (NULL == (query_tr_by_qid = (TR_TUP *) 
                 malloc (150 * sizeof (TR_TUP)))) {
        return (UNDEF);
    }
    num_tr = 150;

    if (UNDEF == read_tr (tr_fd, &temp_tr))
        return (UNDEF);
    return (1);
}

static int
get_trvec (tr_fd, tr_vec)
int tr_fd;
TR_VEC *tr_vec;
{
    long i;
    long save_qid;
    int status;
    
    i = 0;
        
    save_qid = temp_tr.qid;

    do {
        /* Reserve space if needed for new tuple */
        if (i >= num_tr) {
            if (NULL == (query_tr_by_qid = 
                         (TR_TUP *) realloc ((char *) query_tr_by_qid,
                                             (unsigned)(150 + num_tr) * 
                                             sizeof (TR_TUP))))
                return (UNDEF);
            num_tr += 150;
        }
        query_tr_by_qid[i].did = temp_tr.did;
        query_tr_by_qid[i].rank = temp_tr.rank;
        query_tr_by_qid[i].rel = temp_tr.rel;
        query_tr_by_qid[i].action = temp_tr.action;
        query_tr_by_qid[i].iter = temp_tr.iter;
        query_tr_by_qid[i].sim = temp_tr.sim;
        i++;
    } while (1 == (status = read_tr (tr_fd, &temp_tr)) &&
             temp_tr.qid == save_qid);

    tr_vec->qid = save_qid;
    tr_vec->num_tr = i;
    tr_vec->tr = query_tr_by_qid;

    return (status);
}
