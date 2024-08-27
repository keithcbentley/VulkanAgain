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

uint32_t findMemoryTypeIndex(
	vulcpp::PhysicalDevice& physicalDevice,
	uint32_t typeFilter,
	VkMemoryPropertyFlags properties
) {

	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}
	throw std::runtime_error("failed to find suitable memory type!");
}



class BufferAndDeviceMemoryMapped {

	BufferAndDeviceMemoryMapped(vulcpp::Buffer&& buffer, vulcpp::DeviceMemory&& deviceMemory, void* mappedMemory)
		: m_buffer(std::move(buffer))
		, m_deviceMemory(std::move(deviceMemory))
		, m_mappedMemory(mappedMemory) {
	}

public:

	vulcpp::Buffer			m_buffer;
	vulcpp::DeviceMemory	m_deviceMemory;
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
		vulcpp::PhysicalDevice physicalDevice,
		vulcpp::Device device,
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties
	) {
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		vulcpp::Buffer buffer(bufferInfo, device);

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryTypeIndex(physicalDevice, memRequirements.memoryTypeBits, properties);

		vulcpp::DeviceMemory deviceMemory(allocInfo, device);

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
	vulcpp::Fence	m_inFlightFence;
	vulcpp::Semaphore m_imageAvailableSemaphore;
	vulcpp::Semaphore m_renderFinishedSemaphore;
	VkCommandBuffer	m_vkCommandBuffer = nullptr;
	BufferAndDeviceMemoryMapped	m_uniformBufferMemory;
	VkDescriptorSet	m_vkDescriptorSet = nullptr;

	DrawingFrame() {};

	DrawingFrame(const DrawingFrame&) = delete;
	DrawingFrame& operator=(const DrawingFrame&) = delete;
	DrawingFrame(DrawingFrame&&) = delete;
	DrawingFrame& operator=(DrawingFrame&&) = delete;

	void moveInFlightFence(vulcpp::Fence&& fence) {
		m_inFlightFence = std::move(fence);
	}

	void moveImageAvailableSemaphore(vulcpp::Semaphore&& semaphore) {
		m_imageAvailableSemaphore = std::move(semaphore);
	}

	void moveRenderFinishedSemaphore(vulcpp::Semaphore&& semaphore) {
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

	VkDevice getDevice() {
		return m_inFlightFence.getOwner();
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

	int	frameCount() { return m_drawingFrames.size(); }

};

static const int MAX_FRAMES_IN_FLIGHT = 3;
static const int SWAP_CHAIN_IMAGE_COUNT = 5;


static int g_nextFrameToDrawIndex = 0;


vulcpp::VulkanInstance createVulkanInstance() {

	vulcpp::VulkanInstanceCreateInfo vulkanInstanceCreateInfo{};
	vulkanInstanceCreateInfo.addLayer("VK_LAYER_KHRONOS_validation");

	vulkanInstanceCreateInfo.addExtension("VK_EXT_debug_utils");
	vulkanInstanceCreateInfo.addExtension("VK_KHR_surface");
	vulkanInstanceCreateInfo.addExtension("VK_KHR_win32_surface");

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = vulcpp::DebugUtilsMessenger::getCreateInfo();
	vulkanInstanceCreateInfo.pNext = &debugCreateInfo;

	return vulcpp::VulkanInstance(vulkanInstanceCreateInfo);
}


void recordCommandBuffer(
	VkCommandBuffer		commandBuffer,
	uint32_t			swapChainImageIndex,
	vulcpp::SwapChainImageViewsFrameBuffers& swapChainImageViewsFrameBuffers,
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
		vulcpp::PhysicalDevice& physicalDevice,
		vulcpp::Device device,
		int count
	) {
		VkDeviceSize bufferSize = sizeof(ModelViewProjTransform);

		m_uniformBufferMemory.resize(count);

		for (int i = 0; i < count; i++) {
			m_uniformBufferMemory[i] = std::move(
				BufferAndDeviceMemoryMapped::create(
					physicalDevice,
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

	vulcpp::VulkanInstance	g_vulkanInstance;
	vulcpp::Surface	g_surfaceOriginal;
	vulcpp::PhysicalDevice	g_physicalDevice;
	vulcpp::Device	g_deviceOriginal;

	VkQueue				g_vkGraphicsQueue = nullptr;
	VkQueue				g_vkPresentationQueue = nullptr;

	vulcpp::RenderPass g_renderPassOriginal;

	uint32_t						g_swapChainMinImageCount = SWAP_CHAIN_IMAGE_COUNT;
	vulcpp::SwapChainImageViewsFrameBuffers	g_swapChainImageViewsFrameBuffers;

	vulcpp::ShaderModule g_vertShaderModule;
	vulcpp::ShaderModule g_fragShaderModule;

	vulcpp::DescriptorPool			g_descriptorPoolOriginal;
	vulcpp::DescriptorSetLayout		g_descriptorSetLayoutOriginal;

	vulcpp::PipelineLayout	g_pipelineLayout;
	vulcpp::GraphicsPipeline	g_graphicsPipeline;

	vulcpp::Buffer			g_vertexBuffer;
	vulcpp::DeviceMemory		g_vertexDeviceMemory;

	vulcpp::CommandPool		g_commandPoolOriginal;

};
Globals g_globals;
AllDrawingFrames g_allDrawingFrames(MAX_FRAMES_IN_FLIGHT);



vulcpp::DescriptorSetLayout createUboDescriptorSetLayout(VkDevice vkDevice) {
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

	return vulcpp::DescriptorSetLayout(layoutInfo, vkDevice);
}


vulcpp::DescriptorPool createDescriptorPool(VkDevice vkDevice) {

	VkDescriptorPoolSize poolSize{};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	VkDescriptorPoolCreateInfo poolCreateInfo{};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.poolSizeCount = 1;
	poolCreateInfo.pPoolSizes = &poolSize;
	poolCreateInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	return vulcpp::DescriptorPool(poolCreateInfo, vkDevice);
}


void createDescriptorSets(
	vulcpp::Device device,
	VkDescriptorPool		descriptorPool,
	vulcpp::DescriptorSetLayout descriptorSetLayout,
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


void VulkanStuff(HINSTANCE hInstance, HWND hWnd, Globals& globals) {

	vulcpp::VulkanInstance vulkanInstance = createVulkanInstance();
	vulkanInstance.createDebugMessenger();

	vulcpp::PhysicalDevice physicalDevice = vulkanInstance.getPhysicalDevice(0);
	//auto allQueueFamilyProperties = physicalDevice.getAllQueueFamilyProperties();

	vulcpp::DeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.addExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	vulcpp::DeviceQueueCreateInfo deviceQueueCreateInfo{};
	deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
	deviceCreateInfo.queueCreateInfoCount = 1;

	VkPhysicalDeviceFeatures vkPhysicalDeviceFeatures{};
	deviceCreateInfo.pEnabledFeatures = &vkPhysicalDeviceFeatures;

	vulcpp::Device deviceOriginal(deviceCreateInfo, physicalDevice);
	vulcpp::Device deviceClone = deviceOriginal;

	const int queueFamilyIndex = 0;
	const int queueIndex = 0;
	VkQueue vkGraphicsQueue = deviceClone.getDeviceQueue(queueFamilyIndex, queueIndex);
	VkQueue vkPresentationQueue = vkGraphicsQueue;


	VkWin32SurfaceCreateInfoKHR vkWin32SurfaceCreateInfo{};
	vkWin32SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	vkWin32SurfaceCreateInfo.hwnd = hWnd;
	vkWin32SurfaceCreateInfo.hinstance = hInstance;
	vulcpp::Surface surfaceOriginal(vkWin32SurfaceCreateInfo, vulkanInstance);

	VkSurfaceCapabilitiesKHR vkSurfaceCapabilities = surfaceOriginal.getSurfaceCapabilities(physicalDevice);
	std::vector<VkSurfaceFormatKHR> surfaceFormats = surfaceOriginal.getSurfaceFormats(physicalDevice);
	std::vector<VkPresentModeKHR> presentModes = surfaceOriginal.getSurfacePresentModes(physicalDevice);

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

	vulcpp::RenderPass renderPassOriginal(vkRenderPassCreateInfo, deviceClone);


	//vulcpp::RenderPass renderPassOriginal =
	//	vulcpp::RenderPass::createRenderPass(swapChainImageFormat, deviceClone);


	vulcpp::SwapChainImageViewsFrameBuffers::setDevice(deviceClone);
	vulcpp::SwapChainImageViewsFrameBuffers swapChainImageViewsFrameBuffers;
	swapChainImageViewsFrameBuffers.setPhysicalDevice(physicalDevice);
	swapChainImageViewsFrameBuffers.setSurface(surfaceOriginal);
	swapChainImageViewsFrameBuffers.setRenderPass(renderPassOriginal);
	swapChainImageViewsFrameBuffers.setFormat(swapChainImageFormat);
	swapChainImageViewsFrameBuffers.setMinImageCount(SWAP_CHAIN_IMAGE_COUNT);

	swapChainImageViewsFrameBuffers.recreateSwapChainImageViewsFrameBuffers();

	VkBufferCreateInfo vkBufferCreateInfo{};
	vkBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vkBufferCreateInfo.size = sizeof(vertices[0]) * vertices.size();
	vkBufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	vkBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	vulcpp::Buffer vertexBufferOriginal(vkBufferCreateInfo, deviceClone);

	VkMemoryRequirements vkMemoryRequirements;
	vkGetBufferMemoryRequirements(deviceClone, vertexBufferOriginal, &vkMemoryRequirements);

	VkMemoryAllocateInfo vkMemoryAllocateInfo{};
	vkMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	vkMemoryAllocateInfo.allocationSize = vkMemoryRequirements.size;
	vkMemoryAllocateInfo.memoryTypeIndex = findMemoryTypeIndex(
		physicalDevice,
		vkMemoryRequirements.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	vulcpp::DeviceMemory vertexDeviceMemory(vkMemoryAllocateInfo, deviceClone);
	vkBindBufferMemory(deviceClone, vertexBufferOriginal, vertexDeviceMemory, 0);
	void* data;
	vkMapMemory(deviceClone, vertexDeviceMemory, 0, vkBufferCreateInfo.size, 0, &data);
	memcpy(data, vertices.data(), (size_t)vkBufferCreateInfo.size);
	vkUnmapMemory(deviceClone, vertexDeviceMemory);


	{
		UniformBuffersMemory  uniformBuffersMemory;
		uniformBuffersMemory.createUniformBuffers(physicalDevice, deviceClone, MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			g_allDrawingFrames.drawingFrameAt(i).moveUniformMemoryBuffer(std::move(uniformBuffersMemory.m_uniformBufferMemory[i]));
		}
	}


	vulcpp::ShaderModule	vertShaderModule =
		vulcpp::ShaderModule::createShaderModuleFromFile("C:/Shaders/VulkanTriangle/vert2.spv", deviceClone);
	vulcpp::ShaderModule	fragShaderModule =
		vulcpp::ShaderModule::createShaderModuleFromFile("C:/Shaders/VulkanTriangle/frag.spv", deviceClone);



	vulcpp::DescriptorPool descriptorPoolOriginal = createDescriptorPool(deviceClone);

	vulcpp::DescriptorSetLayout descriptorSetLayoutOriginal = createUboDescriptorSetLayout(deviceClone);
	createDescriptorSets(deviceClone, descriptorPoolOriginal, descriptorSetLayoutOriginal, g_allDrawingFrames);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ descriptorSetLayoutOriginal };

	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = (uint32_t)descriptorSetLayouts.size();
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	vulcpp::PipelineLayout pipelineLayout(pipelineLayoutInfo, deviceClone);


	//	VkPipeline			vkGraphicsPipeline = nullptr;
	VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo{};
	vertShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageCreateInfo.module = vertShaderModule;
	vertShaderStageCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo{};
	fragShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageCreateInfo.module = fragShaderModule;
	fragShaderStageCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStageCreateInfos[]
		= { vertShaderStageCreateInfo, fragShaderStageCreateInfo };

	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
	//		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)vkSurfaceCapabilities.currentExtent.width;
	viewport.height = (float)vkSurfaceCapabilities.currentExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = vkSurfaceCapabilities.currentExtent;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	VkGraphicsPipelineCreateInfo vkGraphicsPipelineCreateInfo{};
	vkGraphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	vkGraphicsPipelineCreateInfo.stageCount = 2;
	vkGraphicsPipelineCreateInfo.pStages = shaderStageCreateInfos;

	vkGraphicsPipelineCreateInfo.pVertexInputState = &vertexInputInfo;
	vkGraphicsPipelineCreateInfo.pInputAssemblyState = &inputAssembly;
	vkGraphicsPipelineCreateInfo.pViewportState = &viewportState;
	vkGraphicsPipelineCreateInfo.pRasterizationState = &rasterizer;
	vkGraphicsPipelineCreateInfo.pMultisampleState = &multisampling;
	vkGraphicsPipelineCreateInfo.pDepthStencilState = nullptr; // Optional
	vkGraphicsPipelineCreateInfo.pColorBlendState = &colorBlending;
	vkGraphicsPipelineCreateInfo.pDynamicState = &dynamicState;
	vkGraphicsPipelineCreateInfo.layout = pipelineLayout;
	vkGraphicsPipelineCreateInfo.renderPass = swapChainImageViewsFrameBuffers.getRenderPass();
	vkGraphicsPipelineCreateInfo.subpass = 0;
	vkGraphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	vkGraphicsPipelineCreateInfo.basePipelineIndex = -1; // Optional


	vulcpp::GraphicsPipeline graphicsPipeline(vkGraphicsPipelineCreateInfo, deviceClone);

	VkCommandPoolCreateInfo commandPoolCreateInfo{};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
	vulcpp::CommandPool commandPoolOriginal(commandPoolCreateInfo, deviceClone);

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
			vulcpp::Semaphore(semaphoreInfo, deviceClone));
		g_allDrawingFrames.drawingFrameAt(i).moveRenderFinishedSemaphore(
			vulcpp::Semaphore(semaphoreInfo, deviceClone));
		g_allDrawingFrames.drawingFrameAt(i).moveInFlightFence(
			vulcpp::Fence(fenceInfo, deviceClone));
	}

#undef globals

	globals.g_vulkanInstance = std::move(vulkanInstance);
	globals.g_physicalDevice = std::move(physicalDevice);
	globals.g_deviceOriginal = std::move(deviceOriginal);

	globals.g_hInstance = hInstance;
	globals.g_hWnd = hWnd;
	globals.g_surfaceOriginal = std::move(surfaceOriginal);


	globals.g_vertexBuffer = std::move(vertexBufferOriginal);
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
	VkDevice vkDevice = currentDrawingFrame.getDevice();
	vulcpp::Fence inFlightFence = currentDrawingFrame.m_inFlightFence;
	vulcpp::Semaphore imageAvailableSemaphore = currentDrawingFrame.m_imageAvailableSemaphore;
	vulcpp::Semaphore renderFinishedSemaphore = currentDrawingFrame.m_renderFinishedSemaphore;
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
		// TODO: Add any drawing code that uses hdc here...
		EndPaint(hWnd, &ps);
	}
	break;

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
	WNDCLASSEXW wcex;

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
					catch (vulcpp::ShutdownException&)
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
