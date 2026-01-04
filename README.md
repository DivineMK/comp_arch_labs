# comp_arch_labs
HS2025

## Lab 1 — Simulating and Exploring Cache Behavior

- Implemented a 4-way set-associative instruction cache (8 KB) and an 8-way data cache (64 KB) with LRU replacement in a MIPS timing simulator
- Explored the effect of cache size, block size, associativity, and alternative replacement/insertion policies on IPC

## Lab 2 — Memory Hierarchy (L2 Cache + DRAM Controller)

- Extended the simulator with a unified 16-way L2 cache (256 KB) with MSHRs and true LRU replacement
- Implemented a cycle-accurate DRAM controller modeling a single-rank, 8-bank system with row buffers, ACTIVATE/READ/WRITE/PRECHARGE timing, FR-FCFS scheduling, and an open-row policy

## Lab 3 — Programming a Real Processing-in-Memory Architecture

- Benchmarked host-to-PIM data transfers (serial, parallel, broadcast) across varying DPU counts and transfer sizes
- Implemented AXPY on UPMEM DPUs, analyzing instruction counts across 1–24 tasklets
- Evaluated vector addition/subtraction/multiplication/division across six data types (char, short, int, long long, float, double)
- Compared synchronization strategies (single tasklet, barrier tree, handshake tree, mutex) for parallel vector reduction
- (Bonus) Ran workloads on real UPMEM hardware and implemented RGB-to-grayscale conversion

## Lab 4 — Hardware Data Prefetcher Design and Analysis

- Implemented a GHB-based stride prefetcher (PC/CS scheme, 256-entry IT and GHB) in the ChampSim simulator
- Evaluated the prefetcher under both full and limited main memory bandwidth across 14 traces
- Designed a system-aware prefetcher that dynamically adjusts prefetch degree based on epoch-wise accuracy and DRAM bandwidth usage
- Designed a custom prefetcher with optional comparison against Pythia, a state-of-the-art RL-based prefetcher
