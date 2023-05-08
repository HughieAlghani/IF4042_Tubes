#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libobsolete/p_condoc.c,v 11.0 1992/07/21 18:22:57 chrisb Exp $";
#endif
/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "smart_error.h"
#include "display.h"
#include "docindex.h"
#include "buf.h"

/* Print a SM_INDEX_CONDOC relation to stdout */
void
print_condoc (condoc, unused)
SM_INDEX_CONDOC *condoc;
SM_BUF *unused;
{
    long i;
    for (i = 0; i < condoc->num_concepts; i++) {
        (void) printf ("%d\t%d\t%d\t%d\t%d\t%d\n",
                condoc->concepts[i].con,
                condoc->concepts[i].ctype,
                condoc->concepts[i].sect_num,
                condoc->concepts[i].para_num,
                condoc->concepts[i].sent_num,
                condoc->concepts[i].word_num);
    }
}
