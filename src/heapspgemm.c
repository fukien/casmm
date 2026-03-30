#include "mm/mm_utils.h"

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

typedef struct {
	my_cnt_t runr;
	my_cnt_t loc;
	my_cnt_t key;
	my_cnt_t value;
} entry_t;


typedef struct {
	int _tid;
	numa_cfg_t *numa_cfg;
	my_cnt_t *row_nz;
	my_cnt_t *rows_offset;
	my_cnt_t *local_heap_size;
	my_cnt_t *local_size;
	my_cnt_t **local_col_of_C;
	my_rat_t **local_rat_of_C;
	csr_t *csr_a;
} pthread_local_init_arg_t;

typedef struct {
	int _tid;
	numa_cfg_t *numa_cfg;
	func_op_t func_op;
	csr_t *csr_a;
	csr_t *csr_b;
	my_cnt_t* rows_offset;
	my_cnt_t* row_start_idcs;
	my_cnt_t* row_end_idcs;
	my_cnt_t local_heap_size;
	my_cnt_t* real_row_nz;
	my_cnt_t* local_col_of_C;
	my_rat_t* local_rat_of_C;
#ifdef IN_DEBUG
	double _numc_time;
#endif /* IN_DEBUG */
#ifdef USE_PAPI
	long long _PAPI_numc_values[NUM_PAPI_EVENT];
#endif /* USE_PAPI */
} heap_numeric_arg_t;


typedef struct {
	int _tid;
	csr_t *csr_c;
	my_cnt_t *rows_offset;
	my_cnt_t *row_start_idcs;
	my_cnt_t *row_end_idcs;
	my_cnt_t *local_col_of_C;
	my_rat_t *local_rat_of_C;
#ifdef IN_DEBUG
	double _ccpy_time;
#endif /* IN_DEBUG */
} ccpy_arg_t;


static void* pthread_init_local(void *param) {
	pthread_local_init_arg_t* arg = (pthread_local_init_arg_t*) param;
	int _tid = arg->_tid;
	csr_t *csr_a = arg->csr_a;
	my_cnt_t *row_nz = arg->row_nz;
	my_cnt_t *rows_offset = arg->rows_offset;
	my_cnt_t *local_heap_size = arg->local_heap_size;
	my_cnt_t *local_size = arg->local_size;

	my_cnt_t cur_heap_size = 0, max_heap_size = 0;

	for (my_cnt_t idx = rows_offset[_tid]; idx < rows_offset[_tid+1]; idx++) {

		(*local_size) += row_nz[idx];

		cur_heap_size = csr_a->row_ptrs[idx+1] - csr_a->row_ptrs[idx];

		max_heap_size = ( cur_heap_size > max_heap_size) ? cur_heap_size : max_heap_size;
	}

	*(arg->local_col_of_C) = (my_cnt_t *) alloc2_memory(
		sizeof(my_cnt_t) * (*local_size), arg->numa_cfg
	);
	// memset(*(arg->local_col_of_C), 0, sizeof(my_cnt_t) * (*local_size));

	*(arg->local_rat_of_C) = (my_rat_t *) alloc2_memory(
		sizeof(my_rat_t) * (*local_size), arg->numa_cfg
	);
	// memset(*(arg->local_rat_of_C), 0, sizeof(my_rat_t) * (*local_size));

	*local_heap_size = max_heap_size;

	return NULL;
}

