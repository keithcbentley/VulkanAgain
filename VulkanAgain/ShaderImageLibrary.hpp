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


class ImageLibrary {

public:

	ImageLibrary() {}
	~ImageLibrary();

	static void createImageMemoryViewFromFile(
		const char* imageName,
		const char* fileName,
		vkcpp::Device device,
		vkcpp::CommandPool commandPool,
		vkcpp::Queue graphicsQueue);

	static vkcpp::ImageView imageView(
		const char* imageName);

};
