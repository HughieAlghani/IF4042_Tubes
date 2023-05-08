#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/liblocal/libfile/part_vec.c,v 11.0 1992/07/21 18:22:41 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "rel_header.h"
#include "vector.h"
#include "direct_type.h"
#include "io.h"
#include "part_vector.h"

/* ARGSUSED */
int
create_partvec (file_name, rh)
char *file_name;
REL_HEADER *rh;
{
    REL_HEADER temp_rh;
    temp_rh.type = S_RH_PARTVEC;
    return ( create_direct (file_name, &temp_rh));
}

int
open_partvec (file_name, mode)
char *file_name;
long mode;
{
    if (mode & SCREATE) {
        if (UNDEF == create_partvec (file_name, (REL_HEADER *) NULL)) {
            return (UNDEF);
        }
        mode &= ~SCREATE;
    }
    return ( open_direct (file_name, mode) );
}

int
seek_partvec (index, vector_entry)
int index;
PART_VEC *vector_entry;
{
    return ( seek_direct (index, (char *) vector_entry) );
}

int
read_partvec (index, vector_entry)
int index;
PART_VEC *vector_entry;
{
    return ( read_direct (index, (char *) vector_entry) );
}

int
write_partvec (index, vector_entry)
int index;
PART_VEC *vector_entry;
{
    return ( write_direct (index, (char *) vector_entry) );
}

int
close_partvec (index)
int index;
{
    return ( close_direct (index) );
}


int
destroy_partvec(filename)
char *filename;
{
    return(destroy_direct(filename));
}

int
rename_partvec(in_file_name, out_file_name)
char *in_file_name;
char *out_file_name;
{
    return(rename_direct(in_file_name, out_file_name));
}
