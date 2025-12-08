#!/bin/bash

print_help() {
  cat <<EOF
Usage: $0 [OPTIONS]

Options:
  -b, --binary NAME     Name of ChampSim binary (default: 1C.fullBW.nopref)
  -c, --config NAME     Config name (default: same as binary)
  -j, --jobs N          Number of parallel jobs (default: nproc)
  -h, --help            Show this help message

Example:
  $0 -b 1C.fullBW.nopref -c 1C.fullBW.ghb_stride.json -j 8
EOF
}

BIN_NAME=""
CONFIG=""
LOG_SUFFIX=""
NUM_JOBS="$(nproc)"
DRY_RUN=0

while [[ $# -gt 0 ]]; do
  case "$1" in
  -b | --binary)
    BIN_NAME="$2"
    shift 2
    ;;
  -c | --config)
    CONFIG="$2"
    shift 2
    ;;
  -j | --jobs)
    NUM_JOBS="$2"
    shift 2
    ;;
  -D | --dry-run)
    DRY_RUN=1
    shift 1
    ;;
  -s | --log-suffix)
    LOG_SUFFIX="$2"
    shift 2
    ;;
  -h | --help)
    print_help
    exit 0
    ;;
  *)
    echo "Unknown option: $1"
    print_help
    exit 1
    ;;
  esac
done

# Path to the ChampSim binary
PARENT_DIR="$(realpath $(dirname $0))"
BIN_NAME="${BIN_NAME:-1C.fullBW.nopref}"
CONFIG="${CONFIG:-${BIN_NAME}.json}"
BINARY="${PARENT_DIR}/ChampSim/bin/${BIN_NAME}"

if [[ -n $LOG_SUFFIX ]]; then
  LOG_SUFFIX="_$LOG_SUFFIX"
fi
# Directory containing the trace files
TRACE_DIR="${PARENT_DIR}/traces"
LOG_DIR="$(realpath $(dirname $0))/logs/${BIN_NAME}${LOG_SUFFIX}"

if [ "$DRY_RUN" -eq 1 ]; then
  echo "Dry run"
  echo "log file: ${LOG_DIR}/${BIN_NAME}${LOG_SUFFIX}_trace_tracename"
  exit 0
fi
mkdir -p "$LOG_DIR"
# Export the binary path so it's available to the subshell started by xargs
export BINARY
export LOG_DIR
export LOG_SUFFIX
export BIN_NAME

echo "Configure $(basename $BINARY) with $CONFIG"

cd "$PARENT_DIR/ChampSim" && ./config.sh prefetcher/config/${CONFIG} && make -j
# Check if the binary exists and is executable
if [ ! -x "$BINARY" ]; then
  echo "Error: ChampSim binary not found or not executable at '$BINARY'"
  echo "Please build the project first by running './config.sh && make'"
  exit 1
fi

# Use find to locate all trace files and pipe them to xargs for parallel execution
echo "Starting parallel run with up to $NUM_JOBS jobs..."

# The main execution logic wrapped in a function for xargs
run_single_trace() {
  trace_file="$1"

  trace_name=$(basename "$trace_file")
  trace_name="${trace_name%.xz}"
  trace_name="${trace_name%.gz}"
  log_filebase="$LOG_DIR/${BIN_NAME}${LOG_SUFFIX}_trace_${trace_name}"
  echo "Running trace: $log_filebase"
  # Run the binary with the trace file
  "$BINARY" --warmup-instructions 10000000 --simulation-instructions 50000000 --json "${log_filebase}.json" -- "$trace_file" 2>&1 >"${log_filebase}.log"

  echo "Finished trace: $log_filebase"
  echo ""
}

export -f run_single_trace

find "$TRACE_DIR" -type f -print0 | xargs -0 -P "$NUM_JOBS" -I {} bash -c 'run_single_trace "{}"'

echo "All traces processed."
