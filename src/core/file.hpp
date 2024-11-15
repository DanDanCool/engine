module;

#include "core.h"

export module core.file;
import core.types;
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

	struct file_base {
		file_base() = default;
		file_base(stringview fname, access _access);

		file_base(fwd<file_base> other)
		: handle() {
			*this = forward_data(other);
		}

		virtual ~file_base();

		ref<file_base> operator=(fwd<file_base> other) {
			handle = other.handle;
			return *this;
		}

		virtual u32 write(membuf buf);
		virtual u32 read(ref<buffer> buf);
		virtual u32 read(ref<vector<u8>> buf); // read everything

		void flush();

		core::handle handle;
	};

	struct file : file_base {
		file() = default;

		file(stringview fname, access _access)
		: file_base(fname, _access), data() {}

		file(fwd<file> other)
		: file_base() {
			*this = forward_data(other);
		}

		~file() {
			if (data.index) {
				write();
			}
		}

		ref<file> operator=(fwd<file> other) {
			data = forward_data(other.data);
			file_base::operator=(forward_data(other));

			return *this;
		}

		virtual u32 write(membuf buf) {
			u32 bytes = buf.size;
			buf = data.write(buf);
			bytes -= buf.size;

			while (buf.size) {
				u32 tmp = file_base::write(membuf{ data.data, data.index });
				if (!tmp) return (u32)bytes;
				bytes += tmp;

				data.flush();
				buf = data.write(buf);
			}

			return bytes;
		}

		u32 write() {
			u32 bytes = file_base::write(membuf{ data.data, data.index });
			data.flush();
			return bytes;
		}

		u32 write(u8 c) {
			return write(membuf{&c, 1});
		}

		u32 read(ref<buffer> buf) {
			return file_base::read(buf);
		}

		u32 read(ref<vector<u8>> buf) {
			return file_base::read(buf);
		}

		u32 read() {
			return file_base::read(data);
		}

		buffer data;
	};

	file fopen(stringview fname, access _access) {
		return file(fname, _access);
	}

	file_base fopen_raw(stringview fname, access _access);
}
