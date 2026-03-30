import matplotlib.pyplot as plt
import numpy as np
import os
import plotly.graph_objects as go
import sys

LOG_DIR = f"../../logs/20251011-logs"
FIG_DIR = f"../../figs/20251011-figs"

default_fontsize = 15
default_figsize = (12, 10)
marker_size = 7.5
tick_angle = 45


er_dataset_list = [
	"er_18_1",
	"er_18_2",
	"er_18_4",
	"er_18_8",
	"er_18_16",
	"er_18_32",
	"er_18_64",
	"er_19_1",
	"er_19_2",
	"er_19_4",
	"er_19_8",
	"er_19_16",
	"er_19_32",
	"er_19_64",
	"er_20_1",
	"er_20_2",
	"er_20_4",
	"er_20_8",
	"er_20_16",
	"er_20_32",
	"er_20_64",
	"er_21_1",
	"er_21_2",
	"er_21_4",
	"er_21_8",
	"er_21_16",
	"er_21_32",
	"er_21_64",
]
g500_dataset_list = [
	# "g500_17_1",
	# "g500_17_2",
	# "g500_17_4",
	# "g500_17_8",
	# "g500_17_16",
	# "g500_17_32",
	# "g500_17_64",
	# "g500_18_1",
	# "g500_18_2",
	# "g500_18_4",
	# "g500_18_8",
	# "g500_18_16",
	# "g500_18_32",
	# "g500_18_64",
	# "g500_19_1",
	"g500_19_2",
	"g500_19_4",
	"g500_19_8",
	"g500_19_16",
	"g500_19_32",
	# "g500_19_64",
	# "g500_20_1",
	# "g500_20_2",
	# "g500_20_4",
	# "g500_20_8",
	# "g500_20_16",
	# "g500_20_32",
	# "g500_20_64",
]
ssca_dataset_list = [
	"ssca_17_1",
	"ssca_17_2",
	"ssca_17_4",
	"ssca_17_8",
	"ssca_17_16",
	"ssca_17_32",
	"ssca_17_64",
	"ssca_18_1",
	"ssca_18_2",
	"ssca_18_4",
	"ssca_18_8",
	"ssca_18_16",
	"ssca_18_32",
	"ssca_18_64",
	"ssca_19_1",
	"ssca_19_2",
	"ssca_19_4",
	"ssca_19_8",
	"ssca_19_16",
	"ssca_19_32",
	"ssca_19_64",
	"ssca_20_1",
	"ssca_20_2",
	"ssca_20_4",
	"ssca_20_8",
	"ssca_20_16",
	"ssca_20_32",
	"ssca_20_64",
]
datatype_2_datasetlist = {
	"er": er_dataset_list,
	"g500": g500_dataset_list,
	"ssca": ssca_dataset_list,
}

cfg_list = [
	"daha",
	"daha_simp",
	"daha_lpt",
]

algo_list = [
	"ab_hybacc",
	"ab_hash",
	"ab_hsersc",
]

setup_list = [
	"BGS-vs-CAMB",
	"LPT-vs-CAMB"
]

setup2color = {
	"BGS-vs-CAMB":  "#1f77b4",  # strong blue
	"LPT-vs-CAMB": "#ff7f0e", # orange
}

setup2marker = {
	"BGS-vs-CAMB":  "o",  # circle
	"LPT-vs-CAMB": "s",  # square
}



def round6decimals(value):
	return round(float(value), 6)


def get_time_from_file(filepath, dataset_list):
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
					flop_cnt_list[idx] = int(line[9].split()[-1])
	return symb_time_list, numc_time_list, tot_time_list, \
		(np.array(flop_cnt_list)/np.array(symb_time_list)).tolist(), \
		(np.array(flop_cnt_list)/np.array(numc_time_list)).tolist(), \
		(np.array(flop_cnt_list)/np.array(tot_time_list)).tolist()





def plot_perf_in_ax(ax, bar_width, alpha, 
	x_axis, x_starting_idx_list, dataset_list, 
	time_list_of_y_list, perf_list_of_y_list):
	ax_y_sceond = ax.twinx()
	for setup_idx, setup in enumerate(setup_list):
		# ax.bar(
		# 	x_axis+x_starting_idx_list[setup_idx], 
		# 	time_list_of_y_list[setup_idx], 
		# 	bar_width, 
		# 	color=setup2color[setup], 
		# 	label=setup
		# )
		ax_y_sceond.plot(
			x_axis,
			perf_list_of_y_list[setup_idx],
			color=setup2color[setup],
			linestyle="-",
			marker=setup2marker[setup],
		)
	ax.grid()
	ax.set_xticks(x_axis)
	ax.set_xticklabels(dataset_list, rotation=45, fontsize=default_fontsize/2)
	ax.set_ylabel("time (s)", fontsize=default_fontsize)
	ax_y_sceond.set_ylabel("FLOP/s", fontsize=default_fontsize)
	ax_y_sceond.set_ylim(0, None)





