#include "inc/utils.h"
#include "mm/common.h"


void test_0();	// disk file mapping test
void test_1();	// zeor-based or one-based test for datasets
void test_2();	// parallel memcpy test
void test_3();	// POWER_OF_TWO test
void test_4();	// cf_test for hash/esc macro
void test_5();	// p_qsortmerge_keyval test
void test_6();	// HASH_WITH_INLINE_ASM test
void test_7(); 	// validate the correctness of radix_sort_keyval_toggling_unrolling
void test_8();	// test 2-bit mask (aka. status)
void test_9();	// test p_qsortmerge_frid_nnz

#if defined(USE_WEIGHTED_INTERLEAVING) && LINUX_VERSION_CODE >= KERNEL_VERSION(6, 9, 0)
void test_10();	// test differenet memory interleaving weights dynamically
#endif /* defined(USE_WEIGHTED_INTERLEAVING) && LINUX_VERSION_CODE >= KERNEL_VERSION(6, 9, 0) */

void test_11();	// openmp guided schedule test
void test_12(); // scan_rat
void test_13(); // openmp-based quicksort test


// int main(int argc, char** argv) {
int main() {
	return 0;
}


void test_0() {
	char filepath[CHAR_BUFFER_LEN];
	snprintf(filepath, CHAR_BUFFER_LEN, "/data/dataset/groupby-sorted/tiny.bin");

	delfile(filepath);

	size_t triple_num = 7;
	size_t buffer_size = triple_num * sizeof(triple_t);

	triple_t *data_mem = (triple_t *) map_disk_file(filepath, buffer_size);

	data_mem[0].user_id = 0;
	data_mem[0].item_id = 1;
	data_mem[0].rating = 1;

	data_mem[1].user_id = 0;
	data_mem[1].item_id = 2;
	data_mem[1].rating = 2;

	data_mem[2].user_id = 1;
	data_mem[2].item_id = 2;
	data_mem[2].rating = 3;

	data_mem[3].user_id = 1;
	data_mem[3].item_id = 0;
	data_mem[3].rating = 4;

	data_mem[4].user_id = 2;
	data_mem[4].item_id = 1;
	data_mem[4].rating = 5;

	data_mem[5].user_id = 3;
	data_mem[5].item_id = 2;
	data_mem[5].rating = 6;

	data_mem[6].user_id = 3;
	data_mem[6].item_id = 1;
	data_mem[6].rating = 7;

	munmap(data_mem, buffer_size);

}


void test_1(char **argv) {
	srand(32768);
	char *dataset = argv[1];

	char filepath[CHAR_BUFFER_LEN];
	snprintf(filepath, CHAR_BUFFER_LEN, "%s/%s-sorted.trs.bin", GROUPBY_DIR, dataset);

	int fd;
	struct stat sb;
	fd = open(filepath, O_RDONLY);

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
	close(fd);
	
	triple_t *data_mem = (triple_t *) map_disk_file(filepath, sb.st_size);

	int64_t num_row, num_col, num_nnz;
	num_row = data_mem[0].user_id;
	num_col = data_mem[0].item_id;
	num_nnz = (my_cnt_t) data_mem[0].rating;

	for (int64_t idx = 1; idx <= num_nnz; idx ++) {
		assert( data_mem[idx].user_id >= 0 );
		assert( data_mem[idx].user_id < num_row );
		assert( data_mem[idx].item_id >= 0 );
		assert( data_mem[idx].item_id < num_col );
	}

	munmap(data_mem, sb.st_size);
}


void test_2() {
	my_cnt_t total_num = 10099997;	// a prime number
	my_cnt_t* array_a = (my_cnt_t*) malloc(sizeof(my_cnt_t)*total_num);
	my_cnt_t* array_b = (my_cnt_t*) malloc(sizeof(my_cnt_t)*total_num);

	for (my_cnt_t idx = 0; idx < total_num; idx ++) {
		array_a[idx] = rand();
	}

	memcpy(array_b, array_a, sizeof(my_cnt_t)*total_num);
	parallel_memcpy(array_b, array_a, sizeof(my_cnt_t)*total_num, THREAD_NUM);

	for (my_cnt_t idx = 0; idx < total_num; idx ++) {
		assert(array_a[idx] == array_b[idx]);
	}

	free(array_a);
	free(array_b);
}


void test_3() {
	my_cnt_t a = 536870912;
	printf("%ld\t%d\t%ld\n", 
		a, 
		POWER_OF_TWO_TO_POWER(a),
		NEXT_POW_2(a)
	);

	a = a-2;
	printf("%ld\t%d\t%ld\n", 
		a, 
		POWER_OF_TWO_TO_POWER(a),
		NEXT_POW_2(a)
	);
}


