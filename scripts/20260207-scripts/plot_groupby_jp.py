import math
import os
import re

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import matplotlib.ticker

HERE = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.dirname(os.path.dirname(HERE))
LOG_DIR = os.path.join(REPO_ROOT, "logs", "20260207-logs")
LOG_DIR_ORIG = LOG_DIR
FIG_DIR = os.path.join(REPO_ROOT, "figs", "20260207-figs")

algo2legend = {
	"ab_hybacc_daha": "AMS-HYB",
	"mkl": "MKL",
	"dim3": "DIM3",
	"nphj_sc": "NPHJ",
	"nphj_sc_aggotfoa": "NPHJ-PRJ",
	"phj_rdx_bc": "PHJ",
	"phj_rdx_bc_agg": "PHJ-PRJ",
}

algo2color = {
	"ab_hybacc_daha": "#FFC107",
	"mkl": "#17becf",
	"dim3": "#8c564b",
	"nphj_sc": "#2ca02c",
	"nphj_sc_aggotfoa": "#d62728",
	"phj_rdx_bc": "#9467bd",
	"phj_rdx_bc_agg": "#e377c2",
}

algo2hatch = {
	"ab_hybacc_daha": "...",
	"mkl": "oo",
	"dim3": "--",
	"nphj_sc": "**",
	"nphj_sc_aggotfoa": "||",
	"phj_rdx_bc": "//",
	"phj_rdx_bc_agg": "\\\\",
}

groupby_dataset_list = [
	"imdb_movie",
	"chbc_s_uni",
	"bkgn",
	"chbc_s_skew",
	"ml20m",
]

groupby_dataset_2_title = {
	"imdb_movie": "IMDB",
	"chbc_s_uni": "CLH0",
	"bkgn": "BKGN",
	"chbc_s_skew": "CLH1",
	"ml20m": "MVLN",
}

ALGOS = [
	("ab_hybacc_daha",   "ab_hash_jp_sat",      "jp"),
	("mkl",              "mkl_sp2m_jp",         "jp"),
	("dim3",             "dim3_jp",             "jp"),
	("nphj_sc",          "nphj_sc",             "orig"),
	("nphj_sc_aggotfoa", "nphj_sc_aggotfoa_jp", "jp"),
	("phj_rdx_bc",       "phj_rdx_bc",          "orig"),
	("phj_rdx_bc_agg",   "phj_rdx_bc_agg_jp",   "jp"),
]

REFERENCE = "ab_hybacc_daha"

DATASET_SOURCE = {
	"imdb_movie":  {"mem": "1", "suffix": "_hyp"},
	"chbc_s_uni":  {"mem": "6", "suffix": ""},
	"bkgn":        {"mem": "6", "suffix": ""},
	"chbc_s_skew": {"mem": "6", "suffix": ""},
	"ml20m":       {"mem": "6", "suffix": ""},
}

ALGO_OVERRIDE = {
	"chbc_s_uni":  {"mkl": "_hyp"},
	"bkgn":        {"mkl": "_hyp"},
	"chbc_s_skew": {"mkl": "_hyp"},
	"ml20m":       {"mkl": "_hyp"},
}

ALWAYS_HYP = {"ab_hybacc_daha"}

label_fontsize  = 19.5
tick_fontsize   = 19
title_fontsize  = 20
legend_fontsize = 20

N_ROWS, N_COLS = 1, 5
FIG_W_PER_COL = 3.6
FIG_H_PER_ROW = 2.75
LINEAR_DATASETS = {"bkgn"}

RE_TOTAL      = re.compile(r"dataset:\s+(\S+).*?\btotal_time:\s+([\d.]+)")
RE_TOTAL_DIM3 = re.compile(r"dataset:\s+(\S+).*?avg_total_time:\s+([\d.]+)")
ANSI = re.compile(r"\x1b\[[0-9;]*[A-Za-z]")


def parse_log(path, stem):
	rx = RE_TOTAL_DIM3 if stem.startswith("dim3") else RE_TOTAL
	out = {}
	try:
		with open(path, "r", errors="replace") as fh:
			for line in fh:
				m = rx.search(ANSI.sub("", line))
				if m:
					out[m.group(1)] = float(m.group(2))
	except FileNotFoundError:
		pass
	return out


_cache = {}


