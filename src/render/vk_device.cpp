#include "vk_device.h"

#include <core/log.h>
#include <win32/window.h>

namespace jolly {
	static VKAPI_ATTR VkBool32 VKAPI_CALL vkdebugcb(
		VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT type,
		const VkDebugUtilsMessengerCallbackDataEXT* cbdata,
		void* userdata) {
		LOG_INFO("vulkan: %", cbdata->pMessage);

		return VK_FALSE;
	}

	vk_device::vk_device(cref<core::string> name, core::pair<u32, u32> sz)
	: _window(), _instance(), _debugmsg() {
		_window = core::ptr_create<window>(name, sz);

		core::vector<cstr> enabled_layers = {
			"VK_LAYER_KHRONOS_validation"
		};

		u32 count = 0;
		vkEnumerateInstanceLayerProperties(&count, nullptr);
		core::vector<VkLayerProperties> layers(count);
		vkEnumerateInstanceLayerProperties(&count, layers.data);
		layers.size = count;

		for (auto name : enabled_layers) {
			bool found = false;
			for (auto& layer : layers) {
				if (core::cmpeq(name, layer.layerName)) {
					found = true;
					break;
				}
			}

			assert(found);
		}

		VkApplicationInfo app_info{};
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName = "jolly";
		app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.pEngineName = "jolly_engine";
		app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.apiVersion = VK_API_VERSION_1_0;

		count = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
		core::vector<VkExtensionProperties> extensions(count);
		vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data);
		extensions.size = count;

		LOG_INFO("vkextensions");
		for (auto& extension : extensions) {
			LOG_INFO("extension: %", extension.extensionName);
		}

		core::vector<cstr> enabled_extensions = _window->vkextensions(extensions);
		enabled_extensions.add(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		VkDebugUtilsMessengerCreateInfoEXT msg_info{};
		msg_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		msg_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		msg_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		msg_info.pfnUserCallback = vkdebugcb;
		msg_info.pUserData = nullptr;

		VkInstanceCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		info.pApplicationInfo = &app_info;
		info.enabledExtensionCount = enabled_extensions.size;
		info.ppEnabledExtensionNames = enabled_extensions.data;
		info.enabledLayerCount = enabled_layers.size;
		info.ppEnabledLayerNames = enabled_layers.data;
		info.pNext = &msg_info;

		enabled_extensions.data = nullptr;
		enabled_layers.data = nullptr;
		vkCreateInstance(&info, nullptr, &_instance);

		auto fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT");
		fn(_instance, &msg_info, nullptr, &_debugmsg);
	}

	vk_device::~vk_device() {
		auto fn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT");
		fn(_instance, _debugmsg, nullptr);
		vkDestroyInstance(_instance, nullptr);

		_debugmsg = nullptr;
		_instance = nullptr;
	}

	void vk_device::step(f32 ms) {
		_window->step(ms);
	}
}
