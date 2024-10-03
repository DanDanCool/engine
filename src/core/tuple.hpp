module;

#include "core.h"
#include <cstddef>

export module core.tuple;
import core.types;
import core.traits;

export namespace core {
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
		tuple_node(fwd<type> arg)
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
		_tuple_data(fwd<Ts>... args)
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

		tuple(fwd<Ts>... args)
		: data(forward_data(args)...) {

		}

		tuple(fwd<this_type> other)
		: data() {
			*this = forward_data(other);
		}

		tuple(cref<this_type> other)
		: data() {
			*this = other;
		}

		ref<this_type> operator=(fwd<this_type> other) {
			_move(forward_data(other), sequence_type{});
			return *this;
		}

		ref<this_type> operator=(cref<this_type> other) {
			_copy(other, sequence_type{});
			return *this;
		}

		template<u32... Indices>
		void _move(fwd<this_type> other, index_sequence<Indices...>) {
			auto helper = []<u32 I>(fwd<this_type> src, ref<this_type> dst, uint_constant<I>) {
				dst.get<I>() = forward_data(src.get<I>());
				return 0;
			};

			(helper(forward_data(other), *this, uint_constant<Indices>{}), ...);
		}

		template<u32... Indices>
		void _copy(cref<this_type> other, index_sequence<Indices...>) {
			auto helper = []<u32 I>(cref<this_type> src, ref<this_type> dst, uint_constant<I>) {
				dst.get<I>() = src.get<I>();
				return 0;
			};

			(helper(other, *this, uint_constant<Indices>{}), ...);
		}

		template<std::size_t I>
		cref<tuple_element_t<I, Ts...>> get() const {
			return tuple_get<I>(data);
		}

		template<std::size_t I>
		ref<tuple_element_t<I, Ts...>> get() {
			return tuple_get<I>(data);
		}

		u32 size() {
			return sizeof...(Ts);
		}

		tuple_data<Ts...> data;
	};

}

// implement structured bindings
export namespace std {
	template <typename... Ts>
	struct tuple_size<core::tuple<Ts...>> {
		static const size_t value = sizeof...(Ts);
	};

	template <size_t I, typename... Ts>
	struct tuple_element<I, core::tuple<Ts...>> {
		using type = core::tuple_element_t<I, Ts...>;
	};
}
