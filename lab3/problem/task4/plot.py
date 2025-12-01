import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.ticker import LogLocator, FuncFormatter, ScalarFormatter
import argparse
import math

# Parse command-line arguments
parser = argparse.ArgumentParser(description="Plot DPU bandwidth from CSV")
parser.add_argument(
    "--input-csv",
    "-i",
    default="task4.csv",
    type=str,
    help="Path to the CSV file containing bandwidth data",
)
args = parser.parse_args()

plt.rcParams.update(
    {
        "font.size": 18,  # default for all text
        "axes.titlesize": 24,  # title font size
        "axes.labelsize": 24,  # x/y label font
        "xtick.labelsize": 20,  # tick label font
        "ytick.labelsize": 20,
        "legend.fontsize": 16,  # legend font
    }
)
# Load CSV
df = pd.read_csv(args.input_csv)

# Ensure correct types
df["nr_tasklets"] = pd.to_numeric(df["nr_tasklets"])
df["instructions"] = pd.to_numeric(df["instructions"])
# df["throughput"] = pd.to_numeric(df["throughput"])
df["instructions"] /= 1e6

# Get all unique data types
sync_methods = ["SINGLE_NO", "SINGLE_MUTEX", "TREE_BARRIER", "TREE_HANDSHAKE"]
input_sizes = sorted(df["input_size"].unique())
tasklets = sorted(df["nr_tasklets"].unique())
for value in ["instructions", "dpu_time"]:
    for size in input_sizes:
        df_pivoted = df[df["input_size"] == size].pivot(
            index="nr_tasklets", columns="sync", values=value
        )
        df_pivoted.reindex(columns=sync_methods).to_csv(
            f"task4_pivoted_{value}_{size}KB.csv"
        )

# Define consistent color palette for operations
colors = plt.cm.tab10.colors
color_map = {op: colors[i % len(colors)] for i, op in enumerate(sync_methods)}

labels = ["SINGLE", "MUTEX", "BARRIER", "HANDSHAKE"]
# Create a separate plot for each data type
for input_size in input_sizes:
    fig, ax1 = plt.subplots(figsize=(7, 5))
    # ax2 = ax1.twinx()

    # Filter data for this data type
    input_size_data = df[df["input_size"] == input_size]

    # Plot each operation
    for i, (sync_method, group) in enumerate(input_size_data.groupby("sync")):
        ax1.plot(
            group["nr_tasklets"],
            group["instructions"],
            marker="o",
            markersize=10,
            linestyle="-",
            linewidth=4,
            label=labels[i],
            color=color_map[sync_method],
        )
        # ax2.plot(
        #     group["nr_tasklets"],
        #     group["dpu_time"],
        #     marker="x",
        #     linestyle="--",
        #     linewidth=2,
        #     label=f"{sync_method} time",
        #     color=color_map[sync_method],
        # )
    # ax1.set_yscale("log")
    # ax1.set_xscale("log")
    ax1.set_xticks(tasklets)

    ax1.set_xlabel("Number of Tasklets")
    ax1.set_ylabel("Instructions (M)")
    ax1.legend(title="Sync method", loc="best")
    ax1.grid(True, linestyle=":", alpha=0.6)
    ax1.margins(y=0.02)

    # ax2.set_ylabel("DPU Time (ms)", color="black")
    # Ensure x-axis shows all tasklet counts
    # plt.xlim(df["nr_tasklets"].min() - 0.5, df["nr_tasklets"].max() + 0.5)

    plt.tight_layout()

    # Save figure
    output_filename = f"instruction_{input_size}.png"
    plt.savefig(output_filename, dpi=300, bbox_inches="tight")
    print(f"Saved: {output_filename}")

# plt.show()
