#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libretrieve/rank_did.c,v 11.0 1992/07/21 18:23:50 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Keep track of top ranked documents during course of a retrieval.
 *1 retrieve.rank_tr.rank_did
 *2 rank_did (new, results, inst)
 *3   TOP_RESULT *new;
 *3   RESULT *results;
 *3   int inst;
 *7 Add the new document to results.top_results if the similarity is
 *7 high enough.  Break ties among similarities by ranking the document
 *7 with highest docid first.
***********************************************************************/

#include "common.h"
#include "param.h"
#include "proc.h"
#include "retrieve.h"

int
rank_did (new, results, inst)
TOP_RESULT *new;
RESULT *results;
int inst;
{
    long num_wanted = results->num_top_results;

    TOP_RESULT *last_res = &(results->top_results[num_wanted - 1]);

    TOP_RESULT *new_res;   /* Position to put new doc */
    TOP_RESULT *old_res;   /* Position of doc that gets kicked out */
    TOP_RESULT *temp_ptr;

    if (num_wanted <= 0)
        return (0);

    /* Enter doc into top_results if similarity high enough. */
    if (new->sim < last_res->sim ||
        (new->sim == last_res->sim && 
         new->did <= last_res->did))
        /* Don't need to enter */
        return (0);


    new_res = results->top_results;
    while (new->sim < new_res->sim ||
           (new->sim == new_res->sim && 
            new->did < new_res->did))
        new_res++;

    /* Find location of doc that is to move out of top_results */
    /* (It may be the case that did is already in top_results) */
    for (old_res = new_res;
         old_res <= last_res && old_res->sim != 0.0 && old_res->did !=new->did;
         old_res++)
        ;
    if (old_res > last_res)
        old_res = last_res;

    /* Slide all docs between new_res and old_res down one space */
    for (temp_ptr = old_res-1; temp_ptr >= new_res; temp_ptr--) {
        (temp_ptr+1)->did = temp_ptr->did;
        (temp_ptr+1)->sim = temp_ptr->sim;
    }
    /* Add res_ptr to top_docs at position new_res */
    new_res->did = new->did;
    new_res->sim = new->sim;

    return (1);
}
