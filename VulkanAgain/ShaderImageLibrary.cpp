#include "pragmas.hpp"

#include "ShaderImageLibrary.hpp"

#define VK_USE_PLATFORM_WIN32_KHR
#include "VulkanCpp.hpp"

std::map<std::string, vkcpp::ShaderModule> g_shaderModules;
std::map<std::string, vkcpp::Image_Memory_View> g_ImageMemoryViews;


ShaderLibrary::~ShaderLibrary() {
	//	Hack to control when saved shader module map gets cleared out.
	g_shaderModules.clear();
}

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

void ShaderLibrary::createShaderModuleFromFile(
	const std::string& shaderName,
	const std::string& fileName,
	VkDevice vkDevice
) {
	vkcpp::ShaderModule shaderModule = vkcpp::ShaderModule::createShaderModuleFromFile(fileName.c_str(), vkDevice);
	g_shaderModules.emplace(shaderName, std::move(shaderModule));

}

vkcpp::ShaderModule ShaderLibrary::shaderModule(const std::string& shaderName) {
	return g_shaderModules.at(shaderName);
}


ImageLibrary::~ImageLibrary() {
	//	Hack to control when map gets cleared out.
	g_ImageMemoryViews.clear();
}



void transitionImageLayout(
	vkcpp::Image image,
	VkFormat format,
	VkImageLayout oldLayout,
	VkImageLayout newLayout,
	vkcpp::CommandPool commandPool,
	vkcpp::Queue graphicsQueue
) {
	//	TODO: turn into method on command buffers?
	vkcpp::CommandBuffer commandBuffer(commandPool);
	commandBuffer.beginOneTimeSubmit();

	vkcpp::ImageMemoryBarrier2 imageMemoryBarrier(oldLayout, newLayout, image);
	vkcpp::DependencyInfo dependencyInfo;
	dependencyInfo.addImageMemoryBarrier(imageMemoryBarrier);
	commandBuffer.cmdPipelineBarrier2(dependencyInfo);

	commandBuffer.end();

	graphicsQueue.submit2Fenced(commandBuffer);

}


void copyBufferToImage(
	vkcpp::Buffer buffer,
	vkcpp::Image image,
	int width,
	int height,
	vkcpp::CommandPool commandPool,
	vkcpp::Queue graphicsQueue
) {
	//	TODO: turn into method on command buffers?
	vkcpp::CommandBuffer commandBuffer(commandPool);
	commandBuffer.beginOneTimeSubmit();
	commandBuffer.cmdCopyBufferToImage(buffer, image, width, height);
	commandBuffer.end();

	graphicsQueue.submit2Fenced(commandBuffer);
}


void ImageLibrary::createImageMemoryViewFromFile(
	const char* name,
	const char* fileName,
	vkcpp::Device device,
	vkcpp::CommandPool commandPool,
	vkcpp::Queue graphicsQueue
) {
	const VkFormat targetFormat = VK_FORMAT_R8G8B8A8_SRGB;

	int texWidth;
	int texHeight;
	int texChannels;

	stbi_uc* pixels = stbi_load(fileName, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}

	const VkDeviceSize imageSize = texWidth * texHeight * 4;	//	4 == sizeof R8G8B8A8 pixel

	//	Make a device (gpu) staging buffer and copy the pixels into it.
	vkcpp::Buffer_DeviceMemory stagingBuffer_DeviceMemoryMapped(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		imageSize,
		graphicsQueue.m_queueFamilyIndex,
		vkcpp::MEMORY_PROPERTY_HOST_VISIBLE | vkcpp::MEMORY_PROPERTY_HOST_COHERENT,
		pixels,
		device);

	stbi_image_free(pixels);	//	Don't need these pixels anymore.  Pixels are on gpu now.
	pixels = nullptr;

	//	Make our target image and memory.
	VkExtent2D texExtent;
	texExtent.width = texWidth;
	texExtent.height = texHeight;
	vkcpp::Image_Memory textureImage_DeviceMemory(
		texExtent,
		targetFormat,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		vkcpp::MEMORY_PROPERTY_DEVICE_LOCAL,
		device);

	//	Change image format to be best target for transfer into.
	transitionImageLayout(
		textureImage_DeviceMemory.m_image,
		targetFormat,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		commandPool,
		graphicsQueue);

	//	Copy image pixels from staging buffer into image memory.
	copyBufferToImage(
		stagingBuffer_DeviceMemoryMapped.m_buffer,
		textureImage_DeviceMemory.m_image,
		texWidth,
		texHeight,
		commandPool,
		graphicsQueue);

	//	Now transition the image layout to best for shader images.
	transitionImageLayout(
		textureImage_DeviceMemory.m_image,
		targetFormat,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		commandPool,
		graphicsQueue);

	//	Shaders are accessed through image views, not directly from images.
	vkcpp::ImageViewCreateInfo textureImageViewCreateInfo(
		textureImage_DeviceMemory.m_image,
		VK_IMAGE_VIEW_TYPE_2D,
		targetFormat,
		VK_IMAGE_ASPECT_COLOR_BIT);
	vkcpp::ImageView textureImageView(textureImageViewCreateInfo, device);

	vkcpp::Image_Memory_View image_memory_view(
		std::move(textureImage_DeviceMemory.m_image),
		std::move(textureImage_DeviceMemory.m_deviceMemory),
		std::move(textureImageView));

	g_ImageMemoryViews.emplace(name, std::move(image_memory_view));
}


vkcpp::ImageView ImageLibrary::imageView(const char* name) {
	return g_ImageMemoryViews.at(name).m_imageView;
}