cmake_minimum_required(VERSION 3.16.3)
project(ujmm_hmem)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# C flags
set(CMAKE_C_FLAGS "-O3 -march=native -Wall -Wextra")
# set(CMAKE_C_FLAGS "-O3 -march=native -Wall -Wextra -g")
# set(CMAKE_C_FLAGS "-O3 -march=native -Wall -Wextra -g -fsanitize=address")
# set(CMAKE_C_FLAGS "-O0 -march=native -Wall -Wextra -ggdb3")
# set(CMAKE_C_FLAGS "-O0 -march=native -Wall -Wextra -ggdb3 -fsanitize=address")

# C++ flags
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")


# add_definitions(-DIN_DEBUG)
#[[
#ifdef IN_DEBUG
	printf("%s:%d\n", __FILE__, __LINE__);
#endif /* IN_DEBUG */
]]


add_definitions(-DPROJECT_PATH="${CMAKE_CURRENT_SOURCE_DIR}")

set(SRC_PATH "src")
set(BIN_PATH "bin")
set(INC_PATH "src/inc")
# set(AB_PATH "src/ab")
set(MM_PATH "src/mm")
set(PAPI_PATH "src/papi")
set(PAPI_INC_PATH "/home/huang/papi/src/install/include")
set(PAPI_LIB_PATH "/home/huang/papi/src/install/lib")

set(NUMA_INC_PATH "/usr/local/include")
set(NUMA_LIB_PATH "/usr/local/lib")

set(DIM3_PATH "src/dim3")

add_definitions(-DCHAR_BUFFER_LEN=256)

cmake_host_system_information(RESULT _host_name QUERY HOSTNAME)
message("-- _host_name VARIABLE IS SET TO: " ${_host_name})


# machine-level configurations
add_definitions(-DCORE_NUM=64)
add_definitions(-DSOCKET_CORE_NUM=32)
# add_definitions(-DCFG_PATH="${CMAKE_CURRENT_SOURCE_DIR}/config/mc/mxc0.cfg")
add_definitions(-DCFG_PATH="${CMAKE_CURRENT_SOURCE_DIR}/config/mc/mxc.cfg")
# add_definitions(-DSOCKET_CORE_NUM=8)
# add_definitions(-DCFG_PATH="${CMAKE_CURRENT_SOURCE_DIR}/config/mc/mxc-snc.cfg")
set(IPMWATCH_INC_PATH "/home/huang/intel-pmwatch-3.2.1/src/inc")
set(MKLROOT_PATH "/opt/intel/oneapi/2024.0")
set(VTUNE_INC_PATH "/opt/intel/oneapi/vtune/2024.0/include/")
set(VTUNE_LIB_PATH "/opt/intel/oneapi/vtune/2024.0/lib64/")
set(LINUX_HEADERS_PATH "/home/huang/workspace/numactl/include")
set(LINUX_LIB_PATH "/lib/modules/6.11.0/kernel/lib")
# set(SUITESPARSE_INC_DIR "/home/huang/SuiteSparse/self_defined_installation/include/suitesparse")
# set(SUITESPARSE_LIB_DIR "/home/huang/SuiteSparse/build/GraphBLAS")



foreach(HEADER_PATH IN LISTS LINUX_HEADERS_PATH)
	include_directories(${HEADER_PATH})
endforeach()

foreach(LIB_PATH IN LISTS LINUX_LIB_PATH)
	link_directories(${LIB_PATH})
	list(APPEND CMAKE_EXE_LINKER_FLAGS "-Wl,-rpath,${LIB_PATH}")
endforeach()


