#ifndef SIMP_INVH
#define SIMP_INVH
/*  $Header: /home/smart/release/src/h/simp_inv.h,v 11.0 1992/07/21 18:18:52 chrisb Exp $ */

/* a single node visible to the user */
typedef struct {
        long  id_num;           /* key to access this inverted list with*/
        long  num_list;         /* Number of elements in this list */
        long  *list;            /* pointer to list elements  */
} SIMP_INV;

#define REL_SIMP_INV 4
#endif /* SIMP_INVH */
