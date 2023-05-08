#ifndef DISPLAYH
#define DISPLAYH
/*        $Header: /home/smart/release/src/h/display.h,v 11.0 1992/07/21 18:18:35 chrisb Exp $*/

#define TITLE_LEN 69

typedef struct  {
    long  id_num;                   /* Id of doc */
    short num_sections;             /* Number of sections in this did */
    long  *begin;                   /* beginning of sections 0..numsections-1*/
                                    /* (bytes from start of file - section 0 */
                                    /* is the start of the document */ 
    long  *end;                     /* end of sections 0..numsections -1 */
    char  *file_name;               /* File to find text of doc in */
    char  *title;                   /* Title of doc (up to TITLE_LEN chars */
} DISPLAY;
#endif /* DISPLAYH */
