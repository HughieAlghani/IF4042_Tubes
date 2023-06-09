#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libprint/pobj_dict.c,v 11.0 1992/07/21 18:23:29 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 print dictionary object
 *1 print.obj.dict
 *2 print_obj_dict (in_file, out_file, inst)
 *3   char *in_file;
 *3   char *out_file;
 *3   int inst;
 *4 init_print_obj_dict (spec, unused)
 *5   "print.dict_file"
 *5   "print.dict_file.rmode"
 *5   "print.trace"
 *4 close_print_obj_dict (inst)
 *6 global_start,global_end used to indicate what range of cons will be printed
 *7 The dict relation "in_file" (if not VALID_FILE, then use dict_file),
 *7 will be used to print all dict entries in that file (modulo global_start,
 *7 global_end).  Dict output to go into file "out_file" (if not VALID_FILE,
 *7 then stdout).
***********************************************************************/

#include <fcntl.h>
#include <ctype.h>
#include "common.h"
#include "param.h"
#include "functions.h"
#include "smart_error.h"
#include "proc.h"
#include "spec.h"
#include "docindex.h"
#include "trace.h"
#include "dict.h"
#include "buf.h"

static char *dict_file;
static long dict_mode;

static SPEC_PARAM spec_args[] = {
    "print.dict_file",     getspec_dbfile, (char *) &dict_file,
    "print.dict_file.rmode",getspec_filemode, (char *) &dict_mode,
    TRACE_PARAM ("print.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);


int
init_print_obj_dict (spec, unused)
SPEC *spec;
char *unused;
{
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_print_obj_dict");

    PRINT_TRACE (2, print_string, "Trace: leaving init_print_obj_dict");
    return (0);
}

int
print_obj_dict (in_file, out_file, inst)
char *in_file;
char *out_file;
int inst;
{
    DICT_ENTRY dict;
    int status;
    char *final_dict_file;
    FILE *output;
    SM_BUF output_buf;
    int dict_index;              /* file descriptor for dict_file */

    PRINT_TRACE (2, print_string, "Trace: entering print_obj_dict");

    final_dict_file = VALID_FILE (in_file) ? in_file : dict_file;
    output = VALID_FILE (out_file) ? fopen (out_file, "w") : stdout;
    if (NULL == output)
        return (UNDEF);
    output_buf.size = 0;

    if (UNDEF == (dict_index = open_dict (final_dict_file, dict_mode)))
        return (UNDEF);

    /* Get each document in turn */
    if (global_start > 0) {
        dict.con = global_start;
        dict.token = NULL;
        if (UNDEF == seek_dict (dict_index, &dict)) {
            return (UNDEF);
        }
    }

    while (1 == (status = read_dict (dict_index, &dict)) &&
           dict.con <= global_end) {
        output_buf.end = 0;
        print_dict (&dict, &output_buf);
        (void) fwrite (output_buf.buf, 1, output_buf.end, output);
    }

    if (UNDEF == close_dict (dict_index))
        return (UNDEF);

    if (output != stdin)
        (void) fclose (output);

    PRINT_TRACE (2, print_string, "Trace: leaving print_obj_dict");
    return (status);
}


int
close_print_obj_dict (inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_print_obj_dict");

    PRINT_TRACE (2, print_string, "Trace: leaving close_print_obj_dict");
    return (0);
}
