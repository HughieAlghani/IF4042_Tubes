#ifndef DIRECTH
#define DIRECTH
/*  $Header: /home/smart/release/src/h/direct.h,v 11.0 1992/07/21 18:18:34 chrisb Exp $ */
#ifndef MAXNUMFIELDS
#    include "direct_type.h"
#endif

#define MAX_DIRECT_SIZE 256

#define MAX_NUM_DIRECT  60

#define FIX_INCR 1000
#define VAR_INCR 16000
#define VAR_BUFF 64000

typedef struct {
    char file_name[PATH_LEN];/* base name of vector collection */

    char *fix_mem;          /* start of buffer in memory for fixed */
                            /* length data */    
    char *fix_pos;          /* current buffer pos in memory for fix */
    char *fix_enddata;      /* buffer pos in memory of end of valid fix data*/
    char *fix_end;          /* end of fix buffer in memory */
    long  fix_size;         /* length in bytes of fix file */
                            /* INCLUDING REL_HEADER */
    long  fix_foffset;      /* file offset (bytes) of start of fix buffer */
    int   fix_fd;           /* file_descriptor for fix file */

    char *var_mem;          /* start of buffer in memory for var data */
    char *var_enddata;      /* end of valid data in buffer for var */
    char *var_end;          /* end of var buffer in memory */
    long  var_foffset;      /* file offset (bytes) of start of var buffer */
    long  var_size;         /* length in bytes of physical var file */
    int   var_fd;           /* file descriptor for var file */

    REL_HEADER rh;          /* Information about this relation in  */
                            /* rel_header.h */
    char opened;            /* 1 if this _SGR_FILE in current use, else 0 */
    char shared;            /* nonzero if buffers are shared */
    long  mode;             /* mode relation opened with. See "io.h" */
}  _SDIRECT;

/* Usage of _SDIRECT fields */
    /* If mode&SINCORE, then both the fix and var files are entirely in-core*/
    /* Otherwise, only the current entry of each is kept in-core */
    /* The file offsets contained in the fixed entries are kept */
    /* contiguous for that node_num: only one disk access in needed to fetch*/
    /* them. */

    /* If not SINCORE, foffset always gives offset of the valid information */
    /* contained in the relevant buffer. The current location within the */
    /* relational object is given by the position within the fix relation. */
    /* This is either fix_foffset (if fix_pos == fix_mem) */
    /* or fix_foffset + sizeof (fixed_entry), (if fix_pos != fix_mem). */
    /* The var buffer will contain the variable records for the last tuple */
    /* read, even if this is not the current tuple. */
#endif /* DIRECTH */
