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


static void set_ind_base_0_to_1(mkl_csr_t *mkl_csr) {
	for (my_cnt_t idx = 0; idx <= mkl_csr->num_row; idx ++) {
		mkl_csr->row_ptrs[idx]++;
	}
	for (my_cnt_t idx = 0; idx < mkl_csr->num_nnz; idx ++) {
		mkl_csr->cols[idx]++;
	}
}

#ifdef IN_VERIFY
static void set_ind_base_1_to_0(mkl_csr_t *mkl_csr) {
	for (my_cnt_t idx = 0; idx <= mkl_csr->num_row; idx ++) {
		mkl_csr->row_ptrs[idx]--;
	}
	for (my_cnt_t idx = 0; idx < mkl_csr->num_nnz; idx ++) {
		mkl_csr->cols[idx]--;
	}
}
#endif /* IN_VERIFY */

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


static runtime_t mklspgemm(mkl_csr_t* mkl_csr_a, mkl_csr_t* mkl_csr_b, 
	mkl_csr_t* mkl_csr_c, numa_cfg_t *numa_cfg) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

	mkl_set_num_threads(THREAD_NUM);

	struct timespec time_start, time_end;
	runtime_t runtime = {0.0, 0.0, 0.0, 0.0, 0.0};

	clock_gettime(CLOCK_REALTIME, &time_start);

	MKL_INT request = 1;
	MKL_INT sort = 7; 	// don't sort anything
	MKL_INT ierr = 0; 	// output info flag

#ifdef MKL_ENHANCE
	sort = 6;
#endif /* MKL_ENHANCE */

	mkl_csr_c->num_row = mkl_csr_a->num_row;
	mkl_csr_c->num_col = mkl_csr_b->num_col;
	mkl_csr_c->row_ptrs = (MKL_INT*) alloc2_memory(
		sizeof(MKL_INT)*(mkl_csr_c->num_row+1), numa_cfg
	);
	memset(mkl_csr_c->row_ptrs, 0, sizeof(MKL_INT)*(mkl_csr_c->num_row+1));

	clock_gettime(CLOCK_REALTIME, &time_end);
	runtime.init_time = diff_sec(time_start, time_end);


	clock_gettime(CLOCK_REALTIME, &time_start);
	mkl_dcsrmultcsr((char*)"N", &request, &sort,
		&(mkl_csr_a->num_row), &(mkl_csr_a->num_col), &(mkl_csr_b->num_col),
		mkl_csr_a->rats, mkl_csr_a->cols, mkl_csr_a->row_ptrs,
		mkl_csr_b->rats, mkl_csr_b->cols, mkl_csr_b->row_ptrs,
		NULL, NULL, mkl_csr_c->row_ptrs,
		NULL, &ierr
	);
	clock_gettime(CLOCK_REALTIME, &time_end);
	runtime.symb_time = diff_sec(time_start, time_end);

	clock_gettime(CLOCK_REALTIME, &time_start);

	mkl_csr_c->num_nnz = mkl_csr_c->row_ptrs[mkl_csr_c->num_row]-1;
	mkl_csr_c->cols = (MKL_INT*) alloc2_memory(
		sizeof(MKL_INT)*(mkl_csr_c->num_nnz), numa_cfg
	);
	mkl_csr_c->rats = (my_rat_t*) alloc2_memory(
		sizeof(my_rat_t)*mkl_csr_c->num_nnz, numa_cfg
	);

#ifdef CSRC_PREPOPULATED
	// memset(mkl_csr_c->cols, 0, sizeof(MKL_INT)*mkl_csr_c->num_nnz);
	// memset(mkl_csr_c->rats, 0, sizeof(my_rat_t)*mkl_csr_c->num_nnz);
	parallel_memset(mkl_csr_c->cols, 0, sizeof(MKL_INT)*mkl_csr_c->num_nnz, SOCKET_CORE_NUM);
	parallel_memset(mkl_csr_c->rats, 0, sizeof(my_rat_t)*mkl_csr_c->num_nnz, SOCKET_CORE_NUM);
