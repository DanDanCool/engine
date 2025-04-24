module;

#ifdef _MSC_VER
#include <limits.h>
#endif

export module core.types;

export {
	typedef unsigned int uint;
	typedef const char* cstr;

	template <typename T> using cref = const T&;
	template <typename T> using ref = T&;

	template <typename T> using cptr = const T*;
	template <typename T> using ptr = T*;

	template <typename T> using fwd = T&&;

#ifdef __GNUC__
	typedef __INT8_TYPE__ i8;
	typedef __INT16_TYPE__ i16;
	typedef __INT32_TYPE__ i32;
	typedef __INT64_TYPE__ i64;

	typedef __UINT8_TYPE__ u8;
	typedef __UINT16_TYPE__ u16;
	typedef __UINT32_TYPE__ u32;
	typedef __UINT64_TYPE__ u64;

	typedef float f32;
	typedef double f64;

	constexpr i32 I8_MAX  = __INT8_MAX__;
	constexpr i32 I16_MAX = __INT16_MAX__;
	constexpr i32 I32_MAX = __INT32_MAX__;
	constexpr i64 I64_MAX = __INT64_MAX__;

	constexpr u32 U8_MAX = __UINT8_MAX__;
	constexpr u32 U16_MAX = __UINT16_MAX__;
	constexpr u32 U32_MAX = __UINT32_MAX__;
	constexpr u64 U64_MAX = __UINT64_MAX__;
#endif

#ifdef _MSC_VER
	typedef __int8 i8;
	typedef __int16 i16;
	typedef __int32 i32;
	typedef __int64 i64;

	typedef unsigned __int8 u8;
	typedef unsigned __int16 u16;
	typedef unsigned __int32 u32;
	typedef unsigned __int64 u64;

	typedef float f32;
	typedef double f64;

	constexpr i32 I8_MAX  = SCHAR_MAX;
	constexpr i32 I16_MAX = SHRT_MAX;
	constexpr i32 I32_MAX = INT_MAX;
	constexpr i64 I64_MAX = LLONG_MAX;

	constexpr u32 U8_MAX = UCHAR_MAX;
	constexpr u32 U16_MAX = USHRT_MAX;
	constexpr u32 U32_MAX = UINT_MAX;
	constexpr u64 U64_MAX = ULLONG_MAX;
#endif
}

export namespace core {
	template <typename T>
	T abs(cref<T> a) {
		return a < 0 ? -a : a;
	}

	template <typename T>
	cref<T> min(cref<T> a, cref<T> b) {
		bool val = a < b;
		return val ? a : b;
	}

	template <typename T>
	cref<T> max(cref<T> a, cref<T> b) {
		bool val = a > b;
		return val ? a : b;
	}

	template <typename T>
	T clamp(T a, T min, T max) {
		T tmp = a;
		tmp = tmp < min ? min : tmp;
		tmp = tmp > max ? max : tmp;
		return tmp;
	}

	template <typename Impl>
	struct monad {
		ref<Impl> derived() {
			return *(Impl*)this;
		}

		Impl some_then(auto fn) {
			if (derived()) {
				return fn(derived());
			}
			return derived();
		}

		Impl none_then(auto fn) {
			if (!derived()) {
				return fn(derived());
			}
			return derived();
		}
	};

	struct _none_option {
	} none_option;

	template <typename T>
	struct option : public monad<option<T>> {
		using type = T;

		option(T _value)
		: value(_value), some(true) {}

		option()
		: value(), some(false) {}

		option(_none_option)
		: value(), some(false) {}

		type get() const {
			JOLLY_CORE_ASSERT(some);
			return value;
		}

		type get_or(type other) const {
			if (some) {
				return value;
			}
			return other;
		}

		operator bool() const {
			return some;
		}

		type value;
		bool some;
	};
}