void test_4() {
	// Test values for a and b
	int64_t test_values_a[] = {10, 0, -5, 100, 500};
	int64_t test_values_b[] = {2, 0, 10, 50, 0};

	// Loop over test cases
	printf("Testing CF_HASH and CF_EQSC:\n");
	printf("========================================\n");
	printf("|   a   |   b   |	 CF_HASH(a, b)	|	 CF_EQSC(a, b)	 |\n");
	printf("========================================\n");

	for (size_t i = 0; i < sizeof(test_values_a) / sizeof(test_values_a[0]); ++i) {
		int64_t a = test_values_a[i];
		int64_t b = test_values_b[i];

		double hash_result = CF_HASH(a, b);
		double esc_result = CF_EQSC(a, b);

		printf("| %6ld | %6ld | %20.6f | %20.6f |\n", a, b, hash_result, esc_result);
	}

	printf("========================================\n");
}


void test_5(){
	srand(time(NULL));

	extern numa_cfg_t numa_cfg;
	numa_cfg_init(&numa_cfg);

	int local_numa_idx = numa_cfg.local_cores[0]/SOCKET_CORE_NUM;
	int cxl_numa_idx = 2;
	int cur_numa_idx = local_numa_idx;
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

	my_cnt_t num_nnz = 1 << 12;

	keyval_t* keyval = alloc2_memory(
		sizeof(keyval_t)*num_nnz, &numa_cfg
	);
	memset(keyval, 0, sizeof(keyval_t)*num_nnz);

	for (my_cnt_t idx = 0; idx < num_nnz; idx ++) {
		keyval[idx].key = rand();
		keyval[idx].val = (double) rand()/RAND_MAX;
	}


#if THREAD_NUM > 1
	func_cmp_t func_cmp = compare_keyvals_3;
	struct timespec time_start, time_end;
	clock_gettime(CLOCK_REALTIME, &time_start);
	p_qsortmerge_keyval(keyval, num_nnz, func_cmp, &numa_cfg);
	clock_gettime(CLOCK_REALTIME, &time_end);
	double sorttime = diff_sec(time_start, time_end);
	printf("sorttime: %.9f\n", sorttime);

	for (int idx = 0; idx < num_nnz; idx ++) {
		printf("%ld\t%f\n", keyval[idx].key, keyval[idx].val);
	}
	check_sorted_keyvals(keyval, num_nnz, func_cmp);
#endif /* THREAD_NUM > 1 */

	dealloc_memory(keyval, sizeof(keyval_t)*num_nnz);
	numa_cfg_free(&numa_cfg);
}


void test_6() {
	// my_cnt_t key = 597923;
	// my_cnt_t hashtable_size = 262144; // Power of 2

	// my_cnt_t hash_value = HASH_WITH_INLINE_ASM(key, hashtable_size);
	// printf("Hashed value: %ld\n", hash_value);

	// key = 286867;
	// hash_value = HASH_WITH_INLINE_ASM(key, hashtable_size);
	// printf("Hashed value: %ld\n", hash_value);

	// key = 194134;
	// hash_value = HASH_WITH_INLINE_ASM(key, hashtable_size);
	// printf("Hashed value: %ld\n", hash_value);

	// key = 461260;
	// hash_value = HASH_WITH_INLINE_ASM(key, hashtable_size);
	// printf("Hashed value: %ld\n", hash_value);
}


void test_7() {

	keyval_t * out = (keyval_t *) malloc(sizeof(keyval_t) * 10);

	keyval_t in[10] = {
		{0x0000000000050034, 5},  // LSB: 0x34
		{0x0000000000010034, 1},  // LSB: 0x34
		{0x00000000000A0034, 10}, // LSB: 0x34
		{0x0000000000030034, 3},  // LSB: 0x34
		{0x0000000000040034, 4},  // LSB: 0x34
		{0x0000000000020034, 2},  // LSB: 0x34
		{0x0000000000060034, 6},  // LSB: 0x34
		{0x0000000000090034, 9},  // LSB: 0x34
		{0x0000000000080034, 8},  // LSB: 0x34
		{0x0000000000070034, 7}   // LSB: 0x34
	};

	keyval_t * res = radix_sort_keyval_toggling_unrolling(in, out, 10);

	for (int idx = 0; idx < 10; idx ++) {
		printf("%d: %lx\n", idx, res[idx].key);
	}

	free(out);
}


void test_8() {
	size_t num_statuses = 100;  // Example: 100 entries
	status_arr_t *arr = init_status_array(num_statuses);
	if (!arr) {
		perror("Failed to allocate status array");
		exit(1);
	}

	// Set some statuses
	set_status(arr, 0, 1);
	set_status(arr, 1, 2);
	set_status(arr, 2, 1);
	set_status(arr, 3, 3);
	set_status(arr, 99, 1);  // Last element test

	printf("Status array after setting values:\n");
	print_statuses(arr, num_statuses);

	// Get some statuses
	printf("Status at index 0: %d\n", get_status(arr, 0));
	printf("Status at index 99: %d\n", get_status(arr, 99));

	// Clear a status
	clear_status(arr, 1);
	printf("After clearing index 1:\n");
	print_statuses(arr, num_statuses);

	// Free memory
	free_status_array(arr);
}


