import collections
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
from matplotlib.gridspec import GridSpec
import numpy as np
import os
import pandas as pd
import seaborn as sns
import sys

from utils import *

LOG_DIR="../../logs/20251014-logs"
FIG_DIR = "../../figs/20251014-figs"

RUN_NUM=5
THREAD_NUM=32

default_fontsize = 14
label_fontsize = 11
tick_fontsize = 9
title_fontsize = 13
legend_fontsize = 12
rotation = 0
title_y_coord = -0.5
labelpad = -0.7

def plot_in_ax(
	ax, mem, 
	datatype, 
	rotation, 
	dataset_list, 
	x_axis_label_list
):
	x_axis = np.arange(len(x_axis_label_list)) + 1
	x_data_width = 0.86
	bar_width = x_data_width / len(algo_list) * 3 / 4
	x_starting_idx_list = [
		-x_data_width / 2 + (i + 1 / 2) * x_data_width / len(algo_list)
		for i in range(len(algo_list))
	]

	global_tps_list_of_list = []
	for algo_idx, algo in enumerate(algo_list):
		filepath = f"{LOG_DIR}/{datatype}_{mem}_{algo}.log"
		cur_tps_list = get_tps_from_file(filepath, dataset_list)
		ax.bar(
			x_axis+x_starting_idx_list[algo_idx],
			cur_tps_list,
			width=bar_width,
			edgecolor="black",
			color=algo2color[algo],
			hatch=algo2hatch[algo],
			label=algo2legend[algo],
		)

		global_tps_list_of_list.append(cur_tps_list)


	ab_hash_idx = algo_list.index("ab_hash_daha")
	hash_idx = algo_list.index("hash")
	hash_ratio_list = global_tps_list_of_list[ab_hash_idx]/global_tps_list_of_list[hash_idx]
	ab_hsersc_idx = algo_list.index("ab_hsersc_daha")
	hsersc_idx = algo_list.index("hsersc")
	hsersc_ratio_list = global_tps_list_of_list[ab_hsersc_idx]/global_tps_list_of_list[hsersc_idx]

	print(
		mem, 
		f"ab_hash: {min(hash_ratio_list)}-{max(hash_ratio_list)}",
		f"ab_hsersc: {min(hsersc_ratio_list)}-{max(hsersc_ratio_list)}",
	)

	ax.grid()
	ax.set_xticks(x_axis)
	ax.set_xticklabels(
		x_axis_label_list,
		rotation=rotation,
		fontsize=tick_fontsize
	)
	ax.set_xlim(0.5, len(x_axis_label_list) + 0.5)


