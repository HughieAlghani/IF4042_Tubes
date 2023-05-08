#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libinter/inter_util.c,v 11.0 1992/07/21 18:21:45 chrisb Exp $";
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
#include "sm_display.h"
#include "tr_vec.h"
#include "vector.h"
#include "docindex.h"
#include "inter.h"
#include "context.h"
#include "section.h"

static PROC_INST sect_pre;           /* Section preparser */
static PROC_INST para_pre;           /* Paragraph preparser */
static PROC_INST sent_pre;           /* Sentence preparser */
static char *textloc_file;
static long textloc_mode;

static SPEC_PARAM spec_args[] = {
    "inter.sect.preparse",       getspec_func,    (char *) &sect_pre.ptab,
    "inter.para.preparse",       getspec_func,    (char *) &para_pre.ptab,
    "inter.sent.preparse",       getspec_func,    (char *) &sent_pre.ptab,
    "inter.doc.textloc_file",     getspec_dbfile,     (char *) &textloc_file,
    "inter.doc.textloc_file.rmode", getspec_filemode, (char *) &textloc_mode,
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);


/* Used for convert_partid if partsmap is there. */
static char *partsmap_file;
static long partsmap_mode;

static SPEC_PARAM partsmap_spec_args[] = {
    "inter.parts.map_file",	 getspec_dbfile,   (char *) &partsmap_file,
    "inter.parts.map_file.rmode",getspec_filemode, (char *) &partsmap_mode
    };
static int num_partsmap_spec_args = sizeof(partsmap_spec_args) /
    sizeof(*partsmap_spec_args);

static int parts_map_fd = UNDEF;


static int pp_veclist_inst;
static int did_vec_inst;
static int pp_pp_para_inst;
static int pp_pp_sent_inst;
static int textloc_fd;

static void copy_pp();
static void convert_partid();
static int num_inst = 0;

int
init_inter_util (spec, unused)
SPEC *spec;
char *unused;
{
    CONTEXT old_context;

    if (num_inst > 0) {
        num_inst++;
        return (num_inst);
    }
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    /* Open Textloc file to get file location and section offsets for did */
    if (UNDEF == (textloc_fd = open_textloc (textloc_file, textloc_mode)))
        return (UNDEF);

    /* Prepare for indexing sections of docs, if desired */
    old_context = get_context();
    set_context (CTXT_DOC);
    if (UNDEF == (sect_pre.inst = sect_pre.ptab->init_proc (spec, NULL)) ||
        UNDEF == (para_pre.inst = para_pre.ptab->init_proc (spec, NULL)) ||
        UNDEF == (sent_pre.inst = sent_pre.ptab->init_proc (spec, NULL)))
        return (UNDEF);
    if (UNDEF == (pp_veclist_inst = init_pp_veclist (spec, (char *) NULL)))
        return (UNDEF);
    if (UNDEF == (did_vec_inst = init_did_vec (spec, (char *) NULL)))
        return (UNDEF);
    set_context (CTXT_PARA);
    if (UNDEF == (pp_pp_para_inst = init_pp_pp_parts (spec, (char *) NULL)))
        return (UNDEF);
    set_context (CTXT_SENT);
    if (UNDEF == (pp_pp_sent_inst = init_pp_pp_parts (spec, (char *) NULL)))
        return (UNDEF);
    set_context (old_context);

    /*
     * See if we can do parts-map lookup.  We do it separately because
     * we don't want do die if the values aren't there.
     */
    if (UNDEF == lookup_spec (spec, &partsmap_spec_args[0],
			      num_partsmap_spec_args) ||
	    ! VALID_FILE( partsmap_file ))
	clr_err();
    else
	if (UNDEF == (parts_map_fd=open_vector(partsmap_file,partsmap_mode)))
	    return UNDEF;

    return (0);
}

int
close_inter_util (inst)
int inst;
{
    if (--num_inst > 0)
        return (0);

    if (UNDEF == close_textloc (textloc_fd))
        return (UNDEF);

    if (UNDEF == sect_pre.ptab->close_proc (sect_pre.inst) ||
        UNDEF == para_pre.ptab->close_proc (para_pre.inst) ||
        UNDEF == sent_pre.ptab->close_proc (sent_pre.inst) ||
        UNDEF == close_pp_veclist (pp_veclist_inst) ||
        UNDEF == close_did_vec (did_vec_inst) ||
        UNDEF == close_pp_pp_parts (pp_pp_para_inst) ||
        UNDEF == close_pp_pp_parts (pp_pp_sent_inst))
        return (UNDEF);

    if (parts_map_fd != UNDEF &&
	UNDEF == close_vector( parts_map_fd ))
	return UNDEF;

    return (0);
}

