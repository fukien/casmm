import matplotlib.pyplot as plt
from matplotlib.gridspec import GridSpec
import numpy as np
import os


LOG_DIR="../../logs/20251013-logs"
FIG_DIR="../../figs/20251013-figs"


default_fontsize = 14
label_fontsize = 12
tick_fontsize = 11
title_fontsize = 14
legend_fontsize = 12
rotation = 0
title_y_coord = -0.55
labelpad = -0.7


legned_list = [
	"AMS",
	"MKL",
]



legend2color = {
	"AMS": "#ff7f0e",	# orange
	"MKL": "#bcbd22", 	# olive
}


def round6decimals(value):
	return round(float(value), 6)


def get_tps_from_file(filepath, dataset_list):
	# symb_time_list = [np.nan for _ in range(len(dataset_list))]
	numc_time_list = [np.nan for _ in range(len(dataset_list))]
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
					numc_time_list[idx] = round6decimals(line[7].split()[-1])
					tot_time_list[idx] = round6decimals(line[8].split()[-1])
					flop_cnt_list[idx] = round6decimals(line[9].split()[-1]) / (10**9)
	return np.array(flop_cnt_list)/np.array(numc_time_list)



def plot_in_ax(
	ax, mem, 
	datatype, 
	scale,
	tasktype,
	rotation, 
):

	x_axis_label_list = [
		"2",
		"4",
		"8",
		"16",
		"32",
		"64",
	]


	dataset_list = [
		f"{datatype}_{scale}_{edgefactor}" for edgefactor in x_axis_label_list
	]

	if tasktype == "spmm":
		algo_list = [
			"ab_spmm",
			"mkl_sparse_d_mm",
		]

	else:
		algo_list = [
			"ab_spmv",
			"mkl_cspblas_dcsrgemv",
		]


	x_axis = np.arange(len(x_axis_label_list)) + 1
	x_data_width = 0.74
	bar_width = x_data_width / len(algo_list) * 3 / 4
	x_starting_idx_list = [
		-x_data_width / 2 + (i + 1 / 2) * x_data_width / len(algo_list)
		for i in range(len(algo_list))
	]

	global_tps_list_of_list = []

	for algo_idx, algo in enumerate(algo_list):
		filepath = f"{LOG_DIR}/{datatype}_{mem}_{algo}.log"
		cur_tps_list = get_tps_from_file(filepath, dataset_list)
		legend = legned_list[algo_idx]

		ax.bar(
			x_axis+x_starting_idx_list[algo_idx],
			cur_tps_list,
			width=bar_width,
			edgecolor="black",
			color=legend2color[legend],
			label=legend,
		)

		global_tps_list_of_list.append(cur_tps_list)


	print(mem, 
		f"{algo_list[0]}/{algo_list[1]}", global_tps_list_of_list[0]/global_tps_list_of_list[1],
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

	fig = plt.figure(figsize=(8, 4))
 
	# --- THIS IS THE MODIFIED LINE ---
	# Adjust the hspace value (e.g., 1.0) as needed to increase/decrease the gap
	gs = GridSpec(2, 2, figure=fig, hspace=0.6)
	# -----------------------------------

	ax0 = fig.add_subplot(gs[0, 0:1])
	ax1 = fig.add_subplot(gs[0, 1:2])
	ax2 = fig.add_subplot(gs[1, 0:1])
	ax3 = fig.add_subplot(gs[1, 1:2])



	datatype_list = [
		"er", "g500",	# spmm
		"er", "g500",	# spmv
	]

	scale_list = [
		21, 20,
		21, 20,
	]


	task_type_list = [
		"spmm", "spmm",
		"spmv", "spmv"
	]

	ax_list = [
		ax0, ax1,
		ax2, ax3
	]


	for datatype, scale, tasktype, ax, axid in zip(
		datatype_list,
		scale_list,
		task_type_list,
		ax_list,
		["a", "b", "c", "d"]
	):

		# (Fixed non-ASCII spaces in this function call)
		plot_in_ax(
			ax, mem, 
			datatype,
			scale,
			tasktype,
			0
		)


		ax.set_xlabel("Edgefactor",
			fontsize=label_fontsize,
			labelpad=labelpad,
		)
		ax.set_title(
			f"({axid}) {datatype.upper()} (scale={scale})", 
			fontsize=title_fontsize, fontweight="bold", 
			y=title_y_coord
		)



	ax0.set_ylabel("GFLOPS", fontsize=label_fontsize)
	ax2.set_ylabel("GFLOPS", fontsize=label_fontsize)

	ax0_handles, ax0_labels = ax0.get_legend_handles_labels()
	fig.legend(
		ax0_handles, ax0_labels, 
		loc="upper center", bbox_to_anchor=(0.5, 0.99),
		fancybox=True, shadow=True, frameon=False,
		ncol=len(legned_list), 
		fontsize=legend_fontsize, 
		columnspacing=1.15
	)


	# --- ALTERNATIVE METHOD ---
	# If you didn't add hspace to GridSpec, you could instead
	# add this line right before plt.savefig():
	# fig.subplots_adjust(hspace=1.0)
	# --------------------------

	plt.savefig(os.path.join(FIG_DIR, f"spmmmv_{mem}.pdf"), bbox_inches="tight", format="pdf")
	plt.savefig(os.path.join(FIG_DIR, f"spmmmv_{mem}.eps"), bbox_inches="tight", format="eps")

	plt.close()



if __name__ == "__main__":
	if not os.path.exists(FIG_DIR):
		os.makedirs(FIG_DIR)

	for mem in [
		"1",
		# "6",
	]:
		plot_for_mem(mem)






