/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Generate hypertext-style webs of document relationships
 *2 build_web(is,unused)
 *3    INTER_STATE *is;
 *3    char *unused;
 *4 init_build_web(spec,unused)
 *5    "web.depth.count" - depth to build tree
 *5    "web.breadth.count" - breadth of tree
 *5    "web.breadth.threshold" - trim breadth at this sim threshold
 *5    "web.output.format" - determines output format
 *5    "web.log" - 0=don't/1=do log actions (useful for big webs)
 *5    "retrieve.doc.textloc_file" - for maxdocid
 *5    "inter.coll_sim" - for finding similar documents
 *4 close_build_web(inst)

 *7 The command "Web docid" builds a web of similar documents starting
 *7 from the given docid.  The web is built by finding the top matching
 *7 documents using "coll_sim" (it finds num_wanted documents).  Then
 *7 the process repeats for each of the "breadth.count" top matching
 *7 returned.  If "breadth.threshold" is specified as non-zero, the
 *7 "breadth.count" top is trimmed if any have a similarity lower than
 *7 that specified.
 *7
 *7 The process is repeated recursively up to "depth.count" times.
 *7
 *7 The output format can be "dag" for a format useful in the dag
 *7 program, or "terminal" for a format which displays on the screen.

 *9 Output formats currently vary radically.

***********************************************************************/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "textloc.h"
#include "io.h"
#include "smart_error.h"
#include "spec.h"
#include "proc.h"
#include "sm_display.h"
#include "tr_vec.h"
#include "context.h"
#include "retrieve.h"
#include "docindex.h"
#include "inter.h"


/*
 * Constants for formatting the output.
 */
#define TITLE_WIDTH 64
static char temp_buf[PATH_LEN];

/*
 * Determines how we'll build the hypertext web.  The depth is always
 * limited to the count specified.  The breadth is limited by the count
 * if the threshold is 0.0, or by the threshold if the threshold is not
 * zero.  In the latter case, though, the count limits the breadth
 * if there are too many documents matching above the threshold.
 */
static long depth_count, breadth_count;
static float breadth_thresh;

static PROC_INST p_retrieve;
static char *textloc_file;      /* Used to tell how many docs in collection */
static char *output_format;
static long log_actions;

static SPEC_PARAM spec_args[] = {
    "web.depth.count",		getspec_long,	(char *) &depth_count,
    "web.breadth.count",	getspec_long,	(char *) &breadth_count,
    "web.breadth.threshold",	getspec_float,	(char *) &breadth_thresh,
    "web.output.format",	getspec_string, (char *) &output_format,
    "web.log",			getspec_long,	(char *) &log_actions,

    "retrieve.doc.textloc_file", getspec_dbfile, (char *) &textloc_file,
    "inter.coll_sim",            getspec_func,    (char *) &p_retrieve.ptab,
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

static int num_inst = 0;

static int did_vec_inst;

static float **tr_local_sim;  /* local sims (if stored) */
static TR_VEC *tr_web;	/* a docid and its matching documents */
static int next_free_web; /* next available slot in the web */
static long max_web_size; /* max number of nodes that can be in tree */

static int *web_idx;    /* pointer into tr_web, indexed by docid */
static short *web_where_seen; /* how close to root this docid appeared */
static int max_did_in_coll;

#define Malloc(n,type) (type *)malloc( (unsigned) (n)*sizeof(type))

static int web_recurse();
static int save_topr_in_web();
static void print_web();
static void print_dag_web();
static int not_in();

int init_build_web( spec, unused )
SPEC *spec;
char *unused;
{
    long i;
    REL_HEADER *rh;

    if (num_inst++ > 0) {
        return (num_inst);
    }
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);
    if (UNDEF == (did_vec_inst = init_did_vec (spec, (char *) NULL)))
        return (UNDEF);

    /*
     * Determine the output format that'll be used.
     */
    if (output_format == NULL ||
	(*output_format != 't' && *output_format != 'd')) {
	set_error( SM_ILLPA_ERR, "output_format", "init_build_web" );
	return UNDEF;
    }

    /*
     * Create space for the entire web results. We know that the maximum
     * size of the tree is (B^(D+1) - 1)/(B-1), but we don't store the
     * leaves in here (since they aren't expanded), so can just raise to
     * the power of D.
     */
    max_web_size = ((long) pow( (double)breadth_count,
			       (double)depth_count) - 1) /
	           (breadth_count - 1) + 1;
    if (NULL== (tr_web = Malloc( max_web_size, TR_VEC )) ||
	NULL== (tr_local_sim = Malloc( max_web_size, float * )))
	return UNDEF;
    for (i=0; i<max_web_size; i++)
	    if (NULL== (tr_web[i].tr = Malloc( breadth_count, TR_TUP )) ||
		NULL== (tr_local_sim[i] = Malloc( breadth_count, float )))
		return UNDEF;

    if (NULL== (web_where_seen = Malloc( max_web_size, short )))
	return UNDEF;

    /*
     *  Need a pointer into the web results for each possible docid.
     * 
     */
    if (NULL != (rh = get_rel_header (textloc_file)) && rh->max_primary_value)
        max_did_in_coll = rh->max_primary_value;
    else
        max_did_in_coll = MAX_DID_DEFAULT;
    if (NULL== (web_idx = Malloc( max_did_in_coll, int )))
	return UNDEF;

    return num_inst;
}

