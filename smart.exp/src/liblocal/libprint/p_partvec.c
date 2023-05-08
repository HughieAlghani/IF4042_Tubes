#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/liblocal/libprint/p_partvec.c,v 11.0 1992/07/21 18:22:17 chrisb Exp $";
#endif
/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "smart_error.h"
#include "display.h"
#include "docindex.h"
#include "part_vector.h"
#include "buf.h"

static SM_BUF internal_output = {0, 0, (char *) 0};

/* Print a VEC relation to stdout */
void
print_partvec (pvec, output)
PART_VEC *pvec;
SM_BUF *output;
{
    long i;
    PART_CON_WT *p_conwtp = pvec->part_con_wtp;
    long ctype;
    char temp_buf[PATH_LEN];
    SM_BUF *out_p;

    if (output == NULL) {
        out_p = &internal_output;
        out_p->end = 0;
    }
    else {
        out_p = output;
    }

    for (ctype = 0; ctype < pvec->num_ctype; ctype++) {
        for (i = 0; i < pvec->ctype_len[ctype]; i++) {
            (void) sprintf (temp_buf, 
                            "%ld\t%ld\t%ld\t%f\t%d\n",
                            pvec->id_num,
                            ctype,
                            p_conwtp->con,
                            p_conwtp->wt,
			    p_conwtp->partnum);
            if (UNDEF == add_buf_string (temp_buf, out_p))
                return;
            if (++p_conwtp > &pvec->part_con_wtp[pvec->num_part_conwt]) {
                (void) fprintf (stderr,
                     "within print_partvec: %ld: Inconsistant vector length\n",
                                pvec->id_num);
            }
        }
    }
    if (p_conwtp != &pvec->part_con_wtp[pvec->num_part_conwt]) {
        (void) fprintf (stderr,
               "within print_partvec: %ld: Inconsistant final vector length\n",
                        pvec->id_num);
    }
    if (output == NULL) {
        (void) fwrite (out_p->buf, 1, out_p->end, stdout);
        out_p->end = 0;
    }
}

