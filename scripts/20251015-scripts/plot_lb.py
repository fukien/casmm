import matplotlib.pyplot as plt
from matplotlib.gridspec import GridSpec
import numpy as np
import os

from utils import *

LOG_DIR="../../logs/20251015-logs"
FIG_DIR="../../figs/20251015-figs"


default_fontsize = 14
label_fontsize = 11
tick_fontsize = 9
title_fontsize = 13
legend_fontsize = 12
rotation = 0
title_y_coord = -0.4
labelpad = -0.2


def get_tps_from_file(filepath, dataset_list):
	symb_time_list = [np.nan for _ in range(len(dataset_list))]
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
					symb_time_list[idx] = round6decimals(line[5].split()[-1])
					numc_time_list[idx] = round6decimals(line[7].split()[-1])
					tot_time_list[idx] = round6decimals(line[8].split()[-1])
					flop_cnt_list[idx] = round6decimals(line[9].split()[-1]) / (10**9)

	return np.array(symb_time_list), \
		np.array(numc_time_list), \
		np.array(tot_time_list), \
		np.array(flop_cnt_list)


def plot_in_ax(
	ax, mem, 
	datatype, 
	rotation, 
	dataset_list, 
	x_axis_label_list
):
	x_axis = np.arange(len(x_axis_label_list)) + 1
	x_data_width = 0.8
	bar_width = x_data_width / len(algo_list) * 3 / 4
	x_starting_idx_list = [
		-x_data_width / 2 + (i + 1 / 2) * x_data_width / len(algo_list)
		for i in range(len(algo_list))
	]

	symb_time_list_of_list = []
	numc_time_list_of_list = []
	tot_time_list_of_list = []
	for algo_idx, algo in enumerate(algo_list):
		filepath = f"{LOG_DIR}/{datatype}_{mem}_{algo}.log"
		cur_symb_time_list, cur_numc_time_list, \
			cur_tot_time_list, flop_cnt_list \
				= get_tps_from_file(filepath, dataset_list)
		symb_time_list_of_list.append(cur_symb_time_list)
		numc_time_list_of_list.append(cur_numc_time_list)
		tot_time_list_of_list.append(cur_tot_time_list)



	selected_symb_time_list = symb_time_list_of_list[
		algo_list.index("ompdnr256")
	]

	for algo_idx, algo in enumerate(algo_list):
		ax.bar(
			x_axis+x_starting_idx_list[algo_idx],
			# flop_cnt_list/tot_time_list_of_list[algo_idx],
			# flop_cnt_list/numc_time_list_of_list[algo_idx],
			flop_cnt_list/(numc_time_list_of_list[algo_idx]+selected_symb_time_list),
			width=bar_width,
			edgecolor="black",
			color=algo2color[algo],
			label=algo2legend[algo],
		)

	ax.grid()
	ax.set_xticks(x_axis)
	ax.set_xticklabels(
		x_axis_label_list,
		rotation=rotation,
		fontsize=tick_fontsize
	)

	print(
		mem,
		f"{algo_list[0]}/{algo_list[1]}", (numc_time_list_of_list[1]+selected_symb_time_list)/(numc_time_list_of_list[0]+selected_symb_time_list), 
		f"{algo_list[0]}/{algo_list[2]}", (numc_time_list_of_list[2]+selected_symb_time_list)/(numc_time_list_of_list[0]+selected_symb_time_list), 
		f"{algo_list[0]}/{algo_list[3]}", (numc_time_list_of_list[3]+selected_symb_time_list)/(numc_time_list_of_list[0]+selected_symb_time_list), 

		# f"{algo_list[0]}/{algo_list[1]}", tot_time_list_of_list[1]/tot_time_list_of_list[0], 
		# f"{algo_list[0]}/{algo_list[2]}", tot_time_list_of_list[2]/tot_time_list_of_list[0], 
		# f"{algo_list[0]}/{algo_list[3]}", tot_time_list_of_list[3]/tot_time_list_of_list[0], 

		# f"{algo_list[0]}/{algo_list[1]}", numc_time_list_of_list[1]/numc_time_list_of_list[0], 
		# f"{algo_list[0]}/{algo_list[2]}", numc_time_list_of_list[2]/numc_time_list_of_list[0], 
		# f"{algo_list[0]}/{algo_list[3]}", numc_time_list_of_list[3]/numc_time_list_of_list[0], 
	)


def plot_lb_cmp(mem):
	fig = plt.figure(figsize=(8, 1.75))
	ax = plt.gca()  # Get current Axes

	x_axis_label_list = [
		"2",
		"4",
		"8",
		"16",
		"32",
	]
	datatype = "g500"
	scale = "17"
	dataset_list = [
		f"{datatype}_{scale}_{edgefactor}" for edgefactor in x_axis_label_list
	]	

	plot_in_ax(
		ax, mem, 
		datatype,
		0,
		dataset_list,
		x_axis_label_list
	)



	ax.set_ylabel("GFLOPS", fontsize=label_fontsize)
	ax.set_xlabel("Edgefactor",
		fontsize=label_fontsize,
		labelpad=labelpad,
	)
	ax.set_ylim(0, 2.75)

	ax_handles, ax_labels = ax.get_legend_handles_labels()

	fig.legend(
		ax_handles, ax_labels, 
		loc="upper left", bbox_to_anchor=(0.125, 0.9),
		fancybox=False, shadow=True, frameon=True,
		ncol=len(algo_list)//2, 
		fontsize=legend_fontsize, 
		columnspacing=0.9
	)

	# plt.savefig(os.path.join(FIG_DIR, f"lb_{mem}_g500.png"), bbox_inches="tight", format="png")
	# plt.title("", fontsize=default_fontsize)
	plt.savefig(os.path.join(FIG_DIR, f"lb_{mem}_g500.pdf"), bbox_inches="tight", format="pdf")
	plt.savefig(os.path.join(FIG_DIR, f"lb_{mem}_g500.eps"), bbox_inches="tight", format="eps")

	plt.close()	


if __name__ == "__main__":
	if not os.path.exists(FIG_DIR):
		os.makedirs(FIG_DIR)

	for mem in [
		"1",
		"6"
	]:
		plot_lb_cmp(mem)