#endif /* CSRC_PREPOPULATED */

	clock_gettime(CLOCK_REALTIME, &time_end);
	runtime.mgmt_time = diff_sec(time_start, time_end);


	clock_gettime(CLOCK_REALTIME, &time_start);
	request = 2;
#ifdef MKL_SORT
	sort = 8;
#ifdef MKL_ENHANCE
	sort = 3;
#endif /* MKL_ENHANCE */
#else	/* MKL_SORT */
	sort = 6;
#endif /* MKL_SORT */

	mkl_dcsrmultcsr((char*)"N", &request, &sort,
		&(mkl_csr_a->num_row), &(mkl_csr_a->num_col), &(mkl_csr_b->num_col),
		mkl_csr_a->rats, mkl_csr_a->cols, mkl_csr_a->row_ptrs,
		mkl_csr_b->rats, mkl_csr_b->cols, mkl_csr_b->row_ptrs,
		mkl_csr_c->rats, mkl_csr_c->cols, mkl_csr_c->row_ptrs,
		NULL, &ierr
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

	csr_t csr_a, csr_b;
	memset(&csr_a, 0, sizeof(csr_t));
	memset(&csr_b, 0, sizeof(csr_t));

	raw2csr(&csr_a, matrix_org_path, &numa_cfg);
	raw2csr(&csr_b, matrix_trs_path, &numa_cfg);

	mkl_csr_t mkl_csr_a, mkl_csr_b, mkl_csr_c;
	memset(&mkl_csr_a, 0, sizeof(mkl_csr_t));
	memset(&mkl_csr_b, 0, sizeof(mkl_csr_t));
	memset(&mkl_csr_c, 0, sizeof(mkl_csr_t));
	csr2mklcsr(&csr_a, &mkl_csr_a, &numa_cfg);
	csr2mklcsr(&csr_b, &mkl_csr_b, &numa_cfg);
	free_csr(&csr_a);
	free_csr(&csr_b);
	set_ind_base_0_to_1(&mkl_csr_a);
	set_ind_base_0_to_1(&mkl_csr_b);

	runtime_t global_runtime = {0.0, 0.0, 0.0, 0.0, 0.0};
	for (int idx = 0; idx < RUN_NUM; idx ++) {
		runtime_t runtime = mklspgemm(&mkl_csr_a, &mkl_csr_b, &mkl_csr_c, &numa_cfg);

#ifdef IN_VERIFY
		if (idx > 0) {
			set_ind_base_1_to_0(&mkl_csr_c);
			int64_t sum_col = 0, sum_rat = 0;
			for (my_cnt_t jdx = 0; jdx < mkl_csr_c.num_nnz; jdx ++) {
				sum_col += mkl_csr_c.cols[jdx];
				sum_rat += mkl_csr_c.rats[jdx];
			}
			printf("VERIFYRESULT:\t%lld\t%ld\t%ld\n", mkl_csr_c.num_nnz, sum_col, sum_rat);
		}
#endif /* IN_VERIFY */

		free_mklcsr(&mkl_csr_c);
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

	free_mklcsr(&mkl_csr_a);
	free_mklcsr(&mkl_csr_b);
#if RUN_NUM > 1
#if defined(IN_GROUPBY) || defined(IN_SSJ)
	bin2csr(&csr_a, matrix_org_path, &numa_cfg);
	bin2csr(&csr_b, matrix_trs_path, &numa_cfg);
#else	// defined(IN_GROUPBY) || defined(IN_SSJ)
	mtx2csr(&csr_a, matrix_org_path, &numa_cfg);
	mtx2csr(&csr_b, matrix_trs_path, &numa_cfg);
#endif	// defined(IN_GROUPBY) || defined(IN_SSJ)
	my_cnt_t *row_flop = (my_cnt_t *) alloc2_memory(sizeof(my_cnt_t)*csr_a.num_row, &numa_cfg);
	my_cnt_t total_flop_cnt = get_intprod(&csr_a, &csr_b, row_flop);
	dealloc_memory(row_flop, sizeof(my_cnt_t)*csr_a.num_row);

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
	free_csr(&csr_a);
	free_csr(&csr_b);
#endif /* RUN_NUM > 1 */

	numa_cfg_free(&numa_cfg);

	return 0;
}
















