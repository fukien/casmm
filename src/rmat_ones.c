#include "inc/utils.h"
#include "mm/mm_utils.h"

extern numa_cfg_t numa_cfg;

int main (int argc, char** argv) {
	assert (argc == 2);
	char *dataset = argv[1];

	numa_cfg_init(&numa_cfg);

	int __attribute__((unused)) cur_numa_idx = LOCAL_NUMA_NODE_IDX - 1;
	char matrix_src_path[CHAR_BUFFER_LEN];
	char matrix_dest_org_path[CHAR_BUFFER_LEN];
	char matrix_dest_trs_path[CHAR_BUFFER_LEN];

	snprintf(matrix_src_path, CHAR_BUFFER_LEN, 
		"%s/%s.txt", RMAT_DIR, dataset
	);
	snprintf(matrix_dest_org_path, CHAR_BUFFER_LEN, 
		"%s/%s.org.mtx", RMAT_DIR, dataset
	);
 	snprintf(matrix_dest_trs_path, CHAR_BUFFER_LEN, 
		"%s/%s.trs.mtx", RMAT_DIR, dataset
	);

	txt_transpose(
		matrix_dest_trs_path, matrix_dest_org_path,
		matrix_src_path, 
		// cur_numa_idx
		0
	);

	numa_cfg_free(&numa_cfg);

	return 0;
}