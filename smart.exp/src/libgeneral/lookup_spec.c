#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libgeneral/lookup_spec.c,v 11.0 1992/07/21 18:21:07 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Lookup a set of parameters in a spec object, and set the values for them
 *2 lookup_spec (spec_ptr, spec_param, num_params)
 *3   SPEC *spec_ptr;
 *3   SPEC_PARAM *spec_param;
 *3   int num_params;
 *4 getspec_float
 *4 getspec_double
 *4 getspec_string
 *4 getspec_char
 *4 getspec_long
 *4 getspec_bool
 *4 getspec_filemode
 *4 getspec_dbfile
 *4 getspec_func

 *7 The list of parameters that need to be looked up is given by the array
 *7 of SPEC_PARAM tuples spec_param.  For each parameter, the calling
 *7 procedure specifies the request parameter name, a location for the
 *7 parameter value to be placed, and an interpreting procedure that will
 *7 take the string value found in the spec file for this parameter,
 *7 convert it into an object of the appropriate type, and place the value
 *7 in the designated location.
 *7 
 *7 The function lookup_spec takes the spec file pairs given by
 *7 "spec", and the SPEC_PARAM parameter list, and then finds,
 *7 converts and stores the appropriate values.
 *7 
 *7 The list of system supplied interpretation procedures, along with
 *7 the data type they expect to fill in, are
 *7 
 *7 getspec_float     float
 *7 getspec_double    double
 *7 getspec_string    char *
 *7 getspec_intstring char *   Same as getspec_string, except recognizes
 *7                            standard backslash escape sequences. Note:
 *7                            use sparingly since this mallocs unreclaimable
 *7                            space.
 *7 getspec_char      char
 *7 getspec_bool      int      Recognizes true,on,1,false,off,0
 *7 getspec_long      long
 *7 getspec_filemode  long     Recognizes smart io flags (see h/io.h)
 *7                            possibly several separated by |
 *7 getspec_dbfile    char *   Recognizes smart database filename.
 *7                            If string value is not explictly
 *7                            located elsewhere (by beginning with a
 *7                            '/' or '.') it is assumed to relative
 *7                            to the database name given by the
 *7                            parameter "database".
 *7 getspec_func   PROC_TAB *  Interprets the parameter value as the
 *7                            name of a procedure set.  This name is
 *7                            looked up in a procedure hierarchy to
 *7                            find the corresponding procedures
 *7                            (there'll be an initialization procedure,
 *7                            a main procedure and a closing procedure)
 *7                            See documentation for procedures for
 *7                            more information.
 *7     
 *7     
 *7 PARAMETER_NAME
 *7 The parameter name is given by a dotted tuple.  Examples would
 *7 be from the original spec file lines
 *7         index.query_file.rwmode  SRDWR|SINCORE|SCREATE
 *7         query_file.rmode         SRDONLY|SINCORE
 *7         rmode                    SRDONLY|SINCORE
 *7         rwmode                   SRDWR
 *7 Here, "index.query_file.rwmode" is a parameter name specifying
 *7 what filemode a query file should be stored as when indexing.
 *7 (The parameter value indicates the query file should be opened
 *7 read-write, should be kept in memory while being indexed, and
 *7 will be a new file that will need to be created.)
 *7 
 *7 The key field of the parameter_name is the last field.  The
 *7 preceding fields contain restrictions of when the key field
 *7 matches.
 *7 
 *7 PARAMETER_NAME MATCHES
 *7 Many procedures in SMART require information from the spec file.
 *7 They request the values of specific spec file tuples.  For
 *7 example, the procedure that converts a query file from one
 *7 weighting scheme to another might ask for (among others)
 *7     "convert.query_file"
 *7     "convert.query_file.rmode"
 *7     "convert.new_query_file"
 *7     "convert.new_query_file.rwmode"
 *7 
 *7 The parameter names in the specification file are then examined
 *7 to find the best match.  The "key" field, the last in both the
 *7 procedure request and the spec file paramater name, must match
 *7 exactly; some of the other fields in the request may be dropped
 *7 from the spec file name.  The following algorithm is used to
 *7 determine the best match:
 *7 
 *7 1. An exact match between request name and spec name is best.
 *7 2. If that doesn't exist, each leading field, from left to right,
 *7     of the request name is dropped in turn until a match is found
 *7     or the key field is reached.
 *7 3. If a match still hasn't been found, trailing fields, from
 *7     right to left (ignoring the key field), of the request name
 *7     are dropped until a match is found.
 *7 4. If a match still hasn't been found, an error is reported.
 *7 
 *7 In the examples above, the request "convert.query_file.rmode"
 *7 doesn't have an exact match in the example spec file lines.
 *7 Under rule 2, "query_file.rmode" is then checked against the spec
 *7 file and matches.  Thus the value of the parameter is
 *7 "SRDONLY|SINCORE".   Similarly, the value of
 *7 "convert.new_query_file.rwmode" will be "SRDWR".
 *7 
 *7 Rule 3 most often gets used when a procedure is requesting
 *7 several parameters that are named as an array would be named.
 *7 For example, the spec file lines
 *7         index.section.name none
 *7         index.section.3.name author
 *7 set up a situation where the procedure can request the values for
 *7         index.section.0.name
 *7         index.section.1.name
 *7         index.section.2.name
 *7         index.section.3.name
 *7 to find the names of 4 sections.  The first three sections will
 *7 all have the value "none", while the fourth will have the value
 *7 "author".