void test_9() {
	srand(time(NULL));

	extern numa_cfg_t numa_cfg;
	numa_cfg_init(&numa_cfg);

	int local_numa_idx = numa_cfg.local_cores[0]/SOCKET_CORE_NUM;
	int cxl_numa_idx = 2;
	int cur_numa_idx = local_numa_idx;
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


	my_cnt_t number = 593;
	frid_nnz_t data[number];
	for (int i = 0; i < number; i++) {
		data[i].freq = (i % 10) * 2.5 + (rand() % 100) / 100.0;  // Randomized frequency with step pattern
		data[i].nnz = (rand() % 200) + 1;  // Randomized nnz between 1 and 200
		data[i].rid = i;
	}

	// Perform parallel (or single-threaded) sort
	p_qsortmerge_frid_nnz(data, number, compare_frid_nnz, &numa_cfg);

	// for (int i = 0; i < number; i++) {
	// 	printf("{ .freq = %.2f, .nnz = %ld, .rid = %ld },\n", data[i].freq, data[i].nnz, data[i].rid);
	// }

	check_sorted_frid_nnz(data, number, compare_frid_nnz);

	numa_cfg_free(&numa_cfg);
}


#if defined(USE_WEIGHTED_INTERLEAVING) && LINUX_VERSION_CODE >= KERNEL_VERSION(6, 9, 0)
void test_10() {
	struct timespec time_start, time_end;

	size_t buffer_size = 1UL << 30;

	extern numa_cfg_t numa_cfg;
	numa_cfg_init(&numa_cfg);
	uint8_t numa_mask_val = 6;

	numa_bitmask_clearall(numa_cfg.numa_mask);
	for (int node_idx = 0; node_idx < numa_cfg.nr_nodes; node_idx ++) {
		if ( GETBIT( numa_mask_val, node_idx ) ) {
			numa_bitmask_setbit(numa_cfg.numa_mask, node_idx);
		}
	}
	numa_cfg.weights[0] = 9;
	numa_cfg.weights[1] = 10;
	numa_cfg.weights[2] = 7;
	set_interleaving_weight(&numa_cfg);
	clock_gettime(CLOCK_REALTIME, &time_start);
	void *addr_0 = alloc_weighted(buffer_size, &numa_cfg);
	memset(addr_0, 0, buffer_size);
	clock_gettime(CLOCK_REALTIME, &time_end);
	printf("addr_0_pagefault_time: %.9f\t", diff_sec(time_start, time_end));

	numa_cfg.weights[0] = 6;
	numa_cfg.weights[1] = 99;
	numa_cfg.weights[2] = 1;
	set_interleaving_weight(&numa_cfg);
	clock_gettime(CLOCK_REALTIME, &time_start);
	void *addr_1 = alloc_weighted(buffer_size, &numa_cfg);
	memset(addr_1, 0, buffer_size);
	clock_gettime(CLOCK_REALTIME, &time_end);
	printf("addr_1_pagefault_time: %.9f\t", diff_sec(time_start, time_end));

	numa_cfg.weights[0] = 6;
	numa_cfg.weights[1] = 1;
	numa_cfg.weights[2] = 99;
	set_interleaving_weight(&numa_cfg);
	clock_gettime(CLOCK_REALTIME, &time_start);
	void *addr_2 = alloc_weighted(buffer_size, &numa_cfg);
	memset(addr_2, 0, buffer_size);
	clock_gettime(CLOCK_REALTIME, &time_end);
	printf("addr_2_pagefault_time: %.9f\n", diff_sec(time_start, time_end));


	clock_gettime(CLOCK_REALTIME, &time_start);
	memset(addr_0, 0, buffer_size);
	clock_gettime(CLOCK_REALTIME, &time_end);
	printf("addr_0_set_time: %.9f\t", diff_sec(time_start, time_end));

	clock_gettime(CLOCK_REALTIME, &time_start);
	memset(addr_1, 0, buffer_size);
	clock_gettime(CLOCK_REALTIME, &time_end);
	printf("addr_1_set_time: %.9f\t", diff_sec(time_start, time_end));

	clock_gettime(CLOCK_REALTIME, &time_start);
	memset(addr_2, 0, buffer_size);
	clock_gettime(CLOCK_REALTIME, &time_end);
	printf("addr_2_set_time: %.9f\n", diff_sec(time_start, time_end));


	clock_gettime(CLOCK_REALTIME, &time_start);
	memset(addr_0, 0, buffer_size);
	clock_gettime(CLOCK_REALTIME, &time_end);
	printf("addr_0_reset_time: %.9f\t", diff_sec(time_start, time_end));

	clock_gettime(CLOCK_REALTIME, &time_start);
	memset(addr_1, 0, buffer_size);
	clock_gettime(CLOCK_REALTIME, &time_end);
	printf("addr_1_reset_time: %.9f\t", diff_sec(time_start, time_end));

	clock_gettime(CLOCK_REALTIME, &time_start);
	memset(addr_2, 0, buffer_size);
	clock_gettime(CLOCK_REALTIME, &time_end);
	printf("addr_2_reset_time: %.9f\n", diff_sec(time_start, time_end));

	dealloc_memory(addr_0, buffer_size);
	dealloc_memory(addr_1, buffer_size);
	dealloc_memory(addr_2, buffer_size);



	numa_cfg_free(&numa_cfg);
}
#endif /* defined(USE_WEIGHTED_INTERLEAVING) && LINUX_VERSION_CODE >= KERNEL_VERSION(6, 9, 0) */


