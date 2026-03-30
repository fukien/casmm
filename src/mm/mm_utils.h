#ifndef MM_UTILS_H
#define MM_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../inc/utils.h"

#include <ittnotify.h>

#include <omp.h>


#define NUM64_T_SIZE sizeof(num64_t)
#define NUM64_PER_CACHELINE ( CACHELINE_SIZE/NUM64_T_SIZE )

#define WRITE_NT_64(dest, src)                          \
	do {                                                \
		__m512i *src_512 = (__m512i *)(src);            \
		__m512i *dest_512 = (__m512i *)(dest);          \
		_mm512_stream_si512(dest_512, *src_512);        \
	} while (0)

#define CF_HASH(a, b) ((b) != 0 ? ((double)(a) / (b)) : 0.0)
#define CF_ERSC(a, b) ((b) != 0 ? ((double)(a) / (b)) : 0.0)
#define CF_EQSC(a, b) \
	((b) != 0 && (a) > 0 ? ((double)(a) * log2((double)(a))) / (b) : 0.0)

#define HASH_WITH_INLINE_ASM(key, hashtable_size) ({          \
	my_cnt_t _key = (key);                                    \
	my_cnt_t _hashtable_size = (hashtable_size);              \
	my_cnt_t _result;                                         \
	__asm__ (                                                 \
		"mulq %[constant]"                                    \
		: "=a"(_result)                                       \
		: [constant]"r"(HASH_CONSTANT), "a"(_key)             \
		: "cc"                                                \
	);                                                        \
	_result & (_hashtable_size - 1);                          \
})




#define STATUS_BITS 2
#define STATUS_MASK ((1 << STATUS_BITS) - 1)  // 0b11
typedef struct {
	uint64_t *bitmask;  // The array storing packed 2-bit statuses
	size_t num_elements; // Number of uint64_t elements in bitmask
} status_arr_t;
status_arr_t *init_status_array(size_t num_statuses);	// Initialize status array for 'num_statuses' entries
void set_status(status_arr_t *arr, size_t index, uint8_t status);	// Set the status of a specific index
uint8_t get_status(status_arr_t *arr, size_t index);	// Get the status of a specific index
void clear_status(status_arr_t *arr, size_t index);	// Clear (reset to STATUS_0) the status of a specific index
void free_status_array(status_arr_t *arr);	// Free the allocated memory
void print_statuses(status_arr_t *arr, size_t num_statuses);	// Debug function to print status array




typedef my_rat_t (*func_op_t)(my_rat_t, my_rat_t);

typedef struct {
	double init_time;
	double symb_time;
	double mgmt_time;
	double numc_time;
	double total_time;
} runtime_t;


typedef struct {
	my_cnt_t start_row_idx;
	my_cnt_t end_row_idx;
} gud_rng_t;


static inline void get_matrix_paths(char *matrix_org_path, char *matrix_trs_path, char *dataset) {
#if defined(IN_TAMU)
	snprintf(matrix_org_path, CHAR_BUFFER_LEN, 
		"%s/%s/%s.org.mtx", TAMU_DIR, dataset, dataset
	);
	snprintf(matrix_trs_path, CHAR_BUFFER_LEN, 
		"%s/%s/%s.trs.mtx", TAMU_DIR, dataset, dataset
	);
#elif defined(IN_GROUPBY) 
	snprintf(matrix_org_path, CHAR_BUFFER_LEN, 
		"%s/%s-sorted.org.bin", GROUPBY_DIR, dataset
	);
	snprintf(matrix_trs_path, CHAR_BUFFER_LEN, 
		"%s/%s-sorted.trs.bin", GROUPBY_DIR, dataset
	);
#elif defined(IN_SSJ)
	snprintf(matrix_org_path, CHAR_BUFFER_LEN, 
		"%s/%s-sorted.org.bin", SSJ_DIR, dataset
	);
	snprintf(matrix_trs_path, CHAR_BUFFER_LEN, 
		"%s/%s-sorted.trs.bin", SSJ_DIR, dataset
	);
#else	// defined(IN_TAMU)
	snprintf(matrix_org_path, CHAR_BUFFER_LEN, 
		"%s/%s.org.mtx", RMAT_DIR, dataset
	);
	snprintf(matrix_trs_path, CHAR_BUFFER_LEN, 
		"%s/%s.trs.mtx", RMAT_DIR, dataset
	);
#endif	// defined(IN_TAMU)
}

