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
import argparse
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


def process_logs(logs_dir="logs"):
    """
    Processes log files to extract IPCs, calculate speedups and geometric means.
    Returns (ipcs, speedups) dictionaries indexed by [config][prefetcher][trace].
    """
    # New indexing: ipcs[config][prefetcher][trace]
    ipcs = defaultdict(lambda: defaultdict(dict))

    # Glob pattern to find all relevant log files within subdirectories
    log_pattern = os.path.join(logs_dir, "1C.*", "1C.*_trace_*.log")
    log_files = glob.glob(log_pattern)

    if not log_files:
        print(f"No log files found matching the pattern: {log_pattern}")
        return None, None

    print(f"Found {len(log_files)} log files to process...")

    for log_file in log_files:
        try:
            # --- Extract config, prefetcher, and trace name from the path ---
            dir_name = os.path.basename(os.path.dirname(log_file))
            _, config, prefetcher_name = dir_name.split(".")

            # Get trace name from filename
            filename = os.path.basename(log_file)
            base_name = filename.removesuffix(".log")
            trace_name = base_name.split("_trace_")[-1]

            ipc = parse_log_file(log_file)
            if ipc is not None:
                ipcs[config][prefetcher_name][trace_name] = ipc

        except (ValueError, IndexError):
            print(f"Warning: Could not parse path components from: {log_file}")

    if not ipcs:
        return None, None

    # --- Calculate Speedups and Geometric Means ---
    speedups = defaultdict(lambda: defaultdict(dict))
    for config, prefetchers in ipcs.items():
        if "nopref" not in prefetchers:
            print(
                f"Warning: 'nopref' baseline not found for config {config}. Skipping speedups."
            )
            continue

        baseline_ipcs = prefetchers["nopref"]

        for pref_name, pref_ipcs in prefetchers.items():
            valid_speedups = []
            for trace_name, ipc in pref_ipcs.items():
                if trace_name in baseline_ipcs and baseline_ipcs[trace_name] > 0:
                    speedup = ipc / baseline_ipcs[trace_name]
                    speedups[config][pref_name][trace_name] = speedup
                    if pref_name != "nopref":
                        valid_speedups.append(speedup)

            # Calculate Geometric Mean
            if pref_name == "nopref":
                speedups[config][pref_name]["GEOMEAN"] = 1.0
            elif valid_speedups:
                log_sum = sum(math.log(v) for v in valid_speedups)
                gmean = math.exp(log_sum / len(valid_speedups))
                speedups[config][pref_name]["GEOMEAN"] = gmean
            else:
                speedups[config][pref_name]["GEOMEAN"] = 0

    return ipcs, speedups


