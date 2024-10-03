module;

#include "core.h"
#include <immintrin.h>

export module core.simd;
import core.types;

export namespace core {
	enum {
		BLOCK_32 = 32,
		BLOCK_64 = 64,
		BLOCK_128 = 128,
		BLOCK_256 = 256,
		BLOCK_512 = 512,
		BLOCK_1024 = 1024,
		BLOCK_2048 = 2048,
		BLOCK_4096 = 4096,
		BLOCK_8192 = 8192,

		DEFAULT_ALIGNMENT = BLOCK_32,
	};

	struct v256i {
		alignas(BLOCK_32) u64 data[4];
	};

	u32 align_size256(u32 size) {
		u32 count = (size - 1) / BLOCK_32 + 1;
		return count * BLOCK_32;
	}

	void copy256(ptr<u8> src, ptr<u8> dst, u32 bytes) {
		JOLLY_ASSERT((bytes % BLOCK_32) == 0, "bytes must be a multiple of 32");
		u32 count = bytes / BLOCK_32;
		for (u32 i = 0; i < count; i++) {
			__m256i tmp = _mm256_load_si256((ptr<__m256i>)src);
			_mm256_store_si256((ptr<__m256i>)dst, tmp);
			dst += BLOCK_32;
			src += BLOCK_32;
		}
	}

	void set256(u32 src, ptr<u8> dst, u32 bytes) {
		JOLLY_ASSERT((bytes % BLOCK_32) == 0, "bytes must be a multiple of 32");
		u32 count = bytes / BLOCK_32;
		const __m256i zero = _mm256_set1_epi32(src);
		for (u32 i = 0; i < count; i++) {
			_mm256_store_si256((ptr<__m256i>)dst, zero);
			dst += BLOCK_32;
		}
	}

	void zero256(ptr<u8> dst, u32 bytes) {
		set256(0, dst, bytes);
	}

	bool cmp256(ptr<u8> src, ptr<u8> dst, u32 bytes) {
		JOLLY_ASSERT((bytes % BLOCK_32) == 0, "bytes must be a multiple of 32");
		u32 count = bytes / BLOCK_32;
		for (u32 i = 0; i < count; i++) {
			__m256i x = _mm256_load_si256((ptr<__m256i>)src);
			__m256i y = _mm256_load_si256((ptr<__m256i>)dst);

			__m256i z = _mm256_cmpeq_epi8(x, y);
			int equal = _mm256_testc_si256(z, z);
			if (!equal) return false;

			src += BLOCK_32;
			dst += BLOCK_32;
		}

		return true;
	}

	bool cmp8(ptr<u8> src, ptr<u8> dst, u32 bytes) {
		for (u32 i = 0; i < bytes; i++) {
			bool equal = src[i] == dst[i];
			if (!equal) return false;
		}

		return true;
	}

	void copy8(ptr<u8> src, ptr<u8> dst, u32 bytes) {
		for (u32 i = 0; i < bytes; i++) {
			dst[i] = src[i];
		}
	}

	void zero8(ptr<u8> dst, u32 bytes) {
		for (u32 i = 0; i < bytes; i++) {
			dst[i] = 0;
		}
	}

	template <typename T, typename S>
	T cast(cref<S> val) {
		T dst;
		copy8((ptr<u8>)&val, (ptr<u8>)&dst, sizeof(dst));
		return dst;
	}
}
