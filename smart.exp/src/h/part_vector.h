#ifndef PART_VECTORH
#define PART_VECTORH
/*  $Header: /home/smart/release/src/h/part_vector.h,v 11.0 1992/07/21 18:19:04 chrisb Exp $ */

#include "retrieve.h"

typedef struct {
    long con;                   /* Actual concept number */
    float wt;                   /* and its weight */
    long partnum;		/* part number this concept appears in */
} PART_CON_WT;

/* a single vector, sorted on: ctypes,cons */
typedef struct {
        long   id_num;          /* unique number for this vector within  */
                                /* collection */
	long   max_partnum;	/* largest partnum value that occurs */
	long   partnums_used;	/* number of distinct partnums listed */
        long   num_ctype;       /* number of ctypes for this vector */
        long   num_part_conwt;  /* no. of tuples in the vector */
        long   *ctype_len;      /* length of subvector for each ctype */
        PART_CON_WT *part_con_wtp;  /* pointer to concepts, wts for vector*/

} PART_VEC;

/* Multiple vectors */
typedef struct {
    PART_VEC *pvec;
    long num_pvec;
    long id_num;
} PART_VEC_LIST;

typedef struct {
    PART_VEC_LIST *pvec_list1;
    PART_VEC_LIST *pvec_list2;
} PART_VEC_LIST_PAIR;

typedef struct {
    PART_VEC *pvec1;
    PART_VEC *pvec2;
} PART_VEC_PAIR;

typedef struct {
    long num_results;
    RESULT_TUPLE *results;
    RESULT_TUPLE **res_ptr;/* elt i*maxrow+j points to which results[] used */
} PART_VEC_RESULTS;

typedef PART_VEC SM_PART_VECTOR;

#define REL_PART_VEC 14

int create_partvec(), open_partvec(), seek_partvec(), read_partvec(),
        write_partvec(), close_partvec(), destroy_partvec(), rename_partvec();
void print_partvec();

#endif /* PART_PARTVECH */
