#include "core.h"

import core.log;
import core.types;

namespace assert {
	cstr message(cstr msg) {
		return msg;
	}

	void callback(cstr expr, cstr file, int line, cstr message) {
		LOG_CRIT("% failed! in file: %, line: %\n%", expr, file, line, message);
	}
}
