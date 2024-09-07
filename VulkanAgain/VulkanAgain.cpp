// VulkanAgain.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pragmas.hpp"

#include <iostream>
#include <fstream>
#include <array>
#include <chrono>
#include <map>
#include <variant>

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define VK_USE_PLATFORM_WIN32_KHR

#include "VulkanCpp.hpp"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


struct Vertex {
	glm::vec3	pos;
	glm::vec3	color;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};

		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}
};


const std::vector<Vertex> g_vertices = {
	{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
	{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
	{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

	{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
	{{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
	{{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};

const std::vector<uint16_t> g_vertexIndices = {
	0, 1, 2, 2, 3, 0,
	4, 5, 6, 6, 7, 4
};

const wchar_t* szTitle = TEXT("Vulkan Again");
const wchar_t* szWindowClass = TEXT("Vulkan Again Class");

const int windowPositionX = 500;
const int windowPositionY = 500;
const int windowWidth = 800;
const int windowHeight = 600;

//uint32_t findMemoryTypeIndex(
//	VkPhysicalDevice vkPhysicalDevice,
//	uint32_t usableMemoryIndexBits,
//	VkMemoryPropertyFlags requiredProperties
//) {
//
//	VkPhysicalDeviceMemoryProperties memProperties;
//	vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &memProperties);
//
//	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
//		if ( (usableMemoryIndexBits & (1 << i) )
//			&& (memProperties.memoryTypes[i].propertyFlags & requiredProperties) == requiredProperties) {
//			return i;
//		}
//	}
//	throw std::runtime_error("failed to find suitable memory type!");
//}



class DrawingFrame {

public:
	vkcpp::Fence	m_inFlightFence;
	vkcpp::Semaphore m_imageAvailableSemaphore;
	vkcpp::Semaphore m_renderFinishedSemaphore;
	vkcpp::CommandBuffer	m_commandBuffer;;
	vkcpp::Buffer_DeviceMemory	m_uniformBufferMemory;
	vkcpp::DescriptorSet	m_descriptorSet;

	DrawingFrame() {};

	DrawingFrame(const DrawingFrame&) = delete;
	DrawingFrame& operator=(const DrawingFrame&) = delete;
	DrawingFrame(DrawingFrame&&) = delete;
	DrawingFrame& operator=(DrawingFrame&&) = delete;

	void moveInFlightFence(vkcpp::Fence&& fence) {
		m_inFlightFence = std::move(fence);
	}

	void moveImageAvailableSemaphore(vkcpp::Semaphore&& semaphore) {
		m_imageAvailableSemaphore = std::move(semaphore);
	}

	void moveRenderFinishedSemaphore(vkcpp::Semaphore&& semaphore) {
		m_renderFinishedSemaphore = std::move(semaphore);
	}

	void moveCommandBuffer(vkcpp::CommandBuffer&& vkCommandBuffer) {
		m_commandBuffer = std::move(vkCommandBuffer);
	}

	void moveUniformMemoryBuffer(vkcpp::Buffer_DeviceMemory&& bufferAndDeviceMemoryMapped) {
		m_uniformBufferMemory = std::move(bufferAndDeviceMemoryMapped);
	}

	void moveDescriptorSet(vkcpp::DescriptorSet&& descriptorSet) {
		m_descriptorSet = std::move(descriptorSet);
	}

	VkDevice getVkDevice() const {
		return m_inFlightFence.getVkDevice();
	}
};


class AllDrawingFrames {

private:
	std::vector<DrawingFrame> m_drawingFrames;

public:

	AllDrawingFrames() {}
	~AllDrawingFrames() {
		return;
	}

	AllDrawingFrames(const AllDrawingFrames&) = delete;
	AllDrawingFrames& operator=(const AllDrawingFrames&) = delete;

	AllDrawingFrames(int frameCount) : m_drawingFrames(frameCount) {}

	DrawingFrame& drawingFrameAt(int index) {
		return m_drawingFrames.at(index);
	}

	int	frameCount() { return static_cast<int>(m_drawingFrames.size()); }

};

static const int MAX_FRAMES_IN_FLIGHT = 3;
static const int SWAP_CHAIN_IMAGE_COUNT = 5;


static int g_nextFrameToDrawIndex = 0;


vkcpp::VulkanInstance createVulkanInstance() {

	vkcpp::VulkanInstanceCreateInfo vulkanInstanceCreateInfo{};
	vulkanInstanceCreateInfo.addLayer("VK_LAYER_KHRONOS_validation");

	vulkanInstanceCreateInfo.addExtension("VK_EXT_debug_utils");
	vulkanInstanceCreateInfo.addExtension("VK_KHR_surface");
	vulkanInstanceCreateInfo.addExtension("VK_KHR_win32_surface");

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = vkcpp::DebugUtilsMessenger::getCreateInfo();
	vulkanInstanceCreateInfo.pNext = &debugCreateInfo;

	return vkcpp::VulkanInstance(vulkanInstanceCreateInfo);
}


void recordCommandBuffer(
	vkcpp::CommandBuffer		commandBuffer,
	uint32_t			swapchainImageIndex,
	vkcpp::Swapchain_ImageViews_FrameBuffers& swapchainImageViewsFrameBuffers,
	vkcpp::Buffer			vertexBuffer,
	vkcpp::Buffer			vertexIndexBuffer,
	VkDescriptorSet		descriptorSet,
	VkPipelineLayout	vkPipelineLayout,
	VkPipeline			graphicsPipeline
) {
	const VkExtent2D swapchainImageExtent = swapchainImageViewsFrameBuffers.getImageExtent();

	commandBuffer.begin();

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = swapchainImageViewsFrameBuffers.getRenderPass();
	renderPassInfo.framebuffer = swapchainImageViewsFrameBuffers.getFrameBuffer(swapchainImageIndex);
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = swapchainImageExtent;
	VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapchainImageExtent.width);
	viewport.height = static_cast<float>(swapchainImageExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapchainImageExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

	VkBuffer vertexBuffers[] = { vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

	vkCmdBindIndexBuffer(commandBuffer, vertexIndexBuffer, 0, VK_INDEX_TYPE_UINT16);

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		vkPipelineLayout,
		0, 1,
		&descriptorSet,
		0, nullptr);

	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(g_vertexIndices.size()), 1, 0, 0, 0);

	vkCmdEndRenderPass(commandBuffer);

	commandBuffer.end();

}


struct ModelViewProjTransform {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};



class Globals {

public:

	Globals() {}
	~Globals() {
		return;
	}

	Globals(const Globals&) = delete;
	Globals& operator=(const Globals&) = delete;

	Globals(Globals&&) = delete;
	Globals& operator=(Globals&&) = delete;

	HINSTANCE				g_hInstance = NULL;
	HWND					g_hWnd = NULL;

	vkcpp::VulkanInstance	g_vulkanInstance;
	vkcpp::Surface			g_surfaceOriginal;
	vkcpp::PhysicalDevice	g_physicalDevice;
	vkcpp::Device			g_deviceOriginal;

	VkQueue				g_vkGraphicsQueue = nullptr;
	VkQueue				g_vkPresentationQueue = nullptr;

	vkcpp::RenderPass g_renderPassOriginal;

	uint32_t						g_swapchainMinImageCount = SWAP_CHAIN_IMAGE_COUNT;
	vkcpp::Swapchain_ImageViews_FrameBuffers	g_swapchainImageViewsFrameBuffers;

	vkcpp::ShaderModule g_vertShaderModule;
	vkcpp::ShaderModule g_fragShaderModule;

	vkcpp::DescriptorPool			g_descriptorPoolOriginal;
	vkcpp::DescriptorSetLayout		g_descriptorSetLayoutOriginal;

	vkcpp::PipelineLayout	g_pipelineLayout;
	vkcpp::GraphicsPipeline	g_graphicsPipeline;

	vkcpp::Buffer_DeviceMemory	g_vertexBufferAndDeviceMemory;
	vkcpp::Buffer_DeviceMemory	g_vertexIndicesBufferAndDeviceMemory;

	vkcpp::CommandPool		g_commandPoolOriginal;

	vkcpp::Image_DeviceMemory	g_textureImage_deviceMemory;
	vkcpp::ImageView g_textureImageView;
	vkcpp::Sampler g_textureSampler;

};
Globals g_globals;
AllDrawingFrames g_allDrawingFrames(MAX_FRAMES_IN_FLIGHT);


vkcpp::DescriptorSetLayout createDrawingFrameDescriptorSetLayout(VkDevice vkDevice) {

	vkcpp::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
	descriptorSetLayoutCreateInfo
		.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
		.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

	return vkcpp::DescriptorSetLayout(descriptorSetLayoutCreateInfo, vkDevice);
}


vkcpp::DescriptorPool createDescriptorPool(VkDevice vkDevice) {

	vkcpp::DescriptorPoolCreateInfo poolCreateInfo;
	poolCreateInfo.addDescriptorCount(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT);
	poolCreateInfo.addDescriptorCount(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_FRAMES_IN_FLIGHT);

	poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolCreateInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	return vkcpp::DescriptorPool(poolCreateInfo, vkDevice);
}


void createDescriptorSets(
	vkcpp::Device device,
	vkcpp::DescriptorPool		descriptorPool,
	vkcpp::DescriptorSetLayout descriptorSetLayout,
	vkcpp::ImageView textureImageView,
	vkcpp::Sampler textureSampler,
	AllDrawingFrames& allDrawingFrames
) {
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkcpp::DescriptorSet descriptorSet(descriptorSetLayout, descriptorPool);

		vkcpp::DescriptorSetUpdater descriptorSetUpdater(descriptorSet);
		descriptorSetUpdater.addWriteDescriptor(
			0,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			allDrawingFrames.drawingFrameAt(i).m_uniformBufferMemory.m_buffer,
			sizeof(ModelViewProjTransform));

		descriptorSetUpdater.addWriteDescriptor(
			1,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			textureImageView,
			textureSampler);


		descriptorSetUpdater.updateDescriptorSets();

		allDrawingFrames.drawingFrameAt(i).moveDescriptorSet(std::move(descriptorSet));
	}
}


class  PipelineInputAssemblyStateCreateInfo : public VkPipelineInputAssemblyStateCreateInfo {

public:
	PipelineInputAssemblyStateCreateInfo()
		: VkPipelineInputAssemblyStateCreateInfo{} {
		sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	}

	PipelineInputAssemblyStateCreateInfo& setTopology(VkPrimitiveTopology vkPrimitiveTopology) {
		topology = vkPrimitiveTopology;
		return *this;
	}

};

class PipelineRasterizationStateCreateInfo : public VkPipelineRasterizationStateCreateInfo {

public:
	PipelineRasterizationStateCreateInfo()
		: VkPipelineRasterizationStateCreateInfo{} {
		sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		polygonMode = VK_POLYGON_MODE_FILL;
		lineWidth = 1.0f;
		cullMode = VK_CULL_MODE_BACK_BIT;
		frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	}
};

class PipelineMultisampleStateCreateInfo : public VkPipelineMultisampleStateCreateInfo {

public:
	PipelineMultisampleStateCreateInfo()
		: VkPipelineMultisampleStateCreateInfo{} {
		sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		minSampleShading = 1.0f; // Optional
	}

};


class PipelineColorBlendAttachmentState : public VkPipelineColorBlendAttachmentState {

public:

	PipelineColorBlendAttachmentState()
		: VkPipelineColorBlendAttachmentState{} {
		colorWriteMask
			= VK_COLOR_COMPONENT_R_BIT
			| VK_COLOR_COMPONENT_G_BIT
			| VK_COLOR_COMPONENT_B_BIT
			| VK_COLOR_COMPONENT_A_BIT;
		blendEnable = VK_FALSE;
		srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendOp = VK_BLEND_OP_ADD; // Optional
		srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	}
};


class PipelineColorBlendStateCreateInfo : public VkPipelineColorBlendStateCreateInfo {

public:
	PipelineColorBlendStateCreateInfo()
		: VkPipelineColorBlendStateCreateInfo{} {
		sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		logicOpEnable = VK_FALSE;
		logicOp = VK_LOGIC_OP_COPY; // Optional
	}

};


class GraphicsPipelineConfig {


	VkPipelineShaderStageCreateInfo m_vertShaderStageCreateInfo{};

	VkPipelineShaderStageCreateInfo m_fragShaderStageCreateInfo{};

	std::vector<VkPipelineShaderStageCreateInfo> m_shaderStageCreateInfos;

	std::vector<VkDynamicState> m_dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo m_dynamicState{};

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};

	PipelineInputAssemblyStateCreateInfo m_inputAssemblyStateCreateInfo{};

	VkViewport m_viewport{};
	VkRect2D m_scissor{};

	VkPipelineViewportStateCreateInfo m_viewportState{};

	PipelineRasterizationStateCreateInfo m_pipelineRasterizationStateCreateInfo{};

	PipelineMultisampleStateCreateInfo m_pipelineMultisampleStateCreateInfo{};

	PipelineColorBlendAttachmentState m_pipelineColorBlendAttachmentState{};

	PipelineColorBlendStateCreateInfo m_pipelineColorBlendStateCreateInfo{};


public:

	VkGraphicsPipelineCreateInfo m_vkGraphicsPipelineCreateInfo{};

	vkcpp::PipelineLayout	m_pipelineLayout;
	vkcpp::RenderPass		m_renderPass;
	vkcpp::ShaderModule	m_vertexShaderModule;
	vkcpp::ShaderModule	m_fragmentShaderModule;

	// TODO needs to be smarter
	VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
	std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = Vertex::getAttributeDescriptions();


	PipelineInputAssemblyStateCreateInfo& setInputAssemblyTopology(VkPrimitiveTopology topology) {
		return m_inputAssemblyStateCreateInfo.setTopology(topology);
	}

	void setVertexShaderModule(vkcpp::ShaderModule vertexShaderModule) {
		m_vertexShaderModule = vertexShaderModule;
	}

	void setFragmentShaderModule(vkcpp::ShaderModule fragmentShaderModule) {
		m_fragmentShaderModule = fragmentShaderModule;
	}

	void setViewportExtent(VkExtent2D extent) {
		m_viewport.width = (float)extent.width;
		m_viewport.height = (float)extent.height;
	}

	void setScissorExtent(VkExtent2D extent) {
		m_scissor.extent = extent;
	}

	void setPipelineLayout(vkcpp::PipelineLayout pipelineLayout) {
		m_pipelineLayout = pipelineLayout;
	}

	void setRenderPass(vkcpp::RenderPass renderPass) {
		m_renderPass = renderPass;
	}


	VkGraphicsPipelineCreateInfo& assemble() {

		//	Assemble pipeline create info
		m_vkGraphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

		// TODO make shader module setup smarter
		m_vertShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		m_vertShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		m_vertShaderStageCreateInfo.module = m_vertexShaderModule;
		m_vertShaderStageCreateInfo.pName = "main";

		m_fragShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		m_fragShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		m_fragShaderStageCreateInfo.module = m_fragmentShaderModule;
		m_fragShaderStageCreateInfo.pName = "main";

		m_shaderStageCreateInfos.push_back(m_vertShaderStageCreateInfo);
		m_shaderStageCreateInfos.push_back(m_fragShaderStageCreateInfo);

		m_vkGraphicsPipelineCreateInfo.stageCount = static_cast<uint32_t>(m_shaderStageCreateInfos.size());
		m_vkGraphicsPipelineCreateInfo.pStages = m_shaderStageCreateInfos.data();


		// TODO tied to Vertex class.  Needs to be decoupled and smarter.
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		//VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
		//auto attributeDescriptions = Vertex::getAttributeDescriptions();
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
		m_vkGraphicsPipelineCreateInfo.pVertexInputState = &vertexInputInfo;

		m_vkGraphicsPipelineCreateInfo.pInputAssemblyState = &m_inputAssemblyStateCreateInfo;

		m_viewport.maxDepth = 1.0f;
		//TODO make the viewportState smarter
		m_viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		m_viewportState.viewportCount = 1;
		m_viewportState.pViewports = &m_viewport;
		m_viewportState.scissorCount = 1;
		m_viewportState.pScissors = &m_scissor;
		m_vkGraphicsPipelineCreateInfo.pViewportState = &m_viewportState;

		m_vkGraphicsPipelineCreateInfo.pRasterizationState = &m_pipelineRasterizationStateCreateInfo;

		m_vkGraphicsPipelineCreateInfo.pMultisampleState = &m_pipelineMultisampleStateCreateInfo;

		m_vkGraphicsPipelineCreateInfo.pDepthStencilState = nullptr; // Optional

		m_pipelineColorBlendStateCreateInfo.attachmentCount = 1;
		m_pipelineColorBlendStateCreateInfo.pAttachments = &m_pipelineColorBlendAttachmentState;
		m_vkGraphicsPipelineCreateInfo.pColorBlendState = &m_pipelineColorBlendStateCreateInfo;


		// TODO make this smarter
		m_dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		m_dynamicState.dynamicStateCount = static_cast<uint32_t>(m_dynamicStates.size());
		m_dynamicState.pDynamicStates = m_dynamicStates.data();

		m_vkGraphicsPipelineCreateInfo.pDynamicState = &m_dynamicState;
		m_vkGraphicsPipelineCreateInfo.layout = m_pipelineLayout;
		m_vkGraphicsPipelineCreateInfo.renderPass = m_renderPass;
		m_vkGraphicsPipelineCreateInfo.subpass = 0;
		m_vkGraphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		m_vkGraphicsPipelineCreateInfo.basePipelineIndex = -1; // Optional

		return m_vkGraphicsPipelineCreateInfo;
	}
};


class ImageMemoryBarrier : public VkImageMemoryBarrier {

public:
	ImageMemoryBarrier(VkImageLayout oldLayoutArg, VkImageLayout newLayoutArg, VkImage imageArg)
		:VkImageMemoryBarrier{} {
		sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		oldLayout = oldLayoutArg;
		newLayout = newLayoutArg;
		srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image = imageArg;
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = 1;
		subresourceRange.baseArrayLayer = 0;
		subresourceRange.layerCount = 1;

		//	Note that this grabs oldLayout and newLayout from the structure, not the args.
		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			srcAccessMask = 0;
			dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		}
		else {
			throw std::invalid_argument("unsupported layout transition!");
		}
	}

	VkPipelineStageFlags getSourcePipelineStageFlags() {
		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		}
		if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			return VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		throw std::invalid_argument("unsupported layout transition!");
	}

	VkPipelineStageFlags getDestinationPipelineStageFlags() {
		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			return VK_PIPELINE_STAGE_TRANSFER_BIT;
		}

		if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}

		throw std::invalid_argument("unsupported layout transition!");
	}


};


void transitionImageLayout(
	VkImage image,
	VkFormat format,
	VkImageLayout oldLayout,
	VkImageLayout newLayout,
	vkcpp::CommandPool commandPool,
	VkQueue graphicsQueue) {

	vkcpp::CommandBuffer commandBuffer(commandPool);
	commandBuffer.beginOneTimeSubmit();

	ImageMemoryBarrier barrier(oldLayout, newLayout, image);
	vkCmdPipelineBarrier(
		commandBuffer,
		barrier.getSourcePipelineStageFlags(),
		barrier.getDestinationPipelineStageFlags(),
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	commandBuffer.end();

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	VkCommandBuffer vkCommandBuffer = commandBuffer;
	submitInfo.pCommandBuffers = &vkCommandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

}


void copyBufferToImage(
	VkBuffer buffer,
	VkImage image,
	uint32_t width,
	uint32_t height,
	vkcpp::CommandPool commandPool,
	VkQueue graphicsQueue) {

	vkcpp::CommandBuffer commandBuffer(commandPool);
	commandBuffer.beginOneTimeSubmit();

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
		width,
		height,
		1
	};

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	VkCommandBuffer vkCommandBuffer = commandBuffer;
	submitInfo.pCommandBuffers = &vkCommandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);
}



