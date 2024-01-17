export module core.traits;
import core.types;

export namespace core {
	template <typename T, T val>
	struct basic_constant {
		using type = T;
		static constexpr type value = val;
	};

	template <bool val>
	using bool_constant = basic_constant<bool, val>;

	template <i32 val>
	using int_constant = basic_constant<i32, val>;

	template <u32 val>
	using uint_constant = basic_constant<u32, val>;

	template <typename T>
	struct is_destructible : public bool_constant<__is_destructible(T)> {};

	template <typename T>
	inline constexpr bool is_destructible_v = is_destructible<T>::value;

	template <typename T>
	struct is_constructible : public bool_constant<__is_constructible(T)> {};

	template <typename T>
	inline constexpr bool is_constructible_v = is_constructible<T>::value;

	template<typename T>
	struct remove_ref {
		typedef T type;
	};

	template<typename T>
	struct remove_ref<T&> {
		typedef T type;
	};

	template<typename T>
	struct remove_ref<T&&> {
		typedef T type;
	};

	template <typename T>
	using remove_ref_t = typename remove_ref<T>::type;

	template<typename T>
	struct remove_const {
		typedef T type;
	};

	template<typename T>
	struct remove_const<const T> {
		typedef T type;
	};

	template <typename T>
	using remove_const_t = typename remove_const<T>::type;

	template<typename T>
	struct raw_type {
		typedef remove_const_t<remove_ref_t<T>> type;
	};

	template <typename T>
	using raw_type_t = typename raw_type<T>::type;

	template <bool val>
	struct _destroy {
		template<typename T>
		static void destroy(T* data) {
			// do nothing;
		}
	};

	template <>
	struct _destroy<true> {
		template<typename T>
		static void destroy(T* data) {
			data->~T();
		}
	};

	template <typename T>
	void destroy(T* data) {
		_destroy<is_destructible_v<T>>::destroy<T>(data);
	};
}
