#ifndef RR_VECH
#define RR_VECH
/*        $Header: /home/smart/release/src/h/rr_vec.h,v 11.0 1992/07/21 18:18:51 chrisb Exp $*/

typedef struct {
    long  did;          /* document id */
    long  rank;         /* Rank of this document */
    float sim;          /* similarity of did to qid */
} RR_TUP;

typedef struct {
    long  qid;          /* query id */
    long  num_rr;       /* Number of tuples for rr_vec */
    RR_TUP *rr;         /* tuples.  Invariant: rr sorted increasing did */
} RR_VEC;

#endif /* RR_VECH */