vkcpp::Image_DeviceMemory createImageAndDeviceMemory(
	uint32_t width,
	uint32_t height,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlags usage,
	VkMemoryPropertyFlags properties,
	vkcpp::Device device
) {
	vkcpp::ImageCreateInfo imageCreateInfo(format, usage);
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.tiling = tiling;

	return vkcpp::Image_DeviceMemory(imageCreateInfo, properties, device);

}



vkcpp::Image_DeviceMemory createTextureImageFromFile(
	const char* fileName,
	vkcpp::Device device,
	vkcpp::CommandPool commandPool,
	VkQueue graphicsQueue
) {
	int texWidth;
	int texHeight;
	int texChannels;

	stbi_uc* pixels = stbi_load(fileName, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}

	const VkDeviceSize imageSize = texWidth * texHeight * 4;

	vkcpp::Buffer_DeviceMemory stagingBufferAndDeviceMemoryMapped(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		imageSize,
		pixels,
		device);

	stbi_image_free(pixels);

	vkcpp::Image_DeviceMemory textureImageAndDeviceMemory(
		texWidth,
		texHeight,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		device);

	transitionImageLayout(
		textureImageAndDeviceMemory.m_image,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		commandPool,
		graphicsQueue);

	copyBufferToImage(
		stagingBufferAndDeviceMemoryMapped.m_buffer,
		textureImageAndDeviceMemory.m_image,
		static_cast<uint32_t>(texWidth),
		static_cast<uint32_t>(texHeight),
		commandPool,
		graphicsQueue);

	transitionImageLayout(
		textureImageAndDeviceMemory.m_image,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		commandPool,
		graphicsQueue);

	return textureImageAndDeviceMemory;
}



