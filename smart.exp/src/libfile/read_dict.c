#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libfile/read_dict.c,v 11.0 1992/07/21 18:20:54 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "rel_header.h"
#include "dict.h"
#include "io.h"
#include "smart_error.h"

extern _SDICT _Sdict[];

int
read_dict (dict_index, dict_entry)
int dict_index;
DICT_ENTRY *dict_entry;
{
    HASH_ENTRY *hash_node;
    register _SDICT *dict_ptr = &_Sdict[dict_index];
    long concept;
    char *_Sget_string_node();
    long number_read = 0;

    if (dict_ptr->in_ovfl_file) {
        if (dict_ptr->next_ovfl_index != UNDEF) {
            number_read = read_dict (dict_ptr->next_ovfl_index,
                                     dict_entry);
        }
        if (number_read == 1) {
            dict_entry->con += dict_ptr->hsh_tab_size;
        }
        return (number_read);
    }

    hash_node = dict_ptr->current_hash_entry;

    if (dict_ptr->current_position == END_DICT_ENTRY ||
        EMPTY_NODE (hash_node)) {

        /* Find next hash_node which has a valid entry */
        concept = dict_ptr->current_concept + 1;
        GET_HASH_NODE (concept, dict_ptr, hash_node);
        while (NULL != hash_node && EMPTY_NODE(hash_node)) {
            concept ++;
            GET_HASH_NODE (concept, dict_ptr, hash_node);
        }
        if (NULL == hash_node) {
            /* End of dictionary, try to read more from overflow file */
            
            if (dict_ptr->next_ovfl_index != UNDEF) {
                dict_ptr->in_ovfl_file = 1;

                /* Position at beginning of overflow file */
                (void) seek_dict (dict_ptr->next_ovfl_index, 
                                  (DICT_ENTRY *) NULL);

                /* Read from overflow file */
                number_read = read_dict (dict_ptr->next_ovfl_index,
                                         dict_entry);
                if (number_read == 1) {
                    dict_entry->con += dict_ptr->hsh_tab_size;
                }
            }
            return (number_read);
        }
        else {
            dict_ptr->current_hash_entry = hash_node;
            dict_ptr->current_concept = concept;
        }
    }

    GET_STRING_NODE (hash_node->str_tab_off, dict_ptr, dict_entry->token);
    if (NULL == dict_entry->token) {
        set_error (SM_ILLSK_ERR, dict_ptr->file_name, "read_dict");
        return (UNDEF);
    }
    dict_entry->info  = hash_node->info;
    dict_entry->con   = dict_ptr->current_concept;
        
    dict_ptr->current_position = END_DICT_ENTRY;

    return (1);
}
