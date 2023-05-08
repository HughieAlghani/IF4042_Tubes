#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/liblocal/libindexing/pp_fw.c,v 11.0 1992/07/21 18:22:05 chrisb Exp $";
#endif
/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 pre-parser for a Cornell version of  F&W encyclopedia (1-way rels)
 *1 local.index.preparse.fw
 *2 pp_fw (input_doc, output_doc, inst)
 *3   TEXTLOC *input_doc;
 *3   SM_INDEX_TEXTDOC *output_doc;
 *3   int inst;
 *4 init_pp_fw (spec_ptr, unused)
 *5   "index.preparse.trace"
 *5   "index.preparse.doc_loc"
 *5   "local.index.preparse.fw.docid_map"
 *5   "local.index.preparse.fw.docid_map.rwmode"
 *5   "local.index.preparse.fw.docid_map_size"
 *5   "local.index.preparse.fw.rel_file"
 *5   "local.index.preparse.fw.rel_file.rwcmode"
 *5   "index.preparse.query_loc"
 *5   "local.index.preparse.fw.collect_geninfo"  (boolean)
 *5   "local.index.preparse.fw.geninfo_secid"
 *5   "local.index.preparse.fw.log_warnings" (boolean)
 *5   "local.index.preparse.fw.xref_file"
 *5   "local.index.preparse.fw.xref_file.rwcmode"
 *4 close_pp_fw (inst)

 *6 Uses global_context to tell if working on doc or query (CTXT_DOC, 
 *6 CTXT_QUERY), and if doing original indexing on doc (CTXT_INDEXING_DOC)

 *7 Normal preparser operations on a collection.
 *7 Return the preparsed next document (in output_doc) to caller.  If input_doc
 *7 is NULL, next document comes from the list of document files given in
 *7 in_fd (or from current doc file opened by last calls to pp_line).
 *7 If input_doc is non-NULL, ignore the list of files, and assume that
 *7 input_doc contains exactly one complete document to be preparsed.
 *7 If CTXT_INDEXING_DOC, then as well as preparsing the document, also need
 *7 to store a map of article title to docid in "docid_map", and to keep track
 *7 of articles relevant to current article in "rel_file".  A second doc 
 *7 is relevant to the first article if the first article cites the second.
 *7 Returns 1 if found doc to preparse, 0 if no more docs, UNDEF if error
 *7
 *7 If "collect_geninfo" is false, then the general information sections
 *7 such as volume, date, and so on, are broken into their own sections
 *7 in the output document.  If false, they are all bundled into a
 *7 single section with the name "geninfo_secid".
 *7
 *7 If "log_warnings" is true, then warning errors are displayed on
 *7 standard output.  Otherwise, the warning errors are supressed.
 *7 Warning include bad SGML codes, bad "&xxx;" attributes, and
 *7 other minor problems.  Actually, as a special feature, the "can't
 *7 translate &xxx; code" errors are only included if "log_warnings" has
 *7 a value greater than 1 (cause it happens a LOT).
 *7
 *7 If an article is determined to be only a cross reference (if it is
 *7 one paragraph, and only a cross ref in that paragraph), then its
 *7 F&W article ID and the Smart doc number are written out to the
 *7 file "xref_file".  This is only done if CTXT_INDEXING_DOC.

***********************************************************************/

#include <ctype.h>
#include <string.h>
extern int strcasecmp();  /* not in string.h--and strstr isn't in strings.h.  Dumb. */
#include "common.h"
#include "param.h"
#include "smart_error.h"
#include "docindex.h"
#include "functions.h"
#include "spec.h"
#include "trace.h"
#include "context.h"
#include "dict.h"
#include "rr_vec.h"
#include "io.h"

#include "fw.h"

static FILE *in_fd;                /* File descriptor from which to read
                                      list of files to parse */
static int  in_file_flag;          /* Set if currently reading a doc within
                                      a file (0 indicates must get new
                                      filename from in_fd). */

static char *doc_file;             /* If we're currently reading a doc, the
                                      filename we're reading from */
static char *doc_line;             /* The current line from doc_file */
static long doc_offset;            /* byte offset of doc_line within doc_file*/
static long start_doc_offset;      /* doc_offset at beginning of current word*/
static FILE *f_in;                 /* File descriptor for doc_file */
static FILE *xref_fd;		   /* for list of xref-only articles */

static Boolean suppress_blank_line = FALSE;
                                   /* set to suppress blank line before
				    * the next title */
static parsing_indiv_doc = 0;       /* True if we are to parse one individual
                                       doc handed to fw_pp */
static long end_doc_offset;         /* If parsing_indiv_doc, the end location
                                       of document being given */

static char *buf;                   /* In core copy of current doc */
static char *buf_end;               /* End of allocated space for buf */
static char *buf_ptr;               /* Current position within buf */

#define pp_putbuf(c) { \
    unsigned cur_size; \
    if (buf_ptr >= buf_end) { \
        cur_size = buf_end - buf; \
        if (NULL == (buf = realloc (buf, cur_size + DOC_SIZE))) { \
            set_error (errno, "buf malloc", "preparse"); \
            exit (2); \
        } \
        buf_end = buf + cur_size + DOC_SIZE; \
        buf_ptr = buf + cur_size; \
    } \
    *buf_ptr++ = c; \
}

/* Increment to increase size of buf by */
#define DOC_SIZE 16100

static long num_sections;           /* Number of sections seen in doc */
static long current_id_num;          /* Current Doc id */
static SM_DISP_SEC *mem_disp;        /* Section occurrence info for doc giving
                                        byte offsets within buf */

static unsigned max_num_sections;    /* max current storage reserved for
                                        and mem_disp */


/* Variables for mapping titles to docids */
static int dict_fd = UNDEF;

/* Increment to increase size of section storage by */
#define DISP_SEC_SIZE 60



static void start_new_section(), set_title_level(), set_title_level_up();
static int pp_put_section(), pp_put_end_section(), pp_put_begin_section();
static void pp_copy(), pp_in_copy();
static int init_docid_map(), add_id();
static int init_add_rel_info(), construct_rels();
static long lookup_doc_map();
static void add_rel_info();
static void getword(), getline(), scrub_id(), process_xref(), scrub_title();
static void set_sgml_code();
static int dump_xrefs();
static char *translate();
static int parse_sgml();

static char	curr_article_id[MAX_LINE_LEN];
static char	curr_vol[MAX_LINE_LEN];
static char	curr_page[MAX_LINE_LEN];
static char	curr_date[MAX_LINE_LEN];
static char	curr_category[MAX_LINE_LEN];
static char	curr_subcat[MAX_LINE_LEN];
static char	curr_obs[MAX_LINE_LEN];
static char	curr_bio[MAX_LINE_LEN];

/* SHOULD MALLOC THESE */
static int	curr_subjs;
static char	curr_subj[MAX_SUBJS][MAX_LINE_LEN];

/* SHOULD MALLOC THESE */
static int	curr_xrefs;
static Xref	curr_xref[MAX_XREFS];
static short	close_paren_pending = 0;

/* SHOULD MALLOC THESE */
static int	curr_title_level;
static char	curr_titles[5][MAX_LINE_LEN]; /* titles for sec..secc */
#define TL_main 0
#define TL_sec 1
#define TL_seca 2
#define TL_secb 3
#define TL_secc 4


static char *pp_infile;            /* File in which to read names of input
                                      files to be pre_parsed. */
static char *docid_map_file;       /* File (in dict format) in which to
                                      store map from character id to docid */
static long docid_map_mode;        /* mode to open docid_map_file */
static long docid_map_size;        /* Initial size for docid dict */
static char *rel_file;             /* File (in rr format) in which to
                                      store relevance info, assuming
                                      all references indicate relevance
                                      link (both ways) */
static long rel_mode;

static long collect_geninfo;	   /* boolean: should general information
				      be collected into a common section? */
static char *geninfo_secid;	   /* where to put it, if so */
static long log_warnings;	   /* display warning messages? */

static char *xref_file;		   /* Place to put list of articles that are */
static long xref_file_mode;	   /* only cross references. */