int
close_build_web( inst )
int inst;
{
    long i;

    if (--num_inst > 0)
	return 0;
    if (UNDEF == close_did_vec (did_vec_inst))
	return UNDEF;

    for (i=0; i<max_web_size; i++) {
	(void) free( (char *)tr_web[i].tr );
	(void) free( (char *)tr_local_sim[i] );
    }	
    (void) free( (char *) tr_web );
    (void) free( (char *) tr_local_sim );
    (void) free( (char *) web_idx );
    (void) free( (char *) web_where_seen );

    return 1;
}



/******************************************************************
 *
 * Build the web of related documents.
 *
 ******************************************************************/
static VEC qvec;

int
build_web( is, unused )
INTER_STATE *is;
char *unused;
{
    VEC_LIST vec_list;
    VEC temp_vec;
    RESULT results;	/* full results of any given query */
    TR_VEC *q_tr;
    TEXTLOC textloc;

    (void) bzero( (char *)web_idx, sizeof(int)*(int)max_did_in_coll );
    (void) bzero( (char *)web_where_seen, sizeof(short)*(int)max_web_size );
    next_free_web = 1;	/* omit 0; we use it as an `unused' flag */


    vec_list.vec = &temp_vec;

    if (is->num_command_line < 2 ||
        UNDEF == inter_get_sect_veclist (is->command_line[1], &vec_list) ||
        vec_list.num_vec != 1) {
        if (UNDEF == add_buf_string (
                              "Not a valid single document. Command ignored\n",
                               &is->err_buf))
            return (UNDEF);
        return (0);
    }

    if (vec_list.id_num > max_did_in_coll) {
        if (UNDEF == add_buf_string (
                              "Not a valid docid. Command ignored\n",
                               &is->err_buf))
            return (UNDEF);
        return (0);
    }

    /* MEMORY LEAK */
    if (UNDEF == copy_vec (&qvec, &vec_list.vec[0]))
        return (UNDEF);

    is->retrieval.tr->num_tr = 0;
    is->retrieval.query->vector = (char *) &qvec;
    is->retrieval.query->qid = qvec.id_num;

    /*
     * We get the top-level query results and store them in the
     * standard top-results storage.
     */
    if (log_actions)
	(void) printf( "Finding matches for query %s %d (depth=0)\n",
		      qvec.id_num==0 ? "section of docid" : "docid",
		      qvec.id_num==0 ? vec_list.id_num : qvec.id_num );
    if (UNDEF == p_retrieve.ptab->proc (&is->retrieval,
                                        &results,
                                        p_retrieve.inst))
        return (UNDEF);
    if (UNDEF == res_tr (&results, is->retrieval.tr))
        return (UNDEF);


    /*
     * Store the main query in the web.  Then start building the
     * web of links.
     */
    if ( save_topr_in_web( is->retrieval.query->qid, &results ) != 1 )
	return UNDEF;
    web_where_seen[1] = depth_count;  /* seen at the root */

    q_tr = &tr_web[ web_idx[ is->retrieval.query->qid ]];
    if ( web_recurse( (int)(depth_count-1), q_tr ) != 1 )
	return UNDEF;

    /*
     * Now generate a displayed version of the web (as a tree).
     */
    if (1 != inter_get_textloc( qvec.id_num==0 ? vec_list.id_num : qvec.id_num,
			       &textloc ))
	textloc.title = "Document not in database";

    (void) sprintf( temp_buf, "Web starting from %s %d %.*s\n",
		   qvec.id_num==0 ? "section of" : "",
		   qvec.id_num==0 ? vec_list.id_num : qvec.id_num,
		   TITLE_WIDTH, textloc.title );
    if (UNDEF==add_buf_string( temp_buf, &is->output_buf ))
	return UNDEF;

    if (*output_format == 'd')
	print_dag_web( is, q_tr );
    else
	print_web( is, (int)(depth_count-1),
		   q_tr,
		   tr_local_sim[ web_idx[is->retrieval.query->qid]],
		   "" );

    return 1;
}



