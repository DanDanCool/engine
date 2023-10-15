#include "memory.h"
#include "simd.h"
#include <assert.h>
#include <stdlib.h>

#ifdef JOLLY_WIN32
#include <malloc.h>
#include <crtdbg.h>
#endif

namespace core {
	u32 align_size256(u32 size) {
		u32 count = (size - 1) / BLOCK_32 + 1;
		return count * BLOCK_32;
	}

	memptr alloc256(u32 size) {
		memptr ptr = {0};
		size = align_size256(size);

#ifdef JOLLY_WIN32
		ptr.data = (u8*)_aligned_malloc(size, DEFAULT_ALIGNMENT);
#else
		ptr.data = (u8*)aligned_alloc(DEFAULT_ALIGNMENT, size);
#endif
		ptr.size = size;
		zero256(ptr.data, (u32)ptr.size);
		return ptr;
	}

	void free256(void* ptr) {
#ifdef JOLLY_WIN32
		_aligned_free(ptr);
#else
		free(ptr);
#endif
	}

	memptr alloc8(u32 size) {
		memptr ptr = {0};
		ptr.size = size;
		ptr.data = (u8*)malloc(size);
		zero8(ptr.data, (u32)ptr.size);
		return ptr;
	}

	void free8(void* ptr) {
		free(ptr);
	}

	void copy256(u8* src, u8* dst, u32 bytes) {
		assert((bytes % BLOCK_32) == 0);
		u32 count = bytes / BLOCK_32;
		for (u32 i = 0; i < count; i++) {
			__m256i tmp = _mm256_load_si256((__m256i*)src);
			_mm256_store_si256((__m256i*)dst, tmp);
			dst += BLOCK_32;
			src += BLOCK_32;
		}
	}

	void zero256(u8* dst, u32 bytes) {
		set256(0, dst, bytes);
	}

	void set256(u32 src, u8* dst, u32 bytes) {
		assert((bytes % BLOCK_32) == 0);
		u32 count = bytes / BLOCK_32;
		const __m256i zero = _mm256_set1_epi32(src);
		for (u32 i = 0; i < count; i++) {
			_mm256_store_si256((__m256i*)dst, zero);
			dst += BLOCK_32;
		}
	}

	void copy8(u8* src, u8* dst, u32 bytes) {
		for (u32 i = 0; i < bytes; i++) {
			dst[i] = src[i];
		}
	}

	void zero8(u8* dst, u32 bytes) {
		for (u32 i = 0; i < bytes; i++) {
			dst[i] = 0;
		}
	}

	any::~any() {
		deleter(data);
	}
}
