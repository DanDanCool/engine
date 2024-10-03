// Copyright 2019 Ulf Adams
//
// The contents of this file may be used under the terms of the Apache License,
// Version 2.0.
//
//    (See accompanying file LICENSE-Apache or copy at
//     http://www.apache.org/licenses/LICENSE-2.0)
//
// Alternatively, the contents of this file may be used under the terms of
// the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE-Boost or copy at
//     https://www.boost.org/LICENSE_1_0.txt)
//
// Unless required by applicable law or agreed to in writing, this software
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.

module;

#include <core/core.h>
#ifdef _MSC_VER
#include <intrin.h>
#endif

export module core.impl.ryu_f64;
import core.types;
import core.simd;
import core.memory;
import core.iterator;
import core.impl.ryu.table;
import core.impl.ryu.common;

namespace core::impl::ryu {
	constexpr u64 DOUBLE_MANTISSA_BITS = 52;
	constexpr u64 DOUBLE_EXPONENT_BITS = 11;
	constexpr u64 DOUBLE_EXPONENT_BIAS = 1023;

	u32 floor_log2(u64 value) {
#if defined(_MSC_VER)
		unsigned long index;
		return _BitScanReverse64(&index, value) ? index : 64;
#else
		return 63 - __builtin_clzll(value);
#endif
	}

#ifdef __SIZEOF_INT128__
#define HAS_UINT128
	typedef __uint128_t u128;
#endif

#if defined(_MSC_VER) && defined(_M_X64)
#define HAS_64_BIT_INTRINSICS
#endif

	u64 div5(u64 x) {
		return x / 5;
	}

	u64 div10(u64 x) {
		return x / 10;
	}

	u64 div100(u64 x) {
		return x / 100;
	}

	u64 div1e8(u64 x) {
		return x / 100000000;
	}

	u64 div1e9(u64 x) {
		return x / 1000000000;
	}

	u32 mod1e9(u64 x) {
		return (u32) (x - 1000000000 * div1e9(x));
	}

	u32 pow5Factor(u64 value) {
		u64 m_inv_5 = 14757395258967641293u; // 5 * m_inv_5 = 1 (mod 2^64)
		u64 n_div_5 = 3689348814741910323u;  // #{ n | n = 0 (mod 2^64) } = 2^64 / 5
		u32 count = 0;

		while (true) {
		JOLLY_CORE_ASSERT(value != 0);
		value *= m_inv_5;
		if (value > n_div_5)
		break;
		++count;
		}

		return count;
	}

	// Returns true if value is divisible by 5^p.
	bool multipleOfPowerOf5(u64 value, u32 p) {
		return pow5Factor(value) >= p;
	}

	// Returns true if value is divisible by 2^p.
	bool multipleOfPowerOf2(u64 value, u32 p) {
		JOLLY_CORE_ASSERT(value != 0);
		JOLLY_CORE_ASSERT(p < 64);
		return (value & ((1ull << p) - 1)) == 0;
	}

#if defined(HAS_UINT128) // Best case: use 128-bit type.
	u64 mulShift64(u64 m, cptr<u64> mul, i32 j) {
		u128 b0 = ((u128) m) * mul[0];
		u128 b2 = ((u128) m) * mul[1];
		return (u64) (((b0 >> 64) + b2) >> (j - 64));
	}

	u64 mulShiftAll64(u64 m, cptr<u64> mul, i32 j, ptr<u64> vp, ptr<u64> vm, u32 mmShift) {
		*vp = mulShift64(4 * m + 2, mul, j);
		*vm = mulShift64(4 * m - 1 - mmShift, mul, j);
		return mulShift64(4 * m, mul, j);
	}

#elif defined(HAS_64_BIT_INTRINSICS)
	u64 umul128(u64 a, u64 b, u64* productHi) {
		return _umul128(a, b, productHi);
	}

	// Returns the lower 64 bits of (hi*2^64 + lo) >> dist, with 0 < dist < 64.
	u64 shiftright128(u64 lo, u64 hi, u32 dist) {
		JOLLY_CORE_ASSERT(dist < 64);
		return __shiftright128(lo, hi, (unsigned char) dist);
	}

