#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libinter/save_file.c,v 11.0 1992/07/21 18:21:47 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include <ctype.h>
#include "common.h"
#include "param.h"
#include "functions.h"
#include "io.h"
#include "smart_error.h"
#include "spec.h"
#include "proc.h"
#include "tr_vec.h"
#include "context.h"
#include "retrieve.h"
#include "docindex.h"
#include "inter.h"


static char *save_file = NULL;

int
inter_save_file (is, unused)
INTER_STATE *is;
char *unused;
{
    int outfd;
    char temp_buf[PATH_LEN + 25];

    if (save_file == NULL) {
        if (NULL == (save_file = malloc (PATH_LEN)))
            return (UNDEF);
        save_file[0] = '\0';
    }

    if (is->num_command_line >= 2)
        (void) strcpy (save_file, is->command_line[1]);

    if (save_file[0] == '\0') {
        if (UNDEF == add_buf_string ("No save file specified\n", &is->err_buf))
            return (UNDEF);
        return (0);
    }

    /* Open new output file for appending and copy last output to it */
    if (-1 == (outfd = open (save_file,
                             SWRONLY | SCREATE | SAPPEND, 0644)) ||
        is->output_buf.end != write (outfd,
                                     is->output_buf.buf,
                                     is->output_buf.end) ||
        -1 == close (outfd)) {
        (void) sprintf (temp_buf,
                        "Cannot write file %s\n",
                        save_file);
        if (UNDEF == add_buf_string (temp_buf, &is->err_buf))
            return (UNDEF);
        return (0);
    }
    
    (void) sprintf (temp_buf, "%ld bytes written to file %s\n",
                    is->output_buf.end, save_file);
    if (UNDEF == add_buf_string (temp_buf, &is->err_buf))
        return (UNDEF);

    return (1);
}
