import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.ticker import LogLocator, FuncFormatter, ScalarFormatter
import argparse
import math

# Parse command-line arguments
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
df = pd.read_csv(f"{args.prefix}_task4.csv")

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
            f"{args.prefix}_task4_pivoted_{value}_{size}KB.csv",
            index=False,
            float_format=f"%.{3 if value == 'dpu_time' else 6}f",
        )

# Define consistent color palette for operations
colors = plt.cm.tab10.colors
color_map = {op: colors[i % len(colors)] for i, op in enumerate(sync_methods)}

labels = ["SINGLE", "MUTEX", "BARRIER", "HANDSHAKE"]
# Create a separate plot for each data type
for input_size in input_sizes:
    # Filter data for this data type
    input_size_data = df[df["input_size"] == input_size]

    fig, ax1 = plt.subplots(figsize=(7, 5))
    ax2 = ax1.twinx()

    # Plot each sync method
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

        baseline = group[group["nr_tasklets"] == 1]["instructions"]
        if not baseline.empty:
            baseline_val = baseline.values[0]
            speedup = baseline_val / group["instructions"]
            ax2.plot(
                group["nr_tasklets"],
                speedup,
                marker="x",
                linestyle="--",
                linewidth=2,
                label=f"{sync_method} time",
                color=color_map[sync_method],
            )
    ax1.set_xticks(tasklets)
    ax2.set_yticks(range(1, 17, 2))

    ax1.set_xlabel("Number of Tasklets")
    ax1.set_ylabel("Instructions (K)")
    ax2.set_ylabel("Speedup")
    ax1.legend(title="Sync method", loc="best")
    ax1.grid(True, linestyle=":", alpha=0.6)
    ax1.margins(y=0.02)

    plt.tight_layout()

    # Save figure
    output_filename = f"{args.prefix}_instruction_{input_size}.png"
    plt.savefig(output_filename, dpi=300, bbox_inches="tight")
    print(f"Saved: {output_filename}")
    plt.close()

    if "dpu_time" in df.columns:
        fig, ax1 = plt.subplots(figsize=(7, 5))
        ax2 = ax1.twinx()

        # Plot each sync method
        for i, (sync_method, group) in enumerate(input_size_data.groupby("sync")):
            ax1.plot(
                group["nr_tasklets"],
                group["dpu_time"],
                marker="o",
                markersize=10,
                linestyle="-",
                linewidth=4,
                label=labels[i],
                color=color_map[sync_method],
            )

            baseline = group[group["nr_tasklets"] == 1]["dpu_time"]
            if not baseline.empty:
                baseline_val = baseline.values[0]
                speedup = baseline_val / group["dpu_time"]
                ax2.plot(
                    group["nr_tasklets"],
                    speedup,
                    marker="x",
                    linestyle="--",
                    linewidth=2,
                    label=f"{sync_method} time",
                    color=color_map[sync_method],
                )
        ax1.set_xticks(tasklets)
        ax2.set_yticks(range(1, 17, 2))

        ax1.set_xlabel("Number of Tasklets")
        ax1.set_ylabel("DPU Kernel Time (ms)")
        ax2.set_ylabel("Speedup")
        ax1.legend(title="Sync method", loc="best")
        ax1.grid(True, linestyle=":", alpha=0.6)
        ax1.margins(y=0.02)

        plt.tight_layout()

        # Save figure
        output_filename = f"{args.prefix}_time_{input_size}.png"
        plt.savefig(output_filename, dpi=300, bbox_inches="tight")
        print(f"Saved: {output_filename}")
        plt.close()
