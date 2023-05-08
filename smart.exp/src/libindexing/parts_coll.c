#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libindexing/parts_coll.c,v 11.0 1992/07/21 18:21:34 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Add text of parts to already existing collection
 *1 index.top.parts_coll
 *2 parts_coll (unused1, unused2, inst)
 *3   char *unused1;
 *3   char *unused2;
 *3   int inst;
 *4 init_parts_coll (spec, unused)
 *5   "parts.doc.textloc_file"
 *5   "parts.doc.textloc_file.rmode"
 *5   "parts.text_file"
 *5   "parts.text_file.rwcmode"
 *5   "parts.map_section"
 *5   "parts.map_file"
 *5   "parts.map_file.rwcmode"
 *5   "parts.preparse"
 *5   "index.trace"
 *4 close_parts_coll (inst)
 *7 Toplevel indexing procedure.
 *7 Take each indexed document given by textloc_file and break it into parts,
 *7 with sections determined by parts.preparse.  Write each part out 
 *7 in SMART form (see smart_pp.c) that can later be indexed as a complete doc.
 *7 Store the new docs in text_file, and a map from 
 *7 the original document id to the new part ids in map_file (the
 *7 map is in vector form itself)
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
#include "io.h"
#include "buf.h"

static char *textloc_file;
static long textloc_mode;
static char *text_file;
static long text_mode;
static char *map_file;
static long map_mode;
static char *map_sectionid;
static long title_section;
static PROC_INST pp_parts;

