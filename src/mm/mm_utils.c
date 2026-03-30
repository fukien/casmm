#include "mm_utils.h"

#ifdef IN_VERIFY
int64_t over_predict = 0;
int64_t under_predict = 0;
#endif /* IN_VERIFY */

#ifdef IN_DEBUG
pthread_barrier_t debug_barrier;
int debug_rv = 0;
#endif /* IN_DEBUG */


// Initialize status array for 'num_statuses' entries
status_arr_t *init_status_array(size_t num_statuses) {
	status_arr_t *arr = malloc(sizeof(status_arr_t));
	if (!arr) return NULL;

	arr->num_elements = (num_statuses + 31) / 32;  // Round up to store all statuses
	arr->bitmask = calloc(arr->num_elements, sizeof(uint64_t));  // Zero-initialize

	if (!arr->bitmask) {
		free(arr);
		return NULL;
	}
	return arr;
}

// Set the status of a specific index
void set_status(status_arr_t *arr, size_t index, uint8_t status) {
	size_t element_idx = index / 32;   // Find which uint64_t element
	size_t bit_pos = (index % 32) * STATUS_BITS;  // Find bit position within element

	arr->bitmask[element_idx] &= ~(STATUS_MASK << bit_pos); // Clear old bits
	arr->bitmask[element_idx] |= ((uint64_t)status << bit_pos); // Set new bits
}

// Get the status of a specific index
uint8_t get_status(status_arr_t *arr, size_t index) {
	size_t element_idx = index / 32;
	size_t bit_pos = (index % 32) * STATUS_BITS;

	return (arr->bitmask[element_idx] >> bit_pos) & STATUS_MASK;
}

// Clear (reset to STATUS_0) the status of a specific index
void clear_status(status_arr_t *arr, size_t index) {
	size_t element_idx = index / 32;
	size_t bit_pos = (index % 32) * STATUS_BITS;

	arr->bitmask[element_idx] &= ~(STATUS_MASK << bit_pos); // Clear to 00 (STATUS_0)
}

// Free the allocated memory
void free_status_array(status_arr_t *arr) {
	if (arr) {
		free(arr->bitmask);
		free(arr);
	}
}

// Debug function to print status array
void print_statuses(status_arr_t *arr, size_t num_statuses) {
	for (size_t i = 0; i < num_statuses; i++) {
		printf("%d ", get_status(arr, i));
		if ((i + 1) % 16 == 0) printf("\n");  // Print in rows
	}
	printf("\n");
}


void free_csr(csr_t *csr) {
	dealloc_memory( csr->rats, sizeof(my_rat_t) * csr->num_nnz );
	dealloc_memory( csr->cols, sizeof(my_cnt_t) * csr->num_nnz );
	dealloc_memory( csr->row_ptrs, sizeof(my_cnt_t) * (csr->num_row+1) );
	csr->num_row = 0;
	csr->num_col = 0;
	csr->num_nnz = 0;
}


void free_csc(csc_t *csc) {
	dealloc_memory( csc->rats, sizeof(my_rat_t) * csc->num_nnz );
	dealloc_memory( csc->rows, sizeof(my_cnt_t) * csc->num_nnz );
	dealloc_memory( csc->col_ptrs, sizeof(my_cnt_t) * (csc->num_col+1) );
	csc->num_row = 0;
	csc->num_col = 0;
	csc->num_nnz = 0;
}

void swap(triple_t *a, triple_t *b) {
	triple_t t = *a;
	*a = *b;
	*b = t;
}

int compare_mycnts(const void *a, const void *b) {
	my_cnt_t *cnt_a = (my_cnt_t *) a;
	my_cnt_t *cnt_b = (my_cnt_t *) b;
	return *cnt_a - *cnt_b;
}

int compare_triples(const void *a, const void *b) {
	triple_t *trpl_a = (triple_t *) a;
	triple_t *trpl_b = (triple_t *) b;
	if (trpl_a->user_id != trpl_b->user_id) {
		return (trpl_a->user_id - trpl_b->user_id);
	} else {
		return (trpl_a->item_id - trpl_b->item_id);
	}
}

int compare_keyvals_1(const void *a, const void *b) {	// key first, then val
	keyval_t *kv_a = (keyval_t *) a;
	keyval_t *kv_b = (keyval_t *) b;

	int key_cmp = (kv_a->key > kv_b->key) - (kv_a->key < kv_b->key);
	int val_cmp = (kv_a->val > kv_b->val) - (kv_a->val < kv_b->val);

	return key_cmp ? key_cmp : val_cmp;
}

int compare_keyvals_2(const void *a, const void *b) {	// key only
	keyval_t *keyval_a = (keyval_t *) a;
	keyval_t *keyval_b = (keyval_t *) b;
	return keyval_a->key - keyval_b->key;
}

int compare_keyvals_3(const void *a, const void *b) {	// val first (ascending), then key
	keyval_t *kv_a = (keyval_t *) a;
	keyval_t *kv_b = (keyval_t *) b;

	int val_cmp = (kv_a->val > kv_b->val) - (kv_a->val < kv_b->val);
	int key_cmp = (kv_a->key > kv_b->key) - (kv_a->key < kv_b->key);

	return val_cmp ? val_cmp : key_cmp;
}


int compare_keyvals_4(const void *a, const void *b) {	// val first (descening), then key
	keyval_t *kv_a = (keyval_t *) a;
	keyval_t *kv_b = (keyval_t *) b;

	int val_cmp = (kv_b->val > kv_a->val) - (kv_b->val < kv_a->val);
	int key_cmp = (kv_a->key > kv_b->key) - (kv_a->key < kv_b->key);

	return val_cmp ? val_cmp : key_cmp;
}


void check_sorted_triples(triple_t *trpl, int64_t num_nnz) {
	for (int64_t idx = 0; idx < num_nnz -1; idx ++) {
		if (compare_triples(trpl+idx, trpl+idx+1) > 0) {
			printf(
				"check false: trpl[%ld]{%ld, %ld}, trpl[%ld]{%ld, %ld}\n",
				idx, trpl[idx].user_id, trpl[idx].item_id,
				idx+1, trpl[ idx+1].user_id, trpl[ idx+1].item_id
			);
			continue;
		}
	}
}


void check_sorted_keyvals(keyval_t *keyval, int64_t num_keyval, 
	int (*func_cmp)(const void*, const void*)) {
	for (int64_t idx = 0; idx < num_keyval -1; idx ++) {
		if (func_cmp(keyval+idx, keyval+idx+1) > 0) {
			printf(
				"check false: keyval[%ld]{%ld, %f}, keyval[%ld]{%ld, %f}\n",
				idx, keyval[idx].key, keyval[idx].val,
				idx+1, keyval[idx+1].key, keyval[idx+1].val
			);
			continue;
		}
	}
}


void qsort_my_cnt_t(my_cnt_t *array, my_cnt_t length) {
	qsort(array, length, sizeof(my_cnt_t), compare_mycnts);
}



#if THREAD_NUM > 1
static void* pthread_qsort(void *param) {
	pthread_qsort_arg_t *arg = (pthread_qsort_arg_t *) param;
	arg->res = (triple_t *) alloc_memory(
		sizeof(triple_t) * arg->count, arg->numa_node_idx 
	);

	memcpy(
		arg->res, arg->trpl+arg->start, 
		sizeof(triple_t) * arg->count
	);

	qsort(
		arg->res,
		arg->count,
		sizeof(triple_t), 
		compare_triples
	);

	// check_sorted_triples(arg->res, arg->count);

	return NULL;
}


static void pthread_merge(void *param) {
	pthread_merge_arg_t *arg = (pthread_merge_arg_t *) param;
	int64_t index[2] = {0, 0};

	for (int64_t idx = 0; idx < arg->count0 + arg->count1; idx ++) {
		short flag = 0;

		if (index[1] >= arg->count1) {
			flag = 0;
		} else if (index[0] >= arg->count0) {
			flag = 1;
		} else if (compare_triples(arg->source0 + index[0], arg->source1 + index[1])<=0) {
			flag = 0;
		} else {
			flag = 1;
		}

		if (!flag) {
			arg->res[idx] = arg->source0[index[0]];
			index[0] ++;
		} else {
			arg->res[idx] = arg->source1[index[1]];
			index[1] ++;
		}
	}

	dealloc_memory(arg->source0, sizeof(triple_t)*arg->count0);
	dealloc_memory(arg->source1, sizeof(triple_t)*arg->count1);

	// check_sorted_triples(arg->res, arg->count0+arg->count1);
	
	// return NULL;
}

void p_qsortmerge(triple_t *trpl, my_cnt_t num_nnz, int numa_node_idx) {
	if (num_nnz < 1 << 10) {
		qsort(trpl, num_nnz, sizeof(triple_t), compare_triples);
	} else {
		pthread_t threadpool[THREAD_NUM];
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		cpu_set_t set;
		int core_idx;
		int rv;

		pthread_qsort_arg_t qsort_args[THREAD_NUM];
		memset(qsort_args, 0, sizeof(pthread_qsort_arg_t)*THREAD_NUM);

		int64_t num_per_thr = num_nnz / THREAD_NUM;

		for (int idx = 0; idx < THREAD_NUM; idx++) {
			core_idx = get_cpu_id(idx);
			CPU_ZERO(&set);
			CPU_SET(core_idx, &set);
			pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
			qsort_args[idx]._tid = idx;
			qsort_args[idx].numa_node_idx = numa_node_idx;
			qsort_args[idx].start = idx * num_per_thr;
			qsort_args[idx].count = idx == THREAD_NUM -1 ? 
				(num_nnz - qsort_args[idx].start) : num_per_thr;
			qsort_args[idx].trpl = trpl;
			rv = pthread_create(&threadpool[idx], &attr, pthread_qsort, qsort_args+idx);
			if (rv){
				printf("ERROR; return code from pthread_create() is %d\n", rv);
				exit(EXIT_FAILURE);
			}
		}
		for (int idx = 0; idx < THREAD_NUM; idx++) {
			rv = pthread_join(threadpool[idx], NULL);
			if (rv){
				printf("ERROR; return code from pthread_join() is %d\n", rv);
				exit(EXIT_FAILURE);
			}
		}

		merge_part_t merge_parts[THREAD_NUM];
		memset(merge_parts, 0, sizeof(merge_part_t)*THREAD_NUM);
		for (int idx = 0; idx < THREAD_NUM; idx++) {
			merge_parts[idx].count = qsort_args[idx].count;
			merge_parts[idx].res = qsort_args[idx].res;
		}

		int merge_num = THREAD_NUM;
		// assert(merge_num >=2);
		while (merge_num > 2){
			pthread_merge_arg_t merge_args[merge_num/2];
			memset(merge_args, 0, sizeof(pthread_merge_arg_t)*(merge_num/2));
			
			for (int idx = 0; idx < merge_num/2; idx ++) {
				core_idx = get_cpu_id(idx);
				CPU_ZERO(&set);
				CPU_SET(core_idx, &set);
				pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
				merge_args[idx]._tid = idx;
				merge_args[idx].count0 = merge_parts[2 * idx].count;
				merge_args[idx].count1 = merge_parts[2 * idx + 1].count;
				merge_args[idx].source0 = merge_parts[2 * idx].res;
				merge_args[idx].source1 = merge_parts[2 * idx + 1].res;
				merge_args[idx].res = alloc_memory(
					sizeof(triple_t)*(merge_args[idx].count0+merge_args[idx].count1),
					numa_node_idx
				);
				rv = pthread_create(&threadpool[idx], &attr, (void *(*)(void *)) (uintptr_t) pthread_merge, merge_args+idx);
				if (rv){
					printf("ERROR; return code from pthread_create() is %d\n", rv);
					exit(EXIT_FAILURE);
				}
			}

			for (int idx = 0; idx <  merge_num/2; idx++) {
				rv = pthread_join(threadpool[idx], NULL);
				if (rv){
					printf("ERROR; return code from pthread_join() is %d\n", rv);
					exit(EXIT_FAILURE);
				}
				merge_parts[idx].count = merge_args[idx].count0 + merge_args[idx].count1;
				merge_parts[idx].res = merge_args[idx].res;
			}

			if (merge_num % 2 == 1){
				merge_parts[ (merge_num/2) ] = merge_parts[merge_num - 1];
				merge_num = merge_num / 2 + 1;
			} else{
				merge_num /= 2;
			}
		}

		pthread_merge_arg_t last_merge_arg = {
			._tid = 0,
			.count0 = merge_parts[0].count,
			.count1 = merge_parts[1].count,
			.source0 = merge_parts[0].res,
			.source1 = merge_parts[1].res,
			.res = trpl
		};

		// assert(merge_parts[0].count + merge_parts[1].count == num_nnz);
		pthread_merge(&last_merge_arg);

		// dealloc_memory(last_merge_arg.source0, sizeof(triple_t)*last_merge_arg.count0);
		// dealloc_memory(last_merge_arg.source1, sizeof(triple_t)*last_merge_arg.count1);

		pthread_attr_destroy(&attr);

		// check_sorted_triples(trpl, num_nnz);
	}
}


