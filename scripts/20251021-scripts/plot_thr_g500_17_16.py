import matplotlib.pyplot as plt
from matplotlib.gridspec import GridSpec
import numpy as np
import os


LOG_DIR="../../logs/20251021-logs"
FIG_DIR="../../figs/20251021-figs"

RUN_NUM=2
THREAD_NUM=32


default_fontsize = 14.5
title_y_coord = -0.39
bar_width = 0.8

cfg_list = [
	"6",
	"2",
]

cfg2label = {
	"6": "CXL-Augmented Memory System",
	"2": "Conventional Main-Memory System",
}

cfg2color = {
	"6": "hotpink",
	"2": "cornflowerblue"
}


def round6decimals(value):
	return round(float(value), 6)


def get_thr_numc_time(filepath):
	thr_numc_time = []
	with open(filepath, "r") as f:
		while True:
			line = f.readline()
			if not line:
				break
			if "_numc_time" in line:
				line = line.strip().split()
				_tid = int(line[0].split("_")[0])
				_tid_numc_time = round6decimals(line[1])
				thr_numc_time.append(_tid_numc_time)
				continue

	return thr_numc_time[THREAD_NUM * (RUN_NUM-1):]	# we exclude the runtime of the first run


def plot_g500_in_numc_thr():
	x_axis = np.arange(THREAD_NUM) + 1
	x_data_width = 0.86
	bar_width = x_data_width / len(cfg_list) * 3 / 4
	x_starting_idx_list = [
		-x_data_width / 2 + (i + 1 / 2) * x_data_width / len(cfg_list)
		for i in range(len(cfg_list))
	]

	# legend_bar_list = []
	# legend_label_list = [cfg2label[cfg] for cfg in cfg_list]


	g500_thr_numc_list_6 = get_thr_numc_time(
			f"{LOG_DIR}/g500_hash_6.log"
		)
	g500_thr_numc_list_2 = get_thr_numc_time(
			f"{LOG_DIR}/g500_hash_2.log"
		)

	fig = plt.figure(figsize=(8, 2.25))
	plt.bar(
		x_axis+x_starting_idx_list[0], 
		g500_thr_numc_list_2,
		width=bar_width, 
		edgecolor="black", 
		color=cfg2color["2"],
		label=cfg2label["2"]
	)

	plt.bar(
		x_axis+x_starting_idx_list[1], 
		g500_thr_numc_list_6,
		width=bar_width, 
		edgecolor="black", 
		color=cfg2color["6"],
		label=cfg2label["6"]
	)

	fig.legend(
		loc="upper center", bbox_to_anchor=(0.5, 1.09),
		fancybox=True, shadow=True, frameon=False,
		ncol=len(cfg_list), fontsize=default_fontsize-3, 
		columnspacing=1.15
	)

	plt.grid()
	plt.xticks(
		x_axis, 
		[str(i) for i in range(THREAD_NUM)], 
		rotation=-45, 
		fontsize=default_fontsize-5
	)
	plt.tick_params(axis='y', labelsize=default_fontsize-4)

	plt.xlabel("Thread ID", fontsize=default_fontsize-1)
	plt.ylabel("Elapsed Time (s)", fontsize=default_fontsize-1)

	plt.savefig(
		os.path.join(FIG_DIR, "numc_thr_g500_17_16.eps"),
		bbox_inches="tight", format="eps"
	)

	plt.savefig(
		os.path.join(FIG_DIR, "numc_thr_g500_17_16.pdf"),
		bbox_inches="tight", format="pdf"
	)

	plt.close()



if __name__ == "__main__":
	if not os.path.exists(FIG_DIR):
		os.makedirs(FIG_DIR)
	plot_g500_in_numc_thr()