	u64 mulShift64(u64 m, cptr<u64> mul, i32 j) {
		// m is maximum 55 bits
		u64 high1;                                  // 128
		u64 low1 = umul128(m, mul[1], &high1);		// 64
		u64 high0;                                  // 64
		umul128(m, mul[0], &high0);                 // 0
		u64 sum = high0 + low1;
		if (sum < high0) {
			++high1; // overflow into high1
		}
		return shiftright128(sum, high1, j - 64);
	}

	u64 mulShiftAll64(u64 m, cptr<u64> mul, i32 j, ptr<u64> vp, ptr<u64> vm, u32 mmShift) {
		*vp = mulShift64(4 * m + 2, mul, j);
		*vm = mulShift64(4 * m - 1 - mmShift, mul, j);
		return mulShift64(4 * m, mul, j);
	}
#endif // !defined(HAS_UINT128) && !defined(HAS_64_BIT_INTRINSICS)

	f64 int64Bits2Double(u64 bits) {
		return *(f64*)&bits;
	}

	export f64 ryu_f64_stod(cstr buffer) {
		int m10digits = 0;
		int e10digits = 0;
		int dotIndex = -1;
		int eIndex = -1;
		int dot_underscore = 0;
		int e_underscore = 0;
		u64 m10 = 0;
		i32 e10 = 0;
		bool signedM = buffer[0] == '-';
		bool signedE = false;

		int beg = (buffer[0] == '-') + (buffer[0] == '+');
		for (int i : range(beg, BLOCK_64)) {
			char c = buffer[i];
			if (!c) {
				dotIndex = (dotIndex == -1 ? i : dotIndex) - dot_underscore;
				eIndex = (eIndex == -1 ? i : eIndex) - e_underscore;
				break;
			}

			if (c == '_') {
				dot_underscore += dotIndex == -1;
				e_underscore += eIndex == -1;
				continue;
			}

			if (c == '.') {
				JOLLY_CORE_ASSERT(dotIndex == -1);
				dotIndex = i;
				continue;
			}

			if (c == 'e') {
				JOLLY_CORE_ASSERT(eIndex == -1);
				eIndex = i;
				continue;
			}

			if (c == '-' || c == '+') {
				JOLLY_CORE_ASSERT(eIndex != -1);
				JOLLY_CORE_ASSERT(buffer[i - 1] == 'e');
				signedE = c == '-';
				continue;
			}

			i8 digit = c - '0';
			JOLLY_CORE_ASSERT(digit >= 0 && digit <= 9);
			if (eIndex == -1) {
				JOLLY_CORE_ASSERT(m10digits < 17);
				m10 = 10 * m10 + digit;
				m10digits += m10 != 0;
			} else {
				// TODO: Be more lenient. Return +/-Infinity or +/-0 instead.
				JOLLY_CORE_ASSERT(e10digits <= 3);
				e10 = 10 * e10 + digit;
				e10digits += e10 != 0;
			}
		}

		e10 *= signedE ? -1 : 1;
		e10 -= dotIndex < eIndex ? eIndex - dotIndex - 1 : 0;
		if (m10 == 0) {
			return signedM ? -0.0f : 0.0f;
		}

		// Number is less than 1e-324, which should be rounded down to 0; return +/-0.0.
		if ((m10digits + e10 <= -324) || (m10 == 0)) {
			u64 ieee = ((u64) signedM) << (DOUBLE_EXPONENT_BITS + DOUBLE_MANTISSA_BITS);
			return int64Bits2Double(ieee);
		}

		// Number is larger than 1e+309, which should be rounded to +/-Infinity.
		if (m10digits + e10 >= 310) {
			u64 ieee = (((u64) signedM) << (DOUBLE_EXPONENT_BITS + DOUBLE_MANTISSA_BITS)) | (0x7ffull << DOUBLE_MANTISSA_BITS);
			return int64Bits2Double(ieee);
		}

		// Convert to binary float m2 * 2^e2, while retaining information about whether the conversion was exact (trailingZeros).
		i32 e2;
		u64 m2;
		bool trailingZeros;
		if (e10 >= 0) {
			e2 = floor_log2(m10) + e10 + log2pow5(e10) - (DOUBLE_MANTISSA_BITS + 1);
			int j = e2 - e10 - ceil_log2pow5(e10) + DOUBLE_POW5_BITCOUNT;
			JOLLY_CORE_ASSERT(j >= 0);

			JOLLY_CORE_ASSERT(e10 < DOUBLE_POW5_TABLE_SIZE);
			m2 = mulShift64(m10, DOUBLE_POW5_SPLIT[e10], j);
			trailingZeros = e2 < e10 || (e2 - e10 < 64 && multipleOfPowerOf2(m10, e2 - e10));
		} else {
			e2 = floor_log2(m10) + e10 - ceil_log2pow5(-e10) - (DOUBLE_MANTISSA_BITS + 1);
			int j = e2 - e10 + ceil_log2pow5(-e10) - 1 + DOUBLE_POW5_INV_BITCOUNT;

			JOLLY_CORE_ASSERT(-e10 < DOUBLE_POW5_INV_TABLE_SIZE);
			m2 = mulShift64(m10, DOUBLE_POW5_INV_SPLIT[-e10], j);
			trailingZeros = multipleOfPowerOf5(m10, -e10);
		}

		// Compute the final IEEE exponent.
		u32 ieee_e2 = (u32)max<u32>(0, e2 + DOUBLE_EXPONENT_BIAS + floor_log2(m2));

		// Final IEEE exponent is larger than the maximum representable; return +/-Infinity.
		if (ieee_e2 > 0x7fe) {
			u64 ieee = (((u64) signedM) << (DOUBLE_EXPONENT_BITS + DOUBLE_MANTISSA_BITS)) | (0x7ffull << DOUBLE_MANTISSA_BITS);
			return int64Bits2Double(ieee);
		}

		// We need to figure out how much we need to shift m2. The tricky part is that we need to take
		// the final IEEE exponent into account, so we need to reverse the bias and also special-case
		// the value 0.
		i32 shift = (ieee_e2 == 0 ? 1 : ieee_e2) - e2 - DOUBLE_EXPONENT_BIAS - DOUBLE_MANTISSA_BITS;
		JOLLY_CORE_ASSERT(shift >= 0);

		// need to round up if exact value more than 0.5 above the value we computed. That's
		// We need to update trailingZeros given that we have the exact output exponent ieee_e2 now.
		trailingZeros &= (m2 & ((1ull << (shift - 1)) - 1)) == 0;
		u64 lastRemovedBit = (m2 >> (shift - 1)) & 1;
		bool roundUp = (lastRemovedBit != 0) && (!trailingZeros || (((m2 >> shift) & 1) != 0));

		u64 ieee_m2 = (m2 >> shift) + roundUp;
		JOLLY_CORE_ASSERT(ieee_m2 <= (1ull << (DOUBLE_MANTISSA_BITS + 1)));
		ieee_m2 &= (1ull << DOUBLE_MANTISSA_BITS) - 1;
		if (ieee_m2 == 0 && roundUp) {
			ieee_e2++;
		}

		u64 ieee = (((((u64) signedM) << DOUBLE_EXPONENT_BITS) | (u64)ieee_e2) << DOUBLE_MANTISSA_BITS) | ieee_m2;
		return int64Bits2Double(ieee);
	}

