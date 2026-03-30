import matplotlib.pyplot as plt
import numpy as np
import os


LOG_DIR="../../logs/20251017-logs"
FIG_DIR="../../figs/20251017-figs"



dataset_2_baseline_tps = {
	"webbase-1M": 1525659247.930457830,
}


def round6decimals(value):
	return round(float(value), 6)


def get_tps_llc_miss(filepath):
	with open(filepath, "r") as f:
		while True:
			line = f.readline()
			if not line:
				break
			if "dataset:" in line:
				line = line.strip().split("\t")
				tps = round6decimals(line[-1].split()[-1])
			if "PERF_COUNT_HW_CACHE_MISSES" in line:
				line = line.strip().split("\t")
				llc_miss = int(line[-1])
	return tps/(10**9), llc_miss/(10**6)


def plot_tamu(dataset):
	ddnmin_list = [
		1, 
		2, 
		4, 
		8,
		16,
		32,
		64,
		128,
		256,
		512,
	]


	tps_list = []
	llc_miss_list = []

	for ddnmin in ddnmin_list:
		tps, llc_miss = get_tps_llc_miss(
			f"{LOG_DIR}/{dataset}_1_ddnmin{ddnmin}_hash.log"
		)
		tps_list.append(tps)
		llc_miss_list.append(llc_miss)


	# print(tps_list)
	# print(llc_miss_list)

	default_fontsize = 25
	linewidth = 4
	plt.figure(figsize=(6, 5))
	ax = plt.gca()  # Get current Axes

	x_axis = np.arange(len(ddnmin_list))
	color = "tab:red"
	ax.plot(
		x_axis, 
		tps_list,
		color=color,
		linewidth=linewidth,
	)
	baseline_tps = dataset_2_baseline_tps[dataset] / (10**9)
	ax.axhline(
		y=baseline_tps,
		linestyle="--",
		color=color,
		linewidth=linewidth,
	)


	# ax.annotate(
	# 	"flop-count-based\nscheme",
	# 	xy=(len(ddnmin_list) - 3.65, baseline_tps),
	# 	xytext=(len(ddnmin_list) - 3.6, baseline_tps * 0.86),
	# 	textcoords="data",
	# 	arrowprops=dict(arrowstyle="<-", color=color),
	# 	color=color,
	# 	fontsize=default_fontsize-1,
	# 	ha="center",   # ⬅️ This centers the text block horizontally
	# 	va="bottom"
	# )

	ax.set_ylabel("Throughput (GFLOPS)",
		color=color,
		fontsize=default_fontsize
	)
	ax.tick_params(
		axis="y", 
		labelcolor=color,
		labelsize=default_fontsize
	)


	color = "tab:blue"
	ax_twinx = ax.twinx()  # instantiate a second Axes that shares the same x-axis
	ax_twinx.plot(
		x_axis,
		llc_miss_list,
		color=color,
		linewidth=linewidth,
	)
	ax_twinx.set_ylabel("#LLC-miss (millions)",
		color=color,
		fontsize=default_fontsize
	)
	ax_twinx.tick_params(
		axis="y", 
		labelcolor=color,
		labelsize=default_fontsize
	)


	ax.grid()
	ax.set_xticks(x_axis)
	ax.set_xticklabels(
		[str(i) for i in ddnmin_list],
		rotation=-60,
		fontsize=default_fontsize
	)
	ax.set_xlabel(
		"Morsel size (consec. rows)",
		fontsize=default_fontsize
	)
	

	plt.savefig(os.path.join(FIG_DIR, f"{dataset}-tps-llc_miss.pdf"), 
		bbox_inches="tight", format="pdf"
	)
	plt.savefig(os.path.join(FIG_DIR, f"{dataset}-tps-llc_miss.eps"), 
		bbox_inches="tight", format="eps"
	)

	plt.close()



if __name__ == "__main__":
	if not os.path.exists(FIG_DIR):
		os.makedirs(FIG_DIR)

	dataset = "webbase-1M"
	plot_tamu(dataset)