static SPEC_PARAM pp[] = {
    TRACE_PARAM ("index.preparse.trace")
    "index.preparse.doc_loc",  getspec_string, (char *) &pp_infile,
    "local.index.preparse.fw.docid_map", getspec_dbfile, (char *) &docid_map_file,
    "local.index.preparse.fw.docid_map.rwmode", getspec_filemode, (char *) &docid_map_mode,
    "local.index.preparse.fw.docid_map_size", getspec_long, (char *) &docid_map_size,
    "local.index.preparse.fw.rel_file",  getspec_dbfile, (char *) &rel_file,
    "local.index.preparse.fw.rel_file.rwcmode", getspec_filemode, (char *)&rel_mode,
    "index.preparse.fw.collect_geninfo", getspec_long,(char *)&collect_geninfo,
    "index.preparse.fw.geninfo_secid", getspec_string, (char *)&geninfo_secid,
    "local.index.preparse.fw.log_warnings", getspec_long, (char *)&log_warnings,
    "local.index.preparse.fw.xref_file", getspec_dbfile, (char *) &xref_file,
    "local.index.preparse.fw.xref_file.rwcmode", getspec_filemode, (char *) &xref_file_mode,
    };
static int num_pp = sizeof (pp) / sizeof (pp[0]);

static SPEC_PARAM qpp[] = {
    TRACE_PARAM ("index.preparse.trace")
    "index.preparse.query_loc",  getspec_string, (char *) &pp_infile
    };
static int num_qpp = sizeof (qpp) / sizeof (qpp[0]);

static Word word;

static int num_init = 0;     /* Number of times initialized */
static int indexing_doc_flag = 0;

int
init_pp_fw  (spec_ptr, unused)
SPEC *spec_ptr;
char *unused;
{

    /* BUG: Need to handle multiple instantiations.  For now just ignore */
    if (num_init++) {
        PRINT_TRACE (2, print_string, "Trace: entering/leaving init_pp_fw");
        return (0);
    }

    if (check_context(CTXT_DOC)) {
        if (UNDEF == lookup_spec (spec_ptr, &pp[0], num_pp)) {
            return (UNDEF);
        }
    }
    else if (check_context (CTXT_QUERY)) {
        if (UNDEF == lookup_spec (spec_ptr,
                                  &qpp[0],
                                  num_qpp)) {
            return (UNDEF);
        }
    }
    else {
        set_error (SM_ILLPA_ERR, "Illegal context", "init_pp_fw");
        return (UNDEF);
    }

    PRINT_TRACE (2, print_string, "Trace: entering init_pp_fw");

    if (NULL == (doc_file = malloc (PATH_LEN)) ||
        NULL == (doc_line = malloc (PATH_LEN)) ||
        NULL == (mem_disp =  (SM_DISP_SEC *) malloc (DISP_SEC_SIZE *
                             sizeof (SM_DISP_SEC))) ||
        NULL == (buf = malloc (DOC_SIZE))) {
        return (UNDEF);
    }

    max_num_sections = DISP_SEC_SIZE;
    buf_end = buf + DOC_SIZE;
    buf_ptr = buf;
    in_file_flag = 0;
    f_in = NULL;

    /* Unlike other pre-parsers, cannot accept list of files as
       stdin since need to go through list twice. */
    /* Open file giving list of files to be parsed */
    if (NULL == pp_infile ||
        pp_infile[0] == '\0' ||
        0 == strcmp (pp_infile, "-")) {
        in_fd = stdin;
    }
    else {
        if (NULL == (in_fd = fopen (pp_infile, "r"))) {
            set_error (errno, pp_infile, "init_pp_fw");
            return (UNDEF);
        }
    }

    /* If indexing docs, initialize proc to get relevance info */
    if (check_context (CTXT_INDEXING_DOC)) {
        indexing_doc_flag = 1;
        if (UNDEF == init_add_rel_info ())
            return (UNDEF);
        if (UNDEF == init_docid_map ())
            return (UNDEF);

	if (NULL == (xref_fd = fopen( xref_file, "w" ))) {
	    set_error(errno, xref_file, "init_pp_fw");
	    return UNDEF;
	}
    }
    else {
	xref_fd = NULL;
        indexing_doc_flag = 0;
        /* open docid_map for reading */
        if (dict_fd == UNDEF &&
            UNDEF == (dict_fd = open_dict (docid_map_file, docid_map_mode)))
            return (UNDEF);
    }

    PRINT_TRACE (2, print_string, "Trace: leaving init_pp_fw");

    return (0);
}

/* Return the preparsed next document (in output_doc) to caller.  If input_doc
   is NULL, next document comes from the list of document files given in
   in_fd (or from current doc file opened by last calls to pp_line).
   If input_doc is non-NULL, ignore the list of files, and assume that
   input_doc contains exactly one complete document to be preparsed.
*/
int
pp_fw (input_doc, output_doc, inst)
TEXTLOC *input_doc;
SM_INDEX_TEXTDOC *output_doc;
int inst;
{
    int previous_parse;         /* flag indicating continuation from
                                 previous parse */

    PRINT_TRACE (2, print_string, "Trace: entering pp_fw");

    current_id_num = output_doc->id_num;
    output_doc->mem_doc.id_num = output_doc->textloc_doc.id_num = current_id_num;

    previous_parse = (input_doc == NULL && in_file_flag);

    if (input_doc != NULL) {
        /* Open doc_file */
        (void) strcpy (doc_file, input_doc->file_name);
        doc_offset = input_doc->begin_text;
        end_doc_offset = input_doc->end_text;
        parsing_indiv_doc = 1;
        if (NULL == (f_in = fopen (doc_file, "r")) ||
            -1 == fseek (f_in, doc_offset, 0)) {
            /* non-existent or empty file */
            /* Should be error? */
            return (0);
        }
        in_file_flag = 1;
    }
    else {
        while (! in_file_flag) {
            /* get next doc_file to be parsed */
            if (NULL == fgets (doc_file, PATH_LEN, in_fd)) {
                /* No more files to get. Done parsing. */
                return (0);
            }
            /* Remove trailing \n from doc_file */
            doc_file[strlen(doc_file) - 1] = '\0';
            
            /* Open doc_file */
            if (NULL == (f_in = fopen (doc_file, "r"))) {
                /* non-existent file */
                /* Should be error? */
                continue;
            }
            doc_offset = 0;
            in_file_flag = 1;
        }
    }
    /*
     * Get to the start of the first article in the input file.
     */
    if (! previous_parse) {
        word.Len = 0;
        getword( f_in, &word );
    }
    while ( word.Code != SYM_FW_ART && word.Code != SYM_EOF )
        getword( f_in, &word );
    if (word.Code == SYM_EOF) {
        /* empty file is an error */
        return (UNDEF);
    }

    /* At this point, word.Code is guaranteed to be SYM_FW_ART.
       Preparse this article, ending on SYM_EOF or start of next
       article.  If ending on SYM_EOF, set in_file_flag to 0; */
    if (UNDEF == parse_sgml (output_doc, PARSE_ENTIRE)) {
        return (UNDEF);
    }

    /* If indexing doc, store doc_title doc_id correspondence */
    if (indexing_doc_flag) {
        if (UNDEF == add_id (output_doc))
            return (UNDEF);
    }

    PRINT_TRACE (4, print_int_textdoc, output_doc);
    PRINT_TRACE (2, print_string, "Trace: leaving pp_fw");

    return (1);
}

int
close_pp_fw (inst)
int inst;
{
    if (--num_init) {
        PRINT_TRACE (2, print_string, "Trace: entering/leaving close_pp_fw");
        return (0);
    }

    PRINT_TRACE (2, print_string, "Trace: entering close_pp_fw");

    if (indexing_doc_flag) {
        if (UNDEF == construct_rels())
            return (UNDEF);
        if (dict_fd != UNDEF && UNDEF == close_dict (dict_fd)) 
            return (UNDEF);
        dict_fd = UNDEF;
    }

    if (xref_fd != NULL && -1 == fclose( xref_fd ))
	return UNDEF;

    free (doc_file);
    free (doc_line);
    free (buf);
    free ((char *) mem_disp);

    if (f_in != NULL && -1 == fclose (f_in))
        return (UNDEF);
    if (in_fd != NULL && -1 == fclose (in_fd))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: leaving close_pp_fw");

    return (0);
}


