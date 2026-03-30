#include <mkl.h>

#include "mm/mm_utils.h"

/*** NUMA configuration ***/
extern numa_cfg_t numa_cfg;
extern int64_t file_idx;

#ifdef USE_PAPI
#include "papi/mypapi.h"
extern char* PAPI_event_names[NUM_PAPI_EVENT];
long long PAPI_values[NUM_PAPI_EVENT];
#endif /* USE_PAPI */

int local_numa_idx;
int cxl_numa_idx;
int cur_numa_idx;

#ifdef IN_VERIFY
extern int64_t over_predict;
extern int64_t under_predict;
#endif /* IN_VERIFY */



typedef struct {
	MKL_INT num_row;
	MKL_INT num_col;
	MKL_INT num_nnz;
	MKL_INT *row_ptrs;
	MKL_INT *cols;
	my_rat_t *rats;
} mkl_csr_t;


static void csr2mklcsr(csr_t *csr, mkl_csr_t *mkl_csr, numa_cfg_t *numa_cfg) {
	mkl_csr->num_row = csr->num_row;
	mkl_csr->num_col = csr->num_col;
	mkl_csr->num_nnz = csr->num_nnz;

	mkl_csr->row_ptrs = (MKL_INT*) alloc2_memory(
		sizeof(MKL_INT) * (mkl_csr->num_row+1), numa_cfg
	);
	memset(mkl_csr->row_ptrs, 0, sizeof(MKL_INT) * (mkl_csr->num_row+1) );

	mkl_csr->cols = (MKL_INT*) alloc2_memory(
		sizeof(MKL_INT) * mkl_csr->num_nnz, numa_cfg
	);
	memset(mkl_csr->cols, 0, sizeof(MKL_INT) * mkl_csr->num_nnz );

	mkl_csr->rats = (my_rat_t*) alloc2_memory(
		sizeof(my_rat_t) * mkl_csr->num_nnz, numa_cfg
	);
	memset(mkl_csr->rats, 0, sizeof(my_rat_t) * mkl_csr->num_nnz);

	for (MKL_INT idx = 0; idx < mkl_csr->num_nnz; idx ++) {
		mkl_csr->cols[idx] = csr->cols[idx];
		mkl_csr->rats[idx] = csr->rats[idx];
	}

	for (MKL_INT idx = 0; idx <= mkl_csr->num_row; idx ++) {
		mkl_csr->row_ptrs[idx] = csr->row_ptrs[idx];
	}
}


static void free_mklcsr(mkl_csr_t * mkl_csr) {
	dealloc_memory( mkl_csr->rats, sizeof(my_rat_t) * mkl_csr->num_nnz );
	dealloc_memory( mkl_csr->cols, sizeof(MKL_INT) * mkl_csr->num_nnz );
	dealloc_memory( mkl_csr->row_ptrs, sizeof(MKL_INT) * (mkl_csr->num_row+1) );
	mkl_csr->num_row = 0;
	mkl_csr->num_col = 0;
	mkl_csr->num_nnz = 0;
}


static runtime_t mklspmm(
	mkl_csr_t* mkl_csr_a, 
	sparse_matrix_t* sparse_opt_csr_a, 
	my_rat_t* mat_b, 
	my_rat_t **mat_c, 
	numa_cfg_t *numa_cfg) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

	mkl_set_num_threads(THREAD_NUM);

	struct timespec time_start, time_end;
	runtime_t runtime = {0.0, 0.0, 0.0, 0.0, 0.0};

	clock_gettime(CLOCK_REALTIME, &time_start);
	*mat_c = (my_rat_t *) alloc2_memory(
		sizeof(my_rat_t)*mkl_csr_a->num_row*SPMM_B_K, 
		numa_cfg
	);
	parallel_memset(
		*mat_c, 0, 
		sizeof(my_rat_t)*mkl_csr_a->num_row*SPMM_B_K, 
		THREAD_NUM
	);
	clock_gettime(CLOCK_REALTIME, &time_end);
	runtime.mgmt_time = diff_sec(time_start, time_end);

	clock_gettime(CLOCK_REALTIME, &time_start);
	double alpha = 1.0;   // Scalar alpha
	double beta = 0.0;    // Scalar beta
	MKL_INT mkl_spmm_k = SPMM_B_K;
	// MKL_INT* pntrb = mkl_csr_a->row_ptrs;
	// MKL_INT* pntre = mkl_csr_a->row_ptrs+1;

	struct matrix_descr descrOfA;
	descrOfA.type = SPARSE_MATRIX_TYPE_GENERAL;
	sparse_layout_t layout = SPARSE_LAYOUT_ROW_MAJOR;
	mkl_sparse_d_mm(
		SPARSE_OPERATION_NON_TRANSPOSE, 
		alpha, 
		*sparse_opt_csr_a, 
		descrOfA,
		layout, 
		mat_b, 
		mkl_spmm_k, 
		mkl_spmm_k,
		beta, 
		*mat_c,
		mkl_spmm_k
	);

	clock_gettime(CLOCK_REALTIME, &time_end);
	runtime.numc_time = diff_sec(time_start, time_end);

	runtime.total_time = runtime.init_time+runtime.symb_time+runtime.mgmt_time+runtime.numc_time;

