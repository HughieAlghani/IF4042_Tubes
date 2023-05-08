#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libprint/p_smeval.c,v 11.0 1992/07/21 18:23:16 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "buf.h"
#include "sm_eval.h"

static SM_BUF internal_output = {0, 0, (char *) 0};

static int cutoff[] = CUTOFF_VALUES;

void
print_smeval (eval, output)
SM_EVAL *eval;
SM_BUF *output;
{
    SM_BUF *out_p;
    char temp_buf[PATH_LEN];
    long i;

    if (output == NULL) {
        out_p = &internal_output;
        out_p->end = 0;
    }
    else {
        out_p = output;
    }

    (void) sprintf (temp_buf,
                    "Query\t%ld\nNum_ret %5ld Num_rel %5ld Num_rel_ret %5ld Num_trunc_ret %5ld\nAverage Recall-Precision %6.4f\nRecall-Precisions Points:\n",
                    eval->qid,
                    eval->num_ret, eval->num_rel, 
                    eval->num_rel_ret, eval->num_trunc_ret,
                    eval->av_recall_precis);
    if (UNDEF == add_buf_string (temp_buf, out_p))
        return;

    for (i = 0; i < NUM_RP_PTS; i++) {
        (void) sprintf (temp_buf,
                        "%4.2f\t%6.4f\n",
                        (double) i / (double) (NUM_RP_PTS-1),
                        eval->recall_precis[i]);
        if (UNDEF == add_buf_string (temp_buf, out_p))
            return;
    }

    if (UNDEF == add_buf_string ("Cutoff\tRecall\tPrecis\n", out_p))
        return;
    (void) sprintf (temp_buf,
                    "Exact\t%6.4f\t%6.4f\t%6.4f\n",
                    eval->exact_recall, eval->exact_precis,
                    eval->exact_trunc_precis);
    if (UNDEF == add_buf_string (temp_buf, out_p))
        return;
    for (i = 0; i < NUM_CUTOFF; i++) {
        (void) sprintf (temp_buf,
                        "%d\t%6.4f\t%6.4f\t%6.4f\n",
                        cutoff[i], eval->recall_cut[i], 
                        eval->precis_cut[i], eval->trunc_cut[i]);
        if (UNDEF == add_buf_string (temp_buf, out_p))
            return;
    }

    if (output == NULL) {
        (void) fwrite (out_p->buf, 1, out_p->end, stdout);
        out_p->end = 0;
    }
}

