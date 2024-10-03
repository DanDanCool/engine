module;

#include "core.h"

export module core.multi_vector;
import core.types;
import core.tuple;

export namespace core {
	constexpr u32 MULTI_VECTOR_DEFAULT_SIZE = BLOCK_32;

	template<typename... Ts>
	struct multi_vector {
		using this_type = multi_vector<Ts...>;
		using tuple_type = tuple<ptr<Ts>...>;
		using sequence_type = index_sequential<sizeof...(Ts)>;

		multi_vector()
		: data((ptr<Ts>)nullptr...), reserve(0), size(0) {}

		multi_vector(u32 sz)
		: data((ptr<Ts>)nullptr...), reserve(0), size(0) {
			sz = max<u32>(sz, MULTI_VECTOR_DEFAULT_SIZE);
			allocate(sz, sequence_type{});
		}

		multi_vector(fwd<this_type> other)
		: data((ptr<Ts>)nullptr...), reserve(0), size(0) {
			*this = forward_data(other);
		}

		~multi_vector() {
			destroy(sequence_type{});
			reserve = 0;
			size = 0;
		}

		ref<this_type> operator=(fwd<this_type> other) {
			data = forward_data(other.data);
			reserve = other.reserve;
			size = other.size;

			other.data = tuple_type{ (ptr<Ts>)nullptr... };
			other.reserve = 0;
			other.size = 0;
			return *this;
		}

		template<u32 I>
		ref<tuple_element_t<I, Ts...>> get(u32 idx) const {
			return data.get<I>()[idx];
		}

		void resize(u32 sz) {
			tuple_type old = forward_data(data);
			u32 count = reserve;
			allocate(sz, sequence_type{});
			_resize(old, count, sequence_type{});
		}

		void add(fwd<Ts>... vals) {
			if (reserve <= size) {
				resize(reserve * 2);
			}

			_add(size++, forward_data(vals)..., sequence_type{});
		}

		void del(u32 idx) {
			auto del_helper = []<u32... Indices>(ref<tuple_type> data, u32 idx, u32 last, index_sequence<Indices...>) {
				auto helper = []<u32 I>(ref<tuple_element_t<I, Ts...>> src, ref<tuple_element_t<I, Ts...>> dst, uint_constant<I>) {
					core::destroy(&dst);
					copy8((ptr<u8>)&src, (ptr<u8>)&dst, sizeof(tuple_element_t<I, Ts...>));
					zero8((ptr<u8>)&src, sizeof(tuple_element_t<I, Ts...>));
					return 0;
				};

				(helper(data.get<Indices>()[last], data.get<Indices>()[idx], uint_constant<Indices>{}), ...);
			};

			del_helper(data, idx, --size, sequence_type{});
		}

		template<u32... Indices>
		void allocate(u32 sz, index_sequence<Indices...>) {
			auto helper = []<typename T>(ref<ptr<T>> arg, u32 sz) {
				auto ptr = alloc256(sz * sizeof(T));
				arg = (ptr<T>)ptr.data;
				return 0;
			};

			(helper(data.get<Indices>(), sz), ...);
			reserve = sz;
		}

		template<u32... Indices>
		void destroy(index_sequence<Indices...>) {
			auto helper = []<typename T>(ref<ptr<T>> arg, u32 size) {
				if (!arg) return 0;
				for (int i : range(size)) {
					core::destroy(&arg[i]);
				}

				free256((ptr<void>)arg);
				arg = nullptr;
				return 0;
			};

			(helper(data.get<Indices>(), size), ...);
		}

		template <u32... Indices>
		void _resize(ref<tuple_type> src, u32 count, index_sequence<Indices...>) {
			auto helper = []<typename T>(ptr<T> src, ptr<T> dst, u32 count) {
				u32 bytes = (u32)(count * sizeof(T));
				copy256((ptr<u8>)src, (ptr<u8>)dst, align_size256(bytes));
				free256((ptr<u8>)src);
				return 0;
			};

			(helper(src.get<Indices>(), data.get<Indices>(), count), ...);
		};


		template <u32... Indices>
		void _add(u32 idx, fwd<Ts>... vals, index_sequence<Indices...>) {
			auto helper = []<u32 I>(ref<tuple_element_t<I, Ts...>> dst, fwd<tuple_element_t<I, Ts...>> val, uint_constant<I>) {
				dst = forward_data(val);
				return 0;
			};

			(helper(get<Indices>(idx), forward_data(vals), uint_constant<Indices>{}), ...);
		}

		tuple_type data;
		u32 reserve;
		u32 size;
	};
}
