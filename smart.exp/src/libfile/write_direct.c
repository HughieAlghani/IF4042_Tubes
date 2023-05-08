#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libfile/write_direct.c,v 11.0 1992/07/21 18:21:02 chrisb Exp $";
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
static int write_file_entry();

int
write_direct (dir_index, direct_object)
int dir_index;
register char *direct_object;
{
    register _SDIRECT *dir_ptr = &(_Sdirect[dir_index]);
    char file_entry[MAX_DIRECT_SIZE];

    long node;                  /* Temporary variables for setting current */
                                /* node to be written */
    long entry_num;

    long var_offset;            /* Byte offset within var file of start of */
                                /* this nodes variable record */

    long  raw_size_wanted;       /* actual length var field for an attribute */
    long  size_wanted;           /* above padded to full word boundary */

    register DIRECT_TYPES *dir_type_ptr;/* Pointer to information about what*/
                                /* this relational type looks like */
    register ATT_TYPES *att_ptr; /* Pointer to what current attribute looks */
                                /* like */
    ATT_TYPES *temp_att_ptr;
    ATT_TYPES *end_att_ptr;
    
    register char *ptr;
    long undef;
    unsigned long new_size;

    if (! (dir_ptr->mode & (SRDWR | SWRONLY))) {
        set_error (errno, dir_ptr->file_name, "write_direct");
        return (UNDEF);
    }

    /* Make sure fix relation (or file) will still be complete after this */
    /* tuple has been added. */
    if (dir_ptr->mode & SINCORE) {
        /* If entry_num of object is UNDEF, and we are at end of data, */
        /* then entry_num needs to be set - */
        /* this is a new node being entered into the system */
        entry_num = *((long *) direct_object);
        if ( entry_num == UNDEF && dir_ptr->fix_pos >= dir_ptr->fix_enddata) {
            *((long *) direct_object) = entry_num =
                                (dir_ptr->fix_foffset - sizeof (REL_HEADER) +
                                  (dir_ptr->fix_enddata - dir_ptr->fix_mem)) /
                                 dir_ptr->rh._entry_size;
        }

        /* Need to possibly "zero" out portions of fixed file if this */
        /* location is beyond end of present initialized portion. */
        while (dir_ptr->fix_pos >= dir_ptr->fix_end) {
            undef = UNDEF;
            new_size = (dir_ptr->fix_end - dir_ptr->fix_mem) +
                (dir_ptr->fix_end - dir_ptr->fix_mem) +
                    dir_ptr->rh._entry_size * FIX_INCR;
            if (NULL == (ptr = realloc (dir_ptr->fix_mem,
                                        (unsigned) new_size))) {
                set_error (errno, dir_ptr->file_name, "write_direct");
                return (UNDEF);
            }
            dir_ptr->fix_pos = ptr + (dir_ptr->fix_pos - dir_ptr->fix_mem);
            dir_ptr->fix_end = ptr + new_size;
            dir_ptr->fix_enddata = ptr +
                                  (dir_ptr->fix_enddata - dir_ptr->fix_mem);
            dir_ptr->fix_mem = ptr;
            /* Initialize all new nodes to entry_num of UNDEF */
            for ( ptr = dir_ptr->fix_enddata;
                  ptr < dir_ptr->fix_end; 
                  ptr += dir_ptr->rh._entry_size) {
                bcopy ((char *) &undef, ptr, sizeof (long));
                bzero (((char *) ptr) + sizeof (long),
                        dir_ptr->rh._entry_size - sizeof (long));
            }
        }

        /* Initialize present idea of node */
        bcopy (dir_ptr->fix_pos, (char *) &node, sizeof (long));
    }
    else {
        /* If entry_num of object is UNDEF, and the present location is */
        /* beyond the end of the file, then entry_num needs to be set - */
        /* this is a new node being entered into the system */
        entry_num = *((long *) direct_object);
        if (entry_num == UNDEF && dir_ptr->fix_foffset >= dir_ptr->fix_size) {
            *((long *) direct_object) = entry_num = 
                                (dir_ptr->fix_foffset - sizeof (REL_HEADER)) /
                                 dir_ptr->rh._entry_size;
        }

        /* Initialize present idea of node */
        if (dir_ptr->fix_pos == dir_ptr->fix_mem) {
            bcopy (dir_ptr->fix_pos, (char *) &node, sizeof (long));
        }
        else {
            node = UNDEF;
        }

        /* fix file is not in core, may have to write out all intermediate */
        /* entries if there are holes. Seek to desired foffset */
        if (dir_ptr->fix_foffset >= dir_ptr->fix_size) {
            (void) lseek (dir_ptr->fix_fd, 0L, 2);
            undef = UNDEF;
            bcopy ((char *) &undef, file_entry, sizeof (long));
            bzero (((char *) file_entry) + sizeof (long),
                   dir_ptr->rh._entry_size - sizeof (long));
            while (dir_ptr->fix_foffset > dir_ptr->fix_size) {
                if (dir_ptr->rh._entry_size != 
                                        write (dir_ptr->fix_fd, 
                                               file_entry, 
                                               dir_ptr->rh._entry_size)) {
                    set_error (errno, dir_ptr->file_name, "write_direct");
                    return (UNDEF);
                }
                dir_ptr->fix_size += dir_ptr->rh._entry_size;
            }
        }
        else {
            (void) lseek (dir_ptr->fix_fd, dir_ptr->fix_foffset, 0);
        }

    }        

    /* Update count of number of tuples in relation */
    if (entry_num > dir_ptr->rh.max_primary_value) {
        dir_ptr->rh.max_primary_value = entry_num;
    }
    if (entry_num == UNDEF) {
        if (node != UNDEF) {
            dir_ptr->rh.num_entries--;
        }
    }
    else {
        if (node != UNDEF) {
            if (node != entry_num) {
                set_error (SM_INCON_ERR, dir_ptr->file_name, "write_direct");
                return (UNDEF);
            }
        }
        else {
            dir_ptr->rh.num_entries++;
        }
    }

    if (entry_num == UNDEF) {
        /* Deleting existing node. Only need to overwrite fixed file */
        bcopy (direct_object, 
               file_entry, 
               dir_ptr->rh._entry_size - sizeof(long));
        if (UNDEF == write_file_entry (dir_ptr, file_entry)) {
            return (UNDEF);
        }
        return (0);
    }

    /* Calculate offset of current end of var tuples - this is where */
    /* current tuples will be written (Never overwrite existing var tuples) */
    /* If not SINCORE, need to seek to end of file */
    if (dir_ptr->mode & SINCORE) {
        var_offset = dir_ptr->var_foffset + 
                        (dir_ptr->var_enddata - dir_ptr->var_mem);
    }
    else {
        (void) lseek (dir_ptr->var_fd, 0L, 2);
        var_offset = dir_ptr->var_size;
        dir_ptr->var_enddata = dir_ptr->var_mem;
    }

    dir_type_ptr = &direct_object_type[dir_ptr->rh.type];

    /* Fill in fix information in temporary node file_entry. Do not write */
    /* out file_entry until after var entries have been written (to ensure */
    /* reliability of fix information) */
    bcopy (direct_object, file_entry, dir_ptr->rh._entry_size - sizeof(long));
    if (dir_type_ptr->num_var_fields != 0) {
        bcopy ((char *) &var_offset,
               file_entry + (dir_ptr->rh._entry_size - sizeof (long)),
               sizeof (long));
    }

    end_att_ptr = &dir_type_ptr->atts[dir_type_ptr->num_fixed_fields +
                                      dir_type_ptr->num_var_fields];
    
    for (att_ptr = &dir_type_ptr->atts[dir_type_ptr->num_fixed_fields];
         att_ptr < end_att_ptr;
         att_ptr++) {
        /* The number of tuples in this var attribute is indicated by the */
        /* value of the associated field in direct object EXCEPT if this is */
        /* a string var attribute - indicated by associated field = UNDEF */
        if (att_ptr->associated_field == (char) UNDEF) {
            raw_size_wanted = 1 + strlen (* (char **) (direct_object + 
                                                      att_ptr->field_offset));
        }
        else {
            temp_att_ptr = &dir_type_ptr->atts[att_ptr->associated_field];
            raw_size_wanted = att_ptr->field_size * 
                get_fixed_value (direct_object + temp_att_ptr->field_offset,
                                 (int) temp_att_ptr->field_size);
        }
        size_wanted = (raw_size_wanted + 3) &~ 3;

        /* add attibute to incore buffer */
        if (dir_ptr->var_enddata + size_wanted >= dir_ptr->var_end) {
            new_size = (dir_ptr->var_end - dir_ptr->var_mem) +
                (dir_ptr->var_end - dir_ptr->var_mem) + VAR_INCR + size_wanted;
            if (NULL == (ptr = realloc (dir_ptr->var_mem,
                                        (unsigned) new_size))) {
                set_error (errno, dir_ptr->file_name, "write_direct");
                return (UNDEF);
            }
            dir_ptr->var_enddata = ptr + 
                                (dir_ptr->var_enddata - dir_ptr->var_mem);
            dir_ptr->var_end = ptr + new_size;
            dir_ptr->var_mem = ptr;
        }

        bcopy (*(char **)(direct_object + att_ptr->field_offset),
               dir_ptr->var_enddata, 
               (int) raw_size_wanted);
        if (raw_size_wanted != size_wanted) {
            /* Pad to full word boundary with zeros */
            bzero (dir_ptr->var_enddata + raw_size_wanted,
                   (int) (size_wanted - raw_size_wanted));
        }
        dir_ptr->var_enddata += size_wanted;
    }
    
    /* Actually write out variable record if not SINCORE */
    if (! (dir_ptr->mode & SINCORE)) {
        size_wanted = dir_ptr->var_enddata - dir_ptr->var_mem;
        if (size_wanted) {
            if (size_wanted != write (dir_ptr->var_fd,
                                      dir_ptr->var_mem,
                                      (int) size_wanted)) {
                set_error (errno, dir_ptr->file_name, "write_direct");
                return (UNDEF);
            }
            dir_ptr->var_size += size_wanted;
        }
    }

    /* Actually add the file_entry to the existing relation or file */
    if (UNDEF == write_file_entry (dir_ptr, file_entry)) {
        return (UNDEF);
    }

    return (1);
}


