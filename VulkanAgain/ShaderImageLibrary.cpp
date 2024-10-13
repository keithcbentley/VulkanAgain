#include "pragmas.hpp"

#include "ShaderImageLibrary.hpp"

#define VK_USE_PLATFORM_WIN32_KHR
#include "VulkanCpp.hpp"

std::map<std::string, vkcpp::ShaderModule> g_shaderModules;


ShaderLibrary::~ShaderLibrary() {
	//	Hack to control when saved shader module map gets cleared out.
	g_shaderModules.clear();
}

//#define STB_IMAGE_IMPLEMENTATION
//#include <stb_image.h>

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