/******************************************************************
 *
 * Here is the layout of an F&W encyclopedia article:
 *
 * fw.art id=name, type=name(implied)
 *     	category
 *	subcat		Sub-category
 *	obs		Obscurity level
 *	bio		is it a bio?
 *	pubfm?
 *	    vol		
 *	    page	
 *	    mdt		last modification date
 *		mo?
 *		day?
 *		yr
 *	    subh+	Authorized subject heading
 *	    kw*		Article keywords
 *	bdy
 *	    name?	also <normname>, normalized form of name
 *	    ti+		Article title
 *	    media*
 *	    p+
 *	    (au,sec,seca,secb,secc)*
 *		Can be broken down; each starts with a title.
 *	bm?
 *
 * Emphasis codes: it, b, bi, un, scp, inf, sup
 * Text elements: xr, fo, fl, fr, div, rad, in, sum, au, quote, help, fn
 * Other things: ibmp - inline bitmap
 *		 li - lists
 * Cross references:  xr type=(bib, bio, see, sea, qv, com)
 *			 syntax=(sep, embed, parens, other)
 *			    x+ id=name, doc=cdata
 *
 * We consider an article to be a cross-ref only if it consists
 * of just one paragraph (<p>...</p>) which is a XR of type "see"
 * and syntax "sep" or "embed".
 *
 ******************************************************************/
static int
parse_sgml (output_doc, parse_level)
SM_INDEX_TEXTDOC *output_doc;
int parse_level;               /* parse_level = PARSE_ENTIRE
                                  indicates a single section for the body.
				  PARSE_SECT indicates each section
				  is a separate, um, section in the body.
                                  PARSE_PARA indicates each paragraph
                                  is a separate section in the body.
                                  PARSE_SENT indicates each sentence
                                  is a separate section in the body.
                                */
{
    long i, outlen;
    Boolean section_in_progress;
    int new_title_level;
    char temp[MAX_LINE_LEN];
    long para_count, text_count, xref_count;
    char list_letter = 0;

    /*
     * Signal the start of a new article.
     */
    output_doc->textloc_doc.title = NULL;
    output_doc->mem_doc.title = NULL;
    output_doc->mem_doc.file_name = NULL;

    if (UNDEF == pp_put_begin_section (output_doc,
                                       CODE_start,
                                       start_doc_offset,
                                       doc_file)) {
        return (UNDEF);
    }

    /*
     * The identifier is the stuff that follows the space in
     * this word.  We want that stored.
     */
    scrub_id( word.Text );
    if (UNDEF == pp_put_section (CODE_id)) {
        return (UNDEF);
    }
    (void) pp_copy (word.Text);
    pp_putbuf ('\n');
    (void) strncpy( curr_article_id, word.Text, MAX_LINE_LEN );
    
    *curr_vol = '\0';
    *curr_page = '\0';
    *curr_date = '\0';
    *curr_category = *curr_subcat = *curr_obs = *curr_bio = '\0';
    curr_subjs = 0;
    curr_xrefs = 0;

    getword( f_in, &word );
    while ( word.Code != SYM_BDY ) {
	switch (word.Code) {
	  case SYM_CATEGORY:
	    getline( f_in, &word, word.Code, curr_category );
	    break;
	  case SYM_SUBCATEGORY:
	    getline( f_in, &word, word.Code, curr_subcat );
	    break;
	  case SYM_OBSCURITY:
	    getline( f_in, &word, word.Code, curr_obs );
	    break;
	  case SYM_BIO:
	    getline( f_in, &word, word.Code, curr_bio );
	    break;

	  case SYM_PUBFM:
	    /*
	     * Get the publisher's front matter from the data
	     */
	    getword( f_in, &word );
	    while ( word.Code != SYM_PUBFM ) {
		switch (word.Code) {
		  case SYM_VOL:
		    getline( f_in, &word, SYM_VOL, curr_vol );
		    break;
            
		  case SYM_PAGE:
		    getline( f_in, &word, SYM_PAGE, curr_page );
		    break;
            
		  case SYM_MDT:
		    getword( f_in, &word );
		    *curr_date = '\0';
		    while ( word.Code != SYM_MDT ) {
			switch (word.Code) {
			  case DATESYM_MONTH:
			  case DATESYM_DAY:
			    getline( f_in, &word, word.Code, temp );
			    (void) strcat( curr_date, temp );
			    (void) strcat( curr_date, "/" );
			    break;
			    
			  case DATESYM_YEAR:
			    getline( f_in, &word, word.Code, temp );
			    (void) strcat( curr_date, temp );
			    break;
			    
			}
			getword( f_in, &word );
		    }
		    break;
		    
		  case SYM_SUBJ:
		    getline( f_in, &word, SYM_SUBJ, temp );
		    if ( curr_subjs == MAX_SUBJS ) {
			if (log_warnings)
			    (void) printf( "** Too many subjects in %s (%d); one skipped\n",
					  curr_article_id, current_id_num );
		    }
		    else {
			if ( strlen( temp ) > 0 )
			    (void) strcpy( curr_subj[ curr_subjs++ ], temp );
		    }
		    break;
		    
		  case SYM_SUBH: /* Just pretend they're subjects */
		    getline( f_in, &word, SYM_SUBH, temp );
		    if ( curr_subjs == MAX_SUBJS ) {
			if (log_warnings)
			    (void) printf("** Too many subjects in %s (%d); one skipped\n",
					  curr_article_id, current_id_num );
		    }
		    else {
			if ( strlen( temp ) > 0 ) {
			    (void) strcpy( curr_subj[curr_subjs], "(SUBH) ");
			    (void) strcat( curr_subj[curr_subjs++], temp );
			}
		    }
		    break;
		}
		getword( f_in, &word );
	    } /* end of PUBFM */
	} /* end of switch code */

	getword( f_in, &word );
    }

    /*
     * Now we're into the body of the text.
     */
    curr_title_level = TL_main;
    section_in_progress = FALSE;
    outlen = 0;
    para_count = xref_count = text_count = 0;
    while ( word.Code != SYM_EOF && (word.Code!=SYM_BDY || word.End!=TRUE )) {

	/*
	 * See if this word signals the start of a new section.  If
	 * so, we get out of here now.
	 */
        if ( word.End == FALSE && IS_SECTION_HDR( word.Code )) {
            if ( section_in_progress ) {
                if ( outlen > 0 ) {
                    pp_putbuf ('\n');
                    outlen = 0;
                }
                (void) dump_xrefs( &curr_xrefs, curr_xref );
                if (UNDEF == pp_put_end_section (output_doc,
                                                 CODE_end,
                                                 start_doc_offset))
                    return (UNDEF);
                return (1);
            }
	}           

#define PUT_GENINFO(data,code,describe) \
            if ( strlen( data ) > 0 ) { \
		if (!collect_geninfo) { \
		    if (UNDEF == pp_put_section (code)) \
			return (UNDEF); \
		} \
		else \
		    pp_copy( describe ); \
                pp_copy (data); \
                pp_putbuf ('\n'); \
		outlen = 0; \
            }
	/*
	 * If we're starting a new section, spit out some
	 * general information about the article.
	 */
        if ( word.End == FALSE && IS_SECTION_HDR( word.Code )) {
	    if ( collect_geninfo )
		if (UNDEF == pp_put_section( *geninfo_secid ))
		    return UNDEF;

	    PUT_GENINFO( curr_category, CODE_category, "Category: " );
	    PUT_GENINFO( curr_subcat, CODE_subcat, "Sub-category: " );
	    PUT_GENINFO( curr_obs, CODE_obscurity, "Obscurity: " );
	    PUT_GENINFO( curr_bio, CODE_bio, "Bio?: " );
	    PUT_GENINFO( curr_vol, CODE_volume, "Volume: " );
	    PUT_GENINFO( curr_page, CODE_page, "Page: " );
	    PUT_GENINFO( curr_date, CODE_date, "Date: " );
	    
            for ( i=0; i<curr_subjs; i++ ) {
		PUT_GENINFO( curr_subj[i], CODE_subject, "Subject: " );
            }

            /*
             * Find out what type of section title we're storing and
             * clear all the others just in case.
             */
            switch ( word.Code ) {
              case SYM_BDY: new_title_level = TL_main; break;
              case SYM_SEC: new_title_level = TL_sec; break;
              case SYM_SECA: new_title_level = TL_seca; break;
              case SYM_SECB: new_title_level = TL_secb; break;
              case SYM_SECC: new_title_level = TL_secc; break;
              default:       new_title_level = TL_secc; break;
            }
            for (i=curr_title_level; i<new_title_level; i++ )
                *curr_titles[i] = '\0';
            curr_title_level = new_title_level;
            
            /*
             * Find and spit out the title (if there is one).
             * Include all upper-level headings, too.
             */
            getword( f_in, &word );
            while ( word.Code == SYM_TEXT )
                getword( f_in, &word );

	    /* Discard the normalized name.  */
	    if ( word.Code == SYM_NORMNAME ) {
		getline( f_in, &word, SYM_NORMNAME, temp );
		do {
		    getword( f_in, &word );
		} while ( word.Code == SYM_TEXT );
	    }

            if ( word.Code == SYM_TI ) {
                getline( f_in, &word, SYM_TI, temp );
		scrub_title( temp );
                (void) strcpy( curr_titles[curr_title_level], temp );
            }
            else {
                *curr_titles[curr_title_level] = '\0';
            }

            if (UNDEF == pp_put_section (CODE_title))
                return (UNDEF);
            for (i=0; i<=curr_title_level; i++ )
                if ( *curr_titles[i] != '\0' ) {
                    pp_copy (curr_titles[i]);
                    pp_putbuf ('\n');
		    outlen = 0;
                }
            
            if (UNDEF == pp_put_section (CODE_body))
                return (UNDEF);

	    if ( parse_level == PARSE_SECT ) {  /* include titles here */
		for (i=0; i<=curr_title_level; i++ )
		    if ( *curr_titles[i] != '\0' ) {
			pp_copy (curr_titles[i]);
			pp_putbuf ('\n');
			outlen = 0;
		    }
	    }
            section_in_progress = TRUE;
        }

        else {
            switch (word.Code) {
              case SYM_P:
		if ( word.End == TRUE )
		    para_count++;
                if (outlen > 0 || word.End == FALSE) {
                    pp_putbuf ('\n');
                    outlen = 0;
                }
                /* Start a new section if parse_level indicates */
                if (word.End == TRUE && parse_level == PARSE_PARA) {
                    if (UNDEF == pp_put_section (CODE_body))
                        return (UNDEF);
                }
                break;

              case SYM_SENT:
                /* Start a new section if parse_level indicates */
                if (parse_level == PARSE_SENT) {
                    if (UNDEF == pp_put_section (CODE_body))
                        return (UNDEF);
                }
                break;

              case SYM_SEC:
                if ( word.End == TRUE )
		    set_title_level_up( TL_main );
                else {
		    set_title_level( TL_sec );
		    if ( parse_level == PARSE_SECT )
			start_new_section();
		}
                break;

              case SYM_SECA:
                if ( word.End == TRUE )
                    set_title_level_up( TL_sec );
                else {
                    set_title_level( TL_seca );
		    if ( parse_level == PARSE_SECT )
			start_new_section();
		}
                break;

              case SYM_SECB:
                if ( word.End == TRUE )
                    set_title_level_up( TL_seca );
                else {
                    set_title_level( TL_secb );
		    if ( parse_level == PARSE_SECT )
			start_new_section();
		}
                break;

              case SYM_SECC:
                if ( word.End == TRUE )
                    set_title_level_up( TL_secb );
                else {
                    set_title_level( TL_secc );
		    if ( parse_level == PARSE_SECT )
			start_new_section();
		}
                break;
                
                /*
                 * This is the next title.  Store it in
                 * the appropriate place.
                 */
              case SYM_TI:
                if (outlen > 0 || word.End == FALSE) {
		    if (!suppress_blank_line)
			pp_putbuf ('\n');
                    outlen = 0;
                }
                getline( f_in, &word, SYM_TI, temp );
                scrub_title( temp );
                (void) strcpy( curr_titles[curr_title_level], temp );
                pp_copy (temp);
                pp_putbuf ('\n');
		outlen = 0;
                break;
                
		/*
		 * <list type=xxx> is either arabic or bullet.  We'll just always
		 * assume bullets since that's easier here.  A list always starts on
		 * its own line.
		 */
	      case SYM_LIST:
		if (word.End == FALSE) {
		    if (outlen > 0)
			pp_putbuf( '\n' );
		    outlen = 0;
		    if (NULL==strstr( word.Text, "arabic" ))
			list_letter = 0;
		    else
			list_letter = 'a';
		}	
		else {
		    pp_putbuf( '\n' );
		    outlen = 0;
		}
		break;

		/*
		 * List elements start with an enumerator and end with a newline.
		 */
	      case SYM_LISTELT:
		if (word.End == FALSE) {
		    if (list_letter == 0)
			pp_copy( " * " );
		    else {
			pp_putbuf( ' ' );
			pp_putbuf( list_letter++ );
			pp_copy( ") " );
			if (list_letter > 'z') list_letter = 'A';
		    }
		}
		else {
		    pp_putbuf( '\n' );
		    outlen = 0;
		}
		break;

                /*
                 * This is a cross reference.  Store
                 * the information we need.
                 */
              case SYM_XREF:
                if ( curr_xrefs == MAX_XREFS ) {
                    if (log_warnings)
			(void) printf( "*** Too many Xrefs in %s (%d); one skipped\n",
				      curr_article_id, current_id_num );
		}
		else if ( word.End ) {
		    if (log_warnings)
			(void) printf( "*** End xref when not started in %s (%d)--ignored\n",
				      curr_article_id, current_id_num );
		    break;
		}
                else {
		    xref_count++;
                    process_xref( &word, f_in, &curr_xref[curr_xrefs++] );
                    (void) strcpy( word.Text, curr_xref[curr_xrefs - 1].Text );
                }
                /* Drop through to TEXT --DO NOT break*/
                
              case SYM_TEXT:
		if (isascii(*word.Text) && isalnum(*word.Text))
		    text_count++; /* so we don't count just punctuation */

                if ( strlen(word.Text) + outlen > MAX_OUTLEN ) {
                    pp_putbuf ('\n');
                    outlen = 0;
                }
                if ( outlen == 0 )
                    pp_copy (word.Text);
                else
                    pp_in_copy (word.Text);
                outlen += strlen(word.Text );
                break;
                
              default:
                break;
            }
        }
        getword( f_in, &word );

	if (close_paren_pending==1 && word.Code!=SYM_XREF) {
	    close_paren_pending = 0;
	    pp_putbuf( ')' );	/* close xref's open paren */
	}
    } /* while not SYM_BDY */

    if ( outlen > 0 )
        pp_putbuf ('\n');
    outlen = 0;

    /* Is it a xref-only article? */
    if ( para_count == 1 && text_count <= xref_count )
	if (xref_fd != NULL)
	    fprintf( xref_fd, "%7d %s\n", current_id_num, curr_article_id );

    if ( section_in_progress ) {
        (void) dump_xrefs( &curr_xrefs, curr_xref );
        if (UNDEF == pp_put_end_section (output_doc,
                                         CODE_end, start_doc_offset))
            return (UNDEF);
    }

    /*
     * Now skip to the end of this article.  Then skip to the
     * start of the one AFTER it.
     */
    getword( f_in, &word );
    while ( word.Code != SYM_FW_ART && word.Code != SYM_EOF )
        getword( f_in, &word );
    getword( f_in, &word );
    while ( word.Code != SYM_FW_ART && word.Code != SYM_EOF )
        getword( f_in, &word );

    if (word.Code == SYM_EOF) {
        in_file_flag = 0;
        if (-1 == fclose (f_in))
            return (UNDEF);
        f_in = NULL;
    }
    
    return (0);
}	

