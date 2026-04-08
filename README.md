# Taming Multi-Dimensional Skew in Sparse Matrix Multiplication with Contention-Aware Scheduling

This repository contains the source code for the paper *Taming Multi-Dimensional Skew in Sparse Matrix Multiplication with Contention-Aware Schedulingr*.

![Status](https://img.shields.io/badge/status-under--review-yellow)

---

## Prerequisites

```
1. A two-socket machine with 4th Gen Intel® Xeon® Scalable Processors.  
2. ASIC-based CXL memory (at least 64 GB).
3. Linux kernel version 6.9.0 or above.

Note: If you do not meet the above hardware requirements, you may not be able to reproduce all experimental results.
```

## Software Dependencies

### Install daxctl
```bash
sudo apt install daxctl
```

### Install numactl
Install [numactl](https://github.com/numactl/numactl) (version 2.1.0 or above).

### Install PAPI
Install [PAPI](https://icl.utk.edu/papi/).

### Install Intel oneAPI Base Toolkit
Install [Intel oneAPI Base Toolkit](https://www.intel.com/content/www/us/en/developer/tools/oneapi/base-toolkit-download.html) (version 2024.0 or above).

### Grant Permissions for PAPI
```bash
sudo sh -c "echo -1 > /proc/sys/kernel/perf_event_paranoid"
sudo sh -c "echo 0 > /proc/sys/kernel/kptr_restrict"
```

---

## CXL Memory Configuration

```bash
sudo daxctl reconfigure-device --mode=system-ram --force dax0.0
bash reboot_init.sh
```

---

## Quick Start

### Compile and Build
```bash
./revitalize.sh
```

### Data Generation
```bash
cd scripts/datagen
bash gen_er.sh
```

### Running ABS-HASH SpGEMM
```bash
numactl --physcpubind=32-63,96-127 ./bin/ab_hashspgemm g500_19_8
```

---

## Reproducing Experiments

See the [reproducibility/](reproducibility/) directory for full experimental scripts and instructions.

---

### Contact

For any questions, please contact: <huangwentao@u.nus.edu>.

Thank you! 😉
