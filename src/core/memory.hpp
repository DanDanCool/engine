module;

#include "core.h"
#include <stdlib.h>

#ifdef JOLLY_WIN32
#include <malloc.h>
#include <crtdbg.h>
#endif

#include <new>

export module core.memory;
import core.types;
import core.simd;
import core.traits;
import core.iterator;

export namespace core {
	struct membuf {
		u8* data;
		u64 size;
	};

	membuf alloc256_dbg_win32_(u32 size, const char* fn, int ln);
	membuf alloc8_dbg_win32_(u32 size, const char* fn, int ln);

	membuf alloc256(u32 size) {
		membuf ptr = {0};
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

	membuf alloc8(u32 size) {
		membuf ptr = {0};
		ptr.size = size;
		ptr.data = (u8*)malloc(size);
		zero8(ptr.data, (u32)ptr.size);
		return ptr;
	}

	void free8(void* ptr) {
		free(ptr);
	}

	template <typename T> struct mem;

	template <typename T>
	struct mem_base {
		using type = T;
		using this_type = mem_base<T>;

		mem_base() = default;

		mem_base(this_type&& other)
		: data(nullptr) {
			*this = forward_data(other);
		}

		mem_base(type* in)
		: data(in) {}

		ref<this_type> operator=(this_type&& other) {
			data = other.data;
			other.data = nullptr;
			return *this;
		}

		ref<this_type> operator=(type* in) {
			data = in;
			return *this;
		}

		~mem_base() {
			if (!data) return;
			destroy();
		}

		void destroy() {
			core::destroy(data);
			free8(data);
			data = nullptr;
		}

		operator type*() const {
			return data;
		}

		template <typename S>
		mem<S> cast() {
			mem<S> p((S*)data);
			data = nullptr;
			return forward_data(p);
		}

		type* data;
	};

	template <typename T>
	struct mem : public mem_base<T> {
		using type = T;
		using parent_type = mem_base<T>;
		using parent_type::parent_type;

		ref<type> get() const {
			return *parent_type::data;
		}

		ref<type> operator[](u32 idx) const {
			return parent_type::data[idx];
		}

		type* operator->() const {
			return parent_type::data;
		}
	};

	template<>
	struct mem<void> : public mem_base<void> {
		using parent_type = mem_base<void>;
		using parent_type::parent_type;

		template<typename S>
		cref<S> get() const {
			return *(S*)parent_type::data;
		}

		template<typename S>
		ref<S> get() {
			return *(S*)parent_type::data;
		}
	};

	struct handle {
		handle()
		: _data(nullptr) {}

		handle(void* in)
		: _data(nullptr) {
			*this = in;
		}

		handle(cref<handle> other)
		: _data(nullptr) {
			*this = other;
		}

		ref<handle> operator=(void* in) {
			_data = in;
			return *this;
		}

		ref<handle> operator=(cref<handle> other) {
			*this = other._data.data;
			return *this;
		}

		~handle() {
			_data = nullptr;
		}

		template<typename T>
		cref<T> get() const {
			return _data.get();
		}

		ptr<void> data() const {
			return _data.data;
		}

		operator bool() const {
			return data();
		}

		mem<void> _data;
	};

	template<typename T, typename... Args>
	mem<T> mem_create(Args&&... args) {
		T* data = (T*)alloc8(sizeof(T)).data;
		data = new (data) T(forward_data(args)...);
		return mem<T>(data);
	}

	struct any {
		typedef void (*pfn_deleter)(cref<mem<void>> data);
		any() = default;

		template<typename T>
		any(T* in)
		: data(nullptr), deleter(nullptr) {
			*this = in;
		}

		template<typename T>
		any(mem<T>&& in)
		: data(nullptr), deleter(nullptr) {
			*this = forward_data(in);
		}

		any(any&& in)
		: data(nullptr), deleter(nullptr) {
			*this = forward_data(in);
		}

		~any() {
			if (!data) return;
			deleter(data);
		}

		template<typename T>
		ref<any> operator=(T* in) {
			data = (void*)in;
			deleter = destroy<T>;
			return *this;
		}

		template<typename T>
		ref<any> operator=(mem<T>&& in) {
			*this = in.data;
			in.data = nullptr;
			return *this;
		}

		ref<any> operator=(any&& in) {
			data = forward_data(in.data);
			deleter = in.deleter;
			return *this;
		}

		template <typename T>
		cref<T> get() const {
			return data.get<T>();
		}

		template <typename T>
		ref<T> get() {
			return data.get<T>();
		}

		template<typename T>
		static void destroy(cref<mem<void>> data) {
			core::destroy((ptr<T>)data.data);
		}

		mem<void> data;
		pfn_deleter deleter;
	};

	template<u32 N>
	struct buffer_base {
		static constexpr u32 size = N;
		buffer_base()
		: data(), index(0) {
			data = alloc256(N).data;
		}

		buffer_base(u8* buf)
		: data(buf), index(0) {}

		buffer_base(buffer_base&& other)
		: data(), index(0) {
			*this = other;
		}

		~buffer_base() {
			if (!data) return;
			free256(data);
			data = nullptr;
		}

		ref<buffer_base> operator=(buffer_base&& other) {
			index = other.index;
			data = forward_data(other.data);
			return *this;
		}

		u32 write(u8 character) {
			if (index >= size) return 0;
			u32 idx = (u32)index++;
			data[idx] = character;
			return 1;
		}

		u32 write(u8 character, u32 count) {
			for (i32 i : range(count)) {
				if (index >= size) return i;
				u32 idx = (u32)index++;
				data[idx] = character;
			}

			return count;
		}

		membuf write(membuf buf) {
			u64 bytes = min(buf.size, size - index);
			copy8(buf.data, data + index, (u32)bytes);

			index += bytes;
			return membuf{ buf.data + bytes, buf.size - bytes };
		}

		membuf read(membuf buf) {
			u64 bytes = min(buf.size, index);
			copy8(data, buf.data, bytes);

			index -= bytes;
			copy8(data + bytes, data, index);
			zero8(data + index, bytes);

			return membuf{ buf.data + bytes, buf.size - bytes };
		}

		u64 rem() {
			return size - index;
		}

		void flush() {
			zero256(data, size);
			index = 0;
		}

		ref<u8> operator[](u64 idx) {
			JOLLY_CORE_ASSERT(idx < size);
			return data[(u32)idx];
		}

		mem<u8> data;
		u64 index;
	};

	using buffer = buffer_base<BLOCK_4096>;
}