/******************************************************************
 *
 * Start a new section delimiter.  Put out a new "body" indicator
 * and then dump all previous section headings.
 *
 ******************************************************************/
static void
start_new_section()
{
    long i;

    if (UNDEF == pp_put_section (CODE_body))
	return;

    for (i=0; i<curr_title_level; i++) {
	if ( *curr_titles[i] != '\0' ) {
	    pp_copy( curr_titles[i] );
	    pp_putbuf( '\n' );
	}
    }
    suppress_blank_line = TRUE;
}

static void
set_title_level( new_level )
int new_level;
{
    long i;

    if ( new_level > curr_title_level )
	for (i=curr_title_level+1; i<new_level; i++)
	    *curr_titles[i] = '\0';

    *curr_titles[new_level] = '\0';
    curr_title_level = new_level;
}

/*
 * Same as set_title_level, but don't clear the title at the new
 * setting.  This is used when a section ends and causes a reversion
 * to the previous section level; we don't want to clear the title.
 */
static void
set_title_level_up( new_level )
int new_level;
{
    long i;

    if ( new_level > curr_title_level )
	for (i=curr_title_level+1; i<new_level; i++)
	    *curr_titles[i] = '\0';

    curr_title_level = new_level;
}

/******************************************************************
 *
 * Take a string of the form "fw.art id=XXXX type=mumble" and return just the
 * XXXX portion of it.  We strip a trailing ">" just in case it's
 * there.
 *
 ******************************************************************/
