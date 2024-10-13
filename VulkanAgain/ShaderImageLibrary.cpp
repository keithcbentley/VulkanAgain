#include "pragmas.hpp"

#include "ShaderImageLibrary.hpp"

#define VK_USE_PLATFORM_WIN32_KHR
#include "VulkanCpp.hpp"


//#define STB_IMAGE_IMPLEMENTATION
//#include <stb_image.h>

vkcpp::ShaderModule ShaderLibrary::createShaderModuleFromFile(const char* fileName, VkDevice vkDevice) {

	return vkcpp::ShaderModule::createShaderModuleFromFile(fileName, vkDevice);

}
