module;

#include <core/core.h>
#include <intrin.h>

module core.atom;

#define ATOMIC_LOAD_X86_IMPL(type, obj) \
	type val = obj.data; \
	return val

#define ATOMIC_STORE_X86_IMPL(type, obj, data) \
	obj.data = data

#define ADD_OP_i8(obj, arg) \
	static_assert(sizeof(obj.data) == sizeof(i8)); \
	i8 tmp = _InterlockedExchangeAdd8((i8*)&obj.data, arg)

#define ADD_OP_i16(obj, arg) \
	static_assert(sizeof(obj.data) == sizeof(i16)); \
	i16 tmp = _InterlockedExchangeAdd16((i16*)&obj.data, arg)

#define ADD_OP_i32(obj, arg) \
	static_assert(sizeof(obj.data) == sizeof(i32)); \
	i32 tmp = _InterlockedExchangeAdd((long*)&obj.data, arg)

#define ADD_OP_i64(obj, arg) \
	static_assert(sizeof(obj.data) == sizeof(i64)); \
	i64 tmp = _InterlockedExchangeAdd64((i64*)&obj.data, arg)

#define ADD_OP_u8(obj, arg) ADD_OP_i8(obj, arg)
#define ADD_OP_u16(obj, arg) ADD_OP_i16(obj, arg)
#define ADD_OP_u32(obj, arg) ADD_OP_i32(obj, arg)
#define ADD_OP_u64(obj, arg) ADD_OP_i64(obj, arg)

#define ADD_OP(type, obj, arg) CAT(ADD_OP_, type)(obj, arg)

#define ATOMIC_ADD_X86_IMPL(type, obj, arg) \
	ADD_OP(type, obj, arg);

#define SUB_OP_i8(obj, arg) ADD_OP_i8(obj, -arg)
#define SUB_OP_i16(obj, arg) ADD_OP_i16(obj, -arg)
#define SUB_OP_i32(obj, arg) ADD_OP_i32(obj, -arg)
#define SUB_OP_i64(obj, arg) ADD_OP_i64(obj, -arg)

#define SUB_OP_u8(obj, arg)	ADD_OP_i8(obj, -(i8)arg)
#define SUB_OP_u16(obj, arg) ADD_OP_i16(obj, -(i16)arg)
#define SUB_OP_u32(obj, arg) ADD_OP_i32(obj, -(i32)arg)
#define SUB_OP_u64(obj, arg) ADD_OP_i64(obj, -(i64)arg)

#define SUB_OP(type, obj, arg) CAT(SUB_OP_, type)(obj, arg)

#define ATOMIC_SUB_X86_IMPL(type, obj, arg) \
	SUB_OP(type, obj, arg);

// for some reason the _np version does not exist for i8
#define CMPXCHG_OP_i8(obj, expected, desired) \
	static_assert(sizeof(obj.data) == sizeof(i8)); \
	i8 tmp = _InterlockedCompareExchange8((i8*)&obj.data, *(i8*)&desired, *(i8*)&expected)
#define CMPXCHG_OP_i16(obj, expected, desired) \
	static_assert(sizeof(obj.data) == sizeof(i16)); \
	i16 tmp = _InterlockedCompareExchange16_np((i16*)&obj.data, *(i16*)&desired, *(i16*)&expected)
#define CMPXCHG_OP_i32(obj, expected, desired) \
	static_assert(sizeof(obj.data) == sizeof(i32)); \
	long tmp = _InterlockedCompareExchange_np((long*)&obj.data, *(long*)&desired, *(long*)&expected)
#define CMPXCHG_OP_i64(obj, expected, desired) \
	static_assert(sizeof(obj.data) == sizeof(i64)); \
	i64 tmp = _InterlockedCompareExchange64_np((i64*)&obj.data, *(i64*)&desired, *(i64*)&expected)

#define CMPXCHG_OP_u8(obj, expected, desired) CMPXCHG_OP_i8(obj, expected, desired)
#define CMPXCHG_OP_u16(obj, expected, desired) CMPXCHG_OP_i16(obj, expected, desired)
#define CMPXCHG_OP_u32(obj, expected, desired) CMPXCHG_OP_i32(obj, expected, desired)
#define CMPXCHG_OP_u64(obj, expected, desired) CMPXCHG_OP_i64(obj, expected, desired)
#define CMPXCHG_OP_bool(obj, expected, desired) CMPXCHG_OP_i8(obj, expected, desired)

#define CMPXCHG_OP(type, obj, expected, desired) CAT(CMPXCHG_OP_, type)(obj, expected, desired)

#define ATOMIC_CMPXCHG_X86_IMPL(type, obj, expected, desired) \
	CMPXCHG_OP(type, obj, expected, desired); \
	type result = *(type*)&tmp; \
	bool match = expected == result; \
	expected = result; \
	return match;

// assume x86, memory_order does not do anything
#define ATOMIC_LOAD_IMPL(type) \
template<> type atomic_load<type, _memory_order_relaxed>(cref<atom_base<type>> obj) { \
	ATOMIC_LOAD_X86_IMPL(type, obj); \
} \
template<> type atomic_load<type, _memory_order_acquire>(cref<atom_base<type>> obj) { \
	ATOMIC_LOAD_X86_IMPL(type, obj); \
}

