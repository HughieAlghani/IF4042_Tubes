The heart of the flexibility of SMART comes from specification
files.  All parameters, including what procedures to run, are
given by the spec file.  SMART is called by
    smart  top_level_action  spec_file  [modifications to spec_file]

FORMAT:
The format of the spec file is lines of the form
    parameter_name  parameter_value

Blank lines are ignored.  Lines beginning with a '#' are ignored.
Leading blanks are ignored.  If the parameter value has a blank or
a tab in it, the entire parameter value must be enclosed in
double quotes.

PARAMETER_NAME
The parameter name is given by a dotted tuple.  Examples would
be in the spec file lines
        index.query_file.rwmode  SRDWR|SINCORE|SCREATE
        query_file.rmode         SRDONLY|SINCORE
        rmode                    SRDONLY|SINCORE
        rwmode                   SRDWR
Here, "index.query_file.rwmode" is a parameter name specifying
what filemode a query file should be stored as when indexing.
(The parameter value indicates the query file should be opened
read-write, should be kept in memory while being indexed, and 
will be a new file that will need to be created.)

The key field of the parameter_name is the last field.  The
preceding fields contain restrictions of when the key field
matches. 

PARAMETER_NAME MATCHES
Many procedures in SMART require information from the spec file.
They request the values of specific spec file tuples.  For
example, the procedure that converts a query file from one
weighting scheme to another might ask for (among others)
    "convert.query_file"
    "convert.query_file.rmode"
    "convert.new_query_file"
    "convert.new_query_file.rwmode"

The parameter names in the specification file are then examined
to find the best match.  The "key" field, the last in both the
procedure request and the spec file paramater name, must match
exactly; some of the other fields in the request may be dropped
from the spec file name.  The following algorithm is used to
determine the best match:

1. An exact match between request name and spec name is best.
2. If that doesn't exist, each leading field, from left to right,
    of the request name is dropped in turn until a match is found
    or the key field is reached.
3. If a match still hasn't been found, trailing fields, from
    right to left (ignoring the key field), of the request name 
    are dropped until a match is found.
4. If a match still hasn't been found, an error is reported.

In the examples above, the request "convert.query_file.rmode"
doesn't have an exact match in the example spec file lines.
Under rule 2, "query_file.rmode" is then checked against the spec
file and matches.  Thus the value of the parameter is
"SRDONLY|SINCORE".   Similarly, the value of
"convert.new_query_file.rwmode" will be "SRDWR".

Rule 3 most often gets used when a procedure is requesting
several parameters that are named as an array would be named.
For example, the spec file lines
        index.section.name none
        index.section.3.name author
set up a situation where the procedure can request the values for
        index.section.0.name
        index.section.1.name
        index.section.2.name
        index.section.3.name
to find the names of 4 sections.  The first three sections will
all have the value "none", while the fourth will have the value
"author".


PARAMETER VALUES.
The best way to talk about what parameter values are allowed is
to discuss how a program gets those values and specifies the
interpretation of those values.  In our example of converting the
weight of a query, the relevant code fragments are:

static char *orig_file;          /* original vector,assumed to be
                                    tf weighted */
static long orig_mode;           /* Open original vector in
                                    read-only mode */
static char *new_file;           /* New vector */
static long new_mode;            /* read-write mode */

static SPEC_PARAM query_spec_args[] = {
    "convert.query_file",        getspec_dbfile, (char *) &orig_file,
    "convert.query_file.rmode",  getspec_filemode, (char *) &orig_mode,
    "convert.new_query_file",  getspec_dbfile, (char *) &new_file,
    "convert.new_query_file.rwmode", getspec_filemode, (char *) &new_mode,
    ...
    };
static int num_query_spec_args =
    sizeof (query_spec_args) / sizeof (query_spec_args[0]);

int
init_weight_query (spec, unused)
SPEC *spec;
char *unused;
{
    /* lookup the values of the relevant parameters */
    if (UNDEF == lookup_spec (spec,
                              &query_spec_args[0],
                              num_query_spec_args))
        return (UNDEF);
     ...
}


The list of parameters that this procedure needs to look up is
given by an array of SPEC_PARAM tuples.  For each parameter, the
procedure specifies the request parameter name, a location for
the parameter value to be placed, and an interpreting procedure
that will take the string value found in the spec file for this
parameter, convert it into an object of the appropriate type, and
place the value in the designated location.

The function lookup_spec takes the spec file pairs given by
"spec", and the SPEC_PARAM parameter list, and then finds,
converts and stores the appropriate values.

The function lookup_spec_prefix does the same thing as lookup_spec,
but each effective parameter name in the SPEC_PARAM_PREFIX list
includes a run-time value of a prefix to be prepended to the
compile-time value of the parameter name.

The list of system supplied interpretation procedures, along with
the data type they expect to fill in, are

getspec_float     float
getspec_double    double
getspec_string    char *
getspec_intstring char *   Same as getspec_string, except recognizes
                           standard backslash escape sequences. Note:
                           use sparingly since this mallocs unreclaimable
                           space.
getspec_char      char
getspec_bool      int      Recognizes true,on,1,false,off,0
getspec_long      long
getspec_filemode  long     Recognizes smart io flags (see h/io.h)
                           possibly several separated by |
getspec_dbfile    char *   Recognizes smart database filename.
                           If string value is not explictly
                           located elsewhere (by beginning with a
                           '/' or '.') it is assumed to relative
                           to the database name given by the
                           parameter "database".
getspec_func   PROC_TAB *  Interprets the parameter value as the
                           name of a procedure set.  This name is
                           looked up in a procedure hierarchy to
                           find the corresponding procedures
                           (there'll be an initialization procedure,
                           a main procedure and a closing procedure)
                           See documentation for procedures for
                           more information.