void VulkanStuff(HINSTANCE hInstance, HWND hWnd, Globals& globals) {

	vkcpp::VulkanInstance vulkanInstance = createVulkanInstance();
	vulkanInstance.createDebugMessenger();

	vkcpp::PhysicalDevice physicalDevice = vulkanInstance.getPhysicalDevice(0);
	//auto allQueueFamilyProperties = physicalDevice.getAllQueueFamilyProperties();

	vkcpp::DeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.addExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	vkcpp::DeviceQueueCreateInfo deviceQueueCreateInfo{};
	deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
	deviceCreateInfo.queueCreateInfoCount = 1;

	VkPhysicalDeviceFeatures vkPhysicalDeviceFeatures{};
	deviceCreateInfo.pEnabledFeatures = &vkPhysicalDeviceFeatures;

	vkcpp::Device deviceOriginal(deviceCreateInfo, physicalDevice);

	const int queueFamilyIndex = 0;
	const int queueIndex = 0;
	VkQueue vkGraphicsQueue = deviceOriginal.getDeviceQueue(queueFamilyIndex, queueIndex);
	VkQueue vkPresentationQueue = vkGraphicsQueue;


	VkWin32SurfaceCreateInfoKHR vkWin32SurfaceCreateInfo{};
	vkWin32SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	vkWin32SurfaceCreateInfo.hwnd = hWnd;
	vkWin32SurfaceCreateInfo.hinstance = hInstance;
	vkcpp::Surface surfaceOriginal(vkWin32SurfaceCreateInfo, vulkanInstance, physicalDevice);

	VkSurfaceCapabilitiesKHR vkSurfaceCapabilities = surfaceOriginal.getSurfaceCapabilities();
	//std::vector<VkSurfaceFormatKHR> surfaceFormats = surfaceOriginal.getSurfaceFormats();
	//std::vector<VkPresentModeKHR> presentModes = surfaceOriginal.getSurfacePresentModes();


	const VkFormat swapchainImageFormat = VK_FORMAT_B8G8R8A8_SRGB;

	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = swapchainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription vkSubpassDescription{};
	vkSubpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	vkSubpassDescription.colorAttachmentCount = 1;
	vkSubpassDescription.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency vkSubpassDependency{};
	vkSubpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	vkSubpassDependency.dstSubpass = 0;
	vkSubpassDependency.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	vkSubpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	vkSubpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo vkRenderPassCreateInfo{};
	vkRenderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	vkRenderPassCreateInfo.attachmentCount = 1;
	vkRenderPassCreateInfo.pAttachments = &colorAttachment;
	vkRenderPassCreateInfo.subpassCount = 1;
	vkRenderPassCreateInfo.pSubpasses = &vkSubpassDescription;
	vkRenderPassCreateInfo.dependencyCount = 1;
	vkRenderPassCreateInfo.pDependencies = &vkSubpassDependency;

	vkcpp::RenderPass renderPassOriginal(vkRenderPassCreateInfo, deviceOriginal);

	vkcpp::Swapchain_ImageViews_FrameBuffers::setDevice(deviceOriginal);

	const VkColorSpaceKHR swapchainImageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	const VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

	VkSwapchainCreateInfoKHR swapchainCreateInfo{};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = surfaceOriginal;
	swapchainCreateInfo.minImageCount = SWAP_CHAIN_IMAGE_COUNT;
	swapchainCreateInfo.imageFormat = swapchainImageFormat;
	swapchainCreateInfo.imageColorSpace = swapchainImageColorSpace;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCreateInfo.queueFamilyIndexCount = 0;
	swapchainCreateInfo.pQueueFamilyIndices = nullptr;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.presentMode = swapchainPresentMode;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;


	vkcpp::Swapchain_ImageViews_FrameBuffers swapchainImageViewsFrameBuffers(swapchainCreateInfo, surfaceOriginal);
	swapchainImageViewsFrameBuffers.setRenderPass(renderPassOriginal);

	swapchainImageViewsFrameBuffers.recreateSwapchainImageViewsFrameBuffers();

	const int64_t vertexSize = sizeof(g_vertices[0]) * static_cast<int>(g_vertices.size());
	vkcpp::Buffer_DeviceMemory vertexBufferAndDeviceMemory(
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		vertexSize,
		(void*)g_vertices.data(),
		deviceOriginal);

	const int64_t vertexIndexSize = sizeof(g_vertexIndices[0]) * static_cast<int>(g_vertexIndices.size());
	vkcpp::Buffer_DeviceMemory vertexIndicesBufferAndDeviceMemory(
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		vertexIndexSize,
		(void*)g_vertexIndices.data(),
		deviceOriginal);

	{
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			g_allDrawingFrames.drawingFrameAt(i).moveUniformMemoryBuffer(
				vkcpp::Buffer_DeviceMemory(
					VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					sizeof(ModelViewProjTransform),
					deviceOriginal
				));
		}
	}

	vkcpp::ShaderModule	vertShaderModule =
		vkcpp::ShaderModule::createShaderModuleFromFile("C:/Shaders/VulkanTriangle/vert4.spv", deviceOriginal);
	vkcpp::ShaderModule	fragShaderModule =
		vkcpp::ShaderModule::createShaderModuleFromFile("C:/Shaders/VulkanTriangle/frag4.spv", deviceOriginal);

	VkCommandPoolCreateInfo commandPoolCreateInfo{};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
	vkcpp::CommandPool commandPoolOriginal(commandPoolCreateInfo, deviceOriginal);


	vkcpp::Image_DeviceMemory textureImage_deviceMemory =
		createTextureImageFromFile(
			"c:/vulkan/texture.jpg", deviceOriginal, commandPoolOriginal, vkGraphicsQueue);

	vkcpp::ImageViewCreateInfo textureImageViewCreateInfo(
		VK_IMAGE_VIEW_TYPE_2D,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_ASPECT_COLOR_BIT);
	textureImageViewCreateInfo.image = textureImage_deviceMemory.m_image;

	vkcpp::ImageView textureImageView(textureImageViewCreateInfo, deviceOriginal);
	vkcpp::SamplerCreateInfo textureSamplerCreateInfo;
	vkcpp::Sampler textureSampler(textureSamplerCreateInfo, deviceOriginal);


	vkcpp::DescriptorSetLayout descriptorSetLayoutOriginal = createDrawingFrameDescriptorSetLayout(deviceOriginal);
	vkcpp::DescriptorPool descriptorPoolOriginal = createDescriptorPool(deviceOriginal);
	createDescriptorSets(
		deviceOriginal,
		descriptorPoolOriginal, descriptorSetLayoutOriginal,
		textureImageView, textureSampler,
		g_allDrawingFrames);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ descriptorSetLayoutOriginal };

	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = (uint32_t)descriptorSetLayouts.size();
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	vkcpp::PipelineLayout pipelineLayout(pipelineLayoutInfo, deviceOriginal);

	GraphicsPipelineConfig graphicsPipelineConfig;
	graphicsPipelineConfig.setInputAssemblyTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	graphicsPipelineConfig.setViewportExtent(vkSurfaceCapabilities.currentExtent);
	graphicsPipelineConfig.setPipelineLayout(pipelineLayout);
	graphicsPipelineConfig.setRenderPass(renderPassOriginal);
	graphicsPipelineConfig.setVertexShaderModule(vertShaderModule);
	graphicsPipelineConfig.setFragmentShaderModule(fragShaderModule);

	vkcpp::GraphicsPipeline graphicsPipeline(graphicsPipelineConfig.assemble(), deviceOriginal);


	{
		VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.commandPool = commandPoolOriginal;
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocateInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
		std::vector<vkcpp::CommandBuffer> commandBuffers =
			vkcpp::CommandBuffer::allocateCommandBuffers(commandBufferAllocateInfo, commandPoolOriginal);
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			g_allDrawingFrames.drawingFrameAt(i).moveCommandBuffer(std::move(commandBuffers[i]));
		}
	}

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		g_allDrawingFrames.drawingFrameAt(i).moveImageAvailableSemaphore(
			vkcpp::Semaphore(semaphoreInfo, deviceOriginal));
		g_allDrawingFrames.drawingFrameAt(i).moveRenderFinishedSemaphore(
			vkcpp::Semaphore(semaphoreInfo, deviceOriginal));
		g_allDrawingFrames.drawingFrameAt(i).moveInFlightFence(
			vkcpp::Fence(fenceInfo, deviceOriginal));
	}


