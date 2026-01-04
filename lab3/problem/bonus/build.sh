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
rm "$(dirname "$0")/bin" -rf
rm "$(dirname "$0")/profile_${PREFIX}" -rf
mkdir -p "$(dirname "$0")/profile_${PREFIX}"

PERF="${PERF:-INSTRUCTIONS}"

for tasklet in 1 2 4 8 12 14 16; do
  for i in 1 2 4 8 16 32 64; do
    for gs in LIG AVG LUM; do
      PERF=${PERF} GS=${gs} NR_DPUS=1 NR_TASKLETS=${tasklet} NR_ELEM=360 make || exit 1
      outfile="$(dirname "$0")/profile_${PREFIX}/dpus_1_${gs,,}_tl_${tasklet}_nr_pixels_$((i * 131072)).txt"
      ./bin/host_code -w 2 -e 3 -p $((i * 131072)) >"$outfile"
      make clean
    done
  done
done
