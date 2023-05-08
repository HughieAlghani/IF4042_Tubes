#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libfile/addinfo.c,v 11.0 1992/07/21 18:20:45 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include "common.h"
#include "param.h"
#include "rel_header.h"
#include "inv.h"
#include "direct_type.h"
#include "vector.h"
#include "addinfo.h"
#include "io.h"

/* ARGSUSED */
create_ainv (file_name, rh)
char *file_name;
REL_HEADER *rh;
{
    REL_HEADER temp_rh;
    temp_rh.type = S_RH_AINV;
    return ( create_direct (file_name, &temp_rh));
}

open_ainv (file_name, mode)
char *file_name;
long mode;
{
    if (mode & SCREATE) {
        if (UNDEF == create_ainv (file_name, (REL_HEADER *) NULL)) {
            return (UNDEF);
        }
        mode &= ~SCREATE;
    }
    return ( open_direct (file_name, mode) );
}

seek_ainv (index, inv_entry)
int index;
ADDINV *inv_entry;
{
    return ( seek_direct (index, (char *) inv_entry) );
}

read_ainv (index, inv_entry)
int index;
ADDINV *inv_entry;
{
    return ( read_direct (index, (char *) inv_entry) );
}

write_ainv (index, inv_entry)
int index;
ADDINV *inv_entry;
{
    return ( write_direct (index, (char *) inv_entry) );
}

close_ainv (index)
int index;
{
    return ( close_direct (index) );
}

int
destroy_ainv (filename)
char *filename;
{
    return(destroy_direct(filename));
}

int
rename_ainv (in_file_name, out_file_name)
char *in_file_name;
char *out_file_name;
{
    return(rename_direct(in_file_name, out_file_name));
}

/* ARGSUSED */
create_avec (file_name, rh)
char *file_name;
REL_HEADER *rh;
{
    REL_HEADER temp_rh;
    temp_rh.type = S_RH_AVEC;
    return ( create_direct (file_name, &temp_rh));
}

open_avec (file_name, mode)
char *file_name;
long mode;
{
    if (mode & SCREATE) {
        if (UNDEF == create_avec (file_name, (REL_HEADER *) NULL)) {
            return (UNDEF);
        }
        mode &= ~SCREATE;
    }
    return ( open_direct (file_name, mode) );
}

seek_avec (index, vector_entry)
int index;
ADDVEC *vector_entry;
{
    return ( seek_direct (index, (char *) vector_entry) );
}

read_avec (index, vector_entry)
int index;
ADDVEC *vector_entry;
{
    return ( read_direct (index, (char *) vector_entry) );
}

write_avec (index, vector_entry)
int index;
ADDVEC *vector_entry;
{
    return ( write_direct (index, (char *) vector_entry) );
}

close_avec (index)
int index;
{
    return ( close_direct (index) );
}


int
destroy_avec(filename)
char *filename;
{
    return(destroy_direct(filename));
}

int
rename_avec(in_file_name, out_file_name)
char *in_file_name;
char *out_file_name;
{
    return(rename_direct(in_file_name, out_file_name));
}