int
inter_get_textloc (did, textloc)
long did;
TEXTLOC *textloc;
{
    textloc->id_num = did;
    if (1 != seek_textloc (textloc_fd, textloc) ||
        1 != read_textloc (textloc_fd, textloc))
        return (UNDEF);

    return (1);
}

int
inter_get_pp_sect (did, sect_id, preparser, pp)
long did;
long sect_id;
PROC_INST *preparser;
SM_INDEX_TEXTDOC *pp;
{
    TEXTLOC textloc;

    if (UNDEF == inter_get_textloc (did, &textloc) ||
        UNDEF == preparser->ptab->proc (&textloc, pp, preparser->inst))
        return (UNDEF);

    if (sect_id <= 0)
        return (1);

    /* Want only section sect_id */
    if (sect_id >= pp->mem_doc.num_sections)
        return (UNDEF);
    pp->mem_doc.num_sections = 1;
    pp->mem_doc.sections = &pp->mem_doc.sections[sect_id];
    return (1);
}

/* Parse a user supplied docid with possible sentid, paraid.  Input of form
   [0-9]*[.[SPsp][0-9]*][.[SPsp][0-9]*]
*/
int 
inter_get_docid (ptr, docid, sectid, paraid, sentid)
char *ptr;
long *docid, *sectid, *paraid, *sentid;
{
    *docid = *sectid = *paraid = *sentid = -1;

    if (ptr == NULL)
        return (0);

    if (*ptr == '(')
	convert_partid( &ptr, docid );
    else {
	*docid = atol (ptr);
	while (*ptr && isdigit (*ptr))
	    ptr++;
    }

    while (*ptr == '.') {
        ptr++;
        switch (*ptr) {
          case 'C':
          case 'c':
            /* Found sentence */
            ptr++;
            *sectid = atol (ptr);
            while (*ptr && isdigit (*ptr))
                ptr++;
            break;
          case 'S':
          case 's':
            /* Found sentence */
            ptr++;
            *sentid = atol (ptr);
            while (*ptr && isdigit (*ptr))
                ptr++;
            break;
          case 'P':
          case 'p':
            /* Found paragraph */
            ptr++;
            *paraid = atol (ptr);
            while (*ptr && isdigit (*ptr))
                ptr++;
            break;
          default:
            /* Illegal specifier.  Just return */
            return (0);
        }
    }
    return (1);
}

/*
 * The docnumber looks like "(100.5).s5" or somesuch.  We convert
 * the part in parens to its local meaning using the parts map.
 * We return its local document number in docid, and set ptr to
 * point to the character after the close paren.  On error, return
 * docid set to 0, and set ptr pointing to the trailing null.
 * On entry, ptr points to the open paren.
 */
static void
convert_partid( passed_ptr, docid )
char **passed_ptr;
long *docid;
{
    char *ptr = *passed_ptr;
    long outside_docid;
    long partnum;
    VEC vec;

    if ( UNDEF != parts_map_fd ) {

	ptr++;  /* skip paren */
	outside_docid = atol( ptr );
	while ( *ptr && isdigit (*ptr) )
	    ptr++;

	if ( *ptr == '.' ) {      /* format okay; get partnum */
	    ptr++;
	    partnum = atol( ptr );
	    while ( *ptr && isdigit (*ptr) )
		ptr++;

	    if ( partnum>=0 && *ptr == ')' ) {   /* format okay; convert it */
		ptr++;

		vec.id_num = outside_docid;
		if (UNDEF != seek_vector( parts_map_fd, &vec ) &&
		    UNDEF != read_vector( parts_map_fd, &vec ))

		    if ( partnum < vec.num_conwt ) {  /* all valid */
			*docid = vec.con_wtp[partnum].con;
			*passed_ptr = ptr;
			return;
		    }
	    }
	}
    }
    /* One thing or another went wrong; return the error status */
    while (*ptr) ptr++;
    *passed_ptr = ptr;
    *docid = 0;
    return;
}

