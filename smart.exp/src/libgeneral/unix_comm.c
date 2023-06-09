#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libgeneral/unix_comm.c,v 11.0 1992/07/21 18:21:10 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Output buf through a pager
 *2 unix_page (buf, size)
 *3   char *buf;
 *3   int size;
 *5 Environment variable "PAGER" checked for program to be used.
 *6 Macro DEF_PAGE is used as default pager (see param.h)
 *7 Output buf to stdout, using a paging filter.  If buf is sufficiently
 *7 short, the filter is ignored and stdout is written directly.
 *8 Forks off the process indicated by PAGER (if defined) or DEF_PAGE.
 *8 and writes buf to it.
 *9 Probably system dependant to a small extent.
***********************************************************************/

#include <ctype.h>
#include <errno.h>
#ifdef sun  /* and undoubtedly others, but not sure which */
#include <sys/wait.h>
#define STATUSTYPE union wait
#else
#define STATUSTYPE long
#endif
#include <signal.h>
#include <sys/stat.h>
#include "common.h"
#include "param.h"
#include "functions.h"

/* defines should be in param.h */
#define DEFAULT_LINE_LENGTH 80
#define DEFAULT_NUM_LINES 24

int
unix_page (buf, size)
char *buf;
int size;
{
    int outfd;
    char buffer[PATH_LEN];
    char *page_env;
    int written_so_far, size_written;
    int num_newlines;
    char *ptr;

    /* If buf is short enough, just output to stdout */
    num_newlines = 1;
    if (size <= (DEFAULT_LINE_LENGTH * (DEFAULT_NUM_LINES - 4))) {
        for (ptr = buf; ptr < &buf[size]; ptr++) {
            if (*ptr == '\n') {
                if (++num_newlines > DEFAULT_NUM_LINES)
                    break;
            }
        }
        if (ptr >= &buf[size]) {
            if (0 == fwrite (buf, size, 1, stdout))
                return (UNDEF);
            (void) fflush (stdout);
            return (1);
        }
    }


    /* Ignore signal if can't write to PAGER because it was killed */
    /* (using 'q' or some similar command). */
    (void) signal (SIGPIPE, SIG_IGN);

    if (NULL == (page_env = getenv ("PAGER")))
        (void) strcpy (buffer, DEF_PAGE);
    else
        (void) strncpy (buffer, page_env, PATH_LEN);

    if (UNDEF == (outfd = unix_pipe_open(buffer)))
        outfd = fileno (stdout);

    written_so_far = 0; size_written = 0;
    while (written_so_far < size) {
        if (-1 == (size_written = write (outfd,
                                         &buf[size_written],
                                         size - written_so_far))) {
            if (errno == EPIPE) {
                errno = 0;
                break;
            }
            return (UNDEF);
        }
        written_so_far += size_written;
    }

    if (outfd != fileno (stdout))
        (void) unix_pipe_close(outfd);
    else
        (void) fflush(stdout);
    
    return (1);
}

/* Not convinced this works in general.  As far as I know, not used. */
int
unix_command(s)
char *s;
{
    STATUSTYPE status;
    int  pid, chpid;
    char *shell;

    if (NULL == (shell = getenv("SHELL")))
        shell = "/bin/sh";
    if ((pid = vfork()) == 0) {
        /* Restore real gid before invoking shell */
        /* (void) setregid (-1, getgid()); */
        if (*s)
            (void) execl(shell, shell, "-c", s, 0);
        else
            (void) execl(shell, shell, 0);
        exit (17);
    }
    while ((chpid = wait(&status)) != pid && chpid != -1)
        ;
    if (chpid == -1)

        return (UNDEF);
    return (0);
}

/* Run command but with any occurrence of $in replaced by infile,
   and any occurrence of $out replaced by outfile. If $in does not appear
   and infile is non-NULL, add "< infile" to the end of the command.
   if $out does not appear and outfile is non-NULL, add "> outfile" to
   the end of the command. */
