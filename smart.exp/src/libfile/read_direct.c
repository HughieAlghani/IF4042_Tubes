#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libfile/read_direct.c,v 11.0 1992/07/21 18:20:56 chrisb Exp $";
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
#include "direct.h"
#include "io.h"
#include "smart_error.h"

extern _SDIRECT _Sdirect[];
extern DIRECT_TYPES direct_object_type[];
static long get_fixed_value();

int
read_direct(dir_index, out_direct)
int dir_index;              /* index to fast-direct direct */
register char *out_direct;      /* structure pointer for direct */
{
    char node[MAX_DIRECT_SIZE];
    long entry_num;             /* id number for node */

    register _SDIRECT *dir_ptr = &(_Sdirect[dir_index]);
    long num_read;

    long var_offset;            /* position of first entry for tuple in */
                                /* var rec file. */
    char *var_ptr;              /* pointer to current entry in var buffer */
    
    long size_wanted;           /* actual length var field for an attribute */
    long total_size_wanted;           

    register DIRECT_TYPES *dir_type_ptr;/* Pointer to information about what*/
                                /* this relational type looks like */
    register ATT_TYPES *att_ptr; /* Pointer to what current attribute looks */
                                /* like */
    ATT_TYPES *temp_att_ptr;
    ATT_TYPES *end_att_ptr;


    if (dir_ptr->mode & SWRONLY) {
        set_error (SM_ILLMD_ERR, dir_ptr->file_name, "read_direct");
        return (UNDEF);
    }

    /* Get next non_empty record in fast access file */
    if (dir_ptr->mode & SINCORE) {
        if (dir_ptr->fix_pos >= dir_ptr->fix_enddata) {
            /* if EOF, return 0 to caller */
            return(0);
        }

        bcopy (dir_ptr->fix_pos, (char *) &entry_num, sizeof (long));

        while (entry_num == UNDEF) {  /* if node is UNDEF */
            /* Disregard this node, advance to the next one */
            dir_ptr->fix_pos += dir_ptr->rh._entry_size;
            if (dir_ptr->fix_pos >= dir_ptr->fix_enddata) {
                /* if EOF, return 0 to caller */
                return(0);
            }
            bcopy (dir_ptr->fix_pos, (char *) &entry_num, sizeof (long));
        }
        bcopy (dir_ptr->fix_pos, node, dir_ptr->rh._entry_size);
        dir_ptr->fix_pos += dir_ptr->rh._entry_size;
    }
    else {
        /* fix file is not in core, bring entries in (if needed) */
        if (dir_ptr->fix_foffset >= dir_ptr->fix_size) {
            /* if EOF return 0 to caller */
            return (0);
        }
        
        if (dir_ptr->fix_pos == dir_ptr->fix_mem) {
            /* Entry read during last seek may be valid */
            /* NOTE: depend on last action being seek to avoid physical */
            /* seek of file */
            bcopy (dir_ptr->fix_pos, (char *) &entry_num, sizeof (long));
            bcopy (dir_ptr->fix_pos, node, dir_ptr->rh._entry_size);
            dir_ptr->fix_foffset += dir_ptr->rh._entry_size;
        }
        else {
            entry_num = UNDEF;
            (void) lseek (dir_ptr->fix_fd, dir_ptr->fix_foffset, 0);
        }
        while (entry_num == UNDEF) {
            /* Disregard this direct node, advance to the next one */
            if (dir_ptr->fix_foffset >= dir_ptr->fix_size) {
                /* if EOF, return 0 to caller */
                dir_ptr->fix_pos = NULL;
                return(0);
            }
            dir_ptr->fix_foffset += dir_ptr->rh._entry_size;
            if (dir_ptr->rh._entry_size != 
                            read (dir_ptr->fix_fd, 
                                  node,
                                  dir_ptr->rh._entry_size)) {
                set_error (errno, dir_ptr->file_name, "read_direct");
                dir_ptr->fix_pos = NULL;
                return (UNDEF);
            }
            bcopy (node, (char *) &entry_num, sizeof (long));
        }
        dir_ptr->fix_pos = NULL;
    }

    /* Copy all of the fixed entry information */
    bcopy (node, out_direct, dir_ptr->rh._entry_size - sizeof (long));

    bcopy (node + (dir_ptr->rh._entry_size - sizeof (long)), 
           (char *) &var_offset,
           sizeof (long));

    /* Set pointers to fields in var, possibly bringing in core */
    /* if SINCORE not set */
    dir_type_ptr = &direct_object_type[dir_ptr->rh.type];
    end_att_ptr = &dir_type_ptr->atts[dir_type_ptr->num_fixed_fields +
                                      dir_type_ptr->num_var_fields];
        
    /* Determine where in memory the variable records for this tuple start */
    /* If not in core, they need to be brought into core */
    if (dir_ptr->mode & SINCORE) {
        var_ptr = dir_ptr->var_mem + var_offset;
    }
    else {
        total_size_wanted = 0;
        for (att_ptr = &dir_type_ptr->atts[dir_type_ptr->num_fixed_fields];
             att_ptr < end_att_ptr;
             att_ptr++) {
            /* Associated field is (char) UNDEF if field is string */
            if (att_ptr->associated_field ==  (char) UNDEF) {
                size_wanted = PATH_LEN;
            }
            else {
                temp_att_ptr = &dir_type_ptr->atts[att_ptr->associated_field];
                size_wanted = att_ptr->field_size * 
                    get_fixed_value (out_direct + temp_att_ptr->field_offset,
                                    (int) temp_att_ptr->field_size);
            }
            total_size_wanted += (size_wanted + 3) &~ 3;
        }

        if (total_size_wanted > dir_ptr->var_end - dir_ptr->var_mem) {
            if (NULL == (dir_ptr->var_mem = realloc (dir_ptr->var_mem,
                                            (unsigned)total_size_wanted))) {
                set_error (errno, dir_ptr->file_name, "read_direct");
                dir_ptr->var_end = NULL;
                return (UNDEF);
            }
            dir_ptr->var_end = dir_ptr->var_mem + total_size_wanted;
        }
        
        if (total_size_wanted > 0) {
            (void) lseek (dir_ptr->var_fd, (long) var_offset, 0);
            if (0 >= (num_read = read (dir_ptr->var_fd, 
                                       dir_ptr->var_mem,
                                       (int) total_size_wanted))) {
                set_error (errno, dir_ptr->file_name, "read_direct");
                return (UNDEF);
            }
            dir_ptr->var_enddata = dir_ptr->var_mem + num_read;
        }
        else 
            dir_ptr->var_enddata = dir_ptr->var_mem;
        dir_ptr->var_foffset = var_offset;

        var_ptr = dir_ptr->var_mem;
    }


    /* Assign pointer to each variable record field */
    /* NOTE: Assume that all pointers are of the same size */
    for (att_ptr = &dir_type_ptr->atts[dir_type_ptr->num_fixed_fields];
         att_ptr < end_att_ptr;
         att_ptr++) {
        bcopy ((char *) &var_ptr, 
               out_direct + att_ptr->field_offset,
               sizeof (char *));
        if (att_ptr->associated_field == (char) UNDEF) {
            /* Associated field is (char) UNDEF if field is string */
            size_wanted = strlen (var_ptr) + 1;
        }
        else {
            temp_att_ptr = &dir_type_ptr->atts[att_ptr->associated_field];
            size_wanted = att_ptr->field_size * 
               get_fixed_value (out_direct + temp_att_ptr->field_offset,
                                (int) temp_att_ptr->field_size);
        }
        var_ptr += (size_wanted + 3) &~ 3;
    }

    return (1);
}


/* Interpret each legal value as an integer - no problem if not since */
/* will never be referenced. Return 0 if error */
/* WARNING: assumes length cannot be 8 bytes; problem when 64bit machines */
/* come out? */
/* WARNING: This code is exactly duplicated in write_direct.c (so a smart */
/* compiler can in-line the code) Any changes here must be mirrored there */
static long
get_fixed_value (ptr, length)
char *ptr;
int length;
{
    short temp_short;
    long   temp_long;

    switch (length) {
        case 1:
            return ((long) *ptr);
        case 2: 
            bcopy (ptr, (char *) &temp_short, 2);
            return ((long) temp_short);
        case 4:
            bcopy (ptr, (char *) &temp_long, 4);
            return ((long) temp_long);
        default:
            /* Value is not an integer, so return 0 */
            return (0);
    }
}

