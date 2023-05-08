#ifndef TEXTLOCH
#define TEXTLOCH
/*        $Header: /home/smart/release/src/h/textloc.h,v 11.0 1992/07/21 18:18:56 chrisb Exp $*/

typedef struct {
    long  id_num;                   /* Id of text */
    long begin_text;                /* Offset within file_name of text start */
    long end_text;                  /* Offset within file_name of text end */
    long doc_type;                  /* Collection dependent type of doc */
    char  *file_name;               /* File to find text in */
    char  *title;                   /* Title of text */
} TEXTLOC;

#endif /* TEXTLOCH */