static void init_local(csr_t *csr_a, 
	my_cnt_t *row_nz, my_cnt_t *rows_offset,
	my_cnt_t *local_heap_size, my_cnt_t *local_size,
	my_cnt_t **local_col_of_C, my_rat_t **local_rat_of_C,
	numa_cfg_t *numa_cfg) {


	pthread_t threadpool[THREAD_NUM];
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	cpu_set_t set;
	int core_idx;
	int rv;
	pthread_local_init_arg_t args[THREAD_NUM];
	memset(args, 0, sizeof(pthread_local_init_arg_t)*THREAD_NUM);

	for (int idx = 0; idx < THREAD_NUM; idx ++) {
		core_idx = get_cpu_id(idx);
		CPU_ZERO(&set);
		CPU_SET(core_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		args[idx]._tid = idx;
		args[idx].csr_a = csr_a;
		args[idx].row_nz = row_nz;
		args[idx].rows_offset = rows_offset;
		args[idx].local_heap_size = local_heap_size+idx;
		args[idx].local_size = local_size+idx;
		args[idx].local_col_of_C = &local_col_of_C[idx];
		args[idx].local_rat_of_C = &local_rat_of_C[idx];
		args[idx].numa_cfg = numa_cfg;
		rv = pthread_create(&threadpool[idx], &attr, pthread_init_local, args+idx);
		if (rv){
			printf("ERROR; return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

	for (int idx = 0; idx < THREAD_NUM; idx ++) {
		rv = pthread_join(threadpool[idx], NULL);
		if (rv){
			printf("ERROR; return code from pthread_join() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

	pthread_attr_destroy(&attr);
}


#ifdef IN_DEBUG
static void __attribute__((unused)) check_heap (entry_t *entry, my_cnt_t cnt) {
	for (my_cnt_t idx = 0; idx * 2 + 1 < cnt; idx ++) {
		my_cnt_t left = 2 * idx + 1;
		my_cnt_t right = 2 * idx + 2;

		if (entry[idx].key > entry[left].key) {
			printf("heap property doesn't hold for left childs\n");
			printf("cnt: %ld\tidx: %ld\tleft: %ld:\t%ld-%ld\n", 
				cnt, idx, left, entry[idx].key, entry[left].key
			);
			exit(1);
		}

		if (right == cnt ) {
			break;
		}

		if (entry[idx].key > entry[right].key) {
			printf("heap property doesn't hold for right childs\n");
			printf("cnt: %ld\tidx: %ld\tright: %ld:\t%ld-%ld\n", 
				cnt, idx, right, entry[idx].key, entry[right].key
			);
			exit(1);
		}
	}
	printf("pass one check\n");
}
#endif	/* IN_DEBUG */


// Iterative function to heapify a min heap
static void heapify_down(entry_t *entry, my_cnt_t cnt, my_cnt_t idx) {
	while (idx * 2 + 1 < cnt) {
		my_cnt_t smallest = idx;
		my_cnt_t left = 2 * idx + 1;
		my_cnt_t right = 2 * idx + 2;

		// If left child exists and is smaller than root
		if (left < cnt && entry[left].key < entry[smallest].key) {
			smallest = left;
		}

		// If right child exists and is smaller than the smallest so far
		if (right < cnt && entry[right].key < entry[smallest].key) {
			smallest = right;
		}

		// If the smallest element is the root itself, no further sifting is needed
		if (smallest == idx) {
			break;
		}

		// Otherwise, swap the current node with the smallest child
		entry_t temp = entry[idx];
		entry[idx] = entry[smallest];
		entry[smallest] = temp;

		// Move down to the smallest child and repeat the process
		idx = smallest;
	}
}


static void make_heap(entry_t *entry, my_cnt_t cnt) {
	for (my_cnt_t idx = cnt/2 - 1; idx >= 0; idx --) {
		heapify_down(entry, cnt, idx);
	}
}


static void pop_heap(entry_t *entry, my_cnt_t cnt) {
	if (cnt == 1 || cnt == 0) {
		return;
	}

	entry_t temp = entry[0];
	entry[0] = entry[cnt-1];	// index starts from 0
	entry[cnt-1] = temp;
	cnt --;

	heapify_down(entry, cnt, 0);
}


static void push_heap(entry_t *entry, my_cnt_t idx) {
	idx --;		// index starts from 0
	while (idx > 0) {
		my_cnt_t parent = (idx - 1) / 2;

		if (entry[parent].key <= entry[idx].key) {
			break;
		}

		entry_t temp = entry[idx];
		entry[idx] = entry[parent];
		entry[parent] = temp;

		idx = parent;
	}
}


static void* pthread_heap_numeric(void *param) {
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

	heap_numeric_arg_t *arg = (heap_numeric_arg_t *) param;
	int _tid = arg->_tid;
	func_op_t func_op = arg->func_op;
	csr_t *csr_a = arg->csr_a;
	csr_t *csr_b = arg->csr_b;
	my_cnt_t *rows_offset = arg->rows_offset;
	my_cnt_t *row_start_idcs = arg->row_start_idcs;
	my_cnt_t *row_end_idcs = arg->row_end_idcs;
	my_cnt_t local_heap_size = arg->local_heap_size;
	my_cnt_t *real_row_nz = arg->real_row_nz;
	my_cnt_t *local_col_of_C = arg->local_col_of_C;
	my_rat_t *local_rat_of_C = arg->local_rat_of_C;

	entry_t *merge_heap = (entry_t *) alloc2_memory(
		sizeof(entry_t) * local_heap_size, arg->numa_cfg
	);

	for (my_cnt_t idx = rows_offset[_tid]; idx < rows_offset[_tid+1]; idx ++) {
		my_cnt_t kdx = 0;

#ifdef IN_INSP
		struct timespec insp_time_start, insp_time_end;
		clock_gettime(CLOCK_REALTIME, &insp_time_start);
#endif /* IN_INSP */

		for (my_cnt_t jdx = csr_a->row_ptrs[idx]; jdx < csr_a->row_ptrs[idx+1]; jdx++) {
			my_cnt_t inner = csr_a->cols[jdx];
			my_cnt_t npins = csr_b->row_ptrs[inner+1] - csr_b->row_ptrs[inner];
			if (npins > 0) {
				merge_heap[kdx].loc = 1;
				merge_heap[kdx].runr = jdx;
				merge_heap[kdx].value = func_op(csr_a->rats[jdx], csr_b->rats[csr_b->row_ptrs[inner]]);
				merge_heap[kdx++].key = csr_b->cols[csr_b->row_ptrs[inner]];
			}
		}

		my_cnt_t hsize = kdx;
		make_heap(merge_heap, hsize);

#ifdef IN_DEBUG
		// printf("%s:%d\t", __FILE__, __LINE__);
		// check_heap(merge_heap, hsize);
#endif /* IN_DEBUG */

		while (hsize > 0) {
			pop_heap(merge_heap, hsize);

#ifdef IN_DEBUG
			// printf("%s:%d\t", __FILE__, __LINE__);
			// check_heap(merge_heap, hsize-1);
#endif /* IN_DEBUG */

			entry_t hentry = merge_heap[hsize-1];
			if (row_end_idcs[idx] > row_start_idcs[idx] && 
				local_col_of_C[ row_end_idcs[idx] - row_start_idcs[ rows_offset[_tid] ] - 1 ] == hentry.key) {
				local_rat_of_C[ row_end_idcs[idx] - row_start_idcs[ rows_offset[_tid] ] - 1 ] += hentry.value;
			} else {
				local_col_of_C[ row_end_idcs[idx] - row_start_idcs[ rows_offset[_tid] ] ] = hentry.key;
				local_rat_of_C[ row_end_idcs[idx] - row_start_idcs[ rows_offset[_tid] ] ] = hentry.value;
				row_end_idcs[idx] ++;
			}

			my_cnt_t inner = csr_a->cols[hentry.runr];

			if ( ( csr_b->row_ptrs[inner+1] - csr_b->row_ptrs[inner] ) > hentry.loc) {
				my_cnt_t index = csr_b->row_ptrs[inner] + hentry.loc;
				merge_heap[hsize-1].loc = hentry.loc + 1;
				merge_heap[hsize-1].runr = hentry.runr;
				merge_heap[hsize-1].value = func_op(csr_a->rats[hentry.runr], csr_b->rats[index]);
				merge_heap[hsize-1].key = csr_b->cols[index];
				push_heap(merge_heap, hsize);
#ifdef IN_DEBUG
				// printf("%s:%d\t", __FILE__, __LINE__);
				// check_heap(merge_heap, hsize);
#endif /* IN_DEBUG */

			} else {
				hsize --;
			}
		}

#ifdef IN_INSP
		clock_gettime(CLOCK_REALTIME, &insp_time_end);
		_numc_time_array[idx] += diff_sec(insp_time_start, insp_time_end);
#endif /* IN_INSP */

		real_row_nz[idx] = row_end_idcs[idx] - row_start_idcs[idx];
	}

	dealloc_memory(merge_heap, sizeof(entry_t) * local_heap_size);

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
	return NULL;
}

static void heap_numeric(csr_t *csr_a, csr_t *csr_b,
	my_cnt_t *rows_offset, my_cnt_t *row_start_idcs, my_cnt_t *row_end_idcs,
	my_cnt_t *local_heap_size, my_cnt_t **local_col_of_C, my_rat_t **local_rat_of_C,
	my_cnt_t *real_row_nz, my_rat_t (*func_op)(my_rat_t, my_rat_t), numa_cfg_t *numa_cfg) {
	pthread_t threadpool[THREAD_NUM];
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	cpu_set_t set;
	int core_idx;
	int rv;

	heap_numeric_arg_t args[THREAD_NUM];
	memset(args, 0, sizeof(heap_numeric_arg_t) * THREAD_NUM);

	for (int idx = 0; idx < THREAD_NUM; idx ++) {
		core_idx = get_cpu_id(idx);
		CPU_ZERO(&set);
		CPU_SET(core_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		args[idx]._tid = idx;
		args[idx].numa_cfg = numa_cfg;
		args[idx].func_op = func_op;
		args[idx].csr_a = csr_a;
		args[idx].csr_b = csr_b;
		args[idx].rows_offset = rows_offset;
		args[idx].row_start_idcs = row_start_idcs;
		args[idx].row_end_idcs = row_end_idcs;
		args[idx].local_heap_size = local_heap_size[idx];
		args[idx].real_row_nz = real_row_nz,
		args[idx].local_col_of_C = local_col_of_C[idx];
		args[idx].local_rat_of_C = local_rat_of_C[idx];

#ifdef USE_PAPI
		memset(args[idx]._PAPI_numc_values, 0, sizeof(long long) * NUM_PAPI_EVENT );
#endif /* USE_PAPI */

		rv = pthread_create(&threadpool[idx], &attr, pthread_heap_numeric, args+idx);
		if (rv){
			printf("ERROR; return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

	for (int idx = 0; idx < THREAD_NUM; idx ++) {
		rv = pthread_join(threadpool[idx], NULL);
#ifdef IN_DEBUG
		printf("\t%d_numc_time: %.9f\n", idx, args[idx]._numc_time);
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


static void* pthread_ccpy(void *param) {
#ifdef IN_DEBUG
	struct timespec time_start, time_end;
	clock_gettime(CLOCK_REALTIME, &time_start);
#endif /* IN_DEBUG */
	ccpy_arg_t *arg = (ccpy_arg_t *) param;
	int _tid = arg->_tid;
	csr_t *csr_c = arg->csr_c;
	my_cnt_t *rows_offset = arg->rows_offset;
	my_cnt_t *row_start_idcs = arg->row_start_idcs;
	my_cnt_t *row_end_idcs = arg->row_end_idcs;
	my_cnt_t *local_col_of_C = arg->local_col_of_C;
	my_rat_t *local_rat_of_C = arg->local_rat_of_C;

	for (my_cnt_t idx = rows_offset[_tid]; idx < rows_offset[_tid+1]; idx ++) {
		memcpy(
			csr_c->cols + csr_c->row_ptrs[idx] ,
			local_col_of_C + row_start_idcs[idx] - row_start_idcs[ rows_offset[_tid] ], 
			sizeof(my_cnt_t) * (row_end_idcs[idx] - row_start_idcs[idx])
		);
		memcpy(
			csr_c->rats + csr_c->row_ptrs[idx] ,
			local_rat_of_C + row_start_idcs[idx] - row_start_idcs[ rows_offset[_tid] ], 
			sizeof(my_rat_t) * (row_end_idcs[idx] - row_start_idcs[idx])
		);
	}

#ifdef IN_DEBUG
	clock_gettime(CLOCK_REALTIME, &time_end);
	arg->_ccpy_time = diff_sec(time_start, time_end);
#endif /* IN_DEBUG */

	return NULL;
}


static void ccpy(csr_t *csr_c, 
	my_cnt_t *rows_offset,
	my_cnt_t *row_start_idcs,
	my_cnt_t *row_end_idcs,
	my_cnt_t **local_col_of_C,
	my_rat_t **local_rat_of_C) {
	pthread_t threadpool[THREAD_NUM];
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	cpu_set_t set;
	int core_idx;
	int rv;

	ccpy_arg_t args[THREAD_NUM];
	memset(args, 0, sizeof(ccpy_arg_t)*THREAD_NUM);

	for (int idx = 0; idx < THREAD_NUM; idx ++) {
		core_idx = get_cpu_id(idx);
		CPU_ZERO(&set);
		CPU_SET(core_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		args[idx]._tid = idx;
		args[idx].csr_c = csr_c;
		args[idx].rows_offset = rows_offset;
		args[idx].row_start_idcs = row_start_idcs;
		args[idx].row_end_idcs = row_end_idcs;
		args[idx].local_col_of_C = local_col_of_C[idx];
		args[idx].local_rat_of_C = local_rat_of_C[idx];
		rv = pthread_create(&threadpool[idx], &attr, pthread_ccpy, args+idx);
		if (rv) {
			printf("ERROR; return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

	for (int idx = 0; idx < THREAD_NUM; idx ++) {
		rv = pthread_join(threadpool[idx], NULL);
#ifdef IN_DEBUG
		printf("\t%d_ccpy_time: %.9f\n", idx, args[idx]._ccpy_time);
		if (idx == THREAD_NUM-1) {
			printf("\n");
		}
#endif /* IN_DEBUG */

		if (rv) {
			printf("ERROR; return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

	pthread_attr_destroy(&attr);
}


runtime_t heapspgemm(csr_t* csr_a, csr_t* csr_b, csr_t* csr_c, numa_cfg_t* numa_cfg) {
	struct timespec time_start, time_end;
	runtime_t runtime;

	// INIT
	clock_gettime(CLOCK_REALTIME, &time_start);
	csr_c->num_row = csr_a->num_row;
	csr_c->num_col = csr_b->num_col;
	csr_c->row_ptrs = (my_cnt_t*) alloc2_memory(
		sizeof(my_cnt_t)*(csr_c->num_row+1), numa_cfg
	);

	my_cnt_t *row_nz = (my_cnt_t*) alloc2_memory(
		sizeof(my_cnt_t) * csr_a->num_row, numa_cfg
	);
	my_cnt_t *rows_offset = (my_cnt_t *) alloc2_memory(
		sizeof(my_cnt_t) * (THREAD_NUM+1), numa_cfg
	);
	my_cnt_t *row_start_idcs = (my_cnt_t *)alloc2_memory(
		sizeof(my_cnt_t) * (csr_a->num_row+1), numa_cfg
	);
	my_cnt_t *row_end_idcs = (my_cnt_t *)alloc2_memory(
		sizeof(my_cnt_t) * (csr_a->num_row+1), numa_cfg
	);

	// 1st pass, csr_a->row_ptrs, csr_a->cols, csr_b->row_ptrs, csr_b->cols
	my_cnt_t total_intprod = get_intprod(csr_a, csr_b, row_nz);

	rows_offset[0] = row_start_idcs[0] = row_end_idcs[0] = 0;
	scan(row_start_idcs, row_nz, csr_a->num_row+1);
	parallel_memcpy(row_end_idcs, row_start_idcs, sizeof(my_cnt_t)*csr_a->num_row, SOCKET_CORE_NUM);
	set_rows_offset(csr_a->num_row, total_intprod, rows_offset, row_start_idcs);

	my_cnt_t** local_col_of_C = (my_cnt_t**) alloc2_memory(
		sizeof(my_cnt_t*) * THREAD_NUM, numa_cfg
	);
	my_rat_t** local_rat_of_C = (my_rat_t**) alloc2_memory(
		sizeof(my_rat_t*) * THREAD_NUM, numa_cfg
	);
	my_cnt_t* local_size = (my_cnt_t* ) alloc2_memory(
		sizeof(my_cnt_t) * THREAD_NUM, numa_cfg
	);
	my_cnt_t* local_heap_size = (my_cnt_t *) alloc2_memory(
		sizeof(my_cnt_t) * THREAD_NUM, numa_cfg
	);

	init_local(csr_a, row_nz, rows_offset, local_heap_size, local_size,
		local_col_of_C, local_rat_of_C, numa_cfg
	);

	clock_gettime(CLOCK_REALTIME, &time_end);
	runtime.init_time = diff_sec(time_start, time_end);

	// NUMERIC
	clock_gettime(CLOCK_REALTIME, &time_start);

	my_cnt_t * real_row_nz = (my_cnt_t*) alloc2_memory(
		sizeof(my_cnt_t) * csr_a->num_row, numa_cfg
	);

	heap_numeric(csr_a, csr_b, rows_offset, 
		row_start_idcs, row_end_idcs, local_heap_size, 
		local_col_of_C, local_rat_of_C, 
		real_row_nz, func_mul, numa_cfg
	);

#ifdef IN_VERIFY
	for (my_cnt_t idx = 0; idx < csr_c->num_row; idx ++) {
		if (row_nz[idx] > real_row_nz[idx]) {
			over_predict ++;
		} else if (row_nz[idx] < real_row_nz[idx]) {
			under_predict ++;
		}
	}
	printf("VERIFYPREDICT:\t%ld\t%ld\n", over_predict, under_predict);
	over_predict = under_predict = 0;
#endif /* IN_VERIFY */

	scan(csr_c->row_ptrs, real_row_nz, csr_a->num_row+1);
	csr_c->num_nnz = csr_c->row_ptrs[csr_c->num_row];

#ifdef IN_STATS
	printf("\t%ld\t%ld\t%ld\t%ld\t%ld\t%f\n", 
		csr_a->num_row, csr_a->num_col, csr_a->num_nnz,
		csr_c->num_nnz, total_intprod, 
		(double) total_intprod/csr_c->num_nnz
	);
	exit(1);
#endif /* IN_STATS */

	clock_gettime(CLOCK_REALTIME, &time_end);
	runtime.numc_time = diff_sec(time_start, time_end);

	clock_gettime(CLOCK_REALTIME, &time_start);

	csr_c->cols = (my_cnt_t*) alloc2_memory(
		sizeof(my_cnt_t)*(csr_c->num_nnz), numa_cfg
	);
	csr_c->rats = (my_rat_t*) alloc2_memory(
		sizeof(my_rat_t)*csr_c->num_nnz, numa_cfg
	);
	// memset(csr_c->cols, 0, sizeof(my_cnt_t)*csr_c->num_nnz);
	// memset(csr_c->rats, 0, sizeof(my_rat_t)*csr_c->num_nnz);
	// parallel_memset(csr_c->cols, 0, sizeof(my_cnt_t)*csr_c->num_nnz, SOCKET_CORE_NUM);
	// parallel_memset(csr_c->rats, 0, sizeof(my_rat_t)*csr_c->num_nnz, SOCKET_CORE_NUM);

	ccpy(csr_c, rows_offset,
		row_start_idcs, row_end_idcs, 
		local_col_of_C, local_rat_of_C
	);

	dealloc_memory(row_start_idcs, sizeof(my_cnt_t) * (csr_a->num_row+1));
	dealloc_memory(row_end_idcs, sizeof(my_cnt_t) * (csr_a->num_row+1));
	dealloc_memory(row_nz, sizeof(my_cnt_t) * csr_a->num_row);
	dealloc_memory(real_row_nz, sizeof(my_cnt_t) * csr_a->num_row);

	for (int idx = 0; idx < THREAD_NUM; idx ++) {
		dealloc_memory(local_col_of_C[idx], sizeof(my_cnt_t) * local_size[idx]);
		dealloc_memory(local_rat_of_C[idx], sizeof(my_rat_t) * local_size[idx]);
	}
	dealloc_memory(local_col_of_C, sizeof(my_cnt_t*) * THREAD_NUM);
	dealloc_memory(local_rat_of_C, sizeof(my_rat_t*) * THREAD_NUM);
	dealloc_memory(local_size, sizeof(my_cnt_t) * THREAD_NUM );
	dealloc_memory(local_heap_size, sizeof(my_cnt_t) * THREAD_NUM );
	dealloc_memory(rows_offset, sizeof(my_cnt_t) * (THREAD_NUM+1) );

	clock_gettime(CLOCK_REALTIME, &time_end);
	runtime.mgmt_time = diff_sec(time_start, time_end);

	runtime.symb_time = 0.0;	// no symbolic phase in heapspgemm
	runtime.total_time = runtime.init_time+runtime.symb_time+runtime.numc_time+runtime.mgmt_time;

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
		runtime_t runtime = heapspgemm(&csr_a, &csr_b, &csr_c, &numa_cfg);

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








