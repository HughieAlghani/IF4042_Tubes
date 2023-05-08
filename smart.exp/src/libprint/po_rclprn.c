#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libprint/po_rclprn.c,v 11.0 1992/07/21 18:23:28 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "spec.h"
#include "sm_eval.h"
#include "io.h"
#include "buf.h"
#include "rr_vec.h"
#include "rel_header.h"


static char *rr_file;
static char *run_name;

static SPEC_PARAM spec_args[] = {
    "print.rr_file",         getspec_dbfile, (char *) &rr_file,
    "print.run_name",        getspec_string, (char *) &run_name,
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

static SM_BUF internal_output = {0, 0, (char *) 0};

static int rrvec_smeval_inst;

void
print_rclprn (spec_list, output)
SPEC_LIST *spec_list;
SM_BUF *output;
{
    long i;
    SM_EVAL eval;
    int num_runs;
    int *rr_fd;
    REL_HEADER *rh;
    long qid, max_qid;
    RR_VEC rr_vec;

    SM_BUF *out_p;
    char temp_buf[PATH_LEN];

    if (spec_list == NULL || spec_list->num_spec <= 0)
        return;

    if (output == NULL) {
        out_p = &internal_output;
        out_p->end = 0;
    }
    else {
        out_p = output;
    }

    /* Reserve space for each queries eval output */
    if (NULL == (rr_fd = (int *) 
                 malloc ((unsigned) spec_list->num_spec * sizeof (int))))
        return;
    num_runs = 0;

    /* Initialize rrvec_smeval (actually does nothing except init number 
       of docs in collection. - Eventually won't need) */
    if (UNDEF == (rrvec_smeval_inst = init_rrvec_smeval (spec_list->spec[0],
                                                     (char *) NULL)))
        return;

    /* Get each rr_file in turn */
    for (i = 0; i < spec_list->num_spec; i++) {
        if (UNDEF == lookup_spec (spec_list->spec[i],
                                  &spec_args[0],
                                  num_spec_args))
            return;
        if (! VALID_FILE (rr_file) ||
            UNDEF == (rr_fd[num_runs] =
                      open_rr_vec (rr_file, (long) SRDONLY))) {
            (void) sprintf(temp_buf,
                           "Run %s cannot be evaluated. Ignored\n",
                           rr_file);
            if (UNDEF == add_buf_string (temp_buf, out_p))
                return;
            continue;
        }
        num_runs++;
        (void) sprintf (temp_buf, "%d. %s\n",
                        num_runs,
                        VALID_FILE (run_name) ? run_name : rr_file);
        if (UNDEF == add_buf_string (temp_buf, out_p))
            return;
    }

    if (num_runs == 0) {
        if (UNDEF == add_buf_string ("\n***ERROR*** No valid runs included\n",
                                     out_p))
            return;
    }

    if (NULL == (rh = get_rel_header (rr_file)))
        return;
    max_qid = rh->max_primary_value;
    
    for (qid = 1; qid <= max_qid; qid++) {
        (void) sprintf (temp_buf, "\n%ld", qid);
        if (UNDEF == add_buf_string (temp_buf, out_p))
            return;
        for (i = 0; i < num_runs; i++) {
            rr_vec.qid = qid;
            if (1 != seek_rr_vec (rr_fd[i], &rr_vec) ||
                1 != read_rr_vec (rr_fd[i], &rr_vec) ||
                UNDEF == rrvec_smeval (&rr_vec, &eval, rrvec_smeval_inst)) {
                if (UNDEF == add_buf_string ("\t------", out_p))
                    return;
            }
            else {
                (void) sprintf (temp_buf, "\t%6.4f", eval.av_recall_precis);
                if (UNDEF == add_buf_string (temp_buf, out_p))
                    return;
            }
        }
    }
    if (UNDEF == add_buf_string ("\n", out_p))
        return;

    if (UNDEF == close_rrvec_smeval (rrvec_smeval_inst))
        return;

    for (i = 0; i < num_runs; i++) {
        if (UNDEF == close_rr_vec (rr_fd[i]))
            return;
    }

    (void) free ((char *) rr_fd);

    if (output == NULL) {
        (void) fwrite (out_p->buf, 1, out_p->end, stdout);
        out_p->end = 0;
    }

    return;
}