static void
scrub_id( w )
char *w;
{
    char *t = w;

    while ( *w != '=' && *w != '\0' )	/* skip to after "id=" */
	w++;
    w++;

    /* copy to end */
    while ( !IS_WHITE_SPACE(*w) && *w != '>' && *w != '\0' )
	*t++ = *w++;

    *t = '\0';
}

/******************************************************************
 *
 * Modify the passed title appropriately per the level.  Always
 * remove surrounding whitespace.  Strip trailing punctuation and
 * replace it with a period (this means a title is a sentence).
 *
 ******************************************************************/
static void
scrub_title( t )
char *t;
{
    char *tnew = t;
    char *scant = t;
    char *last_char = NULL;

    while (IS_WHITE_SPACE(*scant))
	scant++;
    while (*scant != '\0' ) {
	if ( !IS_WHITE_SPACE(*scant) )
	    last_char = tnew;

	*tnew++ = *scant++;
    }

    /*
     * Clean up the end of the title, stripping punctuation if needed.
     */
    if ( last_char == NULL )	/* title was empty */
	*t = '\0';
    else {
	while ( last_char > t && (*last_char=='.' ||
				  *last_char==',' ||
				  *last_char==';' ||
				  *last_char=='!' ||
				  *last_char==':' ||
				  *last_char=='?' ||
				  *last_char=='/'))
	       last_char--;

	if ( !ispunct(*last_char) )
	    *++last_char = '.';
	*++last_char = '\0';
    }
}

/******************************************************************
 *
 * Store the cross reference.
 * On entry, w points to the first word of the cross references,
 * the "<xr type=xxx syntax=xxx>" part.
 *
 * Also, f points to the file with the data (so we can get the
 * rest), and xref points to a place to store the information about
 * this cross reference.
 *
 ******************************************************************/
static void
process_xref( w, f, xref )
Word *w;
FILE *f;
Xref *xref;
{
    char *ptr, *xptr;
    char temp[MAX_LINE_LEN];

    /*
     * Copy over the TYPE of cross reference
     */
    for ( ptr=w->Text; *ptr != '=' && *ptr != '\0'; ptr++ );
    if ( *ptr != '\0' ) ptr++;
    xptr = xref->Type;
    for (; !IS_WHITE_SPACE(*ptr) && *ptr != '\0'; ptr++)
	*xptr++ = *ptr;
    *xptr = '\0';

    if ( strcasecmp( "see", xref->Type ) == 0 )
        xref->Type_code = Xtype_see;
    else if ( strcasecmp( "qv", xref->Type ) == 0 )
        xref->Type_code = Xtype_qv;
    else if ( strcasecmp( "sea", xref->Type ) == 0 )
        xref->Type_code = Xtype_see_also;
    else if ( strcasecmp( "com", xref->Type ) == 0 )
	xref->Type_code = Xtype_compare;
    else if ( strcasecmp( "bib", xref->Type ) == 0 )
        xref->Type_code = Xtype_bib;
    else if ( strcasecmp( "bio", xref->Type ) == 0 )
        xref->Type_code = Xtype_bio;
    else {
	if (log_warnings)
	    (void) printf( "   *-Unknown xref type=`%s' in article %s (%d)\n",
                xref->Type, curr_article_id, current_id_num );
        xref->Type_code = Xtype_unknown;
    }

    /*
     * Now get the syntax if we can.
     */
    for ( ; *ptr != '=' && *ptr != '\0'; ptr++ );
    if ( *ptr == '\0' )
        xref->Syntax = Xsyntax_unknown;
    else {
        xptr = temp;
        for (ptr++; !IS_WHITE_SPACE(*ptr) && *ptr != '\0'; ptr++)
            *xptr++ = *ptr;
        *xptr = '\0';

        if ( strcasecmp( "sep", temp ) == 0 )
            xref->Syntax = Xsyntax_sep;
        else if ( strcasecmp( "embed", temp ) == 0 )
            xref->Syntax = Xsyntax_embed;
        else if ( strcasecmp( "other", temp ) == 0 )
            xref->Syntax = Xsyntax_other;
        else if ( strcasecmp( "parens", temp ) == 0 )
            xref->Syntax = Xsyntax_parens;
        else {
	    if (log_warnings)
		(void) printf( "   *-Unknown xref syntax=`%s' in article %s (%d)\n",
                   temp, curr_article_id, current_id_num );
            xref->Syntax = Xsyntax_unknown;
        }
    }

    /*
     * Now get the "X id=YYY doc=DDD" bit.
     */
    while ( w->Code != SYM_XREF_ID )
	getword( f, w );

    for ( ptr=w->Text; *ptr != '=' && *ptr != '\0'; ptr++ );
    if ( *ptr != '\0' ) ptr++;
    xptr = xref->Id;
    for (; !IS_WHITE_SPACE(*ptr) && *ptr != '\0'; ptr++)
	*xptr++ = *ptr;
    *xptr = '\0';

    /* lookup xref->Id in docid_map to get corresponding numeric doc id */
    /* Also add to relevance info */
    if (indexing_doc_flag)
        add_rel_info (current_id_num, xref->Id);
    else 
        xref->docid = lookup_doc_map (xref->Id);

    for ( ; *ptr != '=' && *ptr != '\0'; ptr++ );
    if ( *ptr != '\0' ) ptr++;

    xptr = xref->Doc;
    for (; !IS_WHITE_SPACE(*ptr) && *ptr != '\0'; ptr++)
	*xptr++ = *ptr;
    *xptr = '\0';

    /*
     * And now get the text of the Xref itself.  
     */
    while ( w->Code != SYM_XREF_ID )
	getword( f, w );
    getline( f, w, SYM_XREF, temp );

    /*
     * If this is a syntax=paren, we start with a paren and there needs
     * eventually to be a closing.  If we need a closing and the next
     * this is NOT another cross reference, close the parens.
     */
    if (xref->Syntax==Xsyntax_parens)
	close_paren_pending = 1;

    (void) sprintf( xref->Text, "%s%s%s%s",
                   xref->Syntax==Xsyntax_parens ? "(" : "",
                   xref->Syntax==Xsyntax_other  ? " and " : "",
                   xref->Type_code==Xtype_see ? "see " :
                    xref->Type_code==Xtype_see_also ? "see also " :
		    xref->Type_code==Xtype_compare ? "compare " : "",
                   temp );
}


/******************************************************************
 *
 * Display the cross reference information.
 *
 ******************************************************************/
static int
dump_xrefs( xref_count, xref )
int	*xref_count;
Xref	xref[MAX_XREFS];
{
    long i;
    char local_buf[32];                /* Put numeric docid here */
    if ( *xref_count == 0 )
	return (0);

    if (UNDEF == pp_put_section (CODE_xref))
        return (UNDEF);
    for (i=0; i<*xref_count; i++) {
	switch (xref[i].Type_code) {
	  case Xtype_see:
	  case Xtype_see_also:
	  case Xtype_compare:
	    break;	/* already included in .Text */

	  default:
	    pp_in_copy (xref[i].Type);
	    pp_putbuf (' ');
	}
        pp_in_copy (xref[i].Text);
        pp_putbuf ('\n');
    }

    if (UNDEF == pp_put_section (CODE_xref_ids))
        return (UNDEF);
    for (i=0; i<*xref_count; i++) {
	if ( *xref[i].Doc == '\0' ) {
	    pp_in_copy (xref[i].Type);
	    pp_putbuf (' ');
            pp_in_copy (xref[i].Id);
            (void) sprintf (local_buf, " (in Smart docid %d)", xref[i].docid);
            pp_in_copy (&local_buf[0]);
            pp_putbuf ('\n');
        }
	else {
	    pp_in_copy (xref[i].Type);
	    pp_putbuf (' ');
            pp_in_copy (xref[i].Id);
            pp_in_copy (" (in ");
            pp_in_copy (xref[i].Doc);
            pp_putbuf (')');
            (void) sprintf (local_buf, " (in Smart docid %d)", xref[i].docid);
            pp_in_copy (&local_buf[0]);
            pp_putbuf ('\n');
        }
    }

    *xref_count = 0;
    return (0);
}


/******************************************************************
 *
 * getword.c
 *
 * Utility routine to retrieve a word from the input file.  Words
 * are delimited by spaces.  The special symbol words (surrounded
 * by brockets--<xxx>---are also regarded as word separators.
 * Newlines are ignored.  Finally, diacriticals and special characters
 * are converted to a close 7-bit equivalent, or are deleted when
 * no such equivalent can be found.
 *
 ******************************************************************/

