#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libproc/proc_index.c,v 11.0 1992/07/21 18:23:06 chrisb Exp $";
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
 *0 Hierarchy table giving top level procedures to index a collection
 *1 index.top
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
 *8 Current possibilities are "doc_coll", "query_coll", "exp_coll",
 *8 "add_parts", "parts_coll", and "parts_coll_fw".
***********************************************************************/

extern int init_index_doc_coll(), index_doc_coll(), close_index_doc_coll();
extern int init_reindex_doc_coll();
extern int init_index_exp_coll(), index_exp_coll(), close_index_exp_coll();
extern int init_add_parts(), add_parts(), close_add_parts();
extern int init_parts_coll(), parts_coll(), close_parts_coll();
extern int init_parts_coll_fw(), parts_coll_fw(), close_parts_coll_fw();
extern int init_index_query_coll(), index_query_coll(),
    close_index_query_coll();
static PROC_TAB topproc_index[] = {
    "doc_coll",  init_index_doc_coll,  index_doc_coll,  close_index_doc_coll,
    "reindex_doc",init_reindex_doc_coll,index_doc_coll, close_index_doc_coll,
    "query_coll",init_index_query_coll,index_query_coll,close_index_query_coll,
    "exp_coll",  init_index_exp_coll,  index_exp_coll,  close_index_exp_coll,
    "add_parts", init_add_parts,       add_parts,       close_add_parts,
    "parts_coll",init_parts_coll,      parts_coll,      close_parts_coll,
    "parts_coll_fw",init_parts_coll_fw, parts_coll_fw,  close_parts_coll_fw,
};
static int num_topproc_index =
    sizeof (topproc_index) / sizeof (topproc_index[0]);


/********************   PROCEDURE DESCRIPTION   ************************
 *0 Hierarchy table giving procedures to index a single vector from text
 *1 index.vec
 *2 * (textdisp, vec, inst)
 *3   SM_TEXTDISPLAY *textdisp;
 *3   VEC *vec;
 *3   int inst;
 *4 init_* (spec, unused)
 *4 close_* (inst)
 *7 Procedure table mapping a string procedure name to procedure addresses.
 *7 These procedures take the location of the vector, given by textdisp,
 *7 and index that vector, result in vec.
 *7 Return UNDEF if error, else 1.
 *8 Current possibilities are "doc", "query"
***********************************************************************/
extern int init_index_doc(), index_doc(), close_index_doc();
extern int init_index_query(), index_query(), close_index_query();
static PROC_TAB proc_vec_index[] = {
    "doc",      init_index_doc,    index_doc,   close_index_doc,
    "query",    init_index_query,  index_query, close_index_query,
};
static int num_proc_vec_index =
    sizeof (proc_vec_index) / sizeof (proc_vec_index[0]);


/********************   PROCEDURE DESCRIPTION   ************************
 *0 Hierarchy table: procedures indexing a single vector from preparsed text.
 *1 index.index_pp
 *2 * (pp_vec, vec, inst)
 *3   SM_INDEX_TEXTDOC *pp_vec;
 *3   VEC *vec;
 *3   int inst;
 *4 init_* (spec, unused)
 *4 close_* (inst)
 *7 Procedure table mapping a string procedure name to procedure addresses.
 *7 These procedures take the preparsed text of a vector, given by pp_vec
 *7 and index that vector, result in vec.
 *7 Return UNDEF if error, else 1.
 *8 Current possibilities are "index_pp"
***********************************************************************/
extern int init_index_pp(), index_pp(), close_index_pp();
static PROC_TAB proc_index_pp[] = {
    "index_pp",      init_index_pp,    index_pp,   close_index_pp,
};
static int num_proc_index_pp =
    sizeof (proc_index_pp) / sizeof (proc_index_pp[0]);