***********************************************************************/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Lookup a set of parameters in a spec object, and set the values for them
 *2 lookup_spec_prefix (spec_ptr, spec_param_prefix, num_params)
 *3   SPEC *spec_ptr;
 *3   SPEC_PARAM_PREFIX *spec_param_prefix;
 *3   int num_params;
 *4 getspec_float
 *4 getspec_double
 *4 getspec_string
 *4 getspec_char
 *4 getspec_long
 *4 getspec_bool
 *4 getspec_filemode
 *4 getspec_dbfile
 *4 getspec_func

 *7 The list of parameters that need to be looked up is given by the array
 *7 of SPEC_PARAM_PREFIX tuples spec_param_prefix.  For each parameter, the 
 *7 calling procedure specifies the request parameter basename, a pointer to
 *7 a prefix to be prepended to the basename before lookup, a location for the
 *7 parameter value to be placed, and an interpreting procedure that will
 *7 take the string value found in the spec file for this parameter,
 *7 convert it into an object of the appropriate type, and place the value
 *7 in the designated location.
 *7 
 *7 See documentation for "lookup_spec" for full details.
 *7 Lookup_spec_prefix is normally used instead of lookup_spec when there is
 *7 a run-time value of a prefix to be prepended to a compile time value of
 *7 the parameter name.
***********************************************************************/


#include <ctype.h>
#include "common.h"
#include "param.h"
#include "functions.h"
#include "io.h"
#include "smart_error.h"
#include "proc.h"
#include "spec.h"

static char *delete_leading_field(), *delete_trailing_field(),
            *get_param_value();

int 
lookup_spec (spec_ptr, spec_param, num_params)
SPEC *spec_ptr;
SPEC_PARAM *spec_param;
int num_params;
{
    long i;
    char *param_value;

    for (i = 0; i < num_params; i++) {
        if (NULL == (param_value = get_param_value (spec_ptr,
                                                    spec_param[i].param)) ||
            UNDEF == spec_param[i].convert (spec_ptr,
                                            spec_param[i].param,
                                            param_value,
                                            spec_param[i].result)) {
            set_error (SM_ILLPA_ERR, spec_param[i].param, "lookup_spec");
            return (UNDEF);
        }
    }
    return (0);
}


int 
lookup_spec_prefix (spec_ptr, spec_param, num_params)
SPEC *spec_ptr;
SPEC_PARAM_PREFIX *spec_param;
int num_params;
{
    long i;
    char *param_value;
    char full_param_name[PATH_LEN];
    char *full_param_ptr;             /* Points to full_param_name if
                                         there was a valid prefix, else
                                         points to spec_param.name 
                                         (optimization to avoid copying) */
    int prefix_len;

    for (i = 0; i < num_params; i++) {
        /* First construct full parameter name from catenation of
           spec_param[i].prefix and spec_param[i].param */
        if (spec_param[i].prefix != NULL &&
            *spec_param[i].prefix != NULL &&
            (*spec_param[i].prefix)[0] != '\0') {
            prefix_len = strlen (*spec_param[i].prefix);
            (void) strncpy (full_param_name, *spec_param[i].prefix, PATH_LEN);
            (void) strncpy (&full_param_name[prefix_len],
                            spec_param[i].param,
                            PATH_LEN - prefix_len);
            full_param_ptr = full_param_name;
        }
        else
            full_param_ptr = spec_param[i].param;

        if (NULL == (param_value = get_param_value (spec_ptr,
                                                    full_param_ptr)) ||
            UNDEF == spec_param[i].convert (spec_ptr,
                                            spec_param[i].param,
                                            param_value,
                                            spec_param[i].result)) {
            set_error (SM_ILLPA_ERR, spec_param[i].param, "lookup_spec");
            return (UNDEF);
        }
    }
    return (0);
}