#define ATOMIC_STORE_IMPL(type) \
template<> void atomic_store<type, _memory_order_relaxed>(ref<atom_base<type>> obj, type data) { \
	ATOMIC_STORE_X86_IMPL(type, obj, data); \
} \
template<> void atomic_store<type, _memory_order_release>(ref<atom_base<type>> obj, type data) { \
	ATOMIC_STORE_X86_IMPL(type, obj, data); \
} \

#define ATOMIC_ADD_IMPL(type) \
template<> void atomic_add<type, _memory_order_relaxed>(ref<atom_base<type>> obj, type arg) { \
	ATOMIC_ADD_X86_IMPL(type, obj, arg); \
} \
template<> void atomic_add<type, _memory_order_release>(ref<atom_base<type>> obj, type arg) { \
	ATOMIC_ADD_X86_IMPL(type, obj, arg); \
} \

#define ATOMIC_SUB_IMPL(type) \
template<> void atomic_sub<type, _memory_order_relaxed>(ref<atom_base<type>> obj, type arg) { \
	ATOMIC_SUB_X86_IMPL(type, obj, arg); \
} \
template<> void atomic_sub<type, _memory_order_release>(ref<atom_base<type>> obj, type arg) { \
	ATOMIC_SUB_X86_IMPL(type, obj, arg); \
} \

#define ATOMIC_CMPXCHG_IMPL(type) \
template<> bool atomic_cmpxchg<type, _memory_order_relaxed, _memory_order_relaxed>(ref<atom_base<type>> obj, ref<type> expected, type desired) { \
	ATOMIC_CMPXCHG_X86_IMPL(type, obj, expected, desired); \
} \
template<> bool atomic_cmpxchg<type, _memory_order_relaxed, _memory_order_release>(ref<atom_base<type>> obj, ref<type> expected, type desired) { \
	ATOMIC_CMPXCHG_X86_IMPL(type, obj, expected, desired); \
} \
template<> bool atomic_cmpxchg<type, _memory_order_release, _memory_order_relaxed>(ref<atom_base<type>> obj, ref<type> expected, type desired) { \
	ATOMIC_CMPXCHG_X86_IMPL(type, obj, expected, desired); \
} \
template<> bool atomic_cmpxchg<type, _memory_order_release, _memory_order_release>(ref<atom_base<type>> obj, ref<type> expected, type desired) { \
	ATOMIC_CMPXCHG_X86_IMPL(type, obj, expected, desired); \
} \

namespace core {
	ATOMIC_LOAD_IMPL(i8);
	ATOMIC_LOAD_IMPL(i16);
	ATOMIC_LOAD_IMPL(i32);
	ATOMIC_LOAD_IMPL(i64);
	ATOMIC_LOAD_IMPL(u8);
	ATOMIC_LOAD_IMPL(u16);
	ATOMIC_LOAD_IMPL(u32);
	ATOMIC_LOAD_IMPL(u64);
	ATOMIC_LOAD_IMPL(bool);

	ATOMIC_STORE_IMPL(i8);
	ATOMIC_STORE_IMPL(i16);
	ATOMIC_STORE_IMPL(i32);
	ATOMIC_STORE_IMPL(i64);
	ATOMIC_STORE_IMPL(u8);
	ATOMIC_STORE_IMPL(u16);
	ATOMIC_STORE_IMPL(u32);
	ATOMIC_STORE_IMPL(u64);
	ATOMIC_STORE_IMPL(bool);

	ATOMIC_ADD_IMPL(i8);
	ATOMIC_ADD_IMPL(i16);
	ATOMIC_ADD_IMPL(i32);
	ATOMIC_ADD_IMPL(i64);
	ATOMIC_ADD_IMPL(u8);
	ATOMIC_ADD_IMPL(u16);
	ATOMIC_ADD_IMPL(u32);
	ATOMIC_ADD_IMPL(u64);

	ATOMIC_SUB_IMPL(i8);
	ATOMIC_SUB_IMPL(i16);
	ATOMIC_SUB_IMPL(i32);
	ATOMIC_SUB_IMPL(i64);
	ATOMIC_SUB_IMPL(u8);
	ATOMIC_SUB_IMPL(u16);
	ATOMIC_SUB_IMPL(u32);
	ATOMIC_SUB_IMPL(u64);

	ATOMIC_CMPXCHG_IMPL(i8);
	ATOMIC_CMPXCHG_IMPL(i16);
	ATOMIC_CMPXCHG_IMPL(i32);
	ATOMIC_CMPXCHG_IMPL(i64);
	ATOMIC_CMPXCHG_IMPL(u8);
	ATOMIC_CMPXCHG_IMPL(u16);
	ATOMIC_CMPXCHG_IMPL(u32);
	ATOMIC_CMPXCHG_IMPL(u64);
	ATOMIC_CMPXCHG_IMPL(bool);
}
