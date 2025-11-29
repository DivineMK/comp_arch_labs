#import "@preview/subpar:0.2.2"

#set document(
  title: [*Lab 3: Programming a Real Processing-in-Memory Architecture*],
)
#set heading(numbering: "1.")
#set page(numbering: "1")

#show title: set align(center)
#show link: set text(fill: blue)

#title()
= Introduction

= Task 1/4: Transferring Data between Main Memory and PIM-enabled Memory

In this task, data transfer from main memory to MRAM (DPU memory) is evaluated with 3 types of data transfer: serial, parallel and broadcast; 3 data transfer sizes *per DPU*: 1MB, 24MB and 48MB, and different numbers of DPUs: 1, 2, 4, 8, 16, 32, 64.

The obtained plots for every transfer type are provided below:
#subpar.grid(
  columns: 3,
  figure(
    image("problem/task1/serial_bandwidth_combined.png"),
    caption: [Serial transfer],
  ),
  <serial_transfer>,
  figure(
    image("problem/task1/parallel_bandwidth_combined.png"),
    caption: [Parallel transfer],
  ),

  <parallel_transfer>,

  figure(
    image("problem/task1/broadcast_bandwidth_combined.png"),
    caption: [Broadcast transfer],
  ),
  <broadcast_transfer>,

  grid.cell(
    colspan: 3,
    figure(
      image("problem/task1/bandwidth_combined.png", width: 100% / 3),
      caption: [Transfer type comparison with 48MB/DPU],
    ),
  ),
  caption: [Task 1 plots],
)

For large data transfer size of 24 and 48MB, the benefits of additional DPUs are apparent in both data direction for parallel and broadcast transfer. Serial transfer remains relatively constant as this transfer type does not rely on parallelism. For both parallel and broadcast transfer, bandwidth seem to scale sublinearly with respect to the number of DPUs.

Low data transfer size such as 1MB per DPU, the bandwidth are always higher than the maximum bandwidth based on the DRAM standard. In addition, for serial transfer, the bandwidths are reduced with increasing number of DPUs. Both of these observations might be explained by the possibility that such small transfer size fits in the cache of the UPMEM system, causing the increase in number of DPUs to not improve performance, but only raise communication delays with the DPUs.

Note that in @broadcast_transfer, the lines for data direction from DPU to CPU actually uses parallel transfer. They are included to show the significant differences with @parallel_transfer at 1MB data transfer size for 1, 2 and 4 DPUs, which remains unresolved. In addition, these analyses might indicate that the simulator does not provide accurate timing measurements as some measurements are higher than the theoretical limit of DDR4-2400 DIMMs channel bandwidth (19.2 GB/s). This is because the #link("https://sdk.upmem.com/2025.1.0/06_ControllingDPUFromHost.html#obtaining-dpus")[UPMEM simulator can only use 1 rank].

= Task 2/4: AXPY

In this task, AXPY operation instruction count is measured across different data types with (NR_DPUS=1).
// #let task2 = csv("problem/task2/task2_pivoted.csv")
//
// #figure(
//   table(
//     columns: task2.first().len(),
//     ..task2.flatten(),
//   ),
//   caption: [Task 2 results, number of instructions in unit of 1e6 (NR_DPUS=1)],
// ) <task2>

#subpar.grid(
  columns: 2,
  figure(
    image("problem/task2/instruction.png"),
    caption: [Instruction count (in unit of 1e6)],
  ),
  <task2_inst>,

  figure(
    image("problem/task2/speedup.png"),
    caption: [Speedup from NR_TASKLETS=1],
  ),

  <task2_spd>,
  caption: [Results with respect to different NR_TASKLETS],
)

Regarding differences between data types, for CHAR, SHORT, INT32 and UINT32, it can be seen that the number of instructions required scales with respect to the data size, with data size of 32 bits requiring the least amount of instructions as 32 bits is the data granularity of UPMEM assembly operations. However, INT64, UINT64 have higher instruction count, which is explained by #link("https://sdk.upmem.com/2025.1.0/fff_CodingTips.html#bit-variables-are-expensive")[UPMEM recommended practices]. #link("https://sdk.upmem.com/2025.1.0/fff_CodingTips.html#floating-point-support")[Floating point operations requires software emulation], hence require even higher instruction count.

Regarding speedup with respect to NR_TASKLETS=1, every data types seems to follow the same linear scale with slope of 1. Note that the instruction is obtained from the maximum value of performance counter across tasklets for each DPU (provided from resources). This maximum value seem to approximately represents the total number of instructions required for *all* tasklets, which can be explained if the counter_start and counter_stop calls of every tasklet occurs in sync, probably due to the concurrent nature of the simulator. This simulator discrepancy from hardware also explains the speedup mismatch which does not saturate at NR_TASKLETS=11 due to maximizing DPU pipeline as observed in Gómez-Luna et al., 2022.

It is important to note that in this task, using default build parameters fails due to WRAM overflow (with NR_TASKLETS >= 22), hence a #link("https://sdk.upmem.com/2025.1.0/030_DPURuntimeService_Tasklets.html#tasklets")[default stack size] is set with stack_size_default=200. In addition, unlike task 1, the results produced with performance counters are consistent across runs. clock frequency of dpus (#link("https://sdk.upmem.com/2025.1.0/071_MeasuringPerformances.html#dpu-pmc")[CLOCKS_PER_SEC] is consistent across runs at 500MHz.

= Task 3/4: Operations and Data Types

In this task, instruction count is obtained with respect to arithmetic operations (add, subtract, multiply and divide) and data types. Only results for 1, 2, 4, 8 and 16 tasklets are measured.

