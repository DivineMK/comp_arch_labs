#!/usr/bin/env python3
"""
This script processes ChampSim log files to extract IPC values, calculate speedups,
and generate bar plots for different prefetchers.

It assumes a log directory structure like:
logs/1C.{config}.{prefetcher_name}/1C.{config}.{prefetcher_name}_trace_{trace_name}.log
"""

import os
import re
import glob
import json
import math
from collections import defaultdict
import matplotlib.pyplot as plt


def parse_log_file(file_path):
    """
    Parses a single log file to find the final cumulative IPC for CPU 0.
    """
    # This regex is anchored to the start of the line to specifically match
    # the final summary line under "Region of Interest Statistics".
    ipc_pattern = re.compile(r"^CPU 0 cumulative IPC:\s*([\d\.]+)")
    try:
        with open(file_path, "r") as f:
            for line in f:
                match = ipc_pattern.search(line)
                if match:
                    return float(match.group(1))
    except FileNotFoundError:
        print(f"Warning: Log file not found: {file_path}")
    except Exception as e:
        print(f"Warning: Error reading or parsing log file {file_path}: {e}")

    print(f"Warning: Could not find 'CPU 0 cumulative IPC' in {file_path}")
    return None


def main():
    """
    Main function to find logs, process them, and generate plots.
    """
    logs_dir = "logs"
    # Using defaultdict to easily create nested dictionaries for config -> trace -> prefetcher
    ipcs = defaultdict(lambda: defaultdict(dict))

    # Glob pattern to find all relevant log files within subdirectories
    log_pattern = os.path.join(logs_dir, "1C.*", "1C.*_trace_*.log")
    log_files = glob.glob(log_pattern)

    if not log_files:
        print(f"No log files found matching the pattern: {log_pattern}")
        print(
            "Please ensure that the log files are in the format logs/1C.{config}.{prefetcher_name}/1C.{config}.{prefetcher_name}_trace_{trace_name}.log"
        )
        return

    print(f"Found {len(log_files)} log files to process...")

    for log_file in log_files:
        try:
            # --- Extract config, prefetcher, and trace name from the path ---
            # Get config and prefetcher from directory name
            dir_name = os.path.basename(os.path.dirname(log_file))
            _, config, prefetcher_name = dir_name.split(".")

            # Get trace name from filename
            filename = os.path.basename(log_file)
            base_name = filename.removesuffix(".log")
            _, trace_name = base_name.split("_trace_")

            ipc = parse_log_file(log_file)
            if ipc is not None:
                ipcs[config][trace_name][prefetcher_name] = ipc

        except (ValueError, IndexError):
            print(f"Warning: Could not parse path components from: {log_file}")

    if not ipcs:
        print("No IPC data was successfully extracted. Exiting.")
        return

    # --- Calculate Speedups ---
    speedups = defaultdict(lambda: defaultdict(dict))
    for config, traces in ipcs.items():
        for trace_name, prefetcher_ipcs in traces.items():
            if "nopref" in prefetcher_ipcs:
                baseline_ipc = prefetcher_ipcs["nopref"]
                if baseline_ipc > 0:
                    for prefetcher_name, ipc in prefetcher_ipcs.items():
                        speedups[config][trace_name][prefetcher_name] = (
                            ipc / baseline_ipc
                        )
                else:
                    print(
                        f"Warning: Baseline 'nopref' IPC is 0 for trace {trace_name} in config {config}. Cannot calculate speedup."
                    )
            else:
                print(
                    f"Warning: 'nopref' baseline not found for trace {trace_name} in config {config}. Cannot calculate speedup."
                )

    # --- Calculate Geometric Mean of Speedups ---
    for config, traces_speedups in speedups.items():
        # Get all prefetchers for this config
        all_prefetchers = set()
        for trace_name, trace_data in traces_speedups.items():
            if trace_name == "GEOMEAN":
                continue
            all_prefetchers.update(trace_data.keys())

        # Calculate gmean for each prefetcher
        for prefetcher in all_prefetchers:
            if prefetcher == "nopref":
                gmean = 1.0
            else:
                speedup_values = [
                    trace_data[prefetcher]
                    for trace_name, trace_data in traces_speedups.items()
                    if trace_name != "GEOMEAN"
                    and prefetcher in trace_data
                    and trace_data[prefetcher] > 0
                ]
                if speedup_values:
                    log_sum = sum(math.log(v) for v in speedup_values)
                    gmean = math.exp(log_sum / len(speedup_values))
                else:
                    gmean = 0  # Default if no valid speedups found

            # Store the geometric mean
            if "GEOMEAN" not in speedups[config]:
                speedups[config]["GEOMEAN"] = {}
            speedups[config]["GEOMEAN"][prefetcher] = gmean

    # --- Write the collected speedups dictionary to a file ---
    output_json_filename = "speedups.json"
    print(f"\n--- Writing speedups to {output_json_filename} ---")
    with open(output_json_filename, "w") as f:
        json.dump(speedups, f, indent=4)
    print("------------------------------------------------------\n")

    # --- Generate Bar Plots ---
    if not speedups:
        print("No speedup data to plot.")
        return

    print("Generating plots...")

    # One plot per configuration
    for config, trace_speedups in speedups.items():
        if not trace_speedups:
            print(f"No data to plot for config: {config}")
            continue

        # Get all unique traces and prefetchers for this config
        all_traces = sorted(trace_speedups.keys())
        # Ensure 'GEOMEAN' is plotted last
        if "GEOMEAN" in all_traces:
            all_traces.remove("GEOMEAN")
            all_traces.append("GEOMEAN")

        all_prefetchers = set()
        for trace in all_traces:
            all_prefetchers.update(trace_speedups[trace].keys())

        # We will plot each prefetcher as a bar, grouped by trace.
        # Exclude 'nopref' from the bars as it's the baseline.
        plot_prefetchers = sorted([p for p in all_prefetchers if p != "nopref"])

        if not plot_prefetchers:
            print(f"Only 'nopref' data found for config {config}, skipping plot.")
            continue

        num_prefetchers = len(plot_prefetchers)
        num_traces = len(all_traces)

        # Setup for grouped bar chart
        x = range(num_traces)  # positions for trace groups
        total_width = 0.8
        bar_width = total_width / num_prefetchers

        fig, ax = plt.subplots(figsize=(8 + num_traces, 6))  # Dynamic figure size

        # Plot bars for each prefetcher
        for i, prefetcher in enumerate(plot_prefetchers):
            speedup_values = [
                trace_speedups[trace].get(prefetcher, 0) for trace in all_traces
            ]
            # Calculate position for each bar
            position = [
                pos - (total_width / 2) + (i * bar_width) + (bar_width / 2) for pos in x
            ]
            rects = ax.bar(position, speedup_values, width=bar_width, label=prefetcher)

            # Add labels on top of the bars
            for rect in rects:
                height = rect.get_height()
                ax.annotate(
                    f"{height:.2f}",
                    xy=(rect.get_x() + rect.get_width() / 2, height),
                    xytext=(0, 3),  # 3 points vertical offset
                    textcoords="offset points",
                    ha="center",
                    va="bottom",
                )

        # Formatting
        ax.set_title(f"Prefetcher Speedups for {config} Configuration")
        ax.set_xlabel("Trace")
        ax.set_ylabel("Speedup (relative to 'nopref')")
        ax.set_xticks(x)
        ax.set_xticklabels(all_traces, rotation=45, ha="right")
        ax.axhline(y=1.0, color="r", linestyle="--", label="Baseline (nopref)")
        ax.legend(title="Prefetchers")
        ax.grid(axis="y", linestyle="--", alpha=0.7)

        fig.tight_layout()

        output_filename = f"{config}_speedups.png"
        plt.savefig(output_filename)
        print(f"Saved plot to {output_filename}")
        plt.close(fig)


if __name__ == "__main__":
    main()