/********************   PROCEDURE DESCRIPTION   ************************
 *0 Hierarchy table: procedures preparsing a text object
 *1 index.preparse
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
 *8 Current possibilities are "smart", "news", "expnews", "mail", "fw",
 *8 "fw1", "text", "docsmart", "empty"
***********************************************************************/
extern int init_pp_smart(), pp_smart(), close_pp_smart();
extern int init_pp_docsmart(), pp_docsmart(), close_pp_docsmart();
extern int init_pp_news(), pp_news(), close_pp_news();
extern int init_pp_expnews(), pp_expnews(), close_pp_expnews();
extern int init_pp_mail(), pp_mail(), close_pp_mail();
extern int init_pp_fw(), pp_fw(), close_pp_fw();
extern int init_pp_generic(), pp_generic(), close_pp_generic();
extern int init_pp_text(), pp_text(), close_pp_text();
static PROC_TAB proc_preparse[] = {
    "smart",		init_pp_smart,	pp_smart,	close_pp_smart,
    "news",		init_pp_news,	pp_news,	close_pp_news,
    "expnews",		init_pp_expnews,pp_expnews,	close_pp_expnews,
    "mail",		init_pp_mail,	pp_mail,	close_pp_mail,
    "text",		init_pp_text,	pp_text,	close_pp_text,
    "fw",		init_pp_fw,	pp_fw,	        close_pp_fw,
    "generic",		init_pp_generic,pp_generic,     close_pp_generic,
    "docsmart",		init_pp_docsmart, pp_docsmart,	close_pp_docsmart,
    };
static int num_proc_preparse = sizeof (proc_preparse) / sizeof (proc_preparse[0]);


/********************   PROCEDURE DESCRIPTION   ************************
 *0 Hierarchy table: procedures preparsing a text object into "parts" sections
 *1 index.parts_preparse
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
 *8 Current possibilities are "sent", "para", "sect", "fw_sent", 
 *8 "fw_para", "fw_sect", "empty"
***********************************************************************/
extern int pp_fw_sent(), pp_fw_para(), pp_fw_sect();
extern int init_pp_sent(), pp_parts(), close_pp_parts();
extern int init_pp_para(), init_pp_sect();
static PROC_TAB proc_parts_preparse[] = {
    "sent",		init_pp_sent,	pp_parts,       close_pp_parts,
    "para",		init_pp_para,	pp_parts,       close_pp_parts,
    "sect",		init_pp_sect,	pp_parts,       close_pp_parts,
    "fw_sent",		init_pp_fw,	pp_fw_sent,     close_pp_fw,
    "fw_para",		init_pp_fw,	pp_fw_para,     close_pp_fw,
    "fw_sect",		init_pp_fw,	pp_fw_sect,     close_pp_fw,
    };
static int num_proc_parts_preparse = sizeof (proc_parts_preparse) / sizeof (proc_parts_preparse[0]);


/********************   PROCEDURE DESCRIPTION   ************************
 *0 Hierarchy table: procedures returning next available id
 *1 index.next_vecid
 *2 * (unused, id, inst)
 *3   char *unused;
 *3   long *id;
 *3   int inst;
 *4 init_* (spec, unused)
 *4 close_* (inst)
 *7 Procedure table mapping a string procedure name to procedure addresses.
 *7 These procedures pass a result back in id, giving the next available
 *7 vector id for this collection of vectors.
 *7 Return UNDEF if error, else 1.
 *8 Current possibilities are "next_vecid_1", "next_vecid", "empty"
***********************************************************************/
extern int init_next_vecid_1(), next_vecid_1(), close_next_vecid_1();
extern int init_next_vecid(), next_vecid(), close_next_vecid();
static PROC_TAB proc_next_vecid[] = {
    "next_vecid_1",	init_next_vecid_1,next_vecid_1,	close_next_vecid_1,
    "next_vecid",	init_next_vecid,next_vecid,	close_next_vecid,
    "empty",		INIT_EMPTY,	EMPTY,		CLOSE_EMPTY,
    };
