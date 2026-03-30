#include "inc/utils.h"
#include "mm/mm_utils.h"

extern numa_cfg_t numa_cfg;

int main (int argc, char** argv) {
	assert (argc == 2);
	char *dataset = argv[1];

	numa_cfg_init(&numa_cfg);

	int cur_numa_idx = LOCAL_NUMA_NODE_IDX - 1;
	char matrix_src_org_path[CHAR_BUFFER_LEN];
	char matrix_src_trs_path[CHAR_BUFFER_LEN];
	snprintf(matrix_src_org_path, CHAR_BUFFER_LEN, 
		"%s/%s.org.mtx", RMAT_DIR, dataset
	);
	snprintf(matrix_src_trs_path, CHAR_BUFFER_LEN, 
		"%s/%s.trs.mtx", RMAT_DIR, dataset
	);

	triple_t* trpl = NULL;
	my_cnt_t num_row, num_col, num_nnz;

	mtx2trpl(
		&trpl, &num_row, &num_col, &num_nnz, 
		matrix_src_org_path, cur_numa_idx
	);
	dealloc_triv_dram(trpl, sizeof(triple_t) * num_nnz);
	printf("num_nnz in org: %ld\n", num_nnz);
	num_row = num_col = num_nnz = 0;
	mtx2trpl(
		&trpl, &num_row, &num_col, &num_nnz, 
		matrix_src_trs_path, cur_numa_idx
	);
	dealloc_triv_dram(trpl, sizeof(triple_t) * num_nnz);
	printf("num_nnz in trs: %ld\n", num_nnz);

	numa_cfg_free(&numa_cfg);

	return 0;
}