void free_csr(csr_t *csr);
void free_csc(csc_t *csc);

void swap(triple_t *a, triple_t *b);

int compare_mycnts(const void *a, const void *b);
int compare_keyvals(const void *a, const void *b);
int compare_triples(const void *a, const void *b);
void qsort_my_cnt_t(my_cnt_t *array, my_cnt_t length);


bool mtx2trpl(triple_t** trpl, my_cnt_t* num_row, my_cnt_t* num_col, 
	my_cnt_t* num_nnz, const char *filename, const int numa_node_idx);

void trpl2mtx(const char *filename, triple_t *trpl, my_cnt_t num_row, 
	my_cnt_t num_col, my_cnt_t num_nnz, bool isSymmetric);

void trpl2csr(csr_t *csr, triple_t *trpl, const my_cnt_t num_row, 
	const my_cnt_t num_col, const my_cnt_t num_nnz, numa_cfg_t* numa_cfg);

void mtx2csr(csr_t *csr, const char *filename, numa_cfg_t* numa_cfg);

void bin2csr(csr_t *csr, const char *filename, numa_cfg_t* numa_cfg);

void bin2trpl(triple_t** trpl, my_cnt_t* num_row, my_cnt_t* num_col, 
	my_cnt_t* num_nnz, const char *filename, const int numa_node_idx);

void raw2csr(csr_t *csr, const char *filename, numa_cfg_t* numa_cfg);



void trpl_ones(triple_t* trpl, my_cnt_t num_nnz);
void trpl_transpose(triple_t* trpl, my_cnt_t *num_row, 
	my_cnt_t *num_col, my_cnt_t num_nnz);
void mtx_transpose(
	const char *filename_dest_trs, 
	const char *filename_dest_org, 
	const char* filename_src, 
	const int numa_node_idx
);

// Tailored specifically for PaRMAT-synthesized datasets
void txt_transpose(
	const char *filename_dest_trs, 
	const char *filename_dest_org, 
	const char* filename_src, 
	const int numa_node_idx
);


void print_binary(unsigned char c);
my_cnt_t get_flop(csr_t *csr_a, csr_t *csr_b);
my_cnt_t* lower_bound(my_cnt_t *start, my_cnt_t *end, my_cnt_t val);
my_rat_t* lower_bound_cfo(my_rat_t *start, my_rat_t *end, my_rat_t val);
keyval_t* lower_bound_keyval(keyval_t *start, keyval_t *end, my_rat_t val);



typedef struct {
	int _tid;
	int numa_node_idx;
	int64_t start;
	int64_t count;
	triple_t *trpl;
	triple_t *res;
} pthread_qsort_arg_t;

typedef struct {
	int64_t count;
	triple_t *res;
} merge_part_t;

typedef struct {
	int _tid;
	int64_t count0;
	int64_t count1;
	triple_t *source0;
	triple_t *source1;
	triple_t *res;
} pthread_merge_arg_t;
void p_qsortmerge(triple_t *trpl, my_cnt_t num_nnz, int numa_node_idx);


typedef int (*func_cmp_t)(const void*, const void*);
typedef struct {
	int _tid;
	numa_cfg_t *numa_cfg;
	int64_t start;
	int64_t count;
	keyval_t *keyval;
	keyval_t *res;
	func_cmp_t func_cmp;
} pthread_qsort_keyval_arg_t;

typedef struct {
	int64_t count;
	keyval_t *res;
	func_cmp_t func_cmp;
} merge_part_keyval_t;

typedef struct {
	int _tid;
	int64_t count0;
	int64_t count1;
	keyval_t *source0;
	keyval_t *source1;
	keyval_t *res;
	func_cmp_t func_cmp;
} pthread_merge_keyval_arg_t;
void p_qsortmerge_keyval(
	keyval_t *keyval, my_cnt_t num_keyval, 
	int (*func_cmp)(const void*, const void*), numa_cfg_t *numa_cfg
);
int compare_keyvals_1(const void *a, const void *b);	// key first, then val
int compare_keyvals_2(const void *a, const void *b);	// key only
int compare_keyvals_3(const void *a, const void *b);	// val first (ascending), then key
int compare_keyvals_4(const void *a, const void *b);	// val first (desceningd), then key
void check_sorted_keyvals(
	keyval_t *keyval, int64_t num_keyval, 
	int (*func_cmp)(const void*, const void*)
);

