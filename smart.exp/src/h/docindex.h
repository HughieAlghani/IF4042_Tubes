#ifndef DOCINDEXH
#define DOCINDEXH
/*  $Header: /home/smart/release/src/h/docindex.h,v 11.0 1992/07/21 18:18:37 chrisb Exp $ */

#include "sm_display.h"
#include "textloc.h"
#include "proc.h"
#include "vector.h"

/* Used by preparser, adddisplay, parser to give the original text of the
   doc broken into sections */

typedef struct {
    long id_num;
    char *doc_text;
    TEXTLOC textloc_doc;         /* section occurrence info relating to 
                                    actual text of doc in file */
    SM_DISPLAY mem_doc;          /* section occurrence info relating to
                                    in memory doc (ie doc_text) */
} SM_INDEX_TEXTDOC;

/* Output of tokenizer, giving the tokens found in the text of a doc, along 
   with the tokentype */
typedef struct {
    char *token;
    long tokentype;
} SM_TOKENTYPE;                     /* Individual token */

typedef struct {
    long section_num;               /* Index of the section_id for section */
    long num_tokens;
    SM_TOKENTYPE *tokens;
} SM_TOKENSECT;                     /* Tokens for a section */

typedef struct {      
    long id_num;
    long num_sections;
    SM_TOKENSECT *sections;
} SM_TOKENDOC;                      /* Tokens for a document */

/* Old */
typedef struct {
    long id_num;
    long num_tokens;
    SM_TOKENTYPE *tokens;
} SM_INDEX_PARSEDOC;

/* Tokentype can take on one of the following values in the standard parser */
/* Other parsers may use other values in addition.  By convention, positive
   values indicate that particular tokentype should be used as ctype */
/* Token is all lower case letters */
#define SM_INDEX_LOWER -1
/* Token is mixed case (Combination of upper, lower, digits)  (possibly
   including '.', '!', ''' */
#define SM_INDEX_MIXED -2
/* Token is all digits (possibly including '.') */
#define SM_INDEX_DIGIT -3
/* Punctuation */
#define SM_INDEX_PUNC -4
/* Whitespace, not including newline */
#define SM_INDEX_SPACE -5
/* Whitespace, including newline */
#define SM_INDEX_NLSPACE -6
/* End of Sentence (Token = NULL) */
#define SM_INDEX_SENT -7
/* End of Paragraph (Token = NULL) */
#define SM_INDEX_PARA -8
/* Start of new section (Token[0] = section_id) */
#define SM_INDEX_SECT_START -9
/* End of section (Token[0] = section_id) */
#define SM_INDEX_SECT_END -10
/* Ignore this token completely */
#define SM_INDEX_IGNORE -11


/* Output of parser */
typedef struct {
    long con;                   /* Concept number of input token */
    long ctype;                 /* Ctype of con */
    long para_num;              /* Paragraph within section */
    long sent_num;              /* Sentence number within section */
    long token_num;             /* Token number within section */
} SM_CONLOC;

typedef struct {
    long num_concepts;
    SM_CONLOC *concepts;
} SM_CONSECT;

typedef struct {
    long id_num;
    long num_sections;
    SM_CONSECT *sections;
} SM_CONDOC;


/* Old */
typedef struct {
    long con;                   /* Concept number of input token */
    long ctype;                 /* Ctype of con */
    long sect_num;              /* Section within document */
    long para_num;              /* Paragraph within document */
    long sent_num;              /* Sentence number within document */
    long word_num;              /* Word number within document */
} SM_INDEX_CONLOC;

typedef struct {
    long id_num;
    long num_concepts;
    SM_INDEX_CONLOC *concepts;
} SM_INDEX_CONDOC;

#include "docdesc.h"
/* intermediate structure used to return token map from section parser to
   parser - OLD */
typedef struct {
    SM_TOKENTYPE *last_token;   /* Last token looked at to get new token map */
    char *token_buf;            /* Actual token to be operated on */
    int parse_type;             /* Index in std_parsetypes of the type of
                                   token recognized */
    SM_INDEX_CONLOC *conloc;    /* Place for section parser to put
                                   location info */
} SM_SEC_PARSE;

#endif /* DOCINDEXH */
