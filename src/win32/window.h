#pragma once

#include <core/memory.h>
#include <core/tuple.h>
#include <core/string.h>
#include <core/vector.h>
#include <core/table.h>
#include <core/lock.h>

namespace jolly {
	enum class win_event {
		close,
		destroy,
	};

	struct window;
	typedef void (*pfn_win_cb)(ref<window> state, u32 id, win_event event);

	struct window {
		window() = default;
		window(cref<core::string> name, core::pair<u32, u32> sz = {1280, 720});
		~window();

		void _defaults();

		u32 create(cref<core::string> name, core::pair<u32, u32> sz = {1280, 720});

		void step(f32 ms);

		void vsync(bool sync);
		bool vsync() const;

		void callback(u32 id, win_event event);
		void addcb(win_event event, pfn_win_cb cb);

		void size(core::pair<u32, u32> sz);
		core::pair<u32, u32> size() const;

		core::ptr<void> handle;
		core::table<u32, core::ptr<void>> windows;
		core::table<win_event, core::vector<pfn_win_cb>> callbacks;
		core::mutex lock;
	};
}