#undef globals

	globals.g_vulkanInstance = std::move(vulkanInstance);
	globals.g_physicalDevice = std::move(physicalDevice);
	globals.g_deviceOriginal = std::move(deviceOriginal);

	globals.g_hInstance = hInstance;
	globals.g_hWnd = hWnd;
	globals.g_surfaceOriginal = std::move(surfaceOriginal);


	globals.g_vertexBufferAndDeviceMemory = std::move(vertexBufferAndDeviceMemory);
	globals.g_vertexIndicesBufferAndDeviceMemory = std::move(vertexIndicesBufferAndDeviceMemory);

	globals.g_descriptorSetLayoutOriginal = std::move(descriptorSetLayoutOriginal);
	globals.g_descriptorPoolOriginal = std::move(descriptorPoolOriginal);

	globals.g_pipelineLayout = std::move(pipelineLayout);
	globals.g_graphicsPipeline = std::move(graphicsPipeline);


	globals.g_commandPoolOriginal = std::move(commandPoolOriginal);

	globals.g_vkGraphicsQueue = vkGraphicsQueue;
	globals.g_vkPresentationQueue = vkPresentationQueue;

	globals.g_renderPassOriginal = std::move(renderPassOriginal);
	globals.g_swapchainImageViewsFrameBuffers = std::move(swapchainImageViewsFrameBuffers);

	globals.g_vertShaderModule = std::move(vertShaderModule);
	globals.g_fragShaderModule = std::move(fragShaderModule);

	globals.g_textureImage_deviceMemory = std::move(textureImage_deviceMemory);
	globals.g_textureImageView = std::move(textureImageView);
	globals.g_textureSampler = std::move(textureSampler);

}

