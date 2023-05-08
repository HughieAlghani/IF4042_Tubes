#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libfile/open_direct.c,v 11.0 1992/07/21 18:20:55 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include <stdio.h>
#include "common.h"
#include "param.h"
#include "functions.h"
#include "rel_header.h"
#include "direct.h"
#include "io.h"
#include "smart_error.h"
#ifdef MMAP
#   include <sys/types.h>
#   include <sys/mman.h>
#endif   /* MMAP */

extern _SDIRECT _Sdirect[];

/* open the relation "file_name", obeying restrictions of "mode" */
/* Following treatment of files takes place, based on mode "bits" being set */
/*  SRDWR and SINCORE - Both fix file and entire var file brought into core */
/*                      with additional space being reserved for writing */
/*  SRDONLY and SINCORE - Both fix file and entire var file brought into */
/*                      core. No additional space */
/*  SWRONLY and SINCORE - Entire fix file brought into core with extra */
/*                      space. Space reserved for var for writing but file */
/*                      will only be appended, therefore not brought in */
/*  SRDWR and ~SINCORE - Neither file brought into core. Space reserved for */
/*                      one fix entry, space allocated for var as needed. */
/*                      Each write of object results in physical var */
/*                      file being written. */
/*  SRDONLY and ~SINCORE - Neither file brought into core. Space reserved */
/*                      for one fix entry. Buffer space reserved for reading*/
/*                      var file as needed (but see SMMAP below) */
/*  SWRONLY and ~SINCORE - Neither file brought into core. No space reserved*/
/*                      for either file at all. Each write */
/*                      results in physical var file being written */
/*  SRDONLY and SMMAP   - If MMAP is defined (param.h), then the files are */
/*                      directly mapped into memory.  The SINCORE flag is */
/*                      set so the behavior of seek and */
/*                      read are the same as for SRDONLY|SINCORE. */
/* */

static int get_fix_file(), get_var_file();

int
open_direct(file_name, mode)
char *file_name;
long  mode;
{
    register _SDIRECT *dir_ptr;
    _SDIRECT *already_open, *tdir_ptr;
    REL_HEADER temp_rel_entry;
    int dir_index;

    if (file_name == (char *) NULL) {
        set_error (EMFILE, "NULL file name", "open_direct");
        return (UNDEF);
    }

#ifdef MMAP
    if (mode & SMMAP) {
        /* SMMAP only allowed on SRDONLY */
        if ((mode & SRDWR) || (mode & SWRONLY))
            mode &= ~SMMAP;
        else {
            /* Turn on SINCORE bit in addition */
            mode |= SINCORE;
        }
    }
#endif  /* MMAP */

    /* Find first open slot in direct table. */
    /* Minor optimization of allowing two or more RDONLY,INCORE invocations */
    /* of the same object to share the incore image */
    already_open = NULL;
    dir_ptr = NULL;
    for ( tdir_ptr = &_Sdirect[0]; 
          tdir_ptr < &_Sdirect[MAX_NUM_DIRECT];
          tdir_ptr++) {
        if ( mode == tdir_ptr->mode &&
             (mode & SINCORE) &&
             (! (mode & (SWRONLY|SRDWR))) &&
             strncmp (file_name, tdir_ptr->file_name,PATH_LEN) == 0 &&
             tdir_ptr->opened > 0) {
            /* increment count of number relations sharing readonly buffers */
            tdir_ptr->shared++;
            already_open = tdir_ptr;
        }
        else if (tdir_ptr->opened == 0 && dir_ptr == NULL) {
            /* Assign dir-ptr to first open spot */
            dir_ptr = tdir_ptr;
        }
    }

    if (dir_ptr == NULL) {
        set_error (EMFILE, file_name, "open_direct");
        return (UNDEF);
    }
    dir_index = dir_ptr - &_Sdirect[0];
    
    if (already_open != NULL) {
        bcopy ((char *) already_open, (char *) dir_ptr, sizeof (_SDIRECT));
        (void) seek_direct (dir_index, (char *) NULL);
        return (dir_index);
    }

    dir_ptr->mode = mode;
    
    (void) strncpy(dir_ptr->file_name, file_name, PATH_LEN);

    /* If SCREATE, create direct with default attributes */
    if (mode & SCREATE) {
        temp_rel_entry.max_primary_value = UNDEF;
        temp_rel_entry.max_secondary_value = UNDEF;
        if (UNDEF == create_direct (dir_ptr->file_name, &temp_rel_entry)) {
            return (UNDEF);
        }
        dir_ptr->mode = dir_ptr->mode & (~SCREATE);
    }


    /* Bring fix file into memory, or reserve space for an entry */
    if (UNDEF == get_fix_file (dir_index)) {
        return (UNDEF);
    }

    /* var file may be brought into core, depending on mode: */
    if (UNDEF == get_var_file (dir_index)) {
        return (UNDEF);
    }

    /* Seek to first object */
    if (UNDEF == seek_direct (dir_index, (char *) NULL)) {
        return (UNDEF);
    }

    dir_ptr->shared = 0;
    dir_ptr->opened = 1;

    return (dir_index);
}

