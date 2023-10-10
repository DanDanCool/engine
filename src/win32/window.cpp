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
	assert(fn);
	return fn(instance, info, callbacks, surface);
}

namespace jolly {
	const auto* WNDCLASS_NAME = L"JOLLY_WNDCLASS";

	static void windestroycb(ref<window> state, u32 id, win_event event);
	static LRESULT wndproc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);

	window::window(cref<core::string> name, core::pair<u32, u32> sz)
	: handle(), surface(), windows(), callbacks(), lock() {
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

		auto createcb = [](ref<window> state, u32 id, win_event event) {
			ref<vk_surface> surface = state.surface.ref();

			VkSurfaceKHR tmp = VK_NULL_HANDLE;
			VkWin32SurfaceCreateInfoKHR info{};
			info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			info.hwnd = (HWND)state.windows[id].data;
			info.hinstance = (HINSTANCE)state.handle.data;

			vkCreateWin32SurfaceKHR(surface.device._instance, &info, nullptr, &tmp);
			surface.surfaces[id] = vk_swapchain{};
			surface.surfaces[id].surface = tmp;
		};

		auto closecb = [](ref<window> state, u32 id, win_event event) {
			ref<vk_surface> surface = state.surface.ref();
			auto& gpu = surface.device.main_gpu();
			ref<vk_swapchain> swapchain = surface.surfaces[id];

			vkDeviceWaitIdle(gpu.device);
			for (i32 i : core::range(MAX_FRAMES_IN_FLIGHT)) {
				vkDestroySemaphore(gpu.device, swapchain.image_available[i], nullptr);
				vkDestroySemaphore(gpu.device, swapchain.render_finished[i], nullptr);
			}

			for (auto view : swapchain.views) {
				vkDestroyImageView(gpu.device, view, nullptr);
			}

			for (auto fb : swapchain.framebuffers) {
				vkDestroyFramebuffer(gpu.device, fb, nullptr);
			}

			vkDestroySwapchainKHR(gpu.device, swapchain.swapchain, nullptr);
			vkDestroySurfaceKHR(surface.device._instance, swapchain.surface, nullptr);

			surface.surfaces.del(id);
		};

		for (u32 i : windows.keys()) {
			createcb(*this, i, win_event::create);
		}

		addcb(win_event::create, createcb);
		addcb(win_event::close, closecb);
	}

	void window::vk_term() {
		auto& _surface = surface.ref();
		auto& gpu = _surface.device.main_gpu();
		for (i32 i : core::range(MAX_FRAMES_IN_FLIGHT)) {
			vkDestroyFence(gpu.device, _surface.fence[i], nullptr);
		}
	}

	void window::vk_swapchain_create() {
		auto createcb = [](ref<window> state, u32 id, win_event event) {
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
			assert(surface.format == format.format);

			VkPresentModeKHR present = VK_PRESENT_MODE_FIFO_KHR;
			for (auto& available: present_modes) {
				if (available == VK_PRESENT_MODE_MAILBOX_KHR) {
					present = available;
					break;
				}
			}

			auto [x, y] = state.fbsize(id);
			x = core::clamp(x, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			y = core::clamp(y, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			swapchain.extent = VkExtent2D{x, y};

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

			VkSemaphoreCreateInfo semaphore_info{};
			semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			for (i32 i : core::range(MAX_FRAMES_IN_FLIGHT)) {
				vkCreateSemaphore(gpu.device, &semaphore_info, nullptr, &swapchain.image_available[i]);
				vkCreateSemaphore(gpu.device, &semaphore_info, nullptr, &swapchain.render_finished[i]);
			}
		};

		for (u32 id : windows.keys()) {
			createcb(*this, id, win_event::create);
		}

		auto& _surface = surface.ref();
		auto& gpu = _surface.device.main_gpu();
		VkFence fence = VK_NULL_HANDLE;
		VkFenceCreateInfo fence_info{};
		fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (i32 i : core::range(MAX_FRAMES_IN_FLIGHT)) {
			vkCreateFence(gpu.device, &fence_info, nullptr, &_surface.fence[i]);
		}

		addcb(win_event::create, createcb);
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

		assert(surface && win32_surface);

		return core::vector{ "VK_KHR_surface", "VK_KHR_win32_surface" };
	}

	void window::vsync(bool sync) {

	}

	bool window::vsync() const {
		return true;
	}

	void window::size(u32 id, core::pair<u32, u32> sz) {

	}

	core::pair<u32, u32> window::size(u32 id) const {
		RECT area{};
		GetClientRect((HWND)windows[id].data, &area);
		return core::pair<u32, u32>(area.right, area.bottom);
	}

	core::pair<u32, u32> window::fbsize(u32 id) const {
		return size(id);
	}

	void window::callback(u32 id, win_event event) {
		if (!callbacks.has(event)) return;
		core::lock l(lock);
		for (auto cb : callbacks[event]) {
			cb(*this, id, event);
		}
	}

	void window::addcb(win_event event, pfn_win_cb cb) {
		auto& vec = callbacks[event];
		if (!vec.data) {
			vec = core::vector<pfn_win_cb>(0);
		}

		vec.add(cb);
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
