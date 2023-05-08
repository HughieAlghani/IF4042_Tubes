# IF4042_Tubes

## Preparation:
1. Ganti path sesuai path di lokal kalian:
    - lib/spec.default
    - lib/spec.expcoll
    - src/test_lisa/base_make_lisa
    - src/test_lisa/base_test_lisa
2. Buka terminal
    - Masuk ke path /smart.exp/src/bin, lalu run 'chmod +x smart'
    - Masuk ke path /smart.exp/src/test_lisa, lalu run 'chmod +x make_lisa'

## Make Test Collection Weighting Scheme
1. Buka terminal
2. Masuk ke path /smart.exp/src/test_lisa
3. Run './gen_lisa.sh'
4. Run './make_lisa'
5. Run './test_lisa'
6. File hasil pembuatan test collection dapat dilihat pada /smart.exp/src/test_lisa/test dengan nama format nama file spec.{ddd.qqq} (ddd.qqq -> weighting scheme)

## Evaluate Test Collection Weighting Scheme
1. Pastikan sudah melakukan pembuatan test colelction
2. Masuk ke path /smart.exp/src/test_lisa/test
3. Run '../../bin/smprint tr_eval {nama file test}'

## Get Test Result (Nomor 1)
1. Buka terminal
2. Masuk ke path /smart.exp/src/test_lisa
3. Run './gen_lisa.sh'
4. Run './eval_lisa.sh'
5. Hasil dari pengujian dapat dilihat pada /smart.exp/src/test_lisa/test/result