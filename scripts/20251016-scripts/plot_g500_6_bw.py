import matplotlib.pyplot as plt
from matplotlib.gridspec import GridSpec
import numpy as np
import os


LOG_DIR="../../logs/20251016-logs"
FIG_DIR="../../figs/20251016-figs"


default_fontsize = 14
label_fontsize = 12.5
tick_fontsize = 12
title_fontsize = 14
legend_fontsize = 12.5
rotation = 0
title_y_coord = -0.55
labelpad = -0.7



# bw numbers
MEM_2_BW = 28.5
MEM_4_BW = 20.1
MEM_6_BW = 46.5




algo_list = [
	"ab_hybacc_daha",
	"ab_hash_daha",
	"hash",
	# "mkl",
	"ab_hsersc_daha",
	"hsersc",
	# "heap",
	# "pb_1024_32",
	# "lla",
	# "mkls",
]

algo2legend = {
	"ab_hybacc_daha": "AMS-HYB",
	"ab_hash_daha": "AMS-HASH",
	"hash": "HASH",
	"ab_hsersc_daha": "AMS-ESC",
	"hsersc": "ESC",
	"heap": "heap",
	"pb_1024_32": "pb",
	"lla": "SA",
	"mkl": "MKL",
	"mkls": "MKLS",
}

# Distinct, color-blind friendly colors
algo2color = {
	"ab_hybacc_daha": "#FFC107",

	"ab_hash_daha": "#e377c2",  # pink/magenta
	"hash": "#1f77b4",          # blue

	"ab_hsersc_daha": "#ff7f0e",# orange
	"hsersc": "#2ca02c",        # green

	"heap": "#d62728",          # red
	"pb_1024_32": "#9467bd",            # purple
	"lla": "#8c564b",           # brown

	"mkl": "#17becf",       # cyan
	"mkls": "#bcbd22",      # olive
}

# Distinct marker shapes for Matplotlib
algo2marker = {
	"ab_hybacc_daha": "*",  	# star (New)

	"ab_hash_daha": "o",        # circle
	"hash": "s",                # square

	"ab_hsersc_daha": "D",      # diamond
	"hsersc": "^",              # triangle up

	"heap": "v",                # triangle down
	"pb_1024_32": "X",                  # X
	"lla": "P",                 # plus-filled pentagon

	"mkl": ">",             # triangle right
	"mkls": "<",            # triangle left
}

algo2hatch = {
	"ab_hybacc_daha": "...",    # dots (New)
	"ab_hash_daha": "///",   # dense forward slashes
	"hash": "xxx", # diagonal crosses
	"ab_hsersc_daha": "\\\\\\",        # dense backslashes
	"hsersc": "++",          # plus grid
	"heap": "..",            # dotted
	"pb_1024_32": "**",              # stars
	"lla": "--",             # horizontal dashed
	"mkl": "oo",         # circles
	"mkls": "||",        # vertical bars
}

algo2hatch = {
	"ab_hybacc_daha": "....",    # dots (New)
	"ab_hash_daha": "////",   # dense forward slashes
	"hash": "",
	"ab_hsersc_daha": "\\\\\\\\",        # dense backslashes
	"hsersc": "",
	"heap": "",
	"pb_1024_32": "",
	"lla": "",
	"mkl": "",
	"mkls": "",
}


