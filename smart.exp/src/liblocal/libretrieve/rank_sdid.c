#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/liblocal/libretrieve/rank_sdid.c,v 11.0 1992/07/21 18:22:31 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Keep track of top SIGNED ranked documents during a retrieval.
 *1 retrieve.rank_tr.rank_sdid
 *2 rank_sdid (new, results, inst)
 *3   TOP_RESULT *new;
 *3   RESULT *results;
 *3   int inst;
 *7 Add the new document to results.top_results if the similarity is
 *7 high enough.  Break ties among similarities by ranking the document
 *7 with highest docid first.  Unlike rank_did, this routine allows
 *7 similarities to be negative.
 *9 WARNING: SHOULD NOT BE USED.  Unless you know what you're doing, this
 *9 can easily introduce inconsistencies in the retrieval process (eg,
 *9 ranks of rel docs will not agree with ranks of top docs).
***********************************************************************/

#include "common.h"
#include "param.h"
#include "proc.h"
#include "retrieve.h"

#ifndef MINFLOAT
#define MINFLOAT ((float)1.40129846432481707e-45)
#endif /* MINFLOAT */

/*
 * Because the smart system is nasty about expecting similarities of 0
 * to be unused slots, we convert a 0 similarity to something just slightly
 * above 0.  It's stupid, but it works.
 */
int
rank_sdid (new, results, inst)
TOP_RESULT *new;
RESULT *results;
int inst;
{
    long num_wanted = results->num_top_results;
    float new_sim = new->sim==0.0 ? MINFLOAT : new->sim;

    TOP_RESULT *last_res = &(results->top_results[num_wanted - 1]);

    TOP_RESULT *new_res;   /* Position to put new doc */
    TOP_RESULT *old_res;   /* Position of doc that gets kicked out */
    TOP_RESULT *temp_ptr;

    if (num_wanted <= 0)
        return (0);

    /* Enter doc into top_results if similarity high enough. */
    if ( last_res->did != 0 )
	if (new_sim < last_res->sim ||
	    (new_sim == last_res->sim && 
	     new->did <= last_res->did))
	    /* Don't need to enter */
	    return (0);


    new_res = results->top_results;
    while (new_res->did !=0 &&
	   (new_sim < new_res->sim ||
	    (new_sim == new_res->sim && 
	     new->did < new_res->did)))
        new_res++;

    /* Find location of doc that is to move out of top_results */
    /* (It may be the case that did is already in top_results) */
    for (old_res = new_res;
         old_res <= last_res && old_res->did != 0 && old_res->did !=new->did;
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
    new_res->sim = new_sim;

    return (1);
}