def parse_cached(log_dir, mem, stem, suffix):
	key = (log_dir, mem, stem, suffix)
	if key not in _cache:
		_cache[key] = parse_log(
			os.path.join(log_dir, f"groupby_{mem}_{stem}{suffix}.log"), stem)
	return _cache[key]


def resolve(ds, slot, stem, origin):
	src = DATASET_SOURCE.get(ds, {})
	mem = src.get("mem", "1")
	suffix = src.get("suffix", "")

	if slot in ALWAYS_HYP:
		suffix = "_hyp"
	elif slot in ALGO_OVERRIDE.get(ds, {}):
		suffix = ALGO_OVERRIDE[ds][slot]

	log_dir = LOG_DIR if origin == "jp" else LOG_DIR_ORIG
	return log_dir, mem, suffix


def load():
	data = {slot: {} for slot, _, _ in ALGOS}
	for ds in groupby_dataset_list:
		for slot, stem, origin in ALGOS:
			log_dir, mem, suffix = resolve(ds, slot, stem, origin)
			parsed = parse_cached(log_dir, mem, stem, suffix)
			if ds in parsed:
				data[slot][ds] = parsed[ds]
	return data


def plot_groupby(measured, fig_basename):
	slots = [s for s, _, _ in ALGOS]
	n_algos = len(slots)

	fig, axes = plt.subplots(
		N_ROWS, N_COLS, figsize=(N_COLS * FIG_W_PER_COL, N_ROWS * FIG_H_PER_ROW))
	axes_flat = axes.flatten()
	subfig_labels = [f"({chr(ord('a') + i)})" for i in range(len(groupby_dataset_list))]
	bar_w = 0.65

	for idx, ds in enumerate(groupby_dataset_list):
		ax = axes_flat[idx]
		title = groupby_dataset_2_title.get(ds, ds)
		missing = []

		for i, slot in enumerate(slots):
			val = measured[slot].get(ds)
			if val is None:
				missing.append(i)
				continue
			ax.bar(i, val, width=bar_w,
			       color=algo2color[slot], hatch=algo2hatch[slot],
			       edgecolor="black" if algo2hatch[slot] else algo2color[slot],
			       linewidth=0.6)

		if ds not in LINEAR_DATASETS:
			ax.set_yscale("log")
			ax.yaxis.set_major_formatter(
				matplotlib.ticker.FuncFormatter(lambda x, _: f"{x:g}"))
			ax.yaxis.set_minor_locator(matplotlib.ticker.NullLocator())

			y_lo, y_hi = ax.get_ylim()
			log_lo = math.floor(math.log10(y_lo)) if y_lo > 0 else 0
			log_hi = math.ceil(math.log10(y_hi))
			ticks = [10 ** i for i in range(log_lo, log_hi + 1)]
			if ds in ("chbc_s_uni", "chbc_s_skew", "ml20m"):
				if 1 not in ticks:
					ticks = sorted([1] + ticks)
				if ds == "ml20m" and 10 not in ticks:
					ticks = sorted(ticks + [10])
			else:
				if 0.1 not in ticks:
					ticks = sorted([0.1] + ticks)
			remove_ticks = {
				"chbc_s_uni": {0.1, 1000},
				"chbc_s_skew": {0.1, 1000},
				"ml20m": {0.1, 10000},
			}
			if ds in remove_ticks:
				ticks = [t for t in ticks if t not in remove_ticks[ds]]
			visible = [t for t in ticks if y_lo <= t <= y_hi]
			if len(visible) < 2:
				below = [t for t in ticks if t < y_lo]
				above = [t for t in ticks if t > y_hi]
				if below:
					ax.set_ylim(bottom=below[-1] * 0.7)
				if above and len(visible) + len(below) < 2:
					ax.set_ylim(top=above[0] * 1.3)
			ax.set_yticks(ticks)
			if 1 in ticks and ax.get_ylim()[0] >= 1:
				ax.set_ylim(bottom=1)
		else:
			ax.yaxis.set_major_formatter(
				matplotlib.ticker.FuncFormatter(lambda x, _: f"{x:g}"))
			ax.set_yticks([0, 5, 10])

		ax.set_xlabel(f"{subfig_labels[idx]} {title}",
		              fontsize=title_fontsize, fontweight="bold", labelpad=6)
		ax.set_xticks([])
		ax.set_xlim(-0.6, n_algos - 0.4)
		ax.yaxis.grid(True, linestyle="--", color="#b0b0b0")
		ax.set_axisbelow(True)
		ax.set_ylabel("Elapsed Time (s)", fontsize=label_fontsize)
		ax.tick_params(axis="y", labelsize=tick_fontsize)

		if missing:
			y_min, y_max = ax.get_ylim()
			log_min = math.log10(y_min) if y_min > 0 else 0
			log_max = math.log10(y_max)
			na_height = 10 ** (log_min + (log_max - log_min) * 0.12)
			for i in missing:
				ax.bar(i, na_height, width=bar_w, color="#d3d3d3",
				       edgecolor="black", linewidth=0.6, zorder=3)

	for idx in range(len(groupby_dataset_list), N_ROWS * N_COLS):
		axes_flat[idx].set_visible(False)

	handles = [
		mpatches.Patch(facecolor=algo2color[s], hatch=algo2hatch[s],
		               edgecolor="black" if algo2hatch[s] else algo2color[s],
		               label=algo2legend[s], linewidth=0.6)
		for s in slots
	]
	fig.legend(handles=handles, ncol=n_algos, fontsize=legend_fontsize,
	           loc="upper center", bbox_to_anchor=(0.5, 1.20), framealpha=1.0,
	           handlelength=1.0, handleheight=1.2, handletextpad=0.24,
	           columnspacing=0.66)

	fig.tight_layout(rect=[0, 0, 1, 0.98])
	base = os.path.join(FIG_DIR, fig_basename)
	for ext in ("png", "eps"):
		plt.savefig(f"{base}.{ext}", bbox_inches="tight", format=ext)
	plt.close()
	print(f"\n  wrote {base}.png / .eps")


