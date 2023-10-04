#pragma once

#include "core.h"
#include "string.h"
#include "memory.h"

namespace core {
	enum class access {
		ro = 1 << 0,
		wo = 1 << 1,
		rw = ro | wo,
		txt = 1 << 2,
		app = 1 << 3,
	};

	struct file {
		file(cref<string> fname, access _access);
		virtual ~file();

		virtual u32 write(memptr buf);
		virtual buffer read();
		virtual u32 read(ref<buffer> buf);

		void flush();

		ptr<void> handle;
	};

	struct file_buf : file {
		file_buf(cref<string> fname, access _access);
		~file_buf() = default;

		virtual u32 write(memptr buf);
		virtual buffer read();
		virtual u32 read(ref<buffer> buf);

		buffer data;
	};

	file_buf fopen(cref<string> fname, access _access);
	file fopen_raw(cref<string> fname, access _access);
}
