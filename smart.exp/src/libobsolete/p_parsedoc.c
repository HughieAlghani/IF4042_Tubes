#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libobsolete/p_parsedoc.c,v 11.0 1992/07/21 18:22:58 chrisb Exp $";
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

/* Print a SM_INDEX_PARSEDOC relation to stdout */
void
print_parsedoc (parsedoc, unused)
SM_INDEX_PARSEDOC *parsedoc;
SM_BUF *unused;
{
    long i;
    for (i = 0; i < parsedoc->num_tokens; i++) {
        if (parsedoc->tokens[i].token != NULL) {
            (void) printf ("%s\t%d\n",
                    parsedoc->tokens[i].token,
                    parsedoc->tokens[i].tokentype);
        }
        else {
            (void) printf ("<NULL>\t%d\n", parsedoc->tokens[i].tokentype);
        }
    }
}
