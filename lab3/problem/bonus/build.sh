#!/usr/bin/bash
rm $(dirname "$0")/bin -rf
rm $(dirname "$0")/profile -rf
mkdir -p $(dirname "$0")/profile

out_csv="$(dirname "$0")/bonus.csv"
echo "nr_dpus,nr_tasklets,gs,instructions,throughput" >"$out_csv"

for dpu in 1 2 4 8 16; do
  for tasklet in 1 2 4 8 16; do
    for n_images in 128 256 512 1024; do
      for image_dim in 32 64 128 256; do
        for gs in LIG AVG LUM; do
          PERF=INSTRUCTIONS GS=${gs} NR_DPUS=1 NR_TASKLETS=${tasklet} NR_ELEM=340 make || exit 1
          outfile="$(dirname "$0")/profile/dpus_${dpu}_${gs,,}_tl_${tasklet}.txt"
          ./bin/host_code -w 0 -e 1 -d ${image_dim} -n ${n_images} >"$outfile"
          instructions=$(awk -F',' '/DPU instructions/ {split($1, a, "max: "); print a[2]}' "$outfile")
          throughput=$(awk -F'= ' '/Arithmetic throughput/ {print $2}' "$outfile" | awk '{print $1}')

          echo "${dpu},${tasklet},${gs},$instructions,$throughput" >>"$out_csv"
          make clean
        done
      done
    done
  done
done
