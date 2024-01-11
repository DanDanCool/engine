module;

#include <core/core.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>

module core.file;

namespace core {
	int convert_flags(access _access);
	int get_fd(cref<handle> handle);

	file_base::file_base(cref<string> fname, access _access)
	: handle() {
		int fd = 0;
		int oflag = convert_flags(_access) | _O_CREAT;
		_sopen_s(&fd, fname, oflag, _SH_DENYNO, _S_IWRITE);

		handle = (void*)(i64)fd;
	}

	file_base::~file_base() {
		if (!handle) return;
		int fd = get_fd(handle);
		_close(fd);
	}

	u32 file_base::write(memptr buf) {
		int fd = get_fd(handle);
		int bytes = _write(fd, buf.data, (u32)buf.size);
		return bytes;
	}

	// if buf already has memory allocated this will cause a memory leak
	u32 file_base::read(ref<vector<u8>> buf) {
		int fd = get_fd(handle);

		i32 bytes = (i32)_lseek(fd, 0, SEEK_END);
		JOLLY_ASSERT(bytes > 0, "file seek failed!");
		buf = forward_data(vector<u8>(bytes));
		buf.size = bytes;
		bytes = (i32)_lseek(fd, 0, SEEK_SET);
		JOLLY_ASSERT(bytes == 0, "file seek failed!");
		bytes = _read(fd, buf.data, buf.size);

		return bytes;
	}

	u32 file_base::read(ref<buffer> buf) {
		int fd = get_fd(handle);
		int bytes = _read(fd, buf.data + buf.index, (u32)(buffer::size - buf.index));
		buf.index += bytes;
		return bytes;
	}

	void file_base::flush() {
		int fd = get_fd(handle);
		_commit(fd);
	}

	int convert_flags(access _access) {
		int read = 0;
		read |= cast<bool>(_access & access::ro) ? _O_RDONLY : 0;
		read |= cast<bool>(_access & access::wo) ? _O_WRONLY : 0;
		read = ((_access & access::rw) == access::rw) ? _O_RDWR : read;

		int oflag = read;
		oflag |= cast<bool>(_access & access::txt) ? _O_TEXT : _O_BINARY;
		oflag |= cast<bool>(_access & access::app) ? _O_APPEND : 0;
		return oflag;
	}

	int get_fd(cref<handle> handle) {
		return (int)(i64)handle.data();
	}
}
