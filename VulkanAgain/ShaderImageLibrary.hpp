#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include "VulkanCpp.hpp"

class ShaderLibrary {


public:

	ShaderLibrary() {}
	~ShaderLibrary();

	static void createShaderModuleFromFile(
		const std::string& shaderName,
		const std::string& fileName,
		VkDevice vkDevice);

	static vkcpp::ShaderModule shaderModule(const std::string& shaderName);


};
