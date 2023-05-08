#! /bin/csh
# fdbkrun database docwt.qwt num_expand
#
# shell script which given an indexed smart collection (nnn weights)
#       1. Creates inverted file for docs with the given docwt
#       2. Creates query vectors weighted with qwt
#       3. Makes an initial run with the weighted docs and queries,
#          retrieving 15 docs
#       4. makes a residual collection run on the original docs and queries
#          (ie, removes top 15 docs from consideration for retrieval and
#          evaluation)
#       5. Makes a feedback residual collection run (using relevance
#          judgements from the top 15 docs, but then ignoring them for
#          retrieval and evaluation).
# WARNING: assumes single ctype
# set echo verbose

set database = /hd2/ohsumed/indexed-trec
set workdir = /hd2/ohsumed/trec/fdbk.d.d.d.30.20
set bin = /home/smart.11.0/src/bin
set doc = lnc
set query = ltc
if (! -d $workdir) mkdir $workdir
cd $workdir

# weight is "docwt.querywt" with weights specified as coded in
#       src/libconvert and documented in weight_tri
# num_expand is the number of terms from relevant docs to be added to
#       the new feedback query.
set weight = lnc
set num_expand = 40
set db = ohsumed
# num_wanted is the number of top docs of original run to be used for feedback


set run_name = $weight.$num_expand


# Make original retrieval run
echo original retrieval begin
set num_wanted = 20
/bin/rm -f  $workdir/tr.$run_name $workdir/tr.$run_name.var 
/bin/rm -f  $workdir/rr.orig.$run_name $workdir/rr.orig.$run_name.var
cat > spec.orig.$run_name << EOF
include_file    $database/run_d.spec
inv_file        $database/inv.${doc}
doc_file        $database/doc.${doc}
query_file      $database/query.${query}
tr_file         $workdir/tr.$run_name
rr_file         $workdir/rr.orig.$run_name
num_wanted      $num_wanted
run_name        "$db Orig `date`.  Weight(D.Q) $weight Num_wanted $num_wanted"
EOF
$bin/smart retrieve spec.orig.$run_name

# Make residual collection run
echo residual run begin
set num_wanted = 30
/bin/rm -f  $workdir/rr.resid.$run_name $workdir/rr.resid.$run_name.var
cp $workdir/tr.$run_name $workdir/tr.resid.$run_name
if (-e $workdir/tr.$run_name.var) then
        cp $workdir/tr.$run_name.var $workdir/tr.resid.$run_name.var
endif
cat > spec.resid.$run_name << EOF
include_file    $database/run_d.spec
get_seen_docs   retrieve.get_seen_docs.get_seen_docs
inv_file        $database/inv.${doc}
doc_file        $database/doc.${doc}
query_file      $database/query.${query}
tr_file         $workdir/tr.resid.$run_name
rr_file         $workdir/rr.resid.$run_name
num_wanted      $num_wanted
run_name        "$db. Resid `date`. Weight(D.Q) $weight Num_wanted $num_wanted"
EOF
$bin/smart retrieve spec.resid.$run_name

# Make feedback run
echo feedback run begin
set num_wanted = 30
/bin/rm -f  $workdir/tr.fdbk.$run_name $workdir/tr.fdbk.$run_name.var
/bin/rm -f  $workdir/rr.fdbk.$run_name $workdir/rr.fdbk.$run_name.var
cat > spec.fdbk.$run_name << EOF
include_file    $database/run_d.spec
get_seen_docs   retrieve.get_seen_docs.get_seen_docs
inv_file        $database/inv.${doc}
doc_file        $database/doc.${doc}
query_file      $database/query.${query}
tr_file         $workdir/tr.$run_name
rr_file         $workdir/rr.fdbk.$run_name
num_wanted      $num_wanted
feedback.num_expand  $num_expand
run_name        "$db. Fdbk  `date`. Weight(D.Q) $weight Num_expand $num_expand"
EOF
$bin/smart retrieve.top.ret_fdbk spec.fdbk.$run_name

# Evaluate and print out results
echo evaluation begin
$bin/smart print spec.orig.$run_name proc print.obj.rr_eval \
      in "spec.orig.$run_name spec.resid.$run_name spec.fdbk.$run_name" 

time