def plot_results(ipcs, speedups, suffix, plot_nopref):
    """
    Generates bar plots for speedups and baseline IPCs.
    """
    if not speedups:
        print("No speedup data to plot.")
        return

    print("Generating plots...")

    # 1. Speedup Plots: One plot per configuration
    for config, pref_speedups in speedups.items():
        # Collect all unique traces across all prefetchers in this config
        all_traces = set()
        for pref_data in pref_speedups.values():
            all_traces.update(pref_data.keys())

        traces_to_plot = sorted([t for t in all_traces if t != "GEOMEAN"])
        if "GEOMEAN" in all_traces:
            traces_to_plot.append("GEOMEAN")

        # Prefetchers to show (exclude nopref from bars usually, or include it if desired)
        # Here we exclude 'nopref' from the bars to focus on improvements
        plot_prefetchers = sorted([p for p in pref_speedups.keys() if p != "nopref"])

        if not plot_prefetchers:
            print(
                f"Only 'nopref' data found for config {config}, skipping speedup plot."
            )
            continue

        num_prefs = len(plot_prefetchers)
        num_traces = len(traces_to_plot)

        x = range(num_traces)
        total_width = 0.8
        bar_width = total_width / num_prefs

        fig, ax = plt.subplots(figsize=(8 + num_traces // 2, 6))

        for i, pref in enumerate(plot_prefetchers):
            vals = [pref_speedups[pref].get(t, 0) for t in traces_to_plot]
            pos = [p - (total_width / 2) + (i * bar_width) + (bar_width / 2) for p in x]
            rects = ax.bar(pos, vals, width=bar_width, label=pref)

            # Labels on top
            for rect in rects:
                height = rect.get_height()
                ax.annotate(
                    f"{height:.2f}",
                    xy=(rect.get_x() + rect.get_width() / 2, height),
                    xytext=(0, 3),
                    textcoords="offset points",
                    ha="center",
                    va="bottom",
                    fontsize=8,
                )

        ax.set_title(f"Prefetcher Speedups for {config} Configuration")
        ax.set_ylabel("Speedup (relative to 'nopref')")
        ax.set_xticks(x)
        ax.set_xticklabels(traces_to_plot, rotation=45, ha="right")
        ax.axhline(y=1.0, color="r", linestyle="--", alpha=0.5, label="Baseline")
        ax.legend(title="Prefetchers", bbox_to_anchor=(1.05, 1), loc="upper left")
        ax.grid(axis="y", linestyle="--", alpha=0.7)

        fig.tight_layout()
        output_filename = f"{config}_speedups_{suffix}.png"
        plt.savefig(output_filename, bbox_inches="tight")
        print(f"Saved speedup plot to {output_filename}")
        plt.close(fig)

    # 2. Baseline IPC Plot: Specifically for 'fullBW' nopref
    if not plot_nopref:
        print("Skipping plotting nopref image")
        return

    if "fullBW" in ipcs and "nopref" in ipcs["fullBW"]:
        print("Generating baseline IPC plot for fullBW configuration...")
        nopref_data = ipcs["fullBW"]["nopref"]
        traces = sorted(nopref_data.keys())
        ipc_values = [nopref_data[t] for t in traces]

        fig, ax = plt.subplots(figsize=(10, 6))
        x = range(len(traces))
        rects = ax.bar(x, ipc_values, color="gray", alpha=0.8)

        for rect in rects:
            height = rect.get_height()
            ax.annotate(
                f"{height:.3f}",
                xy=(rect.get_x() + rect.get_width() / 2, height),
                xytext=(0, 3),
                textcoords="offset points",
                ha="center",
                va="bottom",
            )

        ax.set_title("Baseline IPC (nopref) for fullBW Configuration")
        ax.set_ylabel("IPC")
        ax.set_xticks(x)
        ax.set_xticklabels(traces, rotation=45, ha="right")
        ax.grid(axis="y", linestyle="--", alpha=0.5)

        fig.tight_layout()
        output_filename = f"fullBW_nopref.png"
        plt.savefig(output_filename)
        print(f"Saved baseline IPC plot to {output_filename}")
        plt.close(fig)
    else:
        print("Failed to plot nopref")


def main():
    """
    Main function to find logs, process them, and generate plots.
    """
    parser = argparse.ArgumentParser(
        description="Process ChampSim logs and generate plots."
    )
    parser.add_argument(
        "--suffix",
        "-s",
        type=str,
        default="all",
        help="Suffix for plot filenames (default: 'all')",
    )
    parser.add_argument(
        "--force",
        "-f",
        action="store_true",
        help="Force reprocessing and overwriting of existing JSON files without prompting",
    )
    parser.add_argument(
        "--plot_nopref",
        "-p",
        action="store_true",
        help="Plot nopref",
    )
    args = parser.parse_args()

    ipcs = None
    speedups = None
    should_write = True

    # Check if we should overwrite existing JSONs before processing
    if os.path.exists("ipcs.json") and not args.force:
        response = input("ipcs.json already exists. Reprocess and overwrite? (y/N): ")
        if response.lower() != "y":
            should_write = False
            print("Skipping log processing. Attempting to load existing data...")
            try:
                with open("ipcs.json", "r") as f:
                    ipcs = json.load(f)
                with open("speedups.json", "r") as f:
                    speedups = json.load(f)
            except Exception as e:
                print(f"Error loading existing JSON files: {e}")
                return

    # If ipcs is still None, it means we didn't load it (or chose to reprocess)
    if ipcs is None:
        ipcs, speedups = process_logs()

    if not ipcs:
        print("No data successfully extracted or loaded. Exiting.")
        return

    # --- Write the collected dictionaries to files ---
    if should_write:
        print(f"\n--- Writing data to JSON files ---")
        with open("ipcs.json", "w") as f:
            json.dump(ipcs, f, indent=4)
        with open("speedups.json", "w") as f:
            json.dump(speedups, f, indent=4)

    # --- Generate Bar Plots ---
    plot_results(ipcs, speedups, suffix=args.suffix, plot_nopref=args.plot_nopref)


if __name__ == "__main__":
    main()
