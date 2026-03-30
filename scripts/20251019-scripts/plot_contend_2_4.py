import collections
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
from matplotlib.gridspec import GridSpec
import numpy as np
import os
import pandas as pd
import seaborn as sns
import sys

LOG_DIR="../../logs/20251019-logs"
FIG_DIR = "../../figs/20251019-figs"

RUN_NUM=3
THREAD_NUM=32

default_fontsize = 14
label_fontsize = 14
tick_fontsize = 13
title_fontsize = 14


def round6decimals(value):
	return round(float(value), 6)


def average_per_thread(data, run_num, thread_num):
	# Exclude the first run
	data = data[thread_num:run_num * thread_num]
	runs = [data[i * thread_num:(i + 1) * thread_num] for i in range(run_num - 1)]
	per_thread_data = list(zip(*runs))  # transpose
	averaged = []
	for values in per_thread_data:
		averaged.append(round6decimals(sum(values) / len(values)))
	return averaged

def extract_multithread(filepath):
	tmp_thr_numc_time_list = []
	tmp_thr_llc_miss_list = []
	tmp_thr_llc_access_list = []
	tmp_thr_ins_list = []

	with open(filepath, "r") as f:
		while True:
			line = f.readline()
			if not line:
				break
			if "_numc_time:" in line:
				line = line.strip().split(": ")
				tmp_thr_numc_time = round6decimals(line[1])
				tmp_thr_numc_time_list.append(tmp_thr_numc_time)
				continue
			if "_numc_perf:" in line:
				line = line.strip().split("\t")
				tmp_llc_miss = int(line[0].split(": ")[-1])
				tmp_llc_access = int(line[1].split(": ")[-1])
				tmp_ins = int(line[2].split(": ")[-1])
				tmp_thr_llc_miss_list.append(tmp_llc_miss)
				tmp_thr_llc_access_list.append(tmp_llc_access)
				tmp_thr_ins_list.append(tmp_ins)
				continue

	# Calculate per-thread averages across RUN_NUM-1 runs
	avg_thr_numc_time = average_per_thread(tmp_thr_numc_time_list, RUN_NUM, THREAD_NUM)
	avg_thr_llc_miss = average_per_thread(tmp_thr_llc_miss_list, RUN_NUM, THREAD_NUM)
	avg_thr_llc_access = average_per_thread(tmp_thr_llc_access_list, RUN_NUM, THREAD_NUM)
	avg_thr_ins = average_per_thread(tmp_thr_ins_list, RUN_NUM, THREAD_NUM)

	return avg_thr_numc_time, avg_thr_llc_miss, avg_thr_llc_access, avg_thr_ins


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
					flop_cnt_list[idx] = int(line[9].split()[-1])

	return np.array(flop_cnt_list)/np.array(tot_time_list)