static void* pthread_qsort_keyval(void *param) {
	pthread_qsort_keyval_arg_t *arg = (pthread_qsort_keyval_arg_t *) param;
	arg->res = (keyval_t *) alloc2_memory(
		sizeof(keyval_t) * arg->count, arg->numa_cfg 
	);

	memcpy(
		arg->res, arg->keyval+arg->start, 
		sizeof(keyval_t) * arg->count
	);

	qsort(
		arg->res,
		arg->count,
		sizeof(keyval_t), 
		arg->func_cmp
	);

	// check_sorted_keyvals(arg->res, arg->count, arg->func_cmp);

	return NULL;
}


static void pthread_merge_keyval(void *param) {
	pthread_merge_keyval_arg_t *arg = (pthread_merge_keyval_arg_t *) param;
	int64_t index[2] = {0, 0};

	for (int64_t idx = 0; idx < arg->count0 + arg->count1; idx ++) {
		short flag = 0;

		if (index[1] >= arg->count1) {
			flag = 0;
		} else if (index[0] >= arg->count0) {
			flag = 1;
		} else if (arg->func_cmp(arg->source0 + index[0], arg->source1 + index[1])<=0) {
			flag = 0;
		} else {
			flag = 1;
		}

		if (!flag) {
			arg->res[idx] = arg->source0[index[0]];
			index[0] ++;
		} else {
			arg->res[idx] = arg->source1[index[1]];
			index[1] ++;
		}
	}

	dealloc_memory(arg->source0, sizeof(keyval_t)*arg->count0);
	dealloc_memory(arg->source1, sizeof(keyval_t)*arg->count1);

	// check_sorted_keyvals(arg->res, arg->count0+arg->count1, arg->func_cmp);

	// return NULL;
}


