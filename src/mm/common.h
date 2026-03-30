#ifndef COMMON_H
#define COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mm_utils.h"

#ifdef USE_PAPI
#include "../papi/mypapi.h"
#endif /* USE_PAPI */


typedef struct {
	int _tid;
	my_cnt_t *rows_offset;
	csr_t *csr_a;
	csr_t *csr_b;
	my_cnt_t *local_nnz_max;
	numa_cfg_t *numa_cfg;
} comm_ptrl_init_arg_t;


typedef struct {
	int _tid;
	csr_t *csr_a;
	csr_t *csr_b;
	my_cnt_t *rows_offset;
	my_cnt_t *row_nz;
	my_cnt_t local_nnz_max;
	numa_cfg_t *numa_cfg;
#ifdef IN_VERIFY
	int64_t over_predict;
	int64_t under_predict;
#endif /* IN_VERIFY */
#ifdef IN_DEBUG
	double _symb_time;
#endif /* IN_DEBUG */
} esc_symbolic_arg_t;


typedef struct {
	// int thread_num;
	my_cnt_t total_intprod;

	my_cnt_t num_row;

	my_cnt_t max_intprod;
	my_cnt_t min_ht_size;

	// row-level
	char *bin_id;
	my_cnt_t *row_nz;

	// thread-level
	my_cnt_t *rows_offset;
	my_cnt_t ht_size_array[THREAD_NUM];
	my_cnt_t** local_hash_table_id;
	my_rat_t** local_hash_table_val;
} bin_t;


typedef struct {
	int _tid;
	my_cnt_t intprod;
	bin_t *bin;
	csr_t *csr_a;
	csr_t *csr_b;

	// rlrb-specific
	struct tub_t * __attribute__((unused)) tub;
	status_arr_t * __attribute__((unused)) status_arr;
	struct rlrb_t * __attribute__((unused)) rlrb;
	numa_cfg_t * __attribute__((unused)) numa_cfg;
} pthread_set_intprod_num_arg_t;


typedef struct {
	int _tid;
	my_cnt_t num_row;
	my_cnt_t average_intprod;
	my_cnt_t *ps_row_nz;
	bin_t *bin;
} pthread_hash_set_rows_offset_arg_t;


typedef struct {
	int _tid;
	my_cnt_t num_row;
	my_cnt_t num_col;
	bin_t *bin;
} pthread_set_bin_id_arg_t;


typedef struct {
	int _tid;
	numa_cfg_t* numa_cfg;
	my_cnt_t num_col;
	bin_t *bin;
} pthread_bin_create_local_hash_table_arg_t;


typedef struct {
	int _tid;
	csr_t *csr_a;
	csr_t *csr_b;
	bin_t *bin;

	// gud
	void * __attribute__((unused)) asst;

	// gud & hyb
	numa_cfg_t * __attribute__((unused)) numa_cfg;

	// rlrb-specific
	status_arr_t * __attribute__((unused)) status_arr;
	struct rlrb_t * __attribute__((unused)) rlrb;

#ifdef IN_VERIFY
	int64_t over_predict;
	int64_t under_predict;
#endif /* IN_VERIFY */
#ifdef IN_DEBUG
	double _symb_time;
#endif /* IN_DEBUG */
} hash_symbolic_arg_t;

typedef struct {
	int _tid;
	func_op_t func_op;
	csr_t *csr_a;
	csr_t *csr_b;
	csr_t *csr_c;
	bin_t *bin;

	// gud
	void * __attribute__((unused)) asst;

	// gud & hyb
	numa_cfg_t *numa_cfg;

	// rlrb-specific
	status_arr_t * __attribute__((unused)) status_arr;
	struct rlrb_t * __attribute__((unused)) rlrb;

#ifdef IN_DEBUG
	double _numc_time;
#endif /* IN_DEBUG */
#ifdef IN_EXAMINE
	double _set_time;
	double _comp_time;
	double _write_time;
	my_cnt_t _flop_cnt;
#endif /* IN_EXAMINE */
#ifdef USE_PAPI
	long long _PAPI_numc_values[NUM_PAPI_EVENT];
#endif /* USE_PAPI */
} hash_numeric_arg_t;



