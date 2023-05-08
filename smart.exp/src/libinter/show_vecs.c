#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libinter/show_vecs.c,v 11.0 1992/07/21 18:21:52 chrisb Exp $";
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
#include "vector.h"
#include "inter.h"

static int print_vec_dict_inst;

static int num_inst = 0;

int
init_show_vec (spec, unused)
SPEC *spec;
char *unused;
{
    if (num_inst++ > 0) {
        return (num_inst);
    }

    if (UNDEF == (print_vec_dict_inst =
                  init_print_vec_dict (spec, (char *) NULL)))
        return (UNDEF);

    return (num_inst);
}


int
close_show_vec (inst)
int inst;
{
    if (--num_inst > 0)
        return (0);

    if (UNDEF == close_print_vec_dict (print_vec_dict_inst))
        return (UNDEF);
    
    return (1);
}

int
show_qvec (is, unused)
INTER_STATE *is;
char *unused;
{
    char temp_buf[PATH_LEN];

    if (is->retrieval.query->vector == NULL) {
        if (UNDEF == add_buf_string ("No valid query.  Command ignored\n",
                                     &is->err_buf))
            return (UNDEF);
        return (0);
    }
    (void) sprintf (temp_buf,
                    "Query vector %ld\nDocid\tCtype\tCon\tWeight\t\tToken\n",
                    ((VEC *) is->retrieval.query->vector)->id_num);
    if (UNDEF == add_buf_string (temp_buf, &is->output_buf))
        return (UNDEF);
    if (UNDEF == print_vec_dict ((VEC *) is->retrieval.query->vector,
                                 &is->output_buf,
                                 print_vec_dict_inst)) {
        if (UNDEF ==add_buf_string("Query can't be printed. Command ignored\n",
                                    &is->err_buf))
            return (UNDEF);
        return (0);
    }
    
    return (1);
}


int
show_dvec (is, unused)
INTER_STATE *is;
char *unused;
{
    char temp_buf[PATH_LEN];
    VEC_LIST vec_list;
    VEC list[2];
    long i;

    vec_list.vec = &list[0];
    list[0].num_conwt = 0;

    if (is->num_command_line < 2 ||
        UNDEF == inter_get_sect_veclist (is->command_line[1], &vec_list)) {
        if (UNDEF == add_buf_string ("No valid document.  Command ignored\n",
                                     &is->err_buf))
            return (UNDEF);
        return (0);
    }

    (void) sprintf (temp_buf,
                   "Document vector %ld\nDocid\tCtype\tCon\tWeight\t\tToken\n",
                    vec_list.id_num);
    if (UNDEF == add_buf_string (temp_buf, &is->output_buf))
        return (UNDEF);

    for (i = 0; i < vec_list.num_vec; i++) {
        if (vec_list.vec[i].num_conwt > 0) {
            if (UNDEF == print_vec_dict (&vec_list.vec[i],
                                         &is->output_buf,
                                         print_vec_dict_inst)) {
                return (UNDEF);
            }
        }
    }

    return (1);
}
