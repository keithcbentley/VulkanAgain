#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include "VulkanCpp.hpp"

class ShaderLibrary {


public:

	static vkcpp::ShaderModule createShaderModuleFromFile(const char* fileName, VkDevice vkDevice);


};