def print_summary(measured):
	slots = [s for s, _, _ in ALGOS]

	print(f"\n{'=' * 104}")
	print(f"  panel provenance  ({len(groupby_dataset_list)} panels, {len(slots)} bars each)"
	      f"   bar heights are the measured values, unmodified")
	print(f"{'=' * 104}")
	print(f"{'panel':<7}{'dataset':<14}{'mask':>5}{'threads':>9}   bars read from")
	for idx, ds in enumerate(groupby_dataset_list):
		_, mem, suffix = resolve(ds, "dim3", "dim3_jp", "jp")
		th = "64" if suffix == "_hyp" else "32"
		note = []
		if ds in ALGO_OVERRIDE:
			note.append("MKL@64t")
		note.append("AMS-HYB@64t")
		lbl = f"({chr(ord('a') + idx)})"
		print(f"{lbl:<7}{ds:<14}{mem:>5}{th:>9}   {', '.join(note)}")

	header = (f"{'panel':<6}{'dataset':<12}"
	          + "".join(f"{algo2legend[s]:>11}" for s in slots))

	title = "MEASURED runtime (s), straight from the logs — these are the bar heights"
	print(f"\n{'=' * len(header)}\n  {title}\n{'=' * len(header)}")
	print(header)
	print("-" * len(header))
	for idx, ds in enumerate(groupby_dataset_list):
		row = f"({chr(ord('a') + idx)})   {groupby_dataset_2_title.get(ds, ds):<12}"
		for s in slots:
			v = measured[s].get(ds)
			row += f"{'N/A':>11}" if v is None else f"{v:11.3f}"
		print(row)

	print(f"\n{'=' * len(header)}\n  Speedup: other / {algo2legend[REFERENCE]}"
	      f"   (>1 means {algo2legend[REFERENCE]} is faster)\n{'=' * len(header)}")
	print(f"{'':<18}" + "".join(f"{groupby_dataset_2_title[d]:>11}"
	                            for d in groupby_dataset_list))
	for s in slots:
		if s == REFERENCE:
			continue
		row, sp = f"{algo2legend[s]:<18}", []
		for ds in groupby_dataset_list:
			v, r = measured[s].get(ds), measured[REFERENCE].get(ds)
			if v is not None and r:
				sp.append(v / r)
				row += f"{v / r:10.2f}x"
			else:
				row += f"{'N/A':>11}"
		if sp:
			gm = math.exp(sum(map(math.log, sp)) / len(sp))
			row += f"   geomean {gm:.2f}x over {len(sp)}"
		print(row)


def run(fig_basename="groupby_jp_1_tr"):
	if not os.path.exists(FIG_DIR):
		os.makedirs(FIG_DIR)
	measured = load()
	print_summary(measured)
	plot_groupby(measured, fig_basename)
	return measured


if __name__ == "__main__":
	run()