def plot_4(mem):
	fig = plt.figure(
		figsize=(
			14, 
			8 # 9
		), 
		constrained_layout=True)
	gs = GridSpec(4, 3, figure=fig, 
		# height_ratios=[1, 1, 1, 1.5]
	)

	ax0 = fig.add_subplot(gs[0, 0:1])
	ax1 = fig.add_subplot(gs[0, 1:2])
	ax2 = fig.add_subplot(gs[0, 2:3])
	ax3 = fig.add_subplot(gs[1, 0:1])
	ax4 = fig.add_subplot(gs[1, 1:2])
	ax5 = fig.add_subplot(gs[1, 2:3])
	ax6 = fig.add_subplot(gs[2, 0:1])
	ax7 = fig.add_subplot(gs[2, 1:2])
	ax8 = fig.add_subplot(gs[2, 2:3])
	ax9 = fig.add_subplot(gs[3, :])

	for ax in [ax0, ax3, ax6, ax9]:
		ax.set_ylabel("GFLOPS", fontsize=label_fontsize)

	for ax in [
		ax0, ax1, ax2,
		ax3, ax4, ax5,
		ax6, ax7, ax8
	]:
		ax.set_xlabel("Edgefactor",
			fontsize=label_fontsize,
			labelpad=labelpad,
		)

	for ax, axid, datatype, scale in zip(
		[
			ax0, ax1, ax2,
			ax3, ax4, ax5,
			ax6, ax7, ax8
		], 
		[
			"a", "b", "c",
			"d", "e", "f",
			"g", "h", "i",

		],
		[
			"er", "er", "er",
			"ssca", "ssca", "ssca",
			"g500", "g500", "g500",
		],
		[
			19, 20, 21,
			18, 19, 20,
			17, 18, 19
		]
	):
		ax.set_title(
			f"({axid}) {datatype.upper()} (scale={scale})", 
			fontsize=title_fontsize, fontweight="bold", 
			y=title_y_coord
		)

	ax9.set_title(
		f"(j) SuiteSparse Matrix Benchmark", 
		fontsize=title_fontsize, fontweight="bold", 
		y=title_y_coord-0.40
		# y=title_y_coord-0.05

	)


	# rmat
	x_axis_label_list = [
		"2",
		"4",
		"8",
		"16",
		"32",
	]

	datatype = "er"
	scale = "19"
	dataset_list = [
		f"{datatype}_{scale}_{edgefactor}" for edgefactor in x_axis_label_list
	]
	plot_in_ax(
		ax0, mem, 
		datatype,
		rotation, 
		dataset_list,
		x_axis_label_list,	
	)

	scale = "20"
	dataset_list = [
		f"{datatype}_{scale}_{edgefactor}" for edgefactor in x_axis_label_list
	]
	plot_in_ax(
		ax1, mem, 
		datatype,
		rotation, 
		dataset_list,
		x_axis_label_list,	
	)

	scale = "21"
	dataset_list = [
		f"{datatype}_{scale}_{edgefactor}" for edgefactor in x_axis_label_list
	]
	plot_in_ax(
		ax2, mem, 
		datatype,
		rotation, 
		dataset_list,
		x_axis_label_list,	
	)

	# ssca
	datatype = "ssca"
	scale = "18"
	dataset_list = [
		f"{datatype}_{scale}_{edgefactor}" for edgefactor in x_axis_label_list
	]
	plot_in_ax(
		ax3, mem, 
		datatype,
		rotation, 
		dataset_list,
		x_axis_label_list,	
	)

	scale = "19"
	dataset_list = [
		f"{datatype}_{scale}_{edgefactor}" for edgefactor in x_axis_label_list
	]
	plot_in_ax(
		ax4, mem, 
		datatype,
		rotation, 
		dataset_list,
		x_axis_label_list,	
	)

	scale = "20"
	dataset_list = [
		f"{datatype}_{scale}_{edgefactor}" for edgefactor in x_axis_label_list
	]
	plot_in_ax(
		ax5, mem, 
		datatype,
		rotation, 
		dataset_list,
		x_axis_label_list,	
	)


	# g500
	datatype = "g500"
	scale = "17"
	dataset_list = [
		f"{datatype}_{scale}_{edgefactor}" for edgefactor in x_axis_label_list
	]
	plot_in_ax(
		ax6, mem, 
		datatype,
		rotation, 
		dataset_list,
		x_axis_label_list,	
	)

	scale = "18"
	dataset_list = [
		f"{datatype}_{scale}_{edgefactor}" for edgefactor in x_axis_label_list
	]
	plot_in_ax(
		ax7, mem, 
		datatype,
		rotation, 
		dataset_list,
		x_axis_label_list,	
	)

	scale = "19"
	dataset_list = [
		f"{datatype}_{scale}_{edgefactor}" for edgefactor in x_axis_label_list
	]
	plot_in_ax(
		ax8, mem, 
		datatype,
		rotation, 
		dataset_list,
		x_axis_label_list,	
	)

	plot_in_ax(
		ax9, mem, 
		"tamu",
		-40,
		tamu_dataset_list,
		tamu_dataset_list
	)



	ax0_handles, ax0_labels = ax0.get_legend_handles_labels()
	fig.legend(
		ax0_handles, ax0_labels, 
		loc="upper center", bbox_to_anchor=(0.5, 1.04),
		fancybox=True, shadow=True, frameon=False,
		ncol=len(algo_list), 
		fontsize=legend_fontsize, 
		columnspacing=1.15
	)



	# plt.savefig(os.path.join(FIG_DIR, f"rmat_tamu_{mem}_4.png"), bbox_inches="tight", format="png")
	# plt.title("", fontsize=default_fontsize)
	plt.savefig(os.path.join(FIG_DIR, f"rmat_tamu_{mem}_4.pdf"), bbox_inches="tight", format="pdf")
	plt.savefig(os.path.join(FIG_DIR, f"rmat_tamu_{mem}_4.eps"), bbox_inches="tight", format="eps")

	plt.close()



if __name__ == "__main__":
	if not os.path.exists(FIG_DIR):
		os.makedirs(FIG_DIR)

	for mem in [
		# "6", 
		"1"
	]:
		plot_4(mem)