void updateUniformBuffer(
	vkcpp::Buffer_DeviceMemory& uniformBufferMemory,
	const VkExtent2D				swapchainImageExtent
) {
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	ModelViewProjTransform modelViewProjTransform{};
	modelViewProjTransform.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	modelViewProjTransform.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	modelViewProjTransform.proj = glm::perspective(glm::radians(45.0f), (float)swapchainImageExtent.width / (float)swapchainImageExtent.height, 0.1f, 10.0f);
	modelViewProjTransform.proj[1][1] *= -1;

	memcpy(
		uniformBufferMemory.m_mappedMemory,
		&modelViewProjTransform,
		sizeof(modelViewProjTransform));
}


void drawFrame(Globals& globals)
{
	//std::cout << "--->>>drawFrame\n";
	if (!globals.g_swapchainImageViewsFrameBuffers.canDraw()) {
		return;
	}


	const int	currentFrameToDrawIndex = g_nextFrameToDrawIndex;
	DrawingFrame& currentDrawingFrame = g_allDrawingFrames.drawingFrameAt(currentFrameToDrawIndex);

	VkDevice vkDevice = currentDrawingFrame.getVkDevice();
	vkcpp::CommandBuffer commandBuffer = currentDrawingFrame.m_commandBuffer;

	//	Wait for this drawing frame to be free
	currentDrawingFrame.m_inFlightFence.wait();

	uint32_t	swapchainImageIndex;
	vkAcquireNextImageKHR(
		vkDevice,
		globals.g_swapchainImageViewsFrameBuffers.vkSwapchain(),
		UINT64_MAX,
		currentDrawingFrame.m_imageAvailableSemaphore,
		VK_NULL_HANDLE,
		&swapchainImageIndex);

	commandBuffer.reset();
	recordCommandBuffer(
		commandBuffer,
		swapchainImageIndex,
		globals.g_swapchainImageViewsFrameBuffers,
		globals.g_vertexBufferAndDeviceMemory.m_buffer,
		globals.g_vertexIndicesBufferAndDeviceMemory.m_buffer,
		currentDrawingFrame.m_descriptorSet,
		globals.g_pipelineLayout,
		globals.g_graphicsPipeline);

	updateUniformBuffer(
		currentDrawingFrame.m_uniformBufferMemory,
		globals.g_swapchainImageViewsFrameBuffers.getImageExtent()
	);


	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore imageAvailableSemaphores[] = { currentDrawingFrame.m_imageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = imageAvailableSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	VkCommandBuffer commandBuffers[] = { commandBuffer };
	submitInfo.pCommandBuffers = commandBuffers;

	VkSemaphore renderFinishedSemaphores[] = { currentDrawingFrame.m_renderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = renderFinishedSemaphores;

	//	Show that this drawing frame is now in use ..
	//	Fence will be signaled when graphics queue is finished.
	currentDrawingFrame.m_inFlightFence.reset();

	if (vkQueueSubmit(
		globals.g_vkGraphicsQueue,
		1, &submitInfo,
		currentDrawingFrame.m_inFlightFence) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = renderFinishedSemaphores;
	VkSwapchainKHR swapchains[] = { globals.g_swapchainImageViewsFrameBuffers.vkSwapchain() };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapchains;
	presentInfo.pImageIndices = &swapchainImageIndex;
	presentInfo.pResults = nullptr; // Optional
	vkQueuePresentKHR(globals.g_vkPresentationQueue, &presentInfo);

	g_nextFrameToDrawIndex = (g_nextFrameToDrawIndex + 1) % MAX_FRAMES_IN_FLIGHT;


	//std::cout << "<<<---drawFrame\n";
}


//TODO: need a better way to do this
bool g_windowPosChanged = false;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		hdc = hdc;
		EndPaint(hWnd, &ps);
	}
	break;

	case WM_ERASEBKGND:
		return 1L;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_WINDOWPOSCHANGED:
		std::cout << "WM_WINDOWPOSCHANGED\n";
		g_windowPosChanged = true;
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}


ATOM RegisterMyWindowClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex{};

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	//	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINDOWSPROJECT1));
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	//	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_WINDOWSPROJECT1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;
	//	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	wcex.hIconSm = NULL;

	return RegisterClassExW(&wcex);
}


