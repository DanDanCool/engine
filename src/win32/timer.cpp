module;

#include <windows.h>
#include <profileapi.h>

module core.timer;

namespace core {
	timer::timer(ref<f32> res) : start(-1), result(res) {
		if (frequency == -1) {
			LARGE_INTEGER freq;
			QueryPerformanceFrequency(&freq);
			frequency = freq.QuadPart;
		}

		LARGE_INTEGER tick;
		QueryPerformanceCounter(&tick);
		start = tick.QuadPart;
	}

	timer::~timer() {
		const i64 MILLISECONDS = 1000;

		LARGE_INTEGER tick;
		QueryPerformanceCounter(&tick);

		i64 dt = (tick.QuadPart - start) * MILLISECONDS;
		result = (float)(dt / frequency);
	}
}