static int num_proc_next_vecid = sizeof (proc_next_vecid) / sizeof (proc_next_vecid[0]);


/********************   PROCEDURE DESCRIPTION   ************************
 *0 Hierarchy table: procedures storing text location of document
 *1 index.textloc
 *2 * (doc, unused, inst)
 *3   SM_INDEX_TEXTDOC *doc;
 *3   char *unused;
 *3   int inst;
 *4 init_* (spec, unused)
 *4 close_* (inst)
 *7 Procedure table mapping a string procedure name to procedure addresses.
 *7 These procedures store location information of a document (pathname
 *7 plus byte offset of start and end of document) as well as the title of
 *7 the document.
 *7 Return UNDEF if error, else 1.
 *8 Current possibilities are "add_textloc", "empty"
***********************************************************************/
extern int init_add_textloc(), add_textloc(), close_add_textloc();
static PROC_TAB proc_addtextloc[] = {
    "add_textloc",		init_add_textloc,  add_textloc,	close_add_textloc,
    "empty",		INIT_EMPTY,	EMPTY,		CLOSE_EMPTY,
    };
static int num_proc_addtextloc =
    sizeof (proc_addtextloc) / sizeof (proc_addtextloc[0]);


/********************   PROCEDURE DESCRIPTION   ************************
 *0 Hierarchy table: procedures to break full document text into tokens
 *1 index.token
 *2 * (pp_doc, t_doc, inst)
 *3   SM_INDEX_TEXTDOC *pp_doc;
 *3   SM_TOKENDOC *t_doc;
 *3   int inst;
 *4 init_* (spec, unused)
 *4 close_* (inst)
 *7 Procedure table mapping a string procedure name to procedure addresses.
 *7 These procedures break a preparsed text down into tokens, recognizing
 *7 and returning various classes of tokens.  NO LONGER USED
 *7 Return UNDEF if error, else 1.
 *8 Current possibilities are "empty"
***********************************************************************/
/* extern int init_std_token(), std_token(), close_std_token(); */
static PROC_TAB proc_token[] = {
/*    "std_token",	init_std_token,	std_token,	close_std_token, */
      "empty",          INIT_EMPTY,     EMPTY,          CLOSE_EMPTY,
    };
static int num_proc_token = sizeof (proc_token) / sizeof (proc_token[0]);

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Hierarchy table: procedures to break text of a document section into tokens
 *1 index.token_sect
 *2 * (pp_doc, t_doc, inst)
 *3   SM_BUF *pp_doc;
 *3   SM_TOKENSECT *t_doc;
 *3   int inst;
 *4 init_* (spec, unused)
 *4 close_* (inst)
 *7 Procedure table mapping a string procedure name to procedure addresses.
 *7 These procedures break a preparsed text down into tokens, recognizing
 *7 and returning various classes of tokens.  The text corresponding to a
 *7 single section is done at a time.
 *7 Return UNDEF if error, else 1.
 *8 Current possibilities are "token_sect"
***********************************************************************/
extern int init_token_sect(), token_sect(), close_token_sect();
static PROC_TAB proc_token_sect[] = {
    "token_sect",     init_token_sect,	token_sect,	close_token_sect,
    };
static int num_proc_token_sect =
    sizeof (proc_token_sect) / sizeof (proc_token_sect[0]);


/********************   PROCEDURE DESCRIPTION   ************************
 *0 Hierarchy table: procedures to parse a tokenized document
 *1 index.parse
 *2 * (t_doc, p_doc, inst)
 *3   SM_TOKENDOC *t_doc;
 *3   SM_CONDOC *p_doc;
 *3   int inst;
 *4 init_* (spec, unused)
 *4 close_* (inst)
 *7 Procedure table mapping a string procedure name to procedure addresses.
 *7 These procedures take a tokenized document/query (list of tokens), 
 *7 and make a list of concept numbers and concept locations for that doc.
 *7 The entire document is parsed.  NO LONGER USED
 *7 Return UNDEF if error, else 1.
 *8 Current possibilities are "empty"
***********************************************************************/
extern int init_std_parse(), std_parse(), close_std_parse();
static PROC_TAB proc_parse[] = {
/*    "std_parse",	init_std_parse,	std_parse,	close_std_parse, */
      "empty",          INIT_EMPTY,     EMPTY,          CLOSE_EMPTY,
    };