HWND CreateFirstWindow(HINSTANCE hInstance)
{

	HWND hWnd = CreateWindowW(
		szWindowClass,
		szTitle,
		WS_OVERLAPPEDWINDOW,
		windowPositionX,
		windowPositionY,
		windowWidth,
		windowHeight,
		nullptr,
		nullptr,
		hInstance,
		nullptr
	);

	if (!hWnd)
	{
		return hWnd;
	}

	ShowWindow(hWnd, TRUE);
	UpdateWindow(hWnd);

	return hWnd;
}

void MessageLoop(Globals& globals) {

	MSG msg;

	bool	done = false;
	bool	graphicsValid = true;

	while (!done) {
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT) {
				done = true;
				break;
			}

			if (g_windowPosChanged) {
				if (graphicsValid) {
					try {
						globals.g_swapchainImageViewsFrameBuffers.recreateSwapchainImageViewsFrameBuffers();
					}
					catch (vkcpp::ShutdownException&)
					{
						std::cout << "Yikes\n";
						graphicsValid = false;
					}
				}
				g_windowPosChanged = false;
			}

			if (msg.message == WM_KEYDOWN) {
				if (msg.wParam == 'D') {
					std::cout << "Dump Vulkan Info\n";
				}
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		drawFrame(globals);
	}
	return;
}

