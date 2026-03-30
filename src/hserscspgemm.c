#include "mm/common.h"

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

#ifdef MEMSV
numa_cfg_t cxl_numa_cfg;
#endif /* MEMSV */

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


typedef struct {
	int _tid;
	func_op_t func_op;
	csr_t *csr_a;
	csr_t *csr_b;
	csr_t *csr_c;
	my_cnt_t *rows_offset;
	my_cnt_t local_nnz_max;
	numa_cfg_t *numa_cfg;
#ifdef IN_DEBUG
	double _numc_time;
#endif /* IN_DEBUG */
#ifdef IN_EXAMINE
	double _comp_time;
	double _write_time;
	my_cnt_t _flop_cnt;
#endif /* IN_EXAMINE */
#ifdef USE_PAPI
	long long _PAPI_numc_values[NUM_PAPI_EVENT];
#endif /* USE_PAPI */
} esc_numeric_arg_t;


static void* pthread_esc_numeric(void* param) {
#ifdef IN_DEBUG
	struct timespec time_start, time_end;
	clock_gettime(CLOCK_REALTIME, &time_start);
#endif /* IN_DEBUG */

#ifdef USE_PAPI
	int eventset = PAPI_NULL;

	int rv = PAPI_create_eventset(&eventset);
	if (rv != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}

	/* Assign it to the CPU component */
	rv = PAPI_assign_eventset_component(eventset, 0);
	if (rv != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}

	/* Convert the EventSet to a multiplexed event set */
	rv = PAPI_set_multiplex(eventset);
	if (rv != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}

	for (int idx = 0; idx < NUM_PAPI_EVENT; idx ++) {
		rv = PAPI_add_named_event(eventset, PAPI_event_names[idx]);
		if (rv != PAPI_OK) {
			ERROR_RETURN(rv);
			exit(1);
		}		
	}

	PAPI_reset(eventset);
	PAPI_start(eventset);
#endif /* USE_PAPI */

	esc_numeric_arg_t *arg = (esc_numeric_arg_t *) param;
	int _tid = arg->_tid;
	func_op_t func_op = arg->func_op;
	csr_t *csr_a = arg->csr_a;
	csr_t *csr_b = arg->csr_b;
	csr_t *csr_c = arg->csr_c;
	my_cnt_t *rows_offset = arg->rows_offset;
	numa_cfg_t *numa_cfg = arg->numa_cfg;	

	keyval_t *keyval_org = (keyval_t *) alloc2_memory(
		sizeof(keyval_t) * arg->local_nnz_max, numa_cfg
	);
	keyval_t *aux = (keyval_t *) alloc2_memory(
		sizeof(keyval_t) * arg->local_nnz_max, numa_cfg
	);
	memset(keyval_org, 0, sizeof(keyval_t) * arg->local_nnz_max);
	memset(aux, 0, sizeof(keyval_t) * arg->local_nnz_max);

#ifdef IN_EXAMINE
	struct timespec examine_time_start, examine_time_end;
	double _comp_time = 0.0, _write_time = 0.0;
	my_cnt_t _flop_cnt = 0;
#endif /* IN_EXAMINE */

	for (my_cnt_t idx = rows_offset[_tid]; idx < rows_offset[_tid+1]; idx ++) {

#ifdef IN_EXAMINE
		clock_gettime(CLOCK_REALTIME, &examine_time_start);
#endif /* IN_EXAMINE */

		my_cnt_t index_0 = 0;
		keyval_t *keyval = keyval_org;
#ifdef IN_INSP
		struct timespec insp_time_start, insp_time_end;
		clock_gettime(CLOCK_REALTIME, &insp_time_start);
#endif /* IN_INSP */

		for (my_cnt_t jdx = csr_a->row_ptrs[idx]; jdx < csr_a->row_ptrs[idx+1]; jdx ++) {
			my_cnt_t t_acol = csr_a->cols[jdx];

			for (my_cnt_t kdx = csr_b->row_ptrs[t_acol]; kdx < csr_b->row_ptrs[t_acol+1]; kdx ++) {
				my_cnt_t t_bcol = csr_b->cols[kdx];

				keyval[index_0].key = t_bcol;
				keyval[index_0].val = func_op(csr_a->rats[jdx], csr_b->rats[kdx]);

				index_0 ++;
			}
		}

		keyval = radix_sort_keyval_toggling_unrolling(keyval_org, aux, index_0);

		my_cnt_t offset = csr_c->row_ptrs[idx];
		my_cnt_t index_1 = 0;

		if (index_0 <= 0) {
			continue;
		}

		keyval_t* aux_1 = keyval == keyval_org ? aux : keyval_org;

		aux_1[index_1] = keyval[0];

		for (my_cnt_t ldx = 1; ldx < index_0; ldx ++) {
			bool flag = keyval[ldx].key == keyval[ldx-1].key;
			index_1 += !flag;
			aux_1[index_1].key = keyval[ldx].key;
			aux_1[index_1].val = aux_1[index_1].val * flag + keyval[ldx].val;
		}

#ifdef IN_EXAMINE
		clock_gettime(CLOCK_REALTIME, &examine_time_end);
		_comp_time += diff_sec(examine_time_start, examine_time_end);
		clock_gettime(CLOCK_REALTIME, &examine_time_start);
#endif /* IN_EXAMINE */

		for (my_cnt_t ldx = 0; ldx < index_1+1; ldx ++) {
			csr_c->cols[ldx+offset] = aux_1[ldx].key;
			csr_c->rats[ldx+offset] = aux_1[ldx].val;
		}

		// memset(keyval, 0, sizeof(keyval_t) * index_0);
		// memset(aux, 0, sizeof(keyval_t) * index_0);

#ifdef IN_INSP
		clock_gettime(CLOCK_REALTIME, &insp_time_end);
		_numc_time_array[idx] += diff_sec(insp_time_start, insp_time_end);
#endif /* IN_INSP */

#ifdef IN_EXAMINE
		clock_gettime(CLOCK_REALTIME, &examine_time_end);
		_write_time += diff_sec(examine_time_start, examine_time_end);
		_flop_cnt += index_0;
#endif /* IN_EXAMINE */

	}

	dealloc_memory(
		keyval_org,
		sizeof(keyval_t) * arg->local_nnz_max
	);
	dealloc_memory(
		aux,
		sizeof(keyval_t) * arg->local_nnz_max
	);

#ifdef USE_PAPI
	PAPI_stop(eventset, arg->_PAPI_numc_values);
	/* Free all memory and data structures, eventset must be empty. */
	if ( (rv = PAPI_cleanup_eventset(eventset))!=PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}

	if ( (rv = PAPI_destroy_eventset(&eventset)) != PAPI_OK) {
		ERROR_RETURN(rv);
		exit(1);
	}
#endif /* USE_PAPI */

#ifdef IN_DEBUG
	clock_gettime(CLOCK_REALTIME, &time_end);
	arg->_numc_time = diff_sec(time_start, time_end);
#endif /* IN_DEBUG */

#ifdef IN_EXAMINE
	arg->_comp_time = _comp_time;
	arg->_write_time = _write_time;
	arg->_flop_cnt = _flop_cnt;
#endif /* IN_EXAMINE */

	return NULL;
}



