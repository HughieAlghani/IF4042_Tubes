#ifndef OLDINVH
#define OLDINVH
/*  $Header: /home/smart/release/src/h/oldinv.h,v 11.0 1992/07/21 18:18:44 chrisb Exp $ */

/* a single node visible to the user */
typedef struct {
        long  key_num;          /* key to access this inverted list with*/
        unsigned short num_list; /* Number of elements in this list */
        long  *list;            /* pointer to list elements  */
        float *list_weights;    /* Pointer to weights for corresponding */
                                /* elements */
} OLDINV;

#define REL_OLDINV 1
#endif /* OLDINVH */
