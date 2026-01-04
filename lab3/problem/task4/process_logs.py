import csv
import glob
import os
import re
import argparse

parser = argparse.ArgumentParser(description="Plot DPU bandwidth from CSV")
parser.add_argument(
    "--prefix",
    "-p",
    default="sw",
    choices=["sw", "hw"],
    type=str,
    help="Prefix (sw/hw)",
)
args = parser.parse_args()

output_file = f"{args.prefix}_task4.csv"
temp_data = []

# Collect data
for filename in glob.glob(f"profile_{args.prefix}/*.txt"):
    # Example: sync_type_single_mutex_tasklets_16_1024KB.txt
    # nr_dpus,nr_tasklets,data_type,sync,input_size,instructions,dpu_time
    basename = os.path.basename(filename)
    m = re.match(r"sync_type_(\w+)_tasklets_(\d+)_(\d+)KB\.txt", basename)
    if not m:
        print(f"Skipping {filename}")
        continue
    sync, nr_tasklets, input_size = m.groups()
    nr_tasklets = int(nr_tasklets)
    input_size = int(input_size)
    sync = sync.upper()

    dpu_time = instructions = nr_dpus = None

    with open(filename) as f:
        content = f.read()

        # Extract DPU time
        # CPU Time (ms): 2.956000   CPU-DPU Time (ms): 336.571000   DPU Kernel Time (ms): 1284.966000
        m_dpu = re.search(r"DPU Kernel Time \(ms\): ([\d.]+)", content)
        if m_dpu:
            dpu_time = float(m_dpu.group(1))

        # Extract DPU instructions
        m_instr = re.search(r"DPU instructions\s*=\s*max:\s*(\d+)", content)
        if m_instr:
            instructions = int(m_instr.group(1))

        # Extract Arithmetic throughput
        m_thru = re.search(r"Arithmetic throughput\s*=\s*([\d.]+)", content)
        if m_thru:
            throughput = float(m_thru.group(1))

        # Extract nr_dpus
        m_dpus = re.search(r"Allocated (\d+) DPU", content)
        if m_dpus:
            nr_dpus = int(m_dpus.group(1))

    if None not in [dpu_time, instructions]:
        temp_data.append(
            {
                "sync": sync,
                "input_size": input_size,
                "nr_tasklets": nr_tasklets,
                "nr_dpus": nr_dpus,
                "instructions": instructions,
                "dpu_time": dpu_time,
            }
        )

# Sort data
# Sort by operation, data_type, and number of tasklets
temp_data.sort(key=lambda x: (x["sync"], x["nr_tasklets"], x["input_size"]))

# Write to CSV
with open(output_file, "w", newline="") as csvfile:
    fieldnames = [
        "sync",
        "nr_tasklets",
        "input_size",
        "nr_dpus",
        "instructions",
        "dpu_time",
    ]
    writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
    writer.writeheader()
    writer.writerows(temp_data)

print(f"CSV saved to {output_file}")