if(CFG_PATH)
	add_definitions(-DCFG_PATH=\"${CFG_PATH}\")
endif()


if(USE_HUGE)
	add_definitions(-DUSE_HUGE)
endif()

# Declare options with default values
option(USE_INTERLEAVING "Enable interleaving" OFF)
option(USE_WEIGHTED_INTERLEAVING "Enable weighted interleaving" OFF)
if(USE_INTERLEAVING)
	add_definitions(-DUSE_INTERLEAVING)
elseif(USE_WEIGHTED_INTERLEAVING)
	add_definitions(-DUSE_WEIGHTED_INTERLEAVING)
endif()

if(USE_NUMA)
	add_definitions(-DUSE_NUMA)
endif()

# NUMA_MASK_VAL starts from 1
if(NUMA_MASK_VAL)
	add_definitions(-DNUMA_MASK_VAL=${NUMA_MASK_VAL})
else()
	# use local numa node by default
	add_definitions(-DNUMA_MASK_VAL=1)
endif()

if (MEMSV)
	add_definitions(-DMEMSV)
endif()

if (MEMSVB)
	add_definitions(-DMEMSVB)
endif()

if (MEM_MON)
	add_definitions(-DMEM_MON)
endif()

if(USE_NVM)
	add_definitions(-DUSE_NVM)
	add_definitions(-DNVM_LOCAL_PREFIX="/pmemfs1/huang/hmem-io")
	add_definitions(-DNVM_REMOTE_PREFIX="/pmemfs0/huang/hmem-io")
else()
	add_definitions(-DUSE_CXL)
endif()


add_definitions(-DDISK_PREFIX="${CMAKE_CURRENT_SOURCE_DIR}/work")
add_definitions(-DRMAT_DIR="dataset/rmat")
add_definitions(-DTAMU_DIR="dataset/tamu")
add_definitions(-DSUITESPARSE_DIR="dataset/suitesparse")
add_definitions(-DGROUPBY_DIR="dataset/groupby-sorted")
add_definitions(-DSSJ_DIR="dataset/ssj-sorted")


if(IN_TAMU)
	add_definitions(-DIN_TAMU)
endif()

if(IN_GROUPBY)
	add_definitions(-DIN_GROUPBY)
endif()

if(IN_SSJ)
	add_definitions(-DIN_SSJ)
endif()


# We force NUMA_NODE_IDX start from 1, i.e., NUMA_NODE_IDX = numa_node_id + 1
if(NUMA_NODE_IDX)
	add_definitions(-DNUMA_NODE_IDX=${NUMA_NODE_IDX})
else()
	# add_definitions(-DNUMA_NODE_IDX=2)
	add_definitions(-DNUMA_NODE_IDX=2)
endif()

if(LOCAL_NUMA_NODE_IDX)
	add_definitions(-DLOCAL_NUMA_NODE_IDX=${LOCAL_NUMA_NODE_IDX})
else()
	add_definitions(-DLOCAL_NUMA_NODE_IDX=2)
	# add_definitions(-DLOCAL_NUMA_NODE_IDX=8)
endif()

add_definitions(-DLAST_NUMA_NODE_IDX=3)
add_definitions(-DCXL_NUMA_NODE_IDX=3)

if(THREAD_NUM)
	add_definitions(-DTHREAD_NUM=${THREAD_NUM})
else()
	add_definitions(-DTHREAD_NUM=32)
endif()

if(THREAD_NUM_MAJOR)
	add_definitions(-DTHREAD_NUM_MAJOR=${THREAD_NUM_MAJOR})
else()
	add_definitions(-DTHREAD_NUM_MAJOR=6)
endif()

if(THREAD_NUM_MINOR)
	add_definitions(-DTHREAD_NUM_MINOR=${THREAD_NUM_MINOR})
else()
	add_definitions(-DTHREAD_NUM_MINOR=2)
endif()

if(USE_HYPERTHREADING)
	add_definitions(-DUSE_HYPERTHREADING)
endif()

if(CSRC_WRITE_OPTIMIZED)
	add_definitions(-DCSRC_WRITE_OPTIMIZED)
endif()

if(CSRC_PREPOPULATED)
	add_definitions(-DCSRC_PREPOPULATED)
endif()

if(MIN_HT_S)
	add_definitions(-DMIN_HT_S=${MIN_HT_S})
else()
	add_definitions(-DMIN_HT_S=8)
endif()

if(HASH_CONSTANT)
	add_definitions(-DHASH_CONSTANT=${HASH_CONSTANT})
else()
	add_definitions(-DHASH_CONSTANT=2654435761)	# for 32-bit
	# add_definitions(-DHASH_CONSTANT=11400714819323198485)	# for 64-bit
	# add_definitions(-DHASH_CONSTANT=107L)	# mtspgemmlib
	# add_definitions(-DHASH_CONSTANT=3L)
	# add_definitions(-DHASH_CONSTANT=1L)
endif()



if (AB_HIGH_THREAD_NUM)
	add_definitions(-DAB_HIGH_THREAD_NUM=${AB_HIGH_THREAD_NUM})
else()
	add_definitions(-DAB_HIGH_THREAD_NUM=4)
endif()
if(AB_HIGH_THREAD_NUM GREATER THREAD_NUM)
	message(FATAL_ERROR "AB_HIGH_THREAD_NUM (${AB_HIGH_THREAD_NUM}) cannot be greater than THREAD_NUM (${THREAD_NUM})")
endif()


if(AB_SIZE)
	add_definitions(-DAB_SIZE=${AB_SIZE}L)
else()
	add_definitions(-DAB_SIZE=524288L) # 512K
endif()

if(AB_MIN_NUM)
	add_definitions(-DAB_MIN_NUM=${AB_MIN_NUM}L)
else()
	add_definitions(-DAB_MIN_NUM=2048L) # 512K
endif()

if (AB_LPT)
	add_definitions(-DAB_LPT)
endif()

if (AB_SIMP)
	add_definitions(-DAB_SIMP)
endif()

if(AB_PRT)
	add_definitions(-DAB_PRT=${AB_PRT})
else()
	add_definitions(-DAB_PRT=0.80)
endif()

if(AB_MAXFLP)
	add_definitions(-DAB_MAXFLP)
endif()

if(AB_DNR)
	add_definitions(-DAB_DNR=true)
endif()

if(AB_HYOPT)
	add_definitions(-DAB_HYOPT)
endif()

if (AB_ACC_THRES)
	add_definitions(-DAB_ACC_THRES=${AB_ACC_THRES})
else()
	add_definitions(-DAB_ACC_THRES=5.00)
endif()

if(AB_SYMB_THRES)
	add_definitions(-DAB_SYMB_THRES=${AB_SYMB_THRES})
else()
	add_definitions(-DAB_SYMB_THRES=66.0)
endif()

if(AB_NUMC_THRES)
	add_definitions(-DAB_NUMC_THRES=${AB_NUMC_THRES})
else()
	add_definitions(-DAB_NUMC_THRES=1.5)
endif()


if(DAHA)
	add_definitions(-DDAHA)
endif()

if(DAHA_REAR)
	add_definitions(-DDAHA_REAR)
endif()

if(DAHA_QUANTILE)
	add_definitions(-DDAHA_QUANTILE=${DAHA_QUANTILE})
else()
	add_definitions(-DDAHA_QUANTILE=0.90)
endif()

if(DAHA_SYMB_THRES)
	add_definitions(-DDAHA_SYMB_THRES=${DAHA_SYMB_THRES}L)
else()
	add_definitions(-DDAHA_SYMB_THRES=8000000L) # default: 8M
endif()

if(DAHA_NUMC_THRES)
	add_definitions(-DDAHA_NUMC_THRES=${DAHA_NUMC_THRES}L)
else()
	add_definitions(-DDAHA_NUMC_THRES=800000L) # default: 800K
endif()

if(DAHA_SPMV_THRES)
	add_definitions(-DDAHA_SPMV_THRES=${DAHA_SPMV_THRES})
else()
	add_definitions(-DDAHA_SPMV_THRES=800000L) # default: 800K
endif()

if(DAHA_SPMM_THRES)
	add_definitions(-DDAHA_SPMM_THRES=${DAHA_SPMM_THRES})
else()
	add_definitions(-DDAHA_SPMM_THRES=800000L) # default: 800K
endif()

if(DN_MIN)
	add_definitions(-DDN_MIN=${DN_MIN}L)
else()
	add_definitions(-DDN_MIN=1L)	
endif()

if(GUD_ALPHA)
	add_definitions(-DGUD_ALPHA=${GUD_ALPHA})
else()
	add_definitions(-DGUD_ALPHA=0.5)
endif()

if(GUD_MIN)
	add_definitions(-DGUD_MIN=${GUD_MIN}L)
else()
	add_definitions(-DGUD_MIN=1L)
endif()

if(HYB_THRES)
	add_definitions(-DHYB_THRES=${HYB_THRES})
else()
	add_definitions(-DHYB_THRES=0.8)
endif()

if(CF_MARK_0)
	add_definitions(-DCF_MARK_0=${CF_MARK_0})
else()
	add_definitions(-DCF_MARK_0=1.1)
endif()

if(CF_MARK_1)
	add_definitions(-DCF_MARK_1=${CF_MARK_1})
else()
	add_definitions(-DCF_MARK_1=2.1)
endif()

if(RLRB_THLD)
	add_definitions(-DRLRB_THLD=${RLRB_THLD})
else()
	add_definitions(-DRLRB_THLD=0.0001)
endif()

if(DRAM_RATIO)
	add_definitions(-DDRAM_RATIO=${DRAM_RATIO})
else()
	add_definitions(-DDRAM_RATIO=10)
endif()

if(CXL_RATIO)
	add_definitions(-DCXL_RATIO=${CXL_RATIO})
else()
	add_definitions(-DCXL_RATIO=7)
endif()

if(MKL_SORT)
	add_definitions(-DMKL_SORT)
endif()

# MKL_ENHANCE: Enforce ordering for the csr_a and csr_b arrays.
if(MKL_ENHANCE)
	add_definitions(-DMKL_ENHANCE)
endif()

if(MKL_DENSE_VAL)
	add_definitions(-DMKL_DENSE_VAL=${MKL_DENSE_VAL})
else()
	add_definitions(-DMKL_DENSE_VAL=3.0)
endif()

if(SPMM_B_K)
	add_definitions(-DSPMM_B_K=${SPMM_B_K})
else()
	add_definitions(-DSPMM_B_K=64)
endif()

if(RUN_NUM)
	add_definitions(-DRUN_NUM=${RUN_NUM})
else()
	add_definitions(-DRUN_NUM=50)
endif()

if(USE_PAPI)
	add_definitions(-DUSE_PAPI)
endif()

if(IN_STATS)
	add_definitions(-DIN_STATS)
endif()

if(IN_VERIFY)
	add_definitions(-DIN_VERIFY)
endif()

if(IN_INSP)
	add_definitions(-DIN_INSP)
endif()

if(IN_DEBUG)
	add_definitions(-DIN_DEBUG)
endif()

if(IN_EXAMINE)
	add_definitions(-DIN_EXAMINE)
endif()

include_directories(${VTUNE_INC_PATH})
link_directories(${VTUNE_LIB_PATH})

add_subdirectory(${INC_PATH})
link_libraries(Inc)

# add_subdirectory(${AB_PATH})
# link_libraries(Ab)

add_subdirectory(${MM_PATH})
link_libraries(Mm)

include_directories(${NUMA_INC_PATH})
link_directories(${NUMA_LIB_PATH})

include_directories(${PAPI_INC_PATH})
add_subdirectory(${PAPI_PATH})
link_directories(${PAPI_LIB_PATH})
link_libraries(Papi -lpapi)




add_executable(rmat_ones ${SRC_PATH}/rmat_ones.c)
set_target_properties(rmat_ones PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${BIN_PATH})

add_executable(rmat_load ${SRC_PATH}/rmat_load.c)
set_target_properties(rmat_load PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${BIN_PATH})

add_executable(suitesparse_ones ${SRC_PATH}/suitesparse_ones.c)
set_target_properties(suitesparse_ones PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${BIN_PATH})

add_executable(suitesparse_load ${SRC_PATH}/suitesparse_load.c)
set_target_properties(suitesparse_load PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${BIN_PATH})

add_executable(tamu_ones ${SRC_PATH}/tamu_ones.c)
set_target_properties(tamu_ones PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${BIN_PATH})

add_executable(tamu_load ${SRC_PATH}/tamu_load.c)
set_target_properties(tamu_load PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${BIN_PATH})


add_executable(mkl_dcsrmultcsr ${SRC_PATH}/mkl_dcsrmultcsr.c)
set_target_properties(mkl_dcsrmultcsr PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${BIN_PATH})
target_include_directories(mkl_dcsrmultcsr PRIVATE ${MKLROOT_PATH}/include)
target_link_directories(mkl_dcsrmultcsr PRIVATE ${MKLROOT_PATH}/lib/intel64)
target_compile_definitions(mkl_dcsrmultcsr PRIVATE MKL_ILP64)
target_compile_options(mkl_dcsrmultcsr PRIVATE -m64)
target_link_libraries(mkl_dcsrmultcsr PRIVATE
	-Wl,-rpath,${MKLROOT_PATH}/lib/intel64
	-lmkl_intel_ilp64
	-lmkl_gnu_thread
	-lmkl_core
	-lgomp
	-lpthread
	-lm
	-ldl
)

add_executable(mkl_sparse_d_mm ${SRC_PATH}/mkl_sparse_d_mm.c)
set_target_properties(mkl_sparse_d_mm PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${BIN_PATH})
target_include_directories(mkl_sparse_d_mm PRIVATE ${MKLROOT_PATH}/include)
target_link_directories(mkl_sparse_d_mm PRIVATE ${MKLROOT_PATH}/lib/intel64)
target_compile_definitions(mkl_sparse_d_mm PRIVATE MKL_ILP64)
target_compile_options(mkl_sparse_d_mm PRIVATE -m64 -fopenmp)
target_link_libraries(mkl_sparse_d_mm PRIVATE
	-Wl,-rpath,${MKLROOT_PATH}/lib/intel64
	-lmkl_intel_ilp64
	-lmkl_gnu_thread
	-lmkl_core
	-lgomp
	-lpthread
	-lm
	-ldl
)

add_executable(mkl_cspblas_dcsrgemv ${SRC_PATH}/mkl_cspblas_dcsrgemv.c)
set_target_properties(mkl_cspblas_dcsrgemv PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${BIN_PATH})
target_include_directories(mkl_cspblas_dcsrgemv PRIVATE ${MKLROOT_PATH}/include)
target_link_directories(mkl_cspblas_dcsrgemv PRIVATE ${MKLROOT_PATH}/lib/intel64)
target_compile_definitions(mkl_cspblas_dcsrgemv PRIVATE MKL_ILP64)
target_compile_options(mkl_cspblas_dcsrgemv PRIVATE -m64 -fopenmp)
target_link_libraries(mkl_cspblas_dcsrgemv PRIVATE
	-Wl,-rpath,${MKLROOT_PATH}/lib/intel64
	-lmkl_intel_ilp64
	-lmkl_gnu_thread
	-lmkl_core
	-lgomp
	-lpthread
	-lm
	-ldl
)

add_executable(heapspgemm ${SRC_PATH}/heapspgemm.c)
set_target_properties(heapspgemm PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${BIN_PATH})

add_executable(hserscspgemm ${SRC_PATH}/hserscspgemm.c)
set_target_properties(hserscspgemm PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${BIN_PATH})

add_executable(hashspgemm ${SRC_PATH}/hashspgemm.c)
set_target_properties(hashspgemm PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${BIN_PATH})

add_executable(dnr_hashspgemm ${SRC_PATH}/dnr_hashspgemm.c)
set_target_properties(dnr_hashspgemm PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${BIN_PATH})

find_package(OpenMP REQUIRED)
add_executable(omphashspgemm ${SRC_PATH}/omphashspgemm.c)
set_target_properties(omphashspgemm PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${BIN_PATH})
set_target_properties(omphashspgemm PROPERTIES LINK_FLAGS "-fopenmp")
target_link_libraries(omphashspgemm PUBLIC OpenMP::OpenMP_C)

add_executable(toy ${SRC_PATH}/toy.c)
set_target_properties(toy PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${BIN_PATH})
set_target_properties(toy PROPERTIES LINK_FLAGS "-fopenmp")
target_link_libraries(toy PUBLIC OpenMP::OpenMP_C)

