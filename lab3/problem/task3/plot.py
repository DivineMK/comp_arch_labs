import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.ticker import LogLocator, FuncFormatter
import argparse
import math

plt.rcParams.update(
    {
        "font.size": 20,  # default for all text
        "axes.titlesize": 28,  # title font size
        "axes.labelsize": 28,  # x/y label font
        "xtick.labelsize": 20,  # tick label font
        "ytick.labelsize": 20,
        "legend.fontsize": 20,  # legend font
        "legend.title_fontsize": 22,  # legend font
    }
)
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

# Load CSV
df = pd.read_csv(f"{args.prefix}_task3.csv")

# Ensure correct types
df["nr_tasklets"] = pd.to_numeric(df["nr_tasklets"])
df["instructions"] = pd.to_numeric(df["instructions"])
df["instructions"] /= 1e6

df_pivoted = df[df["nr_tasklets"] == 16].pivot(
    index="data_type", columns="operation", values="instructions"
)
order_rows = ["CHAR", "SHORT", "INT32", "UINT32", "INT64", "UINT64", "FLOAT", "DOUBLE"]
order_cols = ["ADD", "SUB", "MUL", "DIVIDE"]
df_pivoted.reindex(index=order_rows, columns=order_cols).to_csv(
    f"{args.prefix}_task3_pivoted.csv",
    float_format="%.6f",
)
# Check if there's an 'operation' column, otherwise create a dummy one
if "operation" not in df.columns:
    df["operation"] = "default"

# Get all unique data types
data_types = sorted(df["data_type"].unique())

# Define consistent color palette for operations
operations = sorted(df["operation"].unique())
nr_tasklets = sorted(df["nr_tasklets"].unique())
colors = plt.cm.tab10.colors
color_map = {op: colors[i % len(colors)] for i, op in enumerate(operations)}

# Create a separate plot for each data type
for dtype in data_types:
    # --- Plot Instructions ---
    fig, ax1 = plt.subplots(figsize=(10, 6))
    ax2 = ax1.twinx()

    # Filter data for this data type
    dtype_data = df[df["data_type"] == dtype]

    # Plot each operation
    for operation, group in dtype_data.groupby("operation"):
        # Plot Instructions on ax1 (Solid)
        ax1.plot(
            group["nr_tasklets"],
            group["instructions"],
            marker="o",
            markersize=10,
            linestyle="-",
            linewidth=4,
            label=f"{operation}",
            color=color_map[operation],
        )

        # Calculate Speedup for Instructions (wrt 1 tasklet)
        baseline = group[group["nr_tasklets"] == 1]["instructions"]
        if not baseline.empty:
            baseline_val = baseline.values[0]
            speedup = baseline_val / group["instructions"]

            # Plot Speedup on ax2 (Dashed)
            ax2.plot(
                group["nr_tasklets"],
                speedup,
                marker="x",
                markersize=8,
                linestyle="--",
                linewidth=2,
                label=f"{operation} Speedup",
                color=color_map[operation],
                alpha=0.7,
            )

    ax1.set_xlabel("# Tasklets")
    ax1.set_ylabel("Instructions (M)")
    ax2.set_ylabel("Speedup")
    ax1.set_xticks(nr_tasklets)
    ax2.set_yticks(range(1, 17, 2))

    # Combine legends
    lines1, labels1 = ax1.get_legend_handles_labels()
    lines2, labels2 = ax2.get_legend_handles_labels()
    # ax1.legend(lines1 + lines2, labels1 + labels2, title="Operation", loc="best")
    # Legend might be too big, just keep operations
    ax1.legend(title="Operation", loc="upper left")

    ax1.grid(True, linestyle=":", alpha=0.6)
    fig.tight_layout()

    # Save figure
    output_filename = f"{args.prefix}_instruction_{dtype.lower()}.png"
    plt.savefig(output_filename, dpi=300, bbox_inches="tight")
    print(f"Saved: {output_filename}")
    plt.close()

    # --- Plot DPU Time ---
    if "dpu_time" in df.columns:
        fig, ax1 = plt.subplots(figsize=(10, 6))
        ax2 = ax1.twinx()

        # Plot each operation
        for operation, group in dtype_data.groupby("operation"):
            # Plot Time on ax1 (Solid)
            ax1.plot(
                group["nr_tasklets"],
                group["dpu_time"],
                marker="o",
                markersize=10,
                linestyle="-",
                linewidth=4,
                label=f"{operation}",
                color=color_map[operation],
            )

            # Calculate Speedup for Time (wrt 1 tasklet)
            baseline = group[group["nr_tasklets"] == 1]["dpu_time"]
            if not baseline.empty:
                baseline_val = baseline.values[0]
                speedup = baseline_val / group["dpu_time"]

                # Plot Speedup on ax2 (Dashed)
                ax2.plot(
                    group["nr_tasklets"],
                    speedup,
                    marker="x",
                    markersize=8,
                    linestyle="--",
                    linewidth=2,
                    label=f"{operation} Speedup",
                    color=color_map[operation],
                    alpha=0.7,
                )

        ax1.set_xlabel("# Tasklets")
        ax1.set_ylabel("DPU Kernel Time (ms)")
        ax2.set_ylabel("Speedup")

        # Fix xticks: use log scale and integer formatting
        # ax1.set_xscale("log", base=2)
        # ax1.xaxis.set_major_locator(LogLocator(base=2.0))
        # ax1.xaxis.set_major_formatter(FuncFormatter(lambda val, pos: f"{int(val)}"))
        ax1.set_xticks(nr_tasklets)
        ax2.set_yticks(range(1, 17, 2))

        ax1.legend(
            title="Operation", loc="upper right"
        )  # Time goes down, so upper right is free? No, Time goes down.
        # Speedup goes up.

        ax1.grid(True, linestyle=":", alpha=0.6)
        fig.tight_layout()

        output_filename = f"{args.prefix}_time_{dtype.lower()}.png"
        plt.savefig(output_filename, dpi=300, bbox_inches="tight")
        print(f"Saved: {output_filename}")
        plt.close()
# plt.show()
