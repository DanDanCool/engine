#pragma once

#include "core.h"
#include "traits.h"

#include <new>

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

	template <typename T> struct ptr;

	template <typename T>
	struct ptr_base {
		using type = T;

		ptr_base() = default;

		// take ownership
		ptr_base(cref<ptr_base<type>> other) : data(other.data) {
			const_cast<ptr_base<type>&>(other) = nullptr;
		}

		ptr_base(ptr_base<type>&& other) : data(other.data) {
			other.data = nullptr;
		}

		ptr_base(type* in) : data(in) {}

		ref<ptr_base<type>> operator=(cref<ptr_base<type>> other) {
			data = other.data;
			const_cast<ptr_base<type>&>(other) = nullptr;
			return *this;
		}

		ref<ptr_base<type>> operator=(ptr_base<type>&& other) {
			data = other.data;
			other.data = nullptr;
			return *this;
		}

		ref<ptr_base<type>> operator=(type* in) {
			data = in;
			return *this;
		}

		~ptr_base() {
			if (!data) return;
			destroy();
		}

		void destroy() {
			cleanup<is_destructible<type>::value>();
			free8(data);
			data = nullptr;
		}

		operator type*() const {
			return data;
		}

		type* operator->() const {
			return data;
		}

		template <typename S>
		ptr<S> cast() {
			ptr<S> p((S*)data);
			data = nullptr;
			return p;
		}

		type* data;

		private:
		template<bool> void cleanup() {}
		template<> void cleanup<true>() {
			data->~type();
		}
	};

	template <typename T>
	struct ptr : public ptr_base<T> {
		using ptr_base::ptr_base;
		using type = T;

		ref<type> ref() const {
			return *ptr_base::data;
		}
	};

	template<>
	struct ptr<void> : public ptr_base<void> {
		using ptr_base::ptr_base;

		template<typename S>
		ref<S> ref() const {
			return *(S*)ptr_base::data;
		}
	};

	template<typename T, typename... Args>
	ptr<T> ptr_create(Args&&... args) {
		T* data = (T*)alloc8(sizeof(T)).data;
		data = new (data) T(forward_data(args)...);
		return move_data(ptr<T>(data));
	}

	template<u32 N>
	struct buffer_base {
		static constexpr u32 size = N;
		buffer_base() : data(), index(0) {
			data = alloc256(N).data;
		}

		buffer_base(u8* buf) : data(buf), index(0) {}

		~buffer_base() {
			if (!data) return;
			free256(data);
			data = nullptr;
		}

		u32 write(u8 character) {
			if (index >= size) return 0;
			data[index++] = character;
			return 1;
		}

		memptr write(memptr buf) {
			u64 bytes = min(buf.size, size - index);
			copy8(buf.data, data + index, (u32)bytes);

			index += bytes;
			return memptr{ buf.data + bytes, buf.size - bytes };
		}

		memptr read(memptr buf) {
			u64 bytes = min(buf.size, index);
			copy8(data, buf.data, bytes);

			index -= bytes;
			copy8(data + bytes, data, index);
			zero8(data + index, bytes);

			return memptr{ buf.data + bytes, buf.size - bytes };
		}

		void flush() {
			zero256(data, size);
			index = 0;
		}

		ptr<u8> data;
		u64 index;
	};

	using buffer = buffer_base<BLOCK_4096>;
}
