#ifndef REL_VECTORH
#define REL_VECTORH
/* a single rel_vector */
typedef struct {
		long   id_num;		/* unique number for this doc within collection */
		long   ctype;		/* ctype number */
		long   con;			/* concept number */
		float  wt;	    	/* weight value */
		}REL_VEC;
#endif /* REL_VECTORH */
