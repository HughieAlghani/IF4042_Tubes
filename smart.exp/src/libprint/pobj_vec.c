#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libprint/pobj_vec.c,v 11.0 1992/07/21 18:23:33 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 print vector object
 *1 print.obj.vec
 *2 print_obj_vec (in_file, out_file, inst)
 *3   char *in_file;
 *3   char *out_file;
 *3   int inst;
 *4 init_print_obj_vec (spec, unused)
 *5   "print.doc_file"
 *5   "print.doc_file.rmode"
 *5   "print.trace"
 *4 close_print_obj_vec (inst)
 *6 global_start,global_end used to indicate what range of cons will be printed
 *7 The vec relation "in_file" (if not VALID_FILE, then use doc_file),
 *7 will be used to print all vec entries in that file (modulo global_start,
 *7 global_end).  Vec output to go into file "out_file" (if not VALID_FILE,
 *7 then stdout).
***********************************************************************/

#include <fcntl.h>
#include "common.h"
#include "param.h"
#include "functions.h"
#include "smart_error.h"
#include "proc.h"
#include "spec.h"
#include "trace.h"
#include "vector.h"
#include "buf.h"

static char *vec_file;
static long vec_mode;
static VEC *vektor;
static  long jumlah, jumlah_d, hitung = 0;
static int ctype, i = 0;

static SPEC_PARAM spec_args[] = {
    "print.doc_file",     getspec_dbfile, (char *) &vec_file,
    "print.doc_file.rmode",getspec_filemode, (char *) &vec_mode,
    TRACE_PARAM ("print.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);


int
init_print_obj_vec (spec, unused)
SPEC *spec;
char *unused;
{
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_print_obj_vec");

    PRINT_TRACE (2, print_string, "Trace: leaving init_print_obj_vec");
    return (0);
}

int
print_obj_vec (in_file, out_file, inst)
char *in_file;
char *out_file;
int inst;
{
    VEC vec;
    int status;
    char *final_vec_file;
    FILE *output;
    SM_BUF output_buf;
    int vec_index;              /* file descriptor for vec_file */

    PRINT_TRACE (2, print_string, "Trace: entering print_obj_vec");

    final_vec_file = VALID_FILE (in_file) ? in_file : vec_file;
    output = VALID_FILE (out_file) ? fopen (out_file, "w") : stdout;
    if (NULL == output)
        return (UNDEF);
    output_buf.size = 0;

    if (UNDEF == (vec_index = open_vector (final_vec_file, vec_mode)))
        return (UNDEF);

    /* Get each document in turn */
    if (global_start > 0) {
        vec.id_num = global_start;
        if (UNDEF == seek_vector (vec_index, &vec)) {
            return (UNDEF);
        }
    }

    while (1 == (status = read_vector (vec_index, &vec)) &&
           vec.id_num <= global_end) {
        output_buf.end = 0;
        vektor = &vec;

    for (ctype = 0; ctype < vektor->num_ctype; ctype++) {
        for (i = 0; i < vektor->ctype_len[ctype]; i++) {
          jumlah_d = jumlah_d + vektor->con_wtp->wt;

        }
    }

        jumlah = jumlah + vektor->num_conwt;
        hitung = hitung + 1;
       /* print_vector (&vec, &output_buf); */
/*        (void) fwrite (output_buf.buf, 1, output_buf.end, output); */
    }
    if (UNDEF == close_vector (vec_index))
        return (UNDEF);

    printf("jumlah document  = %ld \n", hitung);
    printf ("jumlah unique terms  = %ld \n", jumlah);
    printf ("jumlah terms  = %ld \n", jumlah_d);

    if (output != stdin)
        (void) fclose (output);

    PRINT_TRACE (2, print_string, "Trace: leaving print_obj_vec");
    return (status);
}


int
close_print_obj_vec (inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_print_obj_vec");

    PRINT_TRACE (2, print_string, "Trace: leaving close_print_obj_vec");
    return (0);
}
