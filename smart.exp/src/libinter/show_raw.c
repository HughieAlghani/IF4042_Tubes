#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libinter/show_raw.c,v 11.0 1992/07/21 18:21:51 chrisb Exp $";
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
#include "textloc.h"
#include "io.h"
#include "smart_error.h"
#include "spec.h"
#include "proc.h"
#include "tr_vec.h"
#include "context.h"
#include "retrieve.h"
#include "docindex.h"
#include "inter.h"

int
show_raw_tr (is, unused)
INTER_STATE *is;
char *unused;
{
    char temp_buf[PATH_LEN];
    TR_TUP *tr_tup;
    
    if (UNDEF == add_buf_string
        ("Qid\tRank\tDid\tSimilarity\tUseful?\tAction\tIter\n",
         &is->output_buf))
        return (UNDEF);

    for (tr_tup = is->retrieval.tr->tr;
         tr_tup < &is->retrieval.tr->tr[is->retrieval.tr->num_tr];
         tr_tup++) {
        (void) sprintf (temp_buf,
                        "%ld\t%ld\t%ld\t%9.4f\t%c\t%c\t%d\n",
                        is->retrieval.tr->qid,
                        tr_tup->rank,
                        tr_tup->did,
                        tr_tup->sim,
                        (tr_tup->rel ? 'Y' : ' '),
                        (tr_tup->action ? tr_tup->action : ' '),
                        (int) tr_tup->iter);
        if (UNDEF == add_buf_string (temp_buf, &is->output_buf))
            return (UNDEF);
    }

    return (1);
}

static SPEC *save_spec;
int 
init_show_spec (spec, unused)
SPEC *spec;
char *unused;
{
    save_spec = spec;
    return (1);
}

int
show_spec (is, unused)
INTER_STATE *is;
char *unused;
{
    print_spec (save_spec, &is->output_buf);
    return (1);
}

int
show_raw_textloc (is, unused)
INTER_STATE *is;
char *unused;
{
    TEXTLOC textloc;
    char temp_buf[PATH_LEN];
    TR_TUP *tr_tup;

    for (tr_tup = is->retrieval.tr->tr;
         tr_tup < &is->retrieval.tr->tr[is->retrieval.tr->num_tr];
         tr_tup++) {
        if (1 != inter_get_textloc (tr_tup->did, &textloc)) {
            (void) sprintf (temp_buf,
                            "/nDocument %ld Not Available\n",
                            tr_tup->did);
            if (UNDEF == add_buf_string (temp_buf, &is->output_buf))
                return (UNDEF);
        }
        else {
            print_int_textloc (&textloc, &is->output_buf);
        }
    }

    return (1);
}