static void esc_numeric(csr_t *csr_a, csr_t *csr_b, csr_t *csr_c,
	my_cnt_t *rows_offset, my_cnt_t *local_nnz_max, numa_cfg_t *numa_cfg,
	my_rat_t (*func_op)(my_rat_t, my_rat_t)) {
	pthread_t threadpool[THREAD_NUM];
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	cpu_set_t set;
	int core_idx;
	int rv;

	esc_numeric_arg_t args[THREAD_NUM];
	memset(args, 0, sizeof(esc_numeric_arg_t) * THREAD_NUM);

	for (int idx = 0; idx < THREAD_NUM; idx ++) {
		core_idx = get_cpu_id(idx);
		CPU_ZERO(&set);
		CPU_SET(core_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		args[idx]._tid = idx;
		args[idx].func_op = func_op;
		args[idx].csr_a = csr_a;
		args[idx].csr_b = csr_b;
		args[idx].csr_c = csr_c;
		args[idx].rows_offset = rows_offset;
		args[idx].local_nnz_max = local_nnz_max[idx];
		args[idx].numa_cfg = numa_cfg;

#ifdef USE_PAPI
		memset(args[idx]._PAPI_numc_values, 0, sizeof(long long) * NUM_PAPI_EVENT );
#endif /* USE_PAPI */

		rv = pthread_create(&threadpool[idx], &attr, pthread_esc_numeric, args+idx);
		if (rv){
			printf("ERROR; return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

	for (int idx = 0; idx < THREAD_NUM; idx ++) {
		rv = pthread_join(threadpool[idx], NULL);
#ifdef IN_DEBUG
		printf("\t%d_numc_time: %.9f", idx, args[idx]._numc_time);
#ifdef IN_EXAMINE
		printf("\t_comp_time: %.9f\t_write_time: %.9f", 
			args[idx]._comp_time, args[idx]._write_time
		);
		printf("\t_row_cnt: %ld\t_flop_cnt: %ld\t_nnzc_cnt: %ld",
			rows_offset[idx+1]-rows_offset[idx], args[idx]._flop_cnt,
			csr_c->row_ptrs[rows_offset[idx+1]] - csr_c->row_ptrs[rows_offset[idx]]
		);
#endif /* IN_EXAMINE */
		printf("\n");
		if (idx == THREAD_NUM-1) {
			printf("\n");
		}
#endif /* IN_DEBUG */

#ifdef USE_PAPI
		for (int jdx = 0; jdx < NUM_PAPI_EVENT; jdx ++) {
			PAPI_numc_values[jdx] += args[idx]._PAPI_numc_values[jdx];
		}
#endif /* USE_PAPI */

		if (rv) {
			printf("ERROR; return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

#if defined(USE_PAPI) && defined(IN_DEBUG)
	for (int idx = 0; idx < NUM_PAPI_EVENT; idx ++) {
		printf("\t%s", PAPI_event_names[idx]);
	}
	printf("\n");

	for (int idx = 0; idx < THREAD_NUM; idx ++) {
		printf("\t%d", idx);
		for (int jdx = 0; jdx < NUM_PAPI_EVENT; jdx ++) {
			printf("_numc_%s: %lld\t", PAPI_event_names[jdx], args[idx]._PAPI_numc_values[jdx]);
		}
		printf("\n");
	}
	printf("\n");
#endif /* defined(USE_PAPI) && defined(IN_DEBUG) */

	pthread_attr_destroy(&attr);
}




static runtime_t escspgemm(csr_t* csr_a, csr_t* csr_b, csr_t* csr_c, numa_cfg_t* numa_cfg) {
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

	my_cnt_t *row_nz = (my_cnt_t*) alloc2_memory(
		sizeof(my_cnt_t) * csr_a->num_row, numa_cfg
	);
	my_cnt_t *rows_offset = (my_cnt_t *) alloc2_memory(
		sizeof(my_cnt_t) * (THREAD_NUM+1), numa_cfg
	);

	// 1st pass, csr_a->row_ptrs, csr_a->cols, csr_b->row_ptrs, csr_b->cols
	my_cnt_t total_intprod = get_intprod(csr_a, csr_b, row_nz);

	my_cnt_t *row_start_idcs = (my_cnt_t *) alloc2_memory(
		sizeof(my_cnt_t) * (csr_a->num_row+1), numa_cfg
	);
	scan(row_start_idcs, row_nz, csr_a->num_row+1);
	rows_offset[0] = 0;
	set_rows_offset( csr_a->num_row, total_intprod, rows_offset, row_start_idcs );
	dealloc_memory( row_start_idcs, sizeof(my_cnt_t) * (csr_a->num_row+1) );

	my_cnt_t local_nnz_max[THREAD_NUM];
	memset(local_nnz_max, 0, sizeof(my_cnt_t)*THREAD_NUM);
	comm_init_local(csr_a, csr_b, rows_offset, local_nnz_max, numa_cfg);

	clock_gettime(CLOCK_REALTIME, &time_end);
	runtime.init_time = diff_sec(time_start, time_end);

	// SYMBOLIC
	clock_gettime(CLOCK_REALTIME, &time_start);

#if defined(MEMSV) && defined(MEMSVB)
	numa_cfg_t intr_numa_cfg;
	numa_cfg_init(&intr_numa_cfg);
	numa_bitmask_setbit(intr_numa_cfg.numa_mask, LOCAL_NUMA_NODE_IDX-1);
	numa_bitmask_setbit(intr_numa_cfg.numa_mask, CXL_NUMA_NODE_IDX-1);
	csr_t intr_csr_b;
	intr_csr_b.num_row = csr_b->num_row;
	intr_csr_b.num_col = csr_b->num_col;
	intr_csr_b.num_nnz = csr_b->num_nnz;
	intr_csr_b.row_ptrs = (my_cnt_t *) alloc_weighted(
		sizeof(my_cnt_t) * (intr_csr_b.num_row+1), &intr_numa_cfg
	);
	intr_csr_b.cols = (my_cnt_t *) alloc_weighted(
		sizeof(my_cnt_t) * intr_csr_b.num_nnz, &intr_numa_cfg
	);
	parallel_memcpy(
		intr_csr_b.row_ptrs, csr_b->row_ptrs, 
		sizeof(my_cnt_t)*(csr_b->num_row+1), SOCKET_CORE_NUM
	);
	parallel_memcpy(
		intr_csr_b.cols, csr_b->cols, 
		sizeof(my_cnt_t)*csr_b->num_nnz, SOCKET_CORE_NUM
	);
	esc_symbolic(csr_a, &intr_csr_b, rows_offset, row_nz, local_nnz_max, numa_cfg);
	dealloc_memory(intr_csr_b.row_ptrs, sizeof(my_cnt_t)*(intr_csr_b.num_row+1));
	dealloc_memory(intr_csr_b.cols, sizeof(my_cnt_t)*intr_csr_b.num_nnz);
	intr_csr_b.num_row = intr_csr_b.num_col = intr_csr_b.num_nnz = 0;
	numa_cfg_free(&intr_numa_cfg);
#else	/* defined(MEMSV) && defined(MEMSVB) */
	esc_symbolic(csr_a, csr_b, rows_offset, row_nz, local_nnz_max, numa_cfg);
#endif /* defined(MEMSV) && defined(MEMSVB) */ 

	scan(csr_c->row_ptrs, row_nz, csr_c->num_row+1);
	dealloc_memory(row_nz, sizeof(my_cnt_t) * csr_a->num_row);
	csr_c->num_nnz = csr_c->row_ptrs[csr_c->num_row];

	clock_gettime(CLOCK_REALTIME, &time_end);
	runtime.symb_time = diff_sec(time_start, time_end);

#ifdef IN_STATS
	printf("\t%ld\t%ld\t%ld\t%ld\t%ld\t%f\n", 
		csr_a->num_row, csr_a->num_col, csr_a->num_nnz,
		csr_c->num_nnz, total_intprod, 
		(double) total_intprod/csr_c->num_nnz
	);
	exit(1);
#endif /* IN_STATS */

	// MANAGEMENT
	clock_gettime(CLOCK_REALTIME, &time_start);

#ifdef MEMSV
	csr_c->cols = (my_cnt_t*) alloc_dram(
		sizeof(my_cnt_t)*(csr_c->num_nnz), cxl_numa_cfg.node_idx
	);
	csr_c->rats = (my_rat_t*) alloc_dram(
		sizeof(my_rat_t)*csr_c->num_nnz, cxl_numa_cfg.node_idx
	);
#else /* MEMSV */
	csr_c->cols = (my_cnt_t*) alloc2_memory(
		sizeof(my_cnt_t)*(csr_c->num_nnz), numa_cfg
	);
	csr_c->rats = (my_rat_t*) alloc2_memory(
		sizeof(my_rat_t)*csr_c->num_nnz, numa_cfg
	);
#endif /* MEMSV */
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
	// 3rd pass, csr_a->row_ptrs, csr_a->cols, csr_a->rats, csr_b->row_ptrs, csr_b->cols, csr_b->rats
	esc_numeric(csr_a, csr_b, csr_c, rows_offset, local_nnz_max, numa_cfg, func_mul);

	dealloc_memory(rows_offset, sizeof(my_cnt_t) * (THREAD_NUM+1) );

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

#ifdef MEMSV
	numa_cfg_init(&cxl_numa_cfg);
	cxl_numa_cfg.node_idx = cxl_numa_idx;
	numa_bitmask_clearall(cxl_numa_cfg.numa_mask);
	numa_bitmask_setbit(cxl_numa_cfg.numa_mask, cxl_numa_cfg.node_idx);
	// printf("cxl_numa_cfg.numa_mask: %ld\n", 
	// 	*(cxl_numa_cfg.numa_mask->maskp)
	// );
#endif /* MEMSV */


#ifdef USE_NVM
	newdir(NVM_LOCAL_PREFIX);
	newdir(NVM_REMOTE_PREFIX);
#endif /* USE_NVM */

	csr_t csr_a, csr_b, csr_c;
	memset(&csr_a, 0, sizeof(csr_t));
	memset(&csr_b, 0, sizeof(csr_t));
	memset(&csr_c, 0, sizeof(csr_t));

#ifdef MEMSV
	raw2csr(&csr_a, matrix_org_path, &cxl_numa_cfg);
#else /* MEMSV */
	raw2csr(&csr_a, matrix_org_path, &numa_cfg);
#endif /* MEMSV */
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

#ifdef MEM_MON
	char command[CHAR_BUFFER_LEN] = {'\0'};
	snprintf(
		command, sizeof(command),
		"bash %s/mem_mon.sh %d %s < /dev/null &",
		PROJECT_PATH, getpid(), dataset
	);
	system(command);
#endif /* MEM_MON */
	runtime_t global_runtime = {0.0, 0.0, 0.0, 0.0, 0.0};
	for (int idx = 0; idx < RUN_NUM; idx ++) {
		runtime_t runtime = escspgemm(&csr_a, &csr_b, &csr_c, &numa_cfg);

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
		if (idx > 0 && idx == RUN_NUM-1) {
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

#ifdef MEMSV
	numa_cfg_free(&cxl_numa_cfg);
#endif /* MEMSV */
	numa_cfg_free(&numa_cfg);

	return 0;
}












