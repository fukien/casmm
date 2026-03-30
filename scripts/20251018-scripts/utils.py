import matplotlib.pyplot as plt
import numpy as np
import os
import plotly.graph_objects as go
import sys

LOG_DIR = f"../../logs/20251018-logs"
FIG_DIR = f"../../figs/20251018-figs"


default_fontsize = 15
default_figsize = (12, 10)
marker_size = 7.5
tick_angle = 45


algo_list = [
	# "ab_spmm",
	"ab_spmm_hyp",
	# "mkl_dcsrmm",
	# "mkl_dcsrmm_hyp",
	"mkl_sparse_d_mm",
	# "mkl_sparse_d_mm_hyp",
]

# Distinct, color-blind friendly colors
algo2color = {
	"ab_spmm":  "#1f77b4",  # strong blue
	"ab_spmm_hyp": "#ff7f0e", # orange
	"mkl_dcsrmm":  "#2ca02c",  # green
	"mkl_dcsrmm_hyp":  "#d62728",  # red
	"mkl_sparse_d_mm":   "#17becf",  # cyan
	"mkl_sparse_d_mm_hyp":  "#e377c2",  # pink/magenta
}

# Distinct marker shapes for Matplotlib
algo2marker = {
	"ab_spmm":  "o",  # circle
	"ab_spmm_hyp": "s",  # square
	"mkl_dcsrmm":  "D",  # diamond
	"mkl_dcsrmm_hyp":  "^",  # triangle up
	"mkl_sparse_d_mm":   "<",  # triangle left
	"mkl_sparse_d_mm_hyp":  "X",  # X shape
}

# Matching symbolic names for Plotly or alternate legends
algo2symbol = {
	"ab_spmm":  "circle",
	"ab_spmm_hyp": "square",
	"mkl_dcsrmm":  "diamond",
	"mkl_dcsrmm_hyp":  "triangle-up",
	"mkl_sparse_d_mm":   "triangle-left",
	"mkl_sparse_d_mm_hyp":  "x"
}


def round6decimals(value):
	return round(float(value), 6)