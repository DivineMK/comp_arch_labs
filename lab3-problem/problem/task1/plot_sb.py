import pandas as pd, seaborn as sns, matplotlib.pyplot as plt

df = pd.read_csv("bandwidth_summary.csv")
sns.set_theme(style="ticks")
sns.set_palette("tab10")

for t in df["transfer_type"].unique():
    subset = df[df["transfer_type"] == t]
    plt.figure()
    melted = subset.melt(
        id_vars=["num_dpus", "data_size_MB"],
        value_vars=["cpu_dpu_bw_GBs", "dpu_cpu_bw_GBs"],
        var_name="direction",
        value_name="bandwidth_GBs",
    )

    # Map style names
    melted["direction"] = melted["direction"].map(
        {"cpu_dpu_bw_GBs": "CPU-DPU", "dpu_cpu_bw_GBs": "DPU-CPU"}
    )
    sns.lineplot(
        data=melted,
        x="num_dpus",
        y="bandwidth_GBs",
        hue="data_size_MB",
        style="direction",
        markers=True,
        dashes=True,
    )
    plt.title(f"{t.capitalize()} Transfer Bandwidths")
    plt.savefig(f"{t.lower()}_bandwidth_seaborn.png", dpi=300)
plt.show()
