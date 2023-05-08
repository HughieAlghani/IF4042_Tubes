/************************************************************************
*                                                                       *
*       Adding an X-Window interface to SMART --- xfunctions.h          *
*                                                                       *
*                                                                       *
*    by: Victuria Chiu, senior in CS from Univ. of Cal. at Berkeley,    *
*                e-mail: victoria@csr.lbl.gov                           *
*                                                                       *
*        Tong Li, graduate student in CS from Oregon State University,  *
*                e-mail: lit@mist.cs.orst.edu                           *
*                                                                       *
*    at Lawrence berkeley Laboratory, Summer 1991                       *
*                                                                       *
************************************************************************/

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/textsw.h>
#include <xview/xv_xrect.h>
#include </usr/local/guide/include/gdd.h>
#include "smart_xmain_ui.h"
#include "smart_xsec_ui.h"
#include "proc.h"
#include "inter.h"

/*global variables linking the three inteface together*/

extern smart_xmain_basewindow_objects	        *smart_xmain_basewindow;
extern smart_xsec_document_popup_objects       *smart_xsec_document_popup;
extern smart_xsec_help_popup_objects           *smart_xsec_help_popup;
extern smart_xsec_savelast_popup_objects       *smart_xsec_savelast_popup;
extern smart_xsec_advance_popup_objects        *smart_xsec_advance_popup;

/*smart_xmain.c call back and event functions.*/

extern Menu_item x_quit();
extern Menu_item x_xit();
extern Menu_item savelast_popup();
extern Menu_item saveall_popup();
extern Menu_item x_raw_doc();
extern Menu_item x_top_ranked();
extern Menu_item x_location();
extern Menu_item x_spec_file();
extern Menu_item x_dtext();
extern Menu_item x_dvec();
extern Menu_item x_drun();
extern Menu_item x_dsim();
extern Menu_item x_dmatch();
extern Menu_item x_qtext();
extern Menu_item x_qvec();
extern Menu_item x_qsim();
extern Menu_item x_qmatch();

extern void x_retrieve();
extern void x_more_documents();
extern void x_keywords();
extern void x_feedback();
extern void x_next_document();
extern void clear_usertext();
extern void x_help();
extern Notify_value x_hilite_button();
extern int show_document();
extern void do_next_document();
extern void delete_text();

/*smart_xsec.c call back and event functinos.*/

extern void x_docnext_document();
extern void close_doc_popup();
extern void close_help_popup();
extern void x_savelast();
extern void close_adv_popup();
extern void get_text();

/*functions for connecting x-window to smart system*/

typedef struct {
    PROC_TAB proc;           /* Procedure to do action. All procedures
                                return 1 upon success, UNDEF upon
                                system failure, 0 upon minor failure */
    int sign_len;            /* Number of chars significant in command.name */
    int save_output;         /* If false, do not keep output of command
                                (cannot "save" it to a file) */
    int end_inter;           /* If true, exit inter after this command */
    char *explanation;       /* Description of action taken by name */
} COMMAND;

extern COMMAND * parse_command_line();
char * get_doc_num();
extern int row;
extern INTER_STATE is_x; 



