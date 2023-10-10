#pragma once

#include <core/memory.h>
#include <core/tuple.h>
#include <core/string.h>
#include <core/vector.h>
#include <core/table.h>
#include <core/lock.h>

#include <vulkan/vulkan.h>

namespace jolly {
	enum class win_event {
		close,
		destroy,
		create,
		resize,
	};

	struct window;
	struct vk_device;
	struct vk_surface;
	struct vk_swapchain;
	typedef void (*pfn_win_cb)(ref<window> state, u32 id, win_event event);

	struct window {
		window() = default;
		window(cref<core::string> name, core::pair<u32, u32> sz = {1280, 720});
		~window();

		void _defaults();

		u32 create(cref<core::string> name, core::pair<u32, u32> sz = {1280, 720});

		void step(f32 ms);

		void vk_init(cref<vk_device> device); // create surfaces
		void vk_term();
		void vk_swapchain_create(); // create swapchains
		ref<vk_swapchain> vk_main_swapchain() const;
		core::vector<cstr> vk_extensions(cref<core::vector<VkExtensionProperties>> extensions) const;

		void vsync(bool sync);
		bool vsync() const;

		void callback(u32 id, win_event event);
		void add_cb(win_event event, pfn_win_cb cb);

		void size(u32 id, core::pair<u32, u32> sz);
		core::pair<u32, u32> size(u32 id) const;
		core::pair<u32, u32> fbsize(u32 id) const; // size of framebuffer

		core::ptr<void> handle;
		core::ptr<vk_surface> surface; // stores rendering device info
		core::table<u32, core::ptr<void>> windows;
		core::table<win_event, core::vector<pfn_win_cb>> callbacks;
		core::mutex busy;
	};

	void vk_swapchain_create_cb(ref<window> state, u32 id, win_event event);
	void vk_framebuffer_create_cb(ref<window> state, u32 id, win_event event);
	void vk_swapchain_destroy_cb(ref<window> state, u32 id, win_event event);
}
