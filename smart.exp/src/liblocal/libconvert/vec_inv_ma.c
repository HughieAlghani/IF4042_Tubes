#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/liblocal/libconvert/vec_inv_ma.c,v 11.0 1992/07/21 18:21:58 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Add vector vec to inverted file, using temp files instead of memory
 *1 convert.tup.vec_inv_tmp
 *2 vec_inv_tmp (vector, unused, inst)
 *3   VEC *vector;
 *3   char *unused;
 *3   int inst;

 *4 init_vec_inv_tmp (spec, param_prefix)
 *5   "vec_inv.mem_usage"
 *5   "vec_inv.virt_mem_usage"
 *5   "vec_inv.dict_file_size"
 *5   "vec_inv.trace"
 *5   "vec_inv.temp_dir"
 *5   "*.inv_file"
 *5   "*.inv_file.rwmode"
 *4 close_vec_inv_tmp(inst)

 *7 Add vector to the inverted file specified by parameter given by 
 *7 the concatenation of param_prefix and "inv_file".  
 *7 Normally, param_prefix will have some value like "ctype.1." in order
 *7 to obtain the ctype dependant inverted file.  Ie. "ctype.1.inv_file"
 *7 will be looked up in the specifications to find the correct inverted
 *7 file to use.
 *7 Intermediate lists are kept in the directory "temp_dir".
 *7 UNDEF returned if error, else 0.

 *8 Construct linked list of occurrences of terms.  When mem_usage is exceeded
 *8 instead of writing out to inverted file immediately, write out to 
 *8 auxilliary buffer[s] the actual constructed lists.  When virtual_mem_limit
 *8 exceeded, write out to inverted file.  
 *8 
 *8 Malloc array of max_mem_usage size to hold linked list pointer and weight.
 *8 Calloc array of dict_size size to hold head of list and size of list
 *8 Phase 1:
 *8 Read a doc, for each con in doc, put docid.weight on that con's linked list
 *8 Phase 2:
 *8 If mem_usage exceeded by linked list, then construct buffer with actual
 *8 lists from all of linked list, and start linked list over again.  Buffer
 *8 is written out to temp files in temp_dir.
 *8 Phase 3:
 *8 When reach end of docs, or end of virt_mem_usage, construct inverted node
 *8     for each con.  Read the inverted file to get previous inverted list
 *8     for node, and then merge in sorted list(s) from the buffers in
 *8     temp_dir.  (Drawback: if don't have
 *8     garbage collection on inverted list file, then multipass conversion
 *8     (virt_mem_usage exceeded) will result in lots of wasted space in 
 *8     inv.var file).
 *8 If docs not exhausted (virt_mem_usage exceeded) then continue.
 *8 
 *8 Goal: Minimize resident set size while still maintaining reasonable speed.
 *8 Phase 1: Head list + end of linked list + whatever is calling this.
 *8 Phase 2: end of Head_list, all of linked list, end of aux_head, end of 
 *8         aux buf.  whatever is calling will probably get paged out.
 *8 Phase 3. end of all aux lists,  end of all aux_head lists, whatever
 *8         space write_inv takes (end of var file, end of fixed file, but
 *8         if SINCORE may have to realloc var file which will page 
 *8         everything out).
 *8 

 *9 Warning: inv_file not updated until close_vec_inv_tmp is called.
 *9 WARNING: Currently only works if MMAP is defined
**********************************************************************/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "smart_error.h"
#include "spec.h"
#include "trace.h"
#include "vector.h"
#include "inv.h"
#include "io.h"
#include "inst.h"

#ifdef MMAP
#include <sys/mman.h>
#endif  /* MMAP */

static unsigned long mem_usage;     /* Rough estimate of how much memory this
                                       routine should have available in
                                       resident set size (ie without paging)*/
static long dict_size;              /* Rough estimate of max concept number to
                                       be dealt with */
static char *temp_dir;              /* Directory in which to place intermediate
                                       temporary results */
static char *database;              /* Default directory in which to place 
                                       intermediate temporary results */
static SPEC_PARAM spec_args[] = {
    "vec_inv.mem_usage",       getspec_long,    (char *) &mem_usage,
    "vec_inv.dict_file_size",       getspec_long,    (char *) &dict_size,
    "vec_inv.temp_dir",        getspec_dbfile,    (char *) &temp_dir,
    "database",                getspec_string,    (char *) &database,
    TRACE_PARAM ("vec_inv.trace")
    };
static int num_spec_args = sizeof (spec_args) / sizeof (spec_args[0]);

