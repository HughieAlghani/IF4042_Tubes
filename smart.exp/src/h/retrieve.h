#ifndef RETRIEVEH
#define RETRIEVEH
/*        $Header: /home/smart/release/src/h/retrieve.h,v 11.0 1992/07/21 18:18:50 chrisb Exp $*/

#include "tr_vec.h"
#include "proc.h"

/* QUERY_VECTOR is designed to accomodate all types of vectors.  Procedures
   to read a query (eg "retrieve.get_query"), and operate on the query
   (eg. "retrieve.coll_sim") must agree on the interpretation of the
   vector sub-field. */
typedef struct {
    long qid;
    char *vector;
} QUERY_VECTOR;

typedef struct {
    long did;
    float sim;
} TOP_RESULT;

/* top_results is the top documents wanted (those a user could see), and is
   kept in decreasing rank order, sim = 0.0 indicates not retrieved */
/* full_results is the sim of all documents and is an array of sims indexed
   by the did */
typedef struct {
    TOP_RESULT *top_results;
    long num_top_results;
    float *full_results;
    long num_full_results;
    long qid;
} RESULT;

/* Conglomeration giving both query and results */
typedef struct {
    QUERY_VECTOR *query;    /* Current query */
    TR_VEC *tr;             /* Rel judgements for top ranked docs this query */
} RETRIEVAL;

/* Alternate result reporting */
typedef struct {
    long qid;
    long did;
    float sim;
} RESULT_TUPLE;

typedef struct {
    long num_results;
    RESULT_TUPLE *results;
} ALT_RESULTS;

/* This should probably be a spec parameter, but really shouldn't ever
   get used */
#define MAX_DID_DEFAULT 16000

/* Used to keep track of results for parts of docs (eg paragraphs) */
typedef struct {
        long max_doc;
        long max_parts;
        float *sum_results;
        float *max_results;
        int *num_parts;
} PARTS_RESULTS;

#endif /* RETRIEVEH */
