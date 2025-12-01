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
    default="bandwidth_combined",
    type=str,
    help="Output suffix",
)
args = parser.parse_args()
# Load CSV
df = pd.read_csv(args.input_csv)

plt.rcParams.update(
    {
        "font.size": 16,  # default for all text
        "axes.titlesize": 20,  # title font size
        "axes.labelsize": 20,  # x/y label font
        "xtick.labelsize": 18,  # tick label font
        "ytick.labelsize": 18,
        "legend.fontsize": 14,  # legend font
    }
)

# Ensure correct types
df["num_dpus"] = pd.to_numeric(df["num_dpus"])
df["data_size_MB"] = pd.to_numeric(df["data_size_MB"])

y_min = min(df["cpu_dpu_bw_GBs"].min(), df["dpu_cpu_bw_GBs"].min())
y_max = max(df["cpu_dpu_bw_GBs"].max(), df["dpu_cpu_bw_GBs"].max())
# Define consistent color palette by data size
transfer_types = sorted(df["transfer_type"].unique())
sizes = sorted(df["data_size_MB"].unique())
colors = plt.cm.tab10.colors  # or try: plt.cm.viridis, plt.cm.Set2, etc.
color_map_size = {size: colors[i % len(colors)] for i, size in enumerate(sizes)}
color_map_transfer = {
    transfer_type: colors[i % len(colors)]
    for i, transfer_type in enumerate(transfer_types)
}
# One combined plot per transfer type
for transfer in transfer_types:
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
            color=color_map_size[size],
        )
        # DPU→CPU bandwidth (dashed line)
        plt.plot(
            group["num_dpus"],
            group["dpu_cpu_bw_GBs"],
            marker="s",
            linestyle="--",
            label=f"{size}MB DPU-CPU",
            color=color_map_size[size],
        )

    plt.xscale("log", base=2)
    plt.yscale("log", base=2)
    plt.ylim(y_min * 0.7, y_max * 1.3)
    ax = plt.gca()
    ax.xaxis.set_major_locator(LogLocator(base=2.0))
    ax.xaxis.set_major_formatter(FuncFormatter(lambda val, pos: f"{int(val)}"))
    ax.yaxis.set_major_locator(LogLocator(base=2.0))
    ax.yaxis.set_major_formatter(FuncFormatter(lambda val, pos: f"{int(val)}"))
    plt.xlabel("Number of DPUs")
    plt.ylabel("Bandwidth (GB/s, log scale)")
    # plt.title(f"{transfer.capitalize()} Transfer Bandwidths")
    plt.legend(title="Direction & Data Size", ncol=2)
    plt.grid(True, linestyle=":")
    plt.tight_layout()
    plt.savefig(f"{transfer.lower()}_{args.output_suffix}", dpi=300)

df_combined = df[df["data_size_MB"] == 48]
plt.figure(figsize=(7, 5))
for transfer, group in df_combined.groupby("transfer_type"):
    # CPU→DPU bandwidth (solid line)
    plt.plot(
        group["num_dpus"],
        group["cpu_dpu_bw_GBs"],
        marker="o",
        linestyle="-",
        label=f"{transfer} CPU-DPU",
        color=color_map_transfer[transfer],
    )
    # DPU→CPU bandwidth (dashed line)
    plt.plot(
        group["num_dpus"],
        group["dpu_cpu_bw_GBs"],
        marker="s",
        linestyle="--",
        label=f"{transfer} DPU-CPU",
        color=color_map_transfer[transfer],
    )

plt.xscale("log", base=2)
plt.yscale("log", base=2)
ax = plt.gca()
ax.xaxis.set_major_locator(LogLocator(base=2.0))
ax.xaxis.set_major_formatter(FuncFormatter(lambda val, pos: f"{int(val)}"))
ax.yaxis.set_major_locator(LogLocator(base=2.0))
ax.yaxis.set_major_formatter(FuncFormatter(lambda val, pos: f"{int(val)}"))
plt.xlabel("Number of DPUs")
plt.ylabel("Bandwidth (GB/s, log scale)")
plt.legend(title="Transfer type & Direction", ncol=2)
plt.grid(True, linestyle=":")
plt.tight_layout()
plt.savefig(f"{args.output_suffix}.png", dpi=300)

# plt.show()

df_sustained = df[df["data_size_MB"] != 1]
print(
    f"""
    Serial transfer CPU-DPU max {df_sustained[df_sustained["transfer_type"] == "serial"]["cpu_dpu_bw_GBs"].max()}, min {df_sustained[df_sustained["transfer_type"] == "serial"]["cpu_dpu_bw_GBs"].min()}
    Serial transfer DPU-CPU max {df_sustained[df_sustained["transfer_type"] == "serial"]["dpu_cpu_bw_GBs"].max()}, min {df_sustained[df_sustained["transfer_type"] == "serial"]["dpu_cpu_bw_GBs"].min()}
    """
)
