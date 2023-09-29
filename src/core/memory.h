#pragma once

#include "core.h"

namespace core {
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

	struct memptr {
		u8* data;
		u64 size;
	};

	u32 align_size256(u32 size);

#ifdef JOLLY_DEBUG_HEAP
#ifdef JOLLY_WIN32
#define alloc256(sz) core::alloc256_dbg_win32_(sz, __FILE__, __LINE__)
#define alloc8(sz) core::alloc8_dbg_win32_(sz, __FILE__, __LINE__)
#define free256(ptr) core::free256_dbg_win32_(ptr)
#define free8(ptr) core::free8_dbg_win32_(ptr)

	void free256_dbg_win32_(void* ptr);
	void free8_dbg_win32_(void* ptr);
#endif
#else
#define alloc256(sz) core::alloc256_(sz)
#define alloc8(sz) core::alloc8_(sz)
#define free256(ptr) core::free256_(ptr)
#define free8(ptr) core::free8_(ptr)
#endif

	memptr alloc256_(u32 size);
	memptr alloc8_(u32 size);

	memptr alloc256_dbg_win32_(u32 size, const char* fn, int ln);
	memptr alloc8_dbg_win32_(u32 size, const char* fn, int ln);

	void free256_(void* ptr);
	void free8_(void* ptr);

	void copy256(u8* src, u8* dst, u32 bytes);
	void zero256(u8* dst, u32 bytes);
	void copy8(u8* src, u8* dst, u32 bytes);
	void zero8(u8* dst, u32 bytes);
}
