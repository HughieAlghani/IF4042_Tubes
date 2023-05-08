#ifndef REL_HEADERH
#define REL_HEADERH
/* $Header: /home/smart/release/src/h/rel_header.h,v 11.0 1992/07/21 18:18:49 chrisb Exp $ */

typedef struct {
    long  num_entries;              /* Number of entries in this relation */
    long  max_primary_value;        /* Current maximum possible value for the*/
                                    /* attribute relation sorted upon */
    long  max_secondary_value;      /* Same for secondary attribute */
    unsigned char type;             /* Relation type (see below) */
    unsigned char data_type;        /* Low-level implementation method */
    short _entry_size;              /* Number of bytes per fixed record */
                                    /* part of the tuple (implementation */
                                    /* dependent) */
    long  _internal;                /* unused at present */
} REL_HEADER;

/* Different types of relations currently used in SMART */
/* Currently, types 0-31  are reserved for "direct" datatype implementation */
/*            types 32-63 are reserved for "sorted" (NOT YET IMPLEMENTED) */
/*            types 64-95 are not yet used */
/*            types 96-127 are "unique" datatypes (No separate low level */
/*                         implementation in common with anything else)  */
/*            types 127-255 are not yet used */
#define S_RH_GRAPH 0
#define S_RH_OLDINV 1
#define S_RH_VEC   2
#define S_RH_DISP  3
#define S_RH_SINV  4
#define S_RH_HIER  5
#define S_RH_ARRAY 6
#define S_RH_PNORM 7
#define S_RH_AVEC  8
#define S_RH_AINV  9
#define S_RH_INV   10
#define S_RH_TEXT  11
#define S_RH_TRVEC 12
#define S_RH_RRVEC 13
#define S_RH_PARTVEC 14

#define S_RH_RR    32
#define S_RH_TR    33
#define S_RH_EVAL  34

#define S_RH_DICT  96
#define S_RH_LINV  97
#define S_RH_DICT_NOINFO  98

/* The datatype subfield gives the low-level implementation method */
#define S_RD_DIRECT 1
#define S_RD_SORT   2
#define S_RD_DICT   3
#define S_RD_LINV   4
#define S_RD_DICT_NOINFO   5

/* These methods should be replaced by "sorted" eventually */
#define S_RD_RR     32
#define S_RD_TR     33
#define S_RD_EVAL   34
#endif /* REL_HEADERH */