void p_qsortmerge_keyval(keyval_t *keyval, my_cnt_t num_keyval,
	int (*func_cmp)(const void*, const void*), numa_cfg_t *numa_cfg) {
	if (num_keyval < 1 << 10) {
		qsort(keyval, num_keyval, sizeof(keyval_t), func_cmp);
	} else {
		pthread_t threadpool[THREAD_NUM];
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		cpu_set_t set;
		int core_idx;
		int rv;

		pthread_qsort_keyval_arg_t qsort_args[THREAD_NUM];
		memset(qsort_args, 0, sizeof(pthread_qsort_keyval_arg_t)*THREAD_NUM);

		int64_t num_per_thr = num_keyval / THREAD_NUM;

		for (int idx = 0; idx < THREAD_NUM; idx ++) {
			core_idx = get_cpu_id(idx);
			CPU_ZERO(&set);
			CPU_SET(core_idx, &set);
			pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
			qsort_args[idx]._tid = idx;
			qsort_args[idx].numa_cfg = numa_cfg;
			qsort_args[idx].start = idx * num_per_thr;
			qsort_args[idx].count = idx == THREAD_NUM -1 ? 
				(num_keyval - qsort_args[idx].start) : num_per_thr;
			qsort_args[idx].keyval = keyval;
			qsort_args[idx].func_cmp = func_cmp;
			rv = pthread_create(&threadpool[idx], &attr, pthread_qsort_keyval, qsort_args+idx);
			if (rv) {
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


		merge_part_keyval_t merge_parts[THREAD_NUM];
		memset(merge_parts, 0, sizeof(merge_part_keyval_t)*(THREAD_NUM));

		for (int idx = 0; idx < THREAD_NUM; idx ++) {
			merge_parts[idx].count = qsort_args[idx].count;
			merge_parts[idx].res = qsort_args[idx].res;
		}

		int merge_num = THREAD_NUM;
		while (merge_num > 2) {
			pthread_merge_keyval_arg_t merge_args[merge_num/2];
			memset(merge_args, 0, sizeof(pthread_merge_keyval_arg_t)*(merge_num/2));

			for (int idx = 0; idx < merge_num/2; idx ++) {
				core_idx = get_cpu_id(idx);
				CPU_ZERO(&set);
				CPU_SET(core_idx, &set);
				pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
				merge_args[idx]._tid = idx;
				merge_args[idx].count0 = merge_parts[2 * idx].count;
				merge_args[idx].count1 = merge_parts[2 * idx + 1].count;
				merge_args[idx].source0 = merge_parts[2 * idx].res;
				merge_args[idx].source1 = merge_parts[2 * idx + 1].res;
				merge_args[idx].res = alloc2_memory(
					sizeof(keyval_t)*(merge_args[idx].count0+merge_args[idx].count1),
					numa_cfg
				);
				merge_args[idx].func_cmp = func_cmp;
				rv = pthread_create(&threadpool[idx], &attr, (void *(*)(void *)) (uintptr_t) pthread_merge_keyval, merge_args+idx);
				if (rv) {
					printf("ERROR; return code from pthread_create() is %d\n", rv);
					exit(EXIT_FAILURE);
				}
			}

			for (int idx = 0; idx <  merge_num /2; idx++) {
				rv = pthread_join(threadpool[idx], NULL);
				if (rv){
					printf("ERROR; return code from pthread_join() is %d\n", rv);
					exit(EXIT_FAILURE);
				}
				merge_parts[idx].count = merge_args[idx].count0 + merge_args[idx].count1;
				merge_parts[idx].res = merge_args[idx].res;
			}

			if (merge_num % 2 == 1){
				merge_parts[ (merge_num/2) ] = merge_parts[merge_num - 1];
				merge_num = merge_num / 2 + 1;
			} else{
				merge_num /= 2;
			}
		}

		pthread_merge_keyval_arg_t last_merge_arg = {
			._tid = 0,
			.count0 = merge_parts[0].count,
			.count1 = merge_parts[1].count,
			.source0 = merge_parts[0].res,
			.source1 = merge_parts[1].res,
			.res = keyval,
			.func_cmp = func_cmp
		};

		// assert(merge_parts[0].count + merge_parts[1].count == num_keyval);

		// dealloc_memory(last_merge_arg.source0, sizeof(keyval_t)*last_merge_arg.count0);
		// dealloc_memory(last_merge_arg.source1, sizeof(keyval_t)*last_merge_arg.count1);

		pthread_merge_keyval(&last_merge_arg);
		pthread_attr_destroy(&attr);

		// check_sorted_keyvals(keyval, num_keyval, func_cmp);

	}
}
#endif /* THREAD_NUM > 1 */


bool mtx2trpl(triple_t **trpl, my_cnt_t* num_row, my_cnt_t* num_col,
	my_cnt_t* num_nnz, const char *filename, const int numa_node_idx) {
	bool isSymmetric = false;
	my_cnt_t tmp_num_nnz = 0, tmp_user_id = 0, tmp_item_id = 0;
	my_rat_t tmp_rating = 0.0, tmp_rating_double = 0.0;

	triple_t* tmp_trpl = NULL;

	// Open file using open system call with O_DIRECT
	int fd = open(filename, O_RDONLY | O_DIRECT);
	if (fd == -1) {
		fprintf(stderr, "Error opening file with O_DIRECT: %s\n", filename);
		exit(EXIT_FAILURE);
	}

	// Get file size and alignment
	struct stat sb;
	if (fstat(fd, &sb) == -1) {
		perror("Error getting file size");
		close(fd);
		exit(EXIT_FAILURE);
	}
	size_t alignment = sb.st_blksize; // Typically 4096 bytes
	size_t buffer_size = (sb.st_size + alignment - 1) & ~(alignment - 1);
	void *aligned_buffer;
	if (posix_memalign(&aligned_buffer, alignment, buffer_size)) {
		perror("Error allocating aligned memory");
		close(fd);
		exit(EXIT_FAILURE);
	}

	ssize_t bytes_read = 0;
	char *current_ptr = (char *)aligned_buffer;
	bool header_read = false;
	my_cnt_t cnt = 0;

	char leftover[4096];  // Temporary buffer for an incomplete line
	size_t leftover_len = 0;

	while ((bytes_read = read(fd, aligned_buffer, buffer_size)) > 0) {

		current_ptr = (char *)aligned_buffer;

		while (current_ptr < (char *)aligned_buffer + bytes_read) {
			char *line = current_ptr;
			// Find the end of the current line
			char *newline = memchr(line, '\n', (char *)aligned_buffer + bytes_read - current_ptr);
			if (!newline) {

				leftover_len = (char *)aligned_buffer + bytes_read - line;
				memset(leftover, 0, 4096);
				memcpy(leftover, line, leftover_len);
				break;  // Partial line, handle on the next read
			}

			*newline = '\0';  // Null-terminate the line
			current_ptr = newline + 1;

			// Skip comment lines starting with '%'
			if (line[0] == '%') {
				if (strstr(line, "symmetric")) {
					isSymmetric = true;
				}
				continue;
			}

			// Parse the header line
			if (!header_read) {
				if (sscanf(line, "%ld %ld %ld", num_row, num_col, &tmp_num_nnz) != 3) {
					fprintf(stderr, "Error reading header line: %s\n", line);
					free(aligned_buffer);
					close(fd);
					exit(EXIT_FAILURE);
				}
				header_read = true;

				if (isSymmetric) {
					tmp_num_nnz = tmp_num_nnz * 2;
				}

				tmp_trpl = (triple_t *)alloc_dram(sizeof(triple_t) * tmp_num_nnz, numa_node_idx);
				memset(tmp_trpl, 0, sizeof(triple_t) * tmp_num_nnz);
			} else {
				// Parse non-header lines
				int read_columns = sscanf(line, "%ld %ld %lf",
						&tmp_user_id, &tmp_item_id, &tmp_rating_double);

				// try to deal with leftover
				if (leftover_len > 0) { 
					memcpy(leftover+leftover_len, line, current_ptr-line);
					read_columns = sscanf(leftover, "%ld %ld %lf",
						&tmp_user_id, &tmp_item_id, &tmp_rating_double);
					leftover_len = 0;
					memset(leftover, 0, 4096);
				}

				if (read_columns == 2) {
					tmp_rating_double = 1.0;
				} else if (read_columns != 3) {
					fprintf(stderr, "Error reading line: %s\n", line);
					free(aligned_buffer);
					close(fd);
					exit(EXIT_FAILURE);
				}


				if (tmp_rating_double == 0.0) {
					continue;
				}

				tmp_rating = 1.0;
				tmp_trpl[cnt].user_id = tmp_user_id - 1;
				tmp_trpl[cnt].item_id = tmp_item_id - 1;
				tmp_trpl[cnt].rating = tmp_rating;
				cnt++;

				if (isSymmetric) {
					if (tmp_trpl[cnt - 1].user_id != tmp_trpl[cnt - 1].item_id) {
						tmp_trpl[cnt].user_id = tmp_trpl[cnt - 1].item_id;
						tmp_trpl[cnt].item_id = tmp_trpl[cnt - 1].user_id;
						tmp_trpl[cnt].rating = tmp_trpl[cnt - 1].rating;
						cnt++;
					}
				}
			}
		}
	}

	if (bytes_read == -1) {
		perror("Error reading file with O_DIRECT");
		free(aligned_buffer);
		close(fd);
		exit(EXIT_FAILURE);
	}

	free(aligned_buffer);
	close(fd);

	*trpl = (triple_t *) alloc_memory(
		sizeof(triple_t)*cnt, numa_node_idx
	);

	memset(*trpl, 0, sizeof(triple_t)*cnt);

	*num_nnz = cnt;

	memcpy(*trpl, tmp_trpl, sizeof(triple_t)*cnt);
	dealloc_memory(tmp_trpl, sizeof(triple_t)*tmp_num_nnz);

#if THREAD_NUM > 1
	p_qsortmerge(*trpl, *num_nnz, numa_node_idx);
#else	/* THREAD_NUM > 1 */
	qsort(*trpl, *num_nnz, sizeof(triple_t), compare_triples);
#endif	/* THREAD_NUM > 1 */

	return isSymmetric;
}


void trpl2mtx(const char *filename, triple_t *trpl, my_cnt_t num_row, 
	my_cnt_t num_col, my_cnt_t num_nnz, bool isSymmetric) {
	FILE *file = fopen(filename, "w");
	if (!file) {
		perror("Error opening file");
		exit(EXIT_FAILURE);
	}

	// Write the MTX header
	if (isSymmetric) {
		fprintf(file, "%%MatrixMarket matrix coordinate real symmetric\n");
		my_cnt_t actual_cnt = 0;
		for (my_cnt_t idx = 0; idx < num_nnz; idx++) {
			if (trpl[idx].user_id <= trpl[idx].item_id) {
				actual_cnt ++;
			}
		}
		fprintf(file, "%ld %ld %ld\n", num_row, num_col, actual_cnt);
		for (my_cnt_t idx = 0; idx < num_nnz; idx++) {
			if (trpl[idx].user_id <= trpl[idx].item_id) {
				fprintf(file, "%ld %ld %f\n",
					trpl[idx].user_id + 1, trpl[idx].item_id + 1, 1.0
				);
			}
		}
	} else {
		fprintf(file, "%%MatrixMarket matrix coordinate real general\n");
		fprintf(file, "%ld %ld %ld\n", num_row, num_col, num_nnz);
		for (my_cnt_t idx = 0; idx < num_nnz; idx++) {
			fprintf(file, "%ld %ld %f\n",
				trpl[idx].user_id + 1, trpl[idx].item_id + 1, 1.0
			);
		}

	}

	fclose(file);
}


void trpl2csr(csr_t *csr, triple_t *trpl, const my_cnt_t num_row, 
	const my_cnt_t num_col, const my_cnt_t num_nnz, numa_cfg_t* numa_cfg) {
	memset(csr, 0, sizeof(csr_t));
	csr->num_row = num_row;
	csr->num_col = num_col;
	csr->num_nnz = num_nnz;

	if (IS_POWER_OF_TWO(*(numa_cfg->numa_mask->maskp))) {
		csr->row_ptrs = (my_cnt_t *) alloc_dram(
			sizeof(my_cnt_t) * (num_row+1), numa_cfg->node_idx
		);
		csr->cols = (my_cnt_t *) alloc_dram(
			sizeof(my_cnt_t) * num_nnz, numa_cfg->node_idx
		);
		csr->rats = (my_rat_t *) alloc_dram( 
			sizeof(my_rat_t) * num_nnz, numa_cfg->node_idx
		);
	} else {
		csr->row_ptrs = (my_cnt_t *) alloc2_memory(
			sizeof(my_cnt_t) * (num_row+1), numa_cfg
		);
		csr->cols = (my_cnt_t *) alloc2_memory(
			sizeof(my_cnt_t) * num_nnz, numa_cfg
		);
		csr->rats = (my_rat_t *) alloc2_memory( 
			sizeof(my_rat_t) * num_nnz, numa_cfg
		);
	}

	memset(csr->row_ptrs, 0, sizeof(my_cnt_t)*(num_row+1));
	memset(csr->cols, 0, sizeof(my_cnt_t)*num_nnz);
	memset(csr->rats, 0, sizeof(my_rat_t)*num_nnz);

	for (my_cnt_t idx = 0; idx < num_nnz; idx ++) {
		csr->row_ptrs[ trpl[idx].user_id+1 ] ++;
		csr->cols[idx] = trpl[idx].item_id;
		csr->rats[idx] = trpl[idx].rating;
	}

	for (my_cnt_t idx = 1; idx <= num_row; idx ++ ) {
		csr->row_ptrs[idx] += csr->row_ptrs[idx-1];
	}
}


void csr2trpl(triple_t **trpl, my_cnt_t* num_row, my_cnt_t* num_col, 
	my_cnt_t* num_nnz, csr_t *csr, const int numa_node_idx) {
	*num_row = csr->num_row;
	*num_col = csr->num_col;
	*num_nnz = csr->num_nnz;
	*trpl = (triple_t *) alloc_memory(
		sizeof(triple_t) * (*num_nnz), numa_node_idx
	);
	memset(*trpl, 0, sizeof(triple_t) * (*num_nnz));

	my_cnt_t kdx = 0;
	for (my_cnt_t idx = 0; idx < *num_row; idx ++) {
		for (my_cnt_t jdx = csr->row_ptrs[idx]; jdx < csr->row_ptrs[idx+1]; jdx ++) {
			(*trpl)[kdx].user_id = idx;
			(*trpl)[kdx].item_id = csr->cols[jdx];
			(*trpl)[kdx].rating = csr->rats[jdx];
		}
	}

	// qsort(*trpl, *num_nnz, sizeof(triple_t), compare_triples);
}


void mtx2csr(csr_t *csr, const char *filename, numa_cfg_t* numa_cfg) {
	triple_t* trpl = NULL;
	my_cnt_t num_row, num_col, num_nnz;
	mtx2trpl(&trpl, &num_row, &num_col, &num_nnz, filename, numa_cfg->node_idx);
	trpl2csr(csr, trpl, num_row, num_col, num_nnz, numa_cfg);
	dealloc_memory(trpl, sizeof(triple_t) * num_nnz);
}


void bin2trpl(triple_t** trpl, my_cnt_t* num_row, my_cnt_t* num_col, 
	my_cnt_t* num_nnz, const char *filename, const int numa_node_idx) {
	int fd;
	struct stat sb;
	fd = open(filename, O_DIRECT | O_RDONLY);

	if (fd == -1) {
		perror("Error opening file");
		exit(1);
	}
	// Get the file size
	if (fstat(fd, &sb) == -1) {
		perror("Error getting file size");
		close(fd);
		exit(1);
	}

	size_t alignment = sb.st_blksize;
	size_t aligned_size = (sb.st_size + alignment - 1) & ~(alignment - 1);

	// Allocate aligned memory for O_DIRECT
	void *aligned_buffer;
	if (posix_memalign(&aligned_buffer, alignment, aligned_size)) {
		perror("Error allocating aligned memory");
		close(fd);
		exit(1);
	}

	// Read file content with pread
	ssize_t bytes_read = pread(fd, aligned_buffer, aligned_size, 0);
	if (bytes_read == -1) {
		perror("Error reading file with O_DIRECT");
		free(aligned_buffer);
		close(fd);
		exit(1);
	}

	triple_t *data_mem = (triple_t *) aligned_buffer;

	// get header
	*num_row = data_mem[0].user_id;
	*num_col = data_mem[0].item_id;
	*num_nnz = (my_cnt_t) data_mem[0].rating;

	*trpl = (triple_t *) alloc_memory(
		sizeof(triple_t)*(*num_nnz), numa_node_idx
	);
	memset(*trpl, 0, sizeof(triple_t)*(*num_nnz));
	memcpy(*trpl, data_mem+1, sizeof(triple_t)*(*num_nnz));

	// we skip the sorting here since we've already sorted datasets (groupby/ssj) in the preprocessing

	free(aligned_buffer);
	close(fd);
}


void bin2csr(csr_t *csr, const char *filename, numa_cfg_t* numa_cfg) {
	triple_t* trpl = NULL;
	my_cnt_t num_row, num_col, num_nnz;
	bin2trpl(&trpl, &num_row, &num_col, &num_nnz, filename, LOCAL_NUMA_NODE_IDX - 1);
	trpl2csr(csr, trpl, num_row, num_col, num_nnz, numa_cfg);
	dealloc_memory(trpl, sizeof(triple_t) * num_nnz);
}


void csr2mtx(const char *filename, csr_t *csr) {
	FILE *file = fopen(filename, "w");
	if (!file) {
		perror("Error opening file");
		exit(EXIT_FAILURE);
	}

	// Write the MTX header
	fprintf(file, "%%MatrixMarket matrix coordinate real general\n");
	fprintf(file, "%ld %ld %ld\n", csr->num_row, csr->num_col, csr->num_nnz);

	for (my_cnt_t idx = 0; idx < csr->num_row; idx ++) {
		for (my_cnt_t jdx = csr->row_ptrs[idx]; jdx < csr->row_ptrs[idx+1]; jdx ++) {
			fprintf(file, "%ld %ld %f\n", 
				idx+1, csr->cols[jdx]+1, csr->rats[jdx]
			);
		}
	}

	fclose(file);
}


void raw2csr(csr_t *csr, const char *filename, numa_cfg_t* numa_cfg) {
#if defined(IN_GROUPBY) || defined(IN_SSJ)
	bin2csr(csr, filename, numa_cfg);
#else	// defined(IN_GROUPBY) || defined(IN_SSJ)
	mtx2csr(csr, filename, numa_cfg);
#endif	// defined(IN_GROUPBY) || defined(IN_SSJ)
}


void trpl_ones(triple_t* trpl, my_cnt_t num_nnz) {
	for (my_cnt_t idx = 0; idx < num_nnz; idx ++) {
		trpl[idx].rating = 1.0;
	}
}


void trpl_transpose(triple_t* trpl, my_cnt_t *num_row, 
	my_cnt_t *num_col, my_cnt_t num_nnz) {
	for (my_cnt_t idx = 0; idx < num_nnz; idx ++) {
		my_cnt_t tmp = trpl[idx].user_id;
		trpl[idx].user_id = trpl[idx].item_id;
		trpl[idx].item_id = tmp;
	}
	qsort(trpl, num_nnz, sizeof(triple_t), compare_triples);
	my_cnt_t tmp = *num_row;
	*num_row = *num_col;
	*num_col = tmp;
}


void mtx_transpose(
	const char *filename_dest_trs, const char *filename_dest_org,
	const char *filename_src, const int numa_node_idx) {
	triple_t* trpl = NULL;
	my_cnt_t num_row, num_col, num_nnz;
	bool isSymmetric = mtx2trpl(&trpl, &num_row, &num_col, &num_nnz, filename_src, numa_node_idx);
	trpl_ones(trpl, num_nnz);
	trpl2mtx(filename_dest_org, trpl, num_row, num_col, num_nnz, isSymmetric);
	trpl_transpose(trpl, &num_row, &num_col, num_nnz);
	trpl2mtx(filename_dest_trs, trpl, num_row, num_col, num_nnz, isSymmetric);
	dealloc_triv_dram(trpl, sizeof(triple_t) * num_nnz);
}


// Tailored specifically for PaRMAT-synthesized datasets
void txt2trpl(triple_t **trpl, my_cnt_t* num_row, my_cnt_t* num_col,
	my_cnt_t* num_nnz, const char *filename, const int numa_node_idx) {

	int scale = 0, edge_factor = 0;
	// Scan for the pattern 'er_' followed by two numbers separated by '_'
	if (sscanf(filename, RMAT_DIR"/%*[^_]_%d_%d.txt", &scale, &edge_factor) == 2) {
		printf("Extracted numbers: %d and %d\n", scale, edge_factor);
	} else {
		printf("Failed to extract numbers.\n");
	}
	*num_row = *num_col = (1 << scale);

	my_cnt_t tmp_user_id = 0, tmp_item_id = 0,\
		tmp_num_nnz = count_lines_num(filename) * 2;
	assert ( tmp_num_nnz == *num_row * edge_factor * 2 );

	my_rat_t tmp_rating = 0.0, tmp_rating_double = 0.0;

	triple_t* tmp_trpl = (triple_t*) alloc_dram(
		sizeof(triple_t) * tmp_num_nnz, numa_node_idx
	);
	memset(tmp_trpl, 0, sizeof(triple_t)*tmp_num_nnz);


	FILE* file = fopen(filename, "r");
	if (file == NULL) {
		fprintf(stderr, "Error opening file, %s\n", filename);
		exit(EXIT_FAILURE);
	}

	char line[CHAR_BUFFER_LEN];

	my_cnt_t cnt = 0;

	while (fgets(line, sizeof(line), file) != NULL) {
		if (cnt < tmp_num_nnz) {
			int read_columns = sscanf(line, "%ld %ld %lf", 
				&tmp_user_id, &tmp_item_id, &tmp_rating_double
			);

			if (read_columns == 2) {
				tmp_rating_double = 1.0;
			} else if (read_columns != 3) {
				fprintf(stderr, "Error reading line: %s\n", line);
				exit(EXIT_FAILURE);
			}

			if (tmp_rating_double == 0.0) {
				continue;
			}

			// tmp_rating = (float) tmp_rating_double;
			tmp_rating = 1.0;

			// Indexing starts from 0, so no need to subtract or adjust
			tmp_trpl[cnt].user_id = tmp_user_id;
			tmp_trpl[cnt].item_id = tmp_item_id;
			tmp_trpl[cnt].rating = tmp_rating;

			cnt++;


			assert (tmp_trpl[cnt-1].user_id != tmp_trpl[cnt-1].item_id);
			tmp_trpl[cnt].user_id = tmp_trpl[cnt-1].item_id;
			tmp_trpl[cnt].item_id = tmp_trpl[cnt-1].user_id;
			tmp_trpl[cnt].rating = tmp_trpl[cnt-1].rating;
			cnt ++;

		}
	}
	fclose(file);

	*trpl = (triple_t *) alloc_memory(
		sizeof(triple_t)*cnt, numa_node_idx
	);

	memset(*trpl, 0, sizeof(triple_t)*cnt);

	*num_nnz = cnt;

	memcpy(*trpl, tmp_trpl, sizeof(triple_t)*cnt);
	dealloc_memory(tmp_trpl, sizeof(triple_t)*tmp_num_nnz);

#if THREAD_NUM > 1
	p_qsortmerge(*trpl, *num_nnz, numa_node_idx);
#else	/* THREAD_NUM > 1 */
	qsort(*trpl, *num_nnz, sizeof(triple_t), compare_triples);
#endif	/* THREAD_NUM > 1 */
}


// Tailored specifically for PaRMAT-synthesized datasets
void txt_transpose(
	const char *filename_dest_trs, const char *filename_dest_org,
	const char *filename_src, const int numa_node_idx) {
	triple_t* trpl = NULL;
	my_cnt_t num_row, num_col, num_nnz;
	txt2trpl(&trpl, &num_row, &num_col, &num_nnz, filename_src, numa_node_idx);
	trpl_ones(trpl, num_nnz);
	trpl2mtx(filename_dest_org, trpl, num_row, num_col, num_nnz, true);
	trpl_transpose(trpl, &num_row, &num_col, num_nnz);
	trpl2mtx(filename_dest_trs, trpl, num_row, num_col, num_nnz, true);
	dealloc_triv_dram(trpl, sizeof(triple_t) * num_nnz);
}


void print_binary(unsigned char c) {
	for (int i = 7; i >= 0; i--) {
		putchar((c & (1 << i)) ? '1' : '0');
	}
}


// TODO: Parallelize this function
my_cnt_t get_flop(csr_t *csr_a, csr_t *csr_b) {
	my_cnt_t flops = 0; // total flops (multiplication) needed to generate C
	my_cnt_t tflops=0; //thread private flops

	for (my_cnt_t i=0; i < csr_a->num_row; ++i) {	   // for all rows of A
		my_cnt_t locmax = 0;
		for (my_cnt_t j=csr_a->row_ptrs[i]; j < csr_a->row_ptrs[i + 1]; ++j) { // For all the nonzeros of the ith column
			my_cnt_t inner = csr_a->cols[j]; // get the row id of B (or column id of A)
			my_cnt_t npins = csr_b->row_ptrs[inner + 1] - csr_b->row_ptrs[inner]; // get the number of nonzeros in A's corresponding column
			locmax += npins;
		}
		tflops += locmax;
	}
	flops += tflops;
	return (flops * 2);
}


static void *pthread_scan(void *param) {
	pthread_scan_arg_t *arg = (pthread_scan_arg_t *) param;
	int _tid = arg->_tid;
	my_cnt_t num = arg->num;
	my_cnt_t *in = arg->in;
	my_cnt_t *out = arg->out;
	my_cnt_t *partial_sum = arg->partial_sum;

	my_cnt_t each_n = num / THREAD_NUM;
	my_cnt_t start_idx = _tid * each_n;
	my_cnt_t end_idx = (_tid == THREAD_NUM-1) ? num : start_idx + each_n;

	out[start_idx] = 0;
	for (my_cnt_t idx = start_idx; idx < end_idx - 1; idx++) {
		out[idx+1] = out[idx]+in[idx];
	}

	if (_tid < THREAD_NUM-1) {
		partial_sum[_tid] = out[end_idx-1] + in[end_idx-1];
	}

	int rv;
	BARRIER_ARRIVE(arg->barrier, rv);

	my_cnt_t offset = 0;
	for (int idx = 0; idx < _tid; ++idx) {
		offset += partial_sum[idx];
	}

	for (my_cnt_t idx = start_idx; idx < end_idx; ++ idx) {
		out[idx] += offset;
	}

	return NULL;
}


void scan(my_cnt_t *out, my_cnt_t *in, my_cnt_t num) {
	if ( num < (1 << 17) ) {							// single-thread execution
		out[0] = 0;
		for (my_cnt_t idx = 0; idx < num-1; idx ++) {
			out[idx+1] = out[idx] + in[idx];
		}
	} else {											// multi-thread execution
		pthread_t threadpool[THREAD_NUM];
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		cpu_set_t set;
		int core_idx;
		int rv;

		pthread_scan_arg_t args[THREAD_NUM];
		memset(args, 0, sizeof(pthread_scan_arg_t)*THREAD_NUM);
		
		pthread_barrier_t barrier;
		rv = pthread_barrier_init(&barrier, NULL, THREAD_NUM);

		my_cnt_t partial_sum[THREAD_NUM];

		for (int idx = 0; idx < THREAD_NUM; idx ++) {
			core_idx = get_cpu_id(idx);
			CPU_ZERO(&set);
			CPU_SET(core_idx, &set);
			pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);

			args[idx]._tid = idx;
			args[idx].num = num;
			args[idx].in = in;
			args[idx].out = out;
			args[idx].partial_sum = partial_sum;
			args[idx].barrier = &barrier;

			rv = pthread_create(&threadpool[idx], &attr, pthread_scan, args+idx);
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

		pthread_barrier_destroy(&barrier);
		pthread_attr_destroy(&attr);
	}
}



static void *pthread_scan_rat(void *param) {
	pthread_scan_rat_arg_t *arg = (pthread_scan_rat_arg_t *) param;
	int _tid = arg->_tid;
	my_cnt_t num = arg->num;
	my_rat_t *in = arg->in;
	my_rat_t *out = arg->out;
	my_rat_t *partial_sum = arg->partial_sum;

	my_cnt_t each_n = num / THREAD_NUM;
	my_cnt_t start_idx = _tid * each_n;
	my_cnt_t end_idx = (_tid == THREAD_NUM - 1) ? num : start_idx + each_n;

	// Step 1: Compute local prefix sum
	out[start_idx] = 0;
	for (my_cnt_t idx = start_idx; idx < end_idx - 1; idx++) {
		out[idx + 1] = out[idx] + in[idx];
	}

	// Step 2: Store last value in partial_sum (excluding last thread)
	if (_tid < THREAD_NUM - 1) {
		partial_sum[_tid] = out[end_idx - 1] + in[end_idx - 1];
	}

	// Step 3: Synchronize all threads before calculating offsets
	int rv;
	BARRIER_ARRIVE(arg->barrier, rv);

	// Step 4: Compute offset based on previous partial sums
	my_rat_t offset = 0.0;
	for (int idx = 0; idx < _tid; ++idx) {
		offset += partial_sum[idx];
	}

	// Step 5: Apply offset to all local values
	for (my_cnt_t idx = start_idx; idx < end_idx; ++idx) {
		out[idx] += offset;
	}

	return NULL;
}

void scan_rat(my_rat_t *out, my_rat_t *in, my_cnt_t num) {
	if (num < (1 << 17)) {  // Small size: single-threaded execution
		out[0] = 0;
		for (my_cnt_t idx = 0; idx < num - 1; idx++) {
			out[idx + 1] = out[idx] + in[idx];
		}
	} else {  // Large size: multi-threaded execution
		pthread_t threadpool[THREAD_NUM];
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		cpu_set_t set;
		int core_idx;
		int rv;

		pthread_scan_rat_arg_t args[THREAD_NUM];
		memset(args, 0, sizeof(pthread_scan_rat_arg_t) * THREAD_NUM);

		pthread_barrier_t barrier;
		rv = pthread_barrier_init(&barrier, NULL, THREAD_NUM);

		my_rat_t partial_sum[THREAD_NUM];

		for (int idx = 0; idx < THREAD_NUM; idx++) {
			core_idx = get_cpu_id(idx);
			CPU_ZERO(&set);
			CPU_SET(core_idx, &set);
			pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);

			args[idx]._tid = idx;
			args[idx].num = num;
			args[idx].in = in;
			args[idx].out = out;
			args[idx].partial_sum = partial_sum;
			args[idx].barrier = &barrier;

			rv = pthread_create(&threadpool[idx], &attr, pthread_scan_rat, args + idx);
			if (rv) {
				printf("ERROR: pthread_create() returned %d\n", rv);
				exit(EXIT_FAILURE);
			}
		}

		for (int idx = 0; idx < THREAD_NUM; idx++) {
			rv = pthread_join(threadpool[idx], NULL);
			if (rv) {
				printf("ERROR: pthread_join() returned %d\n", rv);
				exit(EXIT_FAILURE);
			}
		}

		pthread_barrier_destroy(&barrier);
		pthread_attr_destroy(&attr);
	}
}



static void *pthread_scan_keyval(void *param) {
	pthread_scan_keyval_arg_t *arg = (pthread_scan_keyval_arg_t *) param;
	int _tid = arg->_tid;
	my_cnt_t num = arg->num;
	keyval_t *in = arg->in;
	my_rat_t *out = arg->out;
	my_rat_t *partial_sum = arg->partial_sum;

	my_cnt_t each_n = num / THREAD_NUM;
	my_cnt_t start_idx = _tid * each_n;
	my_cnt_t end_idx = (_tid == THREAD_NUM-1) ? num : start_idx + each_n;

	out[start_idx] = 0;
	for (my_cnt_t idx = start_idx; idx < end_idx - 1; idx++) {
		out[idx+1] = out[idx]+in[idx].val;
	}

	if (_tid < THREAD_NUM - 1) {
		partial_sum[_tid] = out[end_idx-1] + in[end_idx-1].val;
	}

	int rv;
	BARRIER_ARRIVE(arg->barrier, rv);

	my_rat_t offset = 0;
	for (int idx = 0; idx < _tid; ++idx) {
		offset += partial_sum[idx];
	}

	for (my_cnt_t idx = start_idx; idx < end_idx; ++ idx) {
		out[idx] += offset;
	}

	return NULL;
}


void scan_keyval(my_rat_t *out, keyval_t *in, my_cnt_t num) {
	if ( num < (1 << 17) ) {							// single-thread execution
		out[0] = 0;
		for (my_cnt_t idx = 0; idx < num-1; idx ++) {
			out[idx+1] = out[idx] + in[idx].val;
		}
	} else {
		pthread_t threadpool[THREAD_NUM];
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		cpu_set_t set;
		int core_idx;
		int rv;

		pthread_scan_keyval_arg_t args[THREAD_NUM];
		memset(args, 0, sizeof(pthread_scan_keyval_arg_t)*THREAD_NUM);
		
		pthread_barrier_t barrier;
		rv = pthread_barrier_init(&barrier, NULL, THREAD_NUM);

		my_rat_t partial_sum[THREAD_NUM];

		for (int idx = 0; idx < THREAD_NUM; idx ++) {
			core_idx = get_cpu_id(idx);
			CPU_ZERO(&set);
			CPU_SET(core_idx, &set);
			pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);

			args[idx]._tid = idx;
			args[idx].num = num;
			args[idx].in = in;
			args[idx].out = out;
			args[idx].partial_sum = partial_sum;
			args[idx].barrier = &barrier;

			rv = pthread_create(&threadpool[idx], &attr, pthread_scan_keyval, args+idx);
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

		pthread_barrier_destroy(&barrier);
		pthread_attr_destroy(&attr);
	}	
}


my_cnt_t* lower_bound(my_cnt_t *start, my_cnt_t *end, my_cnt_t val) {
	my_cnt_t* mid;
	while (start < end) {
		mid = start + (end - start) / 2;
		if (*mid < val) {
			start = mid + 1;
		} else {
			end = mid;
		}
	}
	return start;
}


my_rat_t func_mul(my_rat_t a, my_rat_t b) {
	return a * b;
}


my_rat_t func_add(my_rat_t a, my_rat_t b) {
	return a + b;
}



static void* pthread_get_intprod(void *param) {
	pthread_intprod_arg_t *arg = (pthread_intprod_arg_t *)param;
	int _tid = arg->_tid;
	my_cnt_t *row_nz = arg->row_nz;
	csr_t *csr_a = arg->csr_a;
	csr_t *csr_b = arg->csr_b;

	my_cnt_t intprod = 0;
	my_cnt_t nz_per_row = 0;

	my_cnt_t start_idx = _tid * (csr_a->num_row/THREAD_NUM);
	my_cnt_t end_idx = (_tid == THREAD_NUM-1) ? csr_a->num_row : 
		(_tid+1) * (csr_a->num_row/THREAD_NUM);

	for (my_cnt_t idx = start_idx; idx < end_idx; idx ++) {
		nz_per_row = 0;
		for (my_cnt_t jdx = csr_a->row_ptrs[idx]; jdx < csr_a->row_ptrs[idx+1]; jdx ++) {
			nz_per_row += csr_b->row_ptrs[ csr_a->cols[jdx]+1 ] - csr_b->row_ptrs[ csr_a->cols[jdx] ];
		}
		row_nz[idx] = nz_per_row;
		intprod += nz_per_row;
	}

	arg->intprod = intprod;
	return NULL;
}

my_cnt_t get_intprod(csr_t* csr_a, csr_t* csr_b, my_cnt_t *row_nz) {
	my_cnt_t total_intprod = 0;

	pthread_t threadpool[THREAD_NUM];
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	cpu_set_t set;
	int core_idx;
	int rv;
	pthread_intprod_arg_t args[THREAD_NUM];
	memset(args, 0, sizeof(pthread_intprod_arg_t)*THREAD_NUM);

	for (int idx = 0; idx < THREAD_NUM; idx ++) {
		core_idx = get_cpu_id(idx);
		CPU_ZERO(&set);
		CPU_SET(core_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		args[idx]._tid = idx;
		args[idx].intprod = 0;
		args[idx].row_nz = row_nz;
		args[idx].csr_a = csr_a;
		args[idx].csr_b = csr_b;
		rv = pthread_create(&threadpool[idx], &attr, pthread_get_intprod, args+idx);
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
		// printf("\t%d_intprod: %ld\n", idx, args[idx].intprod);
		// if (idx == THREAD_NUM-1) {
		// 	printf("\n");
		// }
#endif /* IN_DEBUG */
		total_intprod += args[idx].intprod;
	}

	pthread_attr_destroy(&attr);

	return total_intprod;
}


static void* pthread_cf_populate(void *param) {
	pthread_intprod_arg_t *arg = (pthread_intprod_arg_t *)param;
	int _tid = arg->_tid;
	csr_t *csr_a = arg->csr_a;
	csr_t *csr_b = arg->csr_b;

	my_cnt_t *row_flp = arg->row_flp;
	my_rat_t *row_cf = arg->row_cf;
	my_cnt_t *row_cnt = arg->row_cnt;

	my_cnt_t flp_per_row = 0;

	my_cnt_t start_idx = _tid * (csr_a->num_row/THREAD_NUM);
	my_cnt_t end_idx = (_tid == THREAD_NUM-1) ? csr_a->num_row : 
		(_tid+1) * (csr_a->num_row/THREAD_NUM);

	for (my_cnt_t idx = start_idx; idx < end_idx; idx ++) {
		flp_per_row = 0;
		for (my_cnt_t jdx = csr_a->row_ptrs[idx]; jdx < csr_a->row_ptrs[idx+1]; jdx ++) {
			flp_per_row += csr_b->row_ptrs[ csr_a->cols[jdx]+1 ] - csr_b->row_ptrs[ csr_a->cols[jdx] ];
		}
		row_flp[idx] = flp_per_row;
		row_cf[idx] = (my_rat_t) row_flp[idx]/(row_cnt[idx+1]-row_cnt[idx]);
	}

	return NULL;
}


void cf_populate(csr_t* csr_a, csr_t* csr_b, 
	my_cnt_t *row_flp, 
	my_rat_t *row_cf, 
	my_cnt_t *row_cnt) {

	pthread_t threadpool[THREAD_NUM];
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	cpu_set_t set;
	int core_idx;
	int rv;
	pthread_intprod_arg_t args[THREAD_NUM];
	memset(args, 0, sizeof(pthread_intprod_arg_t)*THREAD_NUM);

	for (int idx = 0; idx < THREAD_NUM; idx ++) {
		core_idx = get_cpu_id(idx);
		CPU_ZERO(&set);
		CPU_SET(core_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		args[idx]._tid = idx;
		args[idx].csr_a = csr_a;
		args[idx].csr_b = csr_b;

		args[idx].row_flp = row_flp;
		args[idx].row_cf = row_cf;
		args[idx].row_cnt = row_cnt;
		rv = pthread_create(&threadpool[idx], &attr, pthread_cf_populate, args+idx);
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



static void* pthread_flp_populate(void *param) {
	pthread_intprod_arg_t *arg = (pthread_intprod_arg_t *)param;
	int _tid = arg->_tid;
	csr_t *csr_a = arg->csr_a;
	csr_t *csr_b = arg->csr_b;

	my_cnt_t *row_flp = arg->row_flp;

	my_cnt_t flp_per_row = 0;

	my_cnt_t start_idx = _tid * (csr_a->num_row/THREAD_NUM);
	my_cnt_t end_idx = (_tid == THREAD_NUM-1) ? csr_a->num_row : 
		(_tid+1) * (csr_a->num_row/THREAD_NUM);

	for (my_cnt_t idx = start_idx; idx < end_idx; idx ++) {
		flp_per_row = 0;
		for (my_cnt_t jdx = csr_a->row_ptrs[idx]; jdx < csr_a->row_ptrs[idx+1]; jdx ++) {
			flp_per_row += csr_b->row_ptrs[ csr_a->cols[jdx]+1 ] - csr_b->row_ptrs[ csr_a->cols[jdx] ];
		}
		row_flp[idx] = flp_per_row;
	}

	return NULL;
}


void flp_populate(csr_t* csr_a, csr_t* csr_b, my_cnt_t *row_flp) {
	pthread_t threadpool[THREAD_NUM];
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	cpu_set_t set;
	int core_idx;
	int rv;
	pthread_intprod_arg_t args[THREAD_NUM];
	memset(args, 0, sizeof(pthread_intprod_arg_t)*THREAD_NUM);

	for (int idx = 0; idx < THREAD_NUM; idx ++) {
		core_idx = get_cpu_id(idx);
		CPU_ZERO(&set);
		CPU_SET(core_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		args[idx]._tid = idx;
		args[idx].csr_a = csr_a;
		args[idx].csr_b = csr_b;

		args[idx].row_flp = row_flp;
		rv = pthread_create(&threadpool[idx], &attr, pthread_flp_populate, args+idx);
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



static void* pthread_set_rows_offset(void *param) {
	pthread_set_rows_offset_arg_t* arg = (pthread_set_rows_offset_arg_t*) param;
	int _tid = arg->_tid;
	my_cnt_t num_row = arg->num_row;
	my_cnt_t average_intprod = arg->average_intprod;
	my_cnt_t* rows_offset = arg->rows_offset;
	my_cnt_t* row_start_idcs = arg->row_start_idcs;

	rows_offset[_tid+1] = lower_bound(
		row_start_idcs, 
		row_start_idcs + num_row + 1, 
		average_intprod * (_tid + 1)
	) - row_start_idcs;

	return NULL;
}


void set_rows_offset(my_cnt_t num_row, my_cnt_t total_intprod, 
	my_cnt_t* rows_offset, my_cnt_t* row_start_idcs) {
	pthread_t threadpool[THREAD_NUM];
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	cpu_set_t set;
	int core_idx;
	int rv;
	pthread_set_rows_offset_arg_t args[THREAD_NUM];
	memset(args, 0, sizeof(pthread_set_rows_offset_arg_t)*THREAD_NUM);

	my_cnt_t average_intprod = (total_intprod + THREAD_NUM - 1) / THREAD_NUM;

	for (int idx = 0; idx < THREAD_NUM; idx ++) {
		core_idx = get_cpu_id(idx);
		CPU_ZERO(&set);
		CPU_SET(core_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		args[idx]._tid = idx;
		args[idx].num_row = num_row;
		args[idx].average_intprod = average_intprod;
		args[idx].rows_offset = rows_offset;
		args[idx].row_start_idcs = row_start_idcs;
		rv = pthread_create(&threadpool[idx], &attr, pthread_set_rows_offset, args+idx);
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
		// my_cnt_t flop_estimated = row_start_idcs[rows_offset[idx+1]]
			// - row_start_idcs[rows_offset[idx]];
		if (idx == THREAD_NUM -1 ) {
			rows_offset[THREAD_NUM] = num_row;
			// flop_estimated = row_start_idcs[rows_offset[idx+1]]
				// - row_start_idcs[rows_offset[idx]];
		}
		// printf("\t%d_flop_estimated: %ld\n", idx, flop_estimated);
		// if (idx == THREAD_NUM-1) {
		// 	printf("\n");
		// }
#endif /* IN_DEBUG */
	}

	rows_offset[THREAD_NUM] = num_row;

	pthread_attr_destroy(&attr);
}



my_rat_t* lower_bound_cfo(my_rat_t *start, my_rat_t *end, my_rat_t val) {
	my_rat_t* mid;
	while (start < end) {
		mid = start + (end - start) / 2;
		if (*mid < val) {
			start = mid + 1;
		} else {
			end = mid;
		}
	}
	return start;
}


static void* pthread_rebalance_rows_offset(void *param) {
	pthread_rebalance_rows_offset_arg_t* arg = (pthread_rebalance_rows_offset_arg_t*) param;
	int _tid = arg->_tid;
	my_cnt_t num_row = arg->num_row;
	my_rat_t average_cfo = arg->average_cfo;
	my_cnt_t* rows_offset = arg->rows_offset;
	my_rat_t* accum_cfo = arg->accum_cfo;

	rows_offset[_tid+1] = lower_bound_cfo(
		accum_cfo, 
		accum_cfo + num_row + 1, 
		average_cfo * (_tid + 1)
	) - accum_cfo;

	return NULL;
}



void rebalance_rows_offset(my_cnt_t num_row, my_rat_t cfo_sum, 
	my_cnt_t* rows_offset, my_rat_t* accum_cfo) {
	pthread_t threadpool[THREAD_NUM];
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	cpu_set_t set;
	int core_idx;
	int rv;
	pthread_rebalance_rows_offset_arg_t args[THREAD_NUM];
	memset(args, 0, sizeof(pthread_rebalance_rows_offset_arg_t)*THREAD_NUM);

	my_rat_t average_cfo = (cfo_sum + THREAD_NUM - 1) / THREAD_NUM;

	for (int idx = 0; idx < THREAD_NUM; idx ++) {
		core_idx = get_cpu_id(idx);
		CPU_ZERO(&set);
		CPU_SET(core_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		args[idx]._tid = idx;
		args[idx].num_row = num_row;
		args[idx].average_cfo = average_cfo;
		args[idx].rows_offset = rows_offset;
		args[idx].accum_cfo = accum_cfo;
		rv = pthread_create(&threadpool[idx], &attr, pthread_rebalance_rows_offset, args+idx);
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
		my_rat_t cfo_estimated = accum_cfo[rows_offset[idx+1]] - accum_cfo[rows_offset[idx]];
		if (idx == THREAD_NUM -1 ) {
			rows_offset[THREAD_NUM] = num_row;
			cfo_estimated = accum_cfo[rows_offset[idx+1]] - accum_cfo[rows_offset[idx]];
		}
		printf("\t%d_cfo_estimated: %.2f\taverage_cfo: %.2f\t"
			"accum_cfo[%ld]: %.2f\taccum_cfo[%ld]: %.2f\n", idx, 
			cfo_estimated, average_cfo, 
			rows_offset[idx], accum_cfo[rows_offset[idx]], 
			rows_offset[idx+1], accum_cfo[rows_offset[idx+1]]
		);
		if (idx == THREAD_NUM-1) {
			printf("\n");
		}
#endif /* IN_DEBUG */
	}

	rows_offset[THREAD_NUM] = num_row;

	pthread_attr_destroy(&attr);
}



keyval_t* lower_bound_keyval(keyval_t *start, keyval_t *end, my_rat_t val) {
	keyval_t* mid;
	while (start < end) {
		mid = start + (end - start) / 2;
		if (mid->val < val) {
			start = mid + 1;
		} else {
			end = mid;
		}
	}
	return start;
}


void free_csr_nz(csr_nz_t *csr_nz) {
	if (csr_nz->num_nnz > 0) {
		dealloc_memory( csr_nz->rats, sizeof(my_rat_t) * csr_nz->num_nnz );
		dealloc_memory( csr_nz->cols, sizeof(my_cnt_t) * csr_nz->num_nnz );		
	}
	if (csr_nz->num_row > 0) {
		dealloc_memory( csr_nz->row_ptrs, sizeof(my_cnt_t) * (csr_nz->num_row+1) );
		dealloc_memory( csr_nz->row_nz, sizeof(my_cnt_t) * csr_nz->num_row );
	}
	csr_nz->num_row = 0;
	csr_nz->num_col = 0;
	csr_nz->num_nnz = 0;
}


void write_non_temporal_batched(
	my_cnt_t *data_cols,
	my_rat_t *data_vals,
	my_cnt_t offset,
	my_cnt_t length,
	my_cnt_t *ht_check,
	my_rat_t *ht_value
) {
	my_cnt_t write_pos = offset;
	my_cnt_t index = 0;

	// Cacheline-sized temporary buffers for `cols` and `vals`
	my_cnt_t cols_buffer[NUM64_PER_CACHELINE] __attribute__((aligned(CACHELINE_SIZE)));
	my_rat_t vals_buffer[NUM64_PER_CACHELINE] __attribute__((aligned(CACHELINE_SIZE)));;

	uintptr_t start_address_cols = (uintptr_t)(data_cols + offset);  // Starting address for cols
	uintptr_t aligned_start_cols = (start_address_cols + CACHELINE_SIZE - 1) & ~(CACHELINE_SIZE - 1);  // Align to next cacheline

	my_cnt_t unaligned_prefix_entries = (aligned_start_cols - start_address_cols) / NUM64_T_SIZE;  // Entries in unaligned prefix

	// Handle unaligned prefix
	for (my_cnt_t i = 0; i < unaligned_prefix_entries && index < length; index++) {
		if (ht_check[index] != -1) {
			data_cols[write_pos] = ht_check[index];
			data_vals[write_pos] = ht_value[index];
			write_pos++;
		}
	}

	// Handle cacheline-sized aligned entries
	my_cnt_t buffer_pos = 0;
	for (; index < length; index++) {
		if (ht_check[index] != -1) {
			cols_buffer[buffer_pos] = ht_check[index];  // Write to temp buffer
			vals_buffer[buffer_pos] = ht_value[index];
			buffer_pos++;

			// If buffer is full (one cacheline), perform non-temporal write
			if (buffer_pos == NUM64_PER_CACHELINE) {
				WRITE_NT_64(&data_cols[write_pos], cols_buffer);
				WRITE_NT_64(&data_vals[write_pos], vals_buffer);
				write_pos += NUM64_PER_CACHELINE;
				buffer_pos = 0;  // Reset buffer
			}
		}
	}

	// Handle unaligned suffix (remaining entries in the buffer)
	for (my_cnt_t i = 0; i < buffer_pos; i++) {
		data_cols[write_pos] = cols_buffer[i];
		data_vals[write_pos] = vals_buffer[i];
		write_pos++;
	}
}


void write_non_temporal_batched_cols(
	my_cnt_t *data_cols,
	my_cnt_t offset,
	my_cnt_t length,
	my_cnt_t *ht_check
) {
	my_cnt_t write_pos = offset;
	my_cnt_t index = 0;

	// Cacheline-sized temporary buffers for `cols` and `vals`
	my_cnt_t cols_buffer[NUM64_PER_CACHELINE] __attribute__((aligned(CACHELINE_SIZE)));

	uintptr_t start_address_cols = (uintptr_t)(data_cols + offset);  // Starting address for cols
	uintptr_t aligned_start_cols = (start_address_cols + CACHELINE_SIZE - 1) & ~(CACHELINE_SIZE - 1);  // Align to next cacheline

	my_cnt_t unaligned_prefix_entries = (aligned_start_cols - start_address_cols) / NUM64_T_SIZE;  // Entries in unaligned prefix

	// Handle unaligned prefix
	for (my_cnt_t i = 0; i < unaligned_prefix_entries && index < length; index++) {
		if (ht_check[index] != -1) {
			data_cols[write_pos] = ht_check[index];
			write_pos++;
		}
	}

	// Handle cacheline-sized aligned entries
	my_cnt_t buffer_pos = 0;
	for (; index < length; index++) {
		if (ht_check[index] != -1) {
			cols_buffer[buffer_pos] = ht_check[index];  // Write to temp buffer
			buffer_pos++;

			// If buffer is full (one cacheline), perform non-temporal write
			if (buffer_pos == NUM64_PER_CACHELINE) {
				WRITE_NT_64(&data_cols[write_pos], cols_buffer);
				write_pos += NUM64_PER_CACHELINE;
				buffer_pos = 0;  // Reset buffer
			}
		}
	}

	// Handle unaligned suffix (remaining entries in the buffer)
	for (my_cnt_t i = 0; i < buffer_pos; i++) {
		data_cols[write_pos] = cols_buffer[i];
		write_pos++;
	}
}


void write_non_temporal_batched_vals(
	my_rat_t *data_vals,
	my_cnt_t offset,
	my_cnt_t length,
	my_cnt_t *ht_check,
	my_rat_t *ht_value
) {
	my_cnt_t write_pos = offset;
	my_cnt_t index = 0;

	// Cacheline-sized temporary buffers for `cols` and `vals`
	my_rat_t vals_buffer[NUM64_PER_CACHELINE] __attribute__((aligned(CACHELINE_SIZE)));;

	uintptr_t start_address_vals = (uintptr_t)(data_vals + offset);  // Starting address for cols
	uintptr_t aligned_start_vals = (start_address_vals + CACHELINE_SIZE - 1) & ~(CACHELINE_SIZE - 1);  // Align to next cacheline

	my_cnt_t unaligned_prefix_entries = (aligned_start_vals - start_address_vals) / NUM64_T_SIZE;  // Entries in unaligned prefix

	// Handle unaligned prefix
	for (my_cnt_t i = 0; i < unaligned_prefix_entries && index < length; index++) {
		if (ht_check[index] != -1) {
			data_vals[write_pos] = ht_value[index];
			write_pos++;
		}
	}

	// Handle cacheline-sized aligned entries
	my_cnt_t buffer_pos = 0;
	for (; index < length; index++) {
		if (ht_check[index] != -1) {
			vals_buffer[buffer_pos] = ht_value[index];
			buffer_pos++;

			// If buffer is full (one cacheline), perform non-temporal write
			if (buffer_pos == NUM64_PER_CACHELINE) {
				WRITE_NT_64(&data_vals[write_pos], vals_buffer);
				write_pos += NUM64_PER_CACHELINE;
				buffer_pos = 0;  // Reset buffer
			}
		}
	}

	// Handle unaligned suffix (remaining entries in the buffer)
	for (my_cnt_t i = 0; i < buffer_pos; i++) {
		data_vals[write_pos] = vals_buffer[i];
		write_pos++;
	}
}


static void* pthread_max_csr_row(void *param) {
	pthread_max_csr_row_arg_t *arg = (pthread_max_csr_row_arg_t *) param;
	int _tid = arg->_tid;
	csr_t *csr= arg->csr;
	my_cnt_t start_idx = _tid * csr->num_row / THREAD_NUM;
	my_cnt_t end_idx = (_tid == THREAD_NUM-1) ? csr->num_row : (1+_tid) * csr->num_row / THREAD_NUM;

	my_cnt_t max_row = 0;
	for (my_cnt_t row_idx = start_idx; row_idx < end_idx; row_idx ++) {
		my_cnt_t cur_row = csr->row_ptrs[row_idx+1] - csr->row_ptrs[row_idx];
		max_row = (max_row > cur_row) ? max_row : cur_row;
	}
	arg->max_row_array[_tid] = max_row;
	return NULL;
}

void max_csr_row(csr_t *csr, my_cnt_t *max_row_array) {
	pthread_t threadpool[THREAD_NUM];
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	cpu_set_t set;
	int core_idx;
	int rv;
	pthread_max_csr_row_arg_t args[THREAD_NUM];
	memset(args, 0, sizeof(pthread_max_csr_row_arg_t)*THREAD_NUM);

	for (int idx = 0; idx < THREAD_NUM; idx ++) {
		core_idx = get_cpu_id(idx);
		CPU_ZERO(&set);
		CPU_SET(core_idx, &set);
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
		args[idx]._tid = idx;
		args[idx].csr = csr;
		args[idx].max_row_array = max_row_array;
		rv = pthread_create(&threadpool[idx], &attr, pthread_max_csr_row, args+idx);
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


void radix_sort_keyval(keyval_t *arr, keyval_t *aux, size_t n) {
	if (n < 2) {
		return;
	}

	const size_t num_bytes = sizeof(my_cnt_t); // Number of bytes in the key
	for (size_t byte_index = 0; byte_index < num_bytes; byte_index++) {
		size_t count[256] = {0};

		// 1) Count occurrences of the byte_index-th byte in .key
		for (size_t i = 0; i < n; i++) {
			unsigned char *p = (unsigned char *)&arr[i].key;
			unsigned char digit = p[byte_index];
			count[digit]++;
		}

		// 2) Prefix sums
		for (int v = 1; v < 256; v++) {
			count[v] += count[v - 1];
		}

		// 3) Distribute elements into aux from right to left
		for (long i = (long)n - 1; i >= 0; i--) {
			unsigned char *p = (unsigned char *)&arr[i].key;
			unsigned char digit = p[byte_index];
			size_t pos = --count[digit];
			aux[pos] = arr[i];
		}

		// 4) Copy sorted pass back
		memcpy(arr, aux, n * sizeof(keyval_t));
	}
}


keyval_t* radix_sort_keyval_toggling(keyval_t *arr, keyval_t *aux, size_t n) {
	if (n < 2) {
		return arr;
	}

	// ------------------------------------------------------------------
	// 1) Quick check if all keys are the same. If so, no need to sort.
	// ------------------------------------------------------------------
	bool all_same = true;
	for (size_t i = 1; i < n; i++) {
		if (arr[i].key != arr[0].key) {
			all_same = false;
			break;
		}
	}
	if (all_same) {
		return arr; // all identical => already "sorted"
	}

	// ------------------------------------------------------------------
	// 2) Determine how many bytes we actually need to sort
	// ------------------------------------------------------------------
	my_cnt_t max_key = 0;
	for (size_t i = 0; i < n; i++) {
		if (arr[i].key > max_key) {
			max_key = arr[i].key;
		}
	}

	size_t needed_passes = 0;
	while (max_key > 0) {
		max_key >>= 8;  // shift by one byte
		needed_passes++;
	}
	if (needed_passes == 0) {
		needed_passes = 1; // at least 1 pass if all keys happen to be 0
	}
	if (needed_passes > sizeof(my_cnt_t)) {
		needed_passes = sizeof(my_cnt_t); // safety check (for 64-bit => max 8)
	}

	// ------------------------------------------------------------------
	// 3) LSD Radix Sort with toggling
	// ------------------------------------------------------------------
	keyval_t *in  = arr;
	keyval_t *out = aux;

	for (size_t byte_index = 0; byte_index < needed_passes; byte_index++) {
		// 3a) Build histogram (256 buckets)
		size_t count[256] = {0};

		// Single loop for counting
		for (size_t i = 0; i < n; i++) {
			unsigned char d = ((unsigned char *)&in[i].key)[byte_index];
			count[d]++;
		}

		// 3b) Prefix sums for stable positions
		for (int d = 1; d < 256; d++) {
			count[d] += count[d - 1];
		}

		// 3c) Distribute elements (stable) into 'out'
		//     going from right to left
		for (long j = (long)n - 1; j >= 0; j--) {
			unsigned char d = ((unsigned char *)&in[j].key)[byte_index];
			out[--count[d]] = in[j];
		}

		// 3d) Toggle buffers
		keyval_t *tmp = in;
		in  = out;
		out = tmp;
	}

	// Return final pointer (arr or aux)
	return in;
}


keyval_t* radix_sort_keyval_toggling_unrolling(keyval_t *arr, keyval_t *aux, size_t n) {
	if (n < 2) {
		return arr;
	}

	/* ------------------------------------------------------------------
	 * 1) Quick check if all keys are the same. If so, no need to sort.
	 * ----------------------------------------------------------------*/
	bool all_same = true;
	for (size_t i = 1; i < n; i++) {
		if (arr[i].key != arr[0].key) {
			all_same = false;
			break;
		}
	}
	if (all_same) {
		return arr; // already "sorted" since all keys identical
	}

	/* ------------------------------------------------------------------
	 * 2) Determine how many bytes we actually need to sort
	 *    by finding the maximum key in 'arr'.
	 * ----------------------------------------------------------------*/
	my_cnt_t max_key = 0;
	for (size_t i = 0; i < n; i++) {
		if (arr[i].key > max_key) {
			max_key = arr[i].key;
		}
	}

	// Count how many bytes are needed:
	size_t needed_passes = 0;
	while (max_key > 0) {
		max_key >>= 8; // shift by one byte
		needed_passes++;
	}
	if (needed_passes == 0) {
		needed_passes = 1; // at least 1 pass if all keys happen to be 0
	}
	// Safety check, in case something is off (e.g., 8-byte keys => max 8 passes)
	if (needed_passes > sizeof(my_cnt_t)) {
		needed_passes = sizeof(my_cnt_t);
	}

	/* ------------------------------------------------------------------
	 * 3) LSD Radix Sort with toggling buffers
	 * ----------------------------------------------------------------*/
	keyval_t *in  = arr;
	keyval_t *out = aux;

	for (size_t byte_index = 0; byte_index < needed_passes; byte_index++) {
		// 3a) Build histogram (256 buckets)
		size_t count[256] = {0};

		// Unrolled counting in chunks of 4
		size_t i = 0;
		for (; i + 3 < n; i += 4) {
			unsigned char d0 = ((unsigned char *)&in[i+0].key)[byte_index];
			unsigned char d1 = ((unsigned char *)&in[i+1].key)[byte_index];
			unsigned char d2 = ((unsigned char *)&in[i+2].key)[byte_index];
			unsigned char d3 = ((unsigned char *)&in[i+3].key)[byte_index];
			count[d0]++;
			count[d1]++;
			count[d2]++;
			count[d3]++;
		}
		for (; i < n; i++) {
			unsigned char d = ((unsigned char *)&in[i].key)[byte_index];
			count[d]++;
		}

		// 3b) Prefix sums for stable positions
		for (int d = 1; d < 256; d++) {
			count[d] += count[d - 1];
		}

		// 3c) Distribute elements (stable) into 'out'
		long j = (long)n - 1;
		for (; j - 3 >= 0; j -= 4) {
			unsigned char d0 = ((unsigned char *)&in[j  ].key)[byte_index];
			unsigned char d1 = ((unsigned char *)&in[j-1].key)[byte_index];
			unsigned char d2 = ((unsigned char *)&in[j-2].key)[byte_index];
			unsigned char d3 = ((unsigned char *)&in[j-3].key)[byte_index];
			out[ --count[d0] ] = in[j  ];
			out[ --count[d1] ] = in[j-1];
			out[ --count[d2] ] = in[j-2];
			out[ --count[d3] ] = in[j-3];
		}
		// handle leftover elements
		for (; j >= 0; j--) {
			unsigned char d = ((unsigned char *)&in[j].key)[byte_index];
			out[ --count[d] ] = in[j];
		}

		// 3d) Swap buffers (toggle)
		keyval_t *tmp = in;
		in  = out;
		out = tmp;
	}

	// Return pointer to the final sorted data (could be arr or aux)
	return in;
}




/**
 * 
 * Chatgpt
// Partition function using the Lomuto partition scheme
size_t omp_qsort_partition(my_cnt_t* arr, size_t low, size_t high) {
	my_cnt_t pivot = arr[high];
	size_t i = low;
	my_cnt_t temp;
	for (size_t j = low; j < high; j++) {
		if (arr[j] < pivot) {
			temp = arr[i];
			arr[i] = arr[j];
			arr[j] = temp;

			i++;
		}
	}
	// swap(&arr[i], &arr[high]);
	temp = arr[i];
	arr[i] = arr[high];
	arr[high] = temp;
	return i;
}


// Recursive quicksort function with OpenMP tasks for parallelism
void omp_parallel_qsort(my_cnt_t* arr, size_t low, size_t high) {
	size_t threshold = 1UL << 15;
	if (low < high) {
		size_t p = omp_qsort_partition(arr, low, high);

		// Recurse on left partition only if it exists (avoid underflow)
		if (p > low) {
			#pragma omp task shared(arr) if(high - low > threshold)
			omp_parallel_qsort(arr, low, p - 1);
		}

		#pragma omp task shared(arr) if(high - low > threshold)
		omp_parallel_qsort(arr, p + 1, high);

		// Wait for both tasks to complete before returning
		#pragma omp taskwait
	}
}


void qsort_omp_parallel(my_cnt_t* arr, size_t n) {
	#pragma omp parallel num_threads(THREAD_NUM)
	{
		// Only one thread starts the recursive sorting process
		#pragma omp single nowait
		{
			omp_parallel_qsort(arr, 0, n - 1);
		}
	}

	printf("old qsort omp\n");
}
*/


/**
 * 
 * claude.ai
// Safer partition function
static size_t omp_qsort_partition(my_cnt_t* arr, size_t low, size_t high) {
	// AMENDMENT: Prevent underflow and out-of-bounds access
	if (low >= high || arr == NULL) return low;

	// Median-of-three pivot selection
	size_t mid = low + (high - low) / 2;
	
	// Swap with explicit checks
	if (mid < high && arr[mid] < arr[low]) {
		my_cnt_t temp = arr[low];
		arr[low] = arr[mid];
		arr[mid] = temp;
	}
	
	if (high < low) return low;
	
	if (arr[high] < arr[low]) {
		my_cnt_t temp = arr[low];
		arr[low] = arr[high];
		arr[high] = temp;
	}
	
	if (mid < high && arr[mid] < arr[high]) {
		my_cnt_t temp = arr[high];
		arr[high] = arr[mid];
		arr[mid] = temp;
	}
	
	my_cnt_t pivot = arr[high];
	
	// AMENDMENT: Safe index initialization
	size_t i = (low > 0) ? low - 1 : 0;
	
	for (size_t j = low; j < high; j++) {
		if (arr[j] <= pivot) {
			// SAFETY: Prevent potential out-of-bounds access
			if (i + 1 < high) {
				i++;
				my_cnt_t temp = arr[i];
				arr[i] = arr[j];
				arr[j] = temp;
			}
		}
	}
	
	// Safe pivot placement
	if (i + 1 <= high) {
		my_cnt_t temp = arr[i + 1];
		arr[i + 1] = arr[high];
		arr[high] = temp;
	}
	
	return (i + 1 <= high) ? i + 1 : high;
}


// Improved parallel quicksort
void qsort_omp_parallel(my_cnt_t* arr, size_t n) {
	// SAFETY: Input validation
	if (!arr || n == 0) return;

	// AMENDMENT: Limit stack size to prevent unbounded allocation
	const size_t MAX_STACK_SIZE = n * 2;  // Conservative estimate

	// Allocate stack with explicit size limit
	size_t* stack = malloc(MAX_STACK_SIZE * sizeof(size_t));
	if (!stack) {
		fprintf(stderr, "Memory allocation failed\n");
		return;
	}
	
	// Initial stack setup with bounds checking
	size_t top = 0;
	stack[top++] = 0;
	stack[top++] = (n > 0) ? n - 1 : 0;
	
	// Control thread count
	int thread_num = THREAD_NUM;
	omp_set_num_threads(thread_num);
	
	// Parallel sorting with enhanced safety
	#pragma omp parallel
	{
		// Local thread-safe stack
		size_t local_stack[64];
		size_t local_top = 0;
		
		// Thread-safe stack population
		#pragma omp critical
		{
			// SAFETY: Prevent buffer overrun
			if (top > 1 && local_top < sizeof(local_stack)/sizeof(local_stack[0])) {
				local_stack[local_top++] = stack[0];
				local_stack[local_top++] = stack[1];
				top -= 2;
			}
		}
		
		while (local_top > 1) {
			// Safe stack pop with bounds checking
			size_t high = local_stack[--local_top];
			size_t low = local_stack[--local_top];
			
			// SAFETY: Multiple bounds and sanity checks
			if (low >= high || high >= n) continue;
			
			// Partition with safety mechanisms
			size_t pivot_idx = omp_qsort_partition(arr, low, high);
			
			// Safe subarray pushing
			#pragma omp critical
			{
				// Prevent stack overflow and invalid indexing
				if (low < pivot_idx && pivot_idx > 0 && 
					top < MAX_STACK_SIZE - 1) {
					stack[top++] = low;
					stack[top++] = pivot_idx - 1;
				}
				
				if (pivot_idx + 1 < high && 
					top < MAX_STACK_SIZE - 1) {
					stack[top++] = pivot_idx + 1;
					stack[top++] = high;
				}
			}
		}
	}
	
	free(stack);

	printf("new qsort omp\n");
}
*/



// Deepseek
// Partition function with branch prediction hints
static inline size_t omp_qsort_partition(my_cnt_t *arr, size_t low, size_t high) {
    // Median-of-three pivot selection to avoid worst-case scenarios
    size_t mid = low + (high - low) / 2;
    
    // Sort low, mid, high to get median
    if (__builtin_expect(arr[low] > arr[mid], 0)) {
        my_cnt_t tmp = arr[low];
        arr[low] = arr[mid];
        arr[mid] = tmp;
    }
    if (__builtin_expect(arr[low] > arr[high], 0)) {
        my_cnt_t tmp = arr[low];
        arr[low] = arr[high];
        arr[high] = tmp;
    }
    if (__builtin_expect(arr[mid] > arr[high], 0)) {
        my_cnt_t tmp = arr[mid];
        arr[mid] = arr[high];
        arr[high] = tmp;
    }
    
    // Place pivot at high-1
    my_cnt_t pivot = arr[mid];
    arr[mid] = arr[high - 1];
    arr[high - 1] = pivot;
    
    size_t i = low;
    size_t j = high - 1;
    
    for (;;) {
        // Move i forward while elements are less than pivot
        while (__builtin_expect(arr[++i] < pivot, 1)) {}
        
        // Move j backward while elements are greater than pivot
        while (__builtin_expect(arr[--j] > pivot, 1)) {}
        
        if (i >= j) break;
        
        // Swap elements
        my_cnt_t tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
    
    // Restore pivot
    arr[high - 1] = arr[i];
    arr[i] = pivot;
    
    return i;
}

// Insertion sort for small subarrays
static inline void insertion_sort(my_cnt_t *arr, size_t low, size_t high) {
    for (size_t i = low + 1; i <= high; i++) {
        my_cnt_t key = arr[i];
        size_t j = i;
        
        // Move elements that are greater than key
        while (j > low && __builtin_expect(arr[j - 1] > key, 0)) {
            arr[j] = arr[j - 1];
            j--;
        }
        arr[j] = key;
    }
}

// Stack frame for iterative quicksort
typedef struct {
    size_t low;
    size_t high;
} stack_frame;

// Parallel quicksort function
void qsort_omp_parallel(my_cnt_t *arr, size_t n, numa_cfg_t *numa_cfg) {

	size_t insertion_sort_threshold = 32;

    if (n <= 1) return;
    
    // Use a stack to avoid recursion
    stack_frame *stack = (stack_frame *) alloc2_memory(
    	sizeof(stack_frame) * (n / 2 + 1), numa_cfg
    );
    if (!stack) return; // Handle allocation failure
    
    size_t stack_top = 0;
    stack[stack_top].low = 0;
    stack[stack_top].high = n - 1;
    stack_top++;
    
    // Determine the number of threads to use
    int thread_num = THREAD_NUM;
    
    #pragma omp parallel num_threads(thread_num)
    {
        while (1) {
            size_t current_low, current_high;
            int has_work = 0;
            
            // Critical section to get work from stack
            #pragma omp critical
            {
                if (stack_top > 0) {
                    stack_top--;
                    current_low = stack[stack_top].low;
                    current_high = stack[stack_top].high;
                    has_work = 1;
                }
            }
            
            if (!has_work) break;
            
            while (current_high > current_low) {
                if (current_high - current_low < insertion_sort_threshold) {
                    insertion_sort(arr, current_low, current_high);
                    break;
                }
                
                size_t p = omp_qsort_partition(arr, current_low, current_high);
                
                // Push the larger partition onto the stack
                #pragma omp critical
                {
                    if (p - current_low > current_high - p) {
                        if (p > current_low) {
                            stack[stack_top].low = current_low;
                            stack[stack_top].high = p - 1;
                            stack_top++;
                        }
                        current_low = p + 1;
                    } else {
                        if (current_high > p) {
                            stack[stack_top].low = p + 1;
                            stack[stack_top].high = current_high;
                            stack_top++;
                        }
                        current_high = p - 1;
                    }
                }
            }
        }
    }
    
    dealloc_memory(stack, sizeof(stack_frame) * (n / 2 + 1));
}





#ifdef IN_DEBUG
void debug_init_barrier() {
	debug_rv = pthread_barrier_init(&debug_barrier, NULL, THREAD_NUM);
	// BARRIER_ARRIVE(&debug_barrier, debug_rv);
}
#endif /* IN_DEBUG */


void cfg_print() {
	printf("CONFIGURATION:");
#ifdef USE_HUGE
	printf("\tUSE_HUGE");
#endif /* USE_HUGE */
#ifdef CSRC_PREPOPULATED
	printf("\tCSRC_PREPOPULATED");
#endif /* CSRC_PREPOPULATED */
#ifdef CSRC_WRITE_OPTIMIZED
	printf("\tCSRC_WRITE_OPTIMIZED");
#endif /* CSRC_WRITE_OPTIMIZED */
#ifdef USE_HYPERTHREADING
	printf("\tUSE_HYPERTHREADING");
#endif /* USE_HYPERTHREADING */
#ifdef USE_WEIGHTED_INTERLEAVING
	printf("\tUSE_WEIGHTED_INTERLEAVING");
#endif /* USE_WEIGHTED_INTERLEAVING */
#ifdef USE_INTERLEAVING
	printf("\tUSE_INTERLEAVING");
#endif /* USE_INTERLEAVING */
#ifdef MEMSV
	printf("\tMEMSV");
#endif /* MEMSV */
#ifdef MEMSVB
	printf("\tMEMSVB");
#endif /* MEMSVB */
#ifdef MEM_MON
	printf("\tMEM_MON");
#endif /* MEM_MON */
	printf("\tHASH_CONSTANT: %ld", HASH_CONSTANT);
	printf("\tTHREAD_NUM: %d", THREAD_NUM);
	printf("\tAB_HIGH_THREAD_NUM: %d", AB_HIGH_THREAD_NUM);
	printf("\tAB_SIZE: %ld", AB_SIZE);
	printf("\tAB_MIN_NUM: %ld", AB_MIN_NUM);
#ifdef AB_LPT
	printf("\tAB_LPT");
#elif AB_SIMP
	printf("\tAB_SIMP");
#else /* AB_LPT */
	printf("\tAB_PRT: %.6f", AB_PRT);
#endif /* AB_LPT */
#ifdef AB_MAXFLP
	printf("\tAB_MAXFLP");
#endif /* AB_MAXFLP */
#ifdef AB_DNR
	printf("\tAB_DNR");
#endif /* AB_DNR */
#ifdef AB_HYOPT
	printf("\tAB_HYOPT");
	printf("\tAB_SYMB_THRES: %.6f", AB_SYMB_THRES);
	printf("\tAB_NUMC_THRES: %.6f", AB_NUMC_THRES);
#endif /* AB_HYOPT */
	printf("\tAB_ACC_THRES: %.6f", AB_ACC_THRES);
#ifdef DAHA
	printf("\tDAHA");
	printf("\tDAHA_REAR");
	printf("\tDAHA_QUANTILE: %.6f", DAHA_QUANTILE);
	printf("\tDAHA_SYMB_THRES: %ld", DAHA_SYMB_THRES);
	printf("\tDAHA_NUMC_THRES: %ld", DAHA_NUMC_THRES);
	printf("\tDAHA_SPMV_THRES: %ld", DAHA_SPMV_THRES);
	printf("\tDAHA_SPMM_THRES: %ld", DAHA_SPMM_THRES);
#endif /* DAHA */
	printf("\tDN_MIN: %ld", DN_MIN);
	printf("\tGUD_ALPHA: %.6f", GUD_ALPHA);
	printf("\tGUD_MIN: %ld", GUD_MIN);
	printf("\tHYB_THRES: %.6f", HYB_THRES);
	printf("\tSPMM_B_K: %d", SPMM_B_K);
#ifdef MKL_SORT
	printf("\tMKL_SORT");
#endif /* MKL_SORT */
#ifdef MKL_ENHANCE
	printf("\tMKL_ENHANCE");
#endif /* MKL_ENHANCE */
	printf("\tMKL_DENSE_VAL: %.2f", MKL_DENSE_VAL);
	printf("\n");
}













