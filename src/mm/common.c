#include "common.h"

#ifdef IN_VERIFY
extern int64_t over_predict;
extern int64_t under_predict;
#endif /* IN_VERIFY */


static void* comm_pthread_init_local(void *param) {
	comm_ptrl_init_arg_t *arg = (comm_ptrl_init_arg_t *) param;

	int _tid = arg->_tid;
	csr_t *csr_a = arg->csr_a;
	csr_t *csr_b = arg->csr_b;
	my_cnt_t *rows_offset = arg->rows_offset;

	my_cnt_t local_nnz = 0, local_nnz_max = 0;

	for (my_cnt_t idx = rows_offset[_tid]; idx < rows_offset[_tid+1]; idx ++) {
		local_nnz = 0;		
		for (my_cnt_t jdx = csr_a->row_ptrs[idx]; jdx < csr_a->row_ptrs[idx+1]; jdx ++) {
			my_cnt_t t_acol = csr_a->cols[jdx];

			local_nnz += csr_b->row_ptrs[t_acol+1] - csr_b->row_ptrs[t_acol];
		}
		if (local_nnz_max < local_nnz) {
			local_nnz_max = local_nnz;
		}
	}

	*(arg->local_nnz_max) = local_nnz_max;

	return NULL;
}


void comm_init_local(csr_t *csr_a, csr_t *csr_b, my_cnt_t *rows_offset, 
	my_cnt_t*local_nnz_max, numa_cfg_t* numa_cfg) {

	pthread_t threadpool[THREAD_NUM];
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	cpu_set_t set;
	int core_idx;
	int rv;

	comm_ptrl_init_arg_t args[THREAD_NUM];
	memset(args, 0, sizeof(comm_ptrl_init_arg_t)*THREAD_NUM);

	for (int  idx = 0 ; idx < THREAD_NUM; idx ++) {
		core_idx = get_cpu_id(idx);
		CPU_ZERO(&set);
		CPU_SET(core_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		args[idx]._tid = idx;
		args[idx].csr_a = csr_a;
		args[idx].csr_b = csr_b;
		args[idx].rows_offset = rows_offset;
		args[idx].local_nnz_max = local_nnz_max+idx;
		args[idx].numa_cfg = numa_cfg;
		rv = pthread_create(&threadpool[idx], &attr, comm_pthread_init_local, args+idx);
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


static void* pthread_esc_symbolic(void *param) {
#ifdef IN_DEBUG
	struct timespec time_start, time_end;
	clock_gettime(CLOCK_REALTIME, &time_start);
#endif /* IN_DEBUG */

	esc_symbolic_arg_t *arg = (esc_symbolic_arg_t *) param;
	int _tid = arg->_tid;
	csr_t *csr_a = arg->csr_a;
	csr_t *csr_b = arg->csr_b;
	my_cnt_t *rows_offset = arg->rows_offset;
	my_cnt_t *row_nz = arg->row_nz;
	numa_cfg_t * numa_cfg = arg->numa_cfg;

	my_cnt_t local_nnz_max = arg->local_nnz_max;

	if (local_nnz_max > csr_b->num_col) {
		local_nnz_max = csr_b->num_col;
	}

	local_nnz_max = NEXT_POW_2(csr_b->num_col);

	my_cnt_t *check = (my_cnt_t*) alloc2_memory(
		sizeof(my_cnt_t) * local_nnz_max, numa_cfg
	);


	for (my_cnt_t idx = rows_offset[_tid]; idx < rows_offset[_tid + 1]; idx ++) {

		register my_cnt_t nz = 0;

		if (row_nz[idx] > 0) {
			my_cnt_t ht_size = NEXT_POW_2(row_nz[idx]);
			if (ht_size > local_nnz_max) {
				ht_size = local_nnz_max;
			}
			memset(check, -1, sizeof(my_cnt_t)*ht_size);

			for (my_cnt_t jdx = csr_a->row_ptrs[idx]; jdx < csr_a->row_ptrs[idx+1]; jdx ++) {
				my_cnt_t t_acol = csr_a->cols[jdx];
				for (my_cnt_t kdx = csr_b->row_ptrs[t_acol]; kdx < csr_b->row_ptrs[t_acol+1]; kdx ++) {
					my_cnt_t key = csr_b->cols[kdx];
					my_cnt_t hash = (key * HASH_CONSTANT) & (ht_size - 1);

					while (true) { // Loop for hash probing
						if (check[hash] == key) { // if the key is already inserted, it's ok
							break;
						} else if (check[hash] == -1) { // if the key has not been inserted yet, then it's added.
							check[hash] = key;
							nz++;
							break;
						} else { // linear probing: check next entry
							hash = (hash + 1) & (ht_size - 1); //hash = (hash + 1) % ht_size
						}
					}

				}
			}

		}



#ifdef IN_VERIFY
		if (row_nz[idx] > nz) {
			(arg->over_predict) ++;
		}

		if (row_nz[idx] < nz) {
			(arg->under_predict) ++;
		}
#endif /* IN_VERIFY */

		row_nz[idx] = nz;

	}

	dealloc_memory(check, sizeof(my_cnt_t) * local_nnz_max);


#ifdef IN_DEBUG
	clock_gettime(CLOCK_REALTIME, &time_end);
	arg->_symb_time = diff_sec(time_start, time_end);
#endif /* IN_DEBUG */

	return NULL;
}


void esc_symbolic(csr_t* csr_a, csr_t* csr_b,
	my_cnt_t* rows_offset, my_cnt_t* row_nz, 
	my_cnt_t* local_nnz_max, numa_cfg_t* numa_cfg) {
	pthread_t threadpool[THREAD_NUM];
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	cpu_set_t set;
	int core_idx;
	int rv;

	esc_symbolic_arg_t args[THREAD_NUM];
	memset(args, 0, sizeof(esc_symbolic_arg_t) * THREAD_NUM);

	for (int idx = 0; idx < THREAD_NUM; idx ++) {
		core_idx = get_cpu_id(idx);
		CPU_ZERO(&set);
		CPU_SET(core_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		args[idx]._tid = idx;
		args[idx].csr_a = csr_a;
		args[idx].csr_b = csr_b;
		args[idx].rows_offset = rows_offset;
		args[idx].row_nz = row_nz;
		args[idx].local_nnz_max = local_nnz_max[idx];
		args[idx].numa_cfg = numa_cfg;
		rv = pthread_create(&threadpool[idx], &attr, pthread_esc_symbolic, args+idx);
		if (rv){
			printf("ERROR; return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

#ifdef IN_VERIFY
	over_predict = 0;
	under_predict = 0;
#endif /* IN_VERIFY */

	for (int idx = 0; idx < THREAD_NUM; idx ++) {
		rv = pthread_join(threadpool[idx], NULL);

#ifdef IN_DEBUG
		printf("\t%d_symb_time: %.9f\n", idx, args[idx]._symb_time);
		if (idx == THREAD_NUM-1) {
			printf("\n");
		}
#endif /* IN_DEBUG */

#ifdef IN_VERIFY
		over_predict += args[idx].over_predict;
		under_predict += args[idx].under_predict;
#endif /* IN_VERIFY */
		if (rv) {
			printf("ERROR; return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

	pthread_attr_destroy(&attr);

#ifdef IN_VERIFY
	printf("VERIFYPREDICT:\t%ld\t%ld\n", over_predict, under_predict);
#endif /* IN_VERIFY */

}


void init_bin(bin_t* bin, my_cnt_t num_row, my_cnt_t ht_size, numa_cfg_t *numa_cfg) {
	memset(bin, 0, sizeof(bin_t));

	bin->row_nz = (my_cnt_t*) alloc2_memory(
		sizeof(my_cnt_t) * num_row, numa_cfg
	);

	bin->rows_offset = (my_cnt_t*) alloc2_memory(
		sizeof(my_cnt_t) * (THREAD_NUM+1), numa_cfg
	);

	bin->bin_id = (char*) alloc2_memory(
		sizeof(char) * num_row, numa_cfg
	);

	bin->local_hash_table_id = (my_cnt_t **) alloc2_memory(
		sizeof(my_cnt_t*) * THREAD_NUM, numa_cfg
	);

	bin->local_hash_table_val = (my_rat_t **) alloc2_memory(
		sizeof(my_rat_t*) * THREAD_NUM, numa_cfg
	);

	// memset(bin->row_nz, 0, sizeof(my_cnt_t) * num_row);
	// memset(bin->rows_offset, 0, sizeof(my_cnt_t) * (THREAD_NUM+1));
	// memset(bin->bin_id, 0, sizeof(char) * num_row);
	// memset(bin->local_hash_table_id, 0, sizeof(my_cnt_t*) * THREAD_NUM);
	// memset(bin->local_hash_table_val, 0, sizeof(my_rat_t*) * THREAD_NUM);

	bin->num_row = num_row;
	bin->min_ht_size = ht_size;
}


void free_bin(bin_t *bin) {
	dealloc_memory(bin->row_nz, sizeof(my_cnt_t)*bin->num_row);
	dealloc_memory(bin->rows_offset, sizeof(my_cnt_t)*(THREAD_NUM+1));
	dealloc_memory(bin->bin_id, sizeof(char)*bin->num_row);

	for (int idx = 0; idx < THREAD_NUM; idx ++) {
		dealloc_memory(bin->local_hash_table_id[idx], sizeof(my_cnt_t)*bin->ht_size_array[idx]);
		dealloc_memory(bin->local_hash_table_val[idx], sizeof(my_rat_t)*bin->ht_size_array[idx]);
	}

	dealloc_memory(bin->local_hash_table_id, sizeof(my_cnt_t*)*THREAD_NUM);
	dealloc_memory(bin->local_hash_table_val, sizeof(my_rat_t*)*THREAD_NUM);
}


static void *pthread_set_intprod_num(void *param) {
	pthread_set_intprod_num_arg_t *arg = (pthread_set_intprod_num_arg_t *) param;
	int _tid = arg->_tid;
	bin_t *bin = arg->bin;
	csr_t *csr_a = arg->csr_a;
	csr_t *csr_b = arg->csr_b;

	my_cnt_t each_intprod = 0;
	my_cnt_t nz_per_row = 0;

	my_cnt_t start_idx = _tid * (csr_a->num_row/THREAD_NUM);
	my_cnt_t end_idx = (_tid == THREAD_NUM-1) ? csr_a->num_row : 
		(_tid+1) * (csr_a->num_row/THREAD_NUM);

	for (my_cnt_t idx = start_idx; idx < end_idx; idx++) {
		nz_per_row = 0;
		for (my_cnt_t jdx = csr_a->row_ptrs[idx]; jdx < csr_a->row_ptrs[idx+1]; jdx ++) {
			nz_per_row += csr_b->row_ptrs[ csr_a->cols[jdx]+1 ] - csr_b->row_ptrs[ csr_a->cols[jdx] ];
		}
		bin->row_nz[idx] = nz_per_row;
		each_intprod += nz_per_row;
	}

	arg->intprod = each_intprod;
	// __sync_fetch_and_add(&bin->total_intprod, each_intprod);
	return NULL;
}


void set_intprod_num(bin_t *bin, csr_t *csr_a, csr_t *csr_b) {
	pthread_t threadpool[THREAD_NUM];
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	pthread_set_intprod_num_arg_t args[THREAD_NUM];
	memset(args, 0, sizeof(pthread_set_intprod_num_arg_t)*THREAD_NUM);
	cpu_set_t set;
	int core_idx;
	int rv;
	for (int idx = 0; idx < THREAD_NUM; idx ++) {
		core_idx = get_cpu_id(idx);
		CPU_ZERO(&set);
		CPU_SET(core_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		args[idx]._tid = idx;
		args[idx].intprod = 0;
		args[idx].bin = bin;
		args[idx].csr_a = csr_a;
		args[idx].csr_b = csr_b;
		rv = pthread_create(&threadpool[idx], &attr, pthread_set_intprod_num, args+idx);
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
#ifdef IN_DEBUG
		printf("\t%d_intprod: %ld\n", idx, args[idx].intprod);
		if (idx == THREAD_NUM-1) {
			printf("\n");
		}
#endif /* IN_DEBUG */
		bin->total_intprod += args[idx].intprod;
	}

	pthread_attr_destroy(&attr);
}


void *pthread_hash_set_rows_offset(void *param) {
	pthread_hash_set_rows_offset_arg_t *arg = (pthread_hash_set_rows_offset_arg_t *) param;
	int _tid = arg->_tid;
	my_cnt_t *ps_row_nz = arg->ps_row_nz;
	bin_t *bin = arg->bin;
	my_cnt_t num_row = arg->num_row;
	my_cnt_t average_intprod = arg->average_intprod;

	bin->rows_offset[_tid+1] = lower_bound(
		ps_row_nz, 
		ps_row_nz + num_row + 1, 
		average_intprod * (_tid + 1)
	) - ps_row_nz;

	return NULL;
}


void hash_set_rows_offset(bin_t *bin, my_cnt_t num_row, numa_cfg_t* numa_cfg) {
	my_cnt_t *ps_row_nz = (my_cnt_t *) alloc2_memory(
		sizeof(my_cnt_t) * (num_row+1), numa_cfg
	);
	// memset(ps_row_nz, 0, sizeof(my_cnt_t) * num_row);
	scan(ps_row_nz, bin->row_nz, num_row+1);

	my_cnt_t average_intprod = (bin->total_intprod + THREAD_NUM - 1) / THREAD_NUM;

	/* Search end point of each range */
	bin->rows_offset[0] = 0;

	pthread_t threadpool[THREAD_NUM];
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	cpu_set_t set;
	int core_idx;
	int rv;

	pthread_hash_set_rows_offset_arg_t args[THREAD_NUM];
	memset(args, 0, sizeof(pthread_hash_set_rows_offset_arg_t)*THREAD_NUM);

	for (int idx = 0; idx < THREAD_NUM; idx ++) {
		core_idx = get_cpu_id(idx);
		CPU_ZERO(&set);
		CPU_SET(core_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		args[idx]._tid = idx;
		args[idx].num_row = num_row;
		args[idx].average_intprod = average_intprod;
		args[idx].ps_row_nz = ps_row_nz;
		args[idx].bin = bin;
		rv = pthread_create(&threadpool[idx], &attr, pthread_hash_set_rows_offset, args+idx);
		if (rv){
			printf("ERROR; return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

	for (int idx = 0; idx < THREAD_NUM; idx ++) {
		rv = pthread_join(threadpool[idx], NULL);
#ifdef IN_DEBUG
		// my_cnt_t flop_estimated = ps_row_nz[bin->rows_offset[idx+1]]
		// 	- ps_row_nz[bin->rows_offset[idx]];
		if (idx == THREAD_NUM -1 ) {
			bin->rows_offset[THREAD_NUM] = num_row;
			// flop_estimated = ps_row_nz[bin->rows_offset[idx+1]]
				// - ps_row_nz[bin->rows_offset[idx]];
		}
		// printf("\t%d_flop_estimated: %ld\n", idx, flop_estimated);
		// if (idx == THREAD_NUM-1) {
		// 	printf("\n");
		// }
#endif /* IN_DEBUG */

		if (rv) {
			printf("ERROR; return code from pthread_join() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

	bin->rows_offset[THREAD_NUM] = num_row;

	dealloc_memory(ps_row_nz, sizeof(my_cnt_t) * (num_row+1));

	pthread_attr_destroy(&attr);
}


static void *pthread_set_bin_id(void *param) {
	pthread_set_bin_id_arg_t *arg = (pthread_set_bin_id_arg_t *) param;
	int _tid = arg->_tid;
	my_cnt_t num_row = arg->num_row;
	my_cnt_t num_col = arg->num_col;
	bin_t *bin = arg->bin;

	my_cnt_t each_num_row = num_row/THREAD_NUM;
	my_cnt_t start_idx = _tid * each_num_row;
	my_cnt_t end_idx = (_tid == THREAD_NUM-1) ? num_row : (_tid+1) * each_num_row;

	for (my_cnt_t idx = start_idx; idx < end_idx; idx ++) {
		my_cnt_t nz_per_row = bin->row_nz[idx];

		if (nz_per_row > num_col) {
			nz_per_row = num_col;
		}

		if (nz_per_row == 0) {
			bin->bin_id[idx] = 0;
		} else {
			my_cnt_t jdx = 0;
			while (nz_per_row > (bin->min_ht_size << jdx)) {
				jdx++;
			}
			bin->bin_id[idx] = jdx + 1;
		}
	}

	return NULL;
}


void set_bin_id(bin_t *bin, my_cnt_t num_row, my_cnt_t num_col) {
	pthread_t threadpool[THREAD_NUM];
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	cpu_set_t set;
	int core_idx;
	int rv;

	pthread_set_bin_id_arg_t args[THREAD_NUM];
	memset(args, 0, sizeof(pthread_set_bin_id_arg_t)*THREAD_NUM);

	for (int idx = 0; idx < THREAD_NUM; idx ++) {
		core_idx = get_cpu_id(idx);
		CPU_ZERO(&set);
		CPU_SET(core_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		args[idx]._tid = idx;
		args[idx].num_row = num_row;
		args[idx].num_col = num_col;
		args[idx].bin = bin;
		rv = pthread_create(&threadpool[idx], &attr, pthread_set_bin_id, args+idx);
		if (rv){
			printf("ERROR; return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

	for (int idx = 0; idx < THREAD_NUM; idx ++) {
		rv = pthread_join(threadpool[idx], NULL);
		if (rv) {
			printf("ERROR; return code from pthread_join() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

	pthread_attr_destroy(&attr);	
}


// TODO: set ht_size to num_col to skip this computation cost (may not need to, as num_col is several orders larger)
static void *pthread_bin_create_local_hash_table(void *param) {
	pthread_bin_create_local_hash_table_arg_t *arg = \
		(pthread_bin_create_local_hash_table_arg_t *) param;\
	int _tid = arg->_tid;
	my_cnt_t num_col = arg->num_col;
	bin_t *bin = arg->bin;

	my_cnt_t ht_size = 0;

	for (my_cnt_t idx = bin->rows_offset[_tid]; idx < bin->rows_offset[_tid+1]; idx++) {
		if (ht_size < bin->row_nz[idx]) {
			ht_size = bin->row_nz[idx];
		}
	}

	/* the size of hash table is aligned as 2^n */
	if (ht_size > 0) {
		if (ht_size > num_col) {
			ht_size = num_col;
		}

		my_cnt_t k = bin->min_ht_size;
		while (k < ht_size) {
			k <<= 1;
		}
		ht_size = k;
	}

	bin->local_hash_table_id[_tid] = (my_cnt_t*) alloc2_memory(
		sizeof(my_cnt_t)*ht_size, arg->numa_cfg
	);
	// memset(bin->local_hash_table_id[_tid], 0, sizeof(my_cnt_t)*ht_size);

	bin->local_hash_table_val[_tid] = (my_rat_t*) alloc2_memory(
		sizeof(my_rat_t)*ht_size, arg->numa_cfg
	);
	// memset(bin->local_hash_table_val[_tid], 0, sizeof(my_cnt_t)*ht_size);

	bin->ht_size_array[_tid] = ht_size;

	return NULL;
}


void bin_create_local_hash_table(bin_t *bin, my_cnt_t num_col, numa_cfg_t* numa_cfg) {
	pthread_t threadpool[THREAD_NUM];
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	cpu_set_t set;
	int core_idx;
	int rv;

	pthread_bin_create_local_hash_table_arg_t args[THREAD_NUM];
	memset(args, 0, sizeof(pthread_bin_create_local_hash_table_arg_t)*THREAD_NUM);

	for (int idx = 0; idx < THREAD_NUM; idx ++) {
		core_idx = get_cpu_id(idx);
		CPU_ZERO(&set);
		CPU_SET(core_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		args[idx]._tid = idx;
		args[idx].numa_cfg = numa_cfg;
		args[idx].num_col = num_col;
		args[idx].bin = bin;
		rv = pthread_create(&threadpool[idx], &attr, 
			pthread_bin_create_local_hash_table, args+idx);
		if (rv){
			printf("ERROR; return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

	for (int idx = 0; idx < THREAD_NUM; idx ++) {
		rv = pthread_join(threadpool[idx], NULL);
		if (rv) {
			printf("ERROR; return code from pthread_join() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

	pthread_attr_destroy(&attr);
}


static void *pthread_hash_symbolic(void *param) {
#ifdef IN_DEBUG
	struct timespec time_start, time_end;
	clock_gettime(CLOCK_REALTIME, &time_start);
#endif /* IN_DEBUG */

	hash_symbolic_arg_t *arg = (hash_symbolic_arg_t *) param;
	int _tid = arg->_tid;
	csr_t *csr_a = arg->csr_a;
	csr_t *csr_b = arg->csr_b;
	bin_t *bin = arg->bin;

	my_cnt_t start_row = bin->rows_offset[_tid];
	my_cnt_t end_row = bin->rows_offset[_tid+1];

	my_cnt_t *check = bin->local_hash_table_id[_tid];

	for (my_cnt_t idx = start_row; idx < end_row; idx++) {
		my_cnt_t nz = 0;
		my_cnt_t bid = bin->bin_id[idx];

		if (bid > 0) {
			my_cnt_t ht_size = MIN_HT_S << (bid - 1); 	// determine hash table size for i-th row
			memset(check, -1, sizeof(my_cnt_t)*ht_size);

			for (my_cnt_t jdx = csr_a->row_ptrs[idx]; jdx < csr_a->row_ptrs[idx+1]; jdx++) {
				my_cnt_t t_acol = csr_a->cols[jdx];
				for (my_cnt_t kdx = csr_b->row_ptrs[t_acol]; kdx < csr_b->row_ptrs[t_acol+1]; kdx++) {

					my_cnt_t key = csr_b->cols[kdx];
					my_cnt_t hash = (key * HASH_CONSTANT) & (ht_size - 1);

					while (true) { // Loop for hash probing
						if (check[hash] == key) { // if the key is already inserted, it's ok
							break;
						} else if (check[hash] == -1) { // if the key has not been inserted yet, then it's added.
							check[hash] = key;
							nz++;
							break;
						} else { // linear probing: check next entry
							hash = (hash + 1) & (ht_size - 1); //hash = (hash + 1) % ht_size
						}
					}
				}
			}
		}

#ifdef IN_VERIFY
		if (bin->row_nz[idx] > nz) {
			(arg->over_predict) ++;
		}

		if (bin->row_nz[idx] < nz) {
			(arg->under_predict) ++;
		}
#endif /* IN_VERIFY */
		bin->row_nz[idx] = nz;

	}

#ifdef IN_DEBUG
	clock_gettime(CLOCK_REALTIME, &time_end);
	arg->_symb_time = diff_sec(time_start, time_end);
#endif /* IN_DEBUG */

	return NULL;
}

void hash_symbolic(csr_t *csr_a, csr_t *csr_b, bin_t *bin) {
	pthread_t threadpool[THREAD_NUM];
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	cpu_set_t set;
	int core_idx;
	int rv;

	hash_symbolic_arg_t args[THREAD_NUM];
	memset(args, 0, sizeof(hash_symbolic_arg_t)*THREAD_NUM);

	for (int idx = 0; idx < THREAD_NUM; idx ++) {
		core_idx = get_cpu_id(idx);
		CPU_ZERO(&set);
		CPU_SET(core_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		args[idx]._tid = idx;
		args[idx].csr_a = csr_a;
		args[idx].csr_b = csr_b;
		args[idx].bin = bin;
		rv = pthread_create(&threadpool[idx], &attr, pthread_hash_symbolic, args+idx);
		if (rv){
			printf("ERROR; return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

#ifdef IN_VERIFY
	over_predict = 0;
	under_predict = 0;
#endif /* IN_VERIFY */

	for (int idx = 0; idx < THREAD_NUM; idx ++) {
		rv = pthread_join(threadpool[idx], NULL);

#ifdef IN_DEBUG
		printf("\t%d_symb_time: %.9f\n", idx, args[idx]._symb_time);
		if (idx == THREAD_NUM-1) {
			printf("\n");
		}
#endif /* IN_DEBUG */

#ifdef IN_VERIFY
		over_predict += args[idx].over_predict;
		under_predict += args[idx].under_predict;
#endif /* IN_VERIFY */
		if (rv) {
			printf("ERROR; return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

	pthread_attr_destroy(&attr);


#ifdef IN_VERIFY
	printf("VERIFYPREDICT:\t%ld\t%ld\n", over_predict, under_predict);
#endif /* IN_VERIFY */
}



// -------------------------------------------------------------------------
// Comparison function: descending freq, then descending nnz
// -------------------------------------------------------------------------
int compare_frid_nnz(const void *a, const void *b) {
	const frid_nnz_t *fa = (const frid_nnz_t *)a;
	const frid_nnz_t *fb = (const frid_nnz_t *)b;

	// Descending order by freq
	if (fa->freq > fb->freq) return -1; // fa < fb in "qsort terms"
	if (fa->freq < fb->freq) return  1; // fa > fb

	// If freq is the same, descending order by nnz
	if (fa->nnz > fb->nnz)   return -1;
	if (fa->nnz < fb->nnz)   return  1;

	return 0;
}


// -------------------------------------------------------------------------
// Threaded qsort: copies a subarray into a private buffer, sorts it
// -------------------------------------------------------------------------
#if THREAD_NUM > 1
static void* pthread_qsort_frid(void *param) {
	pthread_qsort_frid_arg_t *arg = (pthread_qsort_frid_arg_t *) param;

	// Allocate memory for the chunk
	arg->res = (frid_nnz_t *)alloc2_memory(
		sizeof(frid_nnz_t) * arg->count, arg->numa_cfg
	);
	if (!arg->res) {
		fprintf(stderr, "[thread %d] Failed to allocate memory for qsort.\n", arg->_tid);
		pthread_exit(NULL);
	}

	// Copy data from the original array
	memcpy(arg->res, arg->base + arg->start, sizeof(frid_nnz_t) * arg->count);

	// Sort using qsort
	qsort(arg->res, arg->count, sizeof(frid_nnz_t), arg->func_cmp);

	// (Optional) check_sorted, debug, etc.
	// check_sorted_frid_nnz(arg->res, arg->count);

	pthread_exit(NULL);
	return NULL;
}

// -------------------------------------------------------------------------
// Threaded merge: merges two sorted sub-buffers into a single array
// -------------------------------------------------------------------------
static void pthread_merge_frid(void *param) {
	pthread_merge_frid_arg_t *arg = (pthread_merge_frid_arg_t *) param;
	int64_t index[2] = {0, 0};

	int64_t total_count = arg->count0 + arg->count1;

	for (int64_t idx = 0; idx < total_count; idx++) {
		short flag = 0;
		// If we've exhausted source1
		if (index[1] >= arg->count1) {
			flag = 0; 
		}
		// If we've exhausted source0
		else if (index[0] >= arg->count0) {
			flag = 1;
		}
		// Compare current elements, use the same descending freq/nnz logic
		else if (arg->func_cmp(arg->source0 + index[0], arg->source1 + index[1]) <= 0) {
			// source0[idx0] is "greater or equal" in descending sense
			flag = 0;
		} else {
			flag = 1;
		}

		if (!flag) {
			arg->res[idx] = arg->source0[index[0]];
			index[0]++;
		} else {
			arg->res[idx] = arg->source1[index[1]];
			index[1]++;
		}
	}

	// Free temporary buffers
	dealloc_memory(arg->source0, sizeof(frid_nnz_t)*arg->count0);
	dealloc_memory(arg->source1, sizeof(frid_nnz_t)*arg->count1);

	// (Optional) check_sorted_frid_nnz(arg->res, total_count);
	// In the original snippet, there's no return here, just a function. 
	// If you prefer a thread-based merge, you'd do pthread_exit(NULL).
}
#endif /* THREAD_NUM > 1 */

// -------------------------------------------------------------------------
// p_qsortmerge_frid: top-level function that either sorts single-threaded
//                    or uses multi-threaded chunking + iterative merging
// -------------------------------------------------------------------------
void p_qsortmerge_frid_nnz(frid_nnz_t *base, my_cnt_t number, 
	int (*func_cmp)(const void*, const void*), numa_cfg_t *numa_cfg) {
#if THREAD_NUM > 1
	// If the data size is small, fall back to single-threaded qsort
	if (number < (1 << 10)) {
		qsort(base, number, sizeof(frid_nnz_t), compare_frid_nnz);
		return;
	}

	// Otherwise, proceed with multi-threaded sort
	pthread_t threadpool[THREAD_NUM];
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	// Prepare qsort arguments
	pthread_qsort_frid_arg_t qsort_args[THREAD_NUM];
	memset(qsort_args, 0, sizeof(qsort_args));

	// Divide the data roughly evenly among threads
	int64_t num_per_thr = number / THREAD_NUM;

	for (int idx = 0; idx < THREAD_NUM; idx++) {
		// Optional: set CPU affinity if desired
		int core_idx = get_cpu_id(idx);
		cpu_set_t set;
		CPU_ZERO(&set);
		CPU_SET(core_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);

		qsort_args[idx]._tid           = idx;
		qsort_args[idx].numa_cfg  = numa_cfg;
		qsort_args[idx].start          = idx * num_per_thr;
		qsort_args[idx].count          = (idx == THREAD_NUM - 1) 
										  ? (number - qsort_args[idx].start)
										  : num_per_thr;
		qsort_args[idx].base           = base;
		qsort_args[idx].func_cmp       = func_cmp;
		int rv = pthread_create(&threadpool[idx], &attr, pthread_qsort_frid, &qsort_args[idx]);
		if (rv) {
			printf("ERROR: return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

	// Join all threads
	for (int idx = 0; idx < THREAD_NUM; idx++) {
		int rv = pthread_join(threadpool[idx], NULL);
		if (rv) {
			printf("ERROR: return code from pthread_join() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

	// Now we have THREAD_NUM sorted sub-lists in qsort_args[idx].res
	// Next: iteratively merge them until one remains
	merge_part_frid_t merge_parts[THREAD_NUM];
	memset(merge_parts, 0, sizeof(merge_parts));

	for (int idx = 0; idx < THREAD_NUM; idx++) {
		merge_parts[idx].count = qsort_args[idx].count;
		merge_parts[idx].res   = qsort_args[idx].res;
	}

	int merge_num = THREAD_NUM;
	// While more than 2 lists remain, merge them in pairs
	while (merge_num > 2) {
		int pairs = merge_num / 2;
		// We will do pairs merges
		pthread_merge_frid_arg_t merge_args[pairs];
		memset(merge_args, 0, sizeof(merge_args));

		for (int idx = 0; idx < pairs; idx++) {
			// Optional: set CPU affinity
			int core_idx = get_cpu_id(idx);
			cpu_set_t set;
			CPU_ZERO(&set);
			CPU_SET(core_idx, &set);
			pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);

			merge_args[idx]._tid   = idx;
			merge_args[idx].count0 = merge_parts[2*idx].count;
			merge_args[idx].count1 = merge_parts[2*idx + 1].count;
			merge_args[idx].source0= merge_parts[2*idx].res;
			merge_args[idx].source1= merge_parts[2*idx + 1].res;
			merge_args[idx].res    = (frid_nnz_t *)alloc2_memory(
										 sizeof(frid_nnz_t) * 
										 (merge_args[idx].count0 + merge_args[idx].count1),
										 numa_cfg
									 );
			merge_args[idx].func_cmp = func_cmp;
			int rv = pthread_create(
						  &threadpool[idx],
						  &attr,
						  (void *(*)(void *))(uintptr_t) pthread_merge_frid,
						  &merge_args[idx]
					  );
			if (rv) {
				printf("ERROR: return code from pthread_create() is %d\n", rv);
				exit(EXIT_FAILURE);
			}
		}

		// Join these merging threads
		for (int idx = 0; idx < pairs; idx++) {
			int rv = pthread_join(threadpool[idx], NULL);
			if (rv) {
				printf("ERROR: return code from pthread_join() is %d\n", rv);
				exit(EXIT_FAILURE);
			}
			// Update merge_parts for the next iteration
			merge_parts[idx].count = merge_args[idx].count0 + merge_args[idx].count1;
			merge_parts[idx].res   = merge_args[idx].res;
		}

		// If we have an odd leftover chunk, carry it over
		if (merge_num % 2 == 1) {
			merge_parts[pairs] = merge_parts[merge_num - 1];
			merge_num = pairs + 1;
		} else {
			merge_num = pairs;
		}
	}

	// Finally, merge the last two sub-lists into the original array 'base'
	pthread_merge_frid_arg_t last_merge_arg = {
		._tid    = 0,
		.count0  = merge_parts[0].count,
		.count1  = merge_parts[1].count,
		.source0 = merge_parts[0].res,
		.source1 = merge_parts[1].res,
		.res     = base,  // final output goes here
		.func_cmp = func_cmp
	};
	pthread_merge_frid(&last_merge_arg);

	// Cleanup
	pthread_attr_destroy(&attr);

#else
	// If THREAD_NUM <= 1, just do a single-threaded qsort
	qsort(base, number, sizeof(frid_nnz_t), func_cmp);
#endif /* THREAD_NUM > 1 */
}

void check_sorted_frid_nnz(frid_nnz_t *arr, int64_t num_elements, 
	int (*func_cmp)(const void*, const void*)) {
	for (int64_t idx = 0; idx < num_elements - 1; idx++) {
		if (func_cmp(arr + idx, arr + idx + 1) > 0) {
			printf(
				"check false: arr[%ld]{freq: %.2f, nnz: %ld, rid: %ld}, "
				"arr[%ld]{freq: %.2f, nnz: %ld, rid: %ld}\n",
				idx, arr[idx].freq, arr[idx].nnz, arr[idx].rid,
				idx + 1, arr[idx + 1].freq, arr[idx + 1].nnz, arr[idx + 1].rid
			);
			continue;
		}
	}
}


void init_tub(tub_t *tub, csr_t* csr_b, numa_cfg_t *numa_cfg) {
	memset(tub, 0, sizeof(tub_t));
	tub->flag = false;
	tub->num_b_row = csr_b->num_row;
	tub->trl_b_freq = (my_cnt_t *) alloc2_memory(
		sizeof(my_cnt_t)*THREAD_NUM*csr_b->num_row, numa_cfg
	);
	tub->agg_b_freq = (my_rat_t *) alloc2_memory(
		sizeof(my_rat_t)*csr_b->num_row, numa_cfg
	);
	int __attribute__((unused)) rv = pthread_barrier_init( &(tub->barrier), NULL, THREAD_NUM );
}


void free_tub(tub_t *tub) {
	if (tub->trl_b_freq != NULL) {
		dealloc_memory(tub->trl_b_freq, sizeof(my_cnt_t)*THREAD_NUM*tub->num_b_row);
	}
	if (tub->agg_b_freq != NULL) {
		dealloc_memory(tub->agg_b_freq, sizeof(my_rat_t)*tub->num_b_row);
	}
	pthread_barrier_destroy(&(tub->barrier));
	memset(tub, 0, sizeof(tub_t));
}


void init_rlrb(rlrb_t* rlrb) {
	rlrb->trl_cnt = NULL;
	rlrb->frid_nnz = NULL;
	rlrb->rid_old2new = NULL;
	rlrb->org_csr_b = NULL;
	rlrb->dram_csr_b = NULL;
	rlrb->cxl_csr_b = NULL;
}


void free_rlrb(rlrb_t *rlrb) {
	if (rlrb->trl_cnt != NULL) {
		if (rlrb->frid_nnz != NULL) {
			dealloc_memory(rlrb->frid_nnz, sizeof(frid_nnz_t) * rlrb->trl_cnt[THREAD_NUM]);
			rlrb->frid_nnz = NULL;
		}
		if (rlrb->rid_old2new != NULL) {
			dealloc_memory(rlrb->rid_old2new, sizeof(my_cnt_t) * rlrb->org_csr_b->num_row);
			rlrb->rid_old2new = NULL;
		}
		// dealloc_memory(rlrb->trl_cnt, sizeof(my_cnt_t)*(THREAD_NUM+1));
		free(rlrb->trl_cnt);


		rlrb->org_csr_b = NULL;

		if (rlrb->dram_csr_b != NULL) {
			free_csr(rlrb->dram_csr_b);
			rlrb->dram_csr_b = NULL;
		}

		if (rlrb->cxl_csr_b) {
			free_csr(rlrb->cxl_csr_b);
			rlrb->cxl_csr_b = NULL;		
		}
	}
}


static void *pthread_set_intprod_num_tub(void *param) {
	pthread_set_intprod_num_arg_t *arg = (pthread_set_intprod_num_arg_t *) param;
	int _tid = arg->_tid;
	bin_t *bin = arg->bin;
	csr_t *csr_a = arg->csr_a;
	csr_t *csr_b = arg->csr_b;
	tub_t *tub = arg->tub;

	my_cnt_t *trl_b_freq = tub->trl_b_freq + _tid * csr_b->num_row;

	// zero the thread local b frequency array
	memset(trl_b_freq, 0, sizeof(my_cnt_t) * csr_b->num_row);

	my_cnt_t each_intprod = 0;
	my_cnt_t nz_per_row = 0;

	my_cnt_t start_idx = _tid * (csr_a->num_row/THREAD_NUM);
	my_cnt_t end_idx = (_tid == THREAD_NUM-1) ? csr_a->num_row : 
		(_tid+1) * (csr_a->num_row/THREAD_NUM);

	for (my_cnt_t idx = start_idx; idx < end_idx; idx++) {
		nz_per_row = 0;
		for (my_cnt_t jdx = csr_a->row_ptrs[idx]; jdx < csr_a->row_ptrs[idx+1]; jdx ++) {
			nz_per_row += csr_b->row_ptrs[ csr_a->cols[jdx]+1 ] - csr_b->row_ptrs[ csr_a->cols[jdx] ];

			// count the thread local b row frequency
			trl_b_freq[csr_a->cols[jdx]] ++;
		}
		bin->row_nz[idx] = nz_per_row;
		each_intprod += nz_per_row;
	}

	arg->intprod = each_intprod;

	rlrb_t *rlrb = arg->rlrb;
	if (_tid == 0) {
		// allocate one more than THREAD_NUM,
		rlrb->trl_cnt = (my_cnt_t *) calloc(THREAD_NUM+1, sizeof(my_cnt_t) );
	}

	int rv;
	BARRIER_ARRIVE(&(tub->barrier), rv);

	start_idx = _tid * (csr_b->num_row/THREAD_NUM);
	end_idx = (_tid == THREAD_NUM-1) ? csr_b->num_row : 
		(_tid+1) * (csr_b->num_row/THREAD_NUM);

	// zero the global aggregated b frequency array
	my_rat_t *agg_b_freq = tub->agg_b_freq;
	memset(agg_b_freq, 0, sizeof(my_rat_t)*(end_idx-start_idx));

	my_cnt_t local_cnt = 0;

	double max_freq = 0.0;

	status_arr_t *status_arr = arg->status_arr;
	for (my_cnt_t b_row_idx = start_idx; b_row_idx < end_idx; b_row_idx ++) {
		for (int _ttidd = 0; _ttidd < THREAD_NUM; _ttidd ++) {
			agg_b_freq[b_row_idx] += tub->trl_b_freq[_ttidd * csr_b->num_row + b_row_idx];
		}
		// normalize the global aggregated b frequency array
		agg_b_freq[b_row_idx] /= csr_a->num_nnz;

		if (max_freq < agg_b_freq[b_row_idx]) {
			max_freq = agg_b_freq[b_row_idx];
		}

		set_status(status_arr, b_row_idx, 3 * (agg_b_freq[b_row_idx] > RLRB_THLD));
		local_cnt += agg_b_freq[b_row_idx] > RLRB_THLD;
	}

	rlrb->trl_cnt[_tid] = local_cnt;
	BARRIER_ARRIVE(&(tub->barrier), rv);

	for (my_cnt_t idx = 0; idx < _tid; idx ++) {
		local_cnt += rlrb->trl_cnt[idx];
	}


	if (_tid == THREAD_NUM-1) {
		if (local_cnt > 0){
			rlrb->frid_nnz = (frid_nnz_t *) alloc2_memory(
				sizeof(frid_nnz_t) * local_cnt, arg->numa_cfg
			);

			rlrb->rid_old2new = (my_cnt_t *) alloc2_memory(
				sizeof(my_cnt_t) * csr_b->num_row, arg->numa_cfg
			);

			// set the last element to the total cnt
			rlrb->trl_cnt[THREAD_NUM] = local_cnt;
		}
	}

	BARRIER_ARRIVE(&(tub->barrier), rv);
	my_cnt_t tmp_cnt = 0;
	if (local_cnt > 0) {
		for (my_cnt_t b_row_idx = start_idx; b_row_idx < end_idx; b_row_idx ++) {
			if ( get_status(status_arr, b_row_idx) == 3) {
				rlrb->frid_nnz[local_cnt+tmp_cnt].freq = agg_b_freq[b_row_idx];
				rlrb->frid_nnz[local_cnt+tmp_cnt].nnz = \
					csr_b->row_ptrs[b_row_idx+1] - csr_b->row_ptrs[b_row_idx];
				rlrb->frid_nnz[local_cnt+tmp_cnt].rid = b_row_idx;
			}
		}
	}
	return NULL;
}


void set_intprod_num_tub(bin_t *bin, tub_t *tub, 
	status_arr_t *status_arr, rlrb_t *rlrb,
	csr_t *csr_a, csr_t *csr_b, numa_cfg_t *numa_cfg) {
	pthread_t threadpool[THREAD_NUM];
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	pthread_set_intprod_num_arg_t args[THREAD_NUM];
	memset(args, 0, sizeof(pthread_set_intprod_num_arg_t)*THREAD_NUM);
	cpu_set_t set;
	int core_idx;
	int rv;
	for (int idx = 0; idx < THREAD_NUM; idx ++) {
		core_idx = get_cpu_id(idx);
		CPU_ZERO(&set);
		CPU_SET(core_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		args[idx]._tid = idx;
		args[idx].intprod = 0;
		args[idx].bin = bin;
		args[idx].csr_a = csr_a;
		args[idx].csr_b = csr_b;
		args[idx].tub = tub;
		args[idx].status_arr = status_arr;
		args[idx].rlrb = rlrb;
		args[idx].numa_cfg = numa_cfg;
		rv = pthread_create(&threadpool[idx], &attr, pthread_set_intprod_num_tub, args+idx);
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
#ifdef IN_DEBUG
		printf("\t%d_intprod: %ld\n", idx, args[idx].intprod);
		if (idx == THREAD_NUM-1) {
			printf("\n");
		}
#endif /* IN_DEBUG */
		bin->total_intprod += args[idx].intprod;
	}

	pthread_attr_destroy(&attr);

#ifdef IN_VERIFY
	my_rat_t sum_row_freq = 0;
	for (my_cnt_t idx = 0; idx < csr_b->num_row; idx ++) {
		sum_row_freq += tub->agg_b_freq[idx];
	}
	assert(sum_row_freq == 1.000000);
	// printf("sum_row_freq: %f\n", sum_row_freq);
#endif /* IN_VERIFY */

	dealloc_memory(tub->trl_b_freq, sizeof(my_cnt_t)*THREAD_NUM*tub->num_b_row);
	dealloc_memory(tub->agg_b_freq, sizeof(my_rat_t)*tub->num_b_row);
	tub->trl_b_freq = NULL;
	tub->agg_b_freq = NULL;
}



void rlrb_pack(csr_t* csr_b, status_arr_t* status_arr,
	rlrb_t* rlrb, numa_cfg_t *numa_cfg) {
	if (rlrb->trl_cnt[THREAD_NUM] == 0) {
		return;
	}
	p_qsortmerge_frid_nnz(
		rlrb->frid_nnz, rlrb->trl_cnt[THREAD_NUM], 
		compare_frid_nnz, numa_cfg
	);

	rlrb->org_csr_b = csr_b;

	rlrb->dram_csr_b = (csr_t *) malloc(sizeof(csr_t));
	rlrb->cxl_csr_b = (csr_t *) malloc(sizeof(csr_t));
	memset(rlrb->dram_csr_b, 0, sizeof(csr_t));
	memset(rlrb->cxl_csr_b, 0, sizeof(csr_t));
	rlrb->dram_csr_b->num_col = csr_b->num_col;
	rlrb->cxl_csr_b->num_col = csr_b->num_col;


	for (my_cnt_t b_row_idx = 0; b_row_idx < csr_b->num_row; b_row_idx ++) {
		rlrb->rid_old2new[b_row_idx] = b_row_idx;	// set the default value
		if ( get_status(status_arr, b_row_idx) == 3) {
			if (b_row_idx % (DRAM_RATIO + CXL_RATIO) < DRAM_RATIO) {
				set_status(status_arr, b_row_idx, 1);
				rlrb->rid_old2new[b_row_idx] = (rlrb->dram_csr_b->num_row) ++;
				rlrb->dram_csr_b->num_nnz += csr_b->row_ptrs[b_row_idx+1] - csr_b->row_ptrs[b_row_idx];
			} else {
				set_status(status_arr, b_row_idx, 2);
				rlrb->rid_old2new[b_row_idx] = (rlrb->cxl_csr_b->num_row) ++;
				rlrb->cxl_csr_b->num_nnz += csr_b->row_ptrs[b_row_idx+1] - csr_b->row_ptrs[b_row_idx];
			}
		}
	}


	rlrb->dram_csr_b->row_ptrs = (my_cnt_t *) alloc_dram(
		sizeof(my_cnt_t) * (rlrb->dram_csr_b->num_row+1),
		LOCAL_NUMA_NODE_IDX-1
	);
	rlrb->dram_csr_b->cols = (my_cnt_t *) alloc_dram(
		sizeof(my_cnt_t) * rlrb->dram_csr_b->num_nnz,
		LOCAL_NUMA_NODE_IDX-1
	);
	rlrb->dram_csr_b->rats = (my_rat_t *) alloc_dram(
		sizeof(my_rat_t) * rlrb->dram_csr_b->num_nnz,
		LOCAL_NUMA_NODE_IDX-1
	);
	rlrb->dram_csr_b->row_ptrs[0] = 0;

	rlrb->cxl_csr_b->row_ptrs = (my_cnt_t *) alloc_dram(
		sizeof(my_cnt_t) * (rlrb->cxl_csr_b->num_row+1),
		CXL_NUMA_NODE_IDX-1
	);
	rlrb->cxl_csr_b->cols = (my_cnt_t *) alloc_dram(
		sizeof(my_cnt_t) * rlrb->cxl_csr_b->num_nnz,
		CXL_NUMA_NODE_IDX-1
	);
	rlrb->cxl_csr_b->rats = (my_rat_t *) alloc_dram(
		sizeof(my_rat_t) * rlrb->cxl_csr_b->num_nnz,
		CXL_NUMA_NODE_IDX-1
	);
	rlrb->cxl_csr_b->row_ptrs[0] = 0;

	// printf("dram_cnt: %ld-%.6f\tcxl_cnt: %ld-%.6f\n", 
	// 	rlrb->dram_csr_b->num_nnz, (double) rlrb->dram_csr_b->num_nnz/csr_b->num_nnz,
	// 	rlrb->cxl_csr_b->num_nnz, (double) rlrb->cxl_csr_b->num_nnz/csr_b->num_nnz
	// );

	for (my_cnt_t b_row_idx = 0; b_row_idx < csr_b->num_row; b_row_idx ++) {
		if ( get_status(status_arr, b_row_idx) == 1) {
			rlrb->dram_csr_b->row_ptrs[rlrb->rid_old2new[b_row_idx]+1] = \
				csr_b->row_ptrs[b_row_idx+1] - csr_b->row_ptrs[b_row_idx] \
				+ rlrb->dram_csr_b->row_ptrs[rlrb->rid_old2new[b_row_idx]];

		} else if ( get_status(status_arr, b_row_idx) == 2) {
			rlrb->cxl_csr_b->row_ptrs[rlrb->rid_old2new[b_row_idx]+1] = \
				csr_b->row_ptrs[b_row_idx+1] - csr_b->row_ptrs[b_row_idx] \
				+ rlrb->cxl_csr_b->row_ptrs[rlrb->rid_old2new[b_row_idx]];
		}
	}


	/**
	for (my_cnt_t b_row_idx = 0; b_row_idx < csr_b->num_row; b_row_idx ++) {
		if ( get_status(status_arr, b_row_idx) == 1) {
			memcpy(
				rlrb->dram_csr_b->cols + rlrb->dram_csr_b->row_ptrs[rlrb->rid_old2new[b_row_idx]],
				csr_b->cols + csr_b->row_ptrs[b_row_idx],
				sizeof(my_cnt_t) * (csr_b->row_ptrs[b_row_idx+1] - csr_b->row_ptrs[b_row_idx])
			);
			memcpy(
				rlrb->dram_csr_b->rats + rlrb->dram_csr_b->row_ptrs[rlrb->rid_old2new[b_row_idx]],
				csr_b->rats + csr_b->row_ptrs[b_row_idx],
				sizeof(my_rat_t) * (csr_b->row_ptrs[b_row_idx+1] - csr_b->row_ptrs[b_row_idx])
			);

		} else if ( get_status(status_arr, b_row_idx) == 2) {
			memcpy(
				rlrb->cxl_csr_b->cols + rlrb->cxl_csr_b->row_ptrs[rlrb->rid_old2new[b_row_idx]],
				csr_b->cols + csr_b->row_ptrs[b_row_idx],
				sizeof(my_cnt_t) * (csr_b->row_ptrs[b_row_idx+1] - csr_b->row_ptrs[b_row_idx])
			);
			memcpy(
				rlrb->cxl_csr_b->rats + rlrb->cxl_csr_b->row_ptrs[rlrb->rid_old2new[b_row_idx]],
				csr_b->rats + csr_b->row_ptrs[b_row_idx],
				sizeof(my_rat_t) * (csr_b->row_ptrs[b_row_idx+1] - csr_b->row_ptrs[b_row_idx])
			);
		}
	}
	*/

	#pragma omp parallel num_threads(THREAD_NUM)
	{
		// --- Bind each thread to a physical core (Linux example) ---
	#ifdef __linux__
		int tid = omp_get_thread_num();
		cpu_set_t cpuset;
		CPU_ZERO(&cpuset);
		// Here we assume that the OpenMP thread number maps directly to a physical core.
		CPU_SET(get_cpu_id(tid), &cpuset);
		if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
			perror("pthread_setaffinity_np");
			// Optionally handle the error or exit.
		}
	#endif	/* __linux__ */

		// --- Process the rows in parallel ---
		#pragma omp for schedule(static)
		for (my_cnt_t b_row_idx = 0; b_row_idx < csr_b->num_row; b_row_idx++) {
			// Get the status for this row.
			int status = get_status(status_arr, b_row_idx);
			// Compute the number of elements in this row.
			size_t num_elements = csr_b->row_ptrs[b_row_idx+1] - csr_b->row_ptrs[b_row_idx];

			if (status == 1) {
				// --- DRAM branch ---
				// Compute the destination offset using the mapping.
				my_cnt_t new_row_idx = rlrb->rid_old2new[b_row_idx];
				size_t dest_offset = rlrb->dram_csr_b->row_ptrs[new_row_idx];

				// Copy the column indices.
				__builtin_memcpy(
					rlrb->dram_csr_b->cols + dest_offset,
					csr_b->cols + csr_b->row_ptrs[b_row_idx],
					sizeof(my_cnt_t) * num_elements
				);
				// Copy the associated "rats" (or values).
				__builtin_memcpy(
					rlrb->dram_csr_b->rats + dest_offset,
					csr_b->rats + csr_b->row_ptrs[b_row_idx],
					sizeof(my_rat_t) * num_elements
				);
			}
			else if (status == 2) {
				// --- CXL branch ---
				my_cnt_t new_row_idx = rlrb->rid_old2new[b_row_idx];
				size_t dest_offset = rlrb->cxl_csr_b->row_ptrs[new_row_idx];

				__builtin_memcpy(
					rlrb->cxl_csr_b->cols + dest_offset,
					csr_b->cols + csr_b->row_ptrs[b_row_idx],
					sizeof(my_cnt_t) * num_elements
				);
				__builtin_memcpy(
					rlrb->cxl_csr_b->rats + dest_offset,
					csr_b->rats + csr_b->row_ptrs[b_row_idx],
					sizeof(my_rat_t) * num_elements
				);
			}
			// If the status is neither 1 nor 2, nothing is done.
		} // end for
	} // end parallel region



#ifdef IN_VERIFY
	for (my_cnt_t b_row_idx = 0; b_row_idx < csr_b->num_row; b_row_idx ++) {
		assert ( get_status(status_arr, b_row_idx) != 3 );
	}
	assert(rlrb->dram_csr_b->row_ptrs[rlrb->dram_csr_b->num_row] == rlrb->dram_csr_b->num_nnz);
	assert(rlrb->cxl_csr_b->row_ptrs[rlrb->cxl_csr_b->num_row] == rlrb->cxl_csr_b->num_nnz);
#endif /* IN_VERIFY */
}



static void *pthread_hash_symbolic_rlrb(void *param) {
#ifdef IN_DEBUG
	struct timespec time_start, time_end;
	clock_gettime(CLOCK_REALTIME, &time_start);
#endif /* IN_DEBUG */

	hash_symbolic_arg_t *arg = (hash_symbolic_arg_t *) param;
	int _tid = arg->_tid;
	csr_t *csr_a = arg->csr_a;
	// csr_t *csr_b = arg->csr_b;
	bin_t *bin = arg->bin;

	status_arr_t *status_arr = arg->status_arr;
	rlrb_t *rlrb = arg->rlrb;

	csr_t* csr_b_array[3] = {rlrb->org_csr_b, rlrb->dram_csr_b, rlrb->cxl_csr_b};

	my_cnt_t start_row = bin->rows_offset[_tid];
	my_cnt_t end_row = bin->rows_offset[_tid+1];

	my_cnt_t *check = bin->local_hash_table_id[_tid];

	for (my_cnt_t idx = start_row; idx < end_row; idx++) {
		my_cnt_t nz = 0;
		my_cnt_t bid = bin->bin_id[idx];

		if (bid > 0) {
			my_cnt_t ht_size = MIN_HT_S << (bid - 1); 	// determine hash table size for i-th row
			memset(check, -1, sizeof(my_cnt_t)*ht_size);

			for (my_cnt_t jdx = csr_a->row_ptrs[idx]; jdx < csr_a->row_ptrs[idx+1]; jdx++) {
				my_cnt_t t_acol = csr_a->cols[jdx];
				csr_t *csr_b = csr_b_array[get_status(status_arr, t_acol)];
				my_cnt_t remapped_b_row_idx = rlrb->rid_old2new[t_acol];

				for (my_cnt_t kdx = csr_b->row_ptrs[remapped_b_row_idx]; kdx < csr_b->row_ptrs[remapped_b_row_idx+1]; kdx++) {

					my_cnt_t key = csr_b->cols[kdx];
					my_cnt_t hash = (key * HASH_CONSTANT) & (ht_size - 1);

					while (true) { // Loop for hash probing
						if (check[hash] == key) { // if the key is already inserted, it's ok
							break;
						} else if (check[hash] == -1) { // if the key has not been inserted yet, then it's added.
							check[hash] = key;
							nz++;
							break;
						} else { // linear probing: check next entry
							hash = (hash + 1) & (ht_size - 1); //hash = (hash + 1) % ht_size
						}
					}
				}


			}
		}

#ifdef IN_VERIFY
		if (bin->row_nz[idx] > nz) {
			(arg->over_predict) ++;
		}

		if (bin->row_nz[idx] < nz) {
			(arg->under_predict) ++;
		}
#endif /* IN_VERIFY */
		bin->row_nz[idx] = nz;

	}

#ifdef IN_DEBUG
	clock_gettime(CLOCK_REALTIME, &time_end);
	arg->_symb_time = diff_sec(time_start, time_end);
#endif /* IN_DEBUG */
	return NULL;
}


void hash_symbolic_rlrb(csr_t *csr_a, csr_t *csr_b, bin_t *bin,
	status_arr_t *status_arr, rlrb_t *rlrb) {
	pthread_t threadpool[THREAD_NUM];
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	cpu_set_t set;
	int core_idx;
	int rv;

	hash_symbolic_arg_t args[THREAD_NUM];
	memset(args, 0, sizeof(hash_symbolic_arg_t)*THREAD_NUM);

	for (int idx = 0; idx < THREAD_NUM; idx ++) {
		core_idx = get_cpu_id(idx);
		CPU_ZERO(&set);
		CPU_SET(core_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		args[idx]._tid = idx;
		args[idx].csr_a = csr_a;
		args[idx].csr_b = csr_b;
		args[idx].bin = bin;
		args[idx].status_arr = status_arr;
		args[idx].rlrb = rlrb;
		rv = pthread_create(&threadpool[idx], &attr, pthread_hash_symbolic_rlrb, args+idx);
		if (rv){
			printf("ERROR; return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

#ifdef IN_VERIFY
	over_predict = 0;
	under_predict = 0;
#endif /* IN_VERIFY */

	for (int idx = 0; idx < THREAD_NUM; idx ++) {
		rv = pthread_join(threadpool[idx], NULL);

#ifdef IN_DEBUG
		printf("\t%d_symb_time: %.9f\n", idx, args[idx]._symb_time);
		if (idx == THREAD_NUM-1) {
			printf("\n");
		}
#endif /* IN_DEBUG */

#ifdef IN_VERIFY
		over_predict += args[idx].over_predict;
		under_predict += args[idx].under_predict;
#endif /* IN_VERIFY */
		if (rv) {
			printf("ERROR; return code from pthread_create() is %d\n", rv);
			exit(EXIT_FAILURE);
		}
	}

	pthread_attr_destroy(&attr);


#ifdef IN_VERIFY
	printf("VERIFYPREDICT:\t%ld\t%ld\n", over_predict, under_predict);
#endif /* IN_VERIFY */
}