#subpar.grid(
  columns: 3,
  figure(
    image("problem/task3/instruction_char.png"),
    caption: [CHAR],
  ),
  <task3_char>,
  figure(
    image("problem/task3/instruction_short.png"),
    caption: [SHORT],
  ),

  <task3_short>,
  figure(
    image("problem/task3/instruction_int32.png"),
    caption: [INT32],
  ),
  <task3_int32>,

  figure(
    image("problem/task3/instruction_uint32.png"),
    caption: [UINT32],
  ),
  <task3_uint32>,
  figure(
    image("problem/task3/instruction_int64.png"),
    caption: [INT64],
  ),

  <task3_int64>,
  figure(
    image("problem/task3/instruction_int64.png"),
    caption: [INT64],
  ),
  <task3_int64>,

  figure(
    image("problem/task3/instruction_float.png"),
    caption: [FLOAT],
  ),
  <task3_float>,

  figure(
    image("problem/task3/instruction_double.png"),
    caption: [DOUBLE],
  ),
  <task3_double>,

  caption: [Results with respect to different NR_TASKLETS],
  label: <task3_fig>,
)

@task3_table are provided to show better quantitative results.
#let task3 = csv("problem/task3/task3_pivoted.csv")

#figure(
  table(
    columns: task3.first().len(),
    ..task3.flatten(),
  ),
  caption: [Instruction count (in unit of 1e6, NR_DPUS=1, NR_TASKLETS=16)],
) <task3_table>

In order to understand the results from @task3_table and @task3_fig, an *estimated* instruction count for every data block transferred to WRAM is presented in @operation_count.
#figure(
  table(
    columns: 5,
    align: (left,) * 5,
    table.header[Data type][ADD][SUB][MUL][DIVIDE],
    [CHAR], $32$, $32$, $32$, $24 + 8*36 ("div32")$,
    [SHORT], $32$, $32$, $81$, $24 + 8*36 ("div32")$,
    [INT32], $32$, $32$, $24 + 8*32 ("mulsi3")$, $24 + 8*36 ("div32")$,
    [UINT32], $32$, $32$, $24 + 8*32 ("mulsi3")$, $24 + 8*36 ("div32")$,
    [INT64], $32 + 8$, $32 + 8$, $24 + 8*122 ("muldi3")$, $24 + 8*54 ("div64")$,
    [UINT64], $32 + 8$, $32 + 8$, $24 + 8*122 ("muldi3")$, $24 + 8*54 ("div64")$,

    [FLOAT], $24 + 8*124$, $32 + 8$, $24 + 8*(94 + 122) ("mulsf3")$, $24 + 8*(129 + 7*122) ("divsf3")$,

    [DOUBLE], $24 + 8*252$, $24 + 8*252$, $24 + 8*(266 + 4*122) ("muldf3")$, $24 + 8*(288 + 16*122) ("divsf3")$,
  ),
  caption: [Instruction count per data block for different data types and operations],
) <operation_count>

Similar to task 2, speedup scales linearly with NR_TASKLETS.

= Task 4/4: Vector Reduction
In this task, instruction count for different methods of vector reduction is measured. SINGLE_NO represents final reduction with 1 single tasklet, SINGLE_MUTEX represents final reduction with every tasklets sequentially using mutex, TREE_BARRIER represents final tree-based reduction with barriers and TREE_HANDSHAKE represents final tree-based reduction with handshakes. The plots are provided below in @task4. @task4_table is also provided for better quantitative evaluations.

#let sizes = (16, 1024, 16384)
#let sizes_label = ("16KB", "1MB", "16MB")
#let files_inst = (
  for size in sizes {
    (csv("problem/task4/" + "task4_pivoted_instructions_" + str(size) + "KB.csv"),)
  }
)
#let files_time = (
  for size in sizes {
    (csv("problem/task4/" + "task4_pivoted_dpu_time_" + str(size) + "KB.csv"),)
  }
)
#let task4_table(files, lbl) = {
  let merged-data-rows = ()
  let original-header = files.first().first()
  for (file, size_label) in files.zip(sizes_label) {
    let data-rows = file.slice(1)

    // Loop through the data rows for the current size block
    for (i, row) in data-rows.enumerate() {
      if i == 0 {
        let size-cell = table.cell(
          rowspan: data-rows.len(),
          align: horizon,
          size_label,
        )
        merged-data-rows.push((size-cell,) + row)
      } else {
        merged-data-rows.push(row)
      }
    }
  }
  let new-header = ("",) + original-header
  let final-table-content = merged-data-rows
  [#figure(
      table(
        // Set column configuration
        columns: new-header.len(),
        table.header(..new-header),

        // Unpack all the rows into the table function
        ..final-table-content.flatten(),
      ),
      caption: [Task 4 results, instruction count in unit of 1e6],
    ) #label(lbl)
  ]
}
#task4_table(files_inst, "task4_table")

#subpar.grid(
  columns: 3,
  figure(
    image("problem/task4/instruction_16.png"),
    caption: [16kB],
  ),
  figure(
    image("problem/task4/instruction_1024.png"),
    caption: [1MB],
  ),

  figure(
    image("problem/task4/instruction_16384.png"),
    caption: [16MB],
  ),

  label: <task4>,
  caption: [Task 4 results, instruction count in unit of 1e6],
)
From @task4_table, we can see that SINGLE_NO version is optimal for all data sizes, which can be explained by the overhead of synchronization. The overhead of synchronization is much less significant at larger data sizes where the result calculation step dominates. From results of data size 16KB, barrier introduces very high overhead (45%) compared to handshake (2.8%) and mutex (0.25%). As a sanity check, this order of overhead holds true for all data sizes.
