#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/liblocal/libprint/pobj_partvec.c,v 11.0 1992/07/21 18:22:17 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 print vector object
 *1 print.obj.vec
 *2 print_obj_partvec (in_file, out_file, inst)
 *3   char *in_file;
 *3   char *out_file;
 *3   int inst;
 *4 init_print_obj_partvec (spec, unused)
 *5   "print.doc_file"
 *5   "print.doc_file.rmode"
 *5   "print.trace"
 *4 close_print_obj_partvec (inst)
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
#include "part_vector.h"
#include "buf.h"

static char *partvec_file;
static long partvec_mode;

static SPEC_PARAM spec_args[] = {
    "print.partvec_file",     getspec_dbfile, (char *) &partvec_file,
    "print.partvec_file.rmode",getspec_filemode, (char *) &partvec_mode,
    TRACE_PARAM ("print.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);


int
init_print_obj_partvec (spec, unused)
SPEC *spec;
char *unused;
{
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_print_obj_partvec");

    PRINT_TRACE (2, print_string, "Trace: leaving init_print_obj_partvec");
    return (0);
}

int
print_obj_partvec (in_file, out_file, inst)
char *in_file;
char *out_file;
int inst;
{
    PART_VEC pvec;
    int status;
    char *final_partvec_file;
    FILE *output;
    SM_BUF output_buf;
    int pvec_index;              /* file descriptor for partvec_file */

    PRINT_TRACE (2, print_string, "Trace: entering print_obj_partvec");

    final_partvec_file = VALID_FILE (in_file) ? in_file : partvec_file;
    output = VALID_FILE (out_file) ? fopen (out_file, "w") : stdout;
    if (NULL == output)
        return (UNDEF);
    output_buf.size = 0;

    if (UNDEF == (pvec_index = open_partvec (final_partvec_file,
					    partvec_mode)))
        return (UNDEF);

    /* Get each document in turn */
    if (global_start > 0) {
        pvec.id_num = global_start;
        if (UNDEF == seek_partvec (pvec_index, &pvec)) {
            return (UNDEF);
        }
    }

    while (1 == (status = read_partvec (pvec_index, &pvec)) &&
           pvec.id_num <= global_end) {
        output_buf.end = 0;
        print_partvec (&pvec, &output_buf);
        (void) fwrite (output_buf.buf, 1, output_buf.end, output);
    }

    if (UNDEF == close_partvec (pvec_index))
        return (UNDEF);

    if (output != stdin)
        (void) fclose (output);

    PRINT_TRACE (2, print_string, "Trace: leaving print_obj_partvec");
    return (status);
}


int
close_print_obj_partvec (inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_print_obj_partvec");

    PRINT_TRACE (2, print_string, "Trace: leaving close_print_obj_partvec");
    return (0);
}
