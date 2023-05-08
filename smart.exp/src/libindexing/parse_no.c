#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libindexing/parse_no.c,v 11.0 1992/07/21 18:21:21 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/
/********************   PROCEDURE DESCRIPTION   ************************
 *0 Skip through tokens for this entire section
 *1 index.parse_sect.none
 *2 no_parse (token, consect, inst)
 *3   SM_TOKENSECT *token;
 *3   SM_CONSECT *consect;
 *3   int inst;
 *7 Always return 0;  Set consect->num_concepts to 0;
***********************************************************************/


#include "common.h"
#include "param.h"
#include "functions.h"
#include "smart_error.h"
#include "proc.h"
#include "spec.h"
#include "docindex.h"

int
no_parse (token, consect, inst)
SM_TOKENSECT *token;
SM_CONSECT *consect;
int inst;
{
    consect->num_concepts = 0;
    consect->concepts = (SM_CONLOC *) NULL;

    return (0);
}

