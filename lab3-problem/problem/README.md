# Lo Khanh

New Makefile parameters:
- Task 3:
  - `OP ?= ADD`: operation used (ADD, SUB, MUL or DIVIDE)
- Task 4:
  - `SYNC ?= MUTEX`: type of sync (NO, MUTEX, BARRIER or HANDSHAKE)
  - `SYNC_TYPE ?= SINGLE`: reduction variant (SINGLE or TREE)

In each tasks' folder includes 2 files:
- `build.sh`: sweeps parameters, outputs to `profile/` and a `task*.csv`. Usage: `bash build.sh`
- `plot.py`: use the `task*.csv` to generate the plots and tables. Usage: `python plot.py --output-suffix instruction`
For example in task 1, to reproduce the results: `cd task1 && bash build.sh && python plot.py --output-suffix instruction`
For task 3, the assembly count are obtained through `objdump.sh`.

