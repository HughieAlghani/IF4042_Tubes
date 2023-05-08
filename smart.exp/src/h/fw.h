/******************************************************************
 *
 * Constants and functions used/defined by F&W encyclopedia preparsing.
 *
 ******************************************************************/
#ifndef FWH
#define FWH

#include "param.h"

/* Maximum length of a file's name */
#define FNAME_LEN PATH_LEN
#define MAX_LINE_LEN 500
#define MAX_SUBJS 10
#define MAX_OUTLEN 70
#define MAX_XREFS 200

#define TRUE 1
#define FALSE 0
typedef int Boolean;

typedef struct {
    int code;
    char *word; } SGML_CODE;
extern SGML_CODE sgml_codes[];	/* SGML codes and internal syms */
extern int sgml_COUNT; 		/* number of entries */

typedef struct {
    char *Symbol;
    char *Value; } SGML_CHAR;
extern SGML_CHAR trans[];	/* for `&xxx;' codes */

typedef struct {
    int	Code;
    Boolean End;
    int	Len;
    char Text[MAX_LINE_LEN]; } Word;

typedef struct {
    char Type[10];
    char Id[100];
    char Doc[FNAME_LEN];
    char Text[MAX_LINE_LEN];
    char Type_code;     /* one of Xtype_xxx for 'type=xxx' */
    char Syntax;        /* one of Xsyntax_xxx for 'syntax=xxx' */
    long FirstDiv;      /* fw_sec only: first division of the xref'd article */
    long LastDiv;       /* fw_sec only: last division of the xref'd article */
    long docid;		/* article number of xref'd article */
   } Xref;

#define CODE_start 'i'
#define CODE_end 'e'
#define CODE_id 'c'
#define CODE_volume 'v'
#define CODE_page 'p'
#define CODE_date 'd'
#define CODE_subject 'h'
#define CODE_displaytitle 'y'
#define CODE_title 't'
#define CODE_body 'b'
#define CODE_xref 'x'
#define CODE_xref_ids 'r'
#define CODE_category 'g'
#define CODE_subcat 's'
#define CODE_obscurity 'u'
#define CODE_bio 'z'

#define CR '\015'
#define LF '\012'
#define SPACE ' '
#define SKIP_CHAR(x) (x==(int)CR || x==(int)LF)
#define IS_WHITE_SPACE(x) (x==(int)SPACE || SKIP_CHAR(x))
#define IS_WORD_BREAK(x) (x==(int)'<' || IS_WHITE_SPACE(x))

/******************************************************************
 *
 * sgml.h
 * Describes the SGML commands that are recognized by the
 * parser.  Each one has a symbol associated with it.
 *
 ******************************************************************/

#define SYM_EOF		-1
#define SYM_TEXT	-2
#define SYM_UNKNOWN	0
#define SYM_FW_ART	1
#define SYM_PUBFM	2
#define SYM_VOL		3
#define SYM_PAGE	4
#define SYM_MDT		5
#define SYM_SUBJ	6
#define SYM_TI		7
#define SYM_TI_END	8
#define SYM_BDY		9
#define	SYM_BDY_END	10
#define SYM_P		11
#define SYM_SEC		12
#define SYM_SECA	13
#define SYM_SECB	14
#define SYM_SECC	15
#define SYM_XREF	16
#define SYM_XREF_ID	17

#define SYM_CATEGORY	18
#define SYM_SUBCATEGORY	19
#define SYM_OBSCURITY	20
#define SYM_BIO		21
#define SYM_SUBH	22
#define SYM_LIST	23
#define SYM_LISTELT	24
#define SYM_NAME	25
#define SYM_NORMNAME	25  /* yes, intended to be the same as NAME */

#define DATESYM_MONTH	101
#define DATESYM_DAY	102
#define DATESYM_YEAR	103


#define IS_SECTION_HDR(x) (x==SYM_BDY)


/* Special symbols and constants used to keep track of whether we want 
   sentences (or paragraphs) to indicate end of a section */
#define SYM_SENT 50
#define PARSE_ENTIRE 0
#define PARSE_PARA 1
#define PARSE_SENT 2
#define PARSE_SECT 3

/*
 * Some cross reference information
 */
#define Xtype_unknown   '?'
#define Xtype_see       's'
#define Xtype_see_also  'a'
#define Xtype_qv        'q'
#define Xtype_bib       'b'
#define Xtype_bio       'o'
#define Xtype_compare	'c'

#define Xsyntax_unknown '?'
#define Xsyntax_sep     's'
#define Xsyntax_embed   'e'
#define Xsyntax_parens  'p'
#define Xsyntax_other   'o'


#endif /* FWH */
