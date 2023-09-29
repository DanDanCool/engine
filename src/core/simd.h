#pragma once

#include <immintrin.h>
#include "memory.h"
#include "core.h"

#define ALIGN_256 32

namespace core {
	struct v256i {
		alignas(ALIGN_256) u64 data[4];
	};
}
