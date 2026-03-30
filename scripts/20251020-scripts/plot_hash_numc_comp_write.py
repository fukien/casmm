import matplotlib.pyplot as plt
from matplotlib.gridspec import GridSpec
import numpy as np
import os


LOG_DIR="../../logs/20251020-logs"
FIG_DIR="../../figs/20251020-figs"

RUN_NUM=2
THREAD_NUM=32


default_fontsize = 14.5
title_y_coord = -0.39
bar_width = 0.8

phase_list = [
	"comp",
	"wrie",
]

phase2label = {
	"comp": "computation",
	"write": "materialization",
}

phase2color = {
	"comp": "bisque",
	"write": "limegreen"
}


def round6decimals(value):
	return round(float(value), 6)


def get_thr_numc_time(filepath):
	thr_numc_time = []
	_comp_time_list = []
	_write_time_lis = []
	with open(filepath, "r") as f:
		while True:
			line = f.readline()
			if not line:
				break
			if "_numc_time" in line:
				line = line.strip().split("\t")
				# _tid = int(line[0].split("_")[0])
				_comp_time = round6decimals(line[2].split()[-1])
				_comp_time_list.append(_comp_time)
				_write_time = round6decimals(line[3].split()[-1])
				_write_time_lis.append(_write_time)
				continue

	# we exclude the runtime of the first run
	return _comp_time_list[THREAD_NUM * (RUN_NUM-1):], \
			_write_time_lis[THREAD_NUM * (RUN_NUM-1):]


def plot_numc_comp_write():
	x_axis = np.arange(THREAD_NUM) + 1
	x_data_width = 0.86
	bar_width = x_data_width / len(phase_list) * 3 / 4
	x_starting_idx_list = [
		-x_data_width / 2 + (i + 1 / 2) * x_data_width / len(phase_list)
		for i in range(len(phase_list))
	]


	# g500_thr_numc_list_6 = get_thr_numc_time(
	# 		f"{LOG_DIR}/g500_hash_6.log"
	# 	)
	_comp_time_list, _write_time_list = get_thr_numc_time(
			f"{LOG_DIR}/g500_6_hash.log"
		)

	fig = plt.figure(figsize=(8, 2))
	plt.bar(
		x_axis+x_starting_idx_list[0], 
		_comp_time_list,
		width=bar_width, 
		edgecolor="black", 
		color=phase2color["comp"],
		label=phase2label["comp"]
	)

	plt.bar(
		x_axis+x_starting_idx_list[1], 
		_write_time_list,
		width=bar_width, 
		edgecolor="black", 
		color=phase2color["write"],
		label=phase2label["write"]
	)

	fig.legend(
		loc="upper center", bbox_to_anchor=(0.5, 1.09),
		fancybox=True, shadow=True, frameon=False,
		ncol=len(phase_list), fontsize=default_fontsize-3, 
		columnspacing=1.15
	)

	plt.grid()
	plt.xticks(
		x_axis, 
		[str(i) for i in range(THREAD_NUM)], 
		rotation=-45, 
		fontsize=default_fontsize-5
	)
	plt.xlabel("Thread ID", fontsize=default_fontsize-1.5)
	plt.ylabel("Elapsed Time (s)", fontsize=default_fontsize-1.5)

	plt.savefig(
		os.path.join(FIG_DIR, "g500_19_8_hash_numc_comp_write.eps"),
		bbox_inches="tight", format="eps"
	)

	plt.savefig(
		os.path.join(FIG_DIR, "g500_19_8_hash_numc_comp_write.pdf"),
		bbox_inches="tight", format="pdf"
	)

	plt.close()



if __name__ == "__main__":
	if not os.path.exists(FIG_DIR):
		os.makedirs(FIG_DIR)
	plot_numc_comp_write()
