#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libobsolete/rr.c,v 11.0 1992/07/21 18:22:47 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include "common.h"
#include "param.h"
#include "rel_header.h"
#include "rr.h"
#include "io.h"

_SRR_FILES _Srel_ranks[MAX_NUM_RR];