static int
get_fix_file(dir_index)
int dir_index;
{
    register _SDIRECT *dir_ptr = &(_Sdirect[dir_index]);

    long num_fix_entries;
    long size_fix_entry;
    long size_fix;
    int fd, new_fd;

    /* Open physical fix file for reading */
    if ( -1 == (fd = open (dir_ptr->file_name,
                           (int) ((dir_ptr->mode & SWRONLY) ? SRDWR 
                                                  : dir_ptr->mode & SMASK)))){
        set_error (errno, dir_ptr->file_name, "open_direct");
        return (UNDEF);
    }
    dir_ptr->fix_size = lseek (fd, 0L, 2);
    (void) lseek (fd, 0L, 0);

    /* Get header at beginning of file */
    if (sizeof (REL_HEADER) != read (fd,
                                     (char *) &dir_ptr->rh,
                                     sizeof (REL_HEADER))) {
        set_error (errno, dir_ptr->file_name, "open_direct");
        return (UNDEF);
    }

    num_fix_entries = dir_ptr->rh.max_primary_value + 1;
    size_fix_entry  = dir_ptr->rh._entry_size;
    size_fix = num_fix_entries * size_fix_entry;

    /* Consistency check - Error Flag set but error not returned. With */
    /* concurrent read and write processes, it is possible for the write */
    /* to increase the size of the fixed file before updating the */
    /* rel_header information. A read process will fail the consistency */
    /* check but will actually be fine. */
    if (size_fix != dir_ptr->fix_size - sizeof (REL_HEADER)) {
        set_error (SM_INCON_ERR, dir_ptr->file_name, "open_direct");
        if (dir_ptr->mode & (SWRONLY|SRDWR)) {
            /* Error for sure if inconsistency detected by write process */
            return (UNDEF);
        }
    }


    if (dir_ptr->mode & SINCORE) {
        /* Read entire fa table with no extra space */
#ifdef MMAP
        if (dir_ptr->mode & SMMAP) {
            if ((char *) -1 == (dir_ptr->fix_mem =
                                mmap ((caddr_t) 0,
                                      (size_t) dir_ptr->fix_size,
                                      PROT_READ,
                                      MAP_SHARED,
                                      fd,
                                      (off_t) 0))) {
                set_error (errno, dir_ptr->file_name, "open_direct");
                return (UNDEF);
            }
            dir_ptr->fix_mem += sizeof (REL_HEADER);
        }
        else if (NULL == (dir_ptr->fix_mem = malloc ( (unsigned) size_fix)) ||
                 size_fix != read (fd, dir_ptr->fix_mem, (int) size_fix)) {
            set_error (errno, dir_ptr->file_name, "open_direct");
            return (UNDEF);
        }
#else
        if (NULL == (dir_ptr->fix_mem = malloc ( (unsigned) size_fix)) ||
            size_fix != read (fd, dir_ptr->fix_mem, (int) size_fix)) {
            set_error (errno, dir_ptr->file_name, "open_direct");
            return (UNDEF);
        }
#endif /* MMAP */

        dir_ptr->fix_pos = dir_ptr->fix_mem;
        dir_ptr->fix_end = dir_ptr->fix_mem + size_fix;
        dir_ptr->fix_enddata = dir_ptr->fix_mem + size_fix;
        dir_ptr->fix_foffset = sizeof (REL_HEADER);
        (void) close (fd);
    }
    else {
        /* Reserve space for one fix file entry */
        if (NULL == (dir_ptr->fix_mem =
                             malloc ((unsigned) dir_ptr->rh._entry_size))) {
            set_error (errno, dir_ptr->file_name, "open_direct");
            return (UNDEF);
        }

        /* Must prepare for backup now if writable and SBACKUP. */
        /* Actually, end up writing on new copy of file */
        if ((dir_ptr->mode & (SWRONLY | SRDWR)) && (dir_ptr->mode & SBACKUP)){
            if (UNDEF == (new_fd = prepare_backup (dir_ptr->file_name)) ||
                UNDEF == copy_fd (fd, new_fd)) {
                return (UNDEF);
            }
            (void) close (fd);
            fd = new_fd;
        }

        dir_ptr->fix_pos = NULL;
        dir_ptr->fix_end = dir_ptr->fix_mem + dir_ptr->rh._entry_size;
        dir_ptr->fix_enddata = NULL;
        dir_ptr->fix_foffset = sizeof (REL_HEADER);
        dir_ptr->fix_fd = fd;

    }

    return (0);
}


