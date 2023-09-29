#pragma once

namespace core {
	template <typename T, T val>
	struct basic_constant {
		using type = T;
		static constexpr type value = val;
	};

	template <typename T>
	struct is_destructible : public basic_constant<bool, __is_destructible(T)> {};
}