/* First try an exact match of param.  Then try an exact match deleting
   the leading components of param left_to_right in turn (eg, matching param 
   "index.textloc_file.rwmode" against database "textloc_file.rwmode" or
   "rwmode").  Then try deleting leading components right to left in turn
   (eg, matching param "index.section.3.tocon" against database
   "index.section.tocon").  SHOULDN'T EVER NEED LATTER??? */
   
static
char *
get_param_value(spec_ptr, param)
SPEC *spec_ptr;
char *param;
{
    SPEC_TUPLE *start_ptr, *end_ptr, *ptr;
    char *new_param = param;
    char *param_key_field;           /* Last field of param (required match) */
    char *temp_ptr;

    /* First find section of spec_list that the last field of param
       will match (this match is required.) */
    for (temp_ptr = param_key_field = param; *temp_ptr; temp_ptr++)
        if (*temp_ptr == '.') param_key_field = temp_ptr + 1;

    end_ptr = &spec_ptr->spec_list[spec_ptr->num_list];
    for (start_ptr = spec_ptr->spec_list ;
         start_ptr < end_ptr && strcmp (param_key_field, start_ptr->key_field);
         start_ptr++)
        ;
    for (ptr = start_ptr;
         ptr < end_ptr && (0 == strcmp (param_key_field, ptr->key_field));
         ptr++)
        ;
    end_ptr = ptr;

    /* Search section between start_ptr and end_ptr, first trying to delete
       leading field of param */
    do {
        ptr = start_ptr;
        while (ptr < end_ptr) {
            if (0 == strcmp (ptr->name, new_param))
                return (ptr->value);
            ptr++;
        }
    } while (NULL != (new_param = delete_leading_field (new_param)));

    /* Search section between start_ptr and end_ptr, now trying to delete
       trailing (except for key_field) field of param */
    new_param = param;
    while (NULL != (new_param = delete_trailing_field (new_param))) {
        ptr = start_ptr;
        while (ptr < end_ptr) {
            if (0 == strcmp (ptr->name, new_param))
                return (ptr->value);
            ptr++;
        }
    }

    return (NULL);
}

/* Delete the leading field from param */
static char *
delete_leading_field (param)
char *param;
{
    while (*param && *param != '.') {
        param++;
    }
    if (*param) {
        param++;
        return (param);
    }
    return (NULL);
}


static char *delete_buf;
static unsigned delete_buf_len = 0;

/* Delete the next to the last field from param */
static char *
delete_trailing_field (param)
char *param;
{
    char *last_dot = param;
    char *next_last_dot = param;
    char *ptr, *new_ptr;

    /* Find the starting points of the last fields */
    for (ptr = param; *ptr; ptr++) {
        if (*ptr == '.') {
            next_last_dot = last_dot;
            last_dot = ptr;
        }
    }
    /* If no leading fields, return NULL */
    if (last_dot == param)
        return (NULL);

    /* Ensure that delete_buf is big enough */
    if (delete_buf_len < ptr - param) {
        if (delete_buf_len == 0) {
            if (NULL == (delete_buf = malloc ((unsigned) (ptr - param)))) {
                return (NULL);
            }
        }
        else if (NULL == (delete_buf = realloc (delete_buf,
                                                (unsigned) (ptr - param)))) {
            return (NULL);
        }
        delete_buf_len = ptr - param;
    }

    /* Copy leading fields up to next_to_last field */
    for (ptr = param, new_ptr = delete_buf; ptr < next_last_dot; ptr++) {
        *new_ptr++ = *ptr;
    }
    /* Copy the last field (including dot if needed) */
    if (new_ptr > delete_buf)
        *new_ptr++ = '.';
    for (ptr = last_dot+1; *ptr; ptr++) {
        *new_ptr++ = *ptr;
    }
    *new_ptr = '\0';
    
    return (delete_buf);
}


/* 
 * CONVERSION ROUTINES *
 */

int
getspec_float (spec_ptr, param, value, result)
SPEC *spec_ptr;
char *param;
char *value;
float *result;
{
    *result = (float) atof (value);
    return (1);
}


int
getspec_double (spec_ptr, param, value, result)
SPEC *spec_ptr;
char *param;
char *value;
double *result;
{
    *result = (double) atof (value);
    return (1);
}

int
getspec_string (spec_ptr, param, value, result)
SPEC *spec_ptr;
char *param;
char *value;
char **result;
{
    *result = value;
    return (1);
}

