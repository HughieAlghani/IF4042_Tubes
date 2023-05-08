#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/liblocal/libproc/proc_index.c,v 11.0 1992/07/21 18:22:21 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "proc.h"

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Local hierarchy table giving top level procedures to index a collection
 *1 local.index.top
 *2 * (unused1, unused2, inst)
 *3    char *unused1;
 *3    char *unused2;
 *3    int inst;
 *4 init_* (spec, unused)
 *4 close_* (inst)
 *7 Procedure table mapping a string procedure name to procedure addresses.
 *7 These procedures are top-level procedures which index an entire vector
 *7 object.  As top-level procedures, they are responsible for setting
 *7 trace conditions, and for determining other execution time constraints,
 *7 such as when execution should stop (eg, if global_end is exceeded).
 *7 Return UNDEF if error, else 1.
 *8 Current possibilities are "parts_coll_fw".
***********************************************************************/

extern int init_parts_coll_fw(), parts_coll_fw(), close_parts_coll_fw();
static PROC_TAB topproc_index[] = {
    "parts_coll_fw",init_parts_coll_fw, parts_coll_fw,  close_parts_coll_fw,
};
static int num_topproc_index =
    sizeof (topproc_index) / sizeof (topproc_index[0]);



/********************   PROCEDURE DESCRIPTION   ************************
 *0 Local hierarchy table: procedures preparsing a text object
 *1 local.index.preparse
 *2 * (input_doc, output_doc, inst)
 *3   SM_TEXTDISPLAY *input_doc;
 *3   SM_INDEX_TEXTDOC *output_doc;
 *3   int inst;
 *4 init_* (spec, unused)
 *4 close_* (inst)
 *7 Procedure table mapping a string procedure name to procedure addresses.
 *7 These procedures take document/query text locations, either given for 
 *7 a single vector by input_doc or, if input_doc is NULL, by other parameters,
 *7 and preparse that document.
 *7 This is a collection dependant procedure whose implementation will vary
 *7 widely with collection.
 *7 Output is document text in standard SMART pre_parser format:
 *7 the document is broken up into sections of text, each of which can
 *7 later be parsed separately.  Depending on the collection, the
 *7 preparser can ignore, re-format, or even add, text to the original
 *7 document.  This pass is responsible for first getting document in
 *7 ascii text format; eg by running latex.
 *7 Return UNDEF if error,  1 if a non-empty object was preparsed, and 0
 *7 if no object remains to be preparsed.
 *8 Current possibilities are "expnews", "fw",
***********************************************************************/
extern int init_pp_expnews(), pp_expnews(), close_pp_expnews();
extern int init_pp_fw(), pp_fw(), close_pp_fw();
extern int init_pp_trec(), pp_trec(), close_pp_trec();
extern int init_pp_trec_query(), pp_trec_query(), close_pp_trec_query();
extern int init_pp_smart_nc(), pp_smart_nc(), close_pp_smart_nc();
static PROC_TAB proc_preparse[] = {
    "expnews",		init_pp_expnews,pp_expnews,	close_pp_expnews,
    "fw",		init_pp_fw,	pp_fw,	        close_pp_fw,
    "trec",		init_pp_trec,	pp_trec,        close_pp_trec,
    "trec_query",	init_pp_trec_query,pp_trec_query,close_pp_trec_query,
    "smart_nc",		init_pp_smart_nc,pp_smart_nc,   close_pp_smart_nc,
    };
static int num_proc_preparse = sizeof (proc_preparse) / sizeof (proc_preparse[0]);


/********************   PROCEDURE DESCRIPTION   ************************
 *0 Local hierarchy table: procedures preparsing text object into "parts" 
 *1 local.index.parts_preparse
 *2 * (input_doc, output_doc, inst)
 *3   SM_TEXTDISPLAY *input_doc;
 *3   SM_INDEX_TEXTDOC *output_doc;
 *3   int inst;
 *4 init_* (spec, unused)
 *4 close_* (inst)
 *7 Procedure table mapping a string procedure name to procedure addresses.
 *7 These procedures take document/query text locations, given for 
 *7 a single vector by input_doc and preparse that document.
 *7 Output is document text in standard SMART pre_parser format.
 *7 the document is broken up into sections of text, each of which can
 *7 later be parsed separately.  Sections are determined first by the
 *7 normal collection dependent algorithm, and then further broken down into
 *7 syntactic parts; for example, sentences, paragraphs, or collection
 *7 sections.  Each syntactic part is considered its own section, with the
 *7 same section id as in the original collection dependent algorithm.
 *7 Return UNDEF if error,  1 if a non-empty object was preparsed, and 0
 *7 if no object remains to be preparsed.
 *8 Current possibilities are  "fw_sent",  "fw_para", "fw_sect"
***********************************************************************/
extern int pp_fw_sent(), pp_fw_para(), pp_fw_sect();
static PROC_TAB proc_parts_preparse[] = {
    "fw_sent",		init_pp_fw,	pp_fw_sent,     close_pp_fw,
    "fw_para",		init_pp_fw,	pp_fw_para,     close_pp_fw,
    "fw_sect",		init_pp_fw,	pp_fw_sect,     close_pp_fw,
    };
