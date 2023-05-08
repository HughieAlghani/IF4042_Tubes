#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libconvert/pp_pp_parts.c,v 11.0 1992/07/21 18:20:13 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Break a preparsed doc into smaller parts based on syntax
 *2 pp_pp_parts (in_pp, out_pp, inst)
 *3 SM_INDEX_TEXTDOC *in_pp;
 *3 SM_INDEX_TEXTDOC *out_pp;
 *3 int inst;
 *4 init_pp_pp_parts (spec, unused)
 *5   "convert.pp_pp_parts.trace"
 *4 close_pp_pp_parts (inst)
 *6 global_context is used to tell what the scope of the smaller
 *6 parts shoule be (CTXT_SENT CTXT_PARA CTXT_SECT)
 *7 Look at the text of the preparsed doc in_pp, and divide each section
 *7 of in_pp into smaller sections based on the syntactic division requested.
 *7 For example, if CTXT_SENT is set and a section of in_pp has 3 sentences,
 *7 then out_pp will contain 3 corresponding sections; each section containing
 *7 the text of one sentence.  The section_id of the in_pp section is 
 *7 propogated to each of the corresponding sections of out_pp.
 *7 pp_pp_parts is most often used immediately before a call to pp_veclist,
 *7 which would then index out_pp, giving a vector for each preparsed section.
 *7 This is a way to index individual sentences, paragraphs and sections of
 *7 a document.
 *9 Definition of a sentence and paragraph should agree with that used in
 *9 the parsing procedure, but this is dependant on the tokenizer.
***********************************************************************/

#include <ctype.h>
#include "common.h"
#include "param.h"
#include "functions.h"
#include "smart_error.h"
#include "proc.h"
#include "spec.h"
#include "docindex.h"
#include "trace.h"
#include "inst.h"
#include "context.h"
#include "section.h"

/* Convert a preparsed doc into one where every paragraph and/or
   sentence is a new section.
   Most often used with pp_veclist which then indexes each section as
   a separate vector.
*/

static SPEC_PARAM spec_args[] = {
    TRACE_PARAM ("convert.pp_pp_parts.trace")
    };
static int num_spec_args = sizeof (spec_args) / sizeof (spec_args[0]);

/* Static info to be kept for each instantiation of this proc */
typedef struct {
    /* bookkeeping */
    int valid_info;

    SM_DISP_SEC *sections;
    int num_sections;
    int max_sections;
    int sent_flag;
    int para_flag;
    int sect_flag;
} STATIC_INFO;

static STATIC_INFO *info;
static int max_inst = 0;

static int new_section(), parse_section();


int
init_pp_pp_parts (spec, unused)
SPEC *spec;
char *unused;
{
    STATIC_INFO *ip;
    int new_inst;

    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_pp_pp_parts");

    NEW_INST (new_inst);
    if (UNDEF == new_inst)
        return (UNDEF);
    
    ip = &info[new_inst];

    ip->max_sections = 50;
    if (NULL == (ip->sections = (SM_DISP_SEC *)
                 malloc ((unsigned) ip->max_sections * sizeof (SM_DISP_SEC))))
        return (UNDEF);

    
    ip->sent_flag = (check_context (CTXT_SENT));
    ip->para_flag = (check_context (CTXT_PARA));
    ip->sect_flag = (check_context (CTXT_SECT));
        
    ip->valid_info = 1;
             
    PRINT_TRACE (2, print_string, "Trace: leaving init_pp_pp_parts");

    return (new_inst);
}

int
pp_pp_parts (in_pp, out_pp, inst)
SM_INDEX_TEXTDOC *in_pp;
SM_INDEX_TEXTDOC *out_pp;
int inst;
{
    STATIC_INFO *ip;
    long i;

    PRINT_TRACE (2, print_string, "Trace: entering pp_pp_parts");

    if (! VALID_INST (inst)) {
        set_error (SM_ILLPA_ERR, "Instantiation", "pp_pp_parts");
        return (UNDEF);
    }
    ip  = &info[inst];

    out_pp->id_num = in_pp->id_num;
    out_pp->doc_text = in_pp->doc_text;

    out_pp->textloc_doc.id_num = in_pp->textloc_doc.id_num; 
    out_pp->textloc_doc.begin_text = in_pp->textloc_doc.begin_text;
    out_pp->textloc_doc.end_text = in_pp->textloc_doc.end_text;
    out_pp->textloc_doc.file_name = in_pp->textloc_doc.file_name;
    out_pp->textloc_doc.title = in_pp->textloc_doc.title;
    out_pp->textloc_doc.doc_type = in_pp->textloc_doc.doc_type;

    out_pp->mem_doc.id_num = in_pp->textloc_doc.id_num; 
    out_pp->mem_doc.file_name = in_pp->mem_doc.file_name;
    out_pp->mem_doc.title = in_pp->mem_doc.title;

    /* Go through in_pp->mem_doc section by section, breaking up each section
       into smaller sections of same type at paragraph or sentence breaks */
    ip->num_sections = 0;
    for (i = 0; i < in_pp->mem_doc.num_sections; i++) {
        if (UNDEF == parse_section (ip, 
                                    &in_pp->mem_doc.sections[i],
                                    in_pp->doc_text))
            return (UNDEF);
    }

    out_pp->mem_doc.num_sections = ip->num_sections;
    out_pp->mem_doc.sections = ip->sections;

    PRINT_TRACE (2, print_string, "Trace: leaving pp_pp_parts");

    return (1);
}

