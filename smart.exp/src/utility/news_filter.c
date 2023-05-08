#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/utility/news_filter.c,v 11.0 1992/07/21 18:19:33 chrisb Exp $";
#endif

/* #define DEBUG */

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Find valid news articles in a news hierarchy
 *2 news_filter [-d] [-f] [-t time_file] [-g x_groups] 
 *2             [-p x_patterns] newsdir
 *3    -d            Output entire articles instead of names of articles
 *3    -a            Include all files found
 *3    -t time_file  Output articles received after time indicated by time_file
 *3    -g x_groups   Exclude those groups with names in file x_groups
 *3    -p x_patterns Exclude those articles with any matching pattern
 *3       from file x_patterns occurring within the first 1024 bytes.
 *3       Patterns might include "BINHEX", "ROT13", uudecode key words, etc.
 *3    newsdir       root directory to start looking for new news articles

 *7 Find and output full pathnames of all news articles found in the directory
 *7 headed by newsdir that satisfy certain conditions.  Among those 
 *7 conditions might be 
 *7    1. received after a certain time (eg, last time news_filter was run)
 *7    2. article not posted or cross-posted to certain newsgroups
 *7    3. article does not contain key words indicating that it does not
 *7       consist of English text.  Eg, it might be an encoded executable.
 *7    4. Have a file name consisting of all digits
 *7 Articles are only output once, no matter how many newsgroups it occurs in.
 *7 If -t flag included, then will update the time of time_file.  Time_file
 *7 must be owned by the executor of news_filter to update.
 *7 If -d flag included, then will write entire valid doc to stdout, instead
 *7 of just the pathnames.  This is what SMART uses for its news archives.
 *7 If -g flag included, x_groups should be a file with one group name per 
 *7 line.  The entire news hierarchy starting with such a newsgroup will be
 *7 ignored.
 *7 If -p flag included, x_patterns should be a file with one regular
 *7 expression (in the format of 'ed') per line.  If an article contains
 *7 any line matching one of these regular expressions, it will be ignored.
 *7 If -a flag included, then all regular files with a name not beginning
 *7 with a dot will be examined, not just those with an all-numeric file name.

 *8 Completely independent of all SMART libraries.  Does a recursive
 *8 descent of newsdir, first checking file name against x_groups, then
 *8 checking modify time via stat, then reading the first 1024 bytes and
 *8 checking to see if this is the primary newsgroup (first) that this
 *8 article is posted to, and then doing a string search for the excluded
 *8 patterns.

 *9 Pattern matching done with regexp, which works, but is painful to use.
***********************************************************************/

#include <fcntl.h>
#include <dirent.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdio.h>
#include <utime.h>
#include "sysfunc.h"

static int init_time_file(), init_group_file(), init_pattern_file();
static int close_time_file();
static void recurse(), check_and_add_file();
static int check_newsgroup(), check_groups(), check_patterns();
static int comp_group();

static char *newsgroup;
static char *full_path;

#define UNDEF          -1
#define BUF_SIZE       4096
#define PATH_LEN       MAXPATHLEN

#define NEWSGROUP      "Newsgroups: "
#define LEN_NEWSGROUP  12


static time_t begin_time, end_time;
static time_t save_time;

static char *buf;

static int dflg, tflg, gflg, pflg, aflg;

static char *usage = "news_filter [-d] [-f] [-t time_file] [-g x_groups]\n\
        [-p x_patterns] newsdir";
 
