#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libfile/direct.c,v 11.0 1992/07/21 18:20:51 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include "common.h"
#include "param.h"
#include "rel_header.h"
#include "direct_type.h"
#include "direct.h"
#include "graph.h"
#include "display.h"
#include "vector.h"
#include "pnorm_vector.h"
#include "part_vector.h"
#include "inv.h"
#include "simp_inv.h"
#include "hier.h"
#include "array.h"
#include "io.h"
#include "addinfo.h"
#include "oldinv.h"
#include "textloc.h"
#include "tr_vec.h"
#include "rr_vec.h"

_SDIRECT _Sdirect[MAX_NUM_DIRECT];

/* Note order of definitions of types must agree with that in rel_header.h */
DIRECT_TYPES direct_object_type[] = {
"graph" , 4, 4, 0, 0,
    "node_num",       ATT_INT,                              sizeof (long), 
                      (char *) &(((GRAPH *) NULL)->node_num)
                          - (char *) NULL,    
                      UNDEF,
    "info",           ATT_INT,                              sizeof (long), 
                      (char *) &(((GRAPH *) NULL)->info)
                          - (char *) NULL,        
                      UNDEF,
    "num_parents",    ATT_INT,                              sizeof (short), 
                      (char *) &(((GRAPH *) NULL)->num_parents)
                          - (char *) NULL, 
                      UNDEF,
    "num_children",   ATT_INT,                              sizeof (short), 
                      (char *) &(((GRAPH *) NULL)->num_children)
                          - (char *) NULL,
                      UNDEF,
    "parents",        ATT_INT,                              sizeof (long),
                      (char *) &(((GRAPH *) NULL)->parents)
                          - (char *) NULL,     
                      2,
    "children",       ATT_INT,                              sizeof (long),
                      (char *) &(((GRAPH *) NULL)->children)
                          - (char *) NULL,    
                      3,
    "parent_weight",  ATT_FLT,                              sizeof (float),
                      (char *) &(((GRAPH *) NULL)->parent_weight)
                          - (char *) NULL, 
                      2,
    "children_weight",ATT_FLT,                              sizeof (float),
                      (char *) &(((GRAPH *) NULL)->children_weight)
                          - (char *) NULL,
                      3,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,

"oldinv" , 2, 2, 0, 0,
    "key_num",        ATT_INT,                              sizeof (long), 
                      (char *) &(((OLDINV *) NULL)->key_num)
                          - (char *) NULL,        
                      UNDEF,
    "num_list",       ATT_INT,                              sizeof (short),
                      (char *) &(((OLDINV *) NULL)->num_list)
                          - (char *) NULL,       
                      UNDEF,
    "list",           ATT_INT,                              sizeof (long),
                      (char *) &(((OLDINV *) NULL)->list)
                          - (char *) NULL,           
                      1,
    "list_weights",   ATT_FLT,                              sizeof (float),
                      (char *) &(((OLDINV *) NULL)->list_weights)
                          - (char *) NULL,   
                      1,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,

"vector" , 3, 2, 0, 0,
    "id_num",         ATT_INT,                              sizeof (long),
                      (char *) &(((VEC *) NULL)->id_num)
                          - (char *) NULL,        
                      UNDEF,
    "num_ctype",      ATT_INT,                              sizeof (long),
                      (char *) &(((VEC *) NULL)->num_ctype)
                          - (char *) NULL,     
                      UNDEF,
    "num_conwt",      ATT_INT,                              sizeof (long), 
                      (char *) &(((VEC *) NULL)->num_conwt)
                          - (char *) NULL,     
                      UNDEF,
    "ctype_len",      ATT_INT,                              sizeof (long), 
                      (char *) &(((VEC *) NULL)->ctype_len)
                          - (char *) NULL,     
                      1,
    "con_wtp",        ATT_INT,                              sizeof (CON_WT), 
                      (char *) &(((VEC *) NULL)->con_wtp)
                          - (char *) NULL,       
                      2,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,

"display" , 2, 4, 0, 0,
    "id_num",         ATT_INT,                              sizeof (long),
                      (char *) &(((DISPLAY *) NULL)->id_num)
                          - (char *) NULL,    
                      UNDEF,
    "num_sections",   ATT_INT,                              sizeof (short),
                      (char *) &(((DISPLAY *) NULL)->num_sections)
                          - (char *) NULL,
                      UNDEF,
    "begin",          ATT_INT,                              sizeof (long),
                      (char *) &(((DISPLAY *) NULL)->begin)
                          - (char *) NULL,     
                      1,
    "end",            ATT_INT,                              sizeof (long), 
                      (char *) &(((DISPLAY *) NULL)->end)
                          - (char *) NULL,       
                      1,
    "file_name",      ATT_STR,                              sizeof (char), 
                      (char *) &(((DISPLAY *) NULL)->file_name)
                          - (char *) NULL, 
                      UNDEF,
    "title",          ATT_STR,                              sizeof (char), 
                      (char *) &(((DISPLAY *) NULL)->title)
                          - (char *) NULL,     
                      UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,

"sinv" , 2, 1, 0, 0,
    "id_num",         ATT_INT,                              sizeof (long), 
                      (char *) &(((SIMP_INV *) NULL)->id_num)
                          - (char *) NULL,  
                      UNDEF,
    "num_list",       ATT_INT,                              sizeof (long),
                      (char *) &(((SIMP_INV *) NULL)->num_list)
                          - (char *) NULL,  
                      UNDEF,
    "list",           ATT_INT,                              sizeof (long),
                      (char *) &(((SIMP_INV *) NULL)->list)
                          - (char *) NULL,      
                      1,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,

"hierarchy", 6, 1, 0, 0,
    "node_num",       ATT_INT,                              sizeof (long),
                      (char *) &(((HIER *) NULL)->node_num)
                          - (char *) NULL,      
                      UNDEF,
    "item_id",        ATT_INT,                              sizeof (long),
                      (char *) &(((HIER *) NULL)->item_id)
                          - (char *) NULL,       
                      UNDEF,
    "level",          ATT_FLT,                              sizeof (float),
                      (char *) &(((HIER *) NULL)->level)
                          - (char *) NULL,         
                      UNDEF,
    "parent",         ATT_INT,                              sizeof (long),
                      (char *) &(((HIER *) NULL)->parent)
                          - (char *) NULL,        
                      UNDEF,
    "num_desc_leaves",ATT_INT,                              sizeof (long),
                      (char *) &(((HIER *) NULL)->num_desc_leaves)
                          - (char *) NULL, 
                      UNDEF,
    "num_children",   ATT_INT,                              sizeof (short),
                      (char *) &(((HIER *) NULL)->num_children)
                          - (char *) NULL,  
                      UNDEF,
    "children",       ATT_INT,                              sizeof (long),
                      (char *) &(((HIER *) NULL)->children)
                          - (char *) NULL,      
                      5,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,

"array", 2, 0, 0, 0,
    "index",          ATT_INT,                              sizeof (long),
                      (char *) &(((ARRAY *) NULL)->index)
                          - (char *) NULL,        
                      UNDEF,
    "info",           ATT_INT,                              sizeof (long),
                      (char *) &(((ARRAY *) NULL)->info)
                          - (char *) NULL,         
                      UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,

"pnorm_vec", 5, 4, 0, 0,
    "id_num",         ATT_INT,                              sizeof(long),
                      (char *) &(((PNORM_VEC *) NULL)->id_num)
                          - (char *) NULL,  
                      UNDEF,
    "num_ctype",      ATT_INT,                              sizeof (short),
                      (char *) &(((PNORM_VEC *)NULL)->num_ctype)
                          - (char *) NULL,
                      UNDEF,
    "num_conwt",      ATT_INT,                              sizeof (short), 
                      (char *) &(((PNORM_VEC *)NULL)->num_conwt)
                          - (char *) NULL,
                      UNDEF,
    "num_nodes",      ATT_INT,                              sizeof (short),
                      (char *) &(((PNORM_VEC *)NULL)->num_nodes)
                          - (char *) NULL,
                      UNDEF,
    "num_op_p_wt",    ATT_INT,                              sizeof (short),
                      (char *) &(((PNORM_VEC *)NULL)->num_op_p_wt)
                          - (char *) NULL,
                      UNDEF,
    "ctype_len",      ATT_INT,                              sizeof (short), 
                      (char *) &(((PNORM_VEC *)NULL)->ctype_len)
                          - (char *) NULL,
                      1,
    "con_wtp",        ATT_INT,                              sizeof (CON_WT), 
                      (char *) &(((PNORM_VEC *) NULL)->con_wtp)
                          - (char *) NULL, 
                      2,
    "tree",           ATT_INT,                              sizeof(TREE_NODE),
                      (char *) &(((PNORM_VEC *) NULL)->tree)
                          - (char *) NULL,    
                      3,
    "op_p_wtp",       ATT_INT,                              sizeof (OP_P_WT),
                      (char *) &(((PNORM_VEC *) NULL)->op_p_wtp)
                          - (char *) NULL,
                      4,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,

"addinv" , 2, 3, 0, 0,
    "key_num",        ATT_INT,                              sizeof (long), 
                      (char *) &(((ADDINV *) NULL)->key_num)
                          - (char *) NULL,     
                      UNDEF,
    "num_list",       ATT_INT,                              sizeof (long),
                      (char *) &(((ADDINV *) NULL)->num_list)
                          - (char *) NULL,    
                      UNDEF,
    "list",           ATT_INT,                              sizeof (long),
                      (char *) &(((ADDINV *) NULL)->list)
                          - (char *) NULL,        
                      1,
    "list_weights",   ATT_FLT,                              sizeof (float),
                      (char *) &(((ADDINV *) NULL)->list_weights)
                          - (char *) NULL,
                      1,
    "addinfo",        ATT_INT,                              sizeof (ADDINFO),
                      (char *) &(((ADDINV *) NULL)->addinfo)
                          - (char *) NULL,     
                      1,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,

"addvector" , 3, 3, 0, 0,
    "id_num",         ATT_INT,                              sizeof (long),
                      (char *) &(((ADDVEC *) NULL)->id_num)
                          - (char *) NULL,     
                      UNDEF,
    "num_ctype",      ATT_INT,                              sizeof (long),
                      (char *) &(((ADDVEC *) NULL)->num_ctype)
                          - (char *) NULL,  
                      UNDEF,
    "num_conwt",      ATT_INT,                              sizeof (long), 
                      (char *) &(((ADDVEC *) NULL)->num_conwt)
                          - (char *) NULL,  
                      UNDEF,
    "ctype_len",      ATT_INT,                              sizeof (long), 
                      (char *) &(((ADDVEC *) NULL)->ctype_len)
                          - (char *) NULL,  
                      1,
    "con_wtp",        ATT_INT,                              sizeof (CON_WT), 
                      (char *) &(((ADDVEC *) NULL)->con_wtp)
                          - (char *) NULL,    
                      2,
    "addinfo",        ATT_INT,                              sizeof (ADDINFO),
                      (char *) &(((ADDVEC *) NULL)->addinfo)
                          - (char *) NULL,    
                      2,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,

"inv" , 2, 1, 0, 0,
    "id_num",         ATT_INT,                              sizeof (long), 
                      (char *) &(((INV *) NULL)->id_num)
                          - (char *) NULL,        
                      UNDEF,
    "num_listwt",     ATT_INT,                              sizeof (long),
                      (char *) &(((INV *) NULL)->num_listwt)
                          - (char *) NULL,       
                      UNDEF,
    "listwt",         ATT_INT,                              sizeof (LISTWT),
                      (char *) &(((INV *) NULL)->listwt)
                          - (char *) NULL,           
                      1,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,

"textloc" , 4, 2, 0, 0,
    "id_num",         ATT_INT,                              sizeof (long),
                      (char *) &(((TEXTLOC *) NULL)->id_num)
                          - (char *) NULL,    
                      UNDEF,
    "begin_text",     ATT_INT,                              sizeof (long),
                      (char *) &(((TEXTLOC *) NULL)->begin_text)
                          - (char *) NULL,
                      UNDEF,
    "end_text",       ATT_INT,                              sizeof (long),
                      (char *) &(((TEXTLOC *) NULL)->end_text)
                          - (char *) NULL,     
                      1,
    "doc_type",       ATT_INT,                              sizeof (long), 
                      (char *) &(((TEXTLOC *) NULL)->doc_type)
                          - (char *) NULL,       
                      1,
    "file_name",      ATT_STR,                              sizeof (char), 
                      (char *) &(((TEXTLOC *) NULL)->file_name)
                          - (char *) NULL, 
                      UNDEF,
    "title",          ATT_STR,                              sizeof (char), 
                      (char *) &(((TEXTLOC *) NULL)->title)
                          - (char *) NULL,     
                      UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,


"tr_vec" , 2, 1, 0, 0,
    "qid",            ATT_INT,                              sizeof (long),
                      (char *) &(((TR_VEC *) NULL)->qid)
                          - (char *) NULL,        
                      UNDEF,
    "num_tr",         ATT_INT,                              sizeof (long),
                      (char *) &(((TR_VEC *) NULL)->num_tr)
                          - (char *) NULL,     
                      UNDEF,
    "tr",             ATT_INT,                              sizeof (TR_TUP), 
                      (char *) &(((TR_VEC *) NULL)->tr)
                          - (char *) NULL,       
                      1,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,

"rr_vec" , 2, 1, 0, 0,
    "qid",            ATT_INT,                              sizeof (long),
                      (char *) &(((RR_VEC *) NULL)->qid)
                          - (char *) NULL,        
                      UNDEF,
    "num_rr",         ATT_INT,                              sizeof (long),
                      (char *) &(((RR_VEC *) NULL)->num_rr)
                          - (char *) NULL,     
                      UNDEF,
    "rr",             ATT_INT,                              sizeof (RR_TUP), 
                      (char *) &(((RR_VEC *) NULL)->rr)
                          - (char *) NULL,       
                      1,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,

"part_vec" , 5, 2, 0, 0,
    "id_num",         ATT_INT,                              sizeof (long),
                      (char *) &(((PART_VEC *) NULL)->id_num)
                          - (char *) NULL,        
                      UNDEF,
    "max_partnum",    ATT_INT,                              sizeof (long),
                      (char *) &(((PART_VEC *) NULL)->max_partnum)
                          - (char *) NULL,     
                      UNDEF,
    "partnums_used",  ATT_INT,                              sizeof (long),
                      (char *) &(((PART_VEC *) NULL)->partnums_used)
                          - (char *) NULL,     
                      UNDEF,
    "num_ctype",      ATT_INT,                              sizeof (long),
                      (char *) &(((PART_VEC *) NULL)->num_ctype)
                          - (char *) NULL,     
                      UNDEF,
    "num_part_conwt", ATT_INT,                              sizeof (long), 
                      (char *) &(((PART_VEC *) NULL)->num_part_conwt)
                          - (char *) NULL,     
                      UNDEF,
    "ctype_len",      ATT_INT,                              sizeof (long), 
                      (char *) &(((PART_VEC *) NULL)->ctype_len)
                          - (char *) NULL,     
                      3,
    "part_con_wtp",   ATT_INT,                            sizeof (PART_CON_WT),
                      (char *) &(((PART_VEC *) NULL)->part_con_wtp)
                          - (char *) NULL,       
                      4,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,
    (char *) 0,       ATT_UND,                              0, 
                      0,                                    UNDEF,

};
