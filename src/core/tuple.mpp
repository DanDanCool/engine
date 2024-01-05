module;

#include "core.h"

export module core.tuple;
import core.memory;
import core.traits;
import core.simd;

export namespace core {
	template <typename T1, typename T2>
	struct pair {
		pair() = default;
		pair(cref<T1> a, cref<T2> b)
			: one(a), two(b) {}

		~pair() = default;

		T1 one;
		T2 two;
	};

	template<typename T, u32 N>
	struct array {
		static constexpr u32 size = N;
		using type = T;

		array() : data(), index(0) {
			zero8((u8*)data, sizeof(data));
		}

		void add(cref<type> val) {
			data[index++] = val;
		}

		ref<type> operator[](u32 idx) {
			return data[idx];
		}

		type data[size];
		u32 index;
	};

	template <u32... S>
	struct index_sequence {
		using type = index_sequence;
	};

#ifdef _MSC_VER
	template <u32 N>
	using index_sequential = __make_integer_seq<index_sequence, u32, N>;
#endif

	template<u32 N, typename T>
	struct tuple_node {
		using type = T;

		tuple_node(type&& arg)
		: data(move_data(arg)) {

		}

		type data;
	};

	template <typename Indices, typename... Ts>
	struct _tuple_data {
		static_assert(false); // this should never be instantiated
	};

	template <u32... Indices, typename... Ts>
	struct _tuple_data<index_sequence<Indices...>, Ts...> : public tuple_node<Indices, Ts>... {
		tuple_data(Ts&& args...)
		: tuple_node<Indices, Ts>(forward_data(args))... {

		}
	};

	template <typename... Ts>
	using tuple_data = _tuple_data<index_sequential<sizeof...(Ts)>, Ts...>;

	template<u32 I, typename T, typename... Ts>
	struct tuple_element {
		typedef tuple_element<I - 1, Ts>::type type;
	};

	template<typename T, typename... Ts>
	struct tuple_element<0, T, Ts...> {
		typedef T type;
	};

	template<u32 I, typename... Ts>
	using tuple_element_t = typename tuple_element<I, Ts...>::type;

	template<u32 I, typename... Ts>
	tuple_element_t<I, Ts...>& tuple_get(tuple_data<Ts...> data) {
		using node = tuple_node<I, tuple_element_t<I, Ts...>>;
		return static_cast<node&>(data).data;
	}

	template<typename... Ts>
	struct tuple {
		using this_type = tuple<Ts...>;
		using sequence_type = index_sequential<sizeof...(Ts)>;

		tuple(Ts&& args...)
		: data(forward_data(args)...) {

		}

		tuple(this_type&& other)
		: data() {
			*this = forward_data(other);
		}

		ref<this_type> operator=(this_type&& other) {
			_move(forward_data(other), sequence_type{});
			return *this;
		}

		template<u32... Indices>
		void _move(this_type&& other, index_sequence<Indices...>) {
			swallow(_move_helper<Indices>(forward_data(other)));
		}

		template <u32 I>
		int _move_helper(this_type&& other) {
			get<I>() = forward_data(other.get<I>());
			return 0;
		}

		template<u32 I>
		auto& get() {
			return tuple_get<I>(data);
		}

		u32 size() {
			return sizeof...(Ts);
		}

		tuple_data<Ts...> data;
	};

	constexpr u32 MULTI_VECTOR_DEFAULT_SIZE = BLOCK_32;

	template<typename... Ts>
	struct multi_vector {
		using this_type = multi_vector<Ts...>;
		using tuple_type = tuple<Ts*...>;
		using sequence_type = index_sequential<sizeof...(Ts)>;

		multi_vector() = default;
		multi_vector(u32 sz)
		: data((Ts*)nullptr...), reserve(0), size(0) {
			sz = max<u32>(sz, MULTI_VECTOR_DEFAULT_SIZE);
			allocate(sz, sequence_type{});
		}

		multi_vector(this_type&& other)
		: data((Ts*)nullptr...), reserve(0), size(0) {
			*this = other;
		}

		~multi_vector() {
			destroy(sequence_type{});
			reserve = 0;
			size = 0;
		}

		ref<this_type> operator=(this_type&& other) {
			data = forward_data(other.data);
			reserve = other.reserve;
			size = other.size;

			other.data = tuple_type{ (Ts*)nullptr... };
			other.reserve = 0;
			other.size = 0;
		}

		template<u32 I>
		tuple_element_t<I, Ts...>& get(u32 idx) {
			return data.get<I>()[idx];
		}

		void resize(u32 sz) {
			auto resize_helper = []<u32... Indices>(ref<tuple_type> src, ref<tuple_type> dst, u32 count, index_sequence<Indices...>) {
				auto helper = []<u32 I>(tuple_element_t<I, Ts...>* src, tuple_element<I, Ts...>* dst, u32 count, uint_constant<I>) {
					u32 bytes = (u32)(count * sizeof(tuple_element_t<I, Ts...>));
					copy256((u8*)src, (u8*)dst, align_size256(bytes));
					free256((u8*)src);
					return 0;
				};

				swallow(helper(src.get<Indices>(), dst.get<Indices>(), count, uint_constant<Indices>{})...);
			};

			tuple_type old = forward_data(data);

			count = reserve;
			allocate(sz);
			resize_helper(old, data, count, sequence_type{});
		}

		void add(Ts&&... data) {
			auto add_helper = []<u32... Indices>(ref<tuple_element> dst, u32 idx, Ts&&... data, index_sequence<Indices...>) {
				auto helper = []<u32 I>(ref<tuple_element_t<I, Ts...>> dst, tuple_element_t<I, Ts...>&& data, uint_constant<I>) {
					dst = move_data(data);
					return 0;
				};
				swallow(helper(dst.get<Indices>()[idx], forward_data(data), uint_constant<Indices>{})...);
			};

			if (reserve <= size) {
				resize(reserve * 2);
			}

			add_helper(data, size++, forward_data(data), sequence_type{});
		}

		void del(u32 idx) {
			auto del_helper = []<u32... Indices>(ref<tuple_type> data, u32 idx, u32 last, index_sequence<Indices...>) {
				auto helper = []<u32 I>(ref<tuple_element_t<I, Ts...>> src, ref<tuple_element_t<I, Ts...>> dst, uint_constant<I>) {
					core::destroy(&dst);
					copy8(bytes(src), bytes(dst), sizeof(tuple_element_t<I, Ts...>));
					zero8(bytes(src), sizeof(tuple_element_t<I, Ts...>));
					return 0;
				};

				swallow(helper(data.get<Indices>()[idx], data.get<Indices>()[last], uint_constant<Indices>{})...);
			};

			helper(data, idx, --size, sequence_type{});
		}

		template<u32... Indices>
		void allocate(u32 sz, index_sequence<Indices...>) {
			auto helper = []<typename T>(ref<T*> arg, u32 sz) {
				memptr ptr = alloc256(sz * sizeof(T));
				arg = (T*)ptr.data;
				return 0;
			};

			swallow(helper(data.get<Indices>(), sz)...);
			reserve = sz;
		}

		template<u32... Indices>
		void destroy(index_sequence<Indices...>) {
			auto helper = []<typename T>(ref<T*> arg) {
				if (!arg) return 0;
				for (int i : range(size)) {
					core::destroy(&arg[i]);
				}

				free256((void*)arg);
				arg = nullptr;
				return 0;
			};

			swallow(helper(data.get<Indices>())...);
		}

		tuple_type data;
		u32 reserve;
		u32 size;
	};
};