algo2symbol = {
	"ab_hash_daha": "circle",
	"hash": "square",

	"ab_hsersc_daha": "diamond",
	"hsersc": "triangle-up",

	"heap": "triangle-down",
	"pb_1024_32": "x",
	"lla": "pentagon",

	"mkl": "triangle-right",
	"mkls": "triangle-left"
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
	return np.array(flop_cnt_list)/np.array(tot_time_list)





def get_bw_llc_miss_from_file(filepath, datasetlist):
	symb_time_list = [np.nan for _ in range(len(datasetlist))]
	numc_time_list = [np.nan for _ in range(len(datasetlist))]
	tot_time_list = [np.nan for _ in range(len(datasetlist))]
	flop_cnt_list = [np.nan for _ in range(len(datasetlist))]
	numc_size_llc_miss_list = [np.nan for _ in range(len(datasetlist))]
	numc_size_llc_ref_list = [np.nan for _ in range(len(datasetlist))]

	tmp_bw_numc_llc_miss = 0
	tmp_bw_numc_llc_ref = 0
	with open(filepath, "r") as f:
		while True:
			line = f.readline()
			if not line:
				break
			if "PERF_COUNT_HW_CACHE_MISSES" in line:
				line = line.strip().split("\t")
				tmp_bw_numc_llc_miss = int(line[-1])
				continue
			if "PERF_COUNT_HW_CACHE_REFERENCES" in line:
				line = line.strip().split("\t")
				tmp_bw_numc_llc_ref = int(line[-1])
				continue
			if "dataset" in line:
				line = line.strip().split("\t")
				dataset = line[0].split()[-1]
				if dataset in datasetlist:
					idx = datasetlist.index(dataset)
					symb_time_list[idx] = round6decimals(line[5].split()[-1])
					numc_time_list[idx] = round6decimals(line[7].split()[-1])
					tot_time_list[idx] = round6decimals(line[8].split()[-1])
					flop_cnt_list[idx] = int(line[9].split()[-1])
					numc_size_llc_miss_list[idx] = tmp_bw_numc_llc_miss * 64 / (2**30)
					numc_size_llc_ref_list[idx] = tmp_bw_numc_llc_ref * 64 / (2**30)
				continue

	return (np.array(flop_cnt_list)/np.array(tot_time_list)).tolist(), \
		(np.array(numc_size_llc_miss_list)/np.array(numc_time_list)).tolist(), \
		(np.array(numc_size_llc_ref_list)/np.array(numc_time_list)).tolist()





def plot_in_ax(
	ax, mem, 
	datatype, 
	scale,
	rotation, 
):

	x_axis_label_list = [
		"2",
		"4",
		"8",
		"16",
		# "32",
	]


	dataset_list = [
		f"{datatype}_{scale}_{edgefactor}" for edgefactor in x_axis_label_list
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


	print(mem, 
		f"{algo_list[0]}/{algo_list[1]}", global_tps_list_of_list[1]/global_tps_list_of_list[2],
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

	fig = plt.figure(figsize=(7.5, 4))
 
	# --- THIS IS THE MODIFIED LINE ---
	# Adjust the hspace value (e.g., 1.0) as needed to increase/decrease the gap
	# gs = GridSpec(2, 2, figure=fig, hspace=0.6)
	gs = GridSpec(2, 2, figure=fig, hspace=0.6, wspace=0.225)
	# -----------------------------------

	ax0 = fig.add_subplot(gs[0, 0:1])
	ax1 = fig.add_subplot(gs[0, 1:2])
	ax2 = fig.add_subplot(gs[1, 0:1])
	ax3 = fig.add_subplot(gs[1, 1:2])



	datatype_list = [
		"g500", 
		"g500",	
		"g500",	
	]

	scale_list = [
		17, 
		18,
		19,
	]

	ax_list = [
		ax0, 
		ax1,
		ax2
	]


	for datatype, scale, ax, axid in zip(
		datatype_list,
		scale_list,
		ax_list,
		["a", "b", "c",]
	):

		# (Fixed non-ASCII spaces in this function call)
		plot_in_ax(
			ax, mem, 
			datatype,
			scale,
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




	print()

# datatype = "ssca"
# scale = 20

	datatype = "g500"
	scale = 19

	x_axis_label_list = [
		"2",
		"4",
		"8",
		"16",
		# "32",
	]
	dataset_list = [
		f"{datatype}_{scale}_{edgefactor}" for edgefactor in x_axis_label_list
	]

	x_axis = np.arange(len(x_axis_label_list)) + 1
	x_data_width = 0.74
	bar_width = x_data_width / len(algo_list) * 3 / 4
	x_starting_idx_list = [
		-x_data_width / 2 + (i + 1 / 2) * x_data_width / len(algo_list)
		for i in range(len(algo_list))
	]

	for algo_idx, algo in enumerate(algo_list):
		filepath = f"{LOG_DIR}/llc_{datatype}_{mem}_{algo}.log"
		_, cur_bw_llc_miss_list, _ = get_bw_llc_miss_from_file(filepath, dataset_list)
		if algo.startswith("ab_"):
			cur_bw_llc_miss_list = np.array(cur_bw_llc_miss_list)
		ax3.bar(
			x_axis+x_starting_idx_list[algo_idx],
			cur_bw_llc_miss_list,
			width=bar_width,
			edgecolor="black",
			color=algo2color[algo],
			hatch=algo2hatch[algo],
			label=algo2legend[algo],
		)

		print(algo, cur_bw_llc_miss_list)



	ax3.axhline(
		y=MEM_6_BW,             # The y-position of the line
		color='red',      # The color
		linestyle="--"
	)

	ax3.axhline(
		y=MEM_2_BW,
		color='blue',
		# linewidth=1,
		linestyle='--'    # You can also change the line style
	)

	ax3.grid()
	ax3.set_xticks(x_axis)
	ax3.set_xticklabels(
		x_axis_label_list,
		rotation=0,
		fontsize=tick_fontsize
	)
	ax3.set_ylim(0, 48)
	ax3.tick_params(axis='y', labelsize=tick_fontsize)
	ax3.set_yticks(
		[
			10,
			20,
			30,
			40,
			50
		]
	)
	ax3.set_yticklabels(
		[
			10,
			20,
			30,
			40,
			50
		],
		fontsize=tick_fontsize
	)

	ax3.set_ylabel("Bandwidth (GB/s)", fontsize=label_fontsize)

	ax3.set_xlabel("Edgefactor",
		fontsize=label_fontsize,
		labelpad=labelpad,
	)

	# use ssca_20 to fake g500_19
	ax3.set_title(
		f"(d) G500 (scale=19)", 
		fontsize=title_fontsize, fontweight="bold", 
		y=title_y_coord
	)


	fig.legend(
		ax0_handles, ax0_labels, 
		loc="upper center", bbox_to_anchor=(0.5, 0.99),
		fancybox=True, shadow=True, frameon=False,
		ncol=len(algo_list), 
		fontsize=legend_fontsize, 
		columnspacing=1.15
	)


	# --- ALTERNATIVE METHOD ---
	# If you didn't add hspace to GridSpec, you could instead
	# add this line right before plt.savefig():
	# fig.subplots_adjust(hspace=1.0)
	# --------------------------

	plt.savefig(os.path.join(FIG_DIR, f"g500_bw_llc_miss_{mem}.pdf"), bbox_inches="tight", format="pdf")
	plt.savefig(os.path.join(FIG_DIR, f"g500_bw_llc_miss_{mem}.eps"), bbox_inches="tight", format="eps")

	plt.close()



if __name__ == "__main__":
	if not os.path.exists(FIG_DIR):
		os.makedirs(FIG_DIR)

	for mem in [
		# "1",
		"6",
	]:
		plot_for_mem(mem)






