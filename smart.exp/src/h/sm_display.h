#ifndef SM_DISPLAYH
#define SM_DISPLAYH
/*        $Header: /home/smart/release/src/h/sm_display.h,v 11.0 1992/07/21 18:18:53 chrisb Exp $*/

/* Internal format of a preparsed document */

typedef struct {
    char section_id;
    long begin_section;
    long end_section;
} SM_DISP_SEC;

typedef struct  {
    long  id_num;                   /* Id of doc */
    long  num_sections;             /* Number of sections in this did */
    SM_DISP_SEC *sections;          /* id/start/end of each section. */
    char  *file_name;               /* File to find text of doc in */
    char  *title;                   /* Title of doc */
} SM_DISPLAY;

#endif /* SM_DISPLAYH */