/******************************************************************
 *
 * Display the web as a tree with nice indentation where appropriate.
 * If a document occurs more than once in the tree, the subtree
 * below it is expanded only once, and only the time that it occurs
 * the closest to the root.
 *
 ******************************************************************/
static void
print_web( is, depth, this_tr, this_locsim, prefix )
INTER_STATE *is;
int depth;
TR_VEC *this_tr;
float *this_locsim;
char *prefix;
{
    long i;
    long did;
    int height = depth_count - depth;
    char *new_prefix, bad_prefix;
    int last_child = 0;
    TEXTLOC textloc;

    if (NULL== (new_prefix = Malloc( height*3+1, char ))) {
	new_prefix = &bad_prefix;
	bad_prefix = '0';	/* rather than aborting */
    }

    /* find the last one (this is for pretty-printing) */
    for (i=0; i<breadth_count; i++)
	if ( this_tr->tr[i].sim != 0.0 )
	    last_child = i;

    for (i=0; i<breadth_count; i++)
	if ( this_tr->tr[i].sim != 0.0 ) {
	    did = this_tr->tr[i].did;

	    if (height==1)
		if (UNDEF==add_buf_string( "|\n", &is->output_buf ))
		    return; /* abort */
	    if (1 != inter_get_textloc( did, &textloc ))
		textloc.title = "Document not in database";
	    
	    (void) sprintf( temp_buf,
			   "%s+--%-6d   %4.2f %.*s\n",
			   prefix,
			   did, this_tr->tr[i].sim,
			   TITLE_WIDTH, textloc.title );
	    if (UNDEF==add_buf_string( temp_buf, &is->output_buf ))
		return;

	    /* Local similarity if there is one */
	    if ( this_locsim[i] != 0.0 ) {
		(void) sprintf( temp_buf,
			        "%s%s%-6s   has part sim of %.2f\n",
			        prefix,
			        i==last_child ? "   " : "|  ",
			        depth==0 ? "" : "|",
			        this_locsim[i] );
		if (UNDEF==add_buf_string( temp_buf, &is->output_buf ))
		    return;
	    }

	    if (depth > 0 && depth>=web_where_seen[ web_idx[did] ]) {
		if ( new_prefix != &bad_prefix )
		    (void) sprintf( new_prefix, "%s%s", prefix,
			    i==last_child ? "   " : "|  " );
		print_web( is, depth-1, &tr_web[ web_idx[ did ]],
			   tr_local_sim[ web_idx[ did ]],
			   new_prefix );
	}
    }

}


/******************************************************************
 *
 * Print the web in a format useful in the dag program (from AT&T).
 * Note that this is easier.  All we have to display is the nodes
 * and edges.  No recursion is necessary.
 *
 * WARNING: This routine puts some negative values in the web_idx
 *	    index (to know if a docid has already been displayed as
 *	    a leaf).
 *
 ******************************************************************/
