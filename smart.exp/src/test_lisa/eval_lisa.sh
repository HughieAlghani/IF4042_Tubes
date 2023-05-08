#! /bin/bash

declare  -a methods=(
  "nnn" "ntn" "nnc" "ntc" 
  "bnn" "btn" "bnc" "btc" 
  "ann" "atn" "anc" "atc"
  "lnn" "ltn" "lnc" "ltc"
)

./test_lisa

cd test

for d_method in "${methods[@]}"
do
for q_method in "${methods[@]}"
do
echo "METHOD $d_method.$q_method

$(tcsh ../../bin/smprint rr_eval spec.$d_method.$q_method \
  | tail -n 23 \
  | sed '/5 docs/d' \
  | sed '/10 docs/d' \
  | sed '/30 docs/d' \
  | sed '/^$/d'
)

---------------------------------------" >> result
done
done

echo "Experiment is complete"

# ../../bin/smprint rr_eval spec.nnn.nnn \
#   | tail -n 23 \
#   | sed '/5 docs/d' \
#   | sed '/10 docs/d' \
#   | sed '/30 docs/d' \
#   | sed '/^$/d'

# ../../bin/smprint rr_eval spec.atc.atc \
#   | tail -n 23 \
#   | sed '/5 docs/d' \
#   | sed '/10 docs/d' \
#   | sed '/30 docs/d' \
#   | sed '/^$/d'