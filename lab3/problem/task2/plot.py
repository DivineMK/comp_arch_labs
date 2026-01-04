import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.ticker import LogLocator, FuncFormatter
import argparse

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
df = pd.read_csv(f"{args.prefix}_task2.csv")

# Ensure correct types
df["nr_tasklets"] = pd.to_numeric(df["nr_tasklets"])
df["instructions"] = pd.to_numeric(df["instructions"])
df["instructions"] /= 1e6


def main():
    if "dpu_time" in df.columns:
        baseline = df[df["nr_tasklets"] == 1].set_index("data_type")["dpu_time"]
        df["speedup"] = baseline[df["data_type"]].to_numpy() / df["dpu_time"]
        df.to_csv(f"{args.prefix}_task2_spd.csv", index=False)
    else:
        print("Missing dpu_time column")
        return

    df_pivoted = df.pivot(
        index="nr_tasklets", columns="data_type", values="instructions"
    )
    order = ["CHAR", "SHORT", "INT32", "UINT32", "INT64", "UINT64", "FLOAT", "DOUBLE"]
    df_pivoted[order].to_csv(f"{args.prefix}_task2_pivoted.csv")
    # Get all unique data types
    data_types = sorted(df["data_type"].unique())
    # group data types to plots
    colors = plt.cm.tab10.colors
    color_map = {op: colors[i % len(colors)] for i, op in enumerate(data_types)}

    # Create a separate plot for each data type
    fig_inst, ax_inst = plt.subplots(figsize=(10, 6))
    fig_spd, ax_spd = plt.subplots(figsize=(10, 6))

    for dtype in data_types:
        dtype_data = df[df["data_type"] == dtype]

        if dtype_data.empty:
            print(f"Missing data for {dtype}")
            continue

        ax_inst.plot(
            dtype_data["nr_tasklets"],
            dtype_data["instructions"],
            marker="o",
            markersize=6,
            linestyle="-",
            linewidth=2,
            label=dtype,
            color=color_map[dtype],
        )
        ax_spd.plot(
            dtype_data["nr_tasklets"],
            dtype_data["speedup"],
            marker="o",
            markersize=6,
            linestyle="-",
            linewidth=2,
            label=dtype,
            color=color_map[dtype],
        )

    ax_inst.set_ylabel("Instructions (M)")
    ax_spd.set_ylabel("Speedup")

    for ax in (ax_inst, ax_spd):
        ax.set_xlabel("Number of Tasklets")
        ax.legend(title="Data types", loc="best")
        ax.grid(True, linestyle=":", alpha=0.6)
        ax.margins(y=0.02, x=0.02)
        ax.set_xticks(range(1, 25, 2))

    if args.prefix == "sw":
        ax_spd.set_yticks(range(1, 25, 2))
    else:
        ax_spd.set_yticks(range(1, 13, 2))
    fig_inst.savefig(f"{args.prefix}_instruction.png", dpi=300, bbox_inches="tight")
    fig_spd.savefig(f"{args.prefix}_speedup.png", dpi=300, bbox_inches="tight")


if __name__ == "__main__":
    main()