// -------------------------------------------------------------------------
// Structures for arguments passed to pthread_qsort and pthread_merge
// -------------------------------------------------------------------------
typedef struct {
	my_rat_t freq;
	my_cnt_t nnz;
	my_cnt_t rid;
} frid_nnz_t;


typedef struct {
	int       _tid;            // thread id (for debugging/logging)
	numa_cfg_t	*numa_cfg;   // e.g., which NUMA node to allocate from
	int64_t   start;           // start index
	int64_t   count;           // number of items in this chunk
	frid_nnz_t *base;          // pointer to the original input array
	frid_nnz_t *res;           // pointer to the sorted result for this chunk
	func_cmp_t func_cmp;
} pthread_qsort_frid_arg_t;


typedef struct {
	int       _tid;
	int64_t   count0;
	int64_t   count1;
	frid_nnz_t *source0;
	frid_nnz_t *source1;
	frid_nnz_t *res;  // merged result
	func_cmp_t func_cmp;
} pthread_merge_frid_arg_t;


typedef struct {
	int64_t   count;
	frid_nnz_t *res;
	func_cmp_t func_cmp;
} merge_part_frid_t;


typedef struct tub_t {
	bool flag;	// go with object-level interleaving
	my_cnt_t num_b_row;
	my_cnt_t *trl_b_freq;
	my_rat_t *agg_b_freq;
	pthread_barrier_t barrier;
} tub_t;


typedef struct rlrb_t {
	my_cnt_t *trl_cnt;
	frid_nnz_t *frid_nnz;
	my_cnt_t *rid_old2new;

	csr_t *org_csr_b;
	csr_t *dram_csr_b;
	csr_t *cxl_csr_b;
} rlrb_t;	// row-level redistribution of matrix B

// -------------------------------------------------------------------------
// Comparison function: descending freq, then descending nnz
// -------------------------------------------------------------------------
int compare_frid_nnz(const void *a, const void *b);
void p_qsortmerge_frid_nnz(frid_nnz_t *base, my_cnt_t number, 
	int (*func_cmp)(const void*, const void*), numa_cfg_t *numa_cfg);
void check_sorted_frid_nnz(frid_nnz_t *arr, int64_t num_elements, 
	int (*func_cmp)(const void*, const void*)
);




void comm_init_local(csr_t *csr_a, csr_t *csr_b, my_cnt_t *rows_offset, 
	my_cnt_t*local_nnz_max, numa_cfg_t* numa_cfg);

void esc_symbolic(csr_t* csr_a, csr_t* csr_b,
	my_cnt_t* rows_offset, my_cnt_t* row_nz, 
	my_cnt_t* local_nnz_max, numa_cfg_t* numa_cfg);


void init_bin(bin_t* bin, my_cnt_t num_row, my_cnt_t ht_size, numa_cfg_t *numa_cfg);
void free_bin(bin_t *bin);
void set_intprod_num(bin_t *bin, csr_t *csr_a, csr_t *csr_b);
void *pthread_hash_set_rows_offset(void *param);
void hash_set_rows_offset(bin_t *bin, my_cnt_t num_row, numa_cfg_t* numa_cfg);
void set_bin_id(bin_t *bin, my_cnt_t num_row, my_cnt_t num_col);
void bin_create_local_hash_table(bin_t *bin, my_cnt_t num_col, numa_cfg_t* numa_cfg);
void hash_symbolic(csr_t *csr_a, csr_t *csr_b, bin_t *bin);


void init_tub(tub_t *tub, csr_t* csr_b, numa_cfg_t *numa_cfg);
void free_tub(tub_t *tub);
void init_rlrb(rlrb_t *rlrb);
void free_rlrb(rlrb_t *rlrb);
void rlrb_pack(csr_t* csr_b, status_arr_t* status_arr, 
	rlrb_t* rlrb, numa_cfg_t *numa_cfg);
void set_intprod_num_tub(bin_t *bin, tub_t *tub, 
	status_arr_t *status_arr, rlrb_t *rlrb,
	csr_t *csr_a, csr_t *csr_b, numa_cfg_t *numa_cfg);

void hash_symbolic_rlrb(csr_t *csr_a, csr_t *csr_b, bin_t *bin,
	status_arr_t *status_arr, rlrb_t *rlrb);

#ifdef __cplusplus
}
#endif

#endif /* COMMON_H */















