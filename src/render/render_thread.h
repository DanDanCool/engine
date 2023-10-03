#pragma once

#include <engine/system.h>

#include <core/core.h>
#include <core/atom.h>
#include <core/memory.h>

#include <win32/window.h>

namespace jolly {
	struct render_thread : public system_thread {
		render_thread();
		~render_thread();

		virtual void init();
		virtual void term();

		virtual void run();
		virtual void step(f32 ms);

		core::ptr<window> _window;
		core::atom<bool> _run;
	};
}
