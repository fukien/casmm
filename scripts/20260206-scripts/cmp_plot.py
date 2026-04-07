import re
import os
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np

# ── configuration ──────────────────────────────────────────────────────────────

LOG_DIR = "../../logs/20250917-logs"
FIG_DIR = "../../figs/20250917-figs"

DATASETS = [
	"lsfm",
	"gdbk", "joke3", "joke1", "joke2",
	"lfsoi", "rd23", "isf18",
	"ml20m",
	"sac18", "bkgn",
	"chbc_s_uni", "chbc_s_skew",
]

# (log-file suffix, display label, bar color, hatch)
ALGORITHMS = [
	("ab_hash_daha",   "ab_hash_daha",   "#4C72B0", ""),
	("ab_hsersc_daha", "ab_hsersc_daha", "#DD8452", ""),
	("ab_hybacc_daha", "ab_hybacc_daha", "#55A868", ""),
	("dim3_hyp",       "dim3_hyp",       "#C44E52", "//"),
	("dim3",           "dim3",           "#C44E52", ""),
	("hash_hyp",       "hash_hyp",       "#8172B2", "//"),
	("hash",           "hash",           "#8172B2", ""),
	("mkl_hyp",        "mkl_hyp",        "#937860", "//"),
	("mkl",            "mkl",            "#937860", ""),
	("nphj_sc_hyp",            "nphj_sc_hyp",            "#DA8BC3", "//"),
	("nphj_sc",               "nphj_sc",                "#DA8BC3", ""),
	("nphj_sc_aggotfoa_hyp",  "nphj_sc_aggotfoa_hyp",  "#CCB974", "//"),
	("nphj_sc_aggotfoa",      "nphj_sc_aggotfoa",      "#CCB974", ""),
	("phj_rdx_bc_hyp",        "phj_rdx_bc_hyp",        "#8C8C8C", "//"),
	("phj_rdx_bc",            "phj_rdx_bc",            "#8C8C8C", ""),
	("phj_rdx_bc_agg_hyp",    "phj_rdx_bc_agg_hyp",    "#64B5CD", "//"),
	("phj_rdx_bc_agg",        "phj_rdx_bc_agg",        "#64B5CD", ""),
]

# Human-readable memory configuration labels
MEM_CONFIG_LABEL = {
	"1": "Mem Config 1 (Single-node)",
	"6": "Mem Config 6 (Multi-node)",
}

# Datasets that should use a log-scale y-axis
LOG_SCALE_DATASETS = {"lsfm", "lfsoi", "isf18"}

SETUPS = ["1", "6"]

# ── parsers ────────────────────────────────────────────────────────────────────

RE_TOTAL_GENERIC = re.compile(
	r"dataset:\s+(\S+).*?total_time:\s+([\d.]+)"
)
RE_TOTAL_DIM3 = re.compile(
	r"dataset:\s+(\S+).*?avg_total_time:\s+([\d.]+)"
)


def pick_regex(algo_key):
	if algo_key.startswith("dim3"):
		return RE_TOTAL_DIM3
	return RE_TOTAL_GENERIC


def parse_log(filepath, algo_key):
	"""Return {dataset: total_time} for one log file."""
	result = {}
	rx = pick_regex(algo_key)
	try:
		with open(filepath, "r", errors="replace") as f:
			for line in f:
				m = rx.search(line)
				if m:
					ds, t = m.group(1), float(m.group(2))
					result[ds] = t
	except FileNotFoundError:
		pass
	return result


def load_all(log_dir):
	"""Return data[setup][algo_key][dataset] = total_time or None."""
	data = {}
	for setup in SETUPS:
		data[setup] = {}
		for algo_key, *_ in ALGORITHMS:
			fname = f"groupby_{setup}_{algo_key}.log"
			fpath = os.path.join(log_dir, fname)
			data[setup][algo_key] = parse_log(fpath, algo_key)
	return data


# ── plotting ───────────────────────────────────────────────────────────────────

def plot_setup(setup, algo_data, out_path):
	"""One figure per setup: one subplot per dataset, algorithms on x-axis."""
	n_ds   = len(DATASETS)
	n_algo = len(ALGORITHMS)

	fig, axes = plt.subplots(
		1, n_ds,
		figsize=(5 * n_ds, 5),
		sharey=False,   # independent y-axes so each dataset fills nicely
	)
	if n_ds == 1:
		axes = [axes]

	bar_w = 0.7

	for col, ds in enumerate(DATASETS):
		ax = axes[col]

		for i, (algo_key, label, color, hatch) in enumerate(ALGORITHMS):
			h = algo_data[algo_key].get(ds, None)
			if h is not None:
				ax.bar(
					i, h, width=bar_w,
					color=color, hatch=hatch,
					edgecolor="white" if not hatch else "black",
					linewidth=0.5,
				)

		if ds in LOG_SCALE_DATASETS:
			ax.set_yscale("log")
			ax.set_title(f"{ds} (log scale)", fontsize=11, fontweight="bold")
		else:
			ax.set_title(ds, fontsize=11, fontweight="bold")
		ax.set_xticks([])           # algorithm names shown in shared legend
		ax.set_xlim(-0.6, n_algo - 0.4)
		ax.yaxis.grid(True, linestyle="--", alpha=0.5)
		ax.set_axisbelow(True)
		if col == 0:
			ax.set_ylabel("Total Time (s)", fontsize=10)

	# shared legend below all subplots
	handles = []
	for algo_key, label, color, hatch in ALGORITHMS:
		patch = mpatches.Patch(
			facecolor=color, hatch=hatch,
			edgecolor="black" if hatch else color,
			label=label, linewidth=0.5,
		)
		handles.append(patch)

	fig.legend(
		handles=handles,
		ncol=min(n_algo, 7),
		fontsize=8,
		loc="lower center",
		bbox_to_anchor=(0.5, -0.16),
		framealpha=0.85,
		handlelength=1.8,
		handleheight=1.2,
	)

	mem_label = MEM_CONFIG_LABEL.get(setup, f"Setup {setup}")
	fig.suptitle(
		f"Groupby Benchmark — {mem_label}",
		fontsize=13, fontweight="bold", y=1.02,
	)

	fig.tight_layout()
	fig.savefig(out_path, dpi=150, bbox_inches="tight")
	print(f"Saved: {out_path}")
	plt.close(fig)


def main():
	if not os.path.exists(FIG_DIR):
		os.makedirs(FIG_DIR)
	data = load_all(LOG_DIR)
	for setup in SETUPS:
		out_path = os.path.join(FIG_DIR, f"groupby_setup{setup}_barplot.png")
		plot_setup(setup, data[setup], out_path)


if __name__ == "__main__":
	main()