int
getspec_intstring (spec_ptr, param, value, result)
SPEC *spec_ptr;
char *param;
char *value;
char **result;
{
    char *ptr, *optr;
    for (ptr = value; *ptr && *ptr != '\\'; ptr++)
        ;
    if (! *ptr) {
        *result = value;
        return (1);
    }
    if (NULL == (*result = malloc ((unsigned) strlen (value))))
        return (UNDEF);
    optr = *result;
    for (ptr = value; *ptr; ptr++) {
        if (*ptr != '\\') 
            *optr++ = *ptr;
        else {
            ptr++;
            switch (*ptr) {
              case '\0':
                return (UNDEF);
              case 'b':
                *optr++ = '\b';
                break;
              case 'f':
                *optr++ = '\f';
                break;
              case 'n':
                *optr++ = '\n';
                break;
              case 'r':
                *optr++ = '\r';
                break;
              case 't':
                *optr++ = '\t';
                break;
              default:
                if (*ptr >= '0' && *ptr <= '9') {
                    *optr++ = *ptr - '0';
                    ptr++;
                    while (*ptr >= '0' && *ptr <= '9') {
                        *optr = *optr * 8 + *ptr - '0';
                        optr++; ptr++;
                    }
                    ptr--;
                }
                else
                    *optr++ = *ptr;
            }
        }
    }
    *optr = '\0';
    return (1);
}

int
getspec_char (spec_ptr, param, value, result)
SPEC *spec_ptr;
char *param;
char *value;
char *result;
{
    *result = *value;
    return (1);
}

static int strcmp_case();
int
getspec_bool (spec_ptr, param, value, result)
SPEC *spec_ptr;
char *param;
char *value;
int *result;
{
    if (0 == strcmp_case (value, "true") ||
        0 == strcmp_case (value, "on") ||
        0 == strcmp (value, "1")) {
        *result = 1;
        return (1);
    }
    else if (0 == strcmp_case (value, "false") ||
        0 == strcmp_case (value, "off") ||
        0 == strcmp (value, "0")) {
        *result = 0;
        return (1);
    }
    else {
        set_error (SM_ILLPA_ERR, value, "getspec_bool");
        return (UNDEF);
    }
}

/* Utility return like strcmp, except that each character of the first
   arg is changed from uppercase to lower case if needbe before checking
   whether it equals corresponding char of second arg (which is assumed
   to be in lower case already) */
static int
strcmp_case (mixed, lower)
char *mixed;
char *lower;
{
    char tempchar;
    while (*mixed || *lower) {
        tempchar = (isupper (*mixed) ? tolower (*mixed) : *mixed);
        if (tempchar > *lower)
            return (1);
        if (tempchar < *lower)
            return (-1);
        mixed++; lower++;
    }

    return (0);
}


int
getspec_long (spec_ptr, param, value, result)
SPEC *spec_ptr;
char *param;
char *value;
long *result;
{
    *result = atol (value);
    return (1);
}

struct sm_mode_map {
    char *name;
    long mode;
} mode_map[] = {
"SRDONLY",         SRDONLY,
"SWRONLY",         SWRONLY,
"SRDWR",           SRDWR,
/* "SNBLOCK",         SNBLOCK, */
"SAPPEND",         SAPPEND,
"SCREATE",         SCREATE,
"STRUNCATE",       STRUNCATE,
"SEXCL",           SEXCL,
/*
    "SSHLOCK",         SSHLOCK,
    "SEXLOCK",         SEXLOCK,
    "SUNLOCK",         SUNLOCK,
    "SNB_LOCK",        SNB_LOCK,
*/
"STEMP",           STEMP,
"SINCORE",         SINCORE,
"SBACKUP",         SBACKUP,
"SGCOLL",          SGCOLL,
#ifdef MMAP
"SMMAP",           SMMAP,
#endif
};

static int num_mode_map = sizeof (mode_map) / sizeof (mode_map[0]);


int
getspec_filemode  (spec_ptr, param, value, result)
SPEC *spec_ptr;
char *param;
char *value;
long *result;
{
    char *ptr, *start_ptr;
    long i;

    *result = 0;
    start_ptr = value;
    ptr = value;
    while (*start_ptr) {
        while (*ptr && *ptr != '|') {
            ptr++;
        }
        for (i = 0; i < num_mode_map; i++) {
            if (0 == strncmp (mode_map[i].name, start_ptr, ptr - start_ptr)) {
                break;
            }
        }
        if (i >= num_mode_map) {
            set_error (SM_ILLPA_ERR, value, "getspec_filemode");
            return (UNDEF);
        }
        *result |= mode_map[i].mode;
        if (*ptr)
            start_ptr = ++ptr;
        else 
            break;
    }
    return (1);
}


int
getspec_dbfile  (spec_ptr, param, value, result)
SPEC *spec_ptr;
char *param;
char *value;
char **result;
{
    char *database;

    if (NULL == (database = get_param_value (spec_ptr, "database"))) {
        set_error (SM_ILLPA_ERR, database, "lookup_spec");
        return (UNDEF);
    }
    *result = prepend_db (database, value);
    return (1);
}

/*  getspec_func is given elsewhere */




