#include <algorithm>
#include <cmath>
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <numeric>
#include <omp.h>
#include <random>
#include <sstream>
#include <stdio.h>
#include <string>

#include "../SplitTuples.h"
//#include "overridenew.h"
#include "../CSC.h"
#include "../heap_mult.h"
#include "../multiply.h"
#include "../outer_mult.h"
#include "../utility.h"
#include "sample_common.hpp"

using namespace std;

#define VALUETYPE double
// #define INDEXTYPE int32_t
// #define ITERS 15
#define INDEXTYPE int64_t
#define ITERS 4


template <typename T> void PrintVector(T *v1, T *v2, int size) {
  int counter = 0;
  for (int i = 0; i < size; ++i) {
    if (counter == 20)
      break;
    if (v1[i] != v2[i]) {
      cout << i << "-> " << v1[i] << " " << v2[i] << endl;
      counter++;
    }
  }
  for (int i = 0; i < 20; i++) {
    cout << v1[i] << ' ';
  }
  // cout << endl;
  // for (int i=0; i < size; i++) {
  //     cout << v2[i] << ' ';
  // }
  cout << endl;
}
int main(int argc, char *argv[]) {
  vector<int> tnums = {1};

// omp_set_nested(1);
  CSC<INDEXTYPE, VALUETYPE> A_csc, B_csc, C_csc_corret;
  CSR<INDEXTYPE, VALUETYPE> C_csr_corret;

  if (argc < 4) {
    cout << "Normal usage: ./spgemm {gen|binary|text} {rmat|er|ts|matrix1.txt} "
            "{scale|matrix2.txt} {edgefactor|product.txt} <numthreads>"
         << endl;
    return -1;
  } else if (argc < 6) {
    cout << "Normal usage: ./spgemm {gen|binary|text} {rmat|er|ts|matrix1.txt} "
            "{scale|matrix2.txt} {edgefactor|product.txt} <numthreads>"
         << endl;
  } else {
    cout << "Running on " << argv[5] << " processors" << endl << endl;
    tnums = {atoi(argv[5])};
  }

  /* Generating input matrices based on argument */
  SetInputMatricesAsCSC(A_csc, B_csc, argv);

  CSR<INDEXTYPE, VALUETYPE> B_csr(B_csc);
  CSR<INDEXTYPE, VALUETYPE> A_csr(A_csc);
  A_csc.Sorted();
  A_csr.Sorted();
  B_csc.Sorted();
  B_csr.Sorted();

  A_csc.shuffleIds();
  B_csr.shuffleIds();

  // auto obj = SplitTuples<INDEXTYPE, VALUETYPE>(A_csr, 2);
  // auto left = CSC<INDEXTYPE, VALUETYPE>(obj.splitTuples[0], obj.rows / 2, obj.cols, plus<INDEXTYPE>());
  // auto right = CSC<INDEXTYPE, VALUETYPE>(obj.splitTuples[1], obj.rows / 2, obj.cols, plus<INDEXTYPE>());
  // cout << "left non-zeros = " << obj.splitTuples[0].size() << endl;
  // cout << "right non-zeros = " << obj.splitTuples[1].size() << endl;
  double start, end, sec, ave_sec, flops;
  /* Count total number of floating-point operations */
  auto nfop = get_flop(A_csc, B_csr);
  nfop = nfop / 2; // we count multiplication count only
  cout << "Total number of floating-point operations including addition and "
          "multiplication in SpGEMM (A * B): "
       << nfop << endl
       << endl;


  for (int tnum : tnums) {
    omp_set_num_threads(tnum);

    CSR<INDEXTYPE, VALUETYPE> C_csr;
    /* First execution is excluded from evaluation */

    OuterSpGEMM(A_csc, B_csr, C_csr, atoi(argv[6]), atoi(argv[7]), 1);
    C_csr.make_empty();
    ave_sec = 0;
    double ave_symb_sec = 0;
    double ave_numc_sec = 0;

    for (int i = 0; i < ITERS; ++i) {
      start = omp_get_wtime();
      toy_time_t cur_toy_time = OuterSpGEMM(A_csc, B_csr, C_csr, atoi(argv[6]), atoi(argv[7]), 1);
      end = omp_get_wtime();

      // MKLSpGEMM<true, INDEXTYPE, VALUETYPE>(A_csr, B_csr, C_csr_corret);
      // if (C_csr == C_csr_corret) {
      //     cout << "Your answer is correct!" << endl;
      //     // PrintVector(C_csr.rowptr, converted.rowptr, C_csr.rows+1);
      // } else {
      //     cout << "Your answer is wrong!" << endl;
      //     // PrintVector(C_csr.colids, converted.colids, C_csr.nnz);
      //     // PrintVector(C_csr.rowptr, converted.rowptr, C_csr.rows+1);
      // }

      sec = (end - start);
      ave_sec += sec;
      ave_symb_sec += cur_toy_time.symb_time;
      ave_numc_sec += cur_toy_time.numc_time;

      if (i != ITERS - 1) {
        C_csr.make_empty();
      }
    }

    ave_sec /= ITERS;
    ave_symb_sec /= ITERS;
    ave_numc_sec /= ITERS;

    flops = (double)nfop / ave_sec;

    // printf(
    //     "Left matrix has %d nonzeros, right matrix has %d nonzeros, nrows %d\n",
    //     left.nnz, B_csr.nnz, A_csc.rows);
    // printf("OuterSpGEMM generated %d flops, returned with %d nonzeros. "
    //        "Compression ratio is %f\n",
    //        (nfop / 2), C_csr.nnz, (float)(nfop / 2) / (float)(C_csr.nnz));
    // printf("OuterSpGEMM with %3d threads computes C = A * B in %f [seconds] (%f [FLOPS])\n\n",
    //        tnum, ave_sec, mflops);

    // printf("dataset: %s\ttotal_time: %.9f\tflop_cnt: %ld\tperf: %.9f\touterspgemm\n", 
    //   argv[8], ave_sec, nfop, flops
    // );

    printf("\033[0;32m");
    printf("dataset: %s\tnum_trials: %d\tthread_num: %d\tnuma_mask: placeholder\t"
      "init_time: 1.0\tsymb_time: %.9f\tmgmt_time: 1.0\tnumc_time: %.9f\ttotal_time: %.9f\t"
      "flop_cnt: %ld\tnumc_perf: 1.0\tperf: %.9f\n",
      argv[8], ITERS, tnum, 
      ave_symb_sec, ave_numc_sec, ave_sec, 
      nfop, flops
    );
    printf("\033[0m");

    C_csr.make_empty();
  }

  // auto obj = SplitTuples<INDEXTYPE, VALUETYPE>(A_csc, 2);
  // cout << "Split -> nrows=" << obj.rows << " ncols=" << obj.cols << " nnz=" << obj.nnz <<endl;
  // cout << "Left nnz=" << obj.splitTuples[0].size() << "Right nnz=" <<obj.splitTuples[1].size() << endl;
  // auto left = CSC<INDEXTYPE, VALUETYPE>(obj.splitTuples[0], obj.rows, obj.cols, plus<INDEXTYPE>());
  // cout << left.rows << " " << left.cols << endl;

  // CSR<INDEXTYPE, VALUETYPE> C_upper;
  // OuterSpGEMM(left, B_csr, C_upper, atoi(argv[6]), atoi(argv[7]));

  A_csc.make_empty();
  B_csc.make_empty();
  B_csr.make_empty();

  return 0;
}
