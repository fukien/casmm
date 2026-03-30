import numpy as np
import os
import scipy.io
import sys
import time


if __name__ == "__main__":
	dataset = sys.argv[1]
	org_filepath = "dataset/tamu/{}/{}.org.mtx".format(dataset, dataset)
	trs_filepath = "dataset/tamu/{}/{}.trs.mtx".format(dataset, dataset)

	# org_filepath = "dataset/rmat/{}.org.mtx".format(dataset)
	# trs_filepath = "dataset/rmat/{}.trs.mtx".format(dataset)

	start_time = time.time()

	mat_org = scipy.io.mmread(org_filepath)
	mat_trs = scipy.io.mmread(trs_filepath)
	mat_org = mat_org.tocsr()
	mat_trs = mat_trs.tocsr()

	end_time = time.time()
	load_time = end_time - start_time

	start_time = time.time()

	mat_res = mat_org @ mat_trs

	end_time = time.time()
	mlp_time = end_time - start_time

	# print(mat_res.indptr.shape)
	# print(mat_res.indices.shape)
	# print(mat_res.data.shape)

	# print(np.any(mat_res.data==0))
	# print(mat_res.indices[10:17])
	# print(mat_res.data[10:17])

	print("load_time: ", load_time)
	print("mlp_time: ", mlp_time)
	# print()
	print("VERIFYRESULT:", mat_res.nnz, np.sum(mat_res.indices), np.sum(mat_res.data))
	# print()