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

#include <conio.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "VulkanSynchronization2Only.h"


struct Point {
	glm::vec3	pos;
	glm::vec3	color;
	glm::vec2 texCoord;

	static const int s_bindingIndex = 0;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};

		bindingDescription.binding = s_bindingIndex;
		bindingDescription.stride = sizeof(Point);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

		attributeDescriptions[0].binding = s_bindingIndex;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Point, pos);

		attributeDescriptions[1].binding = s_bindingIndex;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Point, color);

		attributeDescriptions[2].binding = s_bindingIndex;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Point, texCoord);

		return attributeDescriptions;
	}
};




//	IMPORTANT!!! READ THIS!!!
//	The terminology is a bit more accurate but differs from Vulkan.
//	Since we are using indexed drawing, what Vulkan calls a vertex is
//	actually a point.  We call it a point.
//	The point on a shape where two edges meet is a vertex.  When using
//	indexed drawing, the vertex is an index into an array of points.
//	Vulkan calls this a vertex index.  We call it a vertex.
//	Using this terminology helps greatly when defining more complex
//	shapes.  E.g., a square will have five points but have many
//	more vertices since we have to draw it as multiple triangles.
//	(The point at the center of the square has to be specified so that
//	we can draw triangles around the center.)
class PointVertexBuffer {

	std::vector<Point>		m_points;
	std::vector<int16_t>	m_vertices;

public:

	friend class Shape;


	void dump() {
		int vertexIndex = 0;
		for (int16_t pointIndex : m_vertices) {
			std::cout << "vertexIndex: " << vertexIndex << " pointIndex: " << pointIndex << "\n";
			vertexIndex++;
		}
	}


	PointVertexBuffer() {}

	PointVertexBuffer(
		const std::vector<Point>& points,
		const std::vector<int16_t>& vertices)
		: m_points(points)
		, m_vertices(vertices) {}


	int64_t	pointsSizeof() {
		return sizeof(Point) * m_points.size();
	}

	int64_t verticesSizeof() {
		return sizeof(int16_t) * m_vertices.size();
	}

	int16_t pointCount() {
		return static_cast<int16_t>(m_points.size());
	}

	int16_t vertexCount() {
		return static_cast<int16_t>(m_vertices.size());
	}

	Point* pointData() {
		return m_points.data();
	}

	int16_t* vertexData() {
		return m_vertices.data();
	}

	Shape add(const Shape& shape);

	void addOffset(double x, double y, double z, int16_t pointStartIndex, int16_t pointCount) {
		for (int i = pointStartIndex; i < pointStartIndex + pointCount; i++) {
			Point& p = m_points.at(i);
			p.pos.x += x;
			p.pos.y += y;
			p.pos.z += z;
		}
	}

	void scale(double x, double y, double z, int16_t pointStartIndex, int16_t pointCount) {
		for (int i = pointStartIndex; i < pointStartIndex + pointCount; i++) {
			Point& p = m_points.at(i);
			p.pos.x *= x;
			p.pos.y *= y;
			p.pos.z *= z;
		}
	}

};


class Shape {

	PointVertexBuffer& m_pointVertexBuffer;
	int16_t	m_pointStartIndex;
	int16_t m_pointCount;
	int16_t	m_vertexStartIndex;
	int16_t	m_vertexCount;


public:

	friend class PointVertexBuffer;

	Shape(const Shape&) = delete;
	Shape& operator=(const Shape&) = delete;
	Shape(Shape&&) noexcept = delete;
	Shape& operator=(Shape&&) noexcept = delete;

	Shape(
		PointVertexBuffer& pointVertexBuffer,
		int16_t	pointStartIndex,
		int16_t	pointCount,
		int16_t	vertexStartIndex,
		int16_t vertexCount)
		: m_pointVertexBuffer(pointVertexBuffer)
		, m_pointStartIndex(pointStartIndex)
		, m_pointCount(pointCount)
		, m_vertexStartIndex(vertexStartIndex)
		, m_vertexCount(vertexCount) {
	}

	Shape(PointVertexBuffer& pointVertexBuffer)
		: m_pointVertexBuffer(pointVertexBuffer)
		, m_pointStartIndex(0)
		, m_pointCount(pointVertexBuffer.pointCount())
		, m_vertexStartIndex(0)
		, m_vertexCount(pointVertexBuffer.vertexCount()) {
	}

	void addOffset(double x, double y, double z) {
		m_pointVertexBuffer.addOffset(x, y, z, m_pointStartIndex, m_pointCount);
	}

	void scale(double x, double y, double z) {
		m_pointVertexBuffer.scale(x, y, z, m_pointStartIndex, m_pointCount);
	}


};

