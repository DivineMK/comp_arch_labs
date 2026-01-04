#!/bin/bash
DIR="$(dirname "$0")"
# rm -rf "$(dirname $0)"/objdump/
mkdir -p ${DIR}/objdump/
C_FLAGS="${C_FLAGS:-O2}"
SUFFIX="${SUFFIX:-noopt}"
for tasklet in 1; do
  for gs in LIG AVG LUM; do
    DPU_C_FLAGS="-$C_FLAGS" PERF=INSTRUCTIONS GS=${gs} NR_DPUS=1 NR_TASKLETS=${tasklet} NR_ELEM=360 make && llvm-objdump -S ${DIR}/bin/dpu_code >"${DIR}/objdump/objdump_${gs}_tasklet_${tasklet}_flags_${C_FLAGS}_${SUFFIX}.S"
    make clean
  done
done
