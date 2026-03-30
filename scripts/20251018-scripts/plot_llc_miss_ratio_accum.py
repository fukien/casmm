import matplotlib.pyplot as plt
from matplotlib.gridspec import GridSpec
import numpy as np
import os


LOG_DIR="../../logs/20251018-logs"
FIG_DIR="../../figs/20251018-figs"


default_fontsize = 14
label_fontsize = 11
tick_fontsize = 10.5
title_fontsize = 13
legend_fontsize = 11
rotation = 0
title_y_coord = -0.4
labelpad = -0.2


def round6decimals(value):
	return round(float(value), 6)


def get_tps_llc_miss(filepath, dataset_list):
	llc_miss_list = [np.nan for _ in range(len(dataset_list))]
	with open(filepath, "r") as f:
		while True:
			line = f.readline()
			if not line:
				break
			if "PERF_COUNT_HW_CACHE_MISSES" in line:
				line = line.strip().split("\t")
				llc_miss = int(line[-1])
			if "dataset:" in line:
				line = line.strip().split("\t")
				dataset = line[0].split()[-1]
				if dataset in dataset_list:
					llc_miss_list[dataset_list.index(dataset)] = llc_miss/(10**6)
	return llc_miss_list



def plot_llc_accum(mem):
	fig = plt.figure(figsize=(1.8, 1.4))
	ax = plt.gca()  # Get current Axes
	# ax_y_second = ax.twinx()

	x_axis_label_list = [
		"2",
		"4",
		"8",
		"16",
		"32",
	]

	x_axis = np.arange(len(x_axis_label_list)) + 1


	scale = "18"
	datatype = "er"
	dataset_list = [
		f"{datatype}_{scale}_{edgefactor}" for edgefactor in x_axis_label_list
	]
	llc_miss_er_hash_list = get_tps_llc_miss(
		f"{LOG_DIR}/llc_{datatype}_{mem}_hash.log", dataset_list
	)
	llc_miss_er_hsersc_list = get_tps_llc_miss(
		f"{LOG_DIR}/llc_{datatype}_{mem}_hsersc.log", dataset_list
	)

	datatype = "g500"
	dataset_list = [
		f"{datatype}_{scale}_{edgefactor}" for edgefactor in x_axis_label_list
	]
	llc_miss_g500_hash_list = get_tps_llc_miss(
		f"{LOG_DIR}/llc_{datatype}_{mem}_hash.log", dataset_list
	)
	llc_miss_g500_hsersc_list = get_tps_llc_miss(
		f"{LOG_DIR}/llc_{datatype}_{mem}_hsersc.log", dataset_list
	)

	llc_miss_ratio_er_list = np.array(llc_miss_er_hsersc_list)/np.array(llc_miss_er_hash_list)
	llc_miss_ratio_g500_list = np.array(llc_miss_g500_hsersc_list)/np.array(llc_miss_g500_hash_list)

	# Define colors
	color_er = "#1f77b4"	# blue
	color_g500 = "#d62728"  # red

	ax.plot(
		x_axis,
		llc_miss_ratio_er_list,
		color=color_er,
		linestyle="-",
		marker="o",
		label="ER"
	)

	ax.plot(
		x_axis,
		llc_miss_ratio_g500_list,
		color=color_g500,
		linestyle="-",
		marker="*",
		label="G500"
	)


	ax.grid()
	# ax.grid(True, linestyle='--', linewidth=0.75, alpha=0.7, color=color_er)
	ax.set_xticks(x_axis)
	ax.set_xticklabels(
		x_axis_label_list,
		rotation=rotation,
		fontsize=tick_fontsize
	)
	ax.tick_params(
		axis='y', 
		labelsize=tick_fontsize, 
		# colors=color_er
	) # Set tick label color
	ax.set_yticks([1, 2, 3, 4, 5, 6])

	# ax_y_second.grid(True, linestyle='--', linewidth=0.75, alpha=0.7, color=color_g500)
	# ax_y_second.tick_params(axis='y', labelsize=tick_fontsize, colors=color_g500) # Set tick label color here
	# ax_y_second.set_ylim(0.7, 6.2)

	ax.set_ylabel("Relative LLC Miss\nRatio (ESC/HASH)", fontsize=label_fontsize)
	ax.set_xlabel("Edgefactor",
		fontsize=label_fontsize,
		labelpad=labelpad,
	)


	# Set the color of the right y-axis label
	# ax_y_second.set_ylabel("LLC Miss\nRatio", fontsize=label_fontsize, color=color_g500)


	ax_handles, ax_labels = ax.get_legend_handles_labels()
	fig.legend(
		ax_handles, ax_labels, 
		loc="upper center", bbox_to_anchor=(0.6, 1.16),
		fancybox=False, shadow=False, frameon=False,
		ncol=2, 
		fontsize=legend_fontsize, 
		columnspacing=0.95
	)


	plt.savefig(os.path.join(FIG_DIR, f"llc_miss_ratio_accum_{mem}.pdf"), bbox_inches="tight", format="pdf")
	plt.savefig(os.path.join(FIG_DIR, f"llc_miss_ratio_accum_{mem}.eps"), bbox_inches="tight", format="eps")

	plt.close()


if __name__ == "__main__":
	if not os.path.exists(FIG_DIR):
		os.makedirs(FIG_DIR)

	for mem in [
		# "1",
		"6"
	]:
		plot_llc_accum(mem)