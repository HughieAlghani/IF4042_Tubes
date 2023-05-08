#ifndef VECTORH
#define VECTORH
/*  $Header: /home/smart/release/src/h/vector.h,v 11.0 1992/07/21 18:19:00 chrisb Exp $ */

typedef struct {
    long con;                   /* Actual concept number */
    float wt;                   /* and its weight */
} CON_WT;

/* a single vector, sorted on: ctypes,cons */
typedef struct {
        long   id_num;          /* unique number for this vector within  */
                                /* collection */
        long   num_ctype;       /* number of ctypes for this vector */
        long   num_conwt;       /* no. of tuples in the vector */
        long   *ctype_len;      /* length of subvector for each ctype */
        CON_WT *con_wtp;        /* pointer to concepts, weights for vector*/

} VEC;

/* Multiple vectors */
typedef struct {
    VEC *vec;
    long num_vec;
    long id_num;
} VEC_LIST;

typedef struct {
    VEC_LIST *vec_list1;
    VEC_LIST *vec_list2;
} VEC_LIST_PAIR;

typedef struct {
    VEC *vec1;
    VEC *vec2;
} VEC_PAIR;

typedef VEC SM_VECTOR;

#define REL_VECTOR 2
#endif /* VECTORH */