Shape PointVertexBuffer::add(const Shape& shape) {
	const int16_t	thisPointStartIndex = static_cast<int16_t>(m_points.size());		//	remember where we started in this buffer
	const int16_t	thisVertexStartIndex = static_cast<int16_t>(m_vertices.size());	//	remember where started in this buffer

	const int16_t	shapePointStartIndex = shape.m_pointStartIndex;
	const int16_t	shapePointCount = shape.m_pointCount;
	const int16_t	shapeVertexStartIndex = shape.m_vertexStartIndex;
	const int16_t	shapeVertexCount = shape.m_vertexCount;


	//	The point information doesn't change when moved to a new
	//	buffer, so just copy it over.
	for (int16_t i = 0; i < shapePointCount; i++) {
		m_points.push_back(shape.m_pointVertexBuffer.m_points.at(shapePointStartIndex + i));
	}

	for (int16_t i = 0; i < shapeVertexCount; i++) {
		//	Go through the shape point indices, subtract the (start) shape index
		//	to normalize to a zero base, then add this vertex (start) index to rebase.
		int16_t oldPointIndex = shape.m_pointVertexBuffer.m_vertices.at(shapeVertexStartIndex + i);
		int16_t oldNormalIndex = oldPointIndex - shapePointStartIndex;
		int16_t newIndex = oldNormalIndex + thisPointStartIndex;
		m_vertices.push_back(newIndex);
	}
	//this->dump();
	//std::cout << "\n";
	return Shape(*this, thisPointStartIndex, shapePointCount, thisVertexStartIndex, shapeVertexCount);
}


