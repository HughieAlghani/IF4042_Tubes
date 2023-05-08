#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libfile/close_direct.c,v 11.0 1992/07/21 18:20:48 chrisb Exp $";
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
close_direct (dir_index)
int dir_index;
{
    int fd;
    register _SDIRECT *dir_ptr = &_Sdirect[dir_index];
    char var_filename[PATH_LEN];
    long fix_size;
    long var_size;
    _SDIRECT *tdir_ptr;

    if (dir_ptr->opened == 0) {
        set_error (EBADF, dir_ptr->file_name, "close_direct");
        return (UNDEF);
    }

    if (dir_ptr->mode & (SWRONLY | SRDWR)) {
        /* Write out var file if needed */
        if ((dir_ptr->mode & SINCORE) && (dir_ptr->mode & SRDWR)) {
            /* Entire var file is in core. Backup and write entire file */

            /* Construct var file name */
            (void) sprintf (var_filename, "%s.var", dir_ptr->file_name);

#ifdef VAR_BACKUP
            if (dir_ptr->mode & SBACKUP) {
                if (UNDEF == (fd = prepare_backup (var_filename))) {
                    return (UNDEF);
                }
            }
            else 
#endif            
            if (-1 == (fd = open (var_filename, SWRONLY))) {
                set_error (errno, var_filename, "close_direct");
                return (UNDEF);
            }

            var_size = dir_ptr->var_enddata - dir_ptr->var_mem;

            if (var_size != write (fd,
                                   (char *) dir_ptr->var_mem,
                                   (int) var_size)) {
                set_error (errno, var_filename, "close_direct");
                return (UNDEF);
            }
            if (dir_ptr->shared == 0) {
                (void) free (dir_ptr->var_mem);
            }
            (void) close (fd);
        }
        else if ((dir_ptr->mode & SINCORE) && (dir_ptr->mode & SWRONLY)) {
            /* Only newly written var tuples are in core. Copy the */
            /* existing file for backup, and append the new tuples */

            /* Construct var file name */
            (void) sprintf (var_filename, "%s.var", dir_ptr->file_name);

            if (-1 == (fd = open (var_filename, SRDWR))) {
                set_error (errno, var_filename, "close_direct");
                return (UNDEF);
            }

#ifdef VAR_BACKUP
            if (dir_ptr->mode & SBACKUP) {
                if (UNDEF == (new_fd = prepare_backup (var_filename)) ||
                    UNDEF == copy_fd (fd, new_fd)) {
                    return (UNDEF);
                }
                (void) close (fd);
                fd = new_fd;
            }
#endif
            var_size = dir_ptr->var_enddata - dir_ptr->var_mem;

            (void) lseek (fd, dir_ptr->var_foffset, 0);
            if (var_size != write (fd,
                                   (char *) dir_ptr->var_mem,
                                   (int) var_size)) {
                set_error (errno, var_filename, "close_direct");
                return (UNDEF);
            }
            if (dir_ptr->shared == 0) {
                (void) free (dir_ptr->var_mem);
            }
            (void) close (fd);
        }
        else { /* SINCORE is not set */
            /* var file is all done. Just close file */
            (void) close (dir_ptr->var_fd);
            if (dir_ptr->shared == 0) {
                (void) free (dir_ptr->var_mem);
            }
        }

#ifdef VAR_BACKUP
        if (dir_ptr->mode & SBACKUP) {
            if (UNDEF == make_backup (var_filename)) {
                return (UNDEF);
            }
        }
#endif

        /* Write out fix file */
        if (dir_ptr->mode & SINCORE) {
            if (dir_ptr->mode & SBACKUP) {
                if (UNDEF == (fd = prepare_backup (dir_ptr->file_name))) {
                    return (UNDEF);
                }
            }
            else if (-1 == (fd = open (dir_ptr->file_name, SWRONLY))) {
                set_error (errno, dir_ptr->file_name, "close_direct");
                return (UNDEF);
            }

            fix_size = dir_ptr->fix_enddata - dir_ptr->fix_mem;

            /* Consistency check */
            if (fix_size != (dir_ptr->rh.max_primary_value+1) * 
                            dir_ptr->rh._entry_size) {
                set_error (SM_INCON_ERR, dir_ptr->file_name, "close_direct");
                return (UNDEF);
            }

            if (sizeof (REL_HEADER) != write (fd, 
                                              (char *) &dir_ptr->rh,
                                              (int) sizeof (REL_HEADER))) {
                set_error (errno, dir_ptr->file_name, "close_direct");
                return (UNDEF);
            }

            if (fix_size != write (fd,
                                   (char *) dir_ptr->fix_mem,
                                   (int) fix_size)) {
                set_error (errno, dir_ptr->file_name, "close_direct");
                return (UNDEF);
            }

            if (dir_ptr->shared == 0) {
                (void) free (dir_ptr->fix_mem);
            }
            (void) close (fd);
        }
        else {
            /* update rel_header information */

            /* Consistency check */
            if (dir_ptr->fix_size != (dir_ptr->rh.max_primary_value + 1) * 
                              dir_ptr->rh._entry_size + sizeof (REL_HEADER)) {
                set_error (SM_INCON_ERR, dir_ptr->file_name, "close_direct");
                return (UNDEF);
            }

            (void) lseek (dir_ptr->fix_fd, 0L, 0);
            if (sizeof (REL_HEADER) != write (dir_ptr->fix_fd, 
                                              (char *) &dir_ptr->rh,
                                              sizeof (REL_HEADER))) {
                set_error (errno, dir_ptr->file_name, "close_direct");
                return (UNDEF);
            }
            if (dir_ptr->shared == 0) {
                (void) free (dir_ptr->fix_mem);
            }
            (void) close (dir_ptr->fix_fd);
        }

        if (dir_ptr->mode & SBACKUP) {
            if (UNDEF == make_backup (dir_ptr->file_name)) {
                return (UNDEF);
            }
        }
    }
    else {  /* SRDONLY is set */
        /* Free up any memory alloced (except if still used by another rel) */
        if (dir_ptr->shared == 0) {
#ifdef MMAP
            if (dir_ptr->mode & SMMAP) {
                if (-1 == munmap (dir_ptr->fix_mem - sizeof (REL_HEADER),
                                  (int) dir_ptr->fix_size) ||
                    -1 == munmap (dir_ptr->var_mem, (int) dir_ptr->var_size))
                    return (UNDEF);
            }
            else {
                (void) free (dir_ptr->fix_mem);
                (void) free (dir_ptr->var_mem);
            }
#else
            (void) free (dir_ptr->fix_mem);
            (void) free (dir_ptr->var_mem);
#endif  /* MMAP */
        }
        if (! (dir_ptr->mode & SINCORE)) {
            (void) close (dir_ptr->fix_fd);
            (void) close (dir_ptr->var_fd);
        }
    }

    dir_ptr->opened--;
    /* Handle shared files.  All descriptors with shared set for a file
       should have the same dir_ptr->shared value (1 less than the number of
       shared descriptors) */
    if (dir_ptr->shared) {
        for (tdir_ptr = &_Sdirect[0]; 
             tdir_ptr < &_Sdirect[MAX_NUM_DIRECT];
             tdir_ptr++) {
            if (dir_ptr->mode == tdir_ptr->mode &&
                0 == strcmp (dir_ptr->file_name, tdir_ptr->file_name)) {
                /* decrement count of relations sharing readonly buffers */
                tdir_ptr->shared--;
            }
        }
    }
    return (0);
}
