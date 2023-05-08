#ifndef SECT_RETH
#define SECT_RETH
/*        $Header: /home/smart/release/src/h/section.h,v 11.0 1992/07/21 18:18:51 chrisb Exp $*/

#include "docindex.h"
#include "context.h"

typedef struct {
    long sect1;                     /* Section index of first document */
    long sect2;                     /* Section index of second document */
    float sim;                     /* Similarity (non-zero) of the sections */
} SECT_TUP_RESULT;

typedef struct {
    SM_INDEX_TEXTDOC *did1;
    SM_INDEX_TEXTDOC *did2;

    SECT_TUP_RESULT *sect_results;  /* Non-zero similarities */
    long num_sect_results;

    long num_sect_did1;   /* Number of non-empty (after indexing!) sections */
    long num_sect_did2;
} SECT_RESULT;

#define CTXT_SECT (CTXT_LOCAL | 1 << 16)
#define CTXT_PARA (CTXT_LOCAL | 1 << 17)
#define CTXT_SENT (CTXT_LOCAL | 1 << 18)

#endif /* SECT_RETH */
