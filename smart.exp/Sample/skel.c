#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/Sample/skel.c,v 11.0 1992/07/21 18:18:26 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/* Note: in Procedure description, every content line must begin with
   the appropriate " *<digit> ".  Try not to leave blank lines within
   a numbered section.
*/
/********************   PROCEDURE DESCRIPTION   ************************
 *0 one line description of procedure
 *1 procedure hierarchy name
 *2 main procedure name
 *3 typed args
 *4 associated proc names (init_ , close_ ) with args
 *5 spec file args used by proc (or init_ proc)
 *6 global variables used
 *7 full description of actions of procedures including
 *7 return values.
 *8 description of implementation
 *9 bugs and warnings
***********************************************************************/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "smart_error.h"
#include "spec.h"
#include "trace.h"
#include "io.h"
#include "inst.h"
#include "proc.h"

/* Complicated skeleton for low-level hierarchy procedure.  An example
   would be the procedure to store a ctype of a vector in an inverted
   file.  Here, all variables giving the state of an instantiation are
   stored in a STATIC_INFO structure; with each instantiation having
   its own structure (see inst.h for some more details).  In this example,
   instantiations called for the same file_name are put together, so
   that all those instantiations share the same open file.

   In this example, the initialization procedure takes an argument
   specifying a string prefix for the filename wanted.  This enables,
   for instance, two different instantiations to reference the value of
           ctype.1.file_name
           ctype.2.file_name
   where these file_name values can either be different or the same.
*/


static PROC_INST pp;        /* Preparsing procedure */
static long num_ctypes;     /* Max number of ctypes in doc */

static SPEC_PARAM spec_args[] = {
    "skel.doc.preparse",     getspec_func, (char *) &pp.ptab,
    "skel.num_ctypes",       getspec_long,    (char *) &num_ctypes,
    TRACE_PARAM ("skel.trace")
    };
static int num_spec_args = sizeof (spec_args) / sizeof (spec_args[0]);

static char *prefix;
static char *file_name;
static long file_mode;
static SPEC_PARAM_PREFIX prefix_spec_args[] = {
    &prefix, "file_name",       getspec_dbfile,    (char *) &file_name,
    &prefix, "file_name.rwmode",getspec_filemode,  (char *) &file_mode,
    };
static int num_prefix_spec_args = sizeof (prefix_spec_args) /
         sizeof (prefix_spec_args[0]);

/* Static info to be kept for each instantiation of this proc */
typedef struct {
    /* bookkeeping */
    int valid_info;

    char file_name[PATH_LEN];
    int fd;
    long num_ctypes;
    PROC_INST save_pp;
} STATIC_INFO;

static STATIC_INFO *info;
static int max_inst = 0;

int
init_skel (spec, param_prefix)
SPEC *spec;
char *param_prefix;
{
    int i;
    STATIC_INFO *ip;
    SPEC_PARAM spec_param;
    char param[PATH_LEN];
    char *file_name;
    long file_mode;
    int new_inst;

    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    /* Lookup the filename wanted for this application (normally
       param_prefix might be "ctype.<digit>") */
    prefix = param_prefix;
    if (UNDEF == lookup_spec_prefix (spec,
                                     &prefix_spec_args[0],
                                     num_prefix_spec_args))

    PRINT_TRACE (1, print_string, "Trace: entering init_skel");
    PRINT_TRACE (2, print_string, file_name);
    
    /* Check to see if this file_name has already been initialized.  If
       so, that instantiation will be used. */
    for (i = 0; i < max_inst; i++) {
        if (info[i].valid_info && 0 == strcmp (file_name, info[i].file_name)) {
            info[i].valid_info++;
            PRINT_TRACE (1, print_string, "Trace: leaving init_skel");
            return (i);
        }
    }
    
    NEW_INST (new_inst);
    if (UNDEF == new_inst)
        return (UNDEF);
    
    ip = &info[new_inst];

    strncpy (ip->file_name, file_name, PATH_LEN);
    ip->num_ctypes = num_ctypes;

    if (UNDEF == (ip->fd = open_file (file_name, file_mode)))
        return (UNDEF);

    /* Call all initialization procedures for indexing */
    ip->save_pp.ptab = pp.ptab;
    if (UNDEF == (ip->save_pp.inst = pp.ptab->init_proc (spec, param_prefix)))
        return (UNDEF);

...

    ip->valid_info = 1;
             
    PRINT_TRACE (1, print_string, "Trace: leaving init_skel");

    return (new_inst);
}

int
skel (inarg, outarg, inst)
char *inarg;
char *outarg;
int inst;
{
    STATIC_INFO *ip;

    PRINT_TRACE (1, print_string, "Trace: entering skel");
    PRINT_TRACE (3, print_vector, inarg);

    if (! VALID_INST (inst)) {
        set_error (SM_ILLPA_ERR, "Instantiation", "skel");
        return (UNDEF);
    }
    ip  = &info[inst];

...

    PRINT_TRACE (1, print_string, "Trace: leaving skel");

    return (1);
}

int
close_skel(inst)
int inst;
{
    STATIC_INFO *ip;

    PRINT_TRACE (1, print_string, "Trace: entering close_skel");

    if (! VALID_INST (inst)) {
        set_error (SM_ILLPA_ERR, "Instantiation", "close_skel");
        return (UNDEF);
    }

    ip  = &info[inst];
    ip->valid_info--;
    /* Output everything and free buffers if this was last close of this
       inst */
    if (ip->valid_info == 0) {

...
        if (UNDEF == (ip->save_pp.ptab->close_proc (ip->save_pp.inst)))
            return (UNDEF);
        
        if (UNDEF == close_file (ip->fd))
            return (UNDEF);
    }

    PRINT_TRACE (1, print_string, "Trace: leaving close_skel");
    return (0);
}