static int num_proc_parse = sizeof (proc_parse) / sizeof (proc_parse[0]);


/********************   PROCEDURE DESCRIPTION   ************************
 *0 Hierarchy table: procedures to help parse a section from tokenized document
 *1 index.parse_sect
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
 *8 Current possibilities are "full", "name", "asis", "none", "adj", "full_all"
***********************************************************************/
extern int init_full_parse(), full_parse(), close_full_parse();
extern int init_full_all_parse(), full_all_parse(), close_full_all_parse();
extern int init_adj_parse(), adj_parse(), close_adj_parse();
extern int init_name_parse(), name_parse(), close_name_parse();
extern int init_asis_parse(), asis_parse(), close_asis_parse();
extern int  no_parse();
static PROC_TAB proc_parse_sect[] = {
    "full",		init_full_parse, full_parse,	close_full_parse,
    "full_all",	init_full_all_parse,     full_all_parse,close_full_all_parse,
    "adj",		init_adj_parse,  adj_parse,	close_adj_parse,
    "name",		init_name_parse, name_parse,	close_name_parse,
    "asis",		init_asis_parse, asis_parse,	close_asis_parse,
    "none",	        INIT_EMPTY,      no_parse,	CLOSE_EMPTY,
    };
static int num_proc_parse_sect = sizeof (proc_parse_sect) / sizeof (proc_parse_sect[0]);


/********************   PROCEDURE DESCRIPTION   ************************
 *0 Hierarchy table: procedures to determine if token is a stopword
 *1 index.stop
 *2 * (word, stopword_flag, inst)
 *3   char *word;
 *3   int *stopword_flag;
 *3   int inst;
 *4 init_* (spec, unused)
 *4 close_* (inst)
 *7 Procedure table mapping a string procedure name to procedure addresses.
 *7 These procedures take a single token, and determine if this is a common
 *7 word, presumably to be ignored if it is.  Stopword_flag is set to 1
 *7 if a common word, set to 0 otherwise.
 *7 Return UNDEF if error, 0 otherwise
 *8 Current possibilities are "stop_dict", "none"
***********************************************************************/
extern int init_stop_dict(), stop_dict(), close_stop_dict();
extern int stop_no();
static PROC_TAB proc_stop[] = {
    "stop_dict",	init_stop_dict,	stop_dict,	close_stop_dict,
    "none",		INIT_EMPTY,	stop_no,	CLOSE_EMPTY,
    };
static int num_proc_stop = sizeof (proc_stop) / sizeof (proc_stop[0]);


/********************   PROCEDURE DESCRIPTION   ************************
 *0 Hierarchy table: procedures to stem a concept token
 *1 index.stem
 *2 * (word, output_word, inst)
 *3   char *word;
 *3   char **output_word;
 *3   int inst;
 *4 init_* (spec, unused)
 *4 close_* (inst)
 *7 Procedure table mapping a string procedure name to procedure addresses.
 *7 These procedures take a single token, and return a pointer to the start
 *7 of the corresponding stemmed token (appropriate suffixes removed).
 *7 Return UNDEF if error, 1 otherwise
 *8 Current possibilities are "triestem", "remove_s", "none"
***********************************************************************/
extern int init_triestem(), triestem(), close_triestem();
extern int init_remove_s(), remove_s(), close_remove_s();
extern int stem_no();
static PROC_TAB proc_stem[] = {
    "triestem",		init_triestem,	triestem,	close_triestem,
    "remove_s",		init_remove_s,	remove_s,	close_remove_s,
    "none",		INIT_EMPTY,	stem_no,	CLOSE_EMPTY,
    };
