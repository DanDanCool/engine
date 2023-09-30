#pragma once

#include "core.h"
#include "traits.h"

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

	memptr alloc256(u32 size);
	memptr alloc8(u32 size);

	memptr alloc256_dbg_win32_(u32 size, const char* fn, int ln);
	memptr alloc8_dbg_win32_(u32 size, const char* fn, int ln);

	void free256(void* ptr);
	void free8(void* ptr);

	void copy256(u8* src, u8* dst, u32 bytes);
	void zero256(u8* dst, u32 bytes);
	void copy8(u8* src, u8* dst, u32 bytes);
	void zero8(u8* dst, u32 bytes);

	template <typename T>
	struct ptr {
		using type = T;

		ptr() = default;

		// take ownership
		ptr(cref<ptr<T>> other) : data(other.data) {
			const_cast<ptr<T>&>(other).data = nullptr;
		}

		ref<ptr<T>> operator=(cref<ptr<T>> other) {
			data = other.data;
			const_cast<ptr<T>&>(other).data = nullptr;
			return *this;
		}

		template<typename... Args>
		ptr(Args&&... args) : data(nullptr) {
			data = (type*)alloc8(sizeof(type)).data;
			data = new (data) type(forward_data(args)...);
		}

		~ptr() {
			if (!data) return;
			destroy();
		}

		void destroy() {
			cleanup<is_destructible<type>::value>();
			free8(data);
			data = nullptr;
		}

		type* operator->() const {
			return data;
		}

		ref<type> ref() const {
			return *data;
		}

		type* data;

		private:
		template<bool> void cleanup() {};
		template<> void cleanup<true>() {
			data->~type();
		}
	};
}