int
main(argc, argv)
int argc;
char **argv;
{
    int c;
    extern char *optarg;
    extern int optind;
    char *tfile, *gfile, *pfile;
    char *newsdir;
    DIR *dirp;
    struct dirent *dp;
    struct stat stat_buf;
    char *current_dir;

    /* Get command line arguments */
    dflg = tflg = gflg = pflg = aflg = 0;
    while (-1 != (c = getopt(argc, argv, "dat:g:p:"))) {
        switch (c) {
          case 'd':
            dflg++;
            break;
          case 'a':
            aflg++;
            break;
          case 't':
            tflg++;
            tfile = optarg;
            break;
          case 'g':
            gflg++;
            gfile = optarg;
            break;
          case 'p':
            pflg++;
            pfile = optarg;
            break;
          case '?':
            (void)fprintf(stderr, "usage: %s\n", usage);
            exit (2);
        }
    }
    if (optind >= argc) {
        (void)fprintf(stderr, "usage: %s\n", usage);
        exit (2);
    }
    newsdir = argv[optind];

    if (NULL == (buf = malloc (BUF_SIZE + 1)) ||
        NULL == (current_dir = malloc (PATH_LEN)) ||
        NULL == (newsgroup = malloc (PATH_LEN)) ||
        NULL == (full_path = malloc (PATH_LEN)))
        exit (3);

    if (tflg && UNDEF == init_time_file (tfile)) {
        (void)fprintf(stderr, "usage: %s\n", usage);
        exit (4);
    }
    if (gflg && UNDEF == init_group_file (gfile)) {
        (void)fprintf(stderr, "usage: %s\n", usage);
        exit (5);
    }
    if (pflg && UNDEF == init_pattern_file (pfile)) {
        (void)fprintf(stderr, "usage: %s\n", usage);
        exit (6);
    }


    newsgroup[0] = '\0';

    (void) strncpy (full_path, newsdir, PATH_LEN-1);

    if (NULL == getwd (current_dir)) {
        (void)fprintf(stderr, "usage: %s\n", usage);
        exit (65);
    }
        
    if (NULL == (dirp = opendir (newsdir))) {
        (void)fprintf(stderr, "usage: %s\n", usage);
        exit (7);
    }
    if (-1 == chdir (newsdir)) {
        (void)fprintf(stderr, "usage: %s\n", usage);
        exit (8);
    }

    for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
        if (dp->d_name[0] == '.')
            continue;
        if (-1 == stat (dp->d_name, &stat_buf))
            continue;
        if (S_ISDIR (stat_buf.st_mode)) {
            recurse (dp->d_name);
        }
        else if (S_ISREG (stat_buf.st_mode)) {
            check_and_add_file (dp->d_name, &stat_buf);
        }
    }
    (void) closedir (dirp);

    if (-1 == chdir (current_dir)) {
        (void)fprintf(stderr, "usage: %s\n", usage);
        exit (85);
    }

    if (tflg && UNDEF == close_time_file (tfile)) {
        perror ("news_filter: time_file:");
        (void) fflush (stdout);
        exit (9);
    }

    return (0);
}

static void
recurse (dir_name)
char *dir_name;
{
    DIR *dirp;
    struct dirent *dp;
    struct stat stat_buf;
    int end_full_path, end_newsgroup;

#ifdef DEBUG
    char working_directory [MAXPATHLEN];
    char *getwd();
#endif

    if (NULL == (dirp = opendir (dir_name))) {
#ifdef DEBUG
        (void) getwd (working_directory);
        (void) printf ("Newsgroup rejected open %s %s\n    %s - %s\n", full_path,
                newsgroup, working_directory, dir_name);
#endif
        return;
    }
    if (-1 == chdir (dir_name)) {
#ifdef DEBUG
        (void) printf ("Newsgroup rejected chdir %s - %s\n", newsgroup, dir_name);
#endif
        (void) closedir (dirp);
        return;
    }

    end_full_path = strlen (full_path);
    end_newsgroup = strlen (newsgroup);

    full_path[end_full_path] = '/';
    (void) strcpy (&full_path[end_full_path+1], dir_name);
    if (end_newsgroup == 0)
        (void) strcpy (newsgroup, dir_name);
    else {
        newsgroup[end_newsgroup] = '.';
        (void) strcpy (&newsgroup[end_newsgroup+1], dir_name);
    }

    /* Check that this is a valid newgroup to index */
    if (gflg && check_newsgroup(newsgroup)) {
#ifdef DEBUG
        (void) printf ("Newsgroup rejected %s\n", newsgroup);
#endif
        full_path[end_full_path] = '\0';
        newsgroup[end_newsgroup] = '\0';

        if (-1 == chdir (full_path)) {
            exit (11);
        }
        (void) closedir (dirp);
        return;
    }

#ifdef DEBUG
    (void) printf ("Newsgroup entered %s\n", newsgroup);
#endif
    for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
        if (dp->d_name[0] == '.')
            continue;
        if (-1 == stat (dp->d_name, &stat_buf))
            continue;
        if (S_ISDIR (stat_buf.st_mode)) {
            recurse (dp->d_name);
        }
        else if (S_ISREG (stat_buf.st_mode)) {
            check_and_add_file (dp->d_name, &stat_buf);
        }
    }
    full_path[end_full_path] = '\0';
    newsgroup[end_newsgroup] = '\0';

    if (-1 == chdir (full_path)) {
#ifdef DEBUG
        (void) printf ("Newsgroup rejected chdir fullpath %s\n   %s - %s\n", full_path, newsgroup, dir_name);
#endif
        (void) closedir (dirp);
        return;
    }

    full_path[end_full_path] = '\0';
    newsgroup[end_newsgroup] = '\0';

    if (-1 == chdir (full_path)) {
#ifdef DEBUG
        (void) printf ("Newsgroup rejected chdir fullpath %s\n   %s - %s\n", full_path, newsgroup, dir_name);
#endif
        exit (11);
    }

    (void) closedir (dirp);
}

