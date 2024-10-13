
#define VK_USE_PLATFORM_WIN32_KHR
#include "VulkanCpp.hpp"

namespace vkcpp {
	Queue Device::getDeviceQueue(int deviceQueueFamily, int deviceQueueIndex) const {
		VkQueue vkQueue;
		vkGetDeviceQueue(m_handle, deviceQueueFamily, deviceQueueIndex, &vkQueue);
		if (vkQueue == nullptr) {
			throw Exception(VK_ERROR_UNKNOWN);
		}
		return Queue(vkQueue, *this);
	}

}