static SPEC_PARAM spec_args[] = {
    "parts.doc.textloc_file",        getspec_dbfile,   (char *) &textloc_file,
    "parts.doc.textloc_file.rmode",  getspec_filemode, (char *) &textloc_mode,
    "parts.text_file",             getspec_dbfile,   (char *) &text_file,
    "parts.text_file.rwcmode",     getspec_filemode, (char *) &text_mode,
    "parts.map_file",             getspec_dbfile,   (char *) &map_file,
    "parts.map_file.rwcmode",     getspec_filemode, (char *) &map_mode,
    "parts.map_section",          getspec_string,   (char *) &map_sectionid,
    "index.title_section",        getspec_long,     (char *) &title_section,
    "parts.preparse",             getspec_func,     (char *) &pp_parts.ptab,
    TRACE_PARAM ("index.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

static int textloc_fd, text_fd, map_fd;

static SM_INDEX_DOCDESC doc_desc;    /* New style doc description */
static int map_section();

int
init_parts_coll (spec, unused)
SPEC *spec;
char *unused;
{
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_parts_coll");

    if (UNDEF == lookup_spec_docdesc (spec, &doc_desc)) {
        return (UNDEF);
    }

    /* Call all initialization procedures */
    set_context (CTXT_DOC);
    if (UNDEF == (pp_parts.inst =
                  pp_parts.ptab->init_proc (spec, (char *)NULL)))
        return (UNDEF);

    if (! VALID_FILE (text_file) ||
        UNDEF == (text_fd = open (text_file, (int)text_mode & SMASK, 0660)) ||
        ! VALID_FILE (map_file) ||
        UNDEF == (map_fd = open_vector (map_file, map_mode)) ||
        ! VALID_FILE (textloc_file) ||
        UNDEF == (textloc_fd = open_textloc (textloc_file,
                                             textloc_mode)))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: leaving init_parts_coll");
    return (0);
}

int
parts_coll (unused1, unused2, inst)
char *unused1;
char *unused2;
int inst;
{
    char temp_buf[PATH_LEN+100];
    TEXTLOC text_display;
    SM_INDEX_TEXTDOC pp;

    SM_BUF text_buf, old_buf, title_buf;

    long part_docid = 1;

    long num_parts_list = 0;
    VEC parts_vec;
    long ctype_len = 1;
    long i;

    PRINT_TRACE (2, print_string, "Trace: entering parts_coll");
    text_buf.size = old_buf.size = title_buf.size = 0;

    parts_vec.num_ctype = 1;
    parts_vec.ctype_len = &ctype_len;

    /* Get each document in turn */
    while (1 == read_textloc (textloc_fd, &text_display)) {
        if (UNDEF == pp_parts.ptab->proc (&text_display, &pp, pp_parts.inst))
            return (UNDEF);
        /* Reserve space for map from doc to its parts. */
        if (pp.mem_doc.num_sections > num_parts_list) {
            if (num_parts_list > 0)
                (void) free ((char *) parts_vec.con_wtp);
            num_parts_list = pp.mem_doc.num_sections;
            if (NULL == (parts_vec.con_wtp = (CON_WT *)
                         malloc ((unsigned) num_parts_list * sizeof (CON_WT))))
                return (UNDEF);
        }

        /* Find the current title section of the doc */
        /* Warning: this only finds the last title section and ignores any
         *  others */
        for (i = 0; i < pp.mem_doc.num_sections; i++) {
            if (title_section ==
                map_section (pp.mem_doc.sections[i].section_id)){
                title_buf.buf = pp.doc_text +
                    pp.mem_doc.sections[i].begin_section;
                title_buf.end =  pp.mem_doc.sections[i].end_section -
                    pp.mem_doc.sections[i].begin_section;
                if (title_buf.end > 100)
                    title_buf.end = 100;
            }
        }

        /* Write out each section as a separate doc */
        for (i = 0; i < pp.mem_doc.num_sections; i++) {
            /* Construct new doc text which will have two sections.
               First new section will be given by
               map_section and will contain mapping info plus the old
               title of the full document.  This section will presumably
               not be indexed.  The second new section will be a copy
               of the current old section.
             */
            text_buf.end = 0;
            old_buf.buf = &pp.doc_text[pp.mem_doc.sections[i].begin_section];
            old_buf.end = pp.mem_doc.sections[i].end_section -
                pp.mem_doc.sections[i].begin_section;
            (void) sprintf (temp_buf, "\n.i %ld %ld %s\n.%c\n%ld.%ld\t",
                            pp.textloc_doc.begin_text,
                            pp.textloc_doc.end_text,
                            pp.textloc_doc.file_name,
                            map_sectionid[0],
                            text_display.id_num,
                            i);
            if (UNDEF == add_buf_string (temp_buf, &text_buf) || 
                UNDEF == add_buf (&title_buf, &text_buf))
                return (UNDEF);
            (void) sprintf (temp_buf, "\n.%c\n",
                            pp.mem_doc.sections[i].section_id);
            if (UNDEF == add_buf_string (temp_buf, &text_buf) || 
                UNDEF == add_buf (&old_buf, &text_buf))
                return (UNDEF);

            if (-1 == write (text_fd, text_buf.buf, (int) text_buf.end))
                return (UNDEF);

            /* Store map info */
            parts_vec.con_wtp[i].con = part_docid;
            parts_vec.con_wtp[i].wt = 1.0;

            part_docid++;
        }

        parts_vec.id_num = text_display.id_num;
        parts_vec.num_conwt = parts_vec.ctype_len[0] = pp.mem_doc.num_sections;
        if (UNDEF == seek_vector (map_fd, &parts_vec) ||
            UNDEF == write_vector (map_fd, &parts_vec))
            return (UNDEF);
    }

    if (num_parts_list > 0)
        (void) free ((char *) parts_vec.con_wtp);

    PRINT_TRACE (2, print_string, "Trace: leaving parts_coll");
    return (1);
}


int
close_parts_coll (inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_parts_coll");

    if (UNDEF == pp_parts.ptab->close_proc (pp_parts.inst))
        return (UNDEF);

    if (UNDEF == close (text_fd) ||
        UNDEF == close_vector (map_fd) ||
        UNDEF == close_textloc (textloc_fd))
        return (UNDEF);


    PRINT_TRACE (2, print_string, "Trace: leaving close_parts_coll");
    return (0);
}

static int
map_section (section_id)
char section_id;
{
    long i;
    for (i = 0; 
         i < doc_desc.num_sections &&
            doc_desc.sections[i].name[0] != section_id;
         i++)
        ;
    
    if (i >= doc_desc.num_sections) {
        return (UNDEF);
    }
    return (i);
}