	// The average output length is 16.38 digits, so we check high-to-low.
	// Function precondition: v is not an 18, 19, or 20-digit number.
	u32 decimalLength17(u64 v) {
		JOLLY_CORE_ASSERT(v < 100000000000000000L);
		if (v >= 10000000000000000L) { return 17; }
		if (v >= 1000000000000000L) { return 16; }
		if (v >= 100000000000000L) { return 15; }
		if (v >= 10000000000000L) { return 14; }
		if (v >= 1000000000000L) { return 13; }
		if (v >= 100000000000L) { return 12; }
		if (v >= 10000000000L) { return 11; }
		if (v >= 1000000000L) { return 10; }
		if (v >= 100000000L) { return 9; }
		if (v >= 10000000L) { return 8; }
		if (v >= 1000000L) { return 7; }
		if (v >= 100000L) { return 6; }
		if (v >= 10000L) { return 5; }
		if (v >= 1000L) { return 4; }
		if (v >= 100L) { return 3; }
		if (v >= 10L) { return 2; }
		return 1;
	}

	struct f64_decimal {
		u64 mantissa;
		i32 exponent;
	};

	f64_decimal d2d(u64 ieeeMantissa, u32 ieeeExponent) {
		i32 e2;
		u64 m2;

		if (ieeeExponent == 0) {
			// We subtract 2 so that the bounds computation has 2 additional bits.
			e2 = (i32)(1 - DOUBLE_EXPONENT_BIAS - DOUBLE_MANTISSA_BITS - 2);
			m2 = ieeeMantissa;
		} else {
			e2 = (i32) ieeeExponent - DOUBLE_EXPONENT_BIAS - DOUBLE_MANTISSA_BITS - 2;
			m2 = (1ull << DOUBLE_MANTISSA_BITS) | ieeeMantissa;
		}

		bool even = (m2 & 1) == 0;
		bool acceptBounds = even;

		// Step 2: Determine the interval of valid decimal representations.
		u64 mv = 4 * m2;
		// Implicit bool -> int conversion. True is 1, false is 0.
		u32 mmShift = ieeeMantissa != 0 || ieeeExponent <= 1;
		// Step 3: Convert to a decimal power base using 128-bit arithmetic.
		u64 vr, vp, vm;
		i32 e10;
		bool vmIsTrailingZeros = false;
		bool vrIsTrailingZeros = false;

		if (e2 >= 0) {
			u32 q = log10Pow2(e2) - (e2 > 3);
			e10 = (i32) q;
			i32 k = DOUBLE_POW5_INV_BITCOUNT + pow5bits((i32) q) - 1;
			i32 i = -e2 + (i32) q + k;
			vr = mulShiftAll64(m2, DOUBLE_POW5_INV_SPLIT[q], i, &vp, &vm, mmShift);

			if (q <= 21) {
				// This should use q <= 22, but I think 21 is also safe. Smaller values
				// may still be safe, but it's more difficult to reason about them.
				u32 mvMod5 = ((u32) mv) - 5 * ((u32) div5(mv));
				if (mvMod5 == 0) {
					vrIsTrailingZeros = multipleOfPowerOf5(mv, q);
				} else if (acceptBounds) {
					vmIsTrailingZeros = multipleOfPowerOf5(mv - 1 - mmShift, q);
				} else {
					// Same as min(e2 + 1, pow5Factor(mp)) >= q.
					vp -= multipleOfPowerOf5(mv + 2, q);
				}
			}
		} else {
			u32 q = log10Pow5(-e2) - (-e2 > 1);
			e10 = (i32) q + e2;
			i32 i = -e2 - (i32) q;
			i32 k = pow5bits(i) - DOUBLE_POW5_BITCOUNT;
			i32 j = (i32) q - k;
			vr = mulShiftAll64(m2, DOUBLE_POW5_SPLIT[i], j, &vp, &vm, mmShift);
			if (q <= 1) {
				// {vr,vp,vm} is trailing zeros if {mv,mp,mm} has at least q trailing 0 bits.
				// mv = 4 * m2, so it always has at least two trailing 0 bits.
				vrIsTrailingZeros = true;
				if (acceptBounds) {
					// mm = mv - 1 - mmShift, so it has 1 trailing 0 bit iff mmShift == 1.
					vmIsTrailingZeros = mmShift == 1;
				} else {
					// mp = mv + 2, so it always has at least one trailing 0 bit.
					--vp;
				}
			} else if (q < 63) { // TODO(ulfjack): Use a tighter bound here.
				// We want to know if the full product has at least q trailing zeros.
				// We need to compute min(p2(mv), p5(mv) - e2) >= q
				vrIsTrailingZeros = multipleOfPowerOf2(mv, q);
			}
		}

		// Step 4: Find the shortest decimal representation in the interval of valid representations.
		i32 removed = 0;
		u8 lastRemovedDigit = 0;
		u64 output;

		// General case, which happens rarely (~0.7%).
		if (vmIsTrailingZeros || vrIsTrailingZeros) {
			while (true) {
				u64 vpDiv10 = div10(vp);
				u64 vmDiv10 = div10(vm);
				if (vpDiv10 <= vmDiv10) break;

				u32 vmMod10 = ((u32) vm) - 10 * ((u32) vmDiv10);
				u64 vrDiv10 = div10(vr);
				u32 vrMod10 = ((u32) vr) - 10 * ((u32) vrDiv10);
				vmIsTrailingZeros &= vmMod10 == 0;
				vrIsTrailingZeros &= lastRemovedDigit == 0;
				lastRemovedDigit = (u8) vrMod10;
				vr = vrDiv10;
				vp = vpDiv10;
				vm = vmDiv10;
				++removed;
			}

			if (vmIsTrailingZeros) {
				while (true) {
					u64 vmDiv10 = div10(vm);
					u32 vmMod10 = ((u32) vm) - 10 * ((u32) vmDiv10);
					if (vmMod10 != 0) break;

					u64 vpDiv10 = div10(vp);
					u64 vrDiv10 = div10(vr);
					u32 vrMod10 = ((u32) vr) - 10 * ((u32) vrDiv10);
					vrIsTrailingZeros &= lastRemovedDigit == 0;
					lastRemovedDigit = (u8) vrMod10;
					vr = vrDiv10;
					vp = vpDiv10;
					vm = vmDiv10;
					++removed;
				}
			}

			if (vrIsTrailingZeros && lastRemovedDigit == 5 && vr % 2 == 0) {
				lastRemovedDigit = 4;
			}

			// We need to take vr + 1 if vr is outside bounds or we need to round up.
			output = vr + ((vr == vm && (!acceptBounds || !vmIsTrailingZeros)) || lastRemovedDigit >= 5);
		} else {
			// Specialized for the common case (~99.3%). Percentages below are relative to this.
			bool roundUp = false;
			u64 vpDiv100 = div100(vp);
			u64 vmDiv100 = div100(vm);
			if (vpDiv100 > vmDiv100) { // Optimization: remove two digits at a time (~86.2%).
				u64 vrDiv100 = div100(vr);
				u32 vrMod100 = ((u32) vr) - 100 * ((u32) vrDiv100);
				roundUp = vrMod100 >= 50;
				vr = vrDiv100;
				vp = vpDiv100;
				vm = vmDiv100;
				removed += 2;
			}

			while (true) {
				u64 vpDiv10 = div10(vp);
				u64 vmDiv10 = div10(vm);
				if (vpDiv10 <= vmDiv10) break;

				u64 vrDiv10 = div10(vr);
				u32 vrMod10 = ((u32) vr) - 10 * ((u32) vrDiv10);
				roundUp = vrMod10 >= 5;
				vr = vrDiv10;
				vp = vpDiv10;
				vm = vmDiv10;
				++removed;
			}

			// We need to take vr + 1 if vr is outside bounds or we need to round up.
			output = vr + (vr == vm || roundUp);
		}

		i32 exp = e10 + removed;
		return f64_decimal{output, exp};
	}

