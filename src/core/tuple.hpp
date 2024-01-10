module;

#include "core.h"

export module core.tuple;
import core.memory;
import core.traits;
import core.simd;
import core.operations;
import core.iterator;

export namespace core {
	// reference wrapper
	template <typename T>
	struct wref {
		using type = T;
		wref(type& in)
		: data(in) {}

		operator cref<type>() const {
			return data;
		}

		operator ref<type>() {
			return data;
		}

		type* operator->() const {
			return &data;
		}

		type& data;
	};

	template <typename T1, typename T2>
	struct pair {
		using this_type = pair<T1, T2>;
		pair() = default;

		pair(T1&& a, T2&& b)
			: one(forward_data(a)), two(forward_data(b)) {}

		pair(cref<T1> a, cref<T2> b)
			: one(a), two(b) {}

		pair(this_type&& other) {
			*this = forward_data(other);
		}

		pair(cref<this_type> other) {
			*this = other;
		}

		ref<this_type> operator=(this_type&& other) {
			one = forward_data(other.one);
			two = forward_data(other.two);
			return *this;
		}

		ref<this_type> operator=(cref<this_type> other) {
			one = forward_data(copy(other.one));
			two = forward_data(copy(other.two));
			return *this;
		}

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

	template <typename T, T... S>
	struct sequence_base {
		using type = T;
		using this_type = sequence_base<T, S...>;

		static constexpr type size() {
			return sizeof...(S);
		}
	};

#ifdef _MSC_VER
	// https://github.com/microsoft/STL/blob/b84bf09ab6f9267992f899956cea46e7824fe0c3/stl/inc/utility#L53C80-L53C80
	template <typename T, T N>
	using make_sequence = __make_integer_seq<sequence_base, T, N>;
#endif

	template <u32... S>
	using index_sequence = sequence_base<u32, S...>;

	template <u32 N>
	using index_sequential = make_sequence<u32, N>;

	template<u32 N, typename T>
	struct tuple_node {
		using type = T;

		tuple_node() = default;
		tuple_node(type&& arg)
		: data(forward_data(arg)) {}

		type data;
	};

	// http://mitchnull.blogspot.com/2012/06/c11-tuple-implementation-details-part-1.html
	template <typename Indices, typename... Ts>
	struct _tuple_data {
		_tuple_data() = delete;
	};

	template <u32... Indices, typename... Ts>
	struct _tuple_data<index_sequence<Indices...>, Ts...> : public tuple_node<Indices, Ts>... {
		_tuple_data() = default;
		_tuple_data(Ts&&... args)
		: tuple_node<Indices, Ts>(forward_data(args))... {

		}
	};

	template <typename... Ts>
	using tuple_data = _tuple_data<index_sequential<sizeof...(Ts)>, Ts...>;

	// recursive template... look into compiler intrinsics
	template<u32 I, typename T, typename... Ts>
	struct tuple_element {
		typedef tuple_element<I - 1, Ts...>::type type;
	};

	template<typename T, typename... Ts>
	struct tuple_element<0, T, Ts...> {
		typedef T type;
	};

	template<u32 I, typename... Ts>
	using tuple_element_t = typename tuple_element<I, Ts...>::type;

	template<u32 I, typename... Ts>
	cref<tuple_element_t<I, Ts...>> tuple_get(cref<tuple_data<Ts...>> data) {
		using node = tuple_node<I, tuple_element_t<I, Ts...>>;
		return static_cast<cref<node>>(data).data;
	}

	template<u32 I, typename... Ts>
	ref<tuple_element_t<I, Ts...>> tuple_get(ref<tuple_data<Ts...>> data) {
		using node = tuple_node<I, tuple_element_t<I, Ts...>>;
		return static_cast<ref<node>>(data).data;
	}

	template<typename... Ts>
	struct tuple {
		using this_type = tuple<Ts...>;
		using sequence_type = index_sequential<sizeof...(Ts)>;

		tuple(Ts&&... args)
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
			auto helper = []<u32 I>(this_type&& src, ref<this_type> dst, uint_constant<I>) {
				dst.get<I>() = forward_data(src.get<I>());
				return 0;
			};

			swallow(helper(forward_data(other), *this, uint_constant<Indices>{})...);
		}

		template<u32 I>
		cref<tuple_element_t<I, Ts...>> get() const {
			return tuple_get<I>(data);
		}

		template<u32 I>
		ref<tuple_element_t<I, Ts...>> get() {
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

		multi_vector()
		: data((Ts*)nullptr...), reserve(0), size(0) {}

		multi_vector(u32 sz)
		: data((Ts*)nullptr...), reserve(0), size(0) {
			sz = max<u32>(sz, MULTI_VECTOR_DEFAULT_SIZE);
			allocate(sz, sequence_type{});
		}

		multi_vector(this_type&& other)
		: data((Ts*)nullptr...), reserve(0), size(0) {
			*this = forward_data(other);
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
			return *this;
		}

		template<u32 I>
		tuple_element_t<I, Ts...>& get(u32 idx) const {
			return data.get<I>()[idx];
		}

		void resize(u32 sz) {
			tuple_type old = forward_data(data);
			u32 count = reserve;
			allocate(sz, sequence_type{});
			_resize(old, count, sequence_type{});
		}

		void add(Ts&&... vals) {
			if (reserve <= size) {
				resize(reserve * 2);
			}

			_add(size++, forward_data(vals)..., sequence_type{});
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

			del_helper(data, idx, --size, sequence_type{});
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
			auto helper = []<typename T>(ref<T*> arg, u32 size) {
				if (!arg) return 0;
				for (int i : range(size)) {
					core::destroy(&arg[i]);
				}

				free256((void*)arg);
				arg = nullptr;
				return 0;
			};

			swallow(helper(data.get<Indices>(), size)...);
		}

		template <u32... Indices>
		void _resize(ref<tuple_type> src, u32 count, index_sequence<Indices...>) {
			auto helper = []<typename T>(T* src, T* dst, u32 count) {
				u32 bytes = (u32)(count * sizeof(T));
				copy256((u8*)src, (u8*)dst, align_size256(bytes));
				free256((u8*)src);
				return 0;
			};

			swallow(helper(src.get<Indices>(), data.get<Indices>(), count)...);
		};


		template <u32... Indices>
		void _add(u32 idx, Ts&&... vals, index_sequence<Indices...>) {
			auto helper = []<u32 I>(ref<tuple_element_t<I, Ts...>> dst, tuple_element_t<I, Ts...>&& val, uint_constant<I>) {
				dst = forward_data(val);
				return 0;
			};

			swallow(helper(get<Indices>(idx), forward_data(vals), uint_constant<Indices>{})...);
		}

		tuple_type data;
		u32 reserve;
		u32 size;
	};
};
