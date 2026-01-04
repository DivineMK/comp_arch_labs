import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.ticker import LogLocator, FuncFormatter
import argparse

# Parse command-line arguments
parser = argparse.ArgumentParser(description="Plot DPU bandwidth from CSV")
parser.add_argument(
    "--prefix",
    "-p",
    default="sw",
    # choices=["sw", "hw"],
    type=str,
    help="Prefix (sw/hw)",
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
df = pd.read_csv(f"{args.prefix}_task1.csv")

plt.rcParams.update(
    {
        "font.size": 16,  # default for all text
        "axes.titlesize": 20,  # title font size
        "axes.labelsize": 20,  # x/y label font
        "xtick.labelsize": 16,  # tick label font
        "ytick.labelsize": 16,
        "legend.fontsize": 14,  # legend font
    }
)

# Ensure correct types
df["num_dpus"] = pd.to_numeric(df["num_dpus"])
df["data_size_MB"] = pd.to_numeric(df["data_size_MB"])

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

    # y_min = min(subset["cpu_dpu_bw_GBs"].min(), subset["dpu_cpu_bw_GBs"].min())
    # y_max = max(subset["cpu_dpu_bw_GBs"].max(), subset["dpu_cpu_bw_GBs"].max())
    # plt.ylim(y_min * 0.8, y_max * 1.2)
    plt.xscale("log", base=2.0)
    plt.yscale("log", base=2.0)
    ax = plt.gca()
    ax.xaxis.set_major_locator(LogLocator(base=2.0))
    ax.xaxis.set_major_formatter(FuncFormatter(lambda val, pos: f"{int(val)}"))
    ax.yaxis.set_major_locator(LogLocator(base=2.0))
    ax.yaxis.set_major_formatter(FuncFormatter(lambda val, pos: f"{val:.2f}"))

    plt.margins(x=0.1, y=0.1)
    plt.xlabel("Number of DPUs")
    plt.ylabel("Bandwidth (GB/s, log scale)")
    plt.legend(title="Direction & Data Size", ncol=2)
    plt.grid(True, linestyle=":")
    plt.tight_layout()
    plt.savefig(f"{args.prefix}_{transfer.lower()}_{args.output_suffix}.png", dpi=300)
    print(f"Saved fig: {args.prefix}_{transfer.lower()}_{args.output_suffix}.png")

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
ax.yaxis.set_major_formatter(FuncFormatter(lambda val, pos: f"{val:.2f}"))

plt.margins(x=0.1, y=0.1)
plt.xlabel("Number of DPUs")
plt.ylabel("Bandwidth (GB/s, log scale)")
plt.legend(title="Transfer type & Direction", ncol=2)
plt.grid(True, linestyle=":")
plt.tight_layout()
plt.savefig(f"{args.prefix}_all_{args.output_suffix}.png", dpi=300)
print(f"Saved fig: {args.prefix}_all_{args.output_suffix}.png")

# plt.show()

df_sustained = df[df["data_size_MB"] != 1]
print(
    f"""
    Serial transfer CPU-DPU max {df_sustained[df_sustained["transfer_type"] == "serial"]["cpu_dpu_bw_GBs"].max()}, min {df_sustained[df_sustained["transfer_type"] == "serial"]["cpu_dpu_bw_GBs"].min()}
    Serial transfer DPU-CPU max {df_sustained[df_sustained["transfer_type"] == "serial"]["dpu_cpu_bw_GBs"].max()}, min {df_sustained[df_sustained["transfer_type"] == "serial"]["dpu_cpu_bw_GBs"].min()}
    """
)
