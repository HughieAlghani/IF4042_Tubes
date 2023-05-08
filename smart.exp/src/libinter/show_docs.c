#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libinter/show_docs.c,v 11.0 1992/07/21 18:21:48 chrisb Exp $";
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

static PROC_INST print_proc;
static long raw_doc_flag;        

static SPEC_PARAM spec_args[] = {
    "inter.print.indivtext",     getspec_func,    (char *) &print_proc.ptab,
    "inter.print.rawdocflag",    getspec_bool,    (char *) &raw_doc_flag,
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

static int num_inst = 0;

static int show_doc(), show_sects();

int
init_show_doc (spec, unused)
SPEC *spec;
char *unused;
{
    long old_context;

    if (num_inst++ > 0) {
        return (num_inst);
    }
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    /* Initialize procedure for printing a single doc (indicate doc rather
       than query is to be printed by context.
    */

    old_context = get_context();
    set_context (CTXT_DOC);
    if (UNDEF == (print_proc.inst = print_proc.ptab->init_proc (spec, NULL)))
        return (UNDEF);
    set_context (old_context);

    return (num_inst);
}


int
close_show_doc (inst)
int inst;
{
    if (--num_inst > 0)
        return (0);

    if (UNDEF == print_proc.ptab->close_proc (print_proc.inst))
        return (UNDEF);

    return (1);
}

int
show_next_doc (is, unused)
INTER_STATE *is;
char *unused;
{
    TR_TUP *tr_tup;
    if (++is->current_doc > is->retrieval.tr->num_tr) {
        if (UNDEF == add_buf_string ("No more docs!\n", &is->err_buf))
            return (UNDEF);
        return (0);
    }
    tr_tup = &is->retrieval.tr->tr[is->current_doc-1];
    if (tr_tup->action == ' ' ||
        tr_tup->action == '\0')
        tr_tup->action = '*';
    return (show_doc (is, tr_tup->did));
}

int
show_named_doc (is, unused)
INTER_STATE *is;
char *unused;
{
    TR_VEC *tr_vec;
    long doc_id, sect_id, para_id, sent_id;
    long i;

    if (is->num_command_line < 2) {
        if (UNDEF == add_buf_string ("No document specified\n", &is->err_buf))
            return (UNDEF);
        return (0);
    }

    if (UNDEF == inter_get_docid (is->command_line[1],
                                  &doc_id,
				  &sect_id,
                                  &para_id,
                                  &sent_id))
        return (0);
    if (0 >= doc_id) {
        if (UNDEF == add_buf_string ("Illegal document specified\n",
                                     &is->err_buf))
            return (UNDEF);
        return (0);
    }

    if (sect_id >= 0 || para_id >= 0 || sent_id >= 0) 
        return (show_sects (is, doc_id, sect_id, para_id, sent_id));

    tr_vec = is->retrieval.tr;
    for (i = 0; i < tr_vec->num_tr; i++) {
        if (doc_id == tr_vec->tr[i].did) {
            is->current_doc = i + 1;
            if (tr_vec->tr[i].action == ' ' ||
                tr_vec->tr[i].action == '\0')
                tr_vec->tr[i].action = '*';
            break;
        }
    }

    return (show_doc (is, doc_id));
}

int
show_current_doc (is, unused)
INTER_STATE *is;
char *unused;
{
    TR_TUP *tr_tup;
    if (is->current_doc == 0)
        is->current_doc = 1;
    if (is->current_doc > is->retrieval.tr->num_tr) {
        if (UNDEF == add_buf_string ("Illegal document specified\n",
                                     &is->err_buf))
            return (UNDEF);
        return (0);
    }
    tr_tup = &is->retrieval.tr->tr[is->current_doc-1];

    if (tr_tup->action == ' ' ||
        tr_tup->action == '\0')
        tr_tup->action = '*';

    return (show_doc (is, tr_tup->did));
}

static int
show_doc (is, doc_id)
INTER_STATE *is;
long doc_id;
{
    TEXTLOC textloc;
    char temp_buf[PATH_LEN];

    if (UNDEF == inter_get_textloc (doc_id, &textloc)) {
        (void) sprintf (temp_buf, "Can't find document %ld\n", doc_id);
        if (UNDEF == add_buf_string (temp_buf, &is->err_buf))
            return (UNDEF);
        return (0);
    }

    if (raw_doc_flag) {
        print_int_textloc (&textloc, &is->output_buf);
    }
    else if (UNDEF == print_proc.ptab->proc (&textloc,
                                             &is->output_buf,
                                             print_proc.inst)) {
        (void) sprintf (temp_buf, "Can't find document %ld\n", doc_id);
        if (UNDEF == add_buf_string (temp_buf, &is->err_buf))
            return (UNDEF);
        return (0);
    }

    return (1);
}


int
show_query (is, unused)
INTER_STATE *is;
char *unused;
{
    if (is->query_textloc.file_name == (char *) NULL) {
        if (UNDEF == add_buf_string ("No query defined\n", &is->err_buf))
            return (UNDEF);
        return (0);
    }

    print_int_textloc (&is->query_textloc, &is->output_buf);
    return (1);
}


static int
show_sects (is, doc_id, sect_id, para_id, sent_id)
INTER_STATE *is;
long doc_id, sect_id, para_id, sent_id;
{
    SM_INDEX_TEXTDOC pp;
    char temp_buf[PATH_LEN];
    SM_BUF sm_buf;
    long i;

    if (UNDEF == inter_get_sect_pp_all (doc_id, sect_id, para_id, sent_id,
                                        &pp)) {
        if (UNDEF == add_buf_string ("Can't find document\n", &is->err_buf))
            return (UNDEF);
        return (0);
    }
    for (i = 0; i < pp.mem_doc.num_sections; i++) {
        if (sent_id >= 0) {
            (void) sprintf (temp_buf, "\n----- Sentence %ld -----\n",
                            sent_id > 0  ? sent_id : i);
        }
        else if (para_id >= 0) {
            (void) sprintf (temp_buf, "\n----- Paragraph %ld -----\n",
                            para_id > 0  ? para_id : i);
        }
        else if (sect_id >= 0) {
            (void) sprintf (temp_buf, "\n----- Section %ld -----\n",
                            sect_id > 0  ? sect_id : i);
        }
        if (UNDEF == add_buf_string (temp_buf, &is->output_buf))
            return (UNDEF);
        sm_buf.buf = &pp.doc_text[pp.mem_doc.sections[i].begin_section];
        sm_buf.end = pp.mem_doc.sections[i].end_section -
            pp.mem_doc.sections[i].begin_section;
        if (UNDEF == add_buf (&sm_buf, &is->output_buf))
            return (UNDEF);
    }
    
    return (1);
}

int
toggle_raw_doc (is, unused)
INTER_STATE *is;
char *unused;
{
    raw_doc_flag = ! raw_doc_flag;
    if (UNDEF == add_buf_string (raw_doc_flag ?
                                     "Raw document will be printed" :
                                     "Formatted doc will be printed",
                                 &is->output_buf))
        return (UNDEF);
    return (1);
}
