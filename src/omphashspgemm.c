#include "mm/mm_utils.h"
#include <omp.h>

extern numa_cfg_t numa_cfg;
extern int64_t file_idx;

#ifdef USE_PAPI
#include "papi/mypapi.h"
extern char* PAPI_event_names[NUM_PAPI_EVENT];
long long PAPI_numc_values[NUM_PAPI_EVENT];
#endif /* USE_PAPI */

#ifdef IN_DEBUG
extern pthread_barrier_t debug_barrier;
extern int debug_rv;
#endif /* IN_DEBUG */

int local_numa_idx;
int cxl_numa_idx;
int cur_numa_idx;

#ifdef IN_VERIFY
extern int64_t over_predict;
extern int64_t under_predict;
#endif /* IN_VERIFY */


#ifdef IN_INSP
double *_numc_time_array;
#endif /* IN_INSP */


static runtime_t hashspgemm(csr_t* csr_a, csr_t* csr_b, csr_t* csr_c, numa_cfg_t* numa_cfg) {
	struct timespec time_start, time_end;
	runtime_t runtime;

	// INIT
	clock_gettime(CLOCK_REALTIME, &time_start);
	csr_c->num_row = csr_a->num_row;
	csr_c->num_col = csr_b->num_col;
	csr_c->row_ptrs = (my_cnt_t*) alloc2_memory(
		sizeof(my_cnt_t)*(csr_c->num_row+1), numa_cfg
	);
	memset(csr_c->row_ptrs, 0, sizeof(my_cnt_t)*(csr_c->num_row+1));

	my_cnt_t *csr_c_row_nz = (my_cnt_t *) alloc2_memory(
		sizeof(my_cnt_t)*csr_c->num_row, numa_cfg
	);
	memset(csr_c_row_nz, 0, sizeof(my_cnt_t)*csr_c->num_row);

	clock_gettime(CLOCK_REALTIME, &time_end);
	runtime.init_time = diff_sec(time_start, time_end);

	// SYMBOLIC
	clock_gettime(CLOCK_REALTIME, &time_start);
	my_cnt_t *ht_check = (my_cnt_t *) alloc2_memory(
		sizeof(my_cnt_t)*THREAD_NUM*NEXT_POW_2(csr_c->num_col), numa_cfg
	);

	#pragma omp parallel num_threads(THREAD_NUM)
	{
		int _tid = omp_get_thread_num();
#ifdef IN_DEBUG
		struct timespec _tid_time_start, _tid_time_end;
		clock_gettime(CLOCK_REALTIME, &_tid_time_start);
#endif /* IN_DEBUG */


		#pragma omp for schedule(runtime) nowait
		for (my_cnt_t row_idx_a = 0; row_idx_a < csr_a->num_row; row_idx_a ++) {

			my_cnt_t ht_size = 0;

			for (my_cnt_t col_offset_a = csr_a->row_ptrs[row_idx_a];
				col_offset_a < csr_a->row_ptrs[row_idx_a+1]; col_offset_a ++) {
				my_cnt_t col_idx_a = csr_a->cols[col_offset_a];
				ht_size += csr_b->row_ptrs[col_idx_a+1]-csr_b->row_ptrs[col_idx_a];
			}

			my_cnt_t nz = 0;

			if (ht_size > 0) {
				if (ht_size > csr_c->num_col) {
					ht_size = csr_c->num_col;
				}

				ht_size = NEXT_POW_2(ht_size);
				my_cnt_t *thr_ht_check = ht_check + _tid * NEXT_POW_2(csr_c->num_col);
				memset(thr_ht_check, -1, sizeof(my_cnt_t)*ht_size);

				for (my_cnt_t col_offset_a = csr_a->row_ptrs[row_idx_a];
					col_offset_a < csr_a->row_ptrs[row_idx_a+1]; col_offset_a++) {
					my_cnt_t col_idx_a = csr_a->cols[col_offset_a];

					for (my_cnt_t col_offset_b = csr_b->row_ptrs[col_idx_a];
						col_offset_b < csr_b->row_ptrs[col_idx_a+1]; col_offset_b++) {
						my_cnt_t key = csr_b->cols[col_offset_b];
						my_cnt_t hash = (key * HASH_CONSTANT) & (ht_size - 1);

						while (true) { // Loop for hash probing
							if (thr_ht_check[hash] == key) { // if the key is already inserted, it's ok
								break;
							} else if (thr_ht_check[hash] == -1) { // if the key has not been inserted yet, then it's added.
								thr_ht_check[hash] = key;
								nz++;
								break;
							} else { // linear probing: check next entry
								hash = (hash + 1) & (ht_size - 1); //hash = (hash + 1) % ht_size
							}
						}

					}
				}
			}

			csr_c_row_nz[row_idx_a] = nz;
		}

#ifdef IN_DEBUG
		clock_gettime(CLOCK_REALTIME, &_tid_time_end);
		// for g500_17_16, the printed thread-level runtime is close, but htop shows it is load-imbalance
		printf("\t%d_symb_time: %.9f\n", _tid, diff_sec(_tid_time_start, _tid_time_end));
#endif /* IN_DEBUG */
	}

#ifdef IN_DEBUG
	printf("\n");
#endif /* IN_DEBUG */

	scan(csr_c->row_ptrs, csr_c_row_nz, csr_c->num_row+1);

	csr_c->num_nnz = csr_c->row_ptrs[csr_c->num_row];
	clock_gettime(CLOCK_REALTIME, &time_end);
	runtime.symb_time = diff_sec(time_start, time_end);


// #ifdef IN_STATS
// 	printf("\t%ld\t%ld\t%ld\t%ld\t%ld\t%f\n", 
// 		csr_a->num_row, csr_a->num_col, csr_a->num_nnz,
// 		csr_c->num_nnz, bin.total_intprod, 
// 		(double) bin.total_intprod/csr_c->num_nnz
// 	);
// 	exit(1);
// #endif /* IN_STATS */

	// MANAGEMENT
	clock_gettime(CLOCK_REALTIME, &time_start);

	csr_c->cols = (my_cnt_t*) alloc2_memory(
		sizeof(my_cnt_t)*(csr_c->num_nnz), numa_cfg
	);
	csr_c->rats = (my_rat_t*) alloc2_memory(
		sizeof(my_rat_t)*csr_c->num_nnz, numa_cfg
	);

#ifdef CSRC_PREPOPULATED
	// memset(csr_c->cols, 0, sizeof(my_cnt_t)*csr_c->num_nnz);
	// memset(csr_c->rats, 0, sizeof(my_rat_t)*csr_c->num_nnz);
	parallel_memset(csr_c->cols, 0, sizeof(my_cnt_t)*csr_c->num_nnz, SOCKET_CORE_NUM);
	parallel_memset(csr_c->rats, 0, sizeof(my_rat_t)*csr_c->num_nnz, SOCKET_CORE_NUM);
#endif /* CSRC_PREPOPULATED */

	clock_gettime(CLOCK_REALTIME, &time_end);
	runtime.mgmt_time = diff_sec(time_start, time_end);


	// NUMERIC
	clock_gettime(CLOCK_REALTIME, &time_start);
	my_rat_t *ht_value = (my_rat_t*) alloc2_memory(
		sizeof(my_rat_t)*THREAD_NUM*NEXT_POW_2(csr_c->num_col), numa_cfg
	);

	#pragma omp parallel num_threads(THREAD_NUM)
	{
		int _tid = omp_get_thread_num();

#ifdef IN_DEBUG
		struct timespec _tid_time_start, _tid_time_end;
		clock_gettime(CLOCK_REALTIME, &_tid_time_start);
#endif /* IN_DEBUG */

		#pragma omp for schedule(runtime) nowait
		for (my_cnt_t row_idx_a = 0; row_idx_a < csr_a->num_row; row_idx_a ++) {

			my_cnt_t ht_size = 0;

			for (my_cnt_t col_offset_a = csr_a->row_ptrs[row_idx_a];
				col_offset_a < csr_a->row_ptrs[row_idx_a+1]; col_offset_a ++) {
				my_cnt_t col_idx_a = csr_a->cols[col_offset_a];
				ht_size += csr_b->row_ptrs[col_idx_a+1]-csr_b->row_ptrs[col_idx_a];
			}

			if (ht_size > 0) {
				if (ht_size > csr_c->num_col) {
					ht_size = csr_c->num_col;
				}

				ht_size = NEXT_POW_2(ht_size);

				my_cnt_t *thr_ht_check = ht_check + _tid * NEXT_POW_2(csr_c->num_col);
				memset(thr_ht_check, -1, sizeof(my_cnt_t)*ht_size);
				my_rat_t *thr_ht_value = ht_value + _tid * NEXT_POW_2(csr_c->num_col);

				for (my_cnt_t col_offset_a = csr_a->row_ptrs[row_idx_a];
					col_offset_a < csr_a->row_ptrs[row_idx_a+1]; col_offset_a++) {
					my_cnt_t col_idx_a = csr_a->cols[col_offset_a];
					my_cnt_t t_aval = csr_a->rats[col_offset_a];

					for (my_cnt_t col_offset_b = csr_b->row_ptrs[col_idx_a];
						col_offset_b < csr_b->row_ptrs[col_idx_a+1]; col_offset_b++) {
						my_rat_t t_val = func_mul(t_aval, csr_b->rats[col_offset_b]);

						my_cnt_t key = csr_b->cols[col_offset_b];
						my_cnt_t hash = (key * HASH_CONSTANT) & (ht_size - 1);


						while (true) { // Loop for hash probing
							if (thr_ht_check[hash] == key) { // if the key is already inserted, it's ok
								thr_ht_value[hash] += t_val;
								break;
							} else if (thr_ht_check[hash] == -1) { // if the key has not been inserted yet, then it's added.
								thr_ht_check[hash] = key;
								thr_ht_value[hash] = t_val;
								break;
							} else { // linear probing: check next entry
								hash = (hash + 1) & (ht_size - 1); //hash = (hash + 1) % ht_size
							}
						}

					}
				}


				my_cnt_t offset = csr_c->row_ptrs[row_idx_a];
				my_cnt_t index = 0;
				for (my_cnt_t thr_ht_check_idx = 0; thr_ht_check_idx < ht_size; thr_ht_check_idx ++) {
					if (thr_ht_check[thr_ht_check_idx] != -1) {
						csr_c->cols[offset+index] = thr_ht_check[thr_ht_check_idx];
						csr_c->rats[offset+index] = thr_ht_value[thr_ht_check_idx];
						index ++;
					}
				}
			}

		}

#ifdef IN_DEBUG
		clock_gettime(CLOCK_REALTIME, &_tid_time_end);
		// for g500_17_16, the printed thread-level runtime is close, but htop shows it is load-imbalance
		printf("\t%d_numc_time: %.9f\n", _tid, diff_sec(_tid_time_start, _tid_time_end));
#endif /* IN_DEBUG */
	}

#ifdef IN_DEBUG
	printf("\n");
#endif /* IN_DEBUG */

	dealloc_memory(ht_check, sizeof(my_cnt_t)*THREAD_NUM*NEXT_POW_2(csr_c->num_col));
	dealloc_memory(ht_value, sizeof(my_rat_t)*THREAD_NUM*NEXT_POW_2(csr_c->num_col));

	clock_gettime(CLOCK_REALTIME, &time_end);
	runtime.numc_time = diff_sec(time_start, time_end);



	runtime.total_time = runtime.init_time+runtime.symb_time+runtime.mgmt_time+runtime.numc_time;

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

	csr_t csr_a, csr_b, csr_c;
	memset(&csr_a, 0, sizeof(csr_t));
	memset(&csr_b, 0, sizeof(csr_t));
	memset(&csr_c, 0, sizeof(csr_t));

	raw2csr(&csr_a, matrix_org_path, &numa_cfg);
	raw2csr(&csr_b, matrix_trs_path, &numa_cfg);

#ifdef USE_PAPI
	memset(PAPI_numc_values, 0, sizeof(long long) * NUM_PAPI_EVENT );

	int rv;

	/* papi library initialization */
	if ( (rv = PAPI_library_init(PAPI_VER_CURRENT)) != PAPI_VER_CURRENT) {
		printf("Library initialization error! \n");
		exit(1);
	}

	/** PAPI_thread_init initializes thread support in the PAPI library. 
	 * 	Itshould be called only once, just after PAPI_library_init (3), 
	 * 	and before any other PAPI calls. Applications that make no use 
	 * 	of threads do not need to call this routine.
	 */
	rv = PAPI_thread_init( (unsigned long(*) (void) ) (pthread_self) );
	if ( (rv = PAPI_library_init(PAPI_VER_CURRENT) ) != PAPI_VER_CURRENT) {
		printf("Library initialization error! \n");
		exit(1);
	}

	/* Enable and initialize multiplex support */
	rv = PAPI_multiplex_init();
	if (rv != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}
#endif /* USE_PAPI */

#ifdef IN_INSP
	_numc_time_array = (double *) alloc2_memory(sizeof(double) * csr_a.num_row, &numa_cfg);
	memset(_numc_time_array, 0, sizeof(double) * csr_a.num_row);
#endif /* IN_INSP */

	runtime_t global_runtime = {0.0, 0.0, 0.0, 0.0, 0.0};
	for (int idx = 0; idx < RUN_NUM; idx ++) {
		runtime_t runtime = hashspgemm(&csr_a, &csr_b, &csr_c, &numa_cfg);

#ifdef IN_VERIFY
		if (idx > 0) {
			int64_t sum_col = 0, sum_rat = 0;
			for (my_cnt_t jdx = 0; jdx < csr_c.num_nnz; jdx ++) {
				sum_col += csr_c.cols[jdx];
				sum_rat += csr_c.rats[jdx];
			}
			printf("VERIFYRESULT:\t%ld\t%ld\t%ld\n", csr_c.num_nnz, sum_col, sum_rat);
		}
#endif /* IN_VERIFY */

#ifdef IN_INSP
		if (idx > 0 && idx == RUN_NUM - 1) {
			for (my_cnt_t row_idx = 0; row_idx < csr_a.num_row; row_idx ++) {
				my_cnt_t flop = 0;
				for (my_cnt_t col_offset = csr_a.row_ptrs[row_idx]; col_offset < csr_a.row_ptrs[row_idx+1]; col_offset ++) {
					my_cnt_t col_idx = csr_a.cols[col_offset];
					flop += csr_b.row_ptrs[col_idx+1] - csr_b.row_ptrs[col_idx];
				}

				printf("\t\trow_idx: %ld\tnumc_time: %f\ta_nnz: %ld\tc_nnz: %ld\tflop: %ld\n", 
					// note that should be divided by RUN_NUM for RUN_0 also accumulated
					row_idx, _numc_time_array[row_idx]/RUN_NUM,
					csr_a.row_ptrs[row_idx+1] - csr_a.row_ptrs[row_idx],
					csr_c.row_ptrs[row_idx+1] - csr_c.row_ptrs[row_idx],
					flop
				);
			}
		}
#endif /* IN_INSP */

#ifdef USE_PAPI
		if (idx > 0 && idx == RUN_NUM - 1) {
			for (int jdx = 0; jdx < NUM_PAPI_EVENT; jdx ++) {
				printf("[PAPI-numc] PAPI_event_name_%s:\t%lld\n", 
					PAPI_event_names[jdx], 
					PAPI_numc_values[jdx]/RUN_NUM
				);
			}
			PAPI_shutdown();
		}
#endif /* USE_PAPI */

		free_csr(&csr_c);
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
	my_cnt_t *row_flop = (my_cnt_t *) alloc2_memory(sizeof(my_cnt_t)*csr_a.num_row, &numa_cfg);
	my_cnt_t __attribute__((unused)) total_flop_cnt = get_intprod(&csr_a, &csr_b, row_flop);
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
#endif /* RUN_NUM > 1 */


#ifdef IN_INSP
	dealloc_memory(_numc_time_array, sizeof(double) * csr_a.num_row);
#endif /* IN_INSP */


	free_csr(&csr_a);
	free_csr(&csr_b);

	numa_cfg_free(&numa_cfg);

	return 0;
}
















