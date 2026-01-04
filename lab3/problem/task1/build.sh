#!/usr/bin/bash
rm $(dirname "$0")/bin -rf
rm $(dirname "$0")/profile -rf
mkdir -p $(dirname "$0")/profile

out_csv="$(dirname "$0")/task1.csv"
echo "transfer_type,num_dpus,data_size_MB,cpu_dpu_bw_GBs,dpu_cpu_bw_GBs" >"$out_csv"
for transfer in SERIAL PARALLEL BROADCAST; do
  for i in 1 2 4 8 16 32 64; do
    TRANSFER=$transfer NR_DPUS=$i NR_TASKLETS=1 make || exit 1
    for j in 1 24 48; do
      size=$(($j * $i * 128 * 1024)) #type=int64_t
      outfile="$(dirname "$0")/profile/transfer_${transfer,,}_dpus_${i}_tasklets_1_${j}MB.txt"
      ./bin/host_code -w 5 -e 10 -i ${size} >"$outfile"

      cpu_dpu_bw=$(awk -F': ' '/CPU-DPU bandwidth/{ gsub(/\r/, "", $2); print $2}' "$outfile")
      dpu_cpu_bw=$(awk -F': ' '/DPU-CPU bandwidth/{ gsub(/\r/, "", $2); print $2}' "$outfile")

      echo "${transfer,,},$i,$j,$cpu_dpu_bw,$dpu_cpu_bw" >>"$out_csv"
    done
    make clean
  done
done
