module;

#include <core/core.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>

module core.file;

namespace core {
	static int convert_flags(access _access);
	static int get_fd(cref<ptr<void>> handle);

	file::file(cref<string> fname, access _access)
	: handle() {
		int fd = 0;
		int oflag = convert_flags(_access) | _O_CREAT;
		_sopen_s(&fd, fname, oflag, _SH_DENYNO, _S_IWRITE);

		handle = (void*)(i64)fd;
	}

	file::file(file&& other)
	: handle() {
		*this = move_data(other);
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

	vector<u8> file::read() {
		int fd = get_fd(handle);

		i32 bytes = (i32)_lseek(fd, 0, SEEK_END);
		JOLLY_ASSERT(bytes >= 0, "file seek failed!");
		vector<u8> buffer(bytes);
		buffer.size = bytes;
		bytes = (i32)_lseek(fd, 0, SEEK_SET);
		JOLLY_ASSERT(bytes >= 0, "file seek failed!");
		_read(fd, buffer.data, buffer.size);

		return move_data(buffer);
	}

	void file::flush() {
		int fd = get_fd(handle);
		_commit(fd);
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