static char *prefix;
static char *inv_file;
static long inv_mode;
static SPEC_PARAM_PREFIX prefix_spec_args[] = {
    &prefix, "inv_file",       getspec_dbfile,    (char *) &inv_file,
    &prefix, "inv_file.rwmode",getspec_filemode,  (char *) &inv_mode,
    };
static int num_prefix_spec_args = sizeof (prefix_spec_args) /
         sizeof (prefix_spec_args[0]);


/* linked list structures */
typedef struct linkedlist {
    struct linkedlist *link;
    long did;
    float weight;
} LIST;

typedef struct {
    LIST *head;           /* list_hd[0..num_linked] gives head
                             of linked list for the concept.  0 
                             indicates end of list. list is in 
                             decreasing document order */
    long num_linked;
} LIST_HD;

typedef struct {
    long *num_list_buf;        /* num_list_buf[con] gives size of inverted list
                                  for con */
    long max_con;              /* max length of num_list_buf */
    LISTWT *listwt_buf;        /* Global pool for listwts */
    long num_list;             /* length of listwt_buf) */
    char temp_file[PATH_LEN];  /* Temporary file basename.
                                  "<temp_dir>/inv.<pid>.<num_aux_hd>."
                                  num_list_buf written to temp_file.nlb
                                  listwt_buf written to temp_file.lwb */
} AUX_HD;

/* Static info to be kept for each instantiation of this proc */
typedef struct {
    /* bookkeeping */
    int valid_info;
    /* linked list info */
    LIST_HD *list_hd;
    long num_list_hd;
    long max_list_hd;
    LIST *list, *end_list_ptr, *list_ptr;

    /* sequential list buffers (for conversion of one linked list to
       sequential list */
    long *num_list_buf;        /* num_list_buf[con] gives size of inverted list
                                  for con */
    LISTWT *listwt_buf;          /* Global pool for listwts */

    /* aux buffer info */
    AUX_HD *aux_hd;             /* List of aux buffers */
    int num_aux_hd;             /* Number of aux buffers */
    int max_num_aux_hd;         /* Number of aux buffers space reserved for */

    /*inverted file info */
    char file_name[PATH_LEN];
    int inv_fd;
    int file_created;            /* flag indicating this is a new inverted
                                    file, thus do not have to read in old */
    long mem_usage;              /* Local copy of mem_usage spec value */
    long dict_size;              /* Size of dict (guide to space allocation) */
    char base_temp_file[PATH_LEN];/* Base name for temp_files.
                                    "<temp_dir>/tmpinv.<pid>." */
} STATIC_INFO;

#ifdef MMAP
static int linked_to_aux(), inv_output();

static STATIC_INFO *info;
static int max_inst = 0;

static int save_spec_id = 0;

/* Temp space for new inverted list creation */
static unsigned long num_inv_listwt = 0;
static LISTWT *inv_listwt;          
#endif  /* MMAP */


/* How many concepts to write out at once */
#define NLB_CACHE (16 * 1024)