typedef struct {
	int _tid;
	my_cnt_t num;
	my_cnt_t *in;
	my_cnt_t *out;
	my_cnt_t *partial_sum;
	pthread_barrier_t *barrier;
} pthread_scan_arg_t;
void scan(my_cnt_t *out, my_cnt_t *in, my_cnt_t num);


typedef struct {
	int _tid;
	my_cnt_t num;
	my_rat_t *in;
	my_rat_t *out;
	my_rat_t *partial_sum;
	pthread_barrier_t *barrier;
} pthread_scan_rat_arg_t;
void scan_rat(my_rat_t *out, my_rat_t *in, my_cnt_t num);


typedef struct {
	int _tid;
	my_cnt_t num;
	keyval_t *in;
	my_rat_t *out;
	my_rat_t *partial_sum;
	pthread_barrier_t *barrier;
} pthread_scan_keyval_arg_t;
void scan_keyval(my_rat_t *out, keyval_t *in, my_cnt_t num);


typedef struct {
	int _tid;
	my_cnt_t intprod;
	my_cnt_t * __attribute__((unused)) row_nz;
	csr_t *csr_a;
	csr_t *csr_b;
	my_cnt_t * __attribute__((unused)) row_flp;
	my_rat_t * __attribute__((unused)) row_cf;
	my_cnt_t * __attribute__((unused)) row_cnt;
} pthread_intprod_arg_t;
my_cnt_t get_intprod(csr_t* csr_a, csr_t* csr_b, my_cnt_t *row_nz);
void cf_populate(csr_t *csr_a, csr_t *csr_b, 
	my_cnt_t *row_flp, my_rat_t *row_cf, my_cnt_t *row_cnt);
void flp_populate(csr_t *csr_a, csr_t* csr_b, my_cnt_t *row_flp);

typedef struct {
	int _tid;
	my_cnt_t num_row;
	my_cnt_t average_intprod;
	my_cnt_t* rows_offset;
	my_cnt_t* row_start_idcs;
} pthread_set_rows_offset_arg_t;
void set_rows_offset(my_cnt_t num_row, my_cnt_t total_intprod, my_cnt_t* rows_offset, my_cnt_t* row_start_idcs);


typedef struct {
	int _tid;
	my_cnt_t num_row;
	my_rat_t average_cfo;
	my_cnt_t* rows_offset;
	my_rat_t* accum_cfo;
} pthread_rebalance_rows_offset_arg_t;
void rebalance_rows_offset(my_cnt_t num_row, my_rat_t cfo_sum, my_cnt_t* rows_offset, my_rat_t* accum_cfo);


my_rat_t func_mul(my_rat_t, my_rat_t);
my_rat_t func_add(my_rat_t, my_rat_t);


typedef struct {
	my_cnt_t num_row;
	my_cnt_t num_col;
	my_cnt_t num_nnz;
	my_cnt_t *row_nz;
	my_cnt_t *row_ptrs;
	my_cnt_t *cols;
	my_rat_t *rats;
} csr_nz_t;
void free_csr_nz(csr_nz_t *csr_nz);


void write_non_temporal_batched(
	my_cnt_t *data_cols,
	my_rat_t *data_vals,
	my_cnt_t offset,
	my_cnt_t length,
	my_cnt_t *ht_check,
	my_rat_t *ht_value
);

void write_non_temporal_batched_cols(
	my_cnt_t *data_cols,
	my_cnt_t offset,
	my_cnt_t length,
	my_cnt_t *ht_check
);

void write_non_temporal_batched_vals(
	my_rat_t *data_vals,
	my_cnt_t offset,
	my_cnt_t length,
	my_cnt_t *ht_check,
	my_rat_t *ht_value
);


typedef struct {
	int _tid;
	csr_t *csr;
	my_cnt_t* max_row_array;
} pthread_max_csr_row_arg_t;
void max_csr_row(csr_t *csr, my_cnt_t *max_row_array);


void radix_sort_keyval(keyval_t *arr, keyval_t *aux, size_t n);
keyval_t * radix_sort_keyval_toggling(keyval_t *arr, keyval_t *aux, size_t n);
keyval_t * radix_sort_keyval_toggling_unrolling(keyval_t *arr, keyval_t *aux, size_t n);



void qsort_omp_parallel(my_cnt_t* arr, size_t n, numa_cfg_t *numa_cfg);



#ifdef IN_DEBUG
void debug_init_barrier();
#endif /* IN_DEBUG */

void cfg_print();

#ifdef __cplusplus
}
#endif

#endif /* MM_UTILS_H */











