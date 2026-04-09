#ifndef ALLOC_HPP
#define ALLOC_HPP


#include <iostream>
#ifdef _WINDOWS
#include <malloc.h>
#endif


template <typename T>
struct aligned_allocator {
	using value_type = T;
	aligned_allocator() {}
	aligned_allocator(const aligned_allocator&) {}
	template <typename U>
	aligned_allocator(const aligned_allocator<U>&) {}

	T* allocate(std::size_t num) {
		void* ptr = nullptr;
		#ifdef _WINDOWS
			ptr = _aligned_malloc(num * sizeof(T), 0x1000);
			if (ptr == nullptr) {
				std::cout << "Failed to allocate memory" << std::endl;
				exit(EXIT_FAILURE);
			}
		#else
			if (posix_memalign(&ptr, 0x1000, num * sizeof(T))) {
				throw std::bad_alloc();
			}
		#endif
		return reinterpret_cast<T*>(ptr);
	}
	void deallocate(T* p, std::size_t num) {
		#ifdef _WINDOWS
		_aligned_free(p);
		#else
		free(p);
		#endif
	}
};


#endif
