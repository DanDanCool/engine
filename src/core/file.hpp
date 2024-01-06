module;

#include "core.h"

export core.file;

import core.string;
import core.memory;
import core.vector;

export namespace core {
	enum class access {
		ro = 1 << 0,
		wo = 1 << 1,
		rw = ro | wo,
		txt = 1 << 2,
		app = 1 << 3,
	};

	ENUM_CLASS_OPERATORS(access);

	struct file {
		file() = default;
		file(cref<string> fname, access _access);
		file(file&& other);
		virtual ~file();

		ref<file> operator=(file&& other) {
			handle = other.handle;
			return *this;
		}

		virtual u32 write(memptr buf);
		virtual u32 read(ref<buffer> buf);
		virtual vector<u8> read();

		void flush();

		core::handle handle;
	};

	struct file_buf : file {
		file_buf() = default;

		file_buf(cref<string> fname, access _access)
		: file(fname, _access), data() {}

		file_buf(file_buf&& other)
		: file() {
			*this = move_data(other);
		}

		~file_buf() = default;

		ref<file_buf> operator=(file_buf&& other) {
			file::operator=(move_data(other));
			data.data = move_data(other.data.data);
			data.index = move_data(other.data.index);

			return *this;
		}

		virtual u32 write(memptr buf) {
			u64 bytes = buf.size;
			buf = data.write(buf);
			bytes -= buf.size;

			while (buf.size) {
				u32 tmp = file::write(memptr{ data.data, data.index });
				if (!tmp) return (u32)bytes;
				bytes += tmp;

				data.flush();
				buf = data.write(buf);
			}

			return (u32)bytes;
		}

		virtual u32 read(ref<buffer> buf) {
			int fd = get_fd(handle);
			int bytes = _read(fd, buf.data + buf.index, (u32)(buffer::size - buf.index));
			buf.index += bytes;
			return bytes;
		}

		buffer data;
	};

	file_buf fopen(cref<string> fname, access _access) {
	return move_data(file_buf(fname, _access));
	}

	file fopen_raw(cref<string> fname, access _access);
}
