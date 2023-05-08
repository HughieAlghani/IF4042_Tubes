#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libfile/seek_direct.c,v 11.0 1992/07/21 18:21:00 chrisb Exp $";
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


int
seek_direct (dir_index, direct_object)
int dir_index;
register char *direct_object;   /* the first 4 bytes of direct_object */
                                /* is entry_num to locate "reader" at*/
{   
    char *fix_ptr;
    register _SDIRECT *dir_ptr = &(_Sdirect[dir_index]);
    long node;
    long entry_num;

    if (dir_ptr->mode & SINCORE) {
        if (direct_object == NULL) {
            /* Seek to beginning of file */
            dir_ptr->fix_pos = dir_ptr->fix_mem;
            return (1);
        }
        
        entry_num = *((long *) direct_object);
        if (entry_num == UNDEF) {
            /* seek to end of file, to prepare for a write of a new node */
            dir_ptr->fix_pos = dir_ptr->fix_enddata;
            return (0);
        }
    
        /* 'seek' to location in fast-access to read in node */
        fix_ptr = dir_ptr->fix_mem + entry_num * dir_ptr->rh._entry_size;
        dir_ptr->fix_pos = fix_ptr;
        
        if (fix_ptr >= dir_ptr->fix_enddata) {
            /* did is too large */
            return (0);
        }
    
        /* get node number */
        bcopy (fix_ptr, (char *) &node, 4);
    
        if (node == UNDEF) {
            /* node has been removed or was never there */
            return (0);
        }
    
        if (node != entry_num) {
            set_error (SM_INCON_ERR, dir_ptr->file_name, "seek_direct");
            return (UNDEF);
        }
    
        /* return successful completion */
        return (1);
    }
    else { /* fix file in not in core */
        if (direct_object == NULL) {
            /* Seek to beginning of file and read first entry (if exists) */
            dir_ptr->fix_foffset = sizeof (REL_HEADER);
            (void) lseek (dir_ptr->fix_fd, dir_ptr->fix_foffset, 0);
            if (dir_ptr->fix_size <= sizeof (REL_HEADER)) {
                dir_ptr->fix_pos = NULL;
            }
            else {
                if (dir_ptr->rh._entry_size != 
                                read (dir_ptr->fix_fd, 
                                      dir_ptr->fix_mem,
                                      dir_ptr->rh._entry_size)) {
                    set_error (errno, dir_ptr->file_name, "seek_direct");
                    dir_ptr->fix_pos = NULL;
                    return (UNDEF);
                }
                dir_ptr->fix_pos = dir_ptr->fix_mem;
            }
            
            return (1);
        }
        
        entry_num = *((long *) direct_object);
        if (entry_num == UNDEF) {
            /* "seek" to end of file, to prepare for a write of a new node */
            dir_ptr->fix_foffset = dir_ptr->fix_size;
            dir_ptr->fix_pos = NULL;
            return (0);
        }
    
        dir_ptr->fix_foffset = entry_num * dir_ptr->rh._entry_size
                                + sizeof (REL_HEADER);

        if (dir_ptr->fix_foffset >= dir_ptr->fix_size) {
            dir_ptr->fix_pos = NULL;
            return (0);
        }
        
        (void) lseek (dir_ptr->fix_fd, dir_ptr->fix_foffset, 0);
        if (dir_ptr->rh._entry_size != 
                        read (dir_ptr->fix_fd,
                              dir_ptr->fix_mem,
                              dir_ptr->rh._entry_size)) {
            set_error (errno, dir_ptr->file_name, "seek_direct");
            dir_ptr->fix_pos = NULL;
            return (UNDEF);
        }
        dir_ptr->fix_pos = dir_ptr->fix_mem;
        
        /* get node number */
        bcopy (dir_ptr->fix_mem, (char *) &node, 4);
    
        if (node == UNDEF) {
            /* node has been removed or was never there */
            return (0);
        }
    
        if (node != entry_num) {
            set_error (SM_INCON_ERR, dir_ptr->file_name, "seek_direct");
            dir_ptr->fix_pos = NULL;
            return (UNDEF);
        }
    
        /* return successful completion */
        return (1);
    }
}