static void
check_and_add_file (file_name, stat_buf)
char *file_name;
struct stat *stat_buf;
{
    char *ptr;
    int fd;
    int size_wanted, size_read, size_to_write;

    /* Check that file satisfies any time constraints */
    if (tflg) {
        if (stat_buf->st_mtime < begin_time ||
            stat_buf->st_mtime > end_time) {
#ifdef DEBUG
            (void) printf ("Rejected time  %s\t%ld\n", file_name, stat_buf->st_mtime);
#endif
            return;
        }
    }

    /* Check that filename is composed of all digits (except if aflg) */
    if (! aflg) {
        for (ptr = file_name; isdigit (*ptr); ptr++)
            ;
        if (*ptr != '\0') {
#ifdef DEBUG
            (void) printf ("Rejected digit %s\n", file_name);
#endif
            return;
        }
    }

    /* Open file to look at group and pattern requirements */
    if (-1 == (fd = open (file_name, O_RDONLY)))
        return;
    
    size_wanted = stat_buf->st_size;
    if (size_wanted > BUF_SIZE)
        size_wanted = BUF_SIZE;
    if (size_wanted != read (fd, buf, size_wanted)) {
        (void) close (fd);
        return;
    }
    buf[size_wanted] = '\0';

    if (check_groups (buf, size_wanted)) {
#ifdef DEBUG
        (void) printf ("Rejected group %s\n", file_name);
#endif
        (void) close (fd);
        return;
    }
    if (pflg && check_patterns (buf, size_wanted)) {
#ifdef DEBUG
        (void) printf ("Rejected pttrn %s\n", file_name);
#endif
        (void) close (fd);
        return;
    }

    /* If have reached this point, then file satisfies all criteria and
       must be output */
    if (dflg) {
        /* Output entire article */
        (void) lseek (fd, 0L, 0);
        size_to_write = stat_buf->st_size;
        while (size_to_write > 0) {
            if (0 >= (size_read = read (fd, buf, BUF_SIZE))) {
                (void) close (fd);
                return;
            }
            if (-1 == fwrite (buf, 1, size_read, stdout)) {
                (void) fprintf (stderr, "news_filter: Can't write article - Quit");
                exit (10);
            }
            size_to_write -= size_read;
        }
    }
    else {
        /* Just output full path_name of article */
        (void) sprintf (buf, "%s/%s", full_path, file_name);
        puts (buf);
    }

    (void) close (fd);
}

/* Establish begin_time and end_time frame in which time an article must
   have been created. */
static int
init_time_file (file_name)
char *file_name;
{
    struct stat stat_buf;

    if ((time_t) -1 == (save_time = time ((time_t *) NULL)))
        return (UNDEF);
    if (-1 == stat (file_name, &stat_buf))
        return (UNDEF);
    begin_time = stat_buf.st_mtime;
    end_time = save_time;

    return (1);
}

