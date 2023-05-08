#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libindexing/add_parts.c,v 11.0 1992/07/21 18:21:12 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Add indexed parts vectors to already existing collection
 *1 index.top.add_parts
 *2 add_parts (unused1, unused2, inst)
 *3   char *unused1;
 *3   char *unused2;
 *3   int inst;
 *4 init_add_parts (spec, unused)
 *5   "parts.doc.textloc_file"
 *5   "parts.doc.textloc_file.rmode"
 *5   "parts.vec_file"
 *5   "parts.vec_file.rwcmode"
 *5   "parts.map_file"
 *5   "parts.map_file.rwcmode"
 *5   "parts.preparse"
 *5   "index.trace"
 *4 close_add_parts (inst)
 *7 Toplevel indexing procedure.
 *7 Take each indexed document given by textloc_file and reindex it, getting 
 *7 a vector for each section of the document, with sections determined by
 *7 parts.preparse. Store the indexed vectors in vec_file, and a map from 
 *7 the original document id to the new section vector ids in map_file (the
 *7 map is in vector form itself)
 *9 WARNING: Since add_parts does not know what type of parts is being
 *9 requested, it cannot set the context appropriately.  Therefore, 
 *9 in pp_veclist, the context is set to CTXT_SECT, which means that 
 *9 "sect_weight" is used for weighting.
***********************************************************************/
#include "common.h"
#include "param.h"
#include "functions.h"
#include "smart_error.h"
#include "proc.h"
#include "spec.h"
#include "docindex.h"
#include "trace.h"
#include "context.h"
#include "textloc.h"
#include "sm_display.h"
#include "docindex.h"

static char *textloc_file;
static long textloc_mode;
static char *vec_file;
static long vec_mode;
static char *map_file;
static long map_mode;
static PROC_INST pp_parts;

static SPEC_PARAM spec_args[] = {
    "parts.doc.textloc_file",        getspec_dbfile,   (char *) &textloc_file,
    "parts.doc.textloc_file.rmode",  getspec_filemode, (char *) &textloc_mode,
    "parts.vec_file",             getspec_dbfile,   (char *) &vec_file,
    "parts.vec_file.rwcmode",     getspec_filemode, (char *) &vec_mode,
    "parts.map_file",             getspec_dbfile,   (char *) &map_file,
    "parts.map_file.rwcmode",     getspec_filemode, (char *) &map_mode,
    "parts.preparse",             getspec_func,     (char *) &pp_parts.ptab,
    TRACE_PARAM ("index.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

static int textloc_fd, vec_fd, map_fd;
static int pp_veclist_inst;

int
init_add_parts (spec, unused)
SPEC *spec;
char *unused;
{
    PRINT_TRACE (2, print_string, "Trace: entering init_add_parts");
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    /* Call all initialization procedures */
    set_context (CTXT_DOC);
    if (UNDEF == (pp_parts.inst =
                  pp_parts.ptab->init_proc (spec, (char *)NULL)) ||
        UNDEF == (pp_veclist_inst = init_pp_veclist (spec, (char *)NULL)))
        return (UNDEF);

    if (! VALID_FILE (vec_file) ||
        UNDEF == (vec_fd = open_vector (vec_file, vec_mode)) ||
        ! VALID_FILE (map_file) ||
        UNDEF == (map_fd = open_vector (map_file, map_mode)) ||
        ! VALID_FILE (textloc_file) ||
        UNDEF == (textloc_fd = open_textloc (textloc_file, textloc_mode)))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: leaving init_add_parts");
    return (0);
}

int
add_parts (unused1, unused2, inst)
char *unused1;
char *unused2;
int inst;
{
    TEXTLOC text_display;
    SM_INDEX_TEXTDOC pp;
    VEC_LIST vec_list;

    long docid = 0;

    long num_parts_this_doc;
    long num_parts_list = 0;
    VEC parts_vec;
    long ctype_len = 1;
    long i;

    PRINT_TRACE (2, print_string, "Trace: entering add_parts");

    parts_vec.num_ctype = 1;
    parts_vec.ctype_len = &ctype_len;

    /* Get each document in turn */
    while (1 == read_textloc (textloc_fd, &text_display)) {
        if (UNDEF == pp_parts.ptab->proc (&text_display, &pp, pp_parts.inst))
            return (UNDEF);
        if (UNDEF == pp_veclist (&pp, &vec_list, pp_veclist_inst))
            return (UNDEF);

        /* Reserve space for map from doc to its parts. */
        if (vec_list.num_vec > num_parts_list) {
            if (num_parts_list > 0)
                (void) free ((char *) parts_vec.con_wtp);
            num_parts_list = vec_list.num_vec;
            if (NULL == (parts_vec.con_wtp = (CON_WT *)
                         malloc ((unsigned) num_parts_list * sizeof (CON_WT))))
                return (UNDEF);
        }
        num_parts_this_doc = 0;
        for (i = 0; i < vec_list.num_vec; i++) {
            if (vec_list.vec[i].num_conwt > 0) {
                vec_list.vec[i].id_num = ++docid;
                parts_vec.con_wtp[num_parts_this_doc].con = docid;
                parts_vec.con_wtp[num_parts_this_doc].wt = 1.0;
                num_parts_this_doc++;
                if (UNDEF == seek_vector (vec_fd, &vec_list.vec[i]) ||
                    UNDEF == write_vector (vec_fd, &vec_list.vec[i]))
                    return (UNDEF);
            }
        }
        if (num_parts_this_doc > 0) {
            parts_vec.id_num = vec_list.id_num;
            parts_vec.ctype_len[0] = num_parts_this_doc;
            parts_vec.num_conwt = num_parts_this_doc;
            if (UNDEF == seek_vector (map_fd, &parts_vec) ||
                UNDEF == write_vector (map_fd, &parts_vec))
                return (UNDEF);
        }
    }

    if (num_parts_list > 0)
        (void) free ((char *) parts_vec.con_wtp);

    PRINT_TRACE (2, print_string, "Trace: leaving add_parts");
    return (1);
}


int
close_add_parts (inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_add_parts");

    if (UNDEF == pp_parts.ptab->close_proc (pp_parts.inst) ||
        UNDEF == close_pp_veclist (pp_veclist_inst))
        return (UNDEF);

    if (UNDEF == close_vector (vec_fd) ||
        UNDEF == close_vector (map_fd) ||
        UNDEF == close_textloc (textloc_fd))
        return (UNDEF);


    PRINT_TRACE (2, print_string, "Trace: leaving close_add_parts");
    return (0);
}
