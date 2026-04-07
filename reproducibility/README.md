# Artifact for Reproducibility

> Wentao Huang *et al.*  
> *Taming Multi-Dimensional Skew in Sparse Matrix Multiplication with Contention-Aware Scheduling*  
> Under review.

---

## 1. Requirements and Overview

### Hardware Requirements
- 4th Gen Intel® Xeon® Scalable Processors
- 64 GB CXL type-3 memory (20.95 GB/s)
- 32 GB DRAM (30.01 GB/s)
- 192 GB DRAM (179.50 GB/s)
- CPU with at least 32 physical cores

### Software Requirements
- Linux kernel version ≥ 6.9.0
- CMake version ≥ 3.26.5
- GCC version ≥ 11.5.0
- PAPI version ≥ 7.2.0b1
- numactl version ≥ 2.1.0

---

## 2. Setup

### 2.1 Clone this Project
```bash
git clone https://github.com/fukien/sc25-artifact-abs
```

### 2.2 Install numactl
Install [numactl](https://github.com/numactl/numactl) (version 2.1.0 or above).

### 2.3 Install PAPI
Install [PAPI](https://icl.utk.edu/papi/).

### 2.4 Install Intel oneAPI Base Toolkit
Install [Intel oneAPI Base Toolkit](https://www.intel.com/content/www/us/en/developer/tools/oneapi/base-toolkit-download.html) (version 2024.0 or later).

### 2.5 SCM Setup
Configure SCM mounting points as `/pmemfs0` and `/pmemfs1`.
Ensure `/pmemfs0` is the default SCM mounting point for the first socket.

### 2.6 Configuration Parameters
- Adjust `./config/mc/mxc.cfg` according to core IDs obtained from `numactl -H`. Socket-1 is "local" and socket-0 is "remote".
```cpp
cores: {
    local_core_id_arrays = [32, ..., 63];
    remote_core_id_arrays = [0, ..., 31];
};
```
- Adjust `./config/mc/mxc0.cfg` for socket-0 as "local" and socket-1 as "remote".
```cpp
cores: {
    local_core_id_arrays = [0, ..., 31];
    remote_core_id_arrays = [32, ..., 63];
};
```
- Modify the number of cores and per-socket cores in `CMakeLists.txt` (lines 47-48).
- Update paths to PAPI and oneAPI libraries in `CMakeLists.txt` (lines 33-35, 53-55).

### 2.7 Compile and Build
```bash
./revitalize.sh
```

### 2.8 Data Generation
```bash
cd scripts/datagen
bash gen_er.sh
```

### 2.9 Run Initial Test Experiments
- Run ABS-HASH:
```bash
numactl --physcpubind=32-63,96-127 ./bin/ab_hashspgemm g500_19_8
```
- Run ABS-ESC:
```bash
numactl --physcpubind=32-63,96-127 ./bin/ab_hserscspgemm g500_19_8
```

---

## 3. Reproduce the Experimental Results

### General Instructions
Navigate to the indicated script folders and execute the provided scripts (`run_tab*.sh` or `run_fig*.sh`) to reproduce corresponding experiments. Plotting scripts (`plot_*.py`) are provided for further analysis.

Figure | Experiment Scripts
---|---
"Figure 1" | [20260205-scripts](scripts/20260205-scripts)
"Figure 3" | [20260204-scripts](scripts/20260204-scripts)
"Figure 4" | [20260203-scripts](scripts/20260203-scripts)
"Figure 6" | [20260201-scripts](scripts/20260201-scripts)
"Figure 8" | [20260202-scripts](scripts/20260202-scripts)
"Figure 9" | [20260129-scripts](scripts/20260129-scripts)
"Figure 10" | [20260126-scripts](scripts/20260126-scripts)
"Figure 11" | [20260130-scripts](scripts/20260130-scripts)
"Figure 12" | [20260127-scripts](scripts/20260127-scripts)
"Figure 13" | [20260131-scripts](scripts/20260131-scripts)
"Figure 14" | [20260206-scripts](scripts/20260206-scripts)
"Figure 15" | [20260128-scripts](scripts/20260128-scripts)

---

### One-Click Full Reproduction
To reproduce all experiments at once:
```bash
cd reproducibility/
bash run_all.sh
```
Logs will be stored in `./logs/`. Pre-collected logs are also available.

**Note:**
- `table1-scripts` and `table2-scripts` print data to stdout and are recommended to be run individually.
- Running all experiments via `run_all.sh` may take up to **20 hours**.

---

### Plot All Figures
After completing the experiments and collecting logs:
```bash
cd reproducibility/
bash plot_all.sh
```
All figures will be saved under `./figs/`. Pre-generated figures are also available.

---