/* vec_list must initially point to a valid vec_list of length 1, where
   vec_list[0] must be a valid vector */
int
inter_get_sect_veclist (command, vec_list)
char *command;
VEC_LIST *vec_list;
{
    long docid, sectid, paraid, sentid;
    SM_INDEX_TEXTDOC pp;
        
    if (1 != inter_get_docid (command, &docid,&sectid,&paraid, &sentid) ||
        docid <= 0) 
        return (UNDEF);

    vec_list->id_num = docid;

    if (sectid == UNDEF && paraid == UNDEF && sentid == UNDEF) {
        /* Indexing single document */
        if (1 != did_vec (&docid, &vec_list->vec[0], did_vec_inst))
            return (UNDEF);
        vec_list->num_vec = 1;
        return (1);
    }

    if (UNDEF == inter_get_sect_pp_all (docid, sectid, paraid, sentid, &pp))
        return (UNDEF);

    if (UNDEF == pp_veclist (&pp, vec_list, pp_veclist_inst))
        return (UNDEF);
    
    return (1);
}


int 
inter_get_sect_pp_all (docid, sectid, paraid, sentid, pp)
long docid, sectid, paraid, sentid;
SM_INDEX_TEXTDOC *pp;
{
    SM_INDEX_TEXTDOC temp_pp;
    
    if (sectid == UNDEF && paraid == UNDEF && sentid == UNDEF) {
        /* showing single document, no sections */
        return (UNDEF);
    }
    
    if (sectid >= 0) {
        if (UNDEF == inter_get_pp_sect (docid, sectid, &sect_pre, pp))
            return (UNDEF);
    }

    if (paraid >= 0) {
        if (sectid >= 0) {
            /* Must get para from already preparsed section */
            copy_pp (pp, &temp_pp);
            if (UNDEF == pp_pp_parts (&temp_pp, pp, pp_pp_para_inst))
                return (UNDEF);
            if (paraid >= pp->mem_doc.num_sections)
                return (UNDEF);
            if (paraid > 0) {
                pp->mem_doc.num_sections = 1;
                pp->mem_doc.sections = &pp->mem_doc.sections[paraid];
            }
        }
        else {
            /* get para from original text */
            if (UNDEF == inter_get_pp_sect (docid, paraid, &para_pre, pp))
                return (UNDEF);
        }
    }

    if (sentid >= 0) {
        if (sectid >= 0 || paraid >= 0) {
            /* Must get sent from already preparsed section or paragraph */
            copy_pp (pp, &temp_pp);
            if (UNDEF == pp_pp_parts (&temp_pp, pp, pp_pp_sent_inst))
                return (UNDEF);
            if (sentid >= pp->mem_doc.num_sections)
                return (UNDEF);
            if (sentid > 0) {
                pp->mem_doc.num_sections = 1;
                pp->mem_doc.sections = &pp->mem_doc.sections[sentid];
            }
        }
        else {
            /* get sent from original text */
            if (UNDEF == inter_get_pp_sect (docid, sentid, &sent_pre, pp))
                return (UNDEF);
        }
    }

    return (1);
}
        

static void
copy_pp (in_pp, out_pp)
SM_INDEX_TEXTDOC *in_pp;
SM_INDEX_TEXTDOC *out_pp;
{
    out_pp->doc_text = in_pp->doc_text;
    out_pp->id_num = in_pp->id_num;

    out_pp->textloc_doc.id_num = in_pp->textloc_doc.id_num;
    out_pp->textloc_doc.begin_text = in_pp->textloc_doc.begin_text;
    out_pp->textloc_doc.end_text = in_pp->textloc_doc.end_text;
    out_pp->textloc_doc.doc_type = in_pp->textloc_doc.doc_type;
    out_pp->textloc_doc.file_name = in_pp->textloc_doc.file_name;
    out_pp->textloc_doc.title = in_pp->textloc_doc.title;

    out_pp->mem_doc.id_num = in_pp->textloc_doc.id_num;
    out_pp->mem_doc.file_name = in_pp->mem_doc.file_name;
    out_pp->mem_doc.title = in_pp->mem_doc.title;
    out_pp->mem_doc.sections = in_pp->mem_doc.sections;
    out_pp->mem_doc.num_sections = in_pp->mem_doc.num_sections;
}