if __name__ == "__main__":
	if not os.path.exists(FIG_DIR):
		os.makedirs(FIG_DIR)

	fig = plt.figure(figsize=(14, 4), constrained_layout=True)
	gs = GridSpec(2, 17, figure=fig)

	ax0 = fig.add_subplot(gs[0, 0:14])
	ax1 = fig.add_subplot(gs[0, 14:17])
	ax2 = fig.add_subplot(gs[1, 0:4])
	ax3 = fig.add_subplot(gs[1, 4:8])
	ax4 = fig.add_subplot(gs[1, 8:12])
	ax5 = fig.add_subplot(gs[1, 12:17])
	
	# ax0.grid()
	# ax1.grid()
	# ax2.grid()
	# ax3.grid()
	# ax4.grid()
	ax5.grid()

	title_y_coord = -0.465
	ax0.set_title("(a) Per-thread runtime in multithreaded execution", 
		fontsize=title_fontsize, fontweight="bold", y=title_y_coord)
	ax1.set_title("(b) Single-thread runtime", 
		fontsize=title_fontsize, fontweight="bold", y=title_y_coord)

	title_y_coord = -0.56
	ax2.set_title("(c) #LLC-miss", 
		fontsize=title_fontsize, fontweight="bold", y=title_y_coord)
	ax3.set_title("(d) #LLC-access", 
		fontsize=title_fontsize, fontweight="bold", y=title_y_coord)
	ax4.set_title("(e) #instruction", 
		fontsize=title_fontsize, fontweight="bold", y=title_y_coord)
	ax5.set_title("(f) Relative throughput with SMT enabled", 
		fontsize=title_fontsize, fontweight="bold", y=title_y_coord)

	ax0.set_ylabel("Elapsed Time (s)", fontsize=label_fontsize)
	ax2.set_ylabel("Event Count", fontsize=label_fontsize)
	ax5.set_ylabel("Relative Throughput", fontsize=label_fontsize)

	default_avg_thr_numc_time, \
		default_avg_thr_llc_miss, \
		default_avg_thr_llc_access, \
		default_avg_thr_ins = \
			extract_multithread(f"{LOG_DIR}/numc_comp_6_g500_19_8.log")

	first_avg_thr_numc_time, \
		first_avg_thr_llc_miss, \
		first_avg_thr_llc_access, \
		first_avg_thr_ins = \
			extract_multithread(f"{LOG_DIR}/numc_comp_6_g500_19_8_0th.log")

	last_avg_thr_numc_time, \
		last_avg_thr_llc_miss, \
		last_avg_thr_llc_access, \
		last_avg_thr_ins = \
			extract_multithread(f"{LOG_DIR}/numc_comp_6_g500_19_8_31st.log")


	st_first_avg_thr_numc_time, \
		st_first_avg_thr_llc_miss, \
		st_first_avg_thr_llc_access, \
		st_first_avg_thr_ins = \
			extract_multithread(f"{LOG_DIR}/numc_comp_6_g500_19_8_0th_st.log")

	st_last_avg_thr_numc_time, \
		st_last_avg_thr_llc_miss, \
		st_last_avg_thr_llc_access, \
		st_last_avg_thr_ins = \
			extract_multithread(f"{LOG_DIR}/numc_comp_6_g500_19_8_31st_st.log")



	cfg_list = [
		"default",
		"first",
		"last",
	]

	cfg2color = {
		"default": "coral",
		"first": "lightskyblue",
		"last": "chartreuse",
	}

	cfg2hatch = {
		"default": "",
		"first": "////",
		"last": "\\\\\\\\",
	}

	cfg2legend = {
		"default": "default",
		"first": "high-$cf$",
		"last": "low-$cf$",
	}

	# thread-level runtime
	x_axis = np.arange(THREAD_NUM) + 1
	x_data_width = 0.86
	bar_width = x_data_width / len(cfg_list) * 3 / 4
	x_starting_idx_list = [
		-x_data_width / 2 + (i + 1 / 2) * x_data_width / len(cfg_list)
		for i in range(len(cfg_list))
	]

	for cfg, cfg_thr_numc_time in zip(
			cfg_list, 
			[
				default_avg_thr_numc_time,
				first_avg_thr_numc_time,
				last_avg_thr_numc_time,
			]
		):

		cfg_idx = cfg_list.index(cfg)
		ax0.bar(
			x_axis+x_starting_idx_list[cfg_idx],
			cfg_thr_numc_time,
			bar_width,
			edgecolor="black",
			color=cfg2color[cfg], 
			label=cfg2legend[cfg],
			hatch=cfg2hatch[cfg],
		)
	ax0.set_xticks(x_axis)
	ax0.set_xticklabels(
		[str(i) for i in range(THREAD_NUM)], 
		rotation=-45, fontsize=tick_fontsize
	)

	ax0_handles, ax0_labels = ax0.get_legend_handles_labels()
	fig.legend(ax0_handles, ax0_labels, 
		loc="upper center", bbox_to_anchor=(0.5, 1.08),
		fancybox=True, shadow=True, frameon=False,
		ncol=len(cfg_list), 
		fontsize=default_fontsize, 
		columnspacing=1.15
	)





	# single-thread vs multi-thread
	x_axis_label_list = ["first", "last"]
	x_axis = [0.3, 0.7]
	bar_width = 0.3

	tmp_first_list = [
		st_first_avg_thr_numc_time[0],
		0.0
	]
	tmp_last_list = [
		0.0,
		st_last_avg_thr_numc_time[THREAD_NUM-1],
	]
	ax1.bar(
		x_axis,
		tmp_first_list,
		width=bar_width,
		edgecolor="black",
		color=cfg2color["first"],
		hatch=cfg2hatch["first"],
	)
	ax1.bar(
		x_axis,
		tmp_last_list,
		width=bar_width,
		edgecolor="black",
		color=cfg2color["last"],
		hatch=cfg2hatch["last"],
	)
	ax1.set_xticks([0, 1])
	ax1.axes.get_xaxis().set_visible(False)


	# LLC-miss
	x_axis_label_list = ["single-thread", "multithreaded"]
	x_axis = np.arange(len(x_axis_label_list)) + 1
	cfg_list = ["first", "last"]
	x_data_width = 0.7
	bar_width = x_data_width / len(cfg_list) * 3 / 4
	x_starting_idx_list = [
		-x_data_width / 2 + (i + 1 / 2) * x_data_width / len(cfg_list)
		for i in range(len(cfg_list))
	]

	tmp_first_list = [
		st_first_avg_thr_llc_miss[0],
		first_avg_thr_llc_miss[0],
	]
	tmp_last_list = [
		st_last_avg_thr_llc_miss[THREAD_NUM-1],
		last_avg_thr_llc_miss[THREAD_NUM-1],
	]
	ax2.bar(
		x_axis+x_starting_idx_list[0],
		tmp_first_list,
		width=bar_width,
		edgecolor="black",
		color=cfg2color["first"],
		hatch=cfg2hatch["first"],
	)
	ax2.bar(
		x_axis+x_starting_idx_list[1],
		tmp_last_list,
		width=bar_width,
		edgecolor="black",
		color=cfg2color["last"],
		hatch=cfg2hatch["last"],
	)
	ax2.set_xticks(x_axis)
	ax2.set_xticklabels(
		x_axis_label_list,
		fontsize=tick_fontsize
	)


	# LLC-access
	tmp_first_list = [
		st_first_avg_thr_llc_access[0],
		first_avg_thr_llc_access[0],
	]
	tmp_last_list = [
		st_last_avg_thr_llc_access[THREAD_NUM-1],
		last_avg_thr_llc_access[THREAD_NUM-1],
	]
	ax3.bar(
		x_axis+x_starting_idx_list[0],
		tmp_first_list,
		width=bar_width,
		edgecolor="black",
		color=cfg2color["first"],
		hatch=cfg2hatch["first"],
	)
	ax3.bar(
		x_axis+x_starting_idx_list[1],
		tmp_last_list,
		width=bar_width,
		edgecolor="black",
		color=cfg2color["last"],
		hatch=cfg2hatch["last"],
	)
	ax3.set_xticks(x_axis)
	ax3.set_xticklabels(
		x_axis_label_list,
		fontsize=tick_fontsize
	)


	# instruction
	tmp_first_list = [
		st_first_avg_thr_ins[0],
		first_avg_thr_ins[0],
	]
	tmp_last_list = [
		st_last_avg_thr_ins[THREAD_NUM-1],
		last_avg_thr_ins[THREAD_NUM-1],
	]
	ax4.bar(
		x_axis+x_starting_idx_list[0],
		tmp_first_list,
		width=bar_width,
		edgecolor="black",
		color=cfg2color["first"],
		hatch=cfg2hatch["first"],
	)
	ax4.bar(
		x_axis+x_starting_idx_list[1],
		tmp_last_list,
		width=bar_width,
		edgecolor="black",
		color=cfg2color["last"],
		hatch=cfg2hatch["last"],
	)
	ax4.set_xticks(x_axis)
	ax4.set_xticklabels(
		x_axis_label_list,
		fontsize=tick_fontsize
	)


	# normalized throughput of SMT
	er_dataset_list = [
		"er_18_2",
		"er_18_4",
		"er_18_8",
		"er_18_16",
		"er_18_32",
	]
	er_tps_demoniator = get_tps_from_file(
		f"{LOG_DIR}/er_6_ab_hash.log",
		er_dataset_list,
	)
	er_tps_numerator = get_tps_from_file(
		f"{LOG_DIR}/er_6_ab_hash_hyp.log",
		er_dataset_list,
	)
	er_tps_ratio_list = er_tps_numerator/er_tps_demoniator


	g500_dataset_list = [
		"g500_18_2",
		"g500_18_4",
		"g500_18_8",
		"g500_18_16",
		"g500_18_32",
	]
	g500_tps_demoniator = get_tps_from_file(
		f"{LOG_DIR}/g500_6_ab_hash.log",
		g500_dataset_list,
	)
	g500_tps_numg500ator = get_tps_from_file(
		f"{LOG_DIR}/g500_6_ab_hash_hyp.log",
		g500_dataset_list,
	)
	g500_tps_ratio_list = g500_tps_numg500ator/g500_tps_demoniator

	cfg_list = [
		"ER",
		"G500"
	]

	cfg2color = {
		"ER": "hotpink",
		"G500": "cornflowerblue",
	}

	x_axis_label_list = [
		"2", 
		"4",
		"8",
		"16",
		"32"
	]
	x_axis = np.arange(len(x_axis_label_list)) + 1
	x_data_width = 0.86
	bar_width = x_data_width / len(cfg_list) * 3 / 4
	x_starting_idx_list = [
		-x_data_width / 2 + (i + 1 / 2) * x_data_width / len(cfg_list)
		for i in range(len(cfg_list))
	]

	ax5.bar(
		x_axis+x_starting_idx_list[0],
		er_tps_ratio_list,
		width=bar_width,
		edgecolor="black",
		color=cfg2color["ER"],
		label="ER",
		hatch="||||",
	)
	ax5.bar(
		x_axis+x_starting_idx_list[1],
		g500_tps_ratio_list,
		width=bar_width,
		edgecolor="black",
		color=cfg2color["G500"],
		label="G500",
		hatch="xxxx",
	)
	ax5.set_xticks(x_axis)
	ax5.set_xticklabels(
		x_axis_label_list,
		fontsize=tick_fontsize
	)
	ax5.set_xticklabels(
		x_axis_label_list,
		fontsize=tick_fontsize
	)
	ax5.set_xlabel("Edgefactor",
		fontsize=label_fontsize-1,
		labelpad=-1.25,
	)
	ax5.axhline(y=1, color="black", linestyle="--")
	ax5_handles, ax5_labels = ax5.get_legend_handles_labels()
	ax5.legend(
		ax5_handles, ax5_labels, loc='upper center', 
		ncol=len(cfg_list), fontsize=label_fontsize-1.25,
		bbox_to_anchor=(0.5, 1.264),
		frameon=False  # ← this removes the border box
	)



	# plt.savefig(os.path.join(FIG_DIR, "contend_2_4.png"), bbox_inches="tight", format="png")
	# plt.title("", fontsize=default_fontsize)
	plt.savefig(os.path.join(FIG_DIR, "contend_2_4.pdf"), bbox_inches="tight", format="pdf")
	plt.savefig(os.path.join(FIG_DIR, "contend_2_4.eps"), bbox_inches="tight", format="eps")

	plt.close()