static int
close_time_file (file_name)
char *file_name;
{
    struct utimbuf time_buf;
    time_buf.actime = save_time;
    time_buf.modtime = save_time;

    if (-1 == utime (file_name, &time_buf))
        return (UNDEF);
    return (1);
}

static char *group_buf;
static struct group_list {
    char *name;
    int length;
} *group_list;
static int num_group_list = 0;

static int
init_group_file (file_name)
char *file_name;
{
    int fd;
    int size;
    char *ptr, *start_ptr;

    if (-1 == (fd = open (file_name, O_RDONLY)))
        return (UNDEF);
    if (-1 == (size = lseek (fd, 0L, 2)) ||
        -1 == lseek (fd, 0L, 0))
        return (UNDEF);
    if (NULL == (group_buf = malloc ((unsigned) size+1)) ||
        NULL == (group_list = (struct group_list *)
                 malloc ((unsigned) size * sizeof (struct group_list))))
        return (UNDEF);
    if (size != read (fd, group_buf, size))
        return (UNDEF);
    if (-1 == close (fd))
        return (UNDEF);

    start_ptr = group_buf;
    for (ptr = group_buf; ptr < &group_buf[size]; ptr++) {
        if (*ptr == '\n') {
            *ptr = '\0';
            if (ptr > start_ptr) {
                group_list[num_group_list].name = start_ptr;
                group_list[num_group_list].length = ptr - start_ptr;
                num_group_list++;
                start_ptr = ptr + 1;
            }
        }
    }
    if (ptr > start_ptr) {
        group_list[num_group_list].name = start_ptr;
        group_list[num_group_list].length = ptr - start_ptr;
        num_group_list++;
    }

    qsort ((char *) group_list,
           num_group_list,
           sizeof (struct group_list),
           comp_group);

#ifdef DEBUG
    {
        int i;
        for (i = 0; i < num_group_list; i++) {
            (void) printf ("%s\t%d\n", group_list[i].name, group_list[i].length);
        }
    }
#endif

    return (1);
}

static int comp_group (g1, g2)
struct group_list *g1, *g2;
{
    return (strcmp (g1->name, g2->name));
}

/* Return 1 if newsgroup has a match in group_list */
static int
check_newsgroup (newsgroup)
char *newsgroup;
{
    int i;
    int status = 1;

    for (i = 0;
         i < num_group_list &&
         0 < (status = strncmp (newsgroup,
                                group_list[i].name,
                                group_list[i].length));
         i++)
        ;
    if (i < num_group_list && 0 == status)
        return (1);
    return (0);
}

/* Find the newsgroup line in the header of the article, and check if 
   the first newsgroup on the line is the current newsgroup (return 1
   if not).  If gflg, check if any of the newsgroups on the line
   are found in group_list (return 1 if they are) */
static int 
check_groups (buf, size)
char *buf;
int size;
{
    char *ptr, *start_ptr, *end_ptr;

    /* skip to first occurrence of NEWSGROUP */
    end_ptr = &buf[size - LEN_NEWSGROUP - 1];
    for (ptr = buf; ptr < end_ptr; ptr++) {
        if (*ptr == '\n' &&
            0 == strncmp (ptr + 1, NEWSGROUP, LEN_NEWSGROUP))
            break;
    }
    if (ptr >= end_ptr)
        /* No newsgroup line at all */
        return (0);

    ptr += LEN_NEWSGROUP;

    end_ptr = &buf[size];
    start_ptr = ptr;
    while (*start_ptr == ' ')
        start_ptr++;

    /* Check to make sure first newgroup on line matches newsgroup */
    for (; ptr < end_ptr && *ptr != '\n' && *ptr != ','; ptr++)
        ;
    if (*ptr == ',') {
        *ptr = '\0';
        if (0 != strcmp (newsgroup, start_ptr)) {
#ifdef DEBUG
            (void) printf ("Not first newsgroup: %s - %s\n", newsgroup, start_ptr);
#endif
            return (1);
        }
        *ptr = ',';
    }

    if (! gflg)
        return (0);

    ptr = start_ptr;

    for (; ptr < end_ptr && *ptr != '\n'; ptr++) {
        if (*ptr == ',') {
            *ptr = '\0';
            if (check_newsgroup (start_ptr))
                return (1);
            *ptr = ',';
            ptr++;
            while (ptr < end_ptr && *ptr == ' ')
                ptr++;
            start_ptr = ptr;
            while (*start_ptr == ' ')
                start_ptr++;
        }
    }
    if (ptr < end_ptr && start_ptr < ptr) {
        *ptr = '\0';
        if (check_newsgroup (start_ptr))
            return (1);
        *ptr = '\n';
    }
    return (0);
}


