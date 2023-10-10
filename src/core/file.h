#pragma once

#include "core.h"
#include "string.h"
#include "memory.h"
#include "vector.h"

namespace core {
	enum class access {
		ro = 1 << 0,
		wo = 1 << 1,
		rw = ro | wo,
		txt = 1 << 2,
		app = 1 << 3,
	};

	struct file {
		file() = default;
		file(cref<string> fname, access _access);
		file(file&& other);
		virtual ~file();

		ref<file> operator=(file&& other);

		virtual u32 write(memptr buf);
		virtual u32 read(ref<buffer> buf);
		virtual vector<u8> read();

		void flush();

		ptr<void> handle;
	};

	struct file_buf : file {
		file_buf() = default;
		file_buf(cref<string> fname, access _access);
		file_buf(file_buf&& other);
		~file_buf() = default;

		ref<file_buf> operator=(file_buf&& other);

		virtual u32 write(memptr buf);
		virtual u32 read(ref<buffer> buf);
		virtual vector<u8> read();

		buffer data;
	};

	file_buf fopen(cref<string> fname, access _access);
	file fopen_raw(cref<string> fname, access _access);
}
