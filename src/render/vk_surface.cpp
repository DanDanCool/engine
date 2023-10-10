#include "vk_surface.h"

namespace jolly {
	vk_surface::vk_surface(cref<vk_device> _device)
	: device(_device)
	, surfaces()
	, fences{ VK_NULL_HANDLE, VK_NULL_HANDLE }
	, format(VK_FORMAT_UNDEFINED) {}

	ENUM_CLASS_OPERATORS(swapchain_state);
}
