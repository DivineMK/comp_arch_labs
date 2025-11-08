import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.ticker import LogLocator, FuncFormatter
import argparse

# Parse command-line arguments
parser = argparse.ArgumentParser(description="Plot DPU bandwidth from CSV")
parser.add_argument(
    "--input-csv",
    "-i",
    default="task1.csv",
    type=str,
    help="Path to the CSV file containing bandwidth data",
)
parser.add_argument(
    "--output-suffix",
    "-o",
    default="bandwidth_combined.png",
    type=str,
    help="Output suffix",
)
args = parser.parse_args()
# Load CSV
df = pd.read_csv(args.input_csv)

# Ensure correct types
df["num_dpus"] = pd.to_numeric(df["num_dpus"])
df["data_size_MB"] = pd.to_numeric(df["data_size_MB"])

# Define consistent color palette by data size
sizes = sorted(df["data_size_MB"].unique())
colors = plt.cm.tab10.colors  # or try: plt.cm.viridis, plt.cm.Set2, etc.
color_map = {size: colors[i % len(colors)] for i, size in enumerate(sizes)}
# One combined plot per transfer type
for transfer in df["transfer_type"].unique():
    subset = df[df["transfer_type"] == transfer]

    plt.figure(figsize=(7, 5))
    for size, group in subset.groupby("data_size_MB"):
        # CPU→DPU bandwidth (solid line)
        plt.plot(
            group["num_dpus"],
            group["cpu_dpu_bw_GBs"],
            marker="o",
            linestyle="-",
            label=f"{size}MB CPU-DPU",
            color=color_map[size],
        )
        # DPU→CPU bandwidth (dashed line)
        plt.plot(
            group["num_dpus"],
            group["dpu_cpu_bw_GBs"],
            marker="s",
            linestyle="--",
            label=f"{size}MB DPU-CPU",
            color=color_map[size],
        )

    plt.xscale("log", base=2)
    ax = plt.gca()
    ax.xaxis.set_major_locator(LogLocator(base=2.0))
    ax.xaxis.set_major_formatter(FuncFormatter(lambda val, pos: f"{int(val)}"))
    plt.xlabel("Number of DPUs")
    plt.ylabel("Bandwidth (GB/s)")
    plt.title(f"{transfer.capitalize()} Transfer Bandwidths")
    plt.legend(title="Direction & Data Size", ncol=2, fontsize=8)
    plt.grid(True, linestyle=":")
    plt.tight_layout()
    plt.savefig(f"{transfer.lower()}_{args.output_suffix}", dpi=300)

plt.show()