const std::vector<Point> g_rightTrianglePoints{
	{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
	{{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}} };

const std::vector<int16_t> g_rightTriangleVertices{
	0, 1, 2 };

PointVertexBuffer g_rightTrianglePointVertexBuffer(g_rightTrianglePoints, g_rightTriangleVertices);
Shape g_theRightTriangle(g_rightTrianglePointVertexBuffer);


const std::vector<Point> g_squarePoints{
	{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
	{{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
	{{1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
	{{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
	{{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.5f, 0.5f}}
};

const std::vector<int16_t> g_squareVertices{
	0, 1, 4,
	1, 2, 4,
	2, 3, 4,
	3, 0, 4
};

PointVertexBuffer g_squarePointVertexBuffer(g_squarePoints, g_squareVertices);
Shape g_theSquare(g_squarePointVertexBuffer);


PointVertexBuffer g_pointVertexBuffer;




const wchar_t* szTitle = TEXT("Vulkan Again");
const wchar_t* szWindowClass = TEXT("Vulkan Again Class");

const int windowPositionX = 500;
const int windowPositionY = 500;
const int windowWidth = 800;
const int windowHeight = 600;


struct ModelViewProjTransform {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};



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


	void createCommandBuffer(vkcpp::CommandPool commandPool) {
		m_commandBuffer = std::move(vkcpp::CommandBuffer(commandPool));
	}

	void createUniformBuffer(vkcpp::Device device) {
		m_uniformBufferMemory = std::move(
			vkcpp::Buffer_DeviceMemory(
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				sizeof(ModelViewProjTransform),
				device
			));
	}

	void createSyncObjects(vkcpp::Device device) {
		m_imageAvailableSemaphore = std::move(vkcpp::Semaphore(device));
		m_renderFinishedSemaphore = std::move(vkcpp::Semaphore(device));
		m_inFlightFence = std::move(vkcpp::Fence(VK_FENCE_CREATE_SIGNALED_BIT, device));
	}

	void moveDescriptorSet(vkcpp::DescriptorSet&& descriptorSet) {
		m_descriptorSet = std::move(descriptorSet);
	}

	VkDevice getVkDevice() const {
		//	Everything should be using the same device,
		//	so just pick any device.
		return m_inFlightFence.getVkDevice();
	}
};


class AllDrawingFrames {

private:
	std::vector<DrawingFrame> m_drawingFrames;

public:

	AllDrawingFrames() {}
	~AllDrawingFrames() = default;

	AllDrawingFrames(const AllDrawingFrames&) = delete;
	AllDrawingFrames& operator=(const AllDrawingFrames&) = delete;

	AllDrawingFrames(int frameCount) : m_drawingFrames(frameCount) {}

	DrawingFrame& drawingFrameAt(int index) {
		return m_drawingFrames.at(index);
	}

	int	frameCount() { return static_cast<int>(m_drawingFrames.size()); }

	void createUniformBuffers(vkcpp::Device device) {
		for (DrawingFrame& drawingFrame : m_drawingFrames) {
			drawingFrame.createUniformBuffer(device);
		}
	}

	void createCommandBuffers(vkcpp::CommandPool commandPool) {
		for (DrawingFrame& drawingFrame : m_drawingFrames) {
			drawingFrame.createCommandBuffer(commandPool);
		}
	}

	void createSyncObjects(vkcpp::Device device) {
		for (DrawingFrame& drawingFrame : m_drawingFrames) {
			drawingFrame.createSyncObjects(device);
		}
	}

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

	vkcpp::Queue				g_graphicsQueue;
	vkcpp::Queue				g_presentationQueue;

	vkcpp::RenderPass g_renderPassOriginal;

	uint32_t						g_swapchainMinImageCount = SWAP_CHAIN_IMAGE_COUNT;
	vkcpp::Swapchain_ImageViews_FrameBuffers	g_swapchainImageViewsFrameBuffers;

	vkcpp::ShaderModule g_vertShaderModule;
	vkcpp::ShaderModule g_textureFragShaderModule;
	vkcpp::ShaderModule g_identityFragShaderModule;

	vkcpp::DescriptorPool			g_descriptorPoolOriginal;
	vkcpp::DescriptorSetLayout		g_descriptorSetLayoutOriginal;

	vkcpp::PipelineLayout	g_pipelineLayout;
	vkcpp::GraphicsPipeline	g_graphicsPipeline;

	vkcpp::Buffer_DeviceMemory	g_pointBufferAndDeviceMemory;
	vkcpp::Buffer_DeviceMemory	g_vertexBufferAndDeviceMemory;

	vkcpp::CommandPool		g_commandPoolOriginal;

	vkcpp::Image_Memory_View	g_texture;
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
	PipelineInputAssemblyStateCreateInfo(VkPrimitiveTopology vkPrimitiveTopology)
		: VkPipelineInputAssemblyStateCreateInfo{} {
		sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		topology = vkPrimitiveTopology;
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

	std::vector<VkDynamicState> m_dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo m_dynamicState{};

	VkViewport m_viewport{};
	VkRect2D m_scissor{};

	VkPipelineViewportStateCreateInfo m_viewportState{};


	PipelineInputAssemblyStateCreateInfo m_inputAssemblyStateCreateInfo{ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST };
	std::vector<VkVertexInputBindingDescription>	m_vertexInputBindingDescriptions;
	VkPipelineVertexInputStateCreateInfo m_vkPipelineVertexInputStateCreateInfo{};

	std::vector<VkPipelineShaderStageCreateInfo> m_shaderStageCreateInfos;


	PipelineRasterizationStateCreateInfo m_pipelineRasterizationStateCreateInfo{};

	PipelineMultisampleStateCreateInfo m_pipelineMultisampleStateCreateInfo{};

	PipelineColorBlendAttachmentState m_pipelineColorBlendAttachmentState{};

	PipelineColorBlendStateCreateInfo m_pipelineColorBlendStateCreateInfo{};

	VkPipelineDepthStencilStateCreateInfo m_depthStencil{};

	VkGraphicsPipelineCreateInfo m_vkGraphicsPipelineCreateInfo{};

	vkcpp::PipelineLayout	m_pipelineLayout;
	vkcpp::RenderPass		m_renderPass;

public:


	void addVertexInputBindingDescription(const VkVertexInputBindingDescription& vkVertexInputBindingDescription) {
		m_vertexInputBindingDescriptions.push_back(vkVertexInputBindingDescription);
	}

	std::vector<VkVertexInputAttributeDescription> m_vertexInputAttributeDescriptions;
	void addVertexInputAttributeDescription(const VkVertexInputAttributeDescription& vertexInputAttributeDescriptions) {
		m_vertexInputAttributeDescriptions.push_back(vertexInputAttributeDescriptions);
	}

	void addShaderModule(
		vkcpp::ShaderModule shaderModule,
		VkShaderStageFlagBits	vkShaderStageFlagBits,
		const char* entryPointName
	) {
		VkPipelineShaderStageCreateInfo vkPipelineShaderStageCreateInfo{};
		vkPipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vkPipelineShaderStageCreateInfo.stage = vkShaderStageFlagBits;
		vkPipelineShaderStageCreateInfo.module = shaderModule;
		vkPipelineShaderStageCreateInfo.pName = entryPointName;
		m_shaderStageCreateInfos.push_back(vkPipelineShaderStageCreateInfo);
	}

	void setViewportExtent(VkExtent2D extent) {
		m_viewport.width = static_cast<float>(extent.width);
		m_viewport.height = static_cast<float>(extent.height);
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

		m_vkGraphicsPipelineCreateInfo.stageCount = static_cast<uint32_t>(m_shaderStageCreateInfos.size());
		m_vkGraphicsPipelineCreateInfo.pStages = m_shaderStageCreateInfos.data();


		m_vkPipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		m_vkPipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(m_vertexInputBindingDescriptions.size());
		m_vkPipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = m_vertexInputBindingDescriptions.data();
		m_vkPipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_vertexInputAttributeDescriptions.size());
		m_vkPipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = m_vertexInputAttributeDescriptions.data();

		m_vkGraphicsPipelineCreateInfo.pVertexInputState = &m_vkPipelineVertexInputStateCreateInfo;

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


		m_depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		m_depthStencil.depthTestEnable = VK_TRUE;
		m_depthStencil.depthWriteEnable = VK_TRUE;
		m_depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;	// Less depth means in front of
		m_depthStencil.depthBoundsTestEnable = VK_FALSE;
		m_depthStencil.stencilTestEnable = VK_FALSE;
		m_vkGraphicsPipelineCreateInfo.pDepthStencilState = &m_depthStencil;


		m_vkGraphicsPipelineCreateInfo.layout = m_pipelineLayout;
		m_vkGraphicsPipelineCreateInfo.renderPass = m_renderPass;
		m_vkGraphicsPipelineCreateInfo.subpass = 0;
		m_vkGraphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		m_vkGraphicsPipelineCreateInfo.basePipelineIndex = -1; // Optional

		return m_vkGraphicsPipelineCreateInfo;
	}
};



void transitionImageLayout(
	vkcpp::Image image,
	VkFormat format,
	VkImageLayout oldLayout,
	VkImageLayout newLayout,
	vkcpp::CommandPool commandPool,
	vkcpp::Queue graphicsQueue) {

	vkcpp::CommandBuffer commandBuffer(commandPool);
	commandBuffer.beginOneTimeSubmit();

	vkcpp::ImageMemoryBarrier2 imageMemoryBarrier(oldLayout, newLayout, image);
	vkcpp::DependencyInfo dependencyInfo;
	dependencyInfo.addImageMemoryBarrier(imageMemoryBarrier);
	commandBuffer.pipelineBarrier2(dependencyInfo);

	commandBuffer.end();

	graphicsQueue.submit(commandBuffer);
	graphicsQueue.waitIdle();

}


void copyBufferToImage(
	vkcpp::Buffer buffer,
	vkcpp::Image image,
	int width,
	int height,
	vkcpp::CommandPool commandPool,
	vkcpp::Queue graphicsQueue) {

	vkcpp::CommandBuffer commandBuffer(commandPool);
	commandBuffer.beginOneTimeSubmit();

	commandBuffer.copyBufferToImage(buffer, image, width, height);
	commandBuffer.end();

	graphicsQueue.submit(commandBuffer);
	graphicsQueue.waitIdle();
}



vkcpp::Image_Memory_View createTextureFromFile(
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

	const VkDeviceSize imageSize = texWidth * texHeight * 4;

	//	Make a device (gpu) staging buffer and copy the pixels into it.
	vkcpp::Buffer_DeviceMemory stagingBuffer_DeviceMemoryMapped(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		imageSize,
		pixels,
		device);

	stbi_image_free(pixels);	//	Don't need these anymore.  Pixels are on gpu now.

	//	Make our target image and memory.
	VkExtent2D texExtent;
	texExtent.width = texWidth;
	texExtent.height = texHeight;
	vkcpp::Image_Memory textureImage_DeviceMemory(
		texExtent,
		targetFormat,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
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
		VK_IMAGE_VIEW_TYPE_2D,
		targetFormat,
		VK_IMAGE_ASPECT_COLOR_BIT);
	textureImageViewCreateInfo.image = textureImage_DeviceMemory.m_image;
	vkcpp::ImageView textureImageView(textureImageViewCreateInfo, device);

	//	Package everything up for use as a shader.
	return vkcpp::Image_Memory_View(
		std::move(textureImage_DeviceMemory.m_image),
		std::move(textureImage_DeviceMemory.m_deviceMemory),
		std::move(textureImageView));
}


class RenderPassCreateInfo : public VkRenderPassCreateInfo {

public:

	VkFormat								m_vkFormat;

	std::vector<VkAttachmentDescription>	m_attachmentDescriptions;

	VkAttachmentDescription m_colorAttachment{};
	VkAttachmentReference m_colorAttachmentRef{};

	VkAttachmentDescription m_depthAttachment{};
	VkAttachmentReference m_depthAttachmentRef{};

	VkSubpassDescription m_vkSubpassDescription{};
	VkSubpassDependency m_vkSubpassDependency{};

	RenderPassCreateInfo(VkFormat vkFormat)
		: VkRenderPassCreateInfo{}
		, m_vkFormat(vkFormat) {
	}


	RenderPassCreateInfo& assemble() {

		//	TODO: split into configure operations, a simple default, and an assemble/build
		m_colorAttachment.format = m_vkFormat;
		m_colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		m_colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		m_colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		m_colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		m_colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		m_colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		m_colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		m_colorAttachmentRef.attachment = static_cast<uint32_t>(m_attachmentDescriptions.size());	//	size before push is index
		m_colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		m_attachmentDescriptions.push_back(m_colorAttachment);

		m_depthAttachment.format = VK_FORMAT_D32_SFLOAT;
		m_depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		m_depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		m_depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		m_depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		m_depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		m_depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		m_depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		m_depthAttachmentRef.attachment = static_cast<uint32_t>(m_attachmentDescriptions.size());
		m_depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		m_attachmentDescriptions.push_back(m_depthAttachment);

		m_vkSubpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		m_vkSubpassDescription.colorAttachmentCount = 1;
		m_vkSubpassDescription.pColorAttachments = &m_colorAttachmentRef;
		m_vkSubpassDescription.pDepthStencilAttachment = &m_depthAttachmentRef;

		m_vkSubpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		m_vkSubpassDependency.dstSubpass = 0;
		m_vkSubpassDependency.srcStageMask
			= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		m_vkSubpassDependency.srcAccessMask = 0;
		m_vkSubpassDependency.dstStageMask
			= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		m_vkSubpassDependency.dstAccessMask
			= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;


		sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		attachmentCount = static_cast<uint32_t>(m_attachmentDescriptions.size());
		pAttachments = m_attachmentDescriptions.data();
		subpassCount = 1;
		pSubpasses = &m_vkSubpassDescription;
		dependencyCount = 1;
		pDependencies = &m_vkSubpassDependency;

		return *this;

	}


};


vkcpp::RenderPass createRenderPass(
	VkFormat swapchainImageFormat,
	vkcpp::Device device
) {
	RenderPassCreateInfo RenderPassCreateInfo(swapchainImageFormat);
	return vkcpp::RenderPass(RenderPassCreateInfo.assemble(), device);
}


void VulkanStuff(HINSTANCE hInstance, HWND hWnd, Globals& globals) {

	vkcpp::VulkanInstance vulkanInstance = createVulkanInstance();
	vulkanInstance.createDebugMessenger();

	vkcpp::PhysicalDevice physicalDevice = vulkanInstance.getPhysicalDevice(0);
	VkPhysicalDeviceFeatures2 vkPhysicalDeviceFeatures2 = physicalDevice.getPhysicalDeviceFeatures2();

	//auto allQueueFamilyProperties = physicalDevice.getAllQueueFamilyProperties();

	vkcpp::DeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.addExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	vkcpp::DeviceQueueCreateInfo deviceQueueCreateInfo{};
	deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
	deviceCreateInfo.queueCreateInfoCount = 1;

	deviceCreateInfo.pNext = &vkPhysicalDeviceFeatures2;
	deviceCreateInfo.pEnabledFeatures = nullptr;

	vkcpp::Device deviceOriginal(deviceCreateInfo, physicalDevice);

	const int queueFamilyIndex = 0;
	const int queueIndex = 0;
	vkcpp::Queue graphicsQueue = deviceOriginal.getDeviceQueue(queueFamilyIndex, queueIndex);
	vkcpp::Queue presentationQueue = graphicsQueue;


	VkWin32SurfaceCreateInfoKHR vkWin32SurfaceCreateInfo{};
	vkWin32SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	vkWin32SurfaceCreateInfo.hwnd = hWnd;
	vkWin32SurfaceCreateInfo.hinstance = hInstance;
	vkcpp::Surface surfaceOriginal(vkWin32SurfaceCreateInfo, vulkanInstance, physicalDevice);

	VkSurfaceCapabilitiesKHR vkSurfaceCapabilities = surfaceOriginal.getSurfaceCapabilities();
	//std::vector<VkSurfaceFormatKHR> surfaceFormats = surfaceOriginal.getSurfaceFormats();
	//std::vector<VkPresentModeKHR> presentModes = surfaceOriginal.getSurfacePresentModes();

	const VkFormat swapchainImageFormat = VK_FORMAT_B8G8R8A8_SRGB;

	vkcpp::RenderPass renderPassOriginal(createRenderPass(swapchainImageFormat, deviceOriginal));

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

	//	Pay attention to the terminology change.
	vkcpp::Buffer_DeviceMemory pointBufferAndDeviceMemory(
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		g_pointVertexBuffer.pointsSizeof(),
		g_pointVertexBuffer.pointData(),
		deviceOriginal);

	//	Pay attention to the terminology change.
	vkcpp::Buffer_DeviceMemory vertexBufferAndDeviceMemory(
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		g_pointVertexBuffer.verticesSizeof(),
		g_pointVertexBuffer.vertexData(),
		deviceOriginal);

	g_allDrawingFrames.createUniformBuffers(deviceOriginal);

	vkcpp::ShaderModule	vertShaderModule =
		vkcpp::ShaderModule::createShaderModuleFromFile("C:/Shaders/VulkanTriangle/vert4.spv", deviceOriginal);
	vkcpp::ShaderModule	textureFragShaderModule =
		vkcpp::ShaderModule::createShaderModuleFromFile("C:/Shaders/VulkanTriangle/textureFrag.spv", deviceOriginal);
	vkcpp::ShaderModule	identityFragShaderModule =
		vkcpp::ShaderModule::createShaderModuleFromFile("C:/Shaders/VulkanTriangle/identityFrag.spv", deviceOriginal);


	VkCommandPoolCreateInfo commandPoolCreateInfo{};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
	vkcpp::CommandPool commandPoolOriginal(commandPoolCreateInfo, deviceOriginal);


	vkcpp::Image_Memory_View texture =
		createTextureFromFile(
			"c:/vulkan/texture.jpg", deviceOriginal, commandPoolOriginal, graphicsQueue);


	vkcpp::SamplerCreateInfo textureSamplerCreateInfo;
	vkcpp::Sampler textureSampler(textureSamplerCreateInfo, deviceOriginal);


	vkcpp::DescriptorSetLayout descriptorSetLayoutOriginal = createDrawingFrameDescriptorSetLayout(deviceOriginal);
	vkcpp::DescriptorPool descriptorPoolOriginal = createDescriptorPool(deviceOriginal);

	createDescriptorSets(
		deviceOriginal,
		descriptorPoolOriginal,
		descriptorSetLayoutOriginal,
		texture.m_imageView, textureSampler,
		g_allDrawingFrames);

	VkPipelineLayoutCreateInfo vkPipelineLayoutCreateInfo{};
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ descriptorSetLayoutOriginal };

	vkPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	vkPipelineLayoutCreateInfo.setLayoutCount = (uint32_t)descriptorSetLayouts.size();
	vkPipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
	vkPipelineLayoutCreateInfo.pushConstantRangeCount = 0; // Optional
	vkPipelineLayoutCreateInfo.pPushConstantRanges = nullptr; // Optional

	vkcpp::PipelineLayout pipelineLayout(vkPipelineLayoutCreateInfo, deviceOriginal);

	GraphicsPipelineConfig graphicsPipelineConfig;
	graphicsPipelineConfig.addVertexInputBindingDescription(Point::getBindingDescription());
	for (const VkVertexInputAttributeDescription& vkVertexInputAttributeDescription : Point::getAttributeDescriptions()) {
		graphicsPipelineConfig.addVertexInputAttributeDescription(vkVertexInputAttributeDescription);
	}
	graphicsPipelineConfig.setViewportExtent(vkSurfaceCapabilities.currentExtent);
	graphicsPipelineConfig.setPipelineLayout(pipelineLayout);
	graphicsPipelineConfig.setRenderPass(renderPassOriginal);
	graphicsPipelineConfig.addShaderModule(vertShaderModule, VK_SHADER_STAGE_VERTEX_BIT, "main");
	graphicsPipelineConfig.addShaderModule(textureFragShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT, "main");
	//graphicsPipelineConfig.addShaderModule(identityFragShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT, "main");

	vkcpp::GraphicsPipeline graphicsPipeline(graphicsPipelineConfig.assemble(), deviceOriginal);

	g_allDrawingFrames.createCommandBuffers(commandPoolOriginal);
	g_allDrawingFrames.createSyncObjects(deviceOriginal);


#undef globals

	globals.g_vulkanInstance = std::move(vulkanInstance);
	globals.g_physicalDevice = std::move(physicalDevice);
	globals.g_deviceOriginal = std::move(deviceOriginal);

	globals.g_hInstance = hInstance;
	globals.g_hWnd = hWnd;
	globals.g_surfaceOriginal = std::move(surfaceOriginal);


	globals.g_pointBufferAndDeviceMemory = std::move(pointBufferAndDeviceMemory);
	globals.g_vertexBufferAndDeviceMemory = std::move(vertexBufferAndDeviceMemory);

	globals.g_descriptorSetLayoutOriginal = std::move(descriptorSetLayoutOriginal);
	globals.g_descriptorPoolOriginal = std::move(descriptorPoolOriginal);

	globals.g_pipelineLayout = std::move(pipelineLayout);
	globals.g_graphicsPipeline = std::move(graphicsPipeline);


	globals.g_commandPoolOriginal = std::move(commandPoolOriginal);

	globals.g_graphicsQueue = graphicsQueue;
	globals.g_presentationQueue = presentationQueue;

	globals.g_renderPassOriginal = std::move(renderPassOriginal);
	globals.g_swapchainImageViewsFrameBuffers = std::move(swapchainImageViewsFrameBuffers);

	globals.g_vertShaderModule = std::move(vertShaderModule);
	globals.g_textureFragShaderModule = std::move(textureFragShaderModule);
	globals.g_identityFragShaderModule = std::move(identityFragShaderModule);

	globals.g_texture = std::move(texture);
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
	modelViewProjTransform.model = glm::rotate(
		glm::mat4(1.0f),
		time * glm::radians(22.5f),
		glm::vec3(0.0f, 0.0f, 1.0f));
	modelViewProjTransform.view = glm::lookAt(
		glm::vec3(2.0f, 2.0f, 2.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f));
	modelViewProjTransform.proj = glm::perspective(
		glm::radians(45.0f),
		(float)swapchainImageExtent.width / (float)swapchainImageExtent.height,
		0.1f,
		10.0f);
	modelViewProjTransform.proj[1][1] *= -1;

	memcpy(
		uniformBufferMemory.m_mappedMemory,
		&modelViewProjTransform,
		sizeof(modelViewProjTransform));
}



void recordCommandBuffer(
	vkcpp::CommandBuffer		commandBuffer,
	vkcpp::Swapchain_ImageViews_FrameBuffers& swapchainImageViewsFrameBuffers,
	uint32_t			swapchainImageIndex,
	vkcpp::Buffer			pointBuffer,
	vkcpp::Buffer			vertexBuffer,
	VkDescriptorSet		vkDescriptorSet,
	vkcpp::PipelineLayout	pipelineLayout,
	vkcpp::GraphicsPipeline			graphicsPipeline
) {
	const VkExtent2D swapchainImageExtent = swapchainImageViewsFrameBuffers.getImageExtent();

	commandBuffer.reset();
	commandBuffer.begin();

	VkRenderPassBeginInfo vkRenderPassBeginInfo{};
	vkRenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	vkRenderPassBeginInfo.renderPass = swapchainImageViewsFrameBuffers.getRenderPass();
	vkRenderPassBeginInfo.framebuffer = swapchainImageViewsFrameBuffers.getFrameBuffer(swapchainImageIndex);
	vkRenderPassBeginInfo.renderArea.offset = { 0, 0 };
	vkRenderPassBeginInfo.renderArea.extent = swapchainImageExtent;

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f } };
	clearValues[1].depthStencil = { 1.0f, 0 };
	vkRenderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	vkRenderPassBeginInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &vkRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

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

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		0, 1,
		&vkDescriptorSet,
		0, nullptr);

	VkBuffer vkPointBuffer = pointBuffer;
	VkBuffer pointBuffers[] = { vkPointBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, pointBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, vertexBuffer, 0, VK_INDEX_TYPE_UINT16);
	vkCmdDrawIndexed(commandBuffer, g_pointVertexBuffer.vertexCount(), 1, 0, 0, 0);

	vkCmdEndRenderPass(commandBuffer);

	commandBuffer.end();

}

int64_t	g_drawFrameCalls;
int64_t g_drawFrameDraws;

void drawFrame(Globals& globals)
{
	g_drawFrameCalls++;

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
	VkResult vkResult = vkAcquireNextImageKHR(
		vkDevice,
		globals.g_swapchainImageViewsFrameBuffers.vkSwapchain(),
		0,
		currentDrawingFrame.m_imageAvailableSemaphore,
		VK_NULL_HANDLE,
		&swapchainImageIndex);
	if (vkResult == VK_NOT_READY) {
		return;
	}

	updateUniformBuffer(
		currentDrawingFrame.m_uniformBufferMemory,
		globals.g_swapchainImageViewsFrameBuffers.getImageExtent()
	);

	recordCommandBuffer(
		commandBuffer,
		globals.g_swapchainImageViewsFrameBuffers,
		swapchainImageIndex,
		globals.g_pointBufferAndDeviceMemory.m_buffer,
		globals.g_vertexBufferAndDeviceMemory.m_buffer,
		currentDrawingFrame.m_descriptorSet,
		globals.g_pipelineLayout,
		globals.g_graphicsPipeline);

	vkcpp::SubmitInfo submitInfo;
	submitInfo.addWaitSemaphore(
		currentDrawingFrame.m_imageAvailableSemaphore,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	);
	submitInfo.addCommandBuffer(commandBuffer);
	submitInfo.addSignalSemaphore(currentDrawingFrame.m_renderFinishedSemaphore);


	//	Show that this drawing frame is now in use ..
	//	Fence will be signaled when graphics queue is finished.
	currentDrawingFrame.m_inFlightFence.reset();

	g_drawFrameDraws++;

	globals.g_graphicsQueue.submit(submitInfo, currentDrawingFrame.m_inFlightFence);

	vkcpp::PresentInfo presentInfo;
	presentInfo.addWaitSemaphore(currentDrawingFrame.m_renderFinishedSemaphore);
	presentInfo.addSwapchain(
		globals.g_swapchainImageViewsFrameBuffers.vkSwapchain(),
		swapchainImageIndex
	);
	vkResult = globals.g_presentationQueue.present(presentInfo);
	if (vkResult == VK_SUBOPTIMAL_KHR) {
		globals.g_swapchainImageViewsFrameBuffers.stale();
	}

	g_nextFrameToDrawIndex = (g_nextFrameToDrawIndex + 1) % MAX_FRAMES_IN_FLIGHT;

}

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
		g_globals.g_swapchainImageViewsFrameBuffers.stale();
		//		g_windowPosChanged = true;
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

auto g_statStart = std::chrono::high_resolution_clock::now();

void showStats() {

	auto statEnd = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> statDiff = statEnd - g_statStart;
	if (statDiff >= std::chrono::seconds{ 10 }) {
		g_statStart = std::chrono::high_resolution_clock::now();
		std::cout << "stats:\n";
		std::cout << "  time: " << statDiff << "  " << statDiff.count() << "\n";
		std::cout << "  drawFrameCalls: " << g_drawFrameCalls << "\n";
		std::cout << "  drawFrameDraws: " << g_drawFrameDraws << "\n";

		g_drawFrameCalls = 0;
		g_drawFrameDraws = 0;

	}

}

void MessageLoop(Globals& globals) {

	MSG msg;

	bool	done = false;

	while (!done) {
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT) {
				done = true;
				break;
			}

			if (msg.message == WM_KEYDOWN) {
				if (msg.wParam == 'D') {
					std::cout << "Dump Vulkan Info\n";
				}
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		try {
			drawFrame(globals);
		}
		catch (vkcpp::ShutdownException&) {
			done = true;
		}
		showStats();
	}
	return;
}


int main()
{
	std::cout << "Hello World!\n";

	VkRect2D	vkRect2D;
	vkRect2D.extent.width = 640;
	vkRect2D.extent.height = 480;
	vkcpp::Rect2D& rect2D = vkcpp::wrapToRef<VkRect2D, vkcpp::Rect2D>(vkRect2D);
	std::cout << rect2D.extent.width << "\n";
	std::cout << rect2D.extent.height << "\n";
	rect2D.extent.width = 512;
	rect2D.extent.height = 513;
	std::cout << vkRect2D.extent.width << "\n";
	std::cout << vkRect2D.extent.height << "\n";


	HINSTANCE hInstance = GetModuleHandle(NULL);
	RegisterMyWindowClass(hInstance);
	HWND hWnd = CreateFirstWindow(hInstance);

	Shape shape1 = g_pointVertexBuffer.add(g_theSquare);
	//	shape1.scale(1.5, 1.5, 0.0);

	Shape shape2 = g_pointVertexBuffer.add(g_theRightTriangle);
	shape2.addOffset(0.0, 0.0, 0.5);

	//Shape shape3 = g_pointVertexBuffer.add(g_theRightTriangle);
	//shape3.addOffset(0.5, 0.5, 0.2);

	//Shape shape4 = g_pointVertexBuffer.add(g_theRightTriangle);
	//shape4.addOffset(0.75, 0.75, 0.3);

	//Shape shape5 = g_pointVertexBuffer.add(g_theRightTriangle);
	//shape5.addOffset(1.0, 1.0, 0.4);

	//Shape shape6 = g_pointVertexBuffer.add(g_theRightTriangle);
	//shape6.addOffset(1.0, 1.0, 0.5);

	//Shape shape7 = g_pointVertexBuffer.add(g_theRightTriangle);
	//shape7.addOffset(1.0, 1.0, 0.6);

	//Shape shape8 = g_pointVertexBuffer.add(g_theRightTriangle);
	//shape8.addOffset(1.0, 1.0, 0.7);

	//Shape shape9 = g_pointVertexBuffer.add(g_theRightTriangle);
	//shape9.addOffset(1.0, 1.0, 0.8);

	//Shape shape10 = g_pointVertexBuffer.add(g_theRightTriangle);
	//shape10.addOffset(1.0, 1.0, 0.9);

	VulkanStuff(hInstance, hWnd, g_globals);

	MessageLoop(g_globals);

	//	Wait for device to be idle before exiting and cleaning up globals.
	g_globals.g_deviceOriginal.waitIdle();


}