void vectorTest() {

	class Obj {
		int	m_data;
		int m_cc = 0;

	public:

		//Obj() {
		//	std::cout << "Obj(): " << m_data << " " << m_cc << "\n";
		//}

		~Obj() {
			std::cout << "~Obj: " << m_data << " " << m_cc << "\n";
		}

		Obj(int data)
			: m_data(data) {
			std::cout << "Obj(int): " << m_data << " " << m_cc << "\n";
		}


		Obj(const Obj& other)
			: m_data(other.m_data)
			, m_cc(other.m_cc + 1) {
			std::cout << "Obj(const Obj&): " << m_data << " " << m_cc << "\n";
		}

		Obj& operator=(const Obj& other) {
			m_data = other.m_data;
			m_cc = other.m_cc + 1;
			std::cout << "Obj& =(const Obj&): " << m_data << " " << m_cc << "\n";
			return *this;
		}

		Obj(Obj&& other) noexcept
			: m_data(other.m_data)
			, m_cc(other.m_cc) {
			other.m_cc = -1;
			std::cout << "Obj(Obj&&): " << m_data << " " << m_cc << "\n";
		}

		Obj& operator=(Obj&& other) noexcept {
			m_data = other.m_data;
			m_cc = other.m_cc + 1;
			other.m_cc = -1;
			std::cout << "Obj& =(Obj&&): " << m_data << " " << m_cc << "\n";
			return *this;
		}

	};
	if constexpr (std::is_trivially_copyable_v<Obj>) {
		std::cout << "is is_trivially_copyable\n";
	}
	else {
		std::cout << "is is_trivially_copyable\n";
	}


	Obj obj1(1);
	Obj obj2(2);
	Obj obj3(3);
	Obj obj4(4);
	obj3 = obj4;

	{
		std::vector<Obj>	objvec;
		objvec.emplace_back(5);
		objvec.push_back(obj1);
	}

	{
		std::vector<Obj>	objvec;
		objvec.emplace_back(5);
		objvec.push_back(obj1);
		std::cout << "Before clear\n";
		objvec.clear();
		std::cout << "After clear\n";
	}

	return;


}

int main()
{
	std::cout << "Hello World!\n";


	//vectorTest();


	HINSTANCE hInstance = GetModuleHandle(NULL);
	RegisterMyWindowClass(hInstance);
	HWND hWnd = CreateFirstWindow(hInstance);

	VulkanStuff(hInstance, hWnd, g_globals);

	MessageLoop(g_globals);

	//	Wait for device before exiting and cleaning up globals.
	vkDeviceWaitIdle(g_globals.g_deviceOriginal);


}
