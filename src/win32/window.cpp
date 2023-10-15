#include "window.h"

#include <core/log.h>
#include <engine/engine.h>

#include <windows.h>
#include <winuser.h>

#include <render/vk_device.h>
#include <render/vk_surface.h>
#include <vulkan/vulkan.h>

typedef VkFlags VkWin32SurfaceCreateFlagsKHR;
struct VkWin32SurfaceCreateInfoKHR {
    VkStructureType                 sType;
    const void*                     pNext;
    VkWin32SurfaceCreateFlagsKHR    flags;
    HINSTANCE                       hinstance;
    HWND                            hwnd;
};

typedef VkResult (APIENTRY *PFN_vkCreateWin32SurfaceKHR)(VkInstance,const VkWin32SurfaceCreateInfoKHR*,const VkAllocationCallbacks*,VkSurfaceKHR*);
static VkResult vkCreateWin32SurfaceKHR(
		VkInstance instance,
		const VkWin32SurfaceCreateInfoKHR* info,
		const VkAllocationCallbacks* callbacks,
		VkSurfaceKHR* surface) {
	auto fn = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");
	JOLLY_ASSERT(fn, "could not load vkCreateWin32SurfaceKHR");
	return fn(instance, info, callbacks, surface);
}

namespace jolly {
	const auto* WNDCLASS_NAME = L"JOLLY_WNDCLASS";

