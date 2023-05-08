#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/user_contrib/smartx/libproc/proc_top.c,v 11.0 1992/07/21 18:19:17 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/************************************************************************
*                                                                       *
*       For the X interface, two lines with x_* was added.		*
*                                                                       *
*    by Tong Li, graduate student in CS from Oregon State University,   *
*                e-mail: lit@mist.cs.orst.edu                           *
*                                                                       *
*       Victuria Chiu, senior in CS from Univ. of Cal. at Berkeley,     *
*                      e-mail: victoria@csr.lbl.gov                     *
*                                                                       *
*    at Lawrence berkeley Laboratory, summer 1991                       *
*                                                                       *
************************************************************************/

#include <ctype.h>
#include "common.h"
#include "param.h"
#include "functions.h"
#include "proc.h"

extern int init_inter(), inter(), close_inter();
extern int init_inter_sent(), inter_sent(), close_inter_sent();
extern int init_inter_match(), inter_match(), close_inter_match();
extern int init_x_inter_match(), x_inter_match(), close_x_inter_match();
extern int init_retrieve(), retrieve(), close_retrieve();
extern int init_ret_fdbk(), ret_fdbk(), close_ret_fdbk();
PROC_TAB proc_top[] = {
    "inter",        init_inter,     inter,         close_inter,
    "inter_sent",   init_inter_sent,inter_sent,    close_inter_sent,
    "inter_match",  init_inter_match,inter_match,  close_inter_match,
    "x_inter_match",init_x_inter_match,x_inter_match,close_x_inter_match,
    "retrieve",     init_retrieve,  retrieve,      close_retrieve,
    "ret_fdbk",     init_ret_fdbk,  ret_fdbk,      close_ret_fdbk,
};
int num_proc_top =
    sizeof (proc_top) / sizeof (proc_top[0]);



