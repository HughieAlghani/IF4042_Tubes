#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/liblocal/libindexing/parts_coll_fw.c,v 11.0 1992/07/21 18:22:07 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Add text of parts of already existing collection (F&W special version)
 *1 index.top.parts_coll_fw
 *2 parts_coll_fw (unused1, unused2, inst)
 *3   char *unused1;
 *3   char *unused2;
 *3   int inst;
 *4 init_parts_coll_fw (spec, unused)
 *5   "parts.doc.textloc_file"
 *5   "parts.doc.textloc_file.rmode"
 *5   "parts.text_file"
 *5   "parts.text_file.rwcmode"
 *5   "parts.map_section"
 *5   "parts.map_file"
 *5   "parts.map_file.rwcmode"
 *5   "parts.preparse"
 *5   "index.trace"
 *4 close_parts_coll_fw (inst)
 *7 Toplevel indexing procedure for F&W encyclopedia only.
 *7 Take each indexed document given by textloc_file and break it into parts,
 *7 with sections determined by parts.preparse.  Write each part out 
 *7 in SMART form (see smart_pp.c) that can later be indexed as a complete doc.
 *7 Store the new docs in text_file, and a map from 
 *7 the original document id to the new part ids in map_file (the
 *7 map is in vector form itself)
 *7 The "map_section" title is constructed from the contents of the
 *7 section, up to the first blank line.  It is ellided where necessary
 *7 to make it of reasonable length.
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

/* A title-width to shoot for; longer titles will be ellided */
#define TITLE_WIDTH 63

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
static int build_title();

int
init_parts_coll_fw (spec, unused)
SPEC *spec;
char *unused;
{
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_parts_coll_fw");

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

    PRINT_TRACE (2, print_string, "Trace: leaving init_parts_coll_fw");
    return (0);
}

int
parts_coll_fw (unused1, unused2, inst)
char *unused1;
char *unused2;
int inst;
{
    char temp_buf[PATH_LEN+100];
    TEXTLOC text_display;
    SM_INDEX_TEXTDOC pp;

    SM_BUF text_buf, old_buf, title_buf, main_title_buf;

    long part_docid = 1;

    long num_parts_list = 0;
    VEC parts_vec;
    long ctype_len = 1;
    long i;

    PRINT_TRACE (2, print_string, "Trace: entering parts_coll_fw");
    text_buf.size = old_buf.size = title_buf.size = main_title_buf.size = 0;

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
                main_title_buf.buf = pp.doc_text +
                    pp.mem_doc.sections[i].begin_section;
                main_title_buf.end =  pp.mem_doc.sections[i].end_section -
                    pp.mem_doc.sections[i].begin_section;
                if (main_title_buf.end > 100)
                    main_title_buf.end = 100;
            }
        }

        /* Write out each section as a separate doc */
        for (i = 0; i < pp.mem_doc.num_sections; i++) {
            /* Construct new doc text which will have two sections.
             * First new section will be given by
             * map_section and will contain mapping info plus the old
             * title of the full document.  This section will presumably
             * not be indexed.  The second new section will be a copy
             * of the current old section.
             */
            text_buf.end = 0;
            old_buf.buf = &pp.doc_text[pp.mem_doc.sections[i].begin_section];
            old_buf.end = pp.mem_doc.sections[i].end_section -
                pp.mem_doc.sections[i].begin_section;
            (void) sprintf (temp_buf, ".i %d %d %s\n.%c\n",
                            pp.textloc_doc.begin_text,
                            pp.textloc_doc.end_text,
                            pp.textloc_doc.file_name,
                            map_sectionid[0] );
            if (UNDEF == add_buf_string (temp_buf, &text_buf))
		return UNDEF;

	    /* Use the article title for everything except the body,
	     * which can generate its own title.
	     */
	    if ( pp.mem_doc.sections[i].section_id != 'b' ||
		 old_buf.end <= 1) {
		if (UNDEF == build_title( &title_buf, &main_title_buf,
					 text_display.id_num, i ))
		    return UNDEF;
	    }
	    else {
		if (UNDEF == build_title( &title_buf, &old_buf,
					 text_display.id_num, i ))
		    return UNDEF;
	    }
	    if (UNDEF == add_buf (&title_buf, &text_buf))
		    return (UNDEF);

            (void) sprintf (temp_buf, "\n.%c\n",
                            pp.mem_doc.sections[i].section_id);
            if (UNDEF == add_buf_string (temp_buf, &text_buf) || 
                UNDEF == add_buf (&old_buf, &text_buf) ||
		UNDEF == add_buf_string ("\n", &text_buf))
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

    PRINT_TRACE (2, print_string, "Trace: leaving parts_coll_fw");
    return (1);
}


