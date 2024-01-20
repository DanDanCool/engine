# pragma once

namespace assert {
	typedef const char* cstr;
	cstr message(cstr msg = "assertion failed!");
	void callback(cstr expr, cstr file, int line, cstr message);
}

#define ENUM_CLASS_OPERATORS(enum) \
	enum operator|(enum a, enum b) { \
		u64 res = (u64)a | (u64)b; \
		return (enum)res; \
	} \
	enum operator&(enum a, enum b) { \
		u64 res = (u64)a & (u64)b; \
		return (enum)res; \
	} \

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

#define forward_data(...) static_cast<decltype(__VA_ARGS__)&&>(__VA_ARGS__)
