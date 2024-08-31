// VulkanAgain.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pragmas.hpp"

#include <iostream>
#include <fstream>
#include <array>
#include <chrono>

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define VK_USE_PLATFORM_WIN32_KHR

#include "VulkanCpp.hpp"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


struct Vertex {
	glm::vec2	pos;
	glm::vec3	color;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};

		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}
};

const std::vector<Vertex> vertices = {
	{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
	{{0.0f, 1.0f}, {1.0f, 1.0f, 0.0f}},
	{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
};

struct ModelViewProjTransform {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
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



class BufferAndDeviceMemoryMapped {

	BufferAndDeviceMemoryMapped(vkcpp::Buffer&& buffer, vkcpp::DeviceMemory&& deviceMemory, void* mappedMemory)
		: m_buffer(std::move(buffer))
		, m_deviceMemory(std::move(deviceMemory))
		, m_mappedMemory(mappedMemory) {
	}

public:

	vkcpp::Buffer		m_buffer;
	vkcpp::DeviceMemory	m_deviceMemory;
	void* m_mappedMemory = nullptr;

	BufferAndDeviceMemoryMapped() {}

	BufferAndDeviceMemoryMapped(const BufferAndDeviceMemoryMapped&) = delete;
	BufferAndDeviceMemoryMapped& operator=(const BufferAndDeviceMemoryMapped&) = delete;

	BufferAndDeviceMemoryMapped(BufferAndDeviceMemoryMapped&& other) noexcept
		: m_buffer(std::move(other.m_buffer))
		, m_deviceMemory(std::move(other.m_deviceMemory))
		, m_mappedMemory(other.m_mappedMemory) {
	}

	BufferAndDeviceMemoryMapped& operator=(BufferAndDeviceMemoryMapped&& other) noexcept {
		if (this == &other) {
			return *this;
		}
		(*this).~BufferAndDeviceMemoryMapped();
		new(this) BufferAndDeviceMemoryMapped(std::move(other));
		return *this;
	}


	static BufferAndDeviceMemoryMapped create(
		vkcpp::Device device,
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties
	) {
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		vkcpp::Buffer buffer(bufferInfo, device);

		VkMemoryRequirements memRequirements = buffer.getMemoryRequirements();

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = device.findMemoryTypeIndex(memRequirements.memoryTypeBits, properties);

		vkcpp::DeviceMemory deviceMemory(allocInfo, device);

		if (vkBindBufferMemory(device, buffer, deviceMemory, 0) != VK_SUCCESS) {
			throw std::runtime_error("failed to bind buffer memory!");
		}

		void* mappedMemory;
		vkMapMemory(device, deviceMemory, 0, size, 0, &mappedMemory);

		return BufferAndDeviceMemoryMapped(std::move(buffer), std::move(deviceMemory), mappedMemory);

	}

};


class DrawingFrame {

public:
	vkcpp::Fence	m_inFlightFence;
	vkcpp::Semaphore m_imageAvailableSemaphore;
	vkcpp::Semaphore m_renderFinishedSemaphore;
	VkCommandBuffer	m_vkCommandBuffer = nullptr;
	BufferAndDeviceMemoryMapped	m_uniformBufferMemory;
	VkDescriptorSet	m_vkDescriptorSet = nullptr;

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

	void setCommandBuffer(VkCommandBuffer vkCommandBuffer) {
		m_vkCommandBuffer = vkCommandBuffer;
	}

	void moveUniformMemoryBuffer(BufferAndDeviceMemoryMapped&& bufferAndDeviceMemoryMapped) {
		m_uniformBufferMemory = std::move(bufferAndDeviceMemoryMapped);
	}

	void setDescriptorSet(VkDescriptorSet vkDescriptorSet) {
		m_vkDescriptorSet = vkDescriptorSet;
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
	VkCommandBuffer		commandBuffer,
	uint32_t			swapChainImageIndex,
	vkcpp::SwapChainImageViewsFrameBuffers& swapChainImageViewsFrameBuffers,
	VkBuffer			vkVertexBuffer,
	VkDescriptorSet		descriptorSet,
	VkPipelineLayout	vkPipelineLayout,
	VkPipeline			graphicsPipeline
) {
	const VkExtent2D swapChainImageExtent = swapChainImageViewsFrameBuffers.getImageExtent();

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = swapChainImageViewsFrameBuffers.getRenderPass();
	renderPassInfo.framebuffer = swapChainImageViewsFrameBuffers.getFrameBuffer(swapChainImageIndex);
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = swapChainImageExtent;
	VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapChainImageExtent.width);
	viewport.height = static_cast<float>(swapChainImageExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainImageExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

	VkBuffer vertexBuffers[] = { vkVertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		vkPipelineLayout,
		0, 1,
		&descriptorSet,
		0, nullptr);
	vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);

	vkCmdEndRenderPass(commandBuffer);
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}

}



struct UniformBuffersMemory {

	std::vector<BufferAndDeviceMemoryMapped>	m_uniformBufferMemory;

	void createUniformBuffers(
		vkcpp::Device device,
		int count
	) {
		VkDeviceSize bufferSize = sizeof(ModelViewProjTransform);

		m_uniformBufferMemory.resize(count);

		for (int i = 0; i < count; i++) {
			m_uniformBufferMemory[i] = std::move(
				BufferAndDeviceMemoryMapped::create(
					device,
					bufferSize,
					VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
				));
		}
	}
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
	vkcpp::Surface	g_surfaceOriginal;
	vkcpp::PhysicalDevice	g_physicalDevice;
	vkcpp::Device	g_deviceOriginal;

	VkQueue				g_vkGraphicsQueue = nullptr;
	VkQueue				g_vkPresentationQueue = nullptr;

	vkcpp::RenderPass g_renderPassOriginal;

	uint32_t						g_swapChainMinImageCount = SWAP_CHAIN_IMAGE_COUNT;
	vkcpp::SwapChainImageViewsFrameBuffers	g_swapChainImageViewsFrameBuffers;

	vkcpp::ShaderModule g_vertShaderModule;
	vkcpp::ShaderModule g_fragShaderModule;

	vkcpp::DescriptorPool			g_descriptorPoolOriginal;
	vkcpp::DescriptorSetLayout		g_descriptorSetLayoutOriginal;

	vkcpp::PipelineLayout	g_pipelineLayout;
	vkcpp::GraphicsPipeline	g_graphicsPipeline;

	vkcpp::Buffer			g_vertexBuffer;
	vkcpp::DeviceMemory		g_vertexDeviceMemory;

	vkcpp::CommandPool		g_commandPoolOriginal;

};
Globals g_globals;
AllDrawingFrames g_allDrawingFrames(MAX_FRAMES_IN_FLIGHT);



vkcpp::DescriptorSetLayout createUboDescriptorSetLayout(VkDevice vkDevice) {
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &uboLayoutBinding;

	return vkcpp::DescriptorSetLayout(layoutInfo, vkDevice);
}


vkcpp::DescriptorPool createDescriptorPool(VkDevice vkDevice) {

	VkDescriptorPoolSize poolSize{};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	VkDescriptorPoolCreateInfo poolCreateInfo{};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.poolSizeCount = 1;
	poolCreateInfo.pPoolSizes = &poolSize;
	poolCreateInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	return vkcpp::DescriptorPool(poolCreateInfo, vkDevice);
}


void createDescriptorSets(
	vkcpp::Device device,
	VkDescriptorPool		descriptorPool,
	vkcpp::DescriptorSetLayout descriptorSetLayout,
	AllDrawingFrames& allDrawingFrames
) {
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	std::vector<VkDescriptorSet> descriptorSets;
	descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = allDrawingFrames.drawingFrameAt(i).m_uniformBufferMemory.m_buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(ModelViewProjTransform);

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSets[i];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
		allDrawingFrames.drawingFrameAt(i).setDescriptorSet(descriptorSets[i]);
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
	std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = Vertex::getAttributeDescriptions();


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

#ifdef NEVER_DEFINE

class ImageCreateInfo : public VkImageCreateInfo {

public:

	ImageCreateInfo()
		: VkImageCreateInfo{} {
		sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;

		imageType = VK_IMAGE_TYPE_2D;

		extent.depth = 1;
		mipLevels = 1;
		arrayLayers = 1;
		initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		samples = VK_SAMPLE_COUNT_1_BIT;
		sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	}

};

void createImage(
	uint32_t width,
	uint32_t height,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlags usage,
	VkMemoryPropertyFlags properties,
	VkPhysicalDevice vkPhysicalDevice,
	VkDevice vkDevice
) {

	//VkImage& image,
	//	VkDeviceMemory& imageMemory

	ImageCreateInfo imageInfo;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.usage = usage;

	vkcpp::Image image(imageInfo, vkDevice);


	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(vkDevice, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryTypeIndex(vkPhysicalDevice, memRequirements.memoryTypeBits, properties);
	vkcpp::DeviceMemory imageDeviceMemory(allocInfo, vkDevice);

	vkBindImageMemory(vkDevice, image, imageDeviceMemory, 0);

}


VkCommandBuffer beginSingleTimeCommands(vkcpp::CommandPool commandPool) {

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	// TODO: need to figure out command buffers better.
	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(commandPool.getOwner(), &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void endSingleTimeCommands(
	VkCommandBuffer vkCommandBuffer,
	VkQueue  vkGraphicsQueue,
	VkDevice vkDevice,
	VkCommandPool	vkCommandPool) {

	vkEndCommandBuffer(vkCommandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &vkCommandBuffer;

	vkQueueSubmit(vkGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(vkGraphicsQueue);

	vkFreeCommandBuffers(vkDevice, vkCommandPool, 1, &vkCommandBuffer);
}


void transitionImageLayout(
	VkImage vkImage,
	VkFormat vkFormat,
	VkImageLayout oldLayout,
	VkImageLayout newLayout,
	vkcpp::CommandPool commandPool,
	VkQueue  vkGraphicsQueue,
	VkDevice vkDevice,
	) {

	VkCommandBuffer commandBuffer = beginSingleTimeCommands(commandPool);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = vkImage;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	endSingleTimeCommands(commandBuffer, vkGraphicsQueue, vkDevice, commandPool);

}



void copyBufferToImage(
	VkBuffer buffer,
	VkImage image,
	uint32_t width,
	uint32_t height
) {

	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	endSingleTimeCommands(commandBuffer);
}

const char* g_textureImageFileName = "textures/texture.jpg"

void createTextureImageFromFile(const char* fileName) {
	int	texWidth;
	int texHeight;
	int	texChannels;
	stbi_uc* pixels = stbi_load(fileName, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}


	VkDeviceSize imageSize = texWidth * texHeight * 4;


	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(
		imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory
	);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(device, stagingBufferMemory);

	stbi_image_free(pixels);

	createImage(
		texWidth, texHeight,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		textureImage,
		textureImageMemory
	);

	transitionImageLayout(
		textureImage,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	);
	copyBufferToImage(
		stagingBuffer,
		textureImage,
		static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight)
	);
	transitionImageLayout(
		textureImage,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}
#endif


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
	vkcpp::Device deviceClone = deviceOriginal;

	const int queueFamilyIndex = 0;
	const int queueIndex = 0;
	VkQueue vkGraphicsQueue = deviceClone.getDeviceQueue(queueFamilyIndex, queueIndex);
	VkQueue vkPresentationQueue = vkGraphicsQueue;


	VkWin32SurfaceCreateInfoKHR vkWin32SurfaceCreateInfo{};
	vkWin32SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	vkWin32SurfaceCreateInfo.hwnd = hWnd;
	vkWin32SurfaceCreateInfo.hinstance = hInstance;
	vkcpp::Surface surfaceOriginal(vkWin32SurfaceCreateInfo, vulkanInstance, physicalDevice);

	VkSurfaceCapabilitiesKHR vkSurfaceCapabilities = surfaceOriginal.getSurfaceCapabilities();
	//std::vector<VkSurfaceFormatKHR> surfaceFormats = surfaceOriginal.getSurfaceFormats();
	//std::vector<VkPresentModeKHR> presentModes = surfaceOriginal.getSurfacePresentModes();


	const VkFormat swapChainImageFormat = VK_FORMAT_B8G8R8A8_SRGB;

	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = swapChainImageFormat;
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

	vkcpp::RenderPass renderPassOriginal(vkRenderPassCreateInfo, deviceClone);

	vkcpp::SwapChainImageViewsFrameBuffers::setDevice(deviceClone);

	const VkColorSpaceKHR swapChainImageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	const VkPresentModeKHR swapChainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

	VkSwapchainCreateInfoKHR swapChainCreateInfo{};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = surfaceOriginal;
	swapChainCreateInfo.minImageCount = SWAP_CHAIN_IMAGE_COUNT;
	swapChainCreateInfo.imageFormat = swapChainImageFormat;
	swapChainCreateInfo.imageColorSpace = swapChainImageColorSpace;
	swapChainCreateInfo.imageArrayLayers = 1;
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapChainCreateInfo.queueFamilyIndexCount = 0;
	swapChainCreateInfo.pQueueFamilyIndices = nullptr;
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCreateInfo.presentMode = swapChainPresentMode;
	swapChainCreateInfo.clipped = VK_TRUE;
	swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;


	vkcpp::SwapChainImageViewsFrameBuffers swapChainImageViewsFrameBuffers(swapChainCreateInfo, surfaceOriginal);
	swapChainImageViewsFrameBuffers.setRenderPass(renderPassOriginal);

	swapChainImageViewsFrameBuffers.recreateSwapChainImageViewsFrameBuffers();

	VkBufferCreateInfo vkBufferCreateInfo{};
	vkBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vkBufferCreateInfo.size = sizeof(vertices[0]) * vertices.size();
	vkBufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	vkBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	vkcpp::Buffer vertexBuffer(vkBufferCreateInfo, deviceClone);
	vkcpp::DeviceMemory vertexDeviceMemory =
		vertexBuffer.allocateDeviceMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	vkBindBufferMemory(deviceClone, vertexBuffer, vertexDeviceMemory, 0);
	void* data;
	vkMapMemory(deviceClone, vertexDeviceMemory, 0, vkBufferCreateInfo.size, 0, &data);
	memcpy(data, vertices.data(), (size_t)vkBufferCreateInfo.size);
	vkUnmapMemory(deviceClone, vertexDeviceMemory);


	{
		UniformBuffersMemory  uniformBuffersMemory;
		uniformBuffersMemory.createUniformBuffers(deviceClone, MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			g_allDrawingFrames.drawingFrameAt(i).moveUniformMemoryBuffer(std::move(uniformBuffersMemory.m_uniformBufferMemory[i]));
		}
	}


	vkcpp::ShaderModule	vertShaderModule =
		vkcpp::ShaderModule::createShaderModuleFromFile("C:/Shaders/VulkanTriangle/vert2.spv", deviceClone);
	vkcpp::ShaderModule	fragShaderModule =
		vkcpp::ShaderModule::createShaderModuleFromFile("C:/Shaders/VulkanTriangle/frag.spv", deviceClone);



	vkcpp::DescriptorPool descriptorPoolOriginal = createDescriptorPool(deviceClone);

	vkcpp::DescriptorSetLayout descriptorSetLayoutOriginal = createUboDescriptorSetLayout(deviceClone);
	createDescriptorSets(deviceClone, descriptorPoolOriginal, descriptorSetLayoutOriginal, g_allDrawingFrames);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ descriptorSetLayoutOriginal };

	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = (uint32_t)descriptorSetLayouts.size();
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	vkcpp::PipelineLayout pipelineLayout(pipelineLayoutInfo, deviceClone);

	GraphicsPipelineConfig graphicsPipelineConfig;
	graphicsPipelineConfig.setInputAssemblyTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN);
	graphicsPipelineConfig.setViewportExtent(vkSurfaceCapabilities.currentExtent);
	graphicsPipelineConfig.setPipelineLayout(pipelineLayout);
	graphicsPipelineConfig.setRenderPass(renderPassOriginal);
	graphicsPipelineConfig.setVertexShaderModule(vertShaderModule);
	graphicsPipelineConfig.setFragmentShaderModule(fragShaderModule);

	vkcpp::GraphicsPipeline graphicsPipeline(graphicsPipelineConfig.assemble(), deviceClone);

	VkCommandPoolCreateInfo commandPoolCreateInfo{};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
	vkcpp::CommandPool commandPoolOriginal(commandPoolCreateInfo, deviceClone);

	{
		VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.commandPool = commandPoolOriginal;
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocateInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
		std::vector<VkCommandBuffer> commandBuffers = commandPoolOriginal.allocateCommandBuffers(commandBufferAllocateInfo);
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			g_allDrawingFrames.drawingFrameAt(i).setCommandBuffer(commandBuffers[i]);
		}
	}

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		g_allDrawingFrames.drawingFrameAt(i).moveImageAvailableSemaphore(
			vkcpp::Semaphore(semaphoreInfo, deviceClone));
		g_allDrawingFrames.drawingFrameAt(i).moveRenderFinishedSemaphore(
			vkcpp::Semaphore(semaphoreInfo, deviceClone));
		g_allDrawingFrames.drawingFrameAt(i).moveInFlightFence(
			vkcpp::Fence(fenceInfo, deviceClone));
	}

#undef globals

	globals.g_vulkanInstance = std::move(vulkanInstance);
	globals.g_physicalDevice = std::move(physicalDevice);
	globals.g_deviceOriginal = std::move(deviceOriginal);

	globals.g_hInstance = hInstance;
	globals.g_hWnd = hWnd;
	globals.g_surfaceOriginal = std::move(surfaceOriginal);


	globals.g_vertexBuffer = std::move(vertexBuffer);
	globals.g_vertexDeviceMemory = std::move(vertexDeviceMemory);

	globals.g_descriptorSetLayoutOriginal = std::move(descriptorSetLayoutOriginal);
	globals.g_descriptorPoolOriginal = std::move(descriptorPoolOriginal);

	globals.g_pipelineLayout = std::move(pipelineLayout);
	globals.g_graphicsPipeline = std::move(graphicsPipeline);


	globals.g_commandPoolOriginal = std::move(commandPoolOriginal);

	globals.g_vkGraphicsQueue = vkGraphicsQueue;
	globals.g_vkPresentationQueue = vkPresentationQueue;

	globals.g_renderPassOriginal = std::move(renderPassOriginal);
	globals.g_swapChainImageViewsFrameBuffers = std::move(swapChainImageViewsFrameBuffers);

	globals.g_vertShaderModule = std::move(vertShaderModule);
	globals.g_fragShaderModule = std::move(fragShaderModule);

}

void updateUniformBuffer(
	BufferAndDeviceMemoryMapped& uniformBufferMemory,
	const VkExtent2D				swapChainImageExtent
) {
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	ModelViewProjTransform modelViewProjTransform{};
	modelViewProjTransform.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	modelViewProjTransform.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	modelViewProjTransform.proj = glm::perspective(glm::radians(45.0f), (float)swapChainImageExtent.width / (float)swapChainImageExtent.height, 0.1f, 10.0f);
	modelViewProjTransform.proj[1][1] *= -1;

	memcpy(
		uniformBufferMemory.m_mappedMemory,
		&modelViewProjTransform,
		sizeof(modelViewProjTransform));
}


void drawFrame(Globals& globals)
{
	//std::cout << "--->>>drawFrame\n";
	if (!globals.g_swapChainImageViewsFrameBuffers.canDraw()) {
		return;
	}


	const int	currentFrameToDrawIndex = g_nextFrameToDrawIndex;
	DrawingFrame& currentDrawingFrame = g_allDrawingFrames.drawingFrameAt(currentFrameToDrawIndex);;
	VkDevice vkDevice = currentDrawingFrame.getVkDevice();
	vkcpp::Fence inFlightFence = currentDrawingFrame.m_inFlightFence;
	vkcpp::Semaphore imageAvailableSemaphore = currentDrawingFrame.m_imageAvailableSemaphore;
	vkcpp::Semaphore renderFinishedSemaphore = currentDrawingFrame.m_renderFinishedSemaphore;
	VkCommandBuffer& vkCommandBuffer = currentDrawingFrame.m_vkCommandBuffer;
	VkDescriptorSet& vkDescriptorSet = currentDrawingFrame.m_vkDescriptorSet;

	inFlightFence.wait();

	uint32_t	swapChainImageIndex;
	vkAcquireNextImageKHR(
		vkDevice,
		globals.g_swapChainImageViewsFrameBuffers.vkSwapchain(),
		UINT64_MAX,
		imageAvailableSemaphore,
		VK_NULL_HANDLE,
		&swapChainImageIndex);

	vkResetCommandBuffer(vkCommandBuffer, 0);
	recordCommandBuffer(
		vkCommandBuffer,
		swapChainImageIndex,
		globals.g_swapChainImageViewsFrameBuffers,
		globals.g_vertexBuffer,
		vkDescriptorSet,
		globals.g_pipelineLayout,
		globals.g_graphicsPipeline);

	updateUniformBuffer(
		currentDrawingFrame.m_uniformBufferMemory,
		globals.g_swapChainImageViewsFrameBuffers.getImageExtent()
	);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore imageAvailableSemaphores[] = { imageAvailableSemaphore };

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = imageAvailableSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;

	submitInfo.pCommandBuffers = &vkCommandBuffer;

	VkSemaphore renderFinishedSemaphores[] = { renderFinishedSemaphore };

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = renderFinishedSemaphores;

	inFlightFence.reset();

	if (vkQueueSubmit(globals.g_vkGraphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = renderFinishedSemaphores;
	VkSwapchainKHR swapChains[] = { globals.g_swapChainImageViewsFrameBuffers.vkSwapchain() };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &swapChainImageIndex;
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
						globals.g_swapChainImageViewsFrameBuffers.recreateSwapChainImageViewsFrameBuffers();
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
