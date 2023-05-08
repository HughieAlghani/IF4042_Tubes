#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libfeedback/occ_inv.c,v 11.0 1992/07/21 18:20:42 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "inv.h"
#include "tr_vec.h"
#include "vector.h"
#include "feedback.h"
#include "spec.h"
#include "trace.h"

static char *inv_file;           
static long inv_file_mode;

static SPEC_PARAM spec_args[] = {
    "feedback.inv_file",         getspec_dbfile, (char *) &inv_file,
    "feedback.inv_file.rmode",   getspec_filemode, (char *) &inv_file_mode,
    TRACE_PARAM ("feedback.occ_info.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

static int inv_fd = UNDEF;

int
init_occ_inv (spec, unused)
SPEC *spec;
char *unused;
{
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_occ_inv");

    if (inv_fd == UNDEF &&
        UNDEF == (inv_fd = open_inv (inv_file, inv_file_mode)))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: leaving init_occ_inv");
    return (1);
}

int
occ_inv (unused, fdbk_info, inst)
char *unused;
FEEDBACK_INFO *fdbk_info;
int inst;
{
    long i,j, k;
    INV inv;

    PRINT_TRACE (2, print_string, "Trace: entering occ_inv");

    for (j = 0; j < fdbk_info->tr->num_tr; j++) {
        if (fdbk_info->tr->tr[j].rel)
            fdbk_info->num_rel++;
    }

    for (i = 0; i < fdbk_info->num_occ; i++) {
        inv.id_num = fdbk_info->occ[i].con;
        if (1 != seek_inv (inv_fd, &inv) ||
            1 != read_inv (inv_fd, &inv) )
            return (0);

        for (k = 0; k < inv.num_listwt; k++) {
            /* Discover whether doc is in top_docs */
            for (j = 0;
                 j < fdbk_info->tr->num_tr &&
                 fdbk_info->tr->tr[j].did < inv.listwt[k].list;
                 j++)
                ;

            if (j < fdbk_info->tr->num_tr &&
                fdbk_info->tr->tr[j].did == inv.listwt[k].list) {
                if (fdbk_info->tr->tr[j].rel) {
                    fdbk_info->occ[i].rel_ret++;
                    fdbk_info->occ[i].rel_weight += inv.listwt[k].wt;
                }
                else {
                    fdbk_info->occ[i].nrel_ret++;
                    fdbk_info->occ[i].nrel_weight += inv.listwt[k].wt;
                }
            }
            else {
                fdbk_info->occ[i].nret++;
            }
        }
    }

    PRINT_TRACE (4, print_fdbk_info, fdbk_info);
    PRINT_TRACE (2, print_string, "Trace: leaving occ_inv");

    return (1);
}


int
close_occ_inv (inst)
int inst;
{

    PRINT_TRACE (2, print_string, "Trace: entering close_occ_inv");

    if (inv_fd != UNDEF && UNDEF == close_inv (inv_fd))
        return (UNDEF);
    inv_fd = UNDEF;

    PRINT_TRACE (2, print_string, "Trace: leaving close_occ_inv");
    return (1);
}
