import matplotlib.pyplot as plt
import numpy as np
import os
import plotly.graph_objects as go
import sys


algo_list = [
	"ab_hybacc_daha",
	"ab_hash_daha",
	"hash",
	"mkl",
	"ab_hsersc_daha",
	"hsersc",
	"heap",
	"pb_1024_32",
	# "lla",
	"mkls",
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


er_dataset_list = [
	"er_18_1",
	"er_18_2",
	"er_18_4",
	"er_18_8",
	"er_18_16",
	"er_18_32",
	"er_19_1",
	"er_19_2",
	"er_19_4",
	"er_19_8",
	"er_19_16",
	"er_19_32",
	"er_20_1",
	"er_20_2",
	"er_20_4",
	"er_20_8",
	"er_20_16",
	"er_20_32",
	"er_21_1",
	"er_21_2",
	"er_21_4",
	"er_21_8",
	"er_21_16",
	"er_21_32",
]

g500_dataset_list = [
	"g500_17_1",
	"g500_17_2",
	"g500_17_4",
	"g500_17_8",
	"g500_17_16",
	"g500_17_32",
	"g500_18_1",
	"g500_18_2",
	"g500_18_4",
	"g500_18_8",
	"g500_18_16",
	"g500_18_32",
	"g500_19_1",
	"g500_19_2",
	"g500_19_4",
	"g500_19_8",
	"g500_19_16",
	"g500_19_32",
	"g500_20_1",
	"g500_20_2",
	"g500_20_4",
	"g500_20_8",
	"g500_20_16",
	"g500_20_32",
]

ssca_dataset_list = [
	"ssca_17_1",
	"ssca_17_2",
	"ssca_17_4",
	"ssca_17_8",
	"ssca_17_16",
	"ssca_17_32",
	"ssca_18_1",
	"ssca_18_2",
	"ssca_18_4",
	"ssca_18_8",
	"ssca_18_16",
	"ssca_18_32",
	"ssca_19_1",
	"ssca_19_2",
	"ssca_19_4",
	"ssca_19_8",
	"ssca_19_16",
	"ssca_19_32",
	"ssca_20_1",
	"ssca_20_2",
	"ssca_20_4",
	"ssca_20_8",
	"ssca_20_16",
	"ssca_20_32",
]

tamu_dataset_list = [
	"shar_te2-b3",
	"m133-b3",
	"ch8-8-b5",
	"shar_te2-b2",
	"ch7-8-b4",
	"ch7-8-b5",
	"ch8-8-b3",
	"n4c6-b8",
	"n4c6-b7",
	"ch7-9-b5",
	"ch7-9-b4",
	"ch8-8-b4",
	"debr",
	"hugetric-00020",
	"hugetric-00010",
	"hugetric-00000",
	"adaptive",
	"af_shell10",
	"af_2_k101",
	"af_0_k101",
	"af_1_k101",
	"af_5_k101",
	"af_3_k101",
	"af_4_k101",
	"af_shell7",
	"af_shell5",
	"af_shell9",
	"af_shell4",
	"af_shell8",
	"af_shell3",
	"af_shell1",
	"af_shell6",
	"af_shell2",
	"BenElechi1",
	"channel-500x100x100-b050",
	"Hardesty3",
	"Rucci1",
	"nlpkkt160",
	"Transport",
	"nlpkkt120",
	"CurlCurl_4",
	"Emilia_923",
	"nlpkkt80",
	"relat8",
	"GL7d18",
	"Geo_1438",
	"Bump_2911",
	"fcondp2",
	"Hardesty2",
	"Fault_639",
	"GL7d17",
	"333SP",
	"CO",
	"StocF-1465",
	"GL7d16",
	"NLR",
	"M6",
	"AS365",
	"packing-500x100x100-b050",
	"troll",
	"halfb",
	"fullb",
	"GL7d15",
	"auto",
	"bmwcra_1",
	"Serena",
	"delaunay_n23",
	"delaunay_n22",
	"msdoor",
	"ldoor",
	"nv2",
	"rgg_n_2_23_s0",
	"x104",
	"rgg_n_2_22_s0",
	"amazon0601",
	"rgg_n_2_21_s0",
	"ohne2",
	"cage15",
	"cage14",
	"amazon0505",
	"cage13",
	"test1",
	"radiation",
	"amazon0312",
	"Freescale1",
	"dielFilterV2real",
	"PR02R",
	"Hook_1498",
	"PFlow_742",
	"fem_hifreq_circuit",
	"dgreen",
	"dielFilterV2clx",
	"gsm_106857",
	"3Dspectralwave",
	"pkustk14",
	"Si87H76",
	"ss",
	"amazon-2008",
	"webbase-1M",
	"pre2",
	"vas_stokes_2M",
	"Ga10As10H30",
	"rel9",
	"vas_stokes_1M",
	"kkt_power",
	"c-big",
	"Ge87H76",
	"Ga19As19H42",
	"Ge99H100",
	"email-EuAll",
	"web-Google",
	"Linux_call_graph",
	"patents",
	"cnr-2000",
	"cit-Patents",
	"NotreDame_www",
	"web-NotreDame",
	"soc-sign-epinions",
	"wiki-talk-temporal",
]


tamu_dataset_list = [
	"ch7-9-b5",
	"hugetric-00020",
	"hugetric-00010",
	"hugetric-00000",
	"adaptive",
	"channel-500x100x100-b050",
	"Hardesty3",
	"CurlCurl_4",
	"GL7d18",
	# "Hardesty2",
	"GL7d17",
	"333SP",
	"GL7d16",
	"NLR",
	"M6",
	"AS365",
	"GL7d15",
	"auto",
	"delaunay_n23",
	"delaunay_n22",

	"nv2",
	"amazon0601",
	"amazon0505",
	"amazon0312",
	"Freescale1",
	"dgreen",
	"gsm_106857",
	"amazon-2008",
	"webbase-1M",
	"vas_stokes_2M",
	"rel9",
	"vas_stokes_1M",
	"kkt_power",
	"c-big",
	"email-EuAll",
	"web-Google",
	"patents",
	"cnr-2000",
	"cit-Patents",
	"NotreDame_www",
	"web-NotreDame",
	"soc-sign-epinions",
	"wiki-talk-temporal",
]


tamu_dataset_list = [
	# "ch7-9-b5",
	"hugetric-00020",
	"hugetric-00010",
	# "hugetric-00000",
	# "adaptive",
	# "Hardesty3",
	# "CurlCurl_4",
	"GL7d18",
	"GL7d17",
	"333SP",
	"GL7d16",
	"NLR",
	"M6",
	"AS365",
	"GL7d15",
	"auto",
	"delaunay_n23",
	"delaunay_n22",
	"nv2",
	# "amazon0601",
	# "amazon0505",
	"Freescale1",
	"dgreen",
	"gsm_106857",
	"amazon-2008",
	"webbase-1M",
	"vas_stokes_2M",
	"rel9",
	"vas_stokes_1M",
	# "kkt_power",
	# "c-big",
	"email-EuAll",
	"web-Google",
	"patents",
	"cit-Patents",
	"NotreDame_www",
	"web-NotreDame",
	# "soc-sign-epinions",
	# "wiki-talk-temporal",
]


tamu_dataset_list = [
	"hugetric-00020",
	# "hugetric-00010",
	"GL7d18",
	# "GL7d17",
	"333SP",
	# "GL7d16",
	"NLR",
	"M6",
	"AS365",
	# "GL7d15",
	"auto",
	"delaunay_n23",
	# "delaunay_n22",
	"nv2",
	"Freescale1",
	# "dgreen",
	"gsm_106857",
	"amazon-2008",
	"webbase-1M",
	"vas_stokes_2M",
	"rel9",
	"vas_stokes_1M",
	# "email-EuAll",
	# "web-Google",
	"patents",
	"cit-Patents",
	"NotreDame_www",
	"web-NotreDame",
]



tamu_dataset_list = [
	"hugetric-00020",
	# "hugetric-00010",
	"GL7d18",
	# "GL7d17",
	"333SP",
	# "GL7d16",
	"NLR",
	"M6",
	"AS365",
	# "GL7d15",
	"auto",
	"delaunay_n23",
	# "delaunay_n22",
	"nv2",
	"Freescale1",
	# "dgreen",
	"gsm_106857",
	"amazon-2008",
	"webbase-1M",
	"vas_stokes_2M",
	"rel9",
	"vas_stokes_1M",
	# "email-EuAll",
	# "web-Google",
	"patents",
	"cit-Patents",
	"NotreDame_www",
	"web-NotreDame",
]




'''
# gini
tamu_dataset_list = [
	"ch7-9-b5",
	"333SP",
	"GL7d16",
	"auto",
	"delaunay_n23",
	"nv2",
	"Freescale1",
	"dgreen",
	"gsm_106857",
	"amazon-2008",
	"webbase-1M",
	"rel9",
	"patents",
	"cit-Patents",
	"web-NotreDame",
]


# cfo
tamu_dataset_list = [
	"rel9",
	"webbase-1M",
	"GL7d16",
	"ch7-9-b5",
	"patents",
	"cit-Patents",
	"web-NotreDame",
	"delaunay_n23",
	"333SP",
	"Freescale1",
	"auto",
	"amazon-2008",
	"dgreen",
	"nv2",
	"gsm_106857",
]
'''


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
				# if dataset == "ssca_20_16" or dataset == "ssca_19_32":
				# 	continue

				if dataset == "g500_18_32" or dataset == "g500_19_16":
					if "heap" in filepath:
						continue
				if dataset in dataset_list:
					idx = dataset_list.index(dataset)
					# symb_time_list[idx] = round6decimals(line[5].split()[-1])
					# numc_time_list[idx] = round6decimals(line[7].split()[-1])
					tot_time_list[idx] = round6decimals(line[8].split()[-1])
					flop_cnt_list[idx] = round6decimals(line[9].split()[-1]) / (10**9)

	return np.array(flop_cnt_list)/np.array(tot_time_list)

