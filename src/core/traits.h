#pragma once

#define move_data(...) static_cast<core::raw_type<decltype(__VA_ARGS__)>::type&&>(__VA_ARGS__)
#define forward_data(...) static_cast<decltype(__VA_ARGS__)&&>(__VA_ARGS__)

namespace core {
	template <typename T, T val>
	struct basic_constant {
		using type = T;
		static constexpr type value = val;
	};

	template <typename T>
	struct is_destructible : public basic_constant<bool, __is_destructible(T)> {};

	template <typename T>
	struct is_constructible : public basic_constant<bool, __is_constructible(T)> {};

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

	template<typename T>
	struct remove_const {
		typedef T type;
	};

	template<typename T>
	struct remove_const<const T> {
		typedef T type;
	};

	template<typename T>
	struct raw_type {
		typedef typename remove_const<remove_ref<T>>::type type;
	};
}
