#!/usr/bin/bash

print_help() {
  cat <<EOF
Usage: $0 [OPTIONS]

Options:
  -p, --prefix          Prefix to logs (hw/sw)
  -h, --help            Show this help message

Example:
  $0 -p sw
EOF
}

PREFIX="sw"
while getopts "p:h:" opt; do
  case $opt in
  p) PREFIX="$OPTARG" ;;
  h) print_help ;;
  \?)
    echo "Invalid option -$OPTARG" >&2
    exit 1
    ;;
  esac
done

rm $(dirname "$0")/bin -rf
mkdir -p "$(dirname "$0")/profile_${PREFIX}"

out_csv="$(dirname "$0")/${PREFIX}_task4.csv"
echo "nr_dpus,nr_tasklets,data_type,sync,input_size,instructions,dpu_time" >"$out_csv"
nr_dpus=1
for k in 16 1024 16384; do
  for i in SINGLE; do
    for sync in NO MUTEX; do
      for tasklet in 1 2 4 8 16; do
        PRINT=0 PERF=INSTRUCTIONS NR_DPUS=${nr_dpus} NR_TASKLETS=${tasklet} BLOCK=10 SYNC_TYPE=${i} SYNC=${sync} make || exit 1
        outfile="$(dirname "$0")/profile_${PREFIX}/sync_type_${i,,}_${sync,,}_tasklets_${tasklet}_${k}KB.txt"
        ./bin/host_code -w 0 -e 1 -i $((nr_dpus * k * 128)) >"$outfile"
        instructions=$(awk -F',' '/DPU instructions/ {split($1, a, "max: "); print a[2]}' "$outfile")
        dpu_time=$(awk -F'DPU Kernel Time \\(ms\\):' 'NF>1 {print $2+0}' "$outfile")

        echo "1,${tasklet},INT64,${i}_${sync},$k,$instructions,${dpu_time}" >>"$out_csv"
      done
      make clean
    done
  done
done

for k in 16 1024 16384; do
  for i in TREE; do
    for sync in BARRIER HANDSHAKE; do
      for tasklet in 1 2 4 8 16; do
        PRINT=0 PERF=INSTRUCTIONS NR_DPUS=${nr_dpus} NR_TASKLETS=${tasklet} BLOCK=10 SYNC_TYPE=${i} SYNC=${sync} make || exit 1
        outfile="$(dirname "$0")/profile_${PREFIX}/sync_type_${i,,}_${sync,,}_tasklets_${tasklet}_${k}KB.txt"
        ./bin/host_code -w 0 -e 1 -i $((nr_dpus * k * 128)) >"$outfile"
        instructions=$(awk -F',' '/DPU instructions/ {split($1, a, "max: "); print a[2]}' "$outfile")
        dpu_time=$(awk -F'DPU Kernel Time \\(ms\\):' 'NF>1 {print $2+0}' "$outfile")

        echo "1,${tasklet},INT64,${i}_${sync},$k,${instructions},${dpu_time}" >>"$out_csv"
      done
      make clean
    done
  done
done