int
init_vec_inv_tmp (spec, param_prefix)
SPEC *spec;
char *param_prefix;
{
#ifndef MMAP
    set_error (SM_ILLPA_ERR, "MMAP is not defined", "vec_inv_tmp");
    return (UNDEF);
#else
    long i;
    STATIC_INFO *ip;
    int new_inst;
    long pid;

    if (spec->spec_id != save_spec_id &&
        UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    /* Lookup the inverted file wanted for this instantiation (normally
       param_prefix will be "index.ctype.<digit>.") */
    prefix = param_prefix;
    if (UNDEF == lookup_spec_prefix (spec,
                                     &prefix_spec_args[0],
                                     num_prefix_spec_args))

    PRINT_TRACE (2, print_string, "Trace: entering init_vec_inv_tmp");
    PRINT_TRACE (4, print_string, inv_file);
    
    /* Check to see if this file_name has already been initialized.  If
       so, that instantiation will be used. */
    for (i = 0; i < max_inst; i++) {
        if (info[i].valid_info && 0 == strcmp (inv_file, info[i].file_name)) {
            info[i].valid_info++;
            return (i);
        }
    }
    
    NEW_INST (new_inst);
    if (UNDEF == new_inst)
        return (UNDEF);
    
    ip = &info[new_inst];
    
   /* Create array of dict_size nodes */
    /* All values in array are set to 0 */
    if (NULL == (ip->list_hd = (LIST_HD *)
                 calloc ((unsigned) dict_size, sizeof (LIST_HD))) ||
        NULL == (ip->list = (LIST *) valloc ((unsigned) mem_usage))) {
        set_error (errno, "List allocation", "vec_inv_tmp");
        return (UNDEF);
    }
    ip->max_list_hd = dict_size;
    ip->num_list_hd = 0;
    ip->list_ptr = ip->list + 1;
    ip->end_list_ptr = (LIST *)
        (((char *) ip->list) + mem_usage - sizeof (LIST));

    if (NULL == (ip->num_list_buf = (long *)
                 valloc ((unsigned) NLB_CACHE * sizeof (long))) ||
        NULL == (ip->listwt_buf = (LISTWT *)
                 valloc ((unsigned) (mem_usage / sizeof (LIST))
                         * sizeof (LISTWT))))
        return (UNDEF);

    if (NULL == (ip->aux_hd = (AUX_HD *) malloc (4 * sizeof (AUX_HD))))
        return (UNDEF);
    ip->num_aux_hd = 0;
    ip->max_num_aux_hd = 4;

    (void) strncpy (ip->file_name, inv_file, PATH_LEN);
    ip->file_created = 0;
    if (UNDEF == (ip->inv_fd = open_inv (inv_file, inv_mode))) {
        clr_err();
        if (UNDEF == (ip->inv_fd = open_inv (inv_file, inv_mode|SCREATE)))
            return (UNDEF);
        ip->file_created = 1;
    }

    /* Construct basename in which to put temporary files.  Use temp_dir
       if defined; if not, then use database directory itself (default) */
    if (! VALID_FILE (temp_dir))
        temp_dir = database;
    pid = (long) getpid();
    (void) sprintf (ip->base_temp_file, "%s/tmpinv.%ld", temp_dir, pid);

    ip->valid_info = 1;
    ip->mem_usage = mem_usage;
    ip->dict_size = dict_size;
    
    PRINT_TRACE (2, print_string, "Trace: leaving init_vec_inv_tmp");

    return (new_inst);
#endif  /* MMAP */

}

int
vec_inv_tmp (vector, unused, inst)
VEC *vector;
char *unused;
int inst;
{
#ifndef MMAP
    set_error (SM_ILLPA_ERR, "MMAP is not defined", "vec_inv_tmp");
    return (UNDEF);
#else
    register CON_WT *cw_ptr;    /* pointers into vector's tuples */
    register CON_WT *end_vector;
    STATIC_INFO *ip;
    long new_size;

    PRINT_TRACE (2, print_string, "Trace: entering vec_inv_tmp");
    PRINT_TRACE (6, print_vector, vector);

    if (! VALID_INST (inst)) {
        set_error (SM_ILLPA_ERR, "Instantiation", "vec_inv_tmp");
        return (UNDEF);
    }
    ip  = &info[inst];

    end_vector = &vector->con_wtp[vector->num_conwt];
    for (cw_ptr = vector->con_wtp; cw_ptr < end_vector; cw_ptr++) {
        /* Add the new doc_id into each concepts linked list */
        /* Note that linked list guaranteed to be in reverse */
        /* order of did */
        
        /* If we have filled up available memory, output the present */
        /* linked lists, and start constructing new lists */
        if (ip->list_ptr >= ip->end_list_ptr && UNDEF == linked_to_aux (ip))
                return (UNDEF);
        
        /* Check to see if need to expand list_hd */
        if (cw_ptr->con >= ip->num_list_hd) {
            ip->num_list_hd = cw_ptr->con + 1;
            if (cw_ptr->con >= ip->max_list_hd) {
                new_size = ip->max_list_hd + ip->dict_size;
                if (cw_ptr->con >= new_size)
                    new_size = cw_ptr->con + 3000;
                
                if (NULL == (ip->list_hd = (LIST_HD *) 
                             realloc ((char *) ip->list_hd,
                                  (unsigned) (new_size) * sizeof (LIST_HD)))) {
                    return (UNDEF);
                }
                bzero ((char *) &ip->list_hd[ip->max_list_hd],
                       sizeof (LIST_HD) *
                       (int) (new_size - ip->max_list_hd));
                ip->max_list_hd = new_size;
            }
        }
            
        ip->list_ptr->link = ip->list_hd[cw_ptr->con].head;
        ip->list_hd[cw_ptr->con].head = ip->list_ptr;
        ip->list_hd[cw_ptr->con].num_linked++;
        ip->list_ptr->weight = cw_ptr->wt;
        ip->list_ptr->did = vector->id_num;
        ip->list_ptr++;
    }
    PRINT_TRACE (2, print_string, "Trace: leaving vec_inv_tmp");

    return (1);
#endif  /* MMAP */
}

int
close_vec_inv_tmp(inst)
int inst;
{
#ifndef MMAP
    set_error (SM_ILLPA_ERR, "MMAP is not defined", "vec_inv_tmp");
    return (UNDEF);
#else
    STATIC_INFO *ip;

    PRINT_TRACE (2, print_string, "Trace: entering close_vec_inv_tmp");

    if (! VALID_INST (inst)) {
        set_error (SM_ILLPA_ERR, "Instantiation", "close_vec_inv_tmp");
        return (UNDEF);
    }

    ip  = &info[inst];
    ip->valid_info--;
    /* Output everything and free buffers if this was last close of this
       inst */
    if (ip->valid_info == 0) {
        if (UNDEF == linked_to_aux (ip) ||
            UNDEF == inv_output (ip))
            return (UNDEF);
        
        (void) free ((char *)ip->list_hd);
        (void) free ((char *)ip->list);
        (void) free ((char *)ip->aux_hd);
        (void) free ((char *)ip->num_list_buf);
        (void) free ((char *)ip->listwt_buf);

        if (num_inv_listwt > 0) {
            num_inv_listwt = 0;
            (void) free ((char *) inv_listwt);
        }

        if (UNDEF == close_inv (ip->inv_fd))
            return (UNDEF);
    }

    PRINT_TRACE (2, print_string, "Trace: leaving close_vec_inv_tmp");
    return (0);
#endif  /* MMAP */
}

#ifdef MMAP
static int
linked_to_aux (ip)
STATIC_INFO *ip;
{
    long con;
    long con_cache;
    long i;
    LISTWT *listwt_ptr;
    AUX_HD *aux_hd_ptr;
    LIST *linked_ptr;
    int nlb_fd, lwb_fd;
    char *end_temp_file;

    PRINT_TRACE (1, print_string, "Trace: entering linked_to_aux");

    /* Reserve space for this aux_buffer */
    if (ip->num_aux_hd >= ip->max_num_aux_hd) {
        ip->max_num_aux_hd += ip->num_aux_hd;
        if (NULL == (ip->aux_hd = (AUX_HD *)
                     realloc ((char *) ip->aux_hd,
                              sizeof (AUX_HD) *
                              (unsigned) (ip->max_num_aux_hd))))
            return (UNDEF);
    }
    aux_hd_ptr = &ip->aux_hd[ip->num_aux_hd];
    ip->num_aux_hd++;

    aux_hd_ptr->max_con = ip->num_list_hd;
    aux_hd_ptr->num_list = 0;

    /* Construct base name for temp files and open them */
    (void) sprintf (aux_hd_ptr->temp_file, "%s.%d.",
                    ip->base_temp_file, ip->num_aux_hd - 1);
    end_temp_file = &aux_hd_ptr->temp_file[strlen (aux_hd_ptr->temp_file)];
    (void) strcpy (end_temp_file, "nlb");
    if (-1 == (nlb_fd = open (aux_hd_ptr->temp_file,
                              O_WRONLY|O_CREAT|O_EXCL,
                              0600)))
        return (UNDEF);
    (void) strcpy (end_temp_file, "lwb");
    if (-1 == (lwb_fd = open (aux_hd_ptr->temp_file,
                              O_WRONLY|O_CREAT|O_EXCL,
                              0600)))
        return (UNDEF);
    *end_temp_file = '\0';

    listwt_ptr = ip->listwt_buf;
    /* Go through linked lists, constructing inverted lists as you go */
    /* Write out the constructed lists every 16K concepts */
    con_cache = 0;
    for (con = 0; con < ip->num_list_hd; con++) {
        ip->num_list_buf[con_cache] = ip->list_hd[con].num_linked;
        if (ip->num_list_buf[con_cache] != 0) {
            aux_hd_ptr->num_list += ip->num_list_buf[con_cache];
 
            linked_ptr = ip->list_hd[con].head;

            for (i = ip->list_hd[con].num_linked - 1; i >= 0; i--) {
                listwt_ptr[i].list = linked_ptr->did;
                listwt_ptr[i].wt = linked_ptr->weight;
                linked_ptr = linked_ptr->link;
            }
            listwt_ptr += ip->list_hd[con].num_linked;
        }
        con_cache++;
        if (con_cache == NLB_CACHE) {
            if (-1 == write (nlb_fd,
                             (char *) ip->num_list_buf,
                             (int) con_cache * sizeof (long)) ||
                -1 == write (lwb_fd,
                             (char *) ip->listwt_buf,
                             (int) (listwt_ptr - ip->listwt_buf) *
                             sizeof (LISTWT)))
                return (UNDEF);
            con_cache = 0;
            listwt_ptr = ip->listwt_buf;
        }
    }
    /* Write out remaining lists and close files */
    if (con_cache > 0) {
        if (-1 == write (nlb_fd,
                         (char *) ip->num_list_buf,
                         (int) con_cache * sizeof (long)) ||
            -1 == write (lwb_fd,
                         (char *) ip->listwt_buf,
                         (int) (listwt_ptr - ip->listwt_buf) *
                         sizeof (LISTWT)))
            return (UNDEF);
    }
    if (-1 == close (nlb_fd) ||
        -1 == close (lwb_fd))
        return (UNDEF);

    /* Reset linked lists to empty */
    bzero ((char *) ip->list_hd, 
           (int) ip->num_list_hd * sizeof(LIST_HD));
    ip->list_ptr = ip->list + 1;

    PRINT_TRACE (1, print_string, "Trace: leaving linked_to_aux");
    return (0);
}

/* Write the aux buffer(s) to inverted list, after possibly reading in old
   inverted list. */
static int
inv_output (ip)
STATIC_INFO *ip;
{
    INV new_inv, old_inv;
    unsigned long con;          /* Current concept to construct inv_list for */
    int aux_buf;                /* loop index of current aux buffer */
    long num_wanted;            /* total size of all aux buf lists for con */
    long num_listwt;            /* number in current aux buf list */
    LISTWT *listwt_ptr;         /* current position within inv_listwt of
                                   new inverted list for con */
    LISTWT *aux_listwt;         /* current position within aux buf list
                                   for con */
    LISTWT *end_aux_listwt;     /* end of aux buf list */
    AUX_HD *aux_hd_ptr;

    long max_con = 0;
    int status;
    long i;

    long *current_offset;       /* current_offset[i] gives the offset in
                                   aux_hd[i].list_buf for the next concept's
                                   list */

    int nlb_fd, lwb_fd;
    char *end_temp_file;
    
    PRINT_TRACE (1, print_string, "Trace: entering inv_output");

    if (NULL == (current_offset = (long *)
                 calloc ((unsigned) ip->num_aux_hd, sizeof (long))))
        return (UNDEF);
    for (aux_buf = 0; aux_buf < ip->num_aux_hd; aux_buf++) {
        aux_hd_ptr = &ip->aux_hd[aux_buf];
        if (max_con < aux_hd_ptr->max_con)
            max_con = aux_hd_ptr->max_con;

        /* Construct names for temp files, open them, map them into memory, and
           close files*/
        end_temp_file = &aux_hd_ptr->temp_file[strlen (aux_hd_ptr->temp_file)];
        (void) strcpy (end_temp_file, "nlb");
        if (-1 == (nlb_fd = open (aux_hd_ptr->temp_file, O_RDONLY)) ||
            (long *) -1 == (aux_hd_ptr->num_list_buf = (long *)
                            mmap ((caddr_t) 0,
                                  (int) aux_hd_ptr->max_con * sizeof (long),
                                  PROT_READ,
                                  MAP_SHARED,
                                  nlb_fd,
                                  (off_t) 0)) ||
            -1 == close (nlb_fd))
            return (UNDEF);
        (void) strcpy (end_temp_file, "lwb");
        if (-1 == (lwb_fd = open (aux_hd_ptr->temp_file, O_RDONLY)) ||
            (LISTWT *) -1 == (aux_hd_ptr->listwt_buf = (LISTWT *)
                            mmap ((caddr_t) 0,
                                  (int) aux_hd_ptr->num_list * sizeof (LISTWT),
                                  PROT_READ,
                                  MAP_SHARED,
                                  lwb_fd,
                                  (off_t) 0)) ||
            -1 == close (lwb_fd))
            return (UNDEF);
        *end_temp_file = '\0';
    }
    

    for (con = 0; con < max_con; con++) {
        num_wanted = 0;
        for (aux_buf = 0; aux_buf < ip->num_aux_hd; aux_buf++) {
            if (con < ip->aux_hd[aux_buf].max_con)
                num_wanted += ip->aux_hd[aux_buf].num_list_buf[con];
        }
        if (num_wanted == 0)
            continue;
 
        /* Get old inverted list entry */
        old_inv.id_num = con;
        old_inv.num_listwt = 0;
        if (ip->file_created == 0) {
            if (UNDEF == (status = seek_inv (ip->inv_fd, &old_inv))) {
                return (UNDEF);
            }
            if (status == 1 && 1 != read_inv (ip->inv_fd, &old_inv))
                return (UNDEF);
        }

        /* Merge (possibly empty) old list with new aux_lists */
        new_inv.num_listwt = num_wanted + old_inv.num_listwt;
        new_inv.id_num = con;
        
        /* First, make sure have enough room for new list */
        if (new_inv.num_listwt > num_inv_listwt) {
            if (num_inv_listwt != 0)
                (void) free ((char *) inv_listwt);
            num_inv_listwt += new_inv.num_listwt + 1000;
            if (NULL == (inv_listwt = (LISTWT *)
                         malloc (sizeof (LISTWT) * (unsigned) num_inv_listwt)))
                return (UNDEF);
        }
        new_inv.listwt = inv_listwt;

        listwt_ptr = inv_listwt;

        for (aux_buf = 0; aux_buf < ip->num_aux_hd; aux_buf++) {
            if (con >= ip->aux_hd[aux_buf].max_con)
                continue;
            num_listwt = ip->aux_hd[aux_buf].num_list_buf[con];
            if (num_listwt == 0)
                continue;
            aux_listwt = &ip->aux_hd[aux_buf].listwt_buf
                [current_offset[aux_buf]];
            current_offset[aux_buf] += num_listwt;
            if (old_inv.num_listwt == 0) {
                bcopy ((char *) aux_listwt,
                       (char *) listwt_ptr,
                       (int) num_listwt * sizeof (LISTWT));
                listwt_ptr += num_listwt;
            }
            else {
                end_aux_listwt = &aux_listwt[num_listwt];
                /* Must merge old list with this aux buffer */
                while (old_inv.num_listwt != 0 && aux_listwt < end_aux_listwt){
                    if (aux_listwt->list < old_inv.listwt->list) {
                        listwt_ptr->list = aux_listwt->list;
                        listwt_ptr->wt = aux_listwt->wt;
                        listwt_ptr++;
                        aux_listwt++;
                    }
                    else {
                        listwt_ptr->list = old_inv.listwt->list;
                        listwt_ptr->wt = old_inv.listwt->wt;
                        listwt_ptr++;
                        old_inv.listwt++;
                        old_inv.num_listwt--;
                    }
                }
                while (aux_listwt < end_aux_listwt) {
                    listwt_ptr->list = aux_listwt->list;
                    listwt_ptr->wt = aux_listwt->wt;
                    listwt_ptr++;
                    aux_listwt++;
                }
            }
        }
        while (old_inv.num_listwt != 0) {
            listwt_ptr->list = old_inv.listwt->list;
            listwt_ptr->wt = old_inv.listwt->wt;
            listwt_ptr++;
            old_inv.listwt++;
            old_inv.num_listwt--;
        }

        /* Write new inverted list */
        if (UNDEF == seek_inv (ip->inv_fd, &new_inv) ||
            UNDEF == write_inv (ip->inv_fd, &new_inv))
            return (UNDEF);
    }

    /* Free all aux buffers */
    for (i = 0; i < ip->num_aux_hd; i++) {
        aux_hd_ptr = &ip->aux_hd[i];
        end_temp_file = &aux_hd_ptr->temp_file[strlen (aux_hd_ptr->temp_file)];
        if (UNDEF == munmap ((char *) aux_hd_ptr->num_list_buf,
                             (int) aux_hd_ptr->max_con * sizeof (long)) ||
            UNDEF == munmap ((char *) aux_hd_ptr->listwt_buf,
                             (int) aux_hd_ptr->num_list * sizeof (LISTWT)))
            return (UNDEF);
        /* Construct names for temp files, and remove them */
        (void) strcpy (end_temp_file, "nlb");
        if (-1 == unlink (aux_hd_ptr->temp_file))
            return (UNDEF);
        (void) strcpy (end_temp_file, "lwb");
        if (-1 == unlink (aux_hd_ptr->temp_file))
            return (UNDEF);
        *end_temp_file = '\0';
    }
    ip->num_aux_hd = 0;

    (void) free ((char *) current_offset);

    PRINT_TRACE (1, print_string, "Trace: leaving inv_output");

    return (0);
}


#endif  /* MMAP */
