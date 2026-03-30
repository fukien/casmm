// #pragma once


#ifndef STORE_H
#define STORE_H


#define CACHELINE_SIZE 64
#define XPLINE_SIZE 256

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef int64_t my_cnt_t;
typedef double my_rat_t;

typedef struct {
	my_cnt_t user_id;
	my_cnt_t item_id;
	my_rat_t rating;
} triple_t;


typedef struct {
	my_cnt_t user_id;
	my_cnt_t item_id;
} view_t;


typedef struct {
	my_cnt_t l_user_id;
	my_cnt_t r_user_id;
} project_t;


typedef struct {
	my_cnt_t l_user_id;
	my_cnt_t r_user_id;
	my_rat_t l_val;
	my_rat_t r_val;
} groupby_t;


typedef struct {
	my_cnt_t num_row;
	my_cnt_t num_col;
	my_cnt_t num_nnz;
	my_cnt_t *row_ptrs;
	my_cnt_t *cols;
	my_rat_t *rats;
} csr_t;

typedef struct {
	my_cnt_t num_row;
	my_cnt_t num_col;
	my_cnt_t num_nnz;
	my_cnt_t *col_ptrs;
	my_cnt_t *rows;
	my_rat_t *rats;
} csc_t;

typedef struct {
	my_rat_t *rats;
	my_cnt_t num_row;
	my_cnt_t num_col;
} dense_t;


typedef struct {
	my_cnt_t key;
	my_rat_t val;
} keyval_t;


typedef union {
	int64_t i64;	// Integer 64-bit
	double dbl;		// Double precision floating-point
} num64_t;

#ifdef __cplusplus
}
#endif


#endif /* STORE_H */