#ifndef DICTH
#define DICTH
/*        $Header: /home/smart/release/src/h/dict.h,v 11.0 1992/07/21 18:18:34 chrisb Exp $*/

#ifndef S_RH_DICT
#include "rel_header.h"
#endif

typedef struct {
    char  *token;               /* actual string */
    long info;                  /* Usage dependent field */
    long  con;                  /* unique index for this token */
} DICT_ENTRY;


typedef struct hash_entry {
    short collision_ptr;               /* offset of next word hashed here   */
                                       /* If IN_OVF_TABLE, then need to go to*/
                                       /* (next) overflow hash table */
    char prefix[2];                    /* First two bytes of string */
    long str_tab_off;                  /* position of string (byte offset)  */
    long info;                         /* Usage dependent field */
} HASH_ENTRY;

/* Maximum value of collision_ptr. Constrained to be less than 2**15. */
#define MAX_DICT_OFFSET  (512)

#define IN_OVFL_TABLE -2
#define EMPTY_NODE(h) ((h)->str_tab_off == UNDEF)
#define VALID_DICT_INDEX(i) (i >= 0 && i < MAX_NUM_DICT && dict[i].opened)

#define DICT_MALLOC_SIZE (64 * 1024 - 40)

#define MAX_NUM_DICT 40

#define GET_HASH_NODE(concept,dict_ptr,hash_node) \
    if (concept < 0 || concept >= dict_ptr->hsh_tab_size) \
        hash_node = NULL; \
    else { \
        if (dict_ptr->mode & SINCORE) { \
            hash_node = &dict_ptr->hsh_table[concept]; \
        } \
        else { \
            if (-1 == lseek (dict_ptr->fd, \
                             (long) (sizeof (REL_HEADER) + \
                                     (concept * sizeof (HASH_ENTRY))), \
                             0) || \
                sizeof (HASH_ENTRY) != read ( dict_ptr->fd, \
                                      (char *) &(dict_ptr->actual_hash_entry),\
                                      sizeof(HASH_ENTRY))) { \
                set_error (errno, dict_ptr->file_name, "dict");\
                return (UNDEF); \
            } \
           hash_node = &(dict_ptr->actual_hash_entry); \
        } \
    }

#define GET_STRING_NODE(string_offset,dict_ptr,string) \
    if (string_offset == UNDEF) \
        string = NULL; \
    else { \
        if (dict_ptr->mode & SINCORE) { \
            if (string_offset > dict_ptr->str_tab_size) { \
                set_error (SM_ILLSK_ERR, dict_ptr->file_name, "dict"); \
                return (UNDEF); \
            } \
            else \
                string = &dict_ptr->str_table[string_offset]; \
        } \
        else { \
            (void) lseek (dict_ptr->fd, \
                          (long) (sizeof (REL_HEADER) + \
                                  (dict_ptr->rh.max_primary_value * \
                                   sizeof (HASH_ENTRY)) + \
                                  string_offset), \
                          0); \
            if (-1 == read(dict_ptr->fd, \
                           dict_ptr->current_token, \
                           MAX_TOKEN_LEN)) { \
                set_error (errno, dict_ptr->file_name, "dict"); \
                return (UNDEF); \
            } \
            string = dict_ptr->current_token; \
        } \
    }


typedef struct {
    char file_name[PATH_LEN];
    int  fd;
    long file_size;

    HASH_ENTRY *hsh_table;
    char       *str_table;
    long hsh_tab_size;
    long str_tab_size;
    char *str_next_loc;

    HASH_ENTRY actual_hash_entry;
    HASH_ENTRY *current_hash_entry;
    long current_concept;
    char current_token[MAX_TOKEN_LEN]; /* Buffer to read current token into */
    long mode;
#define BEGIN_DICT_ENTRY 0
#define END_DICT_ENTRY   1
    char current_position;      /* if BEGIN, current_hash_entry is the valid */
				/* entry to be returned for next read or */
				/* write. If END, current_hash_entry is the */
				/* entry preceding the current position. */

    char opened;                /* Valid header if opened */
    char shared;                /* If 1, then buffers are shared. */
    char  in_ovfl_file;
    short next_ovfl_index;
    char  next_ovfl_file_name[PATH_LEN];
    REL_HEADER rh;
} _SDICT;
#endif /* DICTH */
