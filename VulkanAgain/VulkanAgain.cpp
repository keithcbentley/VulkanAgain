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

class MagicValues {


public:

	static const int MAX_FRAMES_IN_FLIGHT = 3;
	static const int SWAP_CHAIN_IMAGE_COUNT = 5;

	static const uint32_t	GRAPHICS_QUEUE_FAMILY_INDEX = 0;
	static const uint32_t	GRAPHICS_QUEUE_INDEX = 0;

	static const uint32_t	PRESENTATION_QUEUE_FAMILY_INDEX = 0;
	static const uint32_t	PRESENTATION_QUEUE_INDEX = 0;

	static const int VERTEX_BINDING_INDEX = 0;

	static const int	UBO_DESCRIPTOR_BINDING_INDEX = 0;
	static const int	TEXTURE_DESCRIPTOR_BINDING_INDEX = 1;



};


struct Point {
	glm::vec3	m_pos;
	glm::vec3	m_color;
	glm::vec2	m_textureCoord;


	static vkcpp::VertexBinding getVertexBinding(int bindingIndex) {
		vkcpp::VertexBinding vertexBinding;
		vertexBinding.m_vkVertexInputBindingDescription.binding = bindingIndex;
		vertexBinding.m_vkVertexInputBindingDescription.stride = sizeof(Point);
		vertexBinding.m_vkVertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		//	A little bit fragile since location depends on addition order,
		//	but we need to keep locations explicit for vertex shader.
		vertexBinding.addVertexInputAttributeDescription(
			bindingIndex, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Point, m_pos));

