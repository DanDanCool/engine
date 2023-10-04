#include "window.h"

#include <core/log.h>
#include <engine/engine.h>

#include <windows.h>
#include <winuser.h>

#include <vulkan/vulkan.h>

namespace jolly {
	const auto* WNDCLASS_NAME = L"JOLLY_WNDCLASS";

	static void windestroycb(ref<window> state, u32 id, win_event event);
	static LRESULT wndproc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);

	window::window(cref<core::string> name, core::pair<u32, u32> sz)
	: handle(), windows(), callbacks(), lock() {
		handle = (void*)GetModuleHandle(NULL);

		WNDCLASSEXW info = {};
		info.cbSize = sizeof(WNDCLASSEXW);
		info.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		info.lpfnWndProc = (WNDPROC)wndproc;
		info.hInstance = (HINSTANCE)handle.data;
		info.lpszClassName = WNDCLASS_NAME;

		RegisterClassExW(&info);
		create(name, sz);

		_defaults();
	}

	window::~window() {
		// we stop looking through the message queue so delete manually
		for (auto& [i, hwnd] : windows) {
			windestroycb(*this, i, win_event::destroy);
			DestroyWindow((HWND)hwnd.data);
			hwnd = nullptr;
		}

		UnregisterClassW(WNDCLASS_NAME, (HINSTANCE)handle.data);
		handle = nullptr;
	}

	void window::_defaults() {
		auto closecb = [](ref<window> state, u32 id, win_event event) {
			DestroyWindow((HWND)state.windows[id].data);
		};

		addcb(win_event::close, closecb);
		addcb(win_event::destroy, windestroycb);
	}

	u32 window::create(cref<core::string> name, core::pair<u32, u32> sz) {
		core::lock l(lock);
		u32 id = windows.size;

		core::string_base<i16> wname = name.cast<i16>();

		DWORD exstyle = WS_EX_APPWINDOW;
        DWORD style   = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
		HWND win = CreateWindowExW(
				exstyle,
				WNDCLASS_NAME,
				(LPCWSTR)wname.data,
				style,
				CW_USEDEFAULT, CW_USEDEFAULT,
				sz.one, sz.two,
				NULL, NULL, (HINSTANCE)handle.data, NULL
				);

		core::ptr<u32> data = core::ptr_create<u32>(id);
		SetPropW(win, L"id", (HANDLE)data.data);
		data = nullptr;
		SetPropW(win, L"state", (HANDLE)this);
		ShowWindow(win, SW_SHOW);
		UpdateWindow(win);

		windows[id] = core::ptr<void>((void*)win);
		return id;
	}

	void window::step(f32 ms) {
		MSG msg;
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				LOG_INFO("wm_quit");
				ref<engine> instance = engine::instance();
				instance.stop();
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	core::vector<cstr> window::vkextensions(cref<core::vector<VkExtensionProperties>> extensions) {
		bool surface = false, win32_surface = false;
		for (auto& extension : extensions) {
			if (core::cmpeq(extension.extensionName, "VK_KHR_surface")) {
				surface = true;
			}

			if (core::cmpeq(extension.extensionName, "VK_KHR_win32_surface")) {
				win32_surface = true;
			}
		}

		assert(surface && win32_surface);

		return core::vector{ "VK_KHR_surface", "VK_KHR_win32_surface" };
	}

	void window::vsync(bool sync) {

	}

	bool window::vsync() const {
		return true;
	}

	void window::size(core::pair<u32, u32> sz) {

	}

	core::pair<u32, u32> window::size() const {
		return core::pair<u32, u32>(0, 0);
	}

	void window::callback(u32 id, win_event event) {
		if (!callbacks.has(event)) return;
		core::lock l(lock);
		for (auto cb : callbacks[event]) {
			cb(*this, id, event);
		}
	}

	void window::addcb(win_event event, pfn_win_cb cb) {
		callbacks[event].add(cb);
	}

	static void windestroycb(ref<window> state, u32 id, win_event event) {
		core::ptr<void> hwnd = state.windows[id];
		RemovePropW((HWND)hwnd.data, L"state");

		PROPENUMPROCW delprop = [](HWND hwnd, LPCWSTR name, HANDLE handle) -> BOOL {
			core::ptr<void> del = RemovePropW(hwnd, name);
			return 1;
		};

		EnumPropsW((HWND)hwnd.data, delprop);

		state.windows.del(id);
		hwnd = nullptr;

		if (!state.windows.size) {
			PostQuitMessage(0);
		}
	}

	LRESULT wndproc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam) {
		void* _state = GetPropW(hwnd, L"state");
		void* _id = GetPropW(hwnd, L"id");

		if (!_state || !_id) {
			return DefWindowProc(hwnd, umsg, wparam, lparam);
		}

		ref<window> state = cast<window>(_state);
		u32 id = cast<u32>(_id);

		switch (umsg) {
			case WM_CLOSE: {
				// do something here
				LOG_INFO("wm_close");
				state.callback(id, win_event::close);
				break;
			}

			case WM_DESTROY: {
				state.callback(id, win_event::destroy);
				break;
			}

			default: {
				return DefWindowProc(hwnd, umsg, wparam, lparam);
			}
		};

		return 0;
	}
}