/* Interpret each legal value as an integer - no problem if not since */
/* will never be referenced. Return 0 if error */
/* WARNING: assumes length cannot be 8 bytes; problem when 64bit machines */
/* come out? */
/* WARNING: This code is exactly duplicated in read_direct.c (so a smart */
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

/* Actually add the file_entry to the existing fixed relation or file */
static int
write_file_entry (dir_ptr, file_entry)
register _SDIRECT *dir_ptr;
register char *file_entry;
{
    if (dir_ptr->mode & SINCORE) {
        bcopy (file_entry, 
               dir_ptr->fix_pos, 
               dir_ptr->rh._entry_size);
        dir_ptr->fix_pos  += dir_ptr->rh._entry_size;
        if (dir_ptr->fix_pos > dir_ptr->fix_enddata) {
            dir_ptr->fix_enddata = dir_ptr->fix_pos;
        }
    }
    else {
        if (dir_ptr->rh._entry_size != 
                                write (dir_ptr->fix_fd, 
                                       file_entry, 
                                       dir_ptr->rh._entry_size)) {
            set_error (errno, dir_ptr->file_name, "write_direct");
            dir_ptr->fix_pos = NULL;
            return (UNDEF);
        }
        dir_ptr->fix_foffset += dir_ptr->rh._entry_size;
        if (dir_ptr->fix_foffset > dir_ptr->fix_size) {
            dir_ptr->fix_size = dir_ptr->fix_foffset;
        }
    }
    return (0);
}
