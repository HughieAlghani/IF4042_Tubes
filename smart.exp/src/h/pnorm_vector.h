#ifndef PNORM_VECTORH
#define PNORM_VECTORH
typedef struct {
  int op;                              /* type of operator (AND, OR, NOT) */
  float p;                             /* p value of operator (0.0 for NOT) */
  float wt;                            /* clause wt of clause defined by op */
} OP_P_WT;


typedef struct {
  long info;                          /* pointer into OP_P_WT or CON_WT */
  short child;                        /* pointer to leftmost child */
  short sibling;                      /* ptr to next sibling to the right */
} TREE_NODE;


typedef struct {
  long id_num;                        /* query id */
  short num_ctype;                    /* number of ctypes in doc collection */
  short num_conwt;                    /* number concept-wt pairs in query */
  short num_nodes;                    /* number tree nodes in query */
  short num_op_p_wt;                  /* number of operator nodes in tree */
  short *ctype_len;                   /* number of tuples of each ctype */
  CON_WT *con_wtp;                    /* ptr to concept-weight tuples */
  TREE_NODE *tree;                    /* tree defining structure of query */
  OP_P_WT *op_p_wtp;                  /* ptr to op_p_wt tuples */
} PNORM_VEC;

#define REL_PNORM_VEC 7
#endif /* PNORM_VECTORH */