#pragma GCC diagnostic pop

	return runtime;
}




int main(int argc, char** argv) {
	srand(32768);

#ifdef IN_DEBUG
	debug_init_barrier();
#endif /* IN_DEBUG */

	char matrix_org_path[CHAR_BUFFER_LEN];
	char matrix_trs_path[CHAR_BUFFER_LEN];
	assert(argc == 2);
	char *dataset = argv[1];
	get_matrix_paths(matrix_org_path, matrix_trs_path, dataset);

	cfg_print();
#ifdef IN_STATS
	printf("%s", dataset);
#endif /* IN_STATS */

	/*** NUMA initialization ***/
	numa_cfg_init(&numa_cfg);

	local_numa_idx = numa_cfg.local_cores[0]/SOCKET_CORE_NUM;
	cxl_numa_idx = 2;
	cur_numa_idx = local_numa_idx;
	cur_numa_idx = cxl_numa_idx;
#if IS_POWER_OF_TWO(NUMA_MASK_VAL)
	numa_cfg.node_idx = cur_numa_idx = POWER_OF_TWO_TO_POWER(NUMA_MASK_VAL);
#endif /* IS_POWER_OF_TWO(NUMA_MASK_VAL) */
	numa_cfg.node_idx = cur_numa_idx;

#if defined(USE_WEIGHTED_INTERLEAVING) && LINUX_VERSION_CODE >= KERNEL_VERSION(6, 9, 0)
	numa_bitmask_clearall(numa_cfg.numa_mask);
	for (int node_idx = 0; node_idx < numa_cfg.nr_nodes; node_idx ++) {
		if ( GETBIT( NUMA_MASK_VAL, node_idx ) ) {
			numa_bitmask_setbit(numa_cfg.numa_mask, node_idx);
		}
	}
	// numa_bitmask_setbit(numa_cfg.numa_mask, 1);
	// numa_bitmask_setbit(numa_cfg.numa_mask, 2);
	numa_cfg.weights[0] = 6;
	numa_cfg.weights[1] = 3;
	numa_cfg.weights[2] = 2;
	// set_interleaving_weight(&numa_cfg);
#elif !defined(USE_INTERLEAVING)	/* defined(USE_WEIGHTED_INTERLEAVING) && LINUX_VERSION_CODE >= KERNEL_VERSION(6, 9, 0) */
	numa_bitmask_clearall(numa_cfg.numa_mask);
	numa_bitmask_setbit(numa_cfg.numa_mask, cur_numa_idx);
	numa_cfg.node_idx = cur_numa_idx;
#endif /* defined(USE_WEIGHTED_INTERLEAVING) && LINUX_VERSION_CODE >= KERNEL_VERSION(6, 9, 0) */


#ifdef USE_NVM
	newdir(NVM_LOCAL_PREFIX);
	newdir(NVM_REMOTE_PREFIX);
#endif /* USE_NVM */

	csr_t csr_a;
	memset(&csr_a, 0, sizeof(csr_t));
	raw2csr(&csr_a, matrix_org_path, &numa_cfg);
	mkl_csr_t mkl_csr_a;
	memset(&mkl_csr_a, 0, sizeof(mkl_csr_t));
	csr2mklcsr(&csr_a, &mkl_csr_a, &numa_cfg);
	free_csr(&csr_a);
	sparse_matrix_t sparse_opt_csr_a = NULL;
	mkl_sparse_d_create_csr(
		&sparse_opt_csr_a, 
		SPARSE_INDEX_BASE_ZERO, 
		mkl_csr_a.num_row, 
		mkl_csr_a.num_col, 
		mkl_csr_a.row_ptrs, 
		mkl_csr_a.row_ptrs + 1, 
		mkl_csr_a.cols, 
		mkl_csr_a.rats
	);
	mkl_sparse_optimize(sparse_opt_csr_a);

	my_rat_t *mat_b = (my_rat_t *) alloc2_memory(
		sizeof(my_rat_t)*mkl_csr_a.num_col*SPMM_B_K, 
		&numa_cfg
	);

	#pragma omp parallel for num_threads(THREAD_NUM)
	for (my_cnt_t idx = 0; idx < mkl_csr_a.num_col*SPMM_B_K; idx++) {
		mat_b[idx] = MKL_DENSE_VAL;
	}

	my_rat_t *mat_c = NULL;

	runtime_t global_runtime = {0.0, 0.0, 0.0, 0.0, 0.0};
	for (int idx = 0; idx < RUN_NUM; idx ++) {
		runtime_t runtime = mklspmm(
			&mkl_csr_a, 
			&sparse_opt_csr_a, 
			mat_b, &mat_c, &numa_cfg
		);

#ifdef IN_VERIFY
		if (idx > 0) {
			int64_t sum_rat = 0;
			for (my_cnt_t jdx = 0; jdx < mkl_csr_a.num_row*SPMM_B_K; jdx ++) {
				sum_rat += mat_c[jdx];
			}
			printf("VERIFYRESULT:\t%ld\n", sum_rat);
		}
#endif /* IN_VERIFY */

		if (mat_c != NULL) {
			dealloc_memory(mat_c, sizeof(my_rat_t)*mkl_csr_a.num_row*SPMM_B_K);
		}

		if (idx > 0) {
			global_runtime.init_time += runtime.init_time;
			global_runtime.symb_time += runtime.symb_time;
			global_runtime.mgmt_time += runtime.mgmt_time;
			global_runtime.numc_time += runtime.numc_time;
			global_runtime.total_time += runtime.total_time;
		}
		cyan();
		printf("%dth run\tinit_time: %.9f\tsymb_time: %.9f\tmgmt_time: %.9f\tnumc_time: %.9f\ttotal_time: %.9f\n", 
			idx, runtime.init_time, runtime.symb_time, runtime.mgmt_time, runtime.numc_time, runtime.total_time
		);
		reset();
		printf("\n");
	}


#if RUN_NUM > 1
	my_cnt_t total_flop_cnt = mkl_csr_a.num_nnz * SPMM_B_K; 
	green();
	printf("dataset: %s\tnum_trials: %d\tthread_num: %d\tnuma_mask: %ld\t"
		"init_time: %.9f\tsymb_time: %.9f\tmgmt_time: %.9f\tnumc_time: %.9f\ttotal_time: %.9f\t"
		"flop_cnt: %ld\tnumc_perf: %.9f\tperf: %.9f\n",
		dataset, RUN_NUM-1, THREAD_NUM, *(numa_cfg.numa_mask->maskp),
		global_runtime.init_time/(RUN_NUM-1),
		global_runtime.symb_time/(RUN_NUM-1),
		global_runtime.mgmt_time/(RUN_NUM-1),
		global_runtime.numc_time/(RUN_NUM-1),
		global_runtime.total_time/(RUN_NUM-1),
		total_flop_cnt,
		total_flop_cnt/(global_runtime.numc_time/(RUN_NUM-1)),
		total_flop_cnt/(global_runtime.total_time/(RUN_NUM-1))
	);
	reset();
#endif /* RUN_NUM > 1 */
	dealloc_memory(
		mat_b, 
		sizeof(my_rat_t)*mkl_csr_a.num_col*SPMM_B_K
	);
	mkl_sparse_destroy(sparse_opt_csr_a);
	free_mklcsr(&mkl_csr_a);

	numa_cfg_free(&numa_cfg);

	return 0;
}
















