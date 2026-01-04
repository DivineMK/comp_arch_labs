#!/usr/bin/bash
rm $(dirname "$0")/bin/ -rf
# rm $(dirname "$0")/profile -rf
mkdir -p $(dirname "$0")/profile

out_csv="$(dirname "$0")/task3.csv"
echo "nr_dpus,nr_tasklets,data_type,operation,instructions,throughput" >"$out_csv"
i=1
for op in ADD SUB DIVIDE MUL; do
  for type in INT32 UINT32 INT64 FLOAT DOUBLE UINT64 SHORT CHAR; do
    case $type in
    INT32 | UINT32 | FLOAT) i=2 ;;
    SHORT) i=4 ;;
    CHAR) i=8 ;;
    *) i=1 ;;
    esac
    for tasklet in 1 2 4 8 16; do
      PERF=INSTRUCTIONS NR_DPUS=1 NR_TASKLETS=${tasklet} BLOCK=10 OP=${op} TYPE=${type} make || exit 1
      outfile="$(dirname "$0")/profile/${op,,}_${type,,}_tl_${tasklet}.txt"
      ./bin/host_code -w 0 -e 1 -i $((i * 1048576)) >"$outfile"
      instructions=$(awk -F',' '/DPU instructions/ {split($1, a, "max: "); print a[2]}' "$outfile")
      throughput=$(awk -F'= ' '/Arithmetic throughput/ {print $2}' "$outfile" | awk '{print $1}')

      echo "1,${tasklet},${type},${op},$instructions,$throughput" >>"$out_csv"
      make clean
    done
  done
done