static int num_proc_stem = sizeof (proc_stem) / sizeof (proc_stem[0]);


/********************   PROCEDURE DESCRIPTION   ************************
 *0 Hierarchy table: procedures to take a token and return a concept number
 *1 index.token_to_con
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
 *8 Current possibilities are "dict", "dict_noss", "empty"
***********************************************************************/
extern int init_tokcon_dict(), tokcon_dict(), close_tokcon_dict();
extern int init_tokcon_dict_noss(), tokcon_dict_noss(), close_tokcon_dict_noss();
static PROC_TAB proc_tocon[] = {
    "dict",	init_tokcon_dict,	tokcon_dict,	close_tokcon_dict,
    "dict_noss",init_tokcon_dict_noss, tokcon_dict_noss,close_tokcon_dict_noss,
    "empty",		INIT_EMPTY,	EMPTY,		CLOSE_EMPTY
    };
static int num_proc_tocon = sizeof (proc_tocon) / sizeof (proc_tocon[0]);


/********************   PROCEDURE DESCRIPTION   ************************
 *0 Hierarchy table: procedures to take concept number, and find the token
 *1 index.con_to_token
 *2 * (con, word, inst)
 *3   long *con;
 *3   char **word;
 *3   int inst;
 *4 init_* (spec, unused)
 *4 close_* (inst)
 *7 Procedure table mapping a string procedure name to procedure addresses.
 *7 These procedures take a single concept number and find the token string
 *7 that the concept number represents.
 *7 Return UNDEF if error, 1 otherwise
 *8 Current possibilities are "contok_dict", "empty"
***********************************************************************/
/* NOTE. all procedures below also entered in convert.low proc table */
extern int init_contok_dict(), contok_dict(), close_contok_dict();
static PROC_TAB proc_fromcon[] = {
    "contok_dict",	init_contok_dict,contok_dict,	close_contok_dict,
    "empty",		INIT_EMPTY,	EMPTY,		CLOSE_EMPTY
    };
static int num_proc_fromcon = sizeof (proc_fromcon) / sizeof (proc_fromcon[0]);


/********************   PROCEDURE DESCRIPTION   ************************
 *0 Hierarchy table: procedures to create a vector from a list of concepts
 *1 index.makevec
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
extern int init_makevec(), makevec(), close_makevec();
static PROC_TAB proc_makevec[] = {
    "makevec",		init_makevec,	makevec,	close_makevec,
    };
static int num_proc_makevec = sizeof (proc_makevec) / sizeof (proc_makevec[0]);


/********************   PROCEDURE DESCRIPTION   ************************
 *0 Hierarchy table: procedures to expand a vector.
 *1 index.expand
 *2 * (tf_vec, exp_vec, inst)
 *3   SM_VECTOR *tf_vec;
 *3   SM_VECTOR *exp_vec;
 *3   int inst;
 *4 init_* (spec, unused)
 *4 close_* (inst)
 *7 Procedure table mapping a string procedure name to procedure addresses.
 *7 These procedures take an indexed vector (tf weights) and create a new
 *7 expanded vector by incorporating additional information.
 *7 Return UNDEF if error, 1 otherwise
 *8 Current possibilities are "none"
***********************************************************************/
extern int init_expand_no(), expand_no(), close_expand_no();
static PROC_TAB proc_expand[] = {
    "none",		init_expand_no,	expand_no,	close_expand_no,
    };
static int num_proc_expand = sizeof (proc_expand) / sizeof (proc_expand[0]);


