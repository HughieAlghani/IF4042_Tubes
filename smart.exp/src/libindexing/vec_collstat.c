#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libindexing/vec_collstat.c,v 11.0 1992/07/21 18:21:40 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Add info (frequency and idf) for vector vec to the collection stat file
 *1 convert.tup.vec_collstat
 *2 vec_collstat (vector, unused, inst)
 *3   VEC *vector;
 *3   char *unused;
 *3   int inst;

 *4 init_vec_collstat (spec, param_prefix)
 *5   "vec_collstat.mem_usage"
 *5   "vec_collstat.virt_mem_usage"
 *5   "vec_collstat.dict_file_size"
 *5   "vec_collstat.trace"
 *5   "*.collstat_file"
 *5   "*.collstat_file.rwmode"
 *4 close_vec_collstat(inst)

 *7 Add info from vector to collection file specified by parameter given by 
 *7 the concatenation of param_prefix and "collstat_file".  
 *7 Normally, param_prefix will have some value like "ctype.1." in order
 *7 to obtain the ctype dependant freq file.  Ie. "ctype.1.collstat_file"
 *7 will be looked up in the specifications to find the correct frq
 *7 file to use.
 *7 UNDEF returned if error, else 0.

 *9 Warning: collstat_file not updated until close_vec_collstat is called.
 *9 BUG/WARNING: The idf value is a float, being stored in a simp_inv list
 *9 as a long.  If sizeof (float) != sizeof (long), then the idf values are
 *9 not computed or stored (which means later inefficiencies for idf use).
**********************************************************************/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "smart_error.h"
#include "spec.h"
#include "trace.h"
#include "vector.h"
#include "simp_inv.h"
#include "io.h"
#include "inst.h"
#include "collstat.h"

static SPEC_PARAM spec_args[] = {
    TRACE_PARAM ("vec_collstat.trace")
    };
static int num_spec_args = sizeof (spec_args) / sizeof (spec_args[0]);

static char *prefix;
static char *collstat_file;
static long collstat_mode;
static long dict_file_size;        /* Rough estimate of max concept number to
                                       be dealt with */
static SPEC_PARAM_PREFIX pspec_args[] = {
    &prefix,  "collstat_file",     getspec_dbfile,    (char *) &collstat_file,
    &prefix,  "collstat.rwmode",   getspec_filemode,  (char *) &collstat_mode,
    &prefix,  "dict_file_size",    getspec_long,      (char *) &dict_file_size,
    };
static int num_pspec_args = sizeof (pspec_args) / sizeof (pspec_args[0]);

static int collstat_output();

/* Static info to be kept for each instantiation of this proc */
typedef struct {
    /* bookkeeping */
    int valid_info;

    long *list;
    long num_list;

    /*inverted file info */
    char collstat_file[PATH_LEN];
    long collstat_mode;
    long dict_size;              /* Size of dict (guide to space allocation) */
    long num_doc;                /* Number of docs being added to collstats */
} STATIC_INFO;

static STATIC_INFO *info;
static int max_inst = 0;

int
init_vec_collstat (spec, param_prefix)
SPEC *spec;
char *param_prefix;
{
    long i;
    STATIC_INFO *ip;
    int new_inst;

    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);
    prefix = param_prefix;
    if (UNDEF == lookup_spec_prefix (spec, &pspec_args[0], num_pspec_args))
        return (UNDEF);

    if (! VALID_FILE (collstat_file)) {
        set_error (SM_ILLPA_ERR, "collstat_file", "vec_collstat");
        return (UNDEF);
    }
    
    PRINT_TRACE (2, print_string, "Trace: entering init_vec_collstat");
    PRINT_TRACE (4, print_string, collstat_file);
    
    /* Check to see if this file_name has already been initialized.  If
       so, that instantiation will be used. */
    for (i = 0; i < max_inst; i++) {
        if (info[i].valid_info &&
            0 == strcmp (collstat_file, info[i].collstat_file)) {
            info[i].valid_info++;
            return (i);
        }
    }
    
    NEW_INST (new_inst);
    if (UNDEF == new_inst)
        return (UNDEF);
    
    ip = &info[new_inst];

    if (NULL == (ip->list = (long *)
                 calloc ((unsigned) dict_file_size, sizeof (long))))
        return (UNDEF);
    ip->num_list = dict_file_size;
    
    (void) strncpy (ip->collstat_file, collstat_file, PATH_LEN);
    ip->dict_size = dict_file_size;
    ip->collstat_mode = collstat_mode;
    ip->num_doc = 0;
    
    ip->valid_info = 1;

    PRINT_TRACE (2, print_string, "Trace: leaving init_vec_collstat");

    return (new_inst);
}

