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

nr_dpus=1
for k in 16 1024 16384; do
  for i in SINGLE; do
    for sync in NO MUTEX; do
      for tasklet in 1 2 4 8 16; do
        PRINT=0 PERF=INSTRUCTIONS NR_DPUS=${nr_dpus} NR_TASKLETS=${tasklet} BLOCK=10 SYNC_TYPE=${i} SYNC=${sync} make || exit 1
        outfile="$(dirname "$0")/profile_${PREFIX}/sync_type_${i,,}_${sync,,}_tasklets_${tasklet}_${k}KB.txt"
        ./bin/host_code -w 2 -e 3 -i $((nr_dpus * k * 128)) >"$outfile"
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
        ./bin/host_code -w 2 -e 3 -i $((nr_dpus * k * 128)) >"$outfile"
      done
      make clean
    done
  done
done
