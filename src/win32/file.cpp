#include <core/file.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <io.h>

namespace core {
	ENUM_CLASS_OPERATORS(access);
	static int convert_flags(access _access);
	static int get_fd(cref<ptr<void>> handle);

	file::file(cref<string> fname, access _access)
	: handle() {
		int fd = 0;
		int oflag = convert_flags(_access) | _O_CREAT;
		_sopen_s(&fd, fname, oflag, _SH_DENYNO, _S_IWRITE);

		handle = (void*)(i64)fd;
	}

	file::~file() {
		if (!handle) return;
		int fd = get_fd(handle);
		_close(fd);
		handle = nullptr;
	}

	u32 file::write(memptr buf) {
		int fd = get_fd(handle);
		int bytes = _write(fd, buf.data, (u32)buf.size);
		return bytes;
	}

	buffer file::read() {
		int fd = get_fd(handle);

		buffer buf;
		int bytes = _read(fd, buf.data, buffer::size);
		buf.index = bytes;
		return buf;
	}

	u32 file::read(ref<buffer> buf) {
		int fd = get_fd(handle);
		int bytes = _read(fd, buf.data + buf.index, (u32)(buffer::size - buf.index));
		buf.index += bytes;
		return bytes;
	}

	void file::flush() {
		int fd = get_fd(handle);
		_commit(fd);
	}

	file_buf::file_buf(cref<string> fname, access _access)
	: file(fname, _access), data() {}

	u32 file_buf::write(memptr buf) {
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

	buffer file_buf::read() {
		return file::read();
	}

	u32 file_buf::read(ref<buffer> buf){
		return file::read(buf);
	}

	static int convert_flags(access _access) {
		int read = 0;
		read |= cast<bool>(_access & access::ro) ? _O_RDONLY : 0;
		read |= cast<bool>(_access & access::wo) ? _O_WRONLY : 0;
		read = ((_access & access::rw) == access::rw) ? _O_RDWR : read;

		int oflag = read;
		oflag |= cast<bool>(_access & access::txt) ? _O_TEXT : _O_BINARY;
		oflag |= cast<bool>(_access & access::app) ? _O_APPEND : 0;
		return oflag;
	}

	static int get_fd(cref<ptr<void>> handle) {
		return (int)(i64)handle.data;
	}
}