void test_11() {
	int thread_num = 3;
	int total_iterations = 113;
	int i;

	// Set the number of threads
	omp_set_num_threads(thread_num);

	// Array to track the number of iterations each thread processes
	int thread_iterations[thread_num];
	memset(thread_iterations, 0, sizeof(thread_iterations));


	// Parallel region with a for loop using guided scheduling
	#pragma omp parallel for schedule(guided) private(i)
	for (i = 0; i < total_iterations; i++) {
		int thread_id = omp_get_thread_num();

		// Simulate work by printing the iteration and thread information
		printf("Thread %d processing iteration %d\n", thread_id, i);

		// Increment the count of iterations for this thread
		#pragma omp atomic
		thread_iterations[thread_id]++;
	}

	// Print the number of iterations processed by each thread
	printf("\nIterations processed by each thread:\n");
	for (i = 0; i < thread_num; i++) {
		printf("Thread %d processed %d iterations\n", i, thread_iterations[i]);
	}
}


void test_12() {
	my_cnt_t num = 1000000;
	my_rat_t *input = (my_rat_t *) malloc(sizeof(my_rat_t) * num);
	my_rat_t *serial_output = (my_rat_t *) malloc( sizeof(my_rat_t) * (num+1));
	my_rat_t *parallel_output = (my_rat_t *) malloc( sizeof(my_rat_t) * (num+1));

	srand(time(NULL));
	for (my_cnt_t i = 0; i < num; i++) {
		input[i] = ((my_rat_t)rand() / RAND_MAX) * 100.0;  // Random values between 0 and 100
	}

	serial_output[0] = 0.0;
	for (my_cnt_t i = 0; i < num - 1; i++) {
		serial_output[i + 1] = serial_output[i] + input[i];
	}

	scan_rat(parallel_output, input, num+1);

	const double epsilon = 1e-6;


	for (my_cnt_t i = 0; i < num+1; i++) {
		if ( fabs(serial_output[i]-parallel_output[i]) > epsilon) {
			printf("Mismatch at index %ld: expected %f, got %f with diff %f\n",
				i, serial_output[i], parallel_output[i],
				fabs(serial_output[i]-parallel_output[i])
			);
		}
	}

	free(input);
	free(serial_output);
	free(parallel_output);
}



void test_13() {
	srand(time(NULL));

	extern numa_cfg_t numa_cfg;
	numa_cfg_init(&numa_cfg);

	int local_numa_idx = numa_cfg.local_cores[0]/SOCKET_CORE_NUM;
	int cxl_numa_idx = 2;
	int cur_numa_idx = local_numa_idx;
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

	my_cnt_t num_nnz = 1UL << 27;
	// num_nnz = 110047;
	// num_nnz = 10;
	my_cnt_t *arr = (my_cnt_t *) alloc2_memory(
		sizeof(my_cnt_t) * num_nnz, &numa_cfg
	);

	for (my_cnt_t idx = 0; idx < num_nnz; idx ++) {
		arr[idx] = rand();
	}

	printf("generation done\n");

	struct timespec time_start, time_end;
	clock_gettime(CLOCK_REALTIME, &time_start);
	qsort_omp_parallel(arr, num_nnz, &numa_cfg);
	clock_gettime(CLOCK_REALTIME, &time_end);
	printf("sortime: %.9f\n", diff_sec(time_start, time_end));

	// for (my_cnt_t idx = 0; idx < num_nnz; idx ++) {
	// 	printf("%ld: %ld\n", idx, arr[idx]);
	// }

	dealloc_memory(arr, sizeof(my_cnt_t) * num_nnz);

	numa_cfg_free(&numa_cfg);
}