int
close_parts_coll_fw (inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_parts_coll_fw");

    if (UNDEF == pp_parts.ptab->close_proc (pp_parts.inst))
        return (UNDEF);

    if (UNDEF == close (text_fd) ||
        UNDEF == close_vector (map_fd) ||
        UNDEF == close_textloc (textloc_fd))
        return (UNDEF);


    PRINT_TRACE (2, print_string, "Trace: leaving close_parts_coll_fw");
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


/******************************************************************
 *
 * Build a title for this article.  The title is built by catenating
 * all lines up to the first blank line in the original text.  The
 * final length, though, is bounded by TITLE_WIDTH; if the length
 * would exceed that, portions of the title are ellided away.
 *
 * When elliding, we make great efforts to keep the first and last
 * lines of the title.
 *
 ******************************************************************/
static int
build_title( title_buf, text_buf, orig_docnum, part_num )
SM_BUF *title_buf;
SM_BUF *text_buf;
long orig_docnum;
long part_num;
{
    static char title[TITLE_WIDTH+1];	/* so we can return pointers to it */
    int total_len, num_len, chars_avail;
    int main_head_len, last_head_start;
    long i, pt;
    char *c, *ptr, *b_ptr;
    static char *buff = NULL;
    static long buff_len = 0;

    /* Build a place to store the title as we scan it */
    if (buff_len < text_buf->end + 1) {
	if (buff != NULL)
	    (void) free(buff);
	if (NULL == (buff = malloc( (unsigned) text_buf->end+1 )))
	    return UNDEF;
	buff_len = text_buf->end + 1;
    }

    (void) sprintf( title, "%d.%d %n", orig_docnum, part_num, &num_len );
    chars_avail = TITLE_WIDTH - num_len;

    /* Scan the buffer to find the title and some information about it
     * We also trip trailing periods from the headings since they
     * waste space in the display.
     */
    total_len = -1;
    main_head_len = 0;
    last_head_start = 0;
    for (i=0, ptr=text_buf->buf, b_ptr=buff; i<text_buf->end;
	                                           i++, ptr++) {
	if (*ptr=='\n') {
	    while (b_ptr>buff && *(b_ptr-1)=='.') b_ptr--;

	    if (main_head_len==0) main_head_len = b_ptr - buff;

	    if (i<text_buf->end-1 && *(ptr+1) != '\n')
		last_head_start = (b_ptr-buff)+1;
	    else {
		total_len = (b_ptr-buff);
		break;
	    }
	}
	*b_ptr++ = *ptr;
    }
    if (total_len==-1)
	total_len = b_ptr - buff;

    /* Here, eveything fits without any ellision. */
    if ( total_len <= chars_avail )
	(void) sprintf( title+num_len, "%.*s", total_len, buff );

    /* Here there's only one heading and it doesn't quite fit */
    else if (last_head_start == 0)
	(void) sprintf( title+num_len, "%.*s...", chars_avail-3,
		       buff );

    /*
     * Ellide out a few of the middle subheadings.  We only ellide
     * out complete subheadings.  Note that subheadings are here
     * delimited by \n occurrences.
     */
    else {
	chars_avail -= total_len - last_head_start + 1;  /* for `/last' */
	chars_avail -= main_head_len + 1;		 /* for `main/' */
	chars_avail -= 3;				 /* for `...' */

	/* See how many trailing headings can be included */
	pt = last_head_start - 2; /* char before '\n' */
	while ( chars_avail > 0 ) {
	    if ( buff[pt] != '\n' )
		pt--;
	    else {
		if ( last_head_start - pt + 1 <= chars_avail ) {
		    chars_avail -= last_head_start - pt + 1;
		    last_head_start = pt + 1;
		    pt--;
		}
		else
		    break;
	    }
	}

	/* On a whim, perhaps we can squeeze in another at the head? */
	pt = main_head_len + 1; /* char after '\n' */
	while ( chars_avail > 0 ) {
	    if ( buff[pt] != '\n' )
		pt++;
	    else {
		if ( pt - main_head_len <= chars_avail ) {
		    chars_avail -= pt - main_head_len;
		    main_head_len = pt++;
		}
		else
		    break;
	    }
	}

	/* Better make sure it'll squeeze in at all; lop off
	 * in an attempt to even things out.
	 */
	while ( chars_avail < 0 ) {
	    int last_head_len = total_len - last_head_start;

	    if ( main_head_len > last_head_len ) {
		main_head_len--;
		chars_avail++;
	    }
	    else if ( main_head_len < last_head_len ) {
		last_head_start++;
		chars_avail++;
	    }
	    else {
		main_head_len--;
		last_head_start++;
		chars_avail += 2;
	    }
	}

	(void) sprintf( title+num_len, "%.*s...%.*s",
		       main_head_len+1, buff,
		       total_len - last_head_start+1,
		       &buff[last_head_start-1] );
    }

    for (c=title+num_len; *c != '\0'; c++)
	if ( *c == '\n' ) *c = '/';

    title_buf->size = TITLE_WIDTH;
    title_buf->buf = title;
    title_buf->end = strlen( title );

    return 0;
}


