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
rm "$(dirname "$0")/profile_${PREFIX}" -rf
mkdir -p "$(dirname "$0")/profile_${PREFIX}"

for transfer in SERIAL PARALLEL BROADCAST; do
  for i in 1 2 4 8 16 32 64; do
    TRANSFER=$transfer NR_DPUS=$i NR_TASKLETS=1 make || exit 1
    for j in 1 24 48; do
      size=$((j * i * 128 * 1024)) #type=int64_t
      outfile="$(dirname "$0")/profile_${PREFIX}/transfer_${transfer,,}_dpus_${i}_tasklets_1_${j}MB.txt"
      ./bin/host_code -w 2 -e 3 -i ${size} >"$outfile"

    done
    make clean
  done
done
