#ifndef INVH
#define INVH
/*  $Header: /home/smart/release/src/h/inv.h,v 11.0 1992/07/21 18:18:41 chrisb Exp $ */

typedef struct {
    long  list;
    float wt;
} LISTWT;

/* a single node visible to the user */
typedef struct {
    long  id_num;           /* key to access this inverted list with*/
    long  num_listwt;       /* Number of elements in this list */
    LISTWT *listwt;         /* Pointer to actual list */
} INV;


/* OLD a single node visible to the user */
typedef struct {
        long  key_num;          /* key to access this inverted list with*/
        long  num_list;         /* Number of elements in this list */
        long  *list;            /* pointer to list elements  */
        float *list_weights;    /* Pointer to weights for corresponding */
                                /* elements */
} OLDINV2;

#define REL_INV 10
#endif /* INVH */
