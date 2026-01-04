import csv
import glob
import re
import pandas as pd

output_file = "hw_task1.csv"

with open(output_file, "w", newline="") as csvfile:
    writer = csv.writer(csvfile)
    writer.writerow(
        [
            "transfer_type",
            "num_dpus",
            "data_size_MB",
            "cpu_dpu_bw_GBs",
            "dpu_cpu_bw_GBs",
        ]
    )

    for filename in glob.glob("profile_hw/transfer_*.txt"):
        # Example: transfer_broadcast_dpus_16_tasklets_1_1MB.txt
        m = re.match(r".*transfer_(\w+)_dpus_(\d+)_tasklets_\d+_(\d+)MB\.txt", filename)
        if not m:
            print(filename)
            continue
        transfer_type, num_dpus, data_size = m.groups()

        cpu_dpu_bw = dpu_cpu_bw = None
        with open(filename) as f:
            for line in f:
                if "CPU-DPU bandwidth" in line:
                    cpu_dpu_bw = float(re.search(r"([\d.]+)", line).group(1))
                elif "DPU-CPU bandwidth" in line:
                    dpu_cpu_bw = float(re.search(r"([\d.]+)", line).group(1))

        if cpu_dpu_bw is not None and dpu_cpu_bw is not None:
            writer.writerow(
                [transfer_type, num_dpus, data_size, cpu_dpu_bw, dpu_cpu_bw]
            )

df = pd.read_csv(output_file)
# Sort by number of DPUs to prevent tangled plots
df = df.sort_values(by="num_dpus")
df.to_csv(output_file, index=False)
print(f"CSV saved to {output_file}")