static int
get_var_file(dir_index)
int dir_index;
{
    register _SDIRECT *dir_ptr = &(_Sdirect[dir_index]);
    char path_name[PATH_LEN];

    int fd_var;
#ifdef VAR_BACKUP
    int new_fd;
#endif

    /* Open var file */
    (void) sprintf (path_name,"%s.var",dir_ptr->file_name);
    if (-1 == (fd_var = open (path_name,
                              (int) dir_ptr->mode & SMASK))) {
        set_error (errno, path_name, "open_direct");
        return (UNDEF);
    }
    dir_ptr->var_size = lseek (fd_var, 0L, 2);
    (void) lseek (fd_var, 0L, 0);

    if (dir_ptr->mode & SINCORE) {
        if (dir_ptr->mode & SWRONLY) {
            /* When only writing directs, no need to read in present var */
            /* file since it will only be appended. The fix file can be */
            /* changed.*/

            /* Allocate space for var tuples */
            if (NULL == (dir_ptr->var_mem = 
                                    malloc (VAR_INCR))) {
                set_error (errno, path_name, "open_direct");
                return (UNDEF);
            }

            dir_ptr->var_enddata = dir_ptr->var_mem;
            dir_ptr->var_foffset = dir_ptr->var_size;
            dir_ptr->var_end = dir_ptr->var_mem + VAR_INCR;
            (void) close (fd_var);
        }
        else if (dir_ptr->mode & SRDWR) {
             /* Read entire var table into memory plus extra memory */
            if (NULL == (dir_ptr->var_mem = malloc ((unsigned) 
                           dir_ptr->var_size + VAR_INCR))) {
                set_error (errno, path_name, "open_direct");
                return (UNDEF);
            }

            /* read the file into memory */
            if (dir_ptr->var_size != read (fd_var, 
                                           dir_ptr->var_mem, 
                                           (int) dir_ptr->var_size)) {
                set_error (errno, path_name, "open_direct");
                return (UNDEF);
            }

            dir_ptr->var_enddata = dir_ptr->var_mem + dir_ptr->var_size;
            dir_ptr->var_foffset = 0;
            dir_ptr->var_end = dir_ptr->var_mem + 
                                    dir_ptr->var_size + VAR_INCR;
            (void) close (fd_var);
        }
        else { /* dir_ptr->mode & SRDONLY */
             /* Read entire var table into memory */
#ifdef MMAP
            if (dir_ptr->mode & SMMAP) {
                if ((char *) -1 == (dir_ptr->var_mem =
                                    mmap ((caddr_t) 0,
                                          (size_t) dir_ptr->var_size,
                                          PROT_READ,
                                          MAP_SHARED,
                                          fd_var,
                                          (off_t) 0))) {
                    set_error (errno, dir_ptr->file_name, "open_direct");
                    return (UNDEF);
                }
            }
            else if (NULL == (dir_ptr->var_mem = 
                              malloc ((unsigned) dir_ptr->var_size)) ||
                     dir_ptr->var_size != read (fd_var, 
                                                dir_ptr->var_mem, 
                                                (int) dir_ptr->var_size)) {
                set_error (errno, path_name, "open_direct");
                return (UNDEF);
            }
#else
            if (NULL == (dir_ptr->var_mem = 
                         malloc ((unsigned) dir_ptr->var_size)) ||
                dir_ptr->var_size != read (fd_var, 
                                           dir_ptr->var_mem, 
                                           (int) dir_ptr->var_size)) {
                set_error (errno, path_name, "open_direct");
                return (UNDEF);
            }
#endif /* MMAP */

            dir_ptr->var_enddata = dir_ptr->var_mem + dir_ptr->var_size;
            dir_ptr->var_foffset = 0;
            dir_ptr->var_end = dir_ptr->var_enddata;
            (void) close (fd_var);
        }
        
    }
    else { /* ! dir_ptr->mode & SINCORE */
        /* Reserve reading buffer of size VAR_BUFF */
        if (NULL == (dir_ptr->var_mem = 
                      malloc ((unsigned) VAR_BUFF))) {
            set_error (errno, path_name, "open_direct");
            return (UNDEF);
        }
        dir_ptr->var_enddata = dir_ptr->var_mem;
        dir_ptr->var_foffset = 0;
        dir_ptr->var_end = dir_ptr->var_mem + VAR_BUFF;

#ifdef VAR_BACKUP
        /* Must prepare for backup now if writable and SBACKUP. */
        /* Actually, end up writing on new copy of file */
        if ((dir_ptr->mode & (SWRONLY | SRDWR)) && (dir_ptr->mode & SBACKUP)){
            if (UNDEF == (new_fd = prepare_backup (path_name)) ||
                UNDEF == copy_fd (fd_var, new_fd)) {
                return (UNDEF);
            }
            (void) close (fd_var);
            fd_var = new_fd;
        }
#endif

        /* In all cases (if not SINCORE) keep fd_var around for file access */
        dir_ptr->var_fd = fd_var;
    }

    return (0);
}
