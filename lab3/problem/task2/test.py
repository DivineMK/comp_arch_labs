import csv
import glob
import re

output_file = "task2.csv"

with open(output_file, "w", newline="") as csvfile:
    writer = csv.writer(csvfile)
    writer.writerow(
        ["nr_dpus", "nr_tasklets", "data_type", "instructions", "throughput"]
    )

    for filename in glob.glob("profile/dpus_*.txt"):
        # Example: dpus_1_datatype_char_tl1.txt
        m = re.match(r"profile/dpus_(\d+)_datatype_(\w+)_tl(\d+)\.txt", filename)
        if not m:
            print(filename)
            continue
        nr_dpus, data_type, nr_tasklets = m.groups()

        instructions = throughput = None
        with open(filename) as f:
            for line in f:
                if "instructions" in line:
                    instructions = int(float(re.search(r"([\de.+]+)", line).group(1)))
                if "throughput" in line:
                    throughput = float(re.search(r"([\d.]+)", line).group(1))

        if instructions is not None:
            writer.writerow(
                [nr_dpus, nr_tasklets, data_type.upper(), instructions, throughput]
            )

print(f"CSV saved to {output_file}")