int
close_pp_pp_parts (inst)
int inst;
{
    STATIC_INFO *ip;

    PRINT_TRACE (2, print_string, "Trace: entering close_pp_pp_parts");

    if (! VALID_INST (inst)) {
        set_error (SM_ILLPA_ERR, "Instantiation", "close_pp_pp_parts");
        return (UNDEF);
    }

    ip  = &info[inst];
    ip->valid_info--;
    /* Output everything and free buffers if this was last close of this
       inst */
    if (ip->valid_info == 0) {
        (void) free ((char *) ip->sections);
    }

    PRINT_TRACE (2, print_string, "Trace: leaving close_pp_pp_parts");
    return (0);
}

static int
parse_section (ip, section, doc_text)
STATIC_INFO *ip;
SM_DISP_SEC *section;
char *doc_text;
{
    char *end_ptr = &doc_text[section->end_section];
    char *start_ptr = &doc_text[section->begin_section];
    char *ptr, *new_ptr;
    int new_sentence = 0;

    for (ptr = start_ptr; ptr < end_ptr; ptr++) {
        switch (*ptr) {
            /* Punctuation determining end of sentence */
          case '?':
          case '!':
            if (ip->sent_flag) {
                if (UNDEF == new_section (ip,
                                          section->section_id,
                                          start_ptr - doc_text,
                                          ptr + 1 - doc_text))
                    return (UNDEF);
                start_ptr = ptr+1;
            }
            new_sentence = 1;
            break;
          case '.':
            new_ptr = ptr+1;
            while (new_ptr < end_ptr && ispunct (*new_ptr)) {
                if (*new_ptr == ',')
                    break;
                new_ptr++;
            }
            if (new_ptr >= end_ptr || ! isspace (*new_ptr))
                break;
            /* Skip over blanks (but not over new_lines) */
            while (new_ptr < end_ptr && *new_ptr == ' ')
                new_ptr++;
            /* if now pointing at non-lowercase, with previous char not
               being uppercase (rule out "U.S. Congress") then new sentence */
            if (new_ptr < end_ptr &&
                ! (isupper (*(ptr-1))) &&
                ! (islower (*new_ptr))) {
                ptr = new_ptr - 1;
                if (ip->sent_flag) {
                    if (UNDEF == new_section (ip,
                                              section->section_id,
                                              start_ptr - doc_text,
                                              ptr + 1 - doc_text))
                        return (UNDEF);
                    start_ptr = ptr+1;
                }
                new_sentence = 1;
            }
            break;

            /* Whitespace determining end of paragraph.
               Check to see if whitespace includes 2 '\n',
               Or, if a new_sentence then a '\n' followed by any whitespace
               indicates new para.
               */
          case '\n':
            new_ptr = ptr+1;
            if (new_ptr >= end_ptr || ! (isspace (*new_ptr)))
                break;
            if (new_sentence) {
                if (ip->para_flag) {
                    if (UNDEF == new_section (ip,
                                              section->section_id,
                                              start_ptr - doc_text,
                                              ptr + 1 - doc_text))
                        return (UNDEF);
                    start_ptr = ptr+1;
                }
            }
            else {
                while (new_ptr < end_ptr &&
                       isspace (*new_ptr) &&
                       *new_ptr != '\n')
                    new_ptr++;
                if (new_ptr < end_ptr && *new_ptr == '\n') {
                    /* Have found new paragraph.  Skip over any additional */
                    /* empty lines */
                    while (new_ptr < end_ptr && *new_ptr == '\n')
                        new_ptr++;
                    ptr = new_ptr - 1;
                    if (ip->para_flag) {
                        if (UNDEF == new_section (ip,
                                                  section->section_id,
                                                  start_ptr - doc_text,
                                                  ptr + 1 - doc_text))
                            return (UNDEF);
                        start_ptr = ptr+1;
                    }
                }
            }
            break;
          case ' ':
          case '\t':
            break;
          default:
            new_sentence = 0;
            break;
        }
    }
    if (UNDEF == new_section (ip,
                              section->section_id,
                              start_ptr - doc_text,
                              ptr - doc_text))
        return (UNDEF);
    return (1);
}


static int
new_section (ip, id, begin_section, end_section)
STATIC_INFO *ip;
char id;
int begin_section;
int end_section;
{
    if (ip->num_sections >= ip->max_sections) {
        ip->max_sections += ip->max_sections;
        if (NULL == (ip->sections = (SM_DISP_SEC *)
                     realloc ((char *) ip->sections,
                              (unsigned) ip->max_sections *
                              sizeof (SM_DISP_SEC))))
            return (UNDEF);
    }

    ip->sections[ip->num_sections].section_id = id;
    ip->sections[ip->num_sections].begin_section = begin_section;
    ip->sections[ip->num_sections].end_section = end_section;
    ip->num_sections++;
    return (1);
}