static void
print_dag_web( is, this_tr )
INTER_STATE *is;
TR_VEC *this_tr;
{
    long i, j;
    TEXTLOC textloc;

    if (UNDEF==add_buf_string(
			  "Discard this and all preceding lines\n.GS fill\n",
			  &is->output_buf ))
	return;

    /*
     * Go through all nodes and display them.  We use the convention
     * that nodes get a name "node.nn" where "nn" is its docid.
     * Leaf nodes (though that were not expanded when the web was
     * built) get the name "leaf.nn" where, again, "nn" is the docid.
     * There's no real reason to distinguish between them, actually,
     * but it was done.
     */
    for (i=1; i<next_free_web; i++) {
	if (1 != inter_get_textloc( tr_web[i].qid, &textloc ))
	    textloc.title = "Document not in database";
	(void) sprintf( temp_buf,
		      "draw node.%d as Box label \"%d\\c%.*s\" pointsize 9;\n",
		      tr_web[i].qid, tr_web[i].qid,
		      TITLE_WIDTH, textloc.title );
	if (UNDEF==add_buf_string( temp_buf, &is->output_buf ))
	    return;

	/*
	 * Now get the children of this node (unless the child is a
	 * non-leaf node itself).
	 */
	for (j=0; j<tr_web[i].num_tr; j++)
	    if ( !web_idx[ tr_web[i].tr[j].did ] ) {
		if (1 != inter_get_textloc( tr_web[i].tr[j].did, &textloc ))
		    textloc.title = "Document not in database";
		(void) sprintf( temp_buf, "draw leaf.%d as Box label",
			        tr_web[i].tr[j].did );
		if (UNDEF==add_buf_string( temp_buf, &is->output_buf ))
		    return;

		(void) sprintf( temp_buf, " \"%d\\c%.*s\" pointsize 9;\n",
			       tr_web[i].tr[j].did,
			       TITLE_WIDTH, textloc.title );
		if (UNDEF==add_buf_string( temp_buf, &is->output_buf ))
		    return;

		web_idx[ tr_web[i].tr[j].did ] = -1; /* don't print again */
	    }
    }
    if (UNDEF==add_buf_string( "\n", &is->output_buf ))
	return;

    /*
     * Now dump the edges.
     */
    for (i=1; i<next_free_web; i++) {
	int qid = tr_web[i].qid;
	TR_TUP *trtup = &tr_web[i].tr[0];

	for (j=0; j<tr_web[i].num_tr; j++) {
	    /*
	     * At this point, there's an edge from i-->j.  We
	     * omit drawing it if there's an edge from j to i
	     * also AND i>j.
	     */

	    if ( web_idx[ trtup[j].did ]<= 0||	/* always -->leaf */
		 i < web_idx[trtup[j].did] ||	/* always go from lo->hi */
		 not_in( qid, &tr_web[web_idx[trtup[j].did]] )) {
		(void) sprintf( temp_buf, "edge from node.%d to ", qid );
		if (UNDEF==add_buf_string( temp_buf, &is->output_buf ))
		    return;

		(void) sprintf( temp_buf, "%s.%d",
			       web_idx[trtup[j].did]>0 ? "node" : "leaf",
			       trtup[j].did );
		if (UNDEF==add_buf_string( temp_buf, &is->output_buf ))
		    return;

		(void) sprintf( temp_buf, " label \"%4.2f\" pointsize 9;\n",
			       trtup[j].sim );
		if (UNDEF==add_buf_string( temp_buf, &is->output_buf ))
		    return;
	    }
	}
    }
    if (UNDEF==add_buf_string( ".GE\nDiscard this and all following lines\n",
			      &is->output_buf ))
	return;
}

/******************************************************************
 *
 * Scan a tr vector for a particular document id.  If that doc is
 * found, return false; return true otherwise.
 *
 ******************************************************************/
static int not_in( did, this_tr )
int did;
TR_VEC *this_tr;
{
    long i;

    for (i=0; i<this_tr->num_tr; i++)
	if ( this_tr->tr[i].did == did )
	    return 0;

    return 1;
}

