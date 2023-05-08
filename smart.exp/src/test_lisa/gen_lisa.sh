#! /bin/bash


declare  -a methods=(
  "nnn" "ntn" "nnc" "ntc" 
  "bnn" "btn" "bnc" "btc" 
  "ann" "atn" "anc" "atc"
  "lnn" "ltn" "lnc" "ltc"
)

#GEN MAKE_LISA
cp base_make_lisa make_lisa

for method in "${methods[@]}"
do
if [ "$method" = "nnn" ]
  then
    continue
fi
echo "echo \"Create $method versions at \`date\`\"
\$bin/smart convert \$database/spec   proc convert.obj.weight_query \\
                   in query.nnn  out query.$method   query_weight $method
\$bin/smart convert \$database/spec   proc convert.obj.weight_doc \\
                   in doc.nnn  out doc.$method   doc_weight $method
\$bin/smart convert \$database/spec   proc convert.obj.vec_aux  \\
                   in doc.$method  out inv.$method
" >> make_lisa
done

# GEN TEST_LISA
cp base_test_lisa test_lisa

for d_method in "${methods[@]}"
do
for q_method in "${methods[@]}"
do
echo "# $d_method.$q_method experiment
cat > spec.$d_method.$q_method << EOF
include_file \$database/spec
query_file query.$q_method
doc_file doc.$d_method
inv_file inv.$d_method
tr_file ./tr.$d_method.$q_method
rr_file ./rr.$d_method.$q_method
run_name \" Doc weight == $d_method; Query weight == $q_method;\"
EOF

\$bin/smart retrieve spec.$d_method.$q_method
echo \"Retrieval run $d_method.$q_method done\"
" >> test_lisa
done
done