int
vec_collstat (vector, unused, inst)
VEC *vector;
char *unused;
int inst;
{
    register CON_WT *cw_ptr;    /* pointers into vector's tuples */
    register CON_WT *end_vector;
    STATIC_INFO *ip;
    long new_size;

    PRINT_TRACE (2, print_string, "Trace: entering vec_collstat");
    PRINT_TRACE (6, print_vector, vector);

    if (! VALID_INST (inst)) {
        set_error (SM_ILLPA_ERR, "Instantiation", "vec_collstat");
        return (UNDEF);
    }
    ip  = &info[inst];

    if (vector->num_conwt == 0)
        return (0);

    end_vector = &vector->con_wtp[vector->num_conwt];
    /* Check to see if need to expand list */
    if ((end_vector-1)->con >= ip->num_list) {
        new_size = ip->num_list + ip->dict_size;
        if ((end_vector-1)->con >= new_size)
            new_size = (end_vector-1)->con + 3000;
        
        if (NULL == (ip->list = (long *) 
                     realloc ((char *) ip->list,
                              (unsigned) (new_size) * sizeof (long)))) {
            return (UNDEF);
        }
        bzero ((char *) &ip->list[ip->num_list],
               sizeof (long) *
               (int) (new_size - ip->num_list));
        ip->num_list = new_size;
    }

    for (cw_ptr = vector->con_wtp; cw_ptr < end_vector; cw_ptr++) {
        ip->list[cw_ptr->con]++;
    }
    ip->num_doc++;

    PRINT_TRACE (2, print_string, "Trace: leaving vec_collstat");

    return (1);
}

int
close_vec_collstat(inst)
int inst;
{
    STATIC_INFO *ip;

    PRINT_TRACE (2, print_string, "Trace: entering close_vec_collstat");

    if (! VALID_INST (inst)) {
        set_error (SM_ILLPA_ERR, "Instantiation", "close_vec_collstat");
        return (UNDEF);
    }

    ip  = &info[inst];
    ip->valid_info--;

    /* Output everything and free buffers if this was last close of this
       inst */
    if (ip->valid_info == 0) {
        if (UNDEF == collstat_output (ip))
            return (UNDEF);
        
        free ((char *)ip->list);
    }

    PRINT_TRACE (2, print_string, "Trace: leaving close_vec_collstat");
    return (0);
}

static int
collstat_output (ip)
STATIC_INFO *ip;
{
    SIMP_INV simp_inv;
    long i;
    int collstat_fd;
    float *idf_list;
    double log_num_docs;

    if (UNDEF == (collstat_fd = open_simp_inv (ip->collstat_file,
                                               ip->collstat_mode))) {
        /* New file. Must create, but don't need to merge */
        clr_err();
        if (UNDEF == (collstat_fd = open_simp_inv (ip->collstat_file,
                                             ip->collstat_mode|SCREATE)))
            return (UNDEF);
    }
    else {
        /* File exists; must read in and merge info */
        simp_inv.id_num = COLLSTAT_NUMDOC;
        if (1 != seek_simp_inv (collstat_fd, &simp_inv) ||
            1 != read_simp_inv (collstat_fd, &simp_inv))
            /* Somehow, file exists and is empty */
            return (UNDEF);
        ip->num_doc += simp_inv.list[0];
        
        simp_inv.id_num = COLLSTAT_COLLFREQ;
        if (1 != seek_simp_inv (collstat_fd, &simp_inv) ||
            1 != read_simp_inv (collstat_fd, &simp_inv))
            /* Somehow, file exists and is empty */
            return (UNDEF);
        
        if (simp_inv.num_list > ip->num_list) {
            if (NULL == (ip->list = (long *) 
                         realloc ((char *) ip->list,
                                  (unsigned) (simp_inv.num_list) *
                                  sizeof (long))))
                return (UNDEF);
            bzero ((char *) &ip->list[ip->num_list],
                   sizeof (long) *
                   (int) (simp_inv.num_list - ip->num_list));
            ip->num_list = simp_inv.num_list;
        }
        
        for (i = 0; i < simp_inv.num_list; i++)
            ip->list[i] += simp_inv.list[i];
    }

    simp_inv.num_list = 1;
    simp_inv.list = &ip->num_doc;
    simp_inv.id_num = COLLSTAT_NUMDOC;
    if (UNDEF == seek_simp_inv (collstat_fd, &simp_inv) ||
        UNDEF == write_simp_inv (collstat_fd, &simp_inv))
        return (UNDEF);
    PRINT_TRACE (4, print_simp_inv, &simp_inv);

    simp_inv.num_list = ip->num_list;
    simp_inv.list = ip->list;
    simp_inv.id_num = COLLSTAT_COLLFREQ;
    if (UNDEF == seek_simp_inv (collstat_fd, &simp_inv) ||
        UNDEF == write_simp_inv (collstat_fd, &simp_inv))
        return (UNDEF);
    PRINT_TRACE (4, print_simp_inv, &simp_inv);
    

    /* Construct idf list */
    if (sizeof (float) == sizeof (long)) {
        if (NULL == (idf_list = (float *)
                     malloc ((unsigned) ip->num_list * sizeof (float))))
            return (UNDEF);
        /* Actually use N+1 instead of N to avoid problem of word occurring
           in all docs */
        log_num_docs = log ((double) (ip->num_doc + 1));
        for (i = 0; i < ip->num_list; i++) {
            if (ip->list[i] > 0) 
                idf_list[i] = log_num_docs - log ((double) ip->list[i]);
            else
                idf_list[i] = 0.0;
            PRINT_TRACE (4, print_float, &idf_list[i]);
        }
        simp_inv.num_list = ip->num_list;
        simp_inv.list = (long *) idf_list;
        simp_inv.id_num = COLLSTAT_IDF;
        if (UNDEF == seek_simp_inv (collstat_fd, &simp_inv) ||
            UNDEF == write_simp_inv (collstat_fd, &simp_inv))
            return (UNDEF);
        (void) free ((char *) idf_list);
    }

    if (UNDEF == close_simp_inv (collstat_fd))
        return (UNDEF);

    return (1);
}