/******************************************************************
 *
 * Find the matching documents for each document in the passed
 * top-ranked list (this_tr).  For each of the resulting matches,
 * invoke this routine recursively.  Stop when the `depth' hits 0.
 * Return 1 on success only.
 *
 ******************************************************************/
static int
web_recurse( depth, this_tr )
int depth;
TR_VEC *this_tr;
{
    long i;
    RESULT results;
    RETRIEVAL retrieval;
    QUERY_VECTOR query;
    long did;
    VEC qvec;
    TR_VEC seen_tr;

    if (depth==0) return 1;

    for (i=0; i<breadth_count; i++) {
	did = this_tr->tr[i].did;
	if (log_actions)
	    (void) printf( "Looking up docid %d (depth=%d)\n",
		    did, depth_count-depth );

	if ( !web_idx[did] ) {
	    if (UNDEF == did_vec( &did, &qvec, did_vec_inst ))
		return UNDEF;

	    retrieval.query = &query;
	    retrieval.tr = &seen_tr;
	    query.vector = (char *) &qvec;
	    query.qid = did;
	    seen_tr.num_tr = 0;
	    seen_tr.tr = NULL;

	    if (UNDEF == p_retrieve.ptab->proc (&retrieval,
						&results,
						p_retrieve.inst))
		return (UNDEF);
	    
	    /*
	     * Now we have the query results; save them and
	     * continue.
	     */
	    if ( save_topr_in_web( did, &results ) != 1 )
		return UNDEF;
	}

	/*
	 * Recall the place closest to the root where we encountered
	 * this docid.  (We'll prune the subtree under this everywhere
	 * else when printing.)
	 */
	if ( web_where_seen[ web_idx[did] ] < depth )
	    web_where_seen[ web_idx[did] ] = depth;
    }

    /*
     * Now do the recursive bit.
     */
    for (i=0; i<breadth_count; i++) {
	did = this_tr->tr[i].did;

	if ( web_recurse( depth-1, &tr_web[ web_idx[ did ]] ) != 1 )
	    return UNDEF;
    }

    return 1;
}


/******************************************************************
 *
 * Copy the top-ranked (tr) documents for the indicated query (qid)
 * into the web storage.  Return 1 on success only.
 *
 * If the query itself is discovered in the list of matching docs,
 * it is not saved.
 *
 * See if there is any saved local-results.  If so, grab those, too.
 *
 ******************************************************************/
static int
save_topr_in_web( qid, results )
long qid;
RESULT *results;
{
    long i, j, loc_saved, num_saved;
    int idx = next_free_web++;
    TR_VEC *save_tr = &tr_web[idx];

    TOP_RESULT *t = results->top_results;
    TOP_RESULT *local_sims;
    long num_top = results->num_top_results;

    int local_comb_saved_results();
    web_idx[qid] = idx;

    /*
     * Transfer information, fill out the trailing top-rank tuples
     * with a 0.0 similarity, indicating they're unused.
     */
    num_saved = 0;
    save_tr->qid = qid;
    for (i=0; i<num_top && num_saved<breadth_count; i++) {
	if ( t[i].did != qid && (breadth_thresh == 0.0 ||
				 t[i].sim >= breadth_thresh )) {
	    save_tr->tr[num_saved].did = t[i].did;
	    save_tr->tr[num_saved].sim = t[i].sim;
	    save_tr->tr[num_saved].rank = num_saved+1;
	    tr_local_sim[idx][num_saved] = 0.0;
	    num_saved++;
	}
    }
    save_tr->num_tr = num_saved;

    for (i=num_saved; i<breadth_count; i++) {
	save_tr->tr[i].sim = 0.0;
	tr_local_sim[idx][i] = 0.0;
    }

    /*
     * See if we have any local results available.  Unfortunately,
     * the sorting is out of whack in the two arrays, so we have
     * to do this the slow way.
     */
/*    (void) local_comb_saved_results( &loc_saved, &local_sims ); */
    (void) local_comb_saved_results( &local_sims, &loc_saved );
    for (i=0; i<loc_saved; i++)
	for (j=0; j<num_saved; j++)
	    if ( local_sims[i].did == save_tr->tr[j].did )
		tr_local_sim[idx][j] = local_sims[i].sim;

    return 1;
}
