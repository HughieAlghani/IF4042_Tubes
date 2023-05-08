#ifndef SPECH
#define SPECH
/*        $Header: /home/smart/release/src/h/spec.h,v 11.0 1992/07/21 18:18:55 chrisb Exp $*/
/*        (c) Copyright 1990 Chris Buckley */


typedef struct {
    char *name;
    char *value;
    char *key_field;         /* Last field of name (this must match when
                                looking up parameters) */
} SPEC_TUPLE;

typedef struct {
    int num_list;
    SPEC_TUPLE *spec_list;
    int spec_id;             /* Unique nonzero id assigned to each SPEC
                                instantiation to aid in caching values */
} SPEC;

typedef struct {
    char *param;             /* Parameter name to look up in spec file */
    int (*convert) ();       /* Conversion routine to run on value */
                             /* corresponding to param */
    char *result;            /* Location of put result of conversion */
} SPEC_PARAM;


typedef struct {
    char **prefix;           /* Pointer to string to be prepended to */
                             /* param before lookup */
    char *param;             /* Base parameter name to look up in spec file */
    int (*convert) ();       /* Conversion routine to run on value */
                             /* corresponding to param */
    char *result;            /* Location of put result of conversion */
} SPEC_PARAM_PREFIX;


/* A list of pointers to spec_files and their associated file_names */
typedef struct {
    SPEC **spec;             /* spec[i] is a pointer to the i'th spec file */
    char **spec_name;        /* Original filename of spec[i] */
    int num_spec;            /* number of files in the list */
} SPEC_LIST;

#endif /* SPECH */




