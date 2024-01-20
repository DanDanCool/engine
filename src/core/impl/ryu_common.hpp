// Copyright 2018 Ulf Adams
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

export module core.impl.ryu.common;
import core.types;
import core.memory;
import core.simd;

export namespace core::impl::ryu {
	using fmtbuf = buffer_base<BLOCK_1024>;
	// Returns the number of decimal digits in v, which must not contain more than 9 digits.
	u32 decimalLength9(u32 v) {
	  // Function precondition: v is not a 10-digit number.
	  // (f2s: 9 digits are sufficient for round-tripping.)
	  // (d2fixed: We print 9-digit blocks.)
	  JOLLY_CORE_ASSERT(v < 1000000000);
	  if (v >= 100000000) { return 9; }
	  if (v >= 10000000) { return 8; }
	  if (v >= 1000000) { return 7; }
	  if (v >= 100000) { return 6; }
	  if (v >= 10000) { return 5; }
	  if (v >= 1000) { return 4; }
	  if (v >= 100) { return 3; }
	  if (v >= 10) { return 2; }
	  return 1;
	}

	// Returns e == 0 ? 1 : [log_2(5^e)]; requires 0 <= e <= 3528.
	i32 log2pow5(i32 e) {
	  // This approximation works up to the point that the multiplication overflows at e = 3529.
	  // If the multiplication were done in 64 bits, it would fail at 5^4004 which is just greater
	  // than 2^9297.
	  JOLLY_CORE_ASSERT(e >= 0);
	  JOLLY_CORE_ASSERT(e <= 3528);
	  return (i32)((((u32) e) * 1217359) >> 19);
	}

	// Returns e == 0 ? 1 : ceil(log_2(5^e)); requires 0 <= e <= 3528.
	i32 pow5bits(i32 e) {
	  // This approximation works up to the point that the multiplication overflows at e = 3529.
	  // If the multiplication were done in 64 bits, it would fail at 5^4004 which is just greater
	  // than 2^9297.
	  JOLLY_CORE_ASSERT(e >= 0);
	  JOLLY_CORE_ASSERT(e <= 3528);
	  return (i32)(((((u32) e) * 1217359) >> 19) + 1);
	}

	// Returns e == 0 ? 1 : ceil(log_2(5^e)); requires 0 <= e <= 3528.
	i32 ceil_log2pow5(i32 e) {
	  return log2pow5(e) + 1;
	}

	// Returns floor(log_10(2^e)); requires 0 <= e <= 1650.
	u32 log10Pow2(i32 e) {
	  // The first value this approximation fails for is 2^1651 which is just greater than 10^297.
	  JOLLY_CORE_ASSERT(e >= 0);
	  JOLLY_CORE_ASSERT(e <= 1650);
	  return (((u32) e) * 78913) >> 18;
	}

	// Returns floor(log_10(5^e)); requires 0 <= e <= 2620.
	u32 log10Pow5(i32 e) {
	  // The first value this approximation fails for is 5^2621 which is just greater than 10^1832.
	  JOLLY_CORE_ASSERT(e >= 0);
	  JOLLY_CORE_ASSERT(e <= 2620);
	  return (((u32) e) * 732923) >> 20;
	}

	int copy_special_str(ref<fmtbuf> result, bool sign, bool exponent, bool mantissa) {
	  if (mantissa) {
		  result.write(membuf{(u8*)"nan", 3});
		return 3;
	  }

	  if (sign) {
		  result.write((u8)'-');
	  }

	  if (exponent) {
		  result.write(membuf{(u8*)"inf", 3});
		return sign + 3;
	  }

	  result.write(membuf{(u8*)"0.0", 3});
	  return sign + 3;
	}

	u32 float_to_bits(float f) {
	  return *(u32*)&f;
	}

	u64 double_to_bits(double d) {
	  return *(u64*)&d;
	}
}