	static void win_destroy_cb(ref<window> state, u32 id, win_event event);
	static LRESULT wndproc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);

	window::window(cref<core::string> name, math::vec2i sz)
	: handle()
	, surface()
	, windows()
	, callbacks()
	, busy() {
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
			win_destroy_cb(*this, i, win_event::destroy);
			DestroyWindow((HWND)hwnd.data);
			hwnd = nullptr;
		}

		UnregisterClassW(WNDCLASS_NAME, (HINSTANCE)handle.data);
		handle = nullptr;
	}

	void window::_defaults() {
		auto close_cb = [](ref<window> state, u32 id, win_event event) {
			DestroyWindow((HWND)state.windows[id].data);
		};

		add_cb(win_event::close, close_cb);
		add_cb(win_event::destroy, win_destroy_cb);
	}

	u32 window::create(cref<core::string> name, math::vec2i sz) {
		core::lock lock(busy);
		u32 id = windows.size;

		core::string_base<i16> wname = name.cast<i16>();

		DWORD exstyle = WS_EX_APPWINDOW;
        DWORD style   = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX
			| WS_CAPTION | WS_MAXIMIZEBOX | WS_THICKFRAME;
		HWND win = CreateWindowExW(
				exstyle,
				WNDCLASS_NAME,
				(LPCWSTR)wname.data,
				style,
				CW_USEDEFAULT, CW_USEDEFAULT,
				sz.x, sz.y,
				NULL, NULL, (HINSTANCE)handle.data, NULL
				);

		core::ptr<u32> data = core::ptr_create<u32>(id);
		SetPropW(win, L"id", (HANDLE)data.data);
		data = nullptr;
		SetPropW(win, L"state", (HANDLE)this);
		ShowWindow(win, SW_SHOW);
		UpdateWindow(win);

		windows[id] = core::ptr<void>((void*)win);
		callback(id, win_event::create);
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

	void window::vk_init(cref<vk_device> device) {
		surface = core::ptr_create<vk_surface>(device);

		auto create_cb = [](ref<window> state, u32 id, win_event event) {
			ref<vk_surface> surface = state.surface.ref();

			VkSurfaceKHR tmp = VK_NULL_HANDLE;
			VkWin32SurfaceCreateInfoKHR info{};
			info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			info.hwnd = (HWND)state.windows[id].data;
			info.hinstance = (HINSTANCE)state.handle.data;

			vkCreateWin32SurfaceKHR(surface.device._instance, &info, nullptr, &tmp);
			auto& swapchain = surface.surfaces[id];
			swapchain.surface = tmp;
			swapchain.busy = core::semaphore(1, 1);
		};

		auto close_cb = [](ref<window> state, u32 id, win_event event) {
			ref<vk_surface> surface = state.surface.ref();
			auto& gpu = surface.device.main_gpu();
			ref<vk_swapchain> swapchain = surface.surfaces[id];

			// if minimized carry on
			if (!swapchain.busy.tryacquire()) {
				if ((swapchain.flags & swapchain_state::minimized) != swapchain_state::minimized)
					swapchain.busy.acquire();
			}

			vkDeviceWaitIdle(gpu.device);

			for (i32 i : core::range(MAX_FRAMES_IN_FLIGHT)) {
				vkDestroySemaphore(gpu.device, swapchain.image_available[i], nullptr);
				vkDestroySemaphore(gpu.device, swapchain.render_finished[i], nullptr);
			}

			vk_swapchain_destroy_cb(state, id, event);
			vkDestroySurfaceKHR(surface.device._instance, swapchain.surface, nullptr);

			swapchain.busy.release();
			surface.surfaces.del(id);
		};

		for (u32 i : windows.keys()) {
			create_cb(*this, i, win_event::create);
		}

		add_cb(win_event::create, create_cb);
		add_cb(win_event::close, close_cb);
	}

	void window::vk_term() {
		ref<vk_surface> _surface = surface.ref();
		auto& gpu = _surface.device.main_gpu();

		for (i32 i : core::range(MAX_FRAMES_IN_FLIGHT)) {
			vkDestroyFence(gpu.device, _surface.fences[i], nullptr);
		}
	}

	void window::vk_swapchain_create() {
		auto create_cb = [](ref<window> state, u32 id, win_event event) {
			auto& surface = state.surface.ref();
			auto& gpu = surface.device.main_gpu();
			auto& swapchain = surface.surfaces[id];

			vk_swapchain_create_cb(state, id, event);

			VkSemaphoreCreateInfo semaphore_info{};
			semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			for (i32 i : core::range(MAX_FRAMES_IN_FLIGHT)) {
				vkCreateSemaphore(gpu.device, &semaphore_info, nullptr, &swapchain.image_available[i]);
				vkCreateSemaphore(gpu.device, &semaphore_info, nullptr, &swapchain.render_finished[i]);
			}
		};

		auto resize_cb = [](ref<window> state, u32 id, win_event event) {
			auto& surface = state.surface.ref();
			auto& gpu = surface.device.main_gpu();
			auto& swapchain = surface.surfaces[id];
			if (!swapchain.busy.tryacquire()) {
				if ((swapchain.flags & swapchain_state::minimized) != swapchain_state::minimized) {
					swapchain.busy.acquire();
				} else {
					swapchain.flags = swapchain_state::none;
					swapchain.busy.release();
					return;
				}
			}

			auto [width, height] = state.fbsize(id);
			bool minimized = width == 0 || height == 0;
			if (minimized) {
				swapchain.flags = swapchain_state::minimized;
				return;
			}

			vkDeviceWaitIdle(gpu.device);
			vk_swapchain_destroy_cb(state, id, event);
			vk_swapchain_create_cb(state, id, event);
			vk_framebuffer_create_cb(state, id, event);
			swapchain.busy.release();
		};

		auto& _surface = surface.ref();
		auto& gpu = _surface.device.main_gpu();
		VkFenceCreateInfo fence_info{};
		fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		for (i32 i : core::range(MAX_FRAMES_IN_FLIGHT)) {
			vkCreateFence(gpu.device, &fence_info, nullptr, &_surface.fences[i]);
		}

		for (u32 id : windows.keys()) {
			create_cb(*this, id, win_event::create);
		}

		add_cb(win_event::create, create_cb);
		add_cb(win_event::resize, resize_cb);
	}

	ref<vk_swapchain> window::vk_main_swapchain() const {
		auto& _surface = surface.ref();
		return _surface.surfaces._vals[0];
	}

	core::vector<cstr> window::vk_extensions(cref<core::vector<VkExtensionProperties>> extensions) const {
		bool surface = false, win32_surface = false;
		for (auto& extension : extensions) {
			if (core::cmpeq(extension.extensionName, "VK_KHR_surface")) {
				surface = true;
			}

			if (core::cmpeq(extension.extensionName, "VK_KHR_win32_surface")) {
				win32_surface = true;
			}
		}

		JOLLY_ASSERT(surface && win32_surface, "could not find required instance extensions");

		return core::vector{ "VK_KHR_surface", "VK_KHR_win32_surface" };
	}

	void window::vsync(bool sync) {

	}

	bool window::vsync() const {
		return true;
	}

	void window::size(u32 id, math::vec2i sz) {

	}

	math::vec2i window::size(u32 id) const {
		RECT area{};
		GetClientRect((HWND)windows[id].data, &area);
		return math::vec2i{area.right, area.bottom};
	}

	math::vec2i window::fbsize(u32 id) const {
		return size(id);
	}

	void window::callback(u32 id, win_event event) {
		if (!callbacks.has(event)) return;
		core::lock lock(busy);
		for (auto cb : callbacks[event]) {
			cb(*this, id, event);
		}
	}

	void window::add_cb(win_event event, pfn_win_cb cb) {
		auto& vec = callbacks[event];
		if (!vec.data) {
			vec = core::vector<pfn_win_cb>(0);
		}

		vec.add(cb);
	}

	void vk_swapchain_create_cb(ref<window> state, u32 id, win_event event) {
		auto& surface = state.surface.ref();
		auto& gpu = surface.device.main_gpu();
		auto& swapchain = surface.surfaces[id];

		VkSurfaceCapabilitiesKHR capabilities{};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu.physical, swapchain.surface, &capabilities);

		u32 count = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(gpu.physical, swapchain.surface, &count, nullptr);
		core::vector<VkSurfaceFormatKHR> surface_formats(count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(gpu.physical, swapchain.surface, &count, surface_formats.data);
		surface_formats.size = count;

		count = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(gpu.physical, swapchain.surface, &count, nullptr);
		core::vector<VkPresentModeKHR> present_modes(count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(gpu.physical, swapchain.surface, &count, nullptr);
		present_modes.size = count;

		VkSurfaceFormatKHR format = surface_formats[0];
		for (auto& available: surface_formats) {
			if (available.format == VK_FORMAT_B8G8R8A8_SRGB && available.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				format = available;
				break;
			}
		}

		if (surface.format == VK_FORMAT_UNDEFINED)
			surface.format = format.format;
		JOLLY_ASSERT(surface.format == format.format, "format does not match main format!");

		VkPresentModeKHR present = VK_PRESENT_MODE_FIFO_KHR;
		for (auto& available: present_modes) {
			if (available == VK_PRESENT_MODE_MAILBOX_KHR) {
				present = available;
				break;
			}
		}

		auto [x, y] = state.fbsize(id);
		x = core::clamp<u32>(x, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		y = core::clamp<u32>(y, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		swapchain.extent = VkExtent2D{(u32)x, (u32)y};

		u32 max_count = capabilities.maxImageCount ? capabilities.maxImageCount : U32_MAX;
		u32 image_count = min(capabilities.minImageCount + 1, max_count);

		VkSwapchainCreateInfoKHR info{};
		info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		info.surface = swapchain.surface;
		info.minImageCount = image_count;
		info.imageFormat = format.format;
		info.imageColorSpace = format.colorSpace;
		info.imageExtent = swapchain.extent;
		info.imageArrayLayers = 1;
		info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		core::vector<u32> indices = { gpu.graphics.family, gpu.present.family };
		if (gpu.present.family != gpu.graphics.family) {
			info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			info.queueFamilyIndexCount = 2;
			info.pQueueFamilyIndices = indices.data;
		} else {
			info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		info.preTransform = capabilities.currentTransform;
		info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		info.presentMode = present;
		info.clipped = VK_TRUE;
		info.oldSwapchain = VK_NULL_HANDLE;

		vkCreateSwapchainKHR(gpu.device, &info, nullptr, &swapchain.swapchain);

		vkGetSwapchainImagesKHR(gpu.device, swapchain.swapchain, &image_count, nullptr);
		swapchain.images = core::vector<VkImage>(image_count);
		vkGetSwapchainImagesKHR(gpu.device, swapchain.swapchain, &image_count, swapchain.images.data);
		swapchain.images.size = image_count;

		swapchain.views = core::vector<VkImageView>(image_count);
		for (u32 i : core::range(image_count)) {
			VkImageViewCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			info.image = swapchain.images[i];
			info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			info.format = surface.format;
			info.components = VkComponentMapping{
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY
			};
			info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			info.subresourceRange.baseMipLevel = 0;
			info.subresourceRange.levelCount = 1;
			info.subresourceRange.baseArrayLayer = 0;
			info.subresourceRange.layerCount = 1;

			auto& view = swapchain.views.add();
			vkCreateImageView(gpu.device, &info, nullptr, &view);
		}
	}

	void vk_framebuffer_create_cb(ref<window> state, u32 id, win_event event) {
		auto& surface = state.surface.ref();
		auto& swapchain = surface.surfaces[id];
		auto& gpu = surface.device.main_gpu();
		swapchain.framebuffers = core::vector<VkFramebuffer>(swapchain.views.size);
		for (i32 i : core::range(swapchain.views.size)) {
			VkImageView attachments[] = {
				swapchain.views[i]
			};

			VkFramebufferCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			info.renderPass = surface.device._pipeline.renderpass;
			info.attachmentCount = 1;
			info.pAttachments = attachments;
			info.width = swapchain.extent.width;
			info.height = swapchain.extent.height;
			info.layers = 1;

			VkFramebuffer fb = VK_NULL_HANDLE;
			vkCreateFramebuffer(gpu.device, &info, nullptr, &fb);
			swapchain.framebuffers.add(fb);
		}
	}

	void vk_swapchain_destroy_cb(ref<window> state, u32 id, win_event event) {
		ref<vk_surface> surface = state.surface.ref();
		auto& gpu = surface.device.main_gpu();
		ref<vk_swapchain> swapchain = surface.surfaces[id];

		for (auto view : swapchain.views) {
			vkDestroyImageView(gpu.device, view, nullptr);
		}

		for (auto fb : swapchain.framebuffers) {
			vkDestroyFramebuffer(gpu.device, fb, nullptr);
		}

		vkDestroySwapchainKHR(gpu.device, swapchain.swapchain, nullptr);
	}

	static void win_destroy_cb(ref<window> state, u32 id, win_event event) {
		core::ptr<void> hwnd = state.windows[id];

		RemovePropW((HWND)hwnd.data, L"state");
		core::ptr<void> data = RemovePropW((HWND)hwnd.data, L"id");

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

			case WM_SIZE: {
				int width = LOWORD(lparam);
				int height = HIWORD(lparam);

				LOG_INFO("wm_size % %", width, height);
				state.callback(id, win_event::resize);
				break;
			  }
		};

		return DefWindowProc(hwnd, umsg, wparam, lparam);
	}
}