def plot_datatype(algo, memcfg, datatype, dataset_list):
	x_axis_list = dataset_list
	x_axis = np.arange(len(x_axis_list)) + 1
	x_data_width = 0.8
	bar_width = x_data_width / len(setup_list) * 3 / 4
	x_starting_idx_list = [
		-x_data_width / 2 + (i + 1 / 2) * x_data_width / len(setup_list)
		for i in range(len(setup_list))
	]
	alpha = 0.5

	tmp_symb_time_list_of_list = []
	tmp_numc_time_list_of_list = []
	tmp_tot_time_list_of_list = []
	tmp_symb_perf_list_of_list = []
	tmp_numc_perf_list_of_list = []
	tmp_tot_perf_list_of_list = []


	for cfg in cfg_list:
		filepath = f"{LOG_DIR}/{datatype}_{memcfg}_{algo}_{cfg}.log"
		cur_symb_time_list, cur_numc_time_list, cur_tot_time_list, \
			cur_symb_perf_list, cur_numc_perf_list, cur_tot_perf_list = \
			get_time_from_file(filepath, dataset_list)
		tmp_symb_time_list_of_list.append(cur_symb_time_list)
		tmp_numc_time_list_of_list.append(cur_numc_time_list)
		tmp_tot_time_list_of_list.append(cur_tot_time_list)
		tmp_symb_perf_list_of_list.append(cur_symb_perf_list)
		tmp_numc_perf_list_of_list.append(cur_numc_perf_list)
		tmp_tot_perf_list_of_list.append(cur_tot_perf_list)

	cur_symb_time_ratio_list_of_list = []
	cur_numc_time_ratio_list_of_list = []
	cur_tot_time_ratio_list_of_list = []
	cur_symb_perf_ratio_list_of_list = []
	cur_numc_perf_ratio_list_of_list = []
	cur_tot_perf_ratio_list_of_list = []

	for i in range(1, len(cfg_list)):
		cur_symb_time_ratio = np.array(tmp_symb_time_list_of_list[i]) / \
			np.array(tmp_symb_time_list_of_list[0])
		cur_numc_time_ratio = np.array(tmp_numc_time_list_of_list[i]) / \
			np.array(tmp_numc_time_list_of_list[0])
		cur_tot_time_ratio = np.array(tmp_tot_time_list_of_list[i]) / \
			np.array(tmp_tot_time_list_of_list[0])

		cur_symb_perf_ratio = np.array(tmp_symb_perf_list_of_list[0]) / \
			np.array(tmp_symb_perf_list_of_list[i])
		cur_numc_perf_ratio = np.array(tmp_numc_perf_list_of_list[0]) / \
			np.array(tmp_numc_perf_list_of_list[i])
		cur_tot_perf_ratio = np.array(tmp_tot_perf_list_of_list[0]) / \
			np.array(tmp_tot_perf_list_of_list[i])

		print(algo, cur_numc_perf_ratio)


		cur_symb_time_ratio_list_of_list.append(cur_symb_time_ratio)
		cur_numc_time_ratio_list_of_list.append(cur_numc_time_ratio)
		cur_tot_time_ratio_list_of_list.append(cur_tot_time_ratio)

		cur_symb_perf_ratio_list_of_list.append(cur_symb_perf_ratio)
		cur_numc_perf_ratio_list_of_list.append(cur_numc_perf_ratio)
		cur_tot_perf_ratio_list_of_list.append(cur_tot_perf_ratio)



	fig, axes = plt.subplots(3, 1, figsize=default_figsize)
	plot_perf_in_ax(axes[0], bar_width, alpha,
		x_axis, x_starting_idx_list, dataset_list, 
		cur_symb_time_ratio_list_of_list, cur_symb_perf_ratio_list_of_list
	)
	plot_perf_in_ax(axes[1], bar_width, alpha,
		x_axis, x_starting_idx_list, dataset_list, 
		cur_numc_time_ratio_list_of_list, cur_numc_perf_ratio_list_of_list
	)
	plot_perf_in_ax(axes[2], bar_width, alpha, 
		x_axis, x_starting_idx_list, dataset_list, 
		cur_tot_time_ratio_list_of_list, cur_tot_perf_ratio_list_of_list
	)

	axes[0].set_title("symb", fontsize=default_fontsize)
	axes[1].set_title("numc", fontsize=default_fontsize)
	axes[2].set_title("tot", fontsize=default_fontsize)

	all_handles, all_labels = axes[0].get_legend_handles_labels()
	fig.legend(
		all_handles, all_labels, loc='upper center', 
		ncol=len(setup_list), fontsize=default_fontsize,
		bbox_to_anchor=(0.5, 0.9625)
	)
	fig.suptitle(f"perf_{datatype}_{memcfg}_{algo}", fontsize=default_fontsize)
	plt.tight_layout(rect=[0, 0, 1, 0.95])  # Adjust layout to avoid overlapping with suptitle

	plt.savefig(
		f"{FIG_DIR}/perf_{datatype}_{memcfg}_{algo}.pdf",
		bbox_inches="tight", format="pdf"
	)

	plt.close()



if __name__ == "__main__":
	if not os.path.exists(FIG_DIR):
		os.makedirs(FIG_DIR)

	for datatype in [
		# "er", 
		"g500", 
		# "ssca"
	]:
		for memcfg in [
			"6",
		]:
			for algo in algo_list:
				plot_datatype(
					algo, memcfg, datatype, 
					datatype_2_datasetlist[datatype]
				)
