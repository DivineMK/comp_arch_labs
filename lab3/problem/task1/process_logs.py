import csv
import glob
import re
import argparse

parser = argparse.ArgumentParser(description="Plot DPU bandwidth from CSV")
parser.add_argument(
    "--prefix",
    "-p",
    default="sw",
    # choices=["sw", "hw"],
    type=str,
    help="Prefix (sw/hw)",
)
args = parser.parse_args()

output_file = f"{args.prefix}_task1.csv"
temp_data = []

for filename in glob.glob(f"profile_{args.prefix}/transfer_*.txt"):
    # Example: transfer_broadcast_dpus_16_tasklets_1_1MB.txt
    m = re.match(r".*transfer_(\w+)_dpus_(\d+)_tasklets_\d+_(\d+)MB\.txt", filename)
    if not m:
        print(filename)
        continue
    transfer_type, num_dpus, data_size = m.groups()
    num_dpus = int(num_dpus)

    cpu_dpu_bw = dpu_cpu_bw = None
    with open(filename) as f:
        for line in f:
            if "CPU-DPU bandwidth" in line:
                cpu_dpu_bw = float(re.search(r"([\d.]+)", line).group(1))
            elif "DPU-CPU bandwidth" in line:
                dpu_cpu_bw = float(re.search(r"([\d.]+)", line).group(1))


    if None not in [cpu_dpu_bw, dpu_cpu_bw]:
        temp_data.append(
            {
                "transfer_type": transfer_type,
                "num_dpus": num_dpus,
                "data_size_MB": data_size,
                "cpu_dpu_bw_GBs": cpu_dpu_bw,
                "dpu_cpu_bw_GBs": dpu_cpu_bw,
            }
        )

# Sort data
# Sort by data_type, and number of tasklets
temp_data.sort(key=lambda x: (x["transfer_type"], x["num_dpus"], x["data_size_MB"]))

with open(output_file, "w", newline="") as csvfile:
    fieldnames = [
            "transfer_type",
            "num_dpus",
            "data_size_MB",
            "cpu_dpu_bw_GBs",
            "dpu_cpu_bw_GBs",
    ]
    writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
    writer.writeheader()
    writer.writerows(temp_data)
# Sort by number of DPUs to prevent tangled plots
print(f"CSV saved to {output_file}")
