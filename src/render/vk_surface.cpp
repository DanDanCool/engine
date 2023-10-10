#include "vk_surface.h"

namespace jolly {
	vk_surface::vk_surface(cref<vk_device> _device)
	: device(_device)
	, surfaces()
	, fence{ VK_NULL_HANDLE, VK_NULL_HANDLE }
	, format(VK_FORMAT_UNDEFINED) {}
}
