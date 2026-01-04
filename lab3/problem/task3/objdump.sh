#!/bin/bash
mkdir -p $(dirname $0)/objdump/
for type in CHAR; do
  for op in ADD SUB MUL DIVIDE; do
    for tasklet in {1..24}; do
      PRINT=1 PERF=INSTRUCTIONS NR_DPUS=1 NR_TASKLETS=${tasklet} BLOCK=10 OP=$op TYPE=$type make && llvm-objdump -S $(dirname $0)/bin/dpu_code >"$(dirname $0)/objdump/objdump_${type,,}_${op,,}_tasklet_${tasklet}.S"
    done
  done
done
