import matplotlib.pyplot as plt
from matplotlib.gridspec import GridSpec
import numpy as np
import os


LOG_DIR="../../logs/20251012-logs"
FIG_DIR="../../figs/20251012-figs"


default_fontsize = 14
label_fontsize = 11
tick_fontsize = 9
title_fontsize = 13
legend_fontsize = 12
rotation = 0
title_y_coord = -0.45
labelpad = -0.7

cfg_list = [
	"_daha.log",
	".log",
	"_hyp.log",
]

cfg2legend = {
	"_daha.log": "SMTact",
	".log": "w/o SMT",
	"_hyp.log": "w/ SMT",
}

cfg2color = {
    "_daha.log": "#1f77b4",     # strong blue — stable/baseline
    ".log": "#2ca02c",         # green — plain or default
    "_hyp.log": "#d62728",     # red — highlights "hyp" or experimental mode
}


def round6decimals(value):
	return round(float(value), 6)


def get_tps_from_file(filepath, dataset_list):
	# symb_time_list = [np.nan for _ in range(len(dataset_list))]
	# numc_time_list = [np.nan for _ in range(len(dataset_list))]
	tot_time_list = [np.nan for _ in range(len(dataset_list))]
	flop_cnt_list = [np.nan for _ in range(len(dataset_list))]
	with open(filepath, "r") as f:
		while True:
			line = f.readline()
			if not line:
				break
			if "dataset:" in line:
				line = line.strip().split("\t")
				dataset = line[0].split()[-1]
				if dataset in dataset_list:
					idx = dataset_list.index(dataset)
					# symb_time_list[idx] = round6decimals(line[5].split()[-1])
					# numc_time_list[idx] = round6decimals(line[7].split()[-1])
					tot_time_list[idx] = round6decimals(line[8].split()[-1])
					flop_cnt_list[idx] = round6decimals(line[9].split()[-1]) / (10**9)
	return np.array(flop_cnt_list)/np.array(tot_time_list)




def plot_in_ax(
	ax, mem, 
	algotype, 
	datatype, 
	rotation, 
	dataset_list, 
	x_axis_label_list
):
	x_axis = np.arange(len(x_axis_label_list)) + 1
	x_data_width = 0.74
	bar_width = x_data_width / len(cfg_list) * 3 / 4
	x_starting_idx_list = [
		-x_data_width / 2 + (i + 1 / 2) * x_data_width / len(cfg_list)
		for i in range(len(cfg_list))
	]
	for cfg_idx, cfg in enumerate(cfg_list):
		filepath = f"{LOG_DIR}/{datatype}_{mem}_{algotype}{cfg}"
		cur_tps_list = get_tps_from_file(filepath, dataset_list)

		ax.bar(
			x_axis+x_starting_idx_list[cfg_idx],
			cur_tps_list,
			width=bar_width,
			edgecolor="black",
			color=cfg2color[cfg],
			label=cfg2legend[cfg],
		)
	ax.grid()
	ax.set_xticks(x_axis)
	ax.set_xticklabels(
		x_axis_label_list,
		rotation=rotation,
		fontsize=tick_fontsize
	)
	ax.tick_params(axis='y', labelsize=tick_fontsize)



def plot_for_mem(mem):

	fig = plt.figure(figsize=(8, 1.75))
	gs = GridSpec(1, 2, figure=fig)
	ax0 = fig.add_subplot(gs[0, 0:1])
	ax1 = fig.add_subplot(gs[0, 1:2])

	x_axis_label_list = [
		"2",
		"4",
		"8",
		"16",
		"32",
	]

	algotype = "ab_hybacc"
	scale = 19

	datatype = "er"
	scale = f"{scale}"
	dataset_list = [
		f"{datatype}_{scale}_{edgefactor}" for edgefactor in x_axis_label_list
	]
	plot_in_ax(
		ax0, mem, 
		algotype,
		datatype,
		0,
		dataset_list,
		x_axis_label_list
	)

	datatype = "g500"
	scale = f"{scale}"
	dataset_list = [
		f"{datatype}_{scale}_{edgefactor}" for edgefactor in x_axis_label_list
	]
	plot_in_ax(
		ax1, mem, 
		algotype,
		datatype,
		0,
		dataset_list,
		x_axis_label_list
	)

	for ax, axid, datatype, scale in zip(
		[
			ax0,
			ax1,
		],
		[
			"a",
			"b",
		],
		[
			"ER",
			"G500",
		],
		[
			scale,
			scale,
		]
	):
		ax.set_xlabel("Edgefactor",
			fontsize=label_fontsize,
			labelpad=labelpad,
		)
		ax.set_title(
			f"({axid}) {datatype} (scale={scale})", 
			fontsize=title_fontsize, fontweight="bold", 
			y=title_y_coord
		)

	ax0.set_ylabel("GFLOPS", fontsize=label_fontsize)
	ax0_handles, ax0_labels = ax0.get_legend_handles_labels()
	fig.legend(
		ax0_handles, ax0_labels, 
		loc="upper center", bbox_to_anchor=(0.5, 1.06),
		fancybox=True, shadow=True, frameon=False,
		ncol=len(cfg_list), 
		fontsize=legend_fontsize, 
		columnspacing=1.15
	)


	# plt.savefig(os.path.join(FIG_DIR, f"daha_{mem}.png"), bbox_inches="tight", format="png")
	# plt.title("", fontsize=default_fontsize)
	plt.savefig(os.path.join(FIG_DIR, f"daha_{mem}.pdf"), bbox_inches="tight", format="pdf")
	plt.savefig(os.path.join(FIG_DIR, f"daha_{mem}.eps"), bbox_inches="tight", format="eps")

	plt.close()




if __name__ == "__main__":
	if not os.path.exists(FIG_DIR):
		os.makedirs(FIG_DIR)

	for mem in [
		"1",
		# "6",
	]:
		plot_for_mem(mem)


