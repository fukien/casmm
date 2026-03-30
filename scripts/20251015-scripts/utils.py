import matplotlib.pyplot as plt
import numpy as np
import os
import plotly.graph_objects as go
import sys

LOG_DIR = f"../../logs/20251015-logs"
FIG_DIR = f"../../figs/20251015-figs"


default_fontsize = 15
default_figsize = (12, 10)
marker_size = 7.5
tick_angle = 45


algo_list = [
	"ab_hash_daha",

	# "hyb80",
	"gudmin256",
	"ompdnr256",
	"ompgdr256_hyp",
]

algo2legend = {
	"ab_hash_daha": "AMS",   # magenta/pink
	"hyb80": "HYB",          # green
	"gudmin256": "guided-flop",      # orange
	"ompdnr256": "dynamic-row",      # strong blue
	"ompgdr256_hyp": "guided-row",  # red
}

# Distinct, color-blind friendly colors
algo2color = {
	"ab_hash_daha": "#e377c2",   # magenta/pink
	"hyb80": "#2ca02c",          # green
	"gudmin256": "#ff7f0e",      # orange
	"ompdnr256": "#1f77b4",      # strong blue
	"ompgdr256_hyp": "#d62728",  # red
}



# Distinct marker shapes for Matplotlib
algo2marker = {
	"ab_hash_daha": "o",     # circle
	"hyb80": "s",            # square
	"gudmin256": "D",        # diamond
	"ompdnr256": "^",        # triangle up
	"ompgdr256_hyp": "X",    # X shape
}



# Matching symbolic names for Plotly or alternate legends
algo2symbol = {
	"ab_hash_daha": "circle",
	"hyb80": "square",
	"gudmin256": "diamond",
	"ompdnr256": "triangle-up",
	"ompgdr256_hyp": "x"
}




def round6decimals(value):
	return round(float(value), 6)