	void to_chars(f64_decimal v, bool sign, ref<buffer> buffer) {
		// Step 5: Print the decimal representation.
		if (sign) {
			buffer.write('-');
		}

		u64 index = buffer.index;
		u64 output = v.mantissa;
		u32 olength = decimalLength17(output);
		buffer.write('0', olength + 1);

		// Print the decimal digits.
		u64 i = buffer.index;
		if ((output >> 32) != 0) {
			u64 q = div1e8(output);
			u32 output2 = ((u32) output) - 100000000 * ((u32) q);
			output = q;

			u32 c = output2 % 10000;
			output2 /= 10000;
			u32 d = output2 % 10000;
			u32 c0 = (c % 100) << 1;
			u32 c1 = (c / 100) << 1;
			u32 d0 = (d % 100) << 1;
			u32 d1 = (d / 100) << 1;
			buffer.index = i -= 2;
			buffer.write(membuf{(u8*)(DIGIT_TABLE + c0), 2});
			buffer.index = i -= 2;
			buffer.write(membuf{(u8*)(DIGIT_TABLE + c1), 2});
			buffer.index = i -= 2;
			buffer.write(membuf{(u8*)(DIGIT_TABLE + d0), 2});
			buffer.index = i -= 2;
			buffer.write(membuf{(u8*)(DIGIT_TABLE + d1), 2});
		}

		u32 output2 = (u32) output;
		while (output2 >= 10000) {
			u32 c = output2 % 10000;
			output2 /= 10000;
			u32 c0 = (c % 100) << 1;
			u32 c1 = (c / 100) << 1;
			buffer.index = i -= 2;
			buffer.write(membuf{(u8*)(DIGIT_TABLE + c0), 2});
			buffer.index = i -= 2;
			buffer.write(membuf{(u8*)(DIGIT_TABLE + c1), 2});
		}

		if (output2 >= 100) {
			u32 c = (output2 % 100) << 1;
			output2 /= 100;
			buffer.index = i -= 2;
			buffer.write(membuf{(u8*)(DIGIT_TABLE + c), 2});
		}

		if (output2 >= 10) {
			u32 c = output2 << 1;
			// We can't use memcpy here: the decimal dot goes between these two digits.
			buffer[i -= 1] = DIGIT_TABLE[c + 1];
			buffer[index] = DIGIT_TABLE[c];
		} else {
			buffer[index] = '0' + output2;
		}

		// Print decimal point if needed.
		if (olength > 1) {
			buffer[index + 1] = '.';
			buffer.index = index + olength + 1;
		} else {
			buffer.index = index + 1;
		}

		buffer.write('e');

		i32 exp = v.exponent + (i32) olength - 1;
		if (exp < 0) {
			buffer.write('-');
			exp = -exp;
		}

		if (exp >= 100) {
			i32 c = exp % 10;
			buffer.write(membuf{(u8*)(DIGIT_TABLE + 2 * (exp / 10)), 2});
			buffer.write('0' + c);
		} else if (exp >= 10) {
			buffer.write(membuf{(u8*)(DIGIT_TABLE + 2 * exp), 2});
		} else {
			buffer.write('0' + exp);
		}
	}

	bool d2d_small_int(u64 ieeeMantissa, u32 ieeeExponent, f64_decimal* v) {
		u64 m2 = (1ull << DOUBLE_MANTISSA_BITS) | ieeeMantissa;
		i32 e2 = (i32) ieeeExponent - DOUBLE_EXPONENT_BIAS - DOUBLE_MANTISSA_BITS;

		if (e2 > 0) {
			return false;
		}

		// f < 1.
		if (e2 < -52) {
			return false;
		}

		// Since 2^52 <= m2 < 2^53 and 0 <= -e2 <= 52: 1 <= f = m2 / 2^-e2 < 2^53.
		// Test if the lower -e2 bits of the significand are 0, i.e. whether the fraction is 0.
		u64 mask = (1ull << -e2) - 1;
		u64 fraction = m2 & mask;
		if (fraction != 0) {
			return false;
		}

		// f is an integer in range [1, 2^53). Note: mantissa might contain trailing (decimal) 0's.
		v->mantissa = m2 >> -e2;
		v->exponent = 0;
		return true;
	}

	export void ryu_f64_dtos(f64 arg, ref<buffer> result) {
		// Step 1: Decode the floating-point number, and unify normalized and subnormal cases.
		u64 bits = cast<u64>(arg);

		// Decode bits into sign, mantissa, and exponent.
		bool ieeeSign = ((bits >> (DOUBLE_MANTISSA_BITS + DOUBLE_EXPONENT_BITS)) & 1) != 0;
		u64 ieeeMantissa = bits & ((1ull << DOUBLE_MANTISSA_BITS) - 1);
		u32 ieeeExponent = (u32) ((bits >> DOUBLE_MANTISSA_BITS) & ((1u << DOUBLE_EXPONENT_BITS) - 1));
		// Case distinction; exit early for the easy cases.
		if (ieeeExponent == ((1u << DOUBLE_EXPONENT_BITS) - 1u) || (ieeeExponent == 0 && ieeeMantissa == 0)) {
			copy_special_str(result, ieeeSign, ieeeExponent, ieeeMantissa);
			return;
		}

		f64_decimal v;
		bool isSmallInt = d2d_small_int(ieeeMantissa, ieeeExponent, &v);

		// small integers in range [1, 2^53), v.mantissa may contain trailing (decimal) zeros.
		// For scientific notation we need to move these zeros into the exponent.
		if (isSmallInt) {
			while (true) {
				u64 q = div10(v.mantissa);
				u32 r = ((u32) v.mantissa) - 10 * ((u32) q);
				if (r != 0) break;
				v.mantissa = q;
				++v.exponent;
			}
		} else {
			v = d2d(ieeeMantissa, ieeeExponent);
		}

		to_chars(v, ieeeSign, result);
	}
}
