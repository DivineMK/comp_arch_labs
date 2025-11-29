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
    "--input-csv",
    "-i",
    default="task3.csv",
    type=str,
    help="Path to the CSV file containing bandwidth data",
)
args = parser.parse_args()

# Load CSV
df = pd.read_csv(args.input_csv)

# Ensure correct types
df["nr_tasklets"] = pd.to_numeric(df["nr_tasklets"])
df["instructions"] = pd.to_numeric(df["instructions"])
df["instructions"] /= 1e6

df_pivoted = df[df["nr_tasklets"] == 16].pivot(
    index="data_type", columns="operation", values="instructions"
)
order_rows = ["CHAR", "SHORT", "INT32", "UINT32", "INT64", "UINT64", "FLOAT", "DOUBLE"]
order_cols = ["ADD", "SUB", "MUL", "DIVIDE"]
df_pivoted.reindex(index=order_rows, columns=order_cols).to_csv("task3_pivoted.csv")
# Check if there's an 'operation' column, otherwise create a dummy one
if "operation" not in df.columns:
    df["operation"] = "default"

# Get all unique data types
data_types = sorted(df["data_type"].unique())

# Define consistent color palette for operations
operations = sorted(df["operation"].unique())
colors = plt.cm.tab10.colors
color_map = {op: colors[i % len(colors)] for i, op in enumerate(operations)}

# Create a separate plot for each data type
for dtype in data_types:
    plt.figure(figsize=(10, 6))

    # Filter data for this data type
    dtype_data = df[df["data_type"] == dtype]

    # Plot each operation
    for operation, group in dtype_data.groupby("operation"):
        plt.plot(
            group["nr_tasklets"],
            group["instructions"],
            marker="o",
            markersize=10,
            linestyle="-",
            linewidth=4,
            label=operation,
            color=color_map[operation],
        )

    plt.xlabel("Number of Tasklets")
    plt.ylabel("Instructions (M)")
    plt.legend(title="Operation", loc="best")
    plt.grid(True, linestyle=":", alpha=0.6)

    # Set log scale on y-axis
    # plt.yscale("log")
    plt.margins(y=0.02)

    # Ensure x-axis shows all tasklet counts
    # plt.xlim(df["nr_tasklets"].min() - 0.5, df["nr_tasklets"].max() + 0.5)

    plt.tight_layout()

    # Save figure
    output_filename = f"instruction_{dtype.lower()}.png"
    plt.savefig(output_filename, dpi=300, bbox_inches="tight")
    print(f"Saved: {output_filename}")

# plt.show()