/********************   PROCEDURE DESCRIPTION   ************************
 *0 Hierarchy table: procedures to store an indexed vector in collection
 *1 index.store
 *2 * (vec, ignored, inst)
 *3   SM_VECTOR *vec;
 *3   char *ignored;
 *3   int inst;
 *4 init_* (spec, unused)
 *4 close_* (inst)
 *7 Procedure table mapping a string procedure name to procedure addresses.
 *7 These procedures take a vector and store it in collection database
 *7 files.
 *7 Return UNDEF if error, 1 otherwise
 *8 Current possibilities are "store_vec", "store_vec_aux", 
 *8 "store_aux", "empty"
***********************************************************************/
extern int init_store_vec(), store_vec(), close_store_vec();
extern int init_store_aux(), store_aux(), close_store_aux();
extern int init_store_vec_aux(), store_vec_aux(), close_store_vec_aux();
static PROC_TAB proc_store[] = {
    "store_vec",	init_store_vec,	store_vec,	close_store_vec,
    "store_vec_aux",	init_store_vec_aux,store_vec_aux,close_store_vec_aux,
    "store_aux",	init_store_aux,	store_aux,	close_store_aux,
    "empty",		EMPTY,		EMPTY,		EMPTY
    };
static int num_proc_store = sizeof (proc_store) / sizeof (proc_store[0]);


/********************   PROCEDURE DESCRIPTION   ************************
 *0 Hierarchy table: table of other hierarchy tables for indexing procedures
 *1 index
 *7 Procedure table mapping a string procedure table to either another
 *7 table of hierarchy tables, or to a stem table which maps 
 *7 names to procedures
 *8 Current possibilities are "index", "vec_index", "index_pp",
 *8 "preparse", "parts_preparse", "next_vecid", "addtextloc", "token",
 *8 "parse", "parse_sect", "stop", "stem", "tocon", "fromcon", "makevec",
 *8 "expand", "store"
***********************************************************************/
TAB_PROC_TAB proc_index[] = {
    "top",            TPT_PROC,       NULL,   topproc_index,
                        &num_topproc_index,
    "vec_index",        TPT_PROC,       NULL,   proc_vec_index,
                        &num_proc_vec_index,
    "index_pp",		TPT_PROC,	NULL,	proc_index_pp,	
                        &num_proc_index_pp,
    "preparse",		TPT_PROC,	NULL,	proc_preparse,	
                        &num_proc_preparse,
    "parts_preparse",	TPT_PROC,	NULL,	proc_parts_preparse,
                        &num_proc_parts_preparse,
    "next_vecid",	TPT_PROC,	NULL,	proc_next_vecid,	
                        &num_proc_next_vecid,
    "addtextloc",	TPT_PROC,	NULL,	proc_addtextloc, 
			&num_proc_addtextloc,
    "token",		TPT_PROC,	NULL,	proc_token,	
			&num_proc_token,
    "token_sect",	TPT_PROC,	NULL,	proc_token_sect,	
			&num_proc_token_sect,
    "parse",		TPT_PROC,	NULL,	proc_parse,	
			&num_proc_parse,
    "parse_sect",	TPT_PROC,	NULL,	proc_parse_sect,	
			&num_proc_parse_sect,
    "stop",		TPT_PROC,	NULL, 	proc_stop,	
			&num_proc_stop,
    "stem",		TPT_PROC,	NULL, 	proc_stem,	
			&num_proc_stem,
    "token_to_con",	TPT_PROC,	NULL, 	proc_tocon,	
			&num_proc_tocon,
    "con_to_token",	TPT_PROC,	NULL, 	proc_fromcon,	
			&num_proc_fromcon,
    "makevec",		TPT_PROC,	NULL, 	proc_makevec,	
			&num_proc_makevec,
    "expand",		TPT_PROC,	NULL, 	proc_expand,	
			&num_proc_expand,
    "store",		TPT_PROC,	NULL, 	proc_store,	
			&num_proc_store,
    };

int num_proc_index = sizeof (proc_index) / sizeof (proc_index[0]);