int
unix_inout_command (init_command, infile, outfile)
char *init_command;
char *infile;
char *outfile;
{
    char command[PATH_LEN * 3];
    int incount = 0;
    int outcount = 0;
    char *inptr, *outptr, *tempptr;

    inptr = init_command;
    outptr = command;
    while (*inptr) {
        if (*inptr == '$') {
            if (0 == strncmp (inptr, "$in", 3)) {
                for (tempptr = infile; *tempptr; tempptr++)
                    *outptr++ = *tempptr;
                inptr += 3;
                incount++;
            }
            else if (0 == strncmp (inptr, "$out", 4)) {
                for (tempptr = outfile; *tempptr; tempptr++)
                    *outptr++ = *tempptr;
                inptr += 4;
                outcount++;
            }
            else
                *outptr++ = *inptr++;
        }
        else 
            *outptr++ = *inptr++;
    }

    if (incount == 0 && infile != NULL) {
        *outptr++ = ' ';
        *outptr++ = '<';
        *outptr++ = ' ';
        for (tempptr = infile; *tempptr; tempptr++)
            *outptr++ = *tempptr;
    }
    if (outcount == 0 && outfile != NULL) {
        *outptr++ = ' ';
        *outptr++ = '>';
        *outptr++ = ' ';
        for (tempptr = outfile; *tempptr; tempptr++)
            *outptr++ = *tempptr;
    }

    *outptr = '\0';

    return (unix_command (command));
}


/********************   PROCEDURE DESCRIPTION   ************************
 *0 Call an editor to edit a file
 *2 unix_edit_file (filename)
 *3   char *filename;
 *5 Environment variable "EDITOR" checked for program to be used.
 *6 Macro DEF_EDITOR is used as default editor (see param.h)
 *8 Forks off the process indicated by EDITOR (if defined) or DEF_EDITOR.
 *8 invokes it with argument filename by using UNIX command "system"
***********************************************************************/


int
unix_edit_file (filename)
char *filename;
{
    char *editor;
    char command_buf[PATH_LEN];

    if (filename == (char *) NULL)
        return (UNDEF);

    if (NULL == (editor = getenv ("EDITOR")))
        editor = DEF_EDITOR;

    if (strlen (editor) + strlen (filename) > PATH_LEN)
        return (UNDEF);

    (void) sprintf (command_buf, "%s %s", editor, filename);

    return (system (command_buf));
}


static int pipe_pid[20];

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Open a pipe for running a command under a shell, and executes command.
 *2 unix_pipe_open (command)
 *3   char *command;
 *4 unix_pipe_close (fd)
 *5 Environment variable "SHELL" checked for program to be used.
 *6 /bin/sh is used as default shell.
 *7 Forks off the process indicated by SHELL (or /bin/sh).  Return
 *7 int file descriptor valid for writing to process.
 *7 Shell invoked with argument command by using UNIX command "execl"
 *7 unix_pipe_close closes the file descriptor and waits.
 *9 Undoubtedly somewhat system dependant.
***********************************************************************/

/* Open a pipe for writing to a process running "command" */
/* Return -1 (UNDEF) if not successful */
int
unix_pipe_open (command)
char *command;
{
    int p[2];
    int pid;
    char *shell;

    if (-1 == pipe(p)) {
        return (UNDEF);
    }

    if (NULL == (shell = getenv("SHELL")))
	shell = "/bin/sh";
    if (0 == (pid = vfork())) {
        /* Child : Close access to pipe for writing and dup read descriptor */
        (void) close (p[1]);
        if (p[0] != 0) {
            (void) dup2 (p[0], 0);
            (void) close (p[0]);
        }
        (void) execl (shell, shell, "-c", command, 0);
        exit (17);
    }
    if (pid == -1) {
        (void) close (p[0]);
        (void) close (p[1]);
        return (UNDEF);
    }
    (void) close (p[0]);
    pipe_pid [p[0]] = pid;
    return (p[1]);
}

int
unix_pipe_close (fd)
register int fd;
{
    register int ch_pid;
    void (*istat)();
    STATUSTYPE status;
    
    (void) close (fd);
    istat = signal (SIGINT, SIG_IGN);
    while ((ch_pid = wait (&status)) != pipe_pid[fd] && ch_pid != -1)
        ;
    (void) signal (SIGINT, istat);
    if (ch_pid == -1)
        return (UNDEF);
    return (0);
}

int
unix_file_exists (file_name)
char *file_name;
{
    struct stat buf;

    if (NULL == file_name || file_name[0] == '\0')
        return (0);
    if (-1 == stat (file_name, &buf))
        return (0);
    return (1);
}
