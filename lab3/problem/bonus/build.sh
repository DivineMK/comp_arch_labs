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

out_csv="$(dirname "$0")/${PREFIX}_bonus.csv"
echo "nr_dpus,nr_tasklets,gs,instructions,throughput,n_images,image_dim" >"$out_csv"

for dpu in 1 2 4 8 16 17 19 20; do
  for tasklet in 1 2 4 8 16; do
    for n_images in 128 256 512; do
      for image_dim in 32 64 128; do
        for gs in LIG AVG LUM; do
          PERF=INSTRUCTIONS GS=${gs} NR_DPUS=${dpu} NR_TASKLETS=${tasklet} NR_ELEM=360 make || exit 1
          outfile="$(dirname "$0")/profile_${PREFIX}/dpus_${dpu}_${gs,,}_tl_${tasklet}_n_${n_images}_d_${image_dim}.txt"
          ./bin/host_code -w 0 -e 1 -d ${image_dim} -n ${n_images} >"$outfile"
          instructions=$(awk -F',' '/DPU instructions/ {split($1, a, "max: "); print a[2]}' "$outfile")
          throughput=$(awk -F'= ' '/Arithmetic throughput/ {print $2}' "$outfile" | awk '{print $1}')

          echo "${dpu},${tasklet},${gs},$instructions,$throughput,$n_images,${image_dim}" >>"$out_csv"
          make clean
        done
      done
    done
  done
done