static char *pattern_buf;
static char *pattern_comp;
static int num_patterns;
struct patterns {
    char *comp_pattern;
    int circf;
} *patterns;

#define INIT   register char *sp = instring;
#define GETC() (*sp++)
#define PEEKC()     (*sp)
#define UNGETC(c)   (--sp)
#define RETURN(c)   return(c)
#define ERROR(c)    regerr(c)

static void regerr();
#include <regexp.h>

static int
init_pattern_file (file_name)
char *file_name;
{
    int fd;
    int size;
    char *ptr;
    char *start_pattern;
    char *comp_ptr;
    
    if (-1 == (fd = open (file_name, O_RDONLY)))
        return (UNDEF);
    if (-1 == (size = lseek (fd, 0L, 2)) ||
        -1 == lseek (fd, 0L, 0))
        return (UNDEF);
    if (NULL == (pattern_buf = malloc ((unsigned) size+1)) ||
        NULL == (pattern_comp = malloc (BUF_SIZE)))
        return (UNDEF);
    if (size != read (fd, pattern_buf, size))
        return (UNDEF);
    if (-1 == close (fd))
        return (UNDEF);
    
    num_patterns = 1;
    for (ptr = pattern_buf; ptr < &pattern_buf[size]; ptr++) {
        if (*ptr == '\n')
            num_patterns++;
    }

    if (NULL == (patterns = (struct patterns *)
                 malloc ((unsigned) num_patterns * sizeof (struct patterns))))
        return (UNDEF);

    start_pattern = pattern_buf;
    num_patterns = 0;
    comp_ptr = pattern_comp;
    for (ptr = pattern_buf; ptr < &pattern_buf[size]; ptr++) {
        if (*ptr == '\n') {
            if (start_pattern < ptr) {
                patterns[num_patterns].comp_pattern = comp_ptr;
                comp_ptr = compile (start_pattern,
                                    patterns[num_patterns].comp_pattern,
                                    &pattern_comp[BUF_SIZE],
                                    '\n');
                patterns[num_patterns].circf = circf;
#ifdef PATTERN_DEBUG
                (void) printf ("New pattern %d '%s'\n", num_patterns, start_pattern);
#endif
                num_patterns++;
            }
            start_pattern = ptr + 1;
        }
    }

    return (1);
}

static int
check_patterns (buf, size)
char *buf;
int size;
{
    char *start_line;
    char *ptr;
    int i;

    start_line = buf;
    for (ptr = buf; ptr < &buf[size]; ptr++) {
        if (*ptr == '\n') {
            *ptr = '\0';
            if (start_line < ptr) {
                for (i = 0; i < num_patterns; i++) {
                    circf = patterns[i].circf;
                    if (1 == step (start_line, patterns[i].comp_pattern)) {
#ifdef PATTERN_DEBUG
                        (void) printf ("****Success match %d, '%s'\n    in path %s\n",
                                i, start_line, full_path);
#endif
                        return (1);
                    }
                }
#ifdef PATTERN_DEBUG
                (void) printf ("Failed match '%s'\n", start_line);
#endif
            }
            *ptr = '\n';
            start_line = ptr + 1;
        }
    }

    if (start_line < ptr && 1 == re_exec (start_line))
        return (1);

    return (0);
}

static void
regerr (error_num)
int error_num;
{
    (void) fprintf (stderr,
                    "news_filter: Illegal pattern, error %d\n", error_num);
    exit (11);
}
