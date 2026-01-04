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

output_file = f"{args.prefix}_bonus.csv"
temp_data = []

# Collect data
for filename in glob.glob(f"profile_{args.prefix}/*.txt"):
    # Example: dpus_1_${gs,,}_tl_${tasklet}_nr_pixels_$((i * 131072)).txt"
    # nr_dpus,grayscale,nr_tasklets,nr_pixels,instructions,dpu_time
    basename = os.path.basename(filename)
    m = re.match(r"dpus_1_(\w+)_tl_(\d+)_nr_pixels_(\d+)\.txt", basename)
    if not m:
        print(f"Skipping {filename}")
        continue
    grayscale, nr_tasklets, nr_pixels = m.groups()
    nr_tasklets = int(nr_tasklets)
    nr_pixels = int(nr_pixels)
    grayscale = grayscale.upper()

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
        # m_thru = re.search(r"Arithmetic throughput\s*=\s*([\d.]+)", content)
        # if m_thru:
        #     throughput = float(m_thru.group(1))

        # Extract nr_dpus
        m_dpus = re.search(r"Allocated (\d+) DPU", content)
        if m_dpus:
            nr_dpus = int(m_dpus.group(1))

    if None not in [dpu_time, instructions]:
        temp_data.append(
            {
                "grayscale": grayscale,
                "nr_pixels": nr_pixels,
                "nr_tasklets": nr_tasklets,
                "nr_dpus": nr_dpus,
                "instructions": instructions,
                "dpu_time": dpu_time,
            }
        )

# Sort data
# Sort by operation, data_type, and number of tasklets
temp_data.sort(key=lambda x: (x["grayscale"], x["nr_tasklets"], x["nr_pixels"]))

# Write to CSV
with open(output_file, "w", newline="") as csvfile:
    fieldnames = [
        "grayscale",
        "nr_tasklets",
        "nr_pixels",
        "nr_dpus",
        "instructions",
        "dpu_time",
    ]
    writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
    writer.writeheader()
    writer.writerows(temp_data)

print(f"CSV saved to {output_file}")
