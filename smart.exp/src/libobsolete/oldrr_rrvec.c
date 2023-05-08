#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libobsolete/oldrr_rrvec.c,v 11.0 1992/07/21 18:22:46 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 convert old rel-rank (rr) file object into current format (rr_vec)
 *1 convert.obj.oldrr_rrvec
 *2 oldrr_rrvec (oldrr_file, rr_file, inst)
 *3 char *oldrr_file;
 *3 char *rr_file;
 *3 int inst;
 *4 init_oldrr_rrvec (spec, unused)
 *5   "convert.oldrr_rrvec.rr_file"
 *5   "convert.oldrr_rrvec.rr_file.rmode"
 *5   "convert.oldrr_rrvec.rr_file.rwcmode"
 *5   "oldrr_rrvec.trace"
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
#include "rr.h"
#include "rr_vec.h"
#include "io.h"

static char *default_oldrr_file;
static long oldrr_mode;
static long rr_mode;


static SPEC_PARAM spec_args[] = {
    "convert.oldrr_rrvec.rr_file",  getspec_dbfile, (char *) &default_oldrr_file,
    "convert.oldrr_rrvec.rr_file.rmode",getspec_filemode, (char *) &oldrr_mode,
    "convert.oldrr_rrvec.rr_file.rwcmode",getspec_filemode, (char *) &rr_mode,
    TRACE_PARAM ("oldrr_rrvec.trace")
    };
static int num_spec_args = sizeof (spec_args) / sizeof (spec_args[0]);


static SPEC save_spec;

extern int open_rr(), seek_rr(), read_rr(), close_rr();

static int init_get_rrvec();
static int get_rrvec();

int
init_oldrr_rrvec (spec, unused)
SPEC *spec;
char *unused;
{
    if (UNDEF == lookup_spec (spec,
                              &spec_args[0],
                              num_spec_args)) {
        return (UNDEF);
    }

    PRINT_TRACE (2, print_string, "Trace: entering init_oldrr_rrvec");

    (void) copy_spec (&save_spec, spec);

    PRINT_TRACE (2, print_string, "Trace: leaving init_oldrr_rrvec");

    return (0);
}
int
oldrr_rrvec (oldrr_file, rr_file, inst)
char *oldrr_file;
char *rr_file;
int inst;
{
    RR oldrr;
    RR_VEC rr_vec;
    int status;
    int oldrr_index, rr_index;

    PRINT_TRACE (2, print_string, "Trace: entering oldrr_rrvec");

    if (oldrr_file == NULL)
        oldrr_file = default_oldrr_file;
    if (UNDEF == (oldrr_index = open_rr (oldrr_file, oldrr_mode))){
        return (UNDEF);
    }

    if (UNDEF == (rr_index = open_rr_vec (rr_file, rr_mode))){
        return (UNDEF);
    }

    /* Convert each oldrr in turn */
    if (global_start > 0) {
        oldrr.qid = global_start;
        oldrr.did = UNDEF;
        if (UNDEF == seek_rr (oldrr_index, &oldrr)) {
            return (UNDEF);
        }
    }
    if (UNDEF == init_get_rrvec (oldrr_index))
        return (UNDEF);

    while (1 == (status = get_rrvec (oldrr_index, &rr_vec)) &&
           rr_vec.qid <= global_end) {
        if (0 != seek_rr_vec (rr_index, &rr_vec) ||
            1 != write_rr_vec (rr_index, &rr_vec))
            return (UNDEF);
    }

    if (0 != seek_rr_vec (rr_index, &rr_vec) ||
        1 != write_rr_vec (rr_index, &rr_vec))
        return (UNDEF);

    if (UNDEF == close_rr_vec (rr_index))
        return (UNDEF);

    if (UNDEF == close_rr (oldrr_index))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: leaving oldrr_rrvec");
    return (status);
}

int
close_oldrr_rrvec(inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_oldrr_rrvec");

    PRINT_TRACE (2, print_string, "Trace: entering close_oldrr_rrvec");
    return (0);
}

/* Temporary routine for reading a RR relation by qid, but reporting
   a RR_VEC object */

static RR_TUP *query_rr_by_qid;
static int num_rr = 0;

static RR temp_rr;


static int
init_get_rrvec (rr_fd)
int rr_fd;
{
    if (NULL == (query_rr_by_qid = (RR_TUP *) 
                 malloc (150 * sizeof (RR_TUP)))) {
        return (UNDEF);
    }
    num_rr = 150;

    if (UNDEF == read_rr (rr_fd, &temp_rr))
        return (UNDEF);
    return (1);
}

static int
get_rrvec (rr_fd, rr_vec)
int rr_fd;
RR_VEC *rr_vec;
{
    long i;
    long save_qid;
    int status;
    
    i = 0;
        
    save_qid = temp_rr.qid;

    do {
        /* Reserve space if needed for new tuple */
        if (i >= num_rr) {
            if (NULL == (query_rr_by_qid = 
                         (RR_TUP *) realloc ((char *) query_rr_by_qid,
                                             (unsigned)(150 + num_rr) * 
                                             sizeof (RR_TUP))))
                return (UNDEF);
            num_rr += 150;
        }
        query_rr_by_qid[i].did = temp_rr.did;
        query_rr_by_qid[i].rank = temp_rr.rank;
        query_rr_by_qid[i].sim = temp_rr.sim;
        i++;
    } while (1 == (status = read_rr (rr_fd, &temp_rr)) &&
             temp_rr.qid == save_qid);

    rr_vec->qid = save_qid;
    rr_vec->num_rr = i;
    rr_vec->rr = query_rr_by_qid;

    return (status);
}
