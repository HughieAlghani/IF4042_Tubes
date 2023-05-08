#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libinter/show_titles.c,v 11.0 1992/07/21 18:21:52 chrisb Exp $";
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
#include "proc.h"
#include "tr_vec.h"
#include "context.h"
#include "retrieve.h"
#include "docindex.h"
#include "inter.h"

/* Format of individual title line */
#define DID_WIDTH 6
#define ACT_OFFSET (DID_WIDTH+2)
#define REL_OFFSET (DID_WIDTH+3)
#define SIM_OFFSET (DID_WIDTH+6)
#define TITLE_OFFSET (DID_WIDTH+11)
#define TITLE_WIDTH 63
#define DISPLAY_WIDTH   80

typedef struct {
    SM_BUF menu_buf;
    long *entry_offset;       /* offset into menu_buf of ith entries title */
} MENU;

/* current titles */
static MENU title_menu = {{0, 0, (char *) NULL}, (long *) NULL};

static int compare_tr_rank();

int
inter_prepare_titles (is, unused)
INTER_STATE *is;
char *unused;
{
    long i;
    TEXTLOC textloc;
    int feedback_iter;
    char temp_buf[PATH_LEN];
    TR_VEC *tr_vec = is->retrieval.tr;

    title_menu.menu_buf.end = 0;
    if (UNDEF == add_buf_string ("Num  Action Sim Title\n",
                                 &title_menu.menu_buf))
        return (UNDEF);
    if (tr_vec->num_tr == 0) {
        if (UNDEF == add_buf_string ("No docs retrieved\n",
                                     &title_menu.menu_buf))
            return (UNDEF);
        return (0);
    }

    /* Reserve space for pointers into the full title display */
    if (NULL == title_menu.entry_offset) {
        if (NULL == (title_menu.entry_offset =
                     (long *) malloc ((unsigned) (tr_vec->num_tr + 1)
                                     * sizeof (long))))
            return (UNDEF);
    }
    else {
        if (NULL == (title_menu.entry_offset =
                     (long *) realloc ((char *) title_menu.entry_offset,
                                      (unsigned) (tr_vec->num_tr + 1)
                                      * sizeof (long))))
            return (UNDEF);
    }

    /* Sort incoming docs in decreasing similarity */
    qsort ((char *) tr_vec->tr,
           (int) tr_vec->num_tr,
           sizeof (TR_TUP), compare_tr_rank);

    feedback_iter = tr_vec->tr[0].iter;

    /* Add each doc's title to title menu */
    for (i = 0; i < tr_vec->num_tr; i++) {
        if (feedback_iter != tr_vec->tr[i].iter) {
            (void) sprintf (temp_buf,
                            "\nPreviously retrieved docs from iteration %d\n",
                            (int) tr_vec->tr[i].iter);
            if (UNDEF == add_buf_string (temp_buf, &title_menu.menu_buf))
                return (UNDEF);
            feedback_iter = tr_vec->tr[i].iter;
        }

        if (1 != inter_get_textloc (tr_vec->tr[i].did, &textloc))
            textloc.title = "Document not in database";

        (void) sprintf (temp_buf,
                        "%*d     %4.2f %.*s\n",
                        DID_WIDTH, tr_vec->tr[i].did,
                        tr_vec->tr[i].sim,
                        TITLE_WIDTH, textloc.title);
        title_menu.entry_offset[i] = title_menu.menu_buf.end;
        if (UNDEF == add_buf_string (temp_buf, &title_menu.menu_buf))
            return (UNDEF);
    }

    return (1);
}


int
show_titles (is, unused)
INTER_STATE *is;
char *unused;
{
    register long i;
    TR_VEC *tr_vec = is->retrieval.tr;

    if (tr_vec->num_tr == 0 || title_menu.entry_offset == NULL) {
        if (UNDEF == add_buf_string ("No documents to display",
                                     &is->err_buf))
            return (UNDEF);
        return (0);
    }

    /* Prepare action and rel judgement fields for display */
    for (i = 0; i < tr_vec->num_tr; i++) {
        title_menu.menu_buf.buf[title_menu.entry_offset[i] +
                                ACT_OFFSET] = tr_vec->tr[i].action ?
                                    tr_vec->tr[i].action : ' ';
        title_menu.menu_buf.buf[title_menu.entry_offset[i] +
                                REL_OFFSET] = tr_vec->tr[i].rel ? 'Y' : ' ';
    }

    if (UNDEF == add_buf (&title_menu.menu_buf, &is->output_buf))
        return (UNDEF);

    return (1);
}

static int
compare_tr_rank (tr1, tr2)
register TR_TUP *tr1;
register TR_TUP *tr2;
{
    if (tr1->iter != tr2->iter) {
        if (tr1->iter > tr2->iter)
            return (-1);
        return (1);
    }
    if (tr1->rank < tr2->rank) {
        return (-1);
    }
    if (tr1->rank > tr2->rank) {
        return (1);
    }
    return (0);
}