static int sentence_break = 0;  /* Flag indicating if last word returned
                                   ended a sentence.  If so, return a
                                   manufactured SYM_SENT code */

#define GETC(f) (doc_offset++, getc (f))
#define UNGETC(c,f) (doc_offset--, (void) ungetc (c,f))

static void
getword( f, word )
FILE	*f;
Word	*word;
{
    int		c;
    char	*w = word->Text;
    char	*t, temp[MAX_LINE_LEN];
    int         found_period = 0;    /* flag set if period found at end of
                                        current text word */

    start_doc_offset = doc_offset;

    if (f == (FILE *) NULL) {
        word->Code = SYM_EOF;
        return;
    }

    if (parsing_indiv_doc && doc_offset >= end_doc_offset) {
	word->Code = SYM_EOF;
	return;
    }

    /* If previously found sentence, return a manufactured sentence break */
    if (sentence_break) {
        sentence_break = 0;
        word->End = FALSE;
        word->Code = SYM_SENT;
        return;
    }

    if (EOF == (c = GETC(f))) {
	word->Code = SYM_EOF;
	return;
    }

    /*
     * Stick any leading whitespace onto the word.
     */
    while ( IS_WHITE_SPACE(c) ) {
	if ( !SKIP_CHAR(c) )
	    *w++ = SPACE;
	c = GETC(f);
    }

    /*
     * For a word in brockets, get the word and then turn
     * it into something useful.
     */
    if ( (char)c == '<' ) {
	word->End = FALSE;
	w = word->Text; /* ignore leading spaces */
	c = GETC(f);
	if ( (char)c == '/' ){
	    word->End = TRUE;
	    c = GETC(f);
	}
	while ( (char)c != '>' && (char)c != '<' && /*c != '\15' &&*/ c != EOF ) {
	    *w++ = (char)c;
	    c = GETC(f);
	}
	*w = '\0';
	if ( (char)c != '>' && c != EOF )
	    UNGETC( c, f );
	set_sgml_code( word );
    }

    /*
     * For words that are not in brockets, get the word up to
     * a word break.  Then tack on any trailing spaces that there
     * happen to be.
     */
    else {
	word->Code = SYM_TEXT;

	while ( !IS_WORD_BREAK(c) && c != EOF ) {
            /* Look for a period at the end of non-whitespace */
            /* Or a period followed by punctuation (except ',') */
            if (c == '.' || c=='?' || c=='!')
                found_period = 1;
            else if (found_period) {
                if (! ispunct (c) || c == ',')
                    found_period = 0;
            }

	    if ( c == '&' ) {
		t = temp;
		*t++ = c;
		*t = GETC(f);
		while ( *t != ';' && *t != '<' && *t != ')'
		       && !IS_WHITE_SPACE(*t) && *t != '&' )
		    *(++t) = GETC(f);

		if ( *t != ';' ) {
		    UNGETC( *t, f );  /* remember other delims */
		    *t = ';';
		}

		*(++t) = '\0';
		t = translate( temp );
		while (*t != '\0')
		    *w++ = *t++;
	    }
	    else
		if (!SKIP_CHAR(c))
		    *w++ = (char)c;
	    c = GETC(f);
	}
	while ( IS_WHITE_SPACE(c) ) {
            *w++ = SPACE;
	    c = GETC(f);
	}
        /*
         * If word ended in a period followed by whitespace followed
         *  by an uppercase letter or an SGML code, assume we have found a
         *  sentence_break (to be reported next time).
        */
        if (found_period && isascii (c) && ( isupper(c) || c=='<' || c=='&' ))
            sentence_break = 1;

	*w = '\0';
	if ( c != EOF )
	    UNGETC( c, f );
    }

    word->Len = w - word->Text;
/*    printf( "Got word %s\n", word->Text ); */

}

/******************************************************************
 *
 * Figure which of the significant SGML codes the passed one is.
 * Because some SGML codes have attributes, we only consider up
 * to the first space (if there is one)--we do that in a rather
 * kludgey way, but it works....
 *
 ******************************************************************/
static void
set_sgml_code( word )
Word *word;
{
    int	i;
    int spaceloc = 0;
    char save_char = ' ';

    if ( word->Text[0] != '\0' ) 
	for ( i=1; word->Text[i] != '\0'; i++ )
	    if ( IS_WHITE_SPACE(word->Text[i]) ) {
		save_char = word->Text[i];
		spaceloc = i;
		word->Text[spaceloc] = '\0';
		break;
	    }

    for ( i=0; i != sgml_COUNT
	      && strcmp( sgml_codes[i].word, word->Text ) != 0 ; i++ );
    if ( i == sgml_COUNT )
	word->Code = SYM_UNKNOWN;
    else
	word->Code = sgml_codes[i].code;

    if ( spaceloc != 0 )
	word->Text[spaceloc] = save_char;
}


/******************************************************************
 *
 * Retrieve a string of words until the passed symbol type is
 * returned.  All non-text words in the middle are ignored.
 *
 ******************************************************************/

static void
getline( f, word, stop_symbol, outline )
FILE *f;
Word *word;
int stop_symbol;
char *outline;
{
    *outline = '\0';

    getword( f, word );
    while ( word->Code != stop_symbol ) {
	if ( word->Code == SYM_TEXT )
	    (void) strcat( outline, word->Text );

	getword( f, word );
    }
}

/******************************************************************
 *
 * This routine takes a special character which is described by
 * a string "&xxxx;" and returns an english-language equivalent
 * character or \0 if there is none.
 *
 ******************************************************************/
static char *
translate( w )
char *w;
{
    long i;
    for ( i=0; *(trans[i].Symbol) != '\0'; i++ ) {
	if ( strcmp( trans[i].Symbol, w ) == 0 )
	    return trans[i].Value;

    }
    if (log_warnings > 1)
	(void) printf( "*** No translation found for `%s' in %s (%d)--omitted\n",
		      w, curr_article_id, current_id_num );
    return trans[i].Value;	/* pointer to empty string */
}

static int
pp_put_begin_section (output_doc, section, offset, filename)
SM_INDEX_TEXTDOC *output_doc;
char section;
long offset;
char *filename;
{
    num_sections = 0;
    output_doc->textloc_doc.file_name = filename;
    output_doc->textloc_doc.begin_text = offset;

    buf_ptr = buf;
    mem_disp[0].begin_section = 0;
    mem_disp[0].section_id = section;

    return (0);
}

static int
pp_put_section (section)
char section;
{
    if (num_sections >= max_num_sections) {
        if (NULL == (mem_disp = (SM_DISP_SEC *) realloc ((char *) mem_disp,
                             (max_num_sections + DISP_SEC_SIZE) *
                             sizeof (SM_DISP_SEC)))) {
            set_error (errno, "DISP_SEC malloc", "preparse");
            return (UNDEF);
        }
        max_num_sections += DISP_SEC_SIZE;
    }
    if (num_sections > 0) 
        mem_disp[num_sections-1].end_section = buf_ptr - buf;
    mem_disp[num_sections].begin_section = buf_ptr - buf;
    mem_disp[num_sections].section_id = section;
    num_sections++;

    return (0);
}


static int
pp_put_end_section (output_doc, section, offset)
SM_INDEX_TEXTDOC *output_doc;
char section;
long offset;
{
    output_doc->textloc_doc.end_text = offset;

    if (num_sections > 0)
        mem_disp[num_sections-1].end_section = buf_ptr - buf;

    output_doc->mem_doc.sections = mem_disp;
    output_doc->mem_doc.num_sections = num_sections;
    output_doc->doc_text = buf;

    return (0);
}

static void
pp_copy (ptr)
register char *ptr;
{
    /* Make sure actual text line does not begin with a dot */
    /* by adding an extra space there. */
    if (*ptr == '.') {
        pp_putbuf (' ');
    }

    while (*ptr) {
        /* Do not output non-ascii characters (the parser objects) or */
        /* control-characters */
        if (isascii (*ptr) && (isprint (*ptr) || isspace (*ptr))) {
            pp_putbuf (*ptr);
        }
        ptr++;
    }
}


/* Same as pp_copy except copying within a line, so don't check beginning
   '.' */
static void
pp_in_copy (ptr)
register char *ptr;
{
    while (*ptr) {
        /* Do not output non-ascii characters (the parser objects) or */
        /* control-characters */
        if (isascii (*ptr) && (isprint (*ptr) || isspace (*ptr))) {
            pp_putbuf (*ptr);
        }
        ptr++;
    }
}


