#include <iostream>
#include <vector>
#include <memory>

#include "utils.h"


extern int myallocator_numa_node_idx;

template <typename T>
class MyAllocator {
public:
	// Type definitions
	using size_type = size_t;
	using difference_type = ptrdiff_t;
	using pointer = T*;
	using const_pointer = const T*;
	using reference = T&;
	using const_reference = const T&;
	using value_type = T;

	// Convert an allocator<T> to allocator<U>
	template <typename U>
	struct rebind {
		using other = MyAllocator<U>;
	};

	MyAllocator() = default;

	template <typename U>
	MyAllocator(const MyAllocator<U>& other) {}

	// pointer allocate(size_type n, const void* hint = 0) {
	pointer allocate(size_type n) {
		std::cout << "Allocating " << n << " elements of type " << typeid(T).name() << std::endl;
		if (n == 0) {
			return nullptr;
		}
		if (n > static_cast<size_type>(-1) / sizeof(T)) {
			throw std::bad_alloc();
		}
		void* ptr = alloc_memory(n * sizeof(T), myallocator_numa_node_idx);

		if (!ptr) {
			throw std::bad_alloc();
		}
		return static_cast<pointer>(ptr);
	}

	void deallocate(pointer p, size_type n) {
		// std::cout << "Deallocating " << n << " elements of type " << typeid(T).name() << std::endl;
		dealloc_memory(p, n * sizeof(T));
	}
};


template <typename T>
class MyCXLAllocator : public MyAllocator<T> {
public:
	using typename MyAllocator<T>::pointer;
	using typename MyAllocator<T>::size_type;

	// pointer allocate(size_type n, const void* hint = 0) {
	pointer allocate(size_type n) {
		void *ptr = alloc_memory(n * sizeof(T), LAST_NUMA_NODE_IDX);
		return static_cast<pointer>(ptr);
	}
};


template <typename T, typename U>
bool operator==(const MyAllocator<T>&, const MyAllocator<U>&) { return true; }

template <typename T, typename U>
bool operator!=(const MyAllocator<T>&, const MyAllocator<U>&) { return false; }