#include "vk_surface.h"

namespace jolly {
	vk_surface::vk_surface(cref<vk_device> _device)
	: device(_device), surfaces(), format(VK_FORMAT_UNDEFINED) {}
}