static int
init_docid_map ()
{
    REL_HEADER rh;

    rh.max_primary_value = docid_map_size;

    if (UNDEF == create_dict (docid_map_file, &rh)) {
        return (UNDEF);
    }

    if (UNDEF == (dict_fd = open_dict (docid_map_file,
                                       docid_map_mode)))
        return (UNDEF);

    return (1);
}

static int
add_id (pp_vec)
SM_INDEX_TEXTDOC *pp_vec;
{
    char id[MAX_LINE_LEN];
    long i;
    DICT_ENTRY dict;
    int status;

    for (i = 0;
         i < pp_vec->mem_doc.num_sections &&
         pp_vec->mem_doc.sections[i].section_id != CODE_id;
         i++)
        ;
    if (i <  pp_vec->mem_doc.num_sections) {
        (void) strncpy (&id[0],
                 &pp_vec->doc_text[pp_vec->mem_doc.sections[i].begin_section],
                 (int) pp_vec->mem_doc.sections[i].end_section -
                 (int) pp_vec->mem_doc.sections[i].begin_section);
        id[pp_vec->mem_doc.sections[i].end_section -
           pp_vec->mem_doc.sections[i].begin_section - 1] = '\0';
        dict.token = &id[0];
        dict.con = UNDEF;
        dict.info = current_id_num;
        if (UNDEF == (status = seek_dict (dict_fd, &dict)))
            return (UNDEF);
        if (status == 1) {
	    if (log_warnings)
		(void) printf ("*** Multiple use of id `%s' detected in %s (%d)\n",
			       &id[0], curr_article_id, current_id_num );
            return (0);
        }
        if (1 != write_dict (dict_fd, &dict))
            return (UNDEF);
        return (1);
    }
    return (0);
}

static long
lookup_doc_map (token)
char *token;
{
    DICT_ENTRY dict;
    dict.token = token;
    dict.info = 0;
    dict.con = UNDEF;

    if (1 != seek_dict (dict_fd, &dict) ||
        1 != read_dict (dict_fd, &dict)) {
        return (0);
    }
    return (dict.info);
}

typedef struct {
    long did;
    long xref;
} REL_INFO;

static REL_INFO *rel_info;
static long num_rel_info;
static long end_rel_info;
static char *rel_info_buf;
static long rel_info_buf_offset;
static long rel_info_buf_end;

static int
init_add_rel_info ()
{
    if (NULL == (rel_info_buf = malloc (130000)))
        return (UNDEF);
    rel_info_buf_end = 130000;
    rel_info_buf_offset = 0;
    if (NULL == (rel_info = (REL_INFO *)
                 malloc (64000 * sizeof (REL_INFO))))
        return (UNDEF);
    num_rel_info = 0;
    end_rel_info = 64000;
    return (1);
}

typedef struct {
    long did1;
    long did2;
} RR_DIDS;

static int 
compare_rr_dids (rr_did1, rr_did2)
RR_DIDS *rr_did1, *rr_did2;
{
    if (rr_did1->did1 < rr_did2->did1)
        return (-1);
    if (rr_did1->did1 > rr_did2->did1)
        return (1);
    if (rr_did1->did2 < rr_did2->did2)
        return (-1);
    if (rr_did1->did2 > rr_did2->did2)
        return (1);
    return (0);
}


/******************************************************************
 *
 * Take the relevance judgements that were stashed in rel_info_buf
 * and turn them into an rr_vec format in the qrels file.
 *
 ******************************************************************/
static int
construct_rels()
{
    int qrels_fd;
    RR_VEC rr;
    RR_DIDS *rr_dids;
    RR_TUP *rr_tups;
    long num_rr_dids = 0;
    long i;

    if (NULL == (rr_dids = (RR_DIDS *)
                 malloc ((unsigned) num_rel_info * 2 * sizeof (RR_DIDS))) ||
	NULL == ( rr_tups = (RR_TUP *)
                 malloc ((unsigned) num_rel_info * 2 * sizeof (RR_TUP))))
        return (UNDEF);
    
    for (i = 0; i < num_rel_info; i++) {
        if (0 != (rr_dids[num_rr_dids].did2 =
                  lookup_doc_map (&rel_info_buf[rel_info[i].xref]))) {
            rr_dids[num_rr_dids].did1 = rel_info[i].did;
            num_rr_dids++;
        }
    }
    qsort ((char *) rr_dids, (int) num_rr_dids,
	   sizeof (RR_DIDS), compare_rr_dids);

    /*
     * Copy the relevance judgements out to the rr file.
     */
    if (UNDEF == (qrels_fd = open_rr_vec (rel_file, rel_mode)))
        return (UNDEF);

    rr.qid = -1;
    rr.num_rr = 0;
    rr.rr = rr_tups;
    for (i = 0; i < num_rr_dids; i++) {
	if ( rr_dids[i].did1 != rr.qid ) {
	    if ( rr.num_rr != 0 ) {
		if (UNDEF == seek_rr_vec (qrels_fd, &rr) ||
		    UNDEF == write_rr_vec (qrels_fd, &rr))
		    return (UNDEF);
	    }
	    rr.qid = rr_dids[i].did1;
	    rr.num_rr = 0;
	}

	rr_tups[rr.num_rr].did = rr_dids[i].did2;
	rr_tups[rr.num_rr].rank = 0;
	rr_tups[rr.num_rr].sim = 0.0;
	rr.num_rr++;
    }
    if ( rr.num_rr != 0 ) {
	if (UNDEF == seek_rr_vec (qrels_fd, &rr) ||
	    UNDEF == write_rr_vec (qrels_fd, &rr))
	    return (UNDEF);
    }

    if (UNDEF == (close_rr_vec (qrels_fd)))
        return (UNDEF);

    (void) free ((char *) rr_tups);
    (void) free ((char *) rr_dids);
    return (1);
}

    
static void
add_rel_info (did, doc_title)
long did;
char *doc_title;
{
    int len;
    if (num_rel_info >= end_rel_info) {
        if (NULL == (rel_info = (REL_INFO *)
                     realloc ((char *) rel_info, (unsigned) end_rel_info * 2)))
            return;
    }
    len = strlen (doc_title);
    if (rel_info_buf_offset + len >= rel_info_buf_end) {
        if (NULL == (rel_info_buf =
                     realloc (rel_info_buf, 2*(unsigned)rel_info_buf_offset)))
            return;
    }
    (void) strcpy (&rel_info_buf[rel_info_buf_offset], doc_title);

    rel_info[num_rel_info].did = did;
    rel_info[num_rel_info].xref = rel_info_buf_offset;
    num_rel_info++;
    rel_info_buf_offset += len + 1;
}