static int num_proc_parts_preparse = sizeof (proc_parts_preparse) / sizeof (proc_parts_preparse[0]);


/********************   PROCEDURE DESCRIPTION   ************************
 *0 Local hierarchy table: procedures to parse section from tokenized document
 *1 local.index.parse_sect
 *2 * (token, consect, inst)
 *3   SM_TOKENSECT *token;
 *3   SM_CONSECT *consect;
 *3   int inst;
 *4 init_* (spec, unused)
 *4 close_* (inst)
 *7 Procedure table mapping a string procedure name to procedure addresses.
 *7 These procedures take a list of tokens for a section and return the
 *7 concept number, ctype and the position in the list of tokens 
 *7 that this token was found.
 *7 Return UNDEF if error, 0 if no valid tokens found in this section,
 *7 and 1 upon successful finding of tokens.
 *8 Normally called from a procedure in index.parse.
 *8 Current possibilities are "none",
***********************************************************************/
extern int  no_parse();
static PROC_TAB proc_parse_sect[] = {
    "none",	        INIT_EMPTY,      no_parse,	CLOSE_EMPTY,
    };
static int num_proc_parse_sect = sizeof (proc_parse_sect) / sizeof (proc_parse_sect[0]);

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Local hierarchy table: procedures to take token and return a concept number
 *1 local.index.token_to_con
 *2 * (word, con, inst)
 *3   char *word;
 *3   long *con;
 *3   int inst;
 *4 init_* (spec, unused)
 *4 close_* (inst)
 *7 Procedure table mapping a string procedure name to procedure addresses.
 *7 These procedures take a single token, and find the concept number to
 *7 be used to represent that token.
 *7 Return UNDEF if error, 1 otherwise
 *8 Current possibilities are "empty"
***********************************************************************/
extern int init_tokcon_dict_niss(), tokcon_dict_niss(), close_tokcon_dict_niss()
;
static PROC_TAB proc_tocon[] = {
    "dict_niss",init_tokcon_dict_niss, tokcon_dict_niss,close_tokcon_dict_niss,
    "empty",		INIT_EMPTY,	EMPTY,		CLOSE_EMPTY
    };
static int num_proc_tocon = sizeof (proc_tocon) / sizeof (proc_tocon[0]);

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Local hierarchy table:procedures to create a vector from a list of concepts
 *1 local.index.makevec
 *2 * (p_doc, vec, inst)
 *3   SM_CONDOC *p_doc;
 *3   SM_VECTOR *vec;
 *3   int inst;
 *4 init_* (spec, unused)
 *4 close_* (inst)
 *7 Procedure table mapping a string procedure name to procedure addresses.
 *7 These procedures take a list of concepts (eg from index.parse) and create
 *7 a vector from them, where the vector contains the appropriate concept
 *7 numbers, and the weight of each concept is the number of times the
 *7 concept occurs in the list.
 *7 Return UNDEF if error, 1 otherwise
 *8 Current possibilities are "makevec"
***********************************************************************/
extern int init_makevec_diag(), makevec_diag(), close_makevec_diag();
static PROC_TAB proc_makevec[] = {
    "makevec_diag",  init_makevec_diag,  makevec_diag,  close_makevec_diag,
    };
static int num_proc_makevec = sizeof (proc_makevec) / sizeof (proc_makevec[0]);


/********************   PROCEDURE DESCRIPTION   ************************
 *0 Hierarchy table: table of other hierarchy tables for indexing procedures
 *1 local.index
 *7 Procedure table mapping a string procedure table to either another
 *7 table of hierarchy tables, or to a stem table which maps 
 *7 names to procedures
 *8 Current possibilities are "index", "vec_index", "index_pp",
 *8 "preparse", "parts_preparse", "next_vecid", "addtextloc", "token",
 *8 "parse", "parse_sect", "stop", "stem", "tocon", "fromcon", "makevec",
 *8 "expand", "store"
***********************************************************************/
TAB_PROC_TAB lproc_index[] = {
    "top",            TPT_PROC,       NULL,   topproc_index,
                        &num_topproc_index,
    "preparse",		TPT_PROC,	NULL,	proc_preparse,	
                        &num_proc_preparse,
    "parts_preparse",	TPT_PROC,	NULL,	proc_parts_preparse,
                        &num_proc_parts_preparse,
    "parse_sect",	TPT_PROC,	NULL,	proc_parse_sect,	
			&num_proc_parse_sect,
    "token_to_con",	TPT_PROC,	NULL, 	proc_tocon,	
			&num_proc_tocon,
    "makevec",          TPT_PROC,       NULL,   proc_makevec,
                        &num_proc_makevec,
    };

int num_lproc_index = sizeof (lproc_index) / sizeof (lproc_index[0]);