		vertexBinding.addVertexInputAttributeDescription(
			bindingIndex, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Point, m_color));

		vertexBinding.addVertexInputAttributeDescription(
			bindingIndex, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(Point, m_textureCoord));

		return vertexBinding;
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

	//	TODO: is using an int16_t instead of int32_t really
	//	saving us anything.  Does vulkan care about the
	//	size of a vertex index?  Index size is used
	//	in draw indexed command.
	//	TODO: swith to int32_t or templatize.
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
			p.m_pos.x += x;
			p.m_pos.y += y;
			p.m_pos.z += z;
		}
	}

	void scale(double x, double y, double z, int16_t pointStartIndex, int16_t pointCount) {
		for (int i = pointStartIndex; i < pointStartIndex + pointCount; i++) {
			Point& p = m_points.at(i);
			p.m_pos.x *= x;
			p.m_pos.y *= y;
			p.m_pos.z *= z;
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

	//	TODO: maybe allow moving if we need to save shapes somewhere.
	//	Copying might be a bit problematic since it would allow
	//	modifying the "shape" from two different places.
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
	return Shape(*this, thisPointStartIndex, shapePointCount, thisVertexStartIndex, shapeVertexCount);
}


const std::vector<Point> g_rightTrianglePoints{
	{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
	{{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
	{{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}} };

const std::vector<int16_t> g_rightTriangleVertices{
	0, 1, 2 };

PointVertexBuffer g_rightTrianglePointVertexBuffer(g_rightTrianglePoints, g_rightTriangleVertices);
Shape g_theRightTriangle(g_rightTrianglePointVertexBuffer);


const std::vector<Point> g_squareCenterPoints{
	{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
	{{1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
	{{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	{{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.5f, 0.5f}}
};

const std::vector<int16_t> g_squareCenterVertices{
	0, 1, 4,
	1, 2, 4,
	2, 3, 4,
	3, 0, 4
};

PointVertexBuffer g_squareCenterPointVertexBuffer(g_squareCenterPoints, g_squareCenterVertices);
Shape g_theSquareCenter(g_squareCenterPointVertexBuffer);


const std::vector<Point> g_cubeCenterPoints{
	{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},		//	0
	{{1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
	{{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	{{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.5f, 0.5f}},

	{{1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},		//	5
	{{1.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
	{{1.0f, 1.0f, -1.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
	{{1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	{{1.0f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.5f, 0.5f}},

	{{1.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},	//	10
	{{0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
	{{0.0f, 1.0f, -1.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
	{{1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	{{0.5f, 0.5f, -1.0f}, {1.0f, 1.0f, 1.0f}, {0.5f, 0.5f}},

	{{0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},	//	15
	{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
	{{0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	{{0.0f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.5f, 0.5f}},

	{{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},		//	20
	{{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
	{{1.0f, 1.0f, -1.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.0f, 1.0f, -1.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	{{0.5f, 1.0f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.5f, 0.5f}},

	{{0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},	//	25
	{{1.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
	{{1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
	{{0.5f, 0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.5f, 0.5f}},



};

const std::vector<int16_t> g_cubeCenterVertices{
	0, 1, 4,
	1, 2, 4,
	2, 3, 4,
	3, 0, 4,

	5, 6, 9,
	6, 7, 9,
	7, 8, 9,
	8, 5, 9,

	10, 11, 14,
	11, 12, 14,
	12, 13, 14,
	13, 10, 14,

	15, 16, 19,
	16, 17, 19,
	17, 18, 19,
	18, 15, 19,

	20, 21, 24,
	21, 22, 24,
	22, 23, 24,
	23, 20, 24,

	25, 26, 29,
	26, 27, 29,
	27, 28, 29,
	28, 25, 29


};


PointVertexBuffer g_cubeCenterPointVertexBuffer(g_cubeCenterPoints, g_cubeCenterVertices);
Shape g_theCubeCenter(g_cubeCenterPointVertexBuffer);


PointVertexBuffer g_pointVertexBuffer1;
PointVertexBuffer g_pointVertexBuffer2;




const wchar_t* szTitle = TEXT("Vulkan Again");
const wchar_t* szWindowClass = TEXT("Vulkan Again Class");

const int windowPositionX = 500;
const int windowPositionY = 500;
const int windowWidth = 800;
const int windowHeight = 600;



//	TODO: turn into "camera".
//	TODO: better uniform buffer object handling.
struct ModelViewProjTransform {
	alignas(16) glm::mat4 modelTransform;
	alignas(16) glm::mat4 viewTransform;
	alignas(16) glm::mat4 projTransform;
};



class DrawingFrame {


	void createCommandBuffer(vkcpp::CommandPool commandPool) {
		m_commandBuffer = std::move(vkcpp::CommandBuffer(commandPool));
	}

	void createUniformBuffer() {
		m_uniformBufferMemory = std::move(
			vkcpp::Buffer_DeviceMemory(
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				sizeof(ModelViewProjTransform),
				MagicValues::GRAPHICS_QUEUE_FAMILY_INDEX,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				m_device
			));
	}

	void createSyncObjects() {
		m_swapchainImageAvailableSemaphore = std::move(vkcpp::Semaphore(m_device));
		m_renderFinishedSemaphore = std::move(vkcpp::Semaphore(m_device));
		m_inFlightFence = std::move(vkcpp::Fence(m_device, VKCPP_FENCE_CREATE_OPENED));
	}

	void createDescriptorSet(
		vkcpp::DescriptorPool		descriptorPool,
		vkcpp::DescriptorSetLayout descriptorSetLayout,
		vkcpp::ImageView textureImageView,
		vkcpp::Sampler textureSampler
	) {
		vkcpp::DescriptorSet descriptorSet(descriptorSetLayout, descriptorPool);
		vkcpp::DescriptorSetUpdater descriptorSetUpdater(descriptorSet);
		descriptorSetUpdater.addWriteDescriptor(
			MagicValues::UBO_DESCRIPTOR_BINDING_INDEX,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			m_uniformBufferMemory.m_buffer,
			sizeof(ModelViewProjTransform));
		descriptorSetUpdater.addWriteDescriptor(
			MagicValues::TEXTURE_DESCRIPTOR_BINDING_INDEX,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			textureImageView,
			textureSampler);
		descriptorSetUpdater.updateDescriptorSets();
		m_descriptorSet = std::move(descriptorSet);
	}

	vkcpp::Device m_device;	//	Just remember the device to make things easier.

public:

	vkcpp::Fence	m_inFlightFence;
	vkcpp::Semaphore m_swapchainImageAvailableSemaphore;
	vkcpp::Semaphore m_renderFinishedSemaphore;
	vkcpp::CommandBuffer	m_commandBuffer;;
	vkcpp::Buffer_DeviceMemory	m_uniformBufferMemory;
	vkcpp::DescriptorSet	m_descriptorSet;

	DrawingFrame() {};

	DrawingFrame(const DrawingFrame&) = delete;
	DrawingFrame& operator=(const DrawingFrame&) = delete;
	DrawingFrame(DrawingFrame&&) = delete;
	DrawingFrame& operator=(DrawingFrame&&) = delete;


	void create(
		vkcpp::Device device,
		vkcpp::CommandPool commandPool,
		vkcpp::DescriptorPool descriptorPool,
		vkcpp::DescriptorSetLayout descriptorSetLayout,
		vkcpp::ImageView textureImageView,
		vkcpp::Sampler textureSampler
	) {
		m_device = device;
		createUniformBuffer();
		createCommandBuffer(commandPool);
		createSyncObjects();
		createDescriptorSet(
			descriptorPool,
			descriptorSetLayout,
			textureImageView,
			textureSampler);
	}


	vkcpp::Device getDevice() const {
		return m_device;
	}
};


class AllDrawingFrames {

private:
	std::vector<DrawingFrame> m_drawingFrames;

public:

	int m_nextFrameToDrawIndex = 0;


	AllDrawingFrames() {}
	~AllDrawingFrames() = default;

	AllDrawingFrames(const AllDrawingFrames&) = delete;
	AllDrawingFrames& operator=(const AllDrawingFrames&) = delete;

	AllDrawingFrames(int frameCount) : m_drawingFrames(frameCount) {}

	void advanceNextFrameToDraw() {
		m_nextFrameToDrawIndex = (m_nextFrameToDrawIndex + 1) % m_drawingFrames.size();;
	}

	DrawingFrame& getNextFrameToDraw() {
		return m_drawingFrames.at(m_nextFrameToDrawIndex);
	}

	int	frameCount() { return static_cast<int>(m_drawingFrames.size()); }

	void createDrawingFrames(
		vkcpp::Device device,
		vkcpp::CommandPool commandPool,
		vkcpp::DescriptorPool descriptorPool,
		vkcpp::DescriptorSetLayout descriptorSetLayout,
		vkcpp::ImageView textureImageView,
		vkcpp::Sampler textureSampler
	) {
		for (DrawingFrame& drawingFrame : m_drawingFrames) {
			drawingFrame.create(
				device,
				commandPool,
				descriptorPool,
				descriptorSetLayout,
				textureImageView,
				textureSampler
			);
		}
	}

};





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

	vkcpp::Swapchain_ImageViews_FrameBuffers	g_swapchainImageViewsFrameBuffers;

	vkcpp::ShaderModule g_vertShaderModule;
	vkcpp::ShaderModule g_textureFragShaderModule;
	vkcpp::ShaderModule g_identityFragShaderModule;

	vkcpp::DescriptorPool			g_descriptorPoolOriginal;
	vkcpp::DescriptorSetLayout		g_descriptorSetLayoutOriginal;

	vkcpp::PipelineLayout	g_pipelineLayout;
	vkcpp::GraphicsPipeline	g_graphicsPipeline0;
	vkcpp::GraphicsPipeline	g_graphicsPipeline1;

	vkcpp::Buffer_DeviceMemory	g_pointBufferAndDeviceMemory1;
	vkcpp::Buffer_DeviceMemory	g_vertexBufferAndDeviceMemory1;

	vkcpp::Buffer_DeviceMemory	g_pointBufferAndDeviceMemory2;
	vkcpp::Buffer_DeviceMemory	g_vertexBufferAndDeviceMemory2;


	vkcpp::CommandPool		g_commandPoolOriginal;

	vkcpp::Image_Memory_View	g_texture;
	vkcpp::Sampler g_textureSampler;

};

Globals g_globals;

AllDrawingFrames g_allDrawingFrames(MagicValues::MAX_FRAMES_IN_FLIGHT);


vkcpp::DescriptorSetLayout createDrawingFrameDescriptorSetLayout(VkDevice vkDevice) {

	vkcpp::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
	descriptorSetLayoutCreateInfo
		.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
		.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

	return vkcpp::DescriptorSetLayout(descriptorSetLayoutCreateInfo, vkDevice);
}


vkcpp::DescriptorPool createDescriptorPool(VkDevice vkDevice) {

	vkcpp::DescriptorPoolCreateInfo poolCreateInfo;
	poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	poolCreateInfo.addDescriptorCount(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MagicValues::MAX_FRAMES_IN_FLIGHT);
	poolCreateInfo.addDescriptorCount(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MagicValues::MAX_FRAMES_IN_FLIGHT);

	poolCreateInfo.maxSets = static_cast<uint32_t>(MagicValues::MAX_FRAMES_IN_FLIGHT);

	return vkcpp::DescriptorPool(poolCreateInfo, vkDevice);
}






void transitionImageLayout(
	vkcpp::Image image,
	VkFormat format,
	VkImageLayout oldLayout,
	VkImageLayout newLayout,
	vkcpp::CommandPool commandPool,
	vkcpp::Queue graphicsQueue
) {
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

	vkcpp::CommandBuffer commandBuffer(commandPool);
	commandBuffer.beginOneTimeSubmit();
	commandBuffer.cmdCopyBufferToImage(buffer, image, width, height);
	commandBuffer.end();

	graphicsQueue.submit2Fenced(commandBuffer);
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
		imageSize,
		MagicValues::GRAPHICS_QUEUE_FAMILY_INDEX,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		pixels,
		device);

	stbi_image_free(pixels);	//	Don't need these anymore.  Pixels are on gpu now.
	pixels = nullptr;

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


std::vector<uint32_t> g_imageData;


void makeImageFromBitmap() {

	const VkDeviceSize imageSize = g_imageData.size() * sizeof(uint32_t);

	//	Make a device (gpu) staging buffer and copy the pixels into it.
	vkcpp::Buffer_DeviceMemory stagingBuffer_DeviceMemoryMapped(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		imageSize,
		MagicValues::GRAPHICS_QUEUE_FAMILY_INDEX,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		g_imageData.data(),
		g_globals.g_deviceOriginal);


}



//	TODO: need to tie together pipelines and subpasses better.
//	Right now, it's just magic numbers from the subpass number.
vkcpp::RenderPass createRenderPass(
	VkFormat swapchainImageFormat,
	vkcpp::Device device
) {
	vkcpp::RenderPassCreateInfo renderPassCreateInfo;

	//	Note that the index of the attachment can be pulled out
	//	of the returned attachment reference to use in other
	//	attachment references.
	VkAttachmentReference colorAttachmentReference = renderPassCreateInfo.addAttachment(
		vkcpp::AttachmentDescription::simpleColorAttachmentPresentDescription(swapchainImageFormat),
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	);

	VkAttachmentReference depthAttachmentReference = renderPassCreateInfo.addAttachment(
		vkcpp::AttachmentDescription::simpleDepthAttachmentDescription(),
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	);


	renderPassCreateInfo.addSubpass()
		.addColorAttachmentReference(colorAttachmentReference)
		.setDepthStencilAttachmentReference(depthAttachmentReference);

	//	Just a handy abbreviation
	const VkPipelineStageFlags bothFragmentTests
		= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
		| VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

	//	All depth stencil attachment writes have to finish before
	//	any new depth stencil attachment reads or writes.
	renderPassCreateInfo.addSubpassDependency(VK_SUBPASS_EXTERNAL, 0)
		.addSrc(bothFragmentTests, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)
		.addDst(bothFragmentTests, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT);

	renderPassCreateInfo.addSubpassDependency(VK_SUBPASS_EXTERNAL, 0)
		.addSrc(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
		.addDst(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);


	//renderPassCreateInfo.addSubpass()
	//	.addColorAttachmentReference(colorAttachmentReference)
	//	.setDepthStencilAttachmentReference(depthAttachmentReference);

	//renderPassCreateInfo.addSubpassDependency(0, 1)
	//	.addSrc(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
	//	.addDst(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

	return vkcpp::RenderPass(renderPassCreateInfo, device);
}



void VulkanStuff(HINSTANCE hInstance, HWND hWnd, Globals& globals) {

	vkcpp::VulkanInstance vulkanInstance = createVulkanInstance();
	vulkanInstance.createDebugMessenger();

	vkcpp::PhysicalDevice physicalDevice = vulkanInstance.getPhysicalDevice(0);

	//auto allQueueFamilyProperties = physicalDevice.getAllQueueFamilyProperties();

	vkcpp::DeviceCreateInfo deviceCreateInfo;
	deviceCreateInfo.addExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	deviceCreateInfo.addDeviceQueue(MagicValues::GRAPHICS_QUEUE_FAMILY_INDEX, 1);
	deviceCreateInfo.addDeviceQueue(MagicValues::PRESENTATION_QUEUE_FAMILY_INDEX, 1);

	VkPhysicalDeviceFeatures2 vkPhysicalDeviceFeatures2 = physicalDevice.getPhysicalDeviceFeatures2();
	deviceCreateInfo.pNext = &vkPhysicalDeviceFeatures2;

	vkcpp::Device deviceOriginal(deviceCreateInfo, physicalDevice);

	vkcpp::Queue graphicsQueue = deviceOriginal.getDeviceQueue(
		MagicValues::GRAPHICS_QUEUE_FAMILY_INDEX,
		MagicValues::GRAPHICS_QUEUE_INDEX);

	vkcpp::Queue presentationQueue = deviceOriginal.getDeviceQueue(
		MagicValues::PRESENTATION_QUEUE_FAMILY_INDEX,
		MagicValues::PRESENTATION_QUEUE_INDEX);


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
	swapchainCreateInfo.minImageCount = MagicValues::SWAP_CHAIN_IMAGE_COUNT;
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

	//	TODO: combine point and vertex device memory into one object.
	//	Pay attention to the terminology change.
	vkcpp::Buffer_DeviceMemory pointBufferAndDeviceMemory1(
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		g_pointVertexBuffer1.pointsSizeof(),
		MagicValues::GRAPHICS_QUEUE_FAMILY_INDEX,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		g_pointVertexBuffer1.pointData(),
		deviceOriginal);

	//	Pay attention to the terminology change.
	vkcpp::Buffer_DeviceMemory vertexBufferAndDeviceMemory1(
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		g_pointVertexBuffer1.verticesSizeof(),
		MagicValues::GRAPHICS_QUEUE_FAMILY_INDEX,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		g_pointVertexBuffer1.vertexData(),
		deviceOriginal);

	//	Pay attention to the terminology change.
	vkcpp::Buffer_DeviceMemory pointBufferAndDeviceMemory2(
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		g_pointVertexBuffer2.pointsSizeof(),
		MagicValues::GRAPHICS_QUEUE_FAMILY_INDEX,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		g_pointVertexBuffer2.pointData(),
		deviceOriginal);

	//	Pay attention to the terminology change.
	vkcpp::Buffer_DeviceMemory vertexBufferAndDeviceMemory2(
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		g_pointVertexBuffer2.verticesSizeof(),
		MagicValues::GRAPHICS_QUEUE_FAMILY_INDEX,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		g_pointVertexBuffer2.vertexData(),
		deviceOriginal);


	vkcpp::ShaderModule	vertShaderModule =
		vkcpp::ShaderModule::createShaderModuleFromFile("C:/Shaders/VulkanTriangle/vert4.spv", deviceOriginal);
	vkcpp::ShaderModule	textureFragShaderModule =
		vkcpp::ShaderModule::createShaderModuleFromFile("C:/Shaders/VulkanTriangle/textureFrag.spv", deviceOriginal);
	vkcpp::ShaderModule	identityFragShaderModule =
		vkcpp::ShaderModule::createShaderModuleFromFile("C:/Shaders/VulkanTriangle/identityFrag.spv", deviceOriginal);


	VkCommandPoolCreateInfo commandPoolCreateInfo{};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = MagicValues::GRAPHICS_QUEUE_FAMILY_INDEX;
	vkcpp::CommandPool commandPoolOriginal(commandPoolCreateInfo, deviceOriginal);


	vkcpp::Image_Memory_View texture =
		createTextureFromFile(
			"c:/vulkan/texture.jpg", deviceOriginal, commandPoolOriginal, graphicsQueue);


	vkcpp::SamplerCreateInfo textureSamplerCreateInfo;
	vkcpp::Sampler textureSampler(textureSamplerCreateInfo, deviceOriginal);


	vkcpp::DescriptorSetLayout descriptorSetLayoutOriginal = createDrawingFrameDescriptorSetLayout(deviceOriginal);
	vkcpp::DescriptorPool descriptorPoolOriginal = createDescriptorPool(deviceOriginal);

	vkcpp::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
	pipelineLayoutCreateInfo.addDescriptorSetLayout(descriptorSetLayoutOriginal);
	vkcpp::PipelineLayout pipelineLayout(pipelineLayoutCreateInfo, deviceOriginal);


	vkcpp::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo;
	graphicsPipelineCreateInfo.addDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
	graphicsPipelineCreateInfo.addDynamicState(VK_DYNAMIC_STATE_SCISSOR);
	graphicsPipelineCreateInfo.addDynamicState(VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE);


	graphicsPipelineCreateInfo.addVertexBinding(
		Point::getVertexBinding(MagicValues::VERTEX_BINDING_INDEX));

	//	graphicsPipelineCreateInfo.setViewportExtent(vkSurfaceCapabilities.currentExtent);
	graphicsPipelineCreateInfo.setPipelineLayout(pipelineLayout);
	graphicsPipelineCreateInfo.setRenderPass(renderPassOriginal, 0);
	graphicsPipelineCreateInfo.addShaderModule(vertShaderModule, VK_SHADER_STAGE_VERTEX_BIT, "main");
	graphicsPipelineCreateInfo.addShaderModule(textureFragShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT, "main");
	//graphicsPipelineCreateInfo.addShaderModule(identityFragShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT, "main");

	vkcpp::GraphicsPipeline graphicsPipeline0(graphicsPipelineCreateInfo, deviceOriginal);

	//graphicsPipelineCreateInfo.setRenderPass(renderPassOriginal, 1);
	//vkcpp::GraphicsPipeline graphicsPipeline1(graphicsPipelineCreateInfo, deviceOriginal);

	g_allDrawingFrames.createDrawingFrames(
		deviceOriginal,
		commandPoolOriginal,
		descriptorPoolOriginal,
		descriptorSetLayoutOriginal,
		texture.m_imageView,
		textureSampler
	);



#undef globals

	globals.g_vulkanInstance = std::move(vulkanInstance);
	globals.g_physicalDevice = std::move(physicalDevice);
	globals.g_deviceOriginal = std::move(deviceOriginal);

	globals.g_hInstance = hInstance;
	globals.g_hWnd = hWnd;
	globals.g_surfaceOriginal = std::move(surfaceOriginal);


	globals.g_pointBufferAndDeviceMemory1 = std::move(pointBufferAndDeviceMemory1);
	globals.g_vertexBufferAndDeviceMemory1 = std::move(vertexBufferAndDeviceMemory1);

	globals.g_pointBufferAndDeviceMemory2 = std::move(pointBufferAndDeviceMemory2);
	globals.g_vertexBufferAndDeviceMemory2 = std::move(vertexBufferAndDeviceMemory2);


	globals.g_descriptorSetLayoutOriginal = std::move(descriptorSetLayoutOriginal);
	globals.g_descriptorPoolOriginal = std::move(descriptorPoolOriginal);

	globals.g_pipelineLayout = std::move(pipelineLayout);
	globals.g_graphicsPipeline0 = std::move(graphicsPipeline0);
	//	globals.g_graphicsPipeline1 = std::move(graphicsPipeline1);


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


//	TODO: better uniform buffer handling.
void updateUniformBuffer(
	vkcpp::Buffer_DeviceMemory& uniformBufferMemory,
	const VkExtent2D				swapchainImageExtent
) {
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();


	//	TODO: turn into real "camera".
	ModelViewProjTransform modelViewProjTransform{};
	modelViewProjTransform.modelTransform =
		glm::rotate(
			glm::mat4(1.0f),
			time * glm::radians(10.0f),
			glm::vec3(0.0f, 1.0f, 0.0f));
	modelViewProjTransform.viewTransform = glm::lookAt(
		glm::vec3(0.0f, -2.0f, 2.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f));
	modelViewProjTransform.projTransform = glm::perspective(
		glm::radians(45.0f),
		(float)swapchainImageExtent.width / (float)swapchainImageExtent.height,
		0.1f,
		10.0f);
	modelViewProjTransform.projTransform[1][1] *= -1.0;

	//	TODO: shouldn't this just be a simple assignment?
	//	Maybe use some kind of templated version of the mapped memory?
	memcpy(
		uniformBufferMemory.m_mappedMemory,
		&modelViewProjTransform,
		sizeof(modelViewProjTransform));
}


class Renderer {

public:

	vkcpp::CommandBuffer	m_commandBuffer;

	vkcpp::Buffer			m_pointBuffer1;
	vkcpp::Buffer			m_vertexBuffer1;

	vkcpp::Buffer			m_pointBuffer2;
	vkcpp::Buffer			m_vertexBuffer2;


	VkDescriptorSet			m_vkDescriptorSet;

	//	TODO:	pipelines should remember their layouts,
	//	or vice-versa, or both.
	vkcpp::PipelineLayout		m_pipelineLayout0;
	vkcpp::GraphicsPipeline		m_graphicsPipeline0;

	vkcpp::PipelineLayout		m_pipelineLayout1;
	vkcpp::GraphicsPipeline		m_graphicsPipeline1;


	//	TODO: use a pointer instead of a reference for now, until
	//	we figure out the best structure for the pieces.  Maybe
	//	encapsulate the renderpass in this class.
	vkcpp::Swapchain_ImageViews_FrameBuffers* m_pSwapchainImageViewsFrameBuffers;
	uint32_t									m_swapchainImageIndex;


	void recordCommandBuffer(
		vkcpp::CommandBuffer		commandBuffer
	) {
		const VkExtent2D swapchainImageExtent = m_pSwapchainImageViewsFrameBuffers->getImageExtent();


		VkRenderPassBeginInfo vkRenderPassBeginInfo{};
		vkRenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		vkRenderPassBeginInfo.renderPass = m_pSwapchainImageViewsFrameBuffers->getRenderPass();
		vkRenderPassBeginInfo.framebuffer = m_pSwapchainImageViewsFrameBuffers->getFrameBuffer(m_swapchainImageIndex);
		vkRenderPassBeginInfo.renderArea.offset = { 0, 0 };
		vkRenderPassBeginInfo.renderArea.extent = swapchainImageExtent;

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f } };
		clearValues[1].depthStencil = { 1.0f, 0 };
		vkRenderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		vkRenderPassBeginInfo.pClearValues = clearValues.data();

		commandBuffer.cmdBeginRenderPass(vkRenderPassBeginInfo);

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


		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline0);
		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_pipelineLayout0,
			0, 1,
			&m_vkDescriptorSet,
			0, nullptr);

		vkCmdSetDepthTestEnable(commandBuffer, VK_TRUE);

		//	TODO: encapsulate binding of points and vertices and
		//	maybe the draw indexed call
		{
			VkBuffer vkPointBuffer = m_pointBuffer1;
			VkBuffer pointBuffers[] = { vkPointBuffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, pointBuffers, offsets);
			vkCmdBindIndexBuffer(commandBuffer, m_vertexBuffer1, 0, VK_INDEX_TYPE_UINT16);
			//	TODO: need to track vertext count along with buffer info.
			vkCmdDrawIndexed(commandBuffer, g_pointVertexBuffer1.vertexCount(), 1, 0, 0, 0);
		}

		//VkSubpassContents vkSubpassContents{};
		//vkCmdNextSubpass(commandBuffer, vkSubpassContents);
		//vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline1);
		//vkCmdBindDescriptorSets(
		//	commandBuffer,
		//	VK_PIPELINE_BIND_POINT_GRAPHICS,
		//	m_pipelineLayout1,
		//	0, 1,
		//	&m_vkDescriptorSet,
		//	0, nullptr);


		//vkCmdSetDepthTestEnable(commandBuffer, VK_FALSE);

		//{
		//	VkBuffer vkPointBuffer = m_pointBuffer2;
		//	VkBuffer pointBuffers[] = { vkPointBuffer };
		//	VkDeviceSize offsets[] = { 0 };
		//	vkCmdBindVertexBuffers(commandBuffer, 0, 1, pointBuffers, offsets);
		//	vkCmdBindIndexBuffer(commandBuffer, m_vertexBuffer2, 0, VK_INDEX_TYPE_UINT16);
		//	vkCmdDrawIndexed(commandBuffer, g_pointVertexBuffer2.vertexCount(), 1, 0, 0, 0);
		//}


		commandBuffer.cmdEndRenderPass();


	}


};

Renderer theRenderer;


int64_t	g_drawFrameCalls;
int64_t g_drawFrameDraws;


std::chrono::high_resolution_clock::time_point g_nextFrameTime = std::chrono::high_resolution_clock::now();


void drawFrame(Globals& globals)
{
	g_drawFrameCalls++;

	//	nvidia driver blocks when presenting.
	//	Time the calls ourselves instead of just blasting
	//	thousands of frames.
	//	TODO: do timing outside of call to give time back to OS?
	std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
	if (now < g_nextFrameTime) {
		return;
	}
	double frameInterval = (1.0 / 60.0);
	long long frameIntervalMicro = frameInterval * 1'000'000.0;
	g_nextFrameTime = std::chrono::high_resolution_clock::now() + std::chrono::microseconds(frameIntervalMicro);


	if (!globals.g_swapchainImageViewsFrameBuffers.canDraw()) {
		return;
	}

	DrawingFrame& currentDrawingFrame = g_allDrawingFrames.getNextFrameToDraw();
	//	Wait for this drawing frame to be free
	//	TODO: does this need a warning timer?
	currentDrawingFrame.m_inFlightFence.wait();


	vkcpp::Device device = currentDrawingFrame.getDevice();
	vkcpp::CommandBuffer commandBuffer = currentDrawingFrame.m_commandBuffer;

	commandBuffer.reset();
	commandBuffer.begin();


	uint32_t	swapchainImageIndex;
	//	Try to get the next image available index.  Don't wait though.
	//	If no image available, just return.
	//	(I think) In some cases, this might return an index before the image
	//	is actually available.  The image available semaphore will be
	//	signaled when the image itself is actually available.
	VkResult vkResult = vkAcquireNextImageKHR(
		device,
		globals.g_swapchainImageViewsFrameBuffers.vkSwapchain(),
		0,
		currentDrawingFrame.m_swapchainImageAvailableSemaphore,
		VK_NULL_HANDLE,
		&swapchainImageIndex);
	if (vkResult == VK_NOT_READY) {
		return;
	}

	updateUniformBuffer(
		currentDrawingFrame.m_uniformBufferMemory,
		globals.g_swapchainImageViewsFrameBuffers.getImageExtent()
	);

	//	TODO: this is kind of clunky
	theRenderer.m_pointBuffer1 = globals.g_pointBufferAndDeviceMemory1.m_buffer;
	theRenderer.m_vertexBuffer1 = globals.g_vertexBufferAndDeviceMemory1.m_buffer;

	theRenderer.m_pointBuffer2 = globals.g_pointBufferAndDeviceMemory2.m_buffer;
	theRenderer.m_vertexBuffer2 = globals.g_vertexBufferAndDeviceMemory2.m_buffer;

	theRenderer.m_vkDescriptorSet = currentDrawingFrame.m_descriptorSet;
	theRenderer.m_pipelineLayout0 = globals.g_pipelineLayout;
	theRenderer.m_graphicsPipeline0 = globals.g_graphicsPipeline0;

	theRenderer.m_pipelineLayout1 = globals.g_pipelineLayout;
	theRenderer.m_graphicsPipeline1 = globals.g_graphicsPipeline1;


	theRenderer.m_pSwapchainImageViewsFrameBuffers = &globals.g_swapchainImageViewsFrameBuffers;
	theRenderer.m_swapchainImageIndex = swapchainImageIndex;

	theRenderer.recordCommandBuffer(commandBuffer);



	commandBuffer.end();


	vkcpp::SubmitInfo2 submitInfo2;
	//	Command can proceed but wait for the image to
	//	actually be available before writing, i.e.,
	//	(COLOR_ATTACHMENT_OUTPUT) to the image.
	submitInfo2.addWaitSemaphore(
		currentDrawingFrame.m_swapchainImageAvailableSemaphore,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	);
	submitInfo2.addCommandBuffer(commandBuffer);
	submitInfo2.addSignalSemaphore(currentDrawingFrame.m_renderFinishedSemaphore);


	//	Show that this drawing frame is now in use ..
	//	Fence will be opened when graphics queue is finished.
	currentDrawingFrame.m_inFlightFence.close();
	g_drawFrameDraws++;

	globals.g_graphicsQueue.submit2(submitInfo2, currentDrawingFrame.m_inFlightFence);

	//	TODO: Is this where we are supposed to add an image memory barrier
	//	to avoid the present after write error?


	vkcpp::PresentInfo presentInfo;
	presentInfo.addWaitSemaphore(currentDrawingFrame.m_renderFinishedSemaphore);
	presentInfo.addSwapchain(
		globals.g_swapchainImageViewsFrameBuffers.vkSwapchain(),
		swapchainImageIndex
	);

	//	TODO: add timer to check for blocking call?
	vkResult = globals.g_presentationQueue.present(presentInfo);
	if (vkResult == VK_SUBOPTIMAL_KHR) {
		globals.g_swapchainImageViewsFrameBuffers.stale();
	}

	g_allDrawingFrames.advanceNextFrameToDraw();


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
		g_globals.g_swapchainImageViewsFrameBuffers.stale();
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


class CommandBuffer {

public:


};



WNDPROC g_oldWndProc;
HWND g_commandBufferHwnd;

LRESULT CALLBACK CommandWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	//	std::cout << "message\n";
	return (*g_oldWndProc)(hWnd, message, wParam, lParam);
}

#include <richedit.h>



HWND createCommandBufferHwnd(HINSTANCE hInstance, HWND hwndParent) {

	// Create Edit control for typing to be sent to server
	HWND hwnd = CreateWindowExW(
		0L,
		MSFTEDIT_CLASS,
		NULL,
		WS_OVERLAPPED | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE | ES_MULTILINE,
		100, 100, 200, 200,
		hwndParent,
		NULL,
		hInstance,
		NULL);

	g_commandBufferHwnd = hwnd;
	g_oldWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)CommandWndProc);

	return hwnd;
}



void snapCommandWindow() {
	HDC commandWindowDC = GetDC(g_commandBufferHwnd);

	RECT	clientRect;
	GetClientRect(g_commandBufferHwnd, &clientRect);
	const int clientWidth = clientRect.right - clientRect.left;
	const int clientHeight = clientRect.bottom - clientRect.top;

	RECT windowRect;
	GetWindowRect(g_commandBufferHwnd, &windowRect);
	const int windowWidth = windowRect.right - windowRect.left;
	const int windowHeight = windowRect.bottom - windowRect.top;


	//std::cout << clientRect.left << " " << clientRect.right << " "
	//	<< clientRect.top << " " << clientRect.bottom << "\n";

	HBITMAP bitmapHandle = (HBITMAP)GetCurrentObject(commandWindowDC, OBJ_BITMAP);
	//	std::cout << bitmapHandle << "\n";

	BITMAP	bitmap;
	GetObject(bitmapHandle, sizeof(bitmap), &bitmap);
	std::cout << "bmType: " << bitmap.bmType << "\n";
	std::cout << "bmWidth: " << bitmap.bmWidth << " bmHeight: " << bitmap.bmHeight << "\n";
	std::cout << "bmWidthBytes: " << bitmap.bmWidthBytes << "\n";
	std::cout << "bmPlanes: " << bitmap.bmPlanes << "\n";
	std::cout << "bmBitsPixel: " << bitmap.bmBitsPixel << "\n";
	std::cout << "bmBits: " << bitmap.bmBits << "\n";
	std::cout << "\n";

	struct MyBitmapInfo {
		BITMAPINFOHEADER	bmiHeader;
		uint32_t	m_buffer[32];

		BITMAPINFO* operator&() {
			return (BITMAPINFO*)this;
		}
	};

	MyBitmapInfo	bitmapInfo{};
	bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
	int returnCode = GetDIBits(
		commandWindowDC,
		bitmapHandle,
		0,
		windowHeight,
		NULL,
		&bitmapInfo,
		DIB_RGB_COLORS
	);
	std::cout << "returnCode: " << returnCode << "\n";

	std::cout << "biSizeImage: " << bitmapInfo.bmiHeader.biSizeImage << "\n";

	g_imageData.resize(bitmapInfo.bmiHeader.biSizeImage / sizeof(uint32_t));

	returnCode = GetDIBits(
		commandWindowDC,
		bitmapHandle,
		0,
		windowHeight,
		g_imageData.data(),
		&bitmapInfo,
		DIB_RGB_COLORS
	);

	std::cout << "returnCode: " << returnCode << "\n";

	ReleaseDC(g_commandBufferHwnd, commandWindowDC);

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
				if (msg.wParam == 'S') {
					snapCommandWindow();
				}
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		//		snapCommandWindow();
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

	HMODULE hModule = LoadLibraryW(TEXT("msftedit.dll"));


	std::cout.imbue(std::locale(""));

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

	HWND commandBufferHwnd = createCommandBufferHwnd(hInstance, hWnd);


	Shape shape1 = g_pointVertexBuffer1.add(g_theCubeCenter);
	//shape1.addOffset(0.0, -0.5, 0.0);
	//shape1.scale(1.5, 1.5, 0.0);

	//Shape shape2 = g_pointVertexBuffer1.add(g_theSquareCenter);
	//shape2.scale(0.5, 0.5, 0.0);
	//shape2.addOffset(1.0, 0.0, -0.5);


	Shape shape3 = g_pointVertexBuffer2.add(g_theRightTriangle);
	shape3.addOffset(0.5, 0.5, -0.2);
	shape3.scale(0.1, 0.1, 0.1);

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

int CaptureAnImage(HWND hWnd)
{

	// Retrieve the handle to a display device context for the client 
	// area of the window. 
	HDC windowDC = GetDC(hWnd);

	// Get the client area for size calculation.
	RECT windowClientRect;
	GetClientRect(hWnd, &windowClientRect);
	const int windowClientWidth = windowClientRect.right - windowClientRect.left;
	const int windowClientHeight = windowClientRect.bottom - windowClientRect.top;
	HBITMAP windowBitmapHandle = CreateCompatibleBitmap(windowDC, windowClientWidth, windowClientHeight);


	//	Now get the actual bitmap bits from the memory bitmap.
	BITMAP windowBitmap;
	GetObject(windowBitmapHandle, sizeof(BITMAP), &windowBitmap);

	BITMAPINFOHEADER   bi;

	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = windowBitmap.bmWidth;
	bi.biHeight = windowBitmap.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	DWORD bitmapSize = ((windowBitmap.bmWidth * bi.biBitCount + 31) / 32) * 4 * windowBitmap.bmHeight;

	HANDLE hDIB = GlobalAlloc(GHND, bitmapSize);
	char* lpbitmap = (char*)GlobalLock(hDIB);

	// Gets the "bits" from the bitmap, and copies them into a buffer 
	// that's pointed to by lpbitmap.
	GetDIBits(
		windowDC,
		windowBitmapHandle,
		0,
		(UINT)windowBitmap.bmHeight,
		lpbitmap,
		(BITMAPINFO*)&bi,
		DIB_RGB_COLORS);

	makeImageFromBitmap();


	// Unlock and Free the DIB from the heap.
	GlobalUnlock(hDIB);
	GlobalFree(hDIB);


	// Clean up.
done:
	//DeleteObject(hbmScreen);
	//DeleteObject(hdcMemDC);
	//ReleaseDC(NULL, hdcScreen);
	//ReleaseDC(hWnd, hdcWindow);

	return 0;
}


//class DurationTimer {
//
//public:
//
//
//	std::chrono::high_resolution_clock::time_point m_start = std::chrono::high_resolution_clock::now();
//
//	DurationTimer() {}
//
//	void finish(int limit, const char* msg) {
//		auto end = std::chrono::high_resolution_clock::now();
//		std::chrono::duration<double> diff = end - m_start;
//		if (diff >= std::chrono::milliseconds{ limit }) {
//			std::cout << msg << ": " << diff << "\n";
//		}
//
//	}
//
//};


