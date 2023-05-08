#ifndef INTERH
#define INTERH
/*        $Header: /home/smart/release/src/h/inter.h,v 11.0 1992/07/21 18:18:41 chrisb Exp $*/

#include "proc.h"
#include "buf.h"
#include "vector.h"
#include "retrieve.h"
#include "spec.h"
#include "docdesc.h"
#include "textloc.h"

/* Maximum number of tokens on user supplied command line */
#define MAX_COMMAND_LINE 15

/* Variables describing current state of interactive procedure */
typedef struct {
    RETRIEVAL retrieval;     /* Pointer to current query vector and
                                top docs retrieved for it */
    long current_doc;        /* rank of doc_id last shown to user */
    SM_BUF output_buf;       /* Valid output to be shown user */
    SM_BUF err_buf;          /* Error output to be shown user */
    char *command_line[MAX_COMMAND_LINE];  /* Current user command, broken 
                                down into tokens. */
    long num_command_line;   /* number of tokens in current command_line */
    int raw_doc_flag;        /* Whether raw doc is to be shown user instead
                                of formatted doc */
    int verbose_level;
    TEXTLOC query_textloc;   /* Pointer to last interactive query text */
    SPEC_LIST spec_list;      /* When examining experimental results, the
                                 list of runs */
    SPEC *current_spec;       /* Current spec */

} INTER_STATE;

int init_inter_util(), close_inter_util();

int inter_get_docid(), inter_get_textloc(), inter_get_pp_sect();
int inter_get_sect_pp_all(), inter_get_sect_veclist();
int reset_is(), inter_prepare_titles();
#endif /* INTERH */
