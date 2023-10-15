# pragma once

typedef unsigned int uint;
typedef const char* cstr;

template <typename T> using cref = const T&;
template <typename T> using ref = T&;

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

#define I8_MAX __INT8_MAX__
#define I16_MAX __INT16_MAX__
#define I32_MAX __INT32_MAX__
#define I64_MAX __INT64_MAX__

#define U8_MAX __UINT8_MAX__
#define U16_MAX __UINT16_MAX__
#define U32_MAX __UINT32_MAX__
#define U64_MAX __UINT64_MAX__
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

#include <limits.h>
#define I8_MAX SCHAR_MAX
#define I16_MAX SHRT_MAX
#define I32_MAX INT_MAX
#define I64_MAX LLONG_MAX

#define U8_MAX UCHAR_MAX
#define U16_MAX USHRT_MAX
#define U32_MAX UINT_MAX
#define U64_MAX ULLONG_MAX
#endif

template <typename T, typename S>
ref<T> cast(ref<S> val) {
	return *(T*)&val;
}

template <typename T, typename S>
cref<T> cast(cref<S> val) {
	return *(T*)&val;
}

template <typename T, typename S>
ref<T> cast(S* val) {
	return *(T*)val;
}

template <typename T, typename S>
cref<T> cast(const S* val) {
	return *(T*)val;
}

template <typename S>
bool cast(cref<S> val) {
	return (bool)val;
}

template <typename T>
u8* bytes(cref<T> val) {
	return (u8*)&val;
}

#define ENUM_CLASS_OPERATORS(enum) \
	enum operator|(enum a, enum b) { \
		i64 res = (i64)a | (i64)b; \
		return (enum)res; \
	} \
	enum operator&(enum a, enum b) { \
		i64 res = (i64)a & (i64)b; \
		return (enum)res; \
	} \

#define elif else if

#ifdef JOLLY_WIN32
#ifdef JOLLY_EXPORT
#define JOLLY_API __declspec(dllexport)
#else
#define JOLLY_API __declspec(dllimport)
#endif
#else
#define JOLLY_API
#endif

#define PAREN ()

#define STR(x) #x
#define CAT(a, ...) CAT_(a, __VA_ARGS__)
#define CAT_(a, ...) a ## __VA_ARGS__

#define EMPTY()
#define DEFER(id) id EMPTY()
#define OBSTRUCT(...) __VA_ARGS__ DEFER(EMPTY)()

#define EXPAND(...) EXPAND1(EXPAND1(EXPAND1(EXPAND1(__VA_ARGS__))))
#define EXPAND1(...) EXPAND2(EXPAND2(EXPAND2(EXPAND2(__VA_ARGS__))))
#define EXPAND2(...) EXPAND3(EXPAND3(EXPAND3(EXPAND3(__VA_ARGS__))))
#define EXPAND3(...) EXPAND4(EXPAND4(EXPAND4(EXPAND4(__VA_ARGS__))))
#define EXPAND4(...) __VA_ARGS__

#ifdef _MSC_VER
#define JOLLY_DEBUG_BREAK() __debugbreak()
#endif

namespace assert {
	cstr message(cstr msg = "assertion failed!");
	void callback(cstr expr, cstr file, int line, cstr message);
}

#define JOLLY_ASSERT(expr, ...) \
	if (!(expr)) { \
		assert::callback(#expr, __FILE__, __LINE__, assert::message(__VA_ARGS__)); \
		JOLLY_DEBUG_BREAK(); \
	}

// for use in core libraries, does not print to stdout
#define JOLLY_CORE_ASSERT(expr) \
	if (!(expr)) { \
		JOLLY_DEBUG_BREAK(); \
	}
