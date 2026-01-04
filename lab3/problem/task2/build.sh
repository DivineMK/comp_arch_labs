#!/bin/bash
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

rm "$(dirname "$0")"/bin -rf
rm "$(dirname "$0")/profile_${PREFIX}" -rf
mkdir -p "$(dirname "$0")/profile_${PREFIX}"

for type in CHAR SHORT INT32 UINT32 INT64 DOUBLE UINT64 FLOAT; do
  case $type in
  INT32 | UINT32 | FLOAT) i=2 ;;
  SHORT) i=4 ;;
  CHAR) i=8 ;;
  *) i=1 ;;
  esac
  for dpu in 1; do
    for tasklet in {1..24}; do
      PRINT=0 PERF=INSTRUCTIONS NR_DPUS=${dpu} NR_TASKLETS=${tasklet} BLOCK=10 TYPE=${type} make || exit 1
      outfile="$(dirname "$0")/profile_${PREFIX}/dpus_${dpu}_datatype_${type,,}_tl${tasklet}.txt"
      ./bin/host_code -w 2 -e 3 -i $((dpu * i * 1048576)) >"$outfile"
      make clean
    done
  done
done