/********************   PROCEDURE DESCRIPTION   ************************
 *0 pre-parser for F&W encyclopedia that breaks sections at para boundaries
 *1 local.index.parts_preparse.fw_para
 *2 pp_fw_para (input_doc, output_doc, inst)
 *3   TEXTLOC *input_doc;
 *3   SM_INDEX_TEXTDOC *output_doc;
 *3   int inst;
 *4 init_pp_fw (spec_ptr, unused)
 *5   "index.preparse.trace"
 *5   "index.preparse.doc_loc"
 *5   "local.index.preparse.fw.docid_map"
 *5   "local.index.preparse.fw.docid_map.rwmode"
 *5   "local.index.preparse.fw.docid_map_size"
 *5   "local.index.preparse.fw.rel_file"
 *5   "local.index.preparse.fw.rel_file.rwcmode"
 *5   "index.preparse.query_loc"
 *4 close_pp_fw (inst)

 *6 Uses global_context to tell if working on doc or query (CTXT_DOC, 
 *6 CTXT_QUERY), and if doing original indexing on doc (CTXT_INDEXING_DOC)

 *7 Normal preparser operations on fw (see pp_fw), except preparsed sections
 *7 are broken up at every paragraph boundary (sgml code </p>).
 *7 Return the preparsed next document (in output_doc) to caller. 
 *7 Input_doc must be non-NULL; ignore the list of files, and assume that
 *7 input_doc contains exactly one complete document to be preparsed.
 *8 Uses the same algorithm and procedures as pp_fw, except passes 
 *8 PARSE_PARA flag to parse_sgml.
***********************************************************************/
int
pp_fw_para (input_doc, output_doc, inst)
TEXTLOC *input_doc;
SM_INDEX_TEXTDOC *output_doc;
int inst;
{

    PRINT_TRACE (2, print_string, "Trace: entering pp_fw_para");

    if (input_doc == NULL) {
        set_error (SM_INCON_ERR, "pp_fw_para","Must be called with input_doc");
        return (UNDEF);
    }

    current_id_num = input_doc->id_num;
    output_doc->id_num = output_doc->mem_doc.id_num =
        output_doc->textloc_doc.id_num = current_id_num;

    /* Open doc_file */
    (void) strcpy (doc_file, input_doc->file_name);
    doc_offset = input_doc->begin_text;
    end_doc_offset = input_doc->end_text;
    parsing_indiv_doc = 1;
    if (NULL == (f_in = fopen (doc_file, "r")) ||
        -1 == fseek (f_in, doc_offset, 0)) {
        /* non-existent or empty file */
        /* Should be error? */
        return (0);
    }
    in_file_flag = 1;

    /*
     * Get to the start of the first article in the input file.
     */
    word.Len = 0;
    getword( f_in, &word );
    while ( word.Code != SYM_FW_ART && word.Code != SYM_EOF )
        getword( f_in, &word );
    if (word.Code == SYM_EOF) {
        /* empty file is an error */
        return (UNDEF);
    }

    /* At this point, word.Code is guaranteed to be SYM_FW_ART.
       Preparse this article, ending on SYM_EOF or start of next
       article.  If ending on SYM_EOF, set in_file_flag to 0; */
    if (UNDEF == parse_sgml (output_doc, PARSE_PARA)) {
        return (UNDEF);
    }

    PRINT_TRACE (4, print_int_textdoc, output_doc);
    PRINT_TRACE (2, print_string, "Trace: leaving pp_fw_para");

    return (1);
}
/********************   PROCEDURE DESCRIPTION   ************************
 *0 pre-parser for F&W encyclopedia that breaks sections at sentences
 *1 local.index.parts_preparse.fw_sent
 *2 pp_fw_sent (input_doc, output_doc, inst)
 *3   TEXTLOC *input_doc;
 *3   SM_INDEX_TEXTDOC *output_doc;
 *3   int inst;
 *4 init_pp_fw (spec_ptr, unused)
 *5   "index.preparse.trace"
 *5   "index.preparse.doc_loc"
 *5   "local.index.preparse.fw.docid_map"
 *5   "local.index.preparse.fw.docid_map.rwmode"
 *5   "local.index.preparse.fw.docid_map_size"
 *5   "local.index.preparse.fw.rel_file"
 *5   "local.index.preparse.fw.rel_file.rwcmode"
 *5   "index.preparse.query_loc"
 *4 close_pp_fw (inst)

 *6 Uses global_context to tell if working on doc or query(CTXT_DOC,CTXT_QUERY)

 *7 Normal preparser operations on fw (see pp_fw), except preparsed sections
 *7 are broken up at every sentence boundary (sentences defined in get_word).
 *7 Return the preparsed next document (in output_doc) to caller. 
 *7 Input_doc must be non-NULL; ignore the list of files, and assume that
 *7 input_doc contains exactly one complete document to be preparsed.
 *8 Uses the same algorithm and procedures as pp_fw, except passes 
 *8 PARSE_SENT flag to parse_sgml.
***********************************************************************/

int
pp_fw_sent (input_doc, output_doc, inst)
TEXTLOC *input_doc;
SM_INDEX_TEXTDOC *output_doc;
int inst;
{

    PRINT_TRACE (2, print_string, "Trace: entering pp_fw_sent");

    if (input_doc == NULL) {
        set_error (SM_INCON_ERR, "pp_fw_sent","Must be called with input_doc");
        return (UNDEF);
    }

    current_id_num = input_doc->id_num;
    output_doc->id_num = output_doc->mem_doc.id_num =
        output_doc->textloc_doc.id_num = current_id_num;

    /* Open doc_file */
    (void) strcpy (doc_file, input_doc->file_name);
    doc_offset = input_doc->begin_text;
    end_doc_offset = input_doc->end_text;
    parsing_indiv_doc = 1;
    if (NULL == (f_in = fopen (doc_file, "r")) ||
        -1 == fseek (f_in, doc_offset, 0)) {
        /* non-existent or empty file */
        /* Should be error? */
        return (0);
    }
    in_file_flag = 1;

    /*
     * Get to the start of the first article in the input file.
     */
    word.Len = 0;
    getword( f_in, &word );
    while ( word.Code != SYM_FW_ART && word.Code != SYM_EOF )
        getword( f_in, &word );
    if (word.Code == SYM_EOF) {
        /* empty file is an error */
        return (UNDEF);
    }

    /* At this point, word.Code is guaranteed to be SYM_FW_ART.
       Preparse this article, ending on SYM_EOF or start of next
       article.  If ending on SYM_EOF, set in_file_flag to 0; */
    if (UNDEF == parse_sgml (output_doc, PARSE_SENT)) {
        return (UNDEF);
    }

    PRINT_TRACE (4, print_int_textdoc, output_doc);
    PRINT_TRACE (2, print_string, "Trace: leaving pp_fw_sent");

    return (1);
}

/********************   PROCEDURE DESCRIPTION   ************************
 *0 pre-parser for F&W encyclopedia that breaks pp sections at sections
 *1 local.index.parts_preparse.fw_sect
 *2 pp_fw_sect (input_doc, output_doc, inst)
 *3   TEXTLOC *input_doc;
 *3   SM_INDEX_TEXTDOC *output_doc;
 *3   int inst;
 *4 init_pp_fw (spec_ptr, unused)
 *5   "index.preparse.trace"
 *5   "index.preparse.doc_loc"
 *5   "local.index.preparse.fw.docid_map"
 *5   "local.index.preparse.fw.docid_map.rwmode"
 *5   "local.index.preparse.fw.docid_map_size"
 *5   "local.index.preparse.fw.rel_file"
 *5   "local.index.preparse.fw.rel_file.rwcmode"
 *5   "index.preparse.query_loc"
 *4 close_pp_fw (inst)

 *6 Uses global_context to tell if working on doc or query(CTXT_DOC,CTXT_QUERY)

 *7 Normal preparser operations on fw (see pp_fw), except preparsed sections
 *7 are broken up at every fw defined section.
 *7 Return the preparsed next document (in output_doc) to caller. 
 *7 Input_doc must be non-NULL; ignore the list of files, and assume that
 *7 input_doc contains exactly one complete document to be preparsed.
 *8 Uses the same algorithm and procedures as pp_fw, except passes 
 *8 PARSE_SECT flag to parse_sgml.
***********************************************************************/
int
pp_fw_sect (input_doc, output_doc, inst)
TEXTLOC *input_doc;
SM_INDEX_TEXTDOC *output_doc;
int inst;
{

    PRINT_TRACE (2, print_string, "Trace: entering pp_fw_sect");

    if (input_doc == NULL) {
        set_error (SM_INCON_ERR, "pp_fw_sect","Must be called with input_doc");
        return (UNDEF);
    }

    current_id_num = input_doc->id_num;
    output_doc->id_num = output_doc->mem_doc.id_num =
        output_doc->textloc_doc.id_num = current_id_num;

    /* Open doc_file */
    (void) strcpy (doc_file, input_doc->file_name);
    doc_offset = input_doc->begin_text;
    end_doc_offset = input_doc->end_text;
    parsing_indiv_doc = 1;
    if (NULL == (f_in = fopen (doc_file, "r")) ||
        -1 == fseek (f_in, doc_offset, 0)) {
        /* non-existent or empty file */
        /* Should be error? */
        return (0);
    }
    in_file_flag = 1;

    /*
     * Get to the start of the first article in the input file.
     */
    word.Len = 0;
    getword( f_in, &word );
    while ( word.Code != SYM_FW_ART && word.Code != SYM_EOF )
        getword( f_in, &word );
    if (word.Code == SYM_EOF) {
        /* empty file is an error */
        return (UNDEF);
    }

    /* At this point, word.Code is guaranteed to be SYM_FW_ART.
       Preparse this article, ending on SYM_EOF or start of next
       article.  If ending on SYM_EOF, set in_file_flag to 0; */
    if (UNDEF == parse_sgml (output_doc, PARSE_SECT)) {
        return (UNDEF);
    }

    PRINT_TRACE (4, print_int_textdoc, output_doc);
    PRINT_TRACE (2, print_string, "Trace: leaving pp_fw_sect");

    return (1);
}


