#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/liblocal/libconvert/tloc_pvec_o.c,v 11.0 1992/07/21 18:22:01 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Copy the text pointed to by a textloc relational object to a parts vector
 *1 convert.obj.textloc_partvec_obj
 *2 textloc_partvec_obj (in_textloc_file, out_partvec_file, inst)
 *3   char *in_textloc_file;
 *3   char *out_textloc_file;
 *3   int inst;

 *4 init_textloc_partvec_obj (spec, unused)
 *5   "convert.textloc_partvec.textloc_file"
 *5   "convert.textloc_partvec.textloc_file.rmode"
 *5   "convert.textloc_partvec.partvec_file"
 *5   "convert.textloc_partvec.partvec_file.rwcmode"
 *5   "convert.textloc_partvec.preparse"
 *5   "convert.textloc_partvec.get_partvec"
 *5   "textloc_partvec_obj.trace"
 *4 close_textloc_partvec_obj(inst)

 *6 global_start and global_end are checked.  Only those textlocs with
 *6 id_num falling between them are converted.

 *7 Read textloc tuples from in_textloc_file, get the text and
 *7 preparse it with the indicated preparser.  Convert that into a
 *7 part_vec and write that out to the partvec_file.
 *7 If in_textloc_file is NULL then textloc_file is used.
 *7 If out_partvec_file is NULL, then partvec_file is used.
 *7 Return UNDEF if error, else 0;

***********************************************************************/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "smart_error.h"
#include "spec.h"
#include "trace.h"
#include "context.h"
#include "proc.h"
#include "textloc.h"
#include "io.h"
#include "part_vector.h"
#include "docindex.h"

static char *default_textloc_file;
static long read_mode;
static char *default_partvec_file;
static long write_mode;
static PROC_INST preparser;
static PROC_INST get_partvec;

static SPEC_PARAM spec_args[] = {
    "convert.textloc_partvec.textloc_file",
                  getspec_dbfile, (char *) &default_textloc_file,
    "convert.textloc_partvec.textloc_file.rmode",
                  getspec_filemode, (char *) &read_mode,
    "convert.textloc_partvec.partvec_file",
                  getspec_dbfile, (char *) &default_partvec_file,
    "convert.textloc_partvec.partvec_file.rwcmode",
                  getspec_filemode, (char *) &write_mode,
    "convert.textloc_partvec.preparse",
                  getspec_func, (char *) &preparser.ptab,
    "convert.textloc_partvec.get_partvec",
                  getspec_func, (char *) &get_partvec.ptab,
    TRACE_PARAM ("textloc_partvec_obj.trace")
    };
static int num_spec_args = sizeof (spec_args) / sizeof (spec_args[0]);

int
init_textloc_partvec_obj (spec, unused)
SPEC *spec;
char *unused;
{
    if (UNDEF == lookup_spec (spec,
                              &spec_args[0],
                              num_spec_args)) {
        return (UNDEF);
    }

    PRINT_TRACE (2, print_string, "Trace: entering init_textloc_partvec_obj");

    set_context( CTXT_DOC );
    if (UNDEF == (preparser.inst = preparser.ptab->init_proc( spec,
							     (char *)NULL)) ||
	UNDEF == (get_partvec.inst = get_partvec.ptab->init_proc( spec,
							    (char *)NULL )))
	return UNDEF;
 
   PRINT_TRACE (2, print_string, "Trace: leaving init_textloc_partvec_obj");

    return (0);
}

int
textloc_partvec_obj (in_textloc_file, out_partvec_file, inst)
char *in_textloc_file;
char *out_partvec_file;
int inst;
{
    TEXTLOC textloc;
    int status;
    int f_tl, f_pv;
    SM_INDEX_TEXTDOC textdoc;
    PART_VEC pvec;
	

    PRINT_TRACE (2, print_string, "Trace: entering textloc_partvec_obj");

    if (in_textloc_file == NULL)
        in_textloc_file = default_textloc_file;
    if (UNDEF == (f_tl = open_textloc (in_textloc_file, read_mode))){
        return (UNDEF);
    }

    if (out_partvec_file == NULL)
	out_partvec_file = default_partvec_file;
    if (UNDEF == (f_pv = open_partvec( out_partvec_file, write_mode )))
	return UNDEF;


    /* Copy each textloc in turn */
    textloc.id_num = 0;
    if (global_start > 0) {
        textloc.id_num = global_start;
        if (UNDEF == seek_textloc (f_tl, &textloc)) {
            return (UNDEF);
        }
    }

    while (1 == (status = read_textloc (f_tl, &textloc)) &&
           textloc.id_num <= global_end) {

	if (UNDEF==preparser.ptab->proc( &textloc, &textdoc,
					 preparser.inst ))
	    return UNDEF;
	textdoc.id_num = textloc.id_num;

	if (UNDEF==get_partvec.ptab->proc( &textdoc, &pvec,
					  get_partvec.inst) ||
	    UNDEF==seek_partvec( f_pv, &pvec ) ||
	    UNDEF==write_partvec( f_pv, &pvec ))
	    return UNDEF;

    }

    if (UNDEF == close_textloc(f_tl) || 
        UNDEF == close_partvec(f_pv))
        return (UNDEF);
    
    PRINT_TRACE (2, print_string, "Trace: leaving print_textloc_partvec_obj");
    return (status);
}

int
close_textloc_partvec_obj(inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_textloc_partvec_obj");

    if (UNDEF == preparser.ptab->close_proc(preparser.inst) ||
	UNDEF == get_partvec.ptab->close_proc(get_partvec.inst ))
	return UNDEF;

    PRINT_TRACE (2, print_string, "Trace: entering close_textloc_partvec_obj");
    return (0);
}
