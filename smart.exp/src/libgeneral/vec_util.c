#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libgeneral/vec_util.c,v 11.0 1992/07/21 18:21:11 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include "common.h"
#include "functions.h"
#include "vector.h"

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Copy internal arrays of vec to ensure they are not overwritten
 *2 save_vec (vec)
 *3   VEC *vec;
 *4 free_vec (vec)
 *7 Save_vec saves a vector for future use (remember that the contents of
 *7 internal arrays of an object are not guaranteed to exist past the next
 *7 invocation of the procedure that produced them).
 *7 free_vec frees a vector that was previously saved.
 *8 mallocs space for ctype_len and con_wtp, and copies values into them
***********************************************************************/
int 
save_vec (vec)
VEC *vec;
{
    long *ctype_buf;
    CON_WT *conwt_buf;

    if (vec->num_conwt == 0)
        return (0);
    if (NULL == (ctype_buf = (long *)
                 malloc ((unsigned) vec->num_ctype * sizeof (long))) ||
        NULL == (conwt_buf = (CON_WT *)
                 malloc ((unsigned) vec->num_conwt * sizeof (CON_WT)))) 
        return (UNDEF);

    bcopy ((char *) vec->ctype_len,
           (char *) ctype_buf,
           (int) vec->num_ctype * sizeof (long));
    bcopy ((char *) vec->con_wtp,
           (char *) conwt_buf,
           (int) vec->num_conwt * sizeof (CON_WT));

    vec->ctype_len = ctype_buf;
    vec->con_wtp = conwt_buf;
    return (1);
}


/* Free vecs that have been previously saved with save_vec */
int
free_vec (vec)
VEC *vec;
{
    if (vec->num_conwt == 0)
        return (0);
    (void) free ((char *) vec->ctype_len);
    (void) free ((char *) vec->con_wtp);

    return (1);
}


/********************   PROCEDURE DESCRIPTION   ************************
 *0 Copy entire vector orig_vec to dest_vec
 *2 copy_vec (dest_vec, orig_vec)
 *3   VEC *dest_vec;
 *3   VEC *orig_vec;
 *8 mallocs space for ctype_len and con_wtp, and copies values into them
***********************************************************************/
int 
copy_vec (dest_vec, orig_vec)
VEC *dest_vec;
VEC *orig_vec;
{
    long *ctype_buf = (long *) NULL;
    CON_WT *conwt_buf = (CON_WT *) NULL;

    if (orig_vec->num_conwt != 0) {
        if (NULL == (ctype_buf = (long *)
                   malloc ((unsigned) orig_vec->num_ctype * sizeof (long))) ||
            NULL == (conwt_buf = (CON_WT *)
                   malloc ((unsigned) orig_vec->num_conwt * sizeof (CON_WT)))) 
            return (UNDEF);
        
        bcopy ((char *) orig_vec->ctype_len,
               (char *) ctype_buf,
               (int) orig_vec->num_ctype * sizeof (long));
        bcopy ((char *) orig_vec->con_wtp,
               (char *) conwt_buf,
               (int) orig_vec->num_conwt * sizeof (CON_WT));
    }

    dest_vec->ctype_len = ctype_buf;
    dest_vec->con_wtp = conwt_buf;
    dest_vec->id_num = orig_vec->id_num;
    dest_vec->num_ctype = orig_vec->num_ctype;
    dest_vec->num_conwt = orig_vec->num_conwt;

    return (1);
}


/********************   PROCEDURE DESCRIPTION   ************************
 *0 Copy internal arrays of vec_list to ensure they are not overwritten
 *2 save_vec_list (vec_list)
 *3   VEC_LIST *vec_list;
 *4 free_vec_list (vec_list)
 *7 Save_vec_list saves a vector list for future use (remember that the 
 *7 contents of internal arrays of an object are not guaranteed to exist
 *7 past the next invocation of the procedure that produced them).
 *7 free_vec_list frees a vector_list that was previously saved.
 *8 Malloc space for both top value and internal arrays of each
 *8 vector in vec_list.
***********************************************************************/
int
save_vec_list (vec_list)
VEC_LIST *vec_list;
{
    long i;
    VEC *vec_buf;

    if (NULL == (vec_buf = (VEC *)
                 malloc ((unsigned) vec_list->num_vec * sizeof (VEC))))
        return (UNDEF);

    bcopy ((char *) vec_list->vec,
           (char *) vec_buf,
           (int) vec_list->num_vec * sizeof (VEC));

    vec_list->vec = vec_buf;

    for (i = 0; i < vec_list->num_vec; i++)
        if (UNDEF == save_vec (&vec_list->vec[i]))
            return (UNDEF);

    return (1);
}


int
free_vec_list (vec_list)
VEC_LIST *vec_list;
{
    long i;
    for (i = 0; i < vec_list->num_vec; i++)
        if (UNDEF == free_vec (&vec_list->vec[i]))
            return (UNDEF);
    (void) free ((char *) vec_list->vec);

    return (1);
}

