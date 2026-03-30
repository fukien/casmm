import matplotlib.pyplot as plt
import numpy as np
from scipy.io import mmread
from scipy.sparse import csr_matrix
import os
import sys

FIG_DIR="../../figs/20251017-figs"


if __name__ == "__main__":
	dataset = "webbase-1M"

	# Load matrix from Matrix Market format
	A = mmread(f"../../dataset/tamu/{dataset}/{dataset}.mtx")
	A = csr_matrix(A)

	# Create a spy plot (non-zero pattern)
	plt.figure(figsize=(6, 6))
	# plt.spy(A, markersize=0.5)  # markersize controls the dot size
	plt.spy(A, markersize=0.5, rasterized=True)


	plt.xticks([])
	plt.yticks([])
	plt.box(False)

	# Save in vector format
	plt.savefig(
		f"{FIG_DIR}/{dataset}_adj_nnz.pdf", 
		bbox_inches="tight", format="pdf"
	)

	plt.savefig(
		f"{FIG_DIR}/{dataset}_adj_nnz.eps", 
		bbox_inches="tight", format="eps"
	)

	plt.close()