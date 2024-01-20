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

export module core.impl.ryu_f32;
import core.types;
import core.simd;
import core.memory;
import core.iterator;
import core.impl.ryu.table;
import core.impl.ryu.common;

namespace core::impl::ryu {
	constexpr u64 FLOAT_MANTISSA_BITS = 23;
	constexpr u64 FLOAT_EXPONENT_BITS = 8;
	constexpr u64 FLOAT_EXPONENT_BIAS = 127;

	constexpr u64 FLOAT_POW5_INV_BITCOUNT = DOUBLE_POW5_INV_BITCOUNT - 64;
	constexpr u64 FLOAT_POW5_BITCOUNT = DOUBLE_POW5_BITCOUNT - 64;

	u32 floor_log2(u32 value) {
#if defined(_MSC_VER)
		unsigned long index;
		return _BitScanReverse(&index, value) ? index : 32;
#else
		return 31 - __builtin_clz(value);
#endif
	}

	u32 pow5factor_32(u32 value) {
		u32 count = 0;
		while (true) {
			JOLLY_CORE_ASSERT(value != 0);
			u32 q = value / 5;
			u32 r = value % 5;
			if (r != 0) break;
			value = q;
			count++;
		}

		return count;
	}

	// Returns true if value is divisible by 5^p.
	bool multipleOfPowerOf5_32(u32 value, u32 p) {
		return pow5factor_32(value) >= p;
	}

	// Returns true if value is divisible by 2^p.
	bool multipleOfPowerOf2_32(u32 value, u32 p) {
		return (value & ((1u << p) - 1)) == 0;
	}

	u32 mulShift32(u32 m, u64 factor, i32 shift) {
		JOLLY_CORE_ASSERT(shift > 32);

		u32 factorLo = (u32)(factor);
		u32 factorHi = (u32)(factor >> 32);
		u64 bits0 = (u64)m * factorLo;
		u64 bits1 = (u64)m * factorHi;

		u64 sum = (bits0 >> 32) + bits1;
		u64 shiftedSum = sum >> (shift - 32);
		JOLLY_CORE_ASSERT(shiftedSum <= U32_MAX);
		return (u32) shiftedSum;
	}

	u32 mulPow5InvDivPow2(u32 m, u32 q, i32 j) {
		return mulShift32(m, DOUBLE_POW5_INV_SPLIT[q][1] + 1, j);
	}

	u32 mulPow5divPow2(u32 m, u32 i, i32 j) {
		return mulShift32(m, DOUBLE_POW5_SPLIT[i][1], j);
	}

	struct f32_decimal {
		u32 mantissa;
		i32 exponent;
	};

	f32_decimal f2d(u32 ieeeMantissa, u32 ieeeExponent) {
		i32 e2 = (i32) ieeeExponent - FLOAT_EXPONENT_BIAS - FLOAT_MANTISSA_BITS - 2;
		u32 m2 = (1u << FLOAT_MANTISSA_BITS) | ieeeMantissa;

		// We subtract 2 so that the bounds computation has 2 additional bits.
		if (ieeeExponent == 0) {
			e2 = (i32)(1 - FLOAT_EXPONENT_BIAS - FLOAT_MANTISSA_BITS - 2);
			m2 = ieeeMantissa;
		}

		bool even = (m2 & 1) == 0;
		bool acceptBounds = even;

		// Step 2: Determine the interval of valid decimal representations.
		u32 mv = 4 * m2;
		u32 mp = 4 * m2 + 2;
		// Implicit bool -> int conversion. True is 1, false is 0.
		u32 mmShift = ieeeMantissa != 0 || ieeeExponent <= 1;
		u32 mm = 4 * m2 - 1 - mmShift;

		// Step 3: Convert to a decimal power base using 64-bit arithmetic.
		u32 vr, vp, vm;
		i32 e10;
		bool vmIsTrailingZeros = false;
		bool vrIsTrailingZeros = false;
		u8 lastRemovedDigit = 0;

		if (e2 >= 0) {
			u32 q = log10Pow2(e2);
			e10 = (i32) q;
			i32 k = FLOAT_POW5_INV_BITCOUNT + pow5bits((i32) q) - 1;
			i32 i = -e2 + (i32) q + k;

			vr = mulPow5InvDivPow2(mv, q, i);
			vp = mulPow5InvDivPow2(mp, q, i);
			vm = mulPow5InvDivPow2(mm, q, i);

			// We need to know one removed digit even if we are not going to loop below.
			if (q != 0 && (vp - 1) / 10 <= vm / 10) {
				i32 l = FLOAT_POW5_INV_BITCOUNT + pow5bits((i32) (q - 1)) - 1;
				lastRemovedDigit = (u8) (mulPow5InvDivPow2(mv, q - 1, -e2 + (i32) q - 1 + l) % 10);
			}

			if (q <= 9) {
				// The largest power of 5 that fits in 24 bits is 5^10, but q <= 9 seems to be safe as well.
				// Only one of mp, mv, and mm can be a multiple of 5, if any.
				if (mv % 5 == 0) {
					vrIsTrailingZeros = multipleOfPowerOf5_32(mv, q);
				} else if (acceptBounds) {
					vmIsTrailingZeros = multipleOfPowerOf5_32(mm, q);
				} else {
					vp -= multipleOfPowerOf5_32(mp, q);
				}
			}
		} else {
			u32 q = log10Pow5(-e2);
			e10 = (i32) q + e2;
			i32 i = -e2 - (i32) q;
			i32 k = pow5bits(i) - FLOAT_POW5_BITCOUNT;
			i32 j = (i32) q - k;
			vr = mulPow5divPow2(mv, (u32) i, j);
			vp = mulPow5divPow2(mp, (u32) i, j);
			vm = mulPow5divPow2(mm, (u32) i, j);

			if (q != 0 && (vp - 1) / 10 <= vm / 10) {
				j = (i32) q - 1 - (pow5bits(i + 1) - FLOAT_POW5_BITCOUNT);
				lastRemovedDigit = (u8) (mulPow5divPow2(mv, (u32) (i + 1), j) % 10);
			}

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
			} else if (q < 31) { // TODO(ulfjack): Use a tighter bound here.
				vrIsTrailingZeros = multipleOfPowerOf2_32(mv, q - 1);
			}
		}

		// Step 4: Find the shortest decimal representation in the interval of valid representations.
		i32 removed = 0;
		u32 output;

		// General case, which happens rarely (~4.0%).
		if (vmIsTrailingZeros || vrIsTrailingZeros) {
			while (vp / 10 > vm / 10) {
				vmIsTrailingZeros &= vm % 10 == 0;
				vrIsTrailingZeros &= lastRemovedDigit == 0;
				lastRemovedDigit = (u8) (vr % 10);
				vr /= 10;
				vp /= 10;
				vm /= 10;
				++removed;
			}

			if (vmIsTrailingZeros) {
				while (vm % 10 == 0) {
					vrIsTrailingZeros &= lastRemovedDigit == 0;
					lastRemovedDigit = (u8) (vr % 10);
					vr /= 10;
					vp /= 10;
					vm /= 10;
					++removed;
				}
			}

			// Round even if the exact number is .....50..0.
			if (vrIsTrailingZeros && lastRemovedDigit == 5 && vr % 2 == 0) {
				lastRemovedDigit = 4;
			}

			// We need to take vr + 1 if vr is outside bounds or we need to round up.
			output = vr + ((vr == vm && (!acceptBounds || !vmIsTrailingZeros)) || lastRemovedDigit >= 5);
		} else {
			// Specialized for the common case (~96.0%). Percentages below are relative to this.
			while (vp / 10 > vm / 10) {
				lastRemovedDigit = (u8) (vr % 10);
				vr /= 10;
				vp /= 10;
				vm /= 10;
				++removed;
			}
			// We need to take vr + 1 if vr is outside bounds or we need to round up.
			output = vr + (vr == vm || lastRemovedDigit >= 5);
		}

		i32 exp = e10 + removed;
		return f32_decimal{ output, exp };
	}

	void to_chars(f32_decimal v, bool sign, ref<buffer> buffer) {
		if (sign) {
			buffer.write('-');
		}

		u64 index = buffer.index;
		u32 output = v.mantissa;
		u32 olength = decimalLength9(output);
		buffer.write('0', olength + 1);

		// Print the decimal digits.
		u64 i = buffer.index;
		while (output >= 10000) {
			u32 c = output % 10000;
			output /= 10000;
			u32 c0 = (c % 100) << 1;
			u32 c1 = (c / 100) << 1;
			buffer.index = i -= 2;
			buffer.write(membuf{(u8*)(DIGIT_TABLE + c0), 2});
			buffer.index = i -= 2;
			buffer.write(membuf{(u8*)(DIGIT_TABLE + c1), 2});
		}

		if (output >= 100) {
			u32 c = (output % 100) << 1;
			output /= 100;
			buffer.index = i -= 2;
			buffer.write(membuf{(u8*)(DIGIT_TABLE + c), 2});
		}

		if (output >= 10) {
			u32 c = output << 1;
			buffer[i -= 1] = DIGIT_TABLE[c + 1];
			buffer[index] = DIGIT_TABLE[c];
		} else {
			buffer[index] = '0' + output;
		}

		// Print decimal point if needed.
		if (olength > 1) {
			buffer[index + 1] = '.';
			buffer.index = index + olength + 1;
		} else {
			buffer.index = index + 1;
		}

		// Print the exponent.
		buffer.write('e');
		i32 exp = v.exponent + (i32) olength - 1;
		if (exp < 0) {
			buffer.write('-');
			exp = -exp;
		}

		if (exp >= 10) {
			buffer.write(membuf{(u8*)(DIGIT_TABLE + 2 * exp), 2});
		} else {
			buffer.write('0' + exp);
		}
	}

	export void ryu_f32_ftos(f32 arg, ref<buffer> buffer) {
		// Step 1: Decode the floating-point number, and unify normalized and subnormal cases.
		u32 bits = float_to_bits(arg);

		// Decode bits into sign, mantissa, and exponent.
		bool ieeeSign = ((bits >> (FLOAT_MANTISSA_BITS + FLOAT_EXPONENT_BITS)) & 1) != 0;
		u32 ieeeMantissa = bits & ((1u << FLOAT_MANTISSA_BITS) - 1);
		u32 ieeeExponent = (bits >> FLOAT_MANTISSA_BITS) & ((1u << FLOAT_EXPONENT_BITS) - 1);

		// Case distinction; exit early for the easy cases.
		if (ieeeExponent == ((1u << FLOAT_EXPONENT_BITS) - 1u) || (ieeeExponent == 0 && ieeeMantissa == 0)) {
			copy_special_str(buffer, ieeeSign, ieeeExponent, ieeeMantissa);
			return;
		}

		f32_decimal v = f2d(ieeeMantissa, ieeeExponent);
		to_chars(v, ieeeSign, buffer);
	}

	f32 int32Bits2Float(u32 bits) {
		return *(f32*)&bits;
	}

	export f32 ryu_f32_stof(cstr buffer) {
		int m10digits = 0;
		int e10digits = 0;
		int dotIndex = -1;
		int eIndex = -1;
		int dot_underscore = 0;
		int e_underscore = 0;
		u32 m10 = 0;
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
				JOLLY_CORE_ASSERT(m10digits < 9);
				m10 = 10 * m10 + digit;
				m10digits += m10 != 0;
			} else {
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

		// Number is less than 1e-46, which should be rounded down to 0; return +/-0.0.
		if ((m10digits + e10 <= -46) || (m10 == 0)) {
			u32 ieee = ((u32) signedM) << (FLOAT_EXPONENT_BITS + FLOAT_MANTISSA_BITS);
			return int32Bits2Float(ieee);
		}

		// Number is larger than 1e+39, which should be rounded to +/-Infinity.
		if (m10digits + e10 >= 40) {
			u32 ieee = (((u32) signedM) << (FLOAT_EXPONENT_BITS + FLOAT_MANTISSA_BITS)) | (0xffu << FLOAT_MANTISSA_BITS);
			return int32Bits2Float(ieee);
		}

		// Convert to binary float m2 * 2^e2, while retaining information about whether the conversion was exact (trailingZeros).
		i32 e2;
		u32 m2;
		bool trailingZeros;
		if (e10 >= 0) {
			e2 = floor_log2(m10) + e10 + log2pow5(e10) - (FLOAT_MANTISSA_BITS + 1);
			int j = e2 - e10 - ceil_log2pow5(e10) + FLOAT_POW5_BITCOUNT;
			JOLLY_CORE_ASSERT(j >= 0);
			m2 = mulPow5divPow2(m10, e10, j);
			trailingZeros = e2 < e10 || (e2 - e10 < 32 && multipleOfPowerOf2_32(m10, e2 - e10));
		} else {
			e2 = floor_log2(m10) + e10 - ceil_log2pow5(-e10) - (FLOAT_MANTISSA_BITS + 1);
			int j = e2 - e10 + ceil_log2pow5(-e10) - 1 + FLOAT_POW5_INV_BITCOUNT;
			m2 = mulPow5InvDivPow2(m10, -e10, j);
			trailingZeros = (e2 < e10 || (e2 - e10 < 32 && multipleOfPowerOf2_32(m10, e2 - e10)))
			&& multipleOfPowerOf5_32(m10, -e10);
		}

		// Compute the final IEEE exponent.
		u32 ieee_e2 = max<u32>(0, e2 + FLOAT_EXPONENT_BIAS + floor_log2(m2));

		// Final IEEE exponent is larger than the maximum representable; return +/-Infinity.
		if (ieee_e2 > 0xfe) {
			u32 ieee = (((u32) signedM) << (FLOAT_EXPONENT_BITS + FLOAT_MANTISSA_BITS)) | (0xffu << FLOAT_MANTISSA_BITS);
			return int32Bits2Float(ieee);
		}

		// We need to figure out how much we need to shift m2. The tricky part is that we need to take
		// the final IEEE exponent into account, so we need to reverse the bias and also special-case
		// the value 0.
		i32 shift = (ieee_e2 == 0 ? 1 : ieee_e2) - e2 - FLOAT_EXPONENT_BIAS - FLOAT_MANTISSA_BITS;
		JOLLY_CORE_ASSERT(shift >= 0);

		// We need to round up if the exact value is more than 0.5 above the value we computed
		// We need to update trailingZeros given that we have the exact output exponent ieee_e2 now.
		trailingZeros &= (m2 & ((1u << (shift - 1)) - 1)) == 0;
		u32 lastRemovedBit = (m2 >> (shift - 1)) & 1;
		bool roundUp = (lastRemovedBit != 0) && (!trailingZeros || (((m2 >> shift) & 1) != 0));

		u32 ieee_m2 = (m2 >> shift) + roundUp;
		JOLLY_CORE_ASSERT(ieee_m2 <= (1u << (FLOAT_MANTISSA_BITS + 1)));
		ieee_m2 &= (1u << FLOAT_MANTISSA_BITS) - 1;

		// Rounding up may overflow the mantissa.
		// In this case we move a trailing zero of the mantissa into the exponent.
		// Due to how the IEEE represents +/-Infinity, we don't need to check for overflow here.
		if (ieee_m2 == 0 && roundUp) {
			ieee_e2++;
		}

		u32 ieee = (((((u32) signedM) << FLOAT_EXPONENT_BITS) | (u32)ieee_e2) << FLOAT_MANTISSA_BITS) | ieee_m2;
		return int32Bits2Float(ieee);
	}
}
