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
#include "ShaderImageLibrary.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <conio.h>


#include "VulkanSynchronization2Only.h"


class MagicValues {


public:

	static const int MAX_DRAWING_FRAMES_IN_FLIGHT = 3;
	static const int SWAP_CHAIN_IMAGE_COUNT = 5;

	static const uint32_t	GRAPHICS_QUEUE_FAMILY_INDEX = 0;
	static const uint32_t	GRAPHICS_QUEUE_INDEX = 0;

	static const uint32_t	PRESENTATION_QUEUE_FAMILY_INDEX = 0;
	static const uint32_t	PRESENTATION_QUEUE_INDEX = 0;

	static const int VERTEX_BINDING_INDEX = 0;

	static const int	UBO_DESCRIPTOR_BINDING_INDEX = 0;
	static const int	TEXTURE_DESCRIPTOR_BINDING_INDEX = 1;



};


class VulkanGpuAssets {

	vkcpp::VulkanInstance	m_vulkanInstance;
	vkcpp::PhysicalDevice	m_physicalDevice;


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

	vkcpp::Device createDevice(vkcpp::PhysicalDevice physicalDevice) {
		vkcpp::DeviceCreateInfo deviceCreateInfo;
		deviceCreateInfo.addExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		deviceCreateInfo.addDeviceQueue(MagicValues::GRAPHICS_QUEUE_FAMILY_INDEX, 1);
		deviceCreateInfo.addDeviceQueue(MagicValues::PRESENTATION_QUEUE_FAMILY_INDEX, 1);

		VkPhysicalDeviceFeatures2 vkPhysicalDeviceFeatures2 = physicalDevice.getPhysicalDeviceFeatures2();
		deviceCreateInfo.pNext = &vkPhysicalDeviceFeatures2;

		return vkcpp::Device(deviceCreateInfo, physicalDevice);

	}

	void createQueues() {
		m_graphicsQueue = m_device.getDeviceQueue(
			MagicValues::GRAPHICS_QUEUE_FAMILY_INDEX,
			MagicValues::GRAPHICS_QUEUE_INDEX);

		m_presentationQueue = m_device.getDeviceQueue(
			MagicValues::PRESENTATION_QUEUE_FAMILY_INDEX,
			MagicValues::PRESENTATION_QUEUE_INDEX);

	}


public:

	vkcpp::Device			m_device;

	vkcpp::Queue				m_graphicsQueue;
	vkcpp::Queue				m_presentationQueue;

	vkcpp::VulkanInstance	vulkanInstance() {
		return m_vulkanInstance;
	}

	vkcpp::PhysicalDevice	physicalDevice() {
		return m_physicalDevice;
	}


	VulkanGpuAssets() {
		m_vulkanInstance = createVulkanInstance();
		m_vulkanInstance.createDebugMessenger();

		m_physicalDevice = m_vulkanInstance.getPhysicalDevice(0);

		m_device = createDevice(m_physicalDevice);

		createQueues();

	}

};

VulkanGpuAssets	g_vulkanGpuAssets;




class ShaderName {

public:

	std::string	m_shaderName;
	std::string	m_fileName;

	ShaderName(
		std::string	shaderName,
		std::string fileName)
		: m_shaderName(shaderName)
		, m_fileName(fileName) {
	}
};

std::vector<ShaderName> g_shaderNames{
	{ "vert4", "C:/Shaders/VulkanTriangle/vert4.spv"},
	{ "textureFrag", "C:/Shaders/VulkanTriangle/textureFrag.spv"},
	{ "identityFrag", "C:/Shaders/VulkanTriangle/identityFrag.spv"} };




struct Point {
	glm::vec3	m_pos;
	glm::vec3	m_color;
	glm::vec2	m_textureCoord;


	static vkcpp::VertexBinding getVertexBinding(int bindingIndex) {
		vkcpp::VertexBinding vertexBinding(bindingIndex, sizeof(Point), VK_VERTEX_INPUT_RATE_VERTEX);

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
	//	TODO: swicth to int32_t or templatize?
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


	int64_t	pointsSizeof() const {
		return sizeof(Point) * m_points.size();
	}

	int64_t verticesSizeof() const {
		return sizeof(int16_t) * m_vertices.size();
	}

	int32_t pointCount() const {
		return m_points.size();
	}

	int32_t vertexCount() const {
		return m_vertices.size();
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

	{{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},		//	20
	{{1.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
	{{1.0f, 1.0f, -1.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	{{0.5f, 1.0f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.5f, 0.5f}},

	{{0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},	//	25
	{{1.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
	{{1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
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


PointVertexBuffer g_pointVertexBuffer0;
PointVertexBuffer g_pointVertexBuffer1;



class PointVertexDeviceBuffer {


public:

	vkcpp::Buffer_DeviceMemory m_points;
	vkcpp::Buffer_DeviceMemory m_vertices;
	uint32_t	m_vertexCount = 0;

	PointVertexDeviceBuffer() {}

	PointVertexDeviceBuffer(
		PointVertexBuffer& pointVertexBuffer,
		vkcpp::Device device) {

		//	TODO: combine point and vertex device memory into one object.
		//	Pay attention to the terminology change.
		m_points = vkcpp::Buffer_DeviceMemory(
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			pointVertexBuffer.pointsSizeof(),
			MagicValues::GRAPHICS_QUEUE_FAMILY_INDEX,
			vkcpp::MEMORY_PROPERTY_HOST_VISIBLE | vkcpp::MEMORY_PROPERTY_HOST_COHERENT,
			pointVertexBuffer.pointData(),
			device);

		//	Pay attention to the terminology change.
		m_vertices = vkcpp::Buffer_DeviceMemory(
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			pointVertexBuffer.verticesSizeof(),
			MagicValues::GRAPHICS_QUEUE_FAMILY_INDEX,
			vkcpp::MEMORY_PROPERTY_HOST_VISIBLE | vkcpp::MEMORY_PROPERTY_HOST_COHERENT,
			pointVertexBuffer.vertexData(),
			device);

		m_vertexCount = pointVertexBuffer.vertexCount();
	}

	PointVertexDeviceBuffer(const PointVertexDeviceBuffer& other)
		: m_points(other.m_points)
		, m_vertices(other.m_vertices)
		, m_vertexCount(other.m_vertexCount) {
	}

	PointVertexDeviceBuffer& operator=(const PointVertexDeviceBuffer& other) {
		if (this == &other) {
			return *this;
		}
		(*this).~PointVertexDeviceBuffer();
		new(this) PointVertexDeviceBuffer(other);
		return *this;
	}

	PointVertexDeviceBuffer(PointVertexDeviceBuffer&& other) noexcept
		: m_points(std::move(other.m_points))
		, m_vertices(std::move(other.m_vertices))
		, m_vertexCount(other.m_vertexCount) {
	}

	PointVertexDeviceBuffer& operator=(PointVertexDeviceBuffer&& other) noexcept {
		if (this == &other) {
			return *this;
		}
		(*this).~PointVertexDeviceBuffer();
		new(this) PointVertexDeviceBuffer(std::move(other));
		return *this;
	}

	uint32_t	vertexCount() {
		return m_vertexCount;
	}

	void draw(vkcpp::CommandBuffer commandBuffer) {
		//	TODO: move to command buffer methods
		VkBuffer vkPointBuffer = m_points.m_buffer;
		VkBuffer pointBuffers[] = { vkPointBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, pointBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, m_vertices.m_buffer, 0, VK_INDEX_TYPE_UINT16);
		vkCmdDrawIndexed(commandBuffer, vertexCount(), 1, 0, 0, 0);
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

	vkcpp::Surface		g_surfaceOriginal;
	vkcpp::Swapchain_FrameBuffers	g_swapchain_frameBuffers;


	vkcpp::DescriptorPool			g_descriptorPoolOriginal;
	vkcpp::DescriptorSetLayout		g_descriptorSetLayoutOriginal;

	PointVertexDeviceBuffer		g_pointVertexDeviceBuffer0;
	PointVertexDeviceBuffer		g_pointVertexDeviceBuffer1;

	vkcpp::CommandPool		g_commandPoolOriginal;

	vkcpp::Sampler g_textureSampler;

};

Globals g_globals;



const wchar_t* szTitle = TEXT("Vulkan Again");
const wchar_t* szWindowClass = TEXT("Vulkan Again Class");

const int windowPositionX = 500;
const int windowPositionY = 500;
const int windowWidth = 800;
const int windowHeight = 600;



//	TODO: turn into "camera".
//	TODO: better uniform buffer object handling.
struct ModelViewProjTransform {
	alignas(16) glm::mat4 m_modelTransform;
	alignas(16) glm::mat4 m_viewTransform;
	alignas(16) glm::mat4 m_projTransform;
};


class Camera {

public:

	ModelViewProjTransform m_modelViewProjTransform{};
	VkExtent2D				m_imageExtent = { .width = 1, .height = 1 };

	float	m_eyeX = 0.0;
	float	m_eyeY = -2.0;
	float	m_eyeZ = 2.0;

	float	m_lookCenterX = 0.0;
	float	m_lookCenterY = 0.0;
	float	m_lookCenterZ = 0.0;


	Camera() {
		m_modelViewProjTransform.m_viewTransform = glm::lookAt(
			glm::vec3(m_eyeX, m_eyeY, m_eyeZ),
			glm::vec3(m_lookCenterX, m_lookCenterY, m_lookCenterZ),
			glm::vec3(0.0f, 1.0f, 0.0f));

		m_modelViewProjTransform.m_projTransform = glm::perspective(
			glm::radians(45.0f),
			(float)m_imageExtent.width / (float)m_imageExtent.height,
			0.1f,
			10.0f);

		m_modelViewProjTransform.m_projTransform[1][1] *= -1.0;

	}

	void lookCenterDelta(float deltaX, float deltaY, float deltaZ) {
		m_lookCenterX += deltaX;
		m_lookCenterY += deltaY;
		m_lookCenterZ += deltaZ;
	}

	void eyeDelta(float deltaX, float deltaY, float deltaZ) {
		m_eyeX += deltaX;
		m_eyeY += deltaY;
		m_eyeZ += deltaZ;
	}


	void update(VkExtent2D imageExtent) {

		m_modelViewProjTransform.m_viewTransform = glm::lookAt(
			glm::vec3(m_eyeX, m_eyeY, m_eyeZ),
			glm::vec3(m_lookCenterX, m_lookCenterY, m_lookCenterZ),
			glm::vec3(0.0f, 1.0f, 0.0f));

		m_imageExtent = imageExtent;
		m_modelViewProjTransform.m_projTransform = glm::perspective(
			glm::radians(45.0f),
			(float)m_imageExtent.width / (float)m_imageExtent.height,
			0.1f,
			10.0f);

		m_modelViewProjTransform.m_projTransform[1][1] *= -1.0;

	}
};


Camera g_theCamera;


class UniformBufferMemory {

	static inline	int									s_uniformMemoryBuffersCount;
	static inline	std::vector<UniformBufferMemory>	s_uniformMemoryBufferMemorys;

	UniformBufferMemory(vkcpp::Device device) {
		m_uniformBufferMemory = std::move(
			vkcpp::Buffer_DeviceMemory(
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				sizeof(ModelViewProjTransform),
				MagicValues::GRAPHICS_QUEUE_FAMILY_INDEX,
				vkcpp::MEMORY_PROPERTY_HOST_VISIBLE | vkcpp::MEMORY_PROPERTY_HOST_COHERENT,
				device
			));
	}

public:

	vkcpp::Buffer_DeviceMemory m_uniformBufferMemory;

	static void setUniformBufferMemoryCount(int count) {
		s_uniformMemoryBuffersCount = count;
	}

	static void createUniformBufferMemorys(vkcpp::Device device) {
		for (int i = 0; i < s_uniformMemoryBuffersCount; i++) {
			s_uniformMemoryBufferMemorys.push_back(std::move(UniformBufferMemory(device)));
		}
	}

	static UniformBufferMemory& get(int index) {
		return s_uniformMemoryBufferMemorys.at(index);
	}


	//	TODO: better uniform buffer handling.
	static void updateUniformBuffer(
		int					index,
		const VkExtent2D	swapchainImageExtent
	) {
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		//	TODO: the camera should be updated as part of the
		//	global game loop and then its info pulled in.
		g_theCamera.update(swapchainImageExtent);

		//	TODO: turn into real "camera".
		ModelViewProjTransform modelViewProjTransform = g_theCamera.m_modelViewProjTransform;
		modelViewProjTransform.m_modelTransform =
			glm::rotate(
				glm::mat4(1.0f),
				time * glm::radians(10.0f),
				glm::vec3(0.0f, 1.0f, 0.0f));


		//	TODO: Maybe use some kind of templated version of the mapped memory?
		vkcpp::Buffer_DeviceMemory& buffer_deviceMemory =
			s_uniformMemoryBufferMemorys.at(index).m_uniformBufferMemory;
		*((ModelViewProjTransform*)buffer_deviceMemory.m_mappedMemory) = modelViewProjTransform;
	}

};



class DescriptorSetWithBinding {

	static inline int								s_descriptorSetCount;
	static inline std::vector<vkcpp::DescriptorSet>	s_descriptorSets;


	vkcpp::DescriptorSet m_descriptorSet;


public:

	static void setDescriptorSetCount(int descriptorSetCount) {
		s_descriptorSetCount = descriptorSetCount;
	}

	static void createDescriptorSets(
		vkcpp::DescriptorSetLayout	descriptorSetLayout,
		vkcpp::DescriptorPool		descriptorPool,
		vkcpp::ImageView			textureImageView,
		vkcpp::Sampler				textureSampler
	) {
		for (int i = 0; i < s_descriptorSetCount; i++) {
			s_descriptorSets.emplace_back(descriptorSetLayout, descriptorPool);
			vkcpp::DescriptorSet& descriptorSet = s_descriptorSets.back();

			descriptorSet.addWriteDescriptor(
				MagicValues::UBO_DESCRIPTOR_BINDING_INDEX,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				UniformBufferMemory::get(i).m_uniformBufferMemory.m_buffer,
				sizeof(ModelViewProjTransform));
			descriptorSet.addWriteDescriptor(
				MagicValues::TEXTURE_DESCRIPTOR_BINDING_INDEX,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				textureImageView,
				textureSampler);
			descriptorSet.updateDescriptors();

		}
	}

	static vkcpp::DescriptorSet& getDescriptorSet(int index) {
		return s_descriptorSets.at(index);
	}


};





class DrawingFrame {

	static inline	int							s_frameCount;
	static inline	std::vector<DrawingFrame>	s_drawingFrames;

	static inline	int	s_nextFrameToDrawIndex;

	vkcpp::Device m_device;	//	Just remember the device to make things easier.


public:

	//	TODO: should the command buffer be stored here?
	//	Easier to create if not.

	//	Frames remember their index for convenience.
	//	The frame index is used as the index into the
	//	other data arrays that hold the per frame data.
	vkcpp::Fence			m_inFlightFence;
	vkcpp::Semaphore		m_swapchainImageAvailableSemaphore;
	vkcpp::Semaphore		m_renderFinishedSemaphore;
	vkcpp::CommandBuffer	m_commandBuffer;
	int						m_index = 0;

private:


	void createCommandBuffer(vkcpp::CommandPool commandPool) {
		m_commandBuffer = std::move(vkcpp::CommandBuffer(commandPool));
	}

	void createSyncObjects() {
		m_swapchainImageAvailableSemaphore = std::move(vkcpp::Semaphore(m_device));
		m_renderFinishedSemaphore = std::move(vkcpp::Semaphore(m_device));
		m_inFlightFence = std::move(vkcpp::Fence(m_device, VKCPP_FENCE_CREATE_OPENED));
	}

public:


	static void setFrameCount(int frameCount) {
		s_frameCount = frameCount;
	}

	static void createDrawingFrames(
		vkcpp::Device& device,
		vkcpp::CommandPool& commandPool
	) {
		for (int i = 0; i < s_frameCount; i++) {
			s_drawingFrames.emplace_back(device, commandPool);
		}
	}

	static DrawingFrame& getNextFrameToDraw() {
		return s_drawingFrames.at(s_nextFrameToDrawIndex);
	}

	static void advanceNextFrameToDrawIndex() {
		s_nextFrameToDrawIndex = (s_nextFrameToDrawIndex + 1) % s_frameCount;
	}


	DrawingFrame() {};

	DrawingFrame(
		vkcpp::Device device,
		vkcpp::CommandPool commandPool
	) {
		m_device = device;
		createCommandBuffer(commandPool);
		createSyncObjects();
	}



	vkcpp::Device getDevice() const {
		return m_device;
	}



};



ShaderLibrary	g_shaderLibrary;	//	Hack to control when shader library is cleared out.
ImageLibrary	g_imageLibrary;		//	Hack to control when image library is cleared out.




std::vector<vkcpp::DescriptorSetLayoutBinding>	g_descriptorSetLayoutBindings{
	{ MagicValues::UBO_DESCRIPTOR_BINDING_INDEX, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,	vkcpp::SHADER_STAGE_VERTEX},
	{ MagicValues::TEXTURE_DESCRIPTOR_BINDING_INDEX, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,	vkcpp::SHADER_STAGE_FRAGMENT}
};



vkcpp::DescriptorPool createDescriptorPool(VkDevice vkDevice) {

	vkcpp::DescriptorPoolCreateInfo poolCreateInfo;
	poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	poolCreateInfo.addDescriptorCount(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MagicValues::MAX_DRAWING_FRAMES_IN_FLIGHT);
	poolCreateInfo.addDescriptorCount(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MagicValues::MAX_DRAWING_FRAMES_IN_FLIGHT);

	poolCreateInfo.maxSets = static_cast<uint32_t>(MagicValues::MAX_DRAWING_FRAMES_IN_FLIGHT);

	return vkcpp::DescriptorPool(poolCreateInfo, vkDevice);
}



//std::vector<uint32_t> g_imageData;
//uint32_t	g_width;
//uint32_t	g_height;
//vkcpp::Image_Memory g_image_memory;


//void makeImageFromBitmap() {
//
//	const VkFormat targetFormat = VK_FORMAT_R8G8B8A8_SRGB;
//
//	const VkDeviceSize imageSize = g_imageData.size() * sizeof(uint32_t);
//
//	//	Make a device (gpu) staging buffer and copy the pixels into it.
//	vkcpp::Buffer_DeviceMemory stagingBuffer_DeviceMemoryMapped(
//		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
//		imageSize,
//		MagicValues::GRAPHICS_QUEUE_FAMILY_INDEX,
//		vkcpp::MEMORY_PROPERTY_HOST_VISIBLE | vkcpp::MEMORY_PROPERTY_HOST_COHERENT,
//		g_imageData.data(),
//		g_globals.g_deviceOriginal);
//
//
//	//	Make our target image and memory.  Bits are copied later.
//	vkcpp::Extent2D extent(g_width, g_height);
//	vkcpp::Image_Memory image_memory(
//		extent,
//		targetFormat,
//		VK_IMAGE_USAGE_TRANSFER_DST_BIT,
//		vkcpp::MEMORY_PROPERTY_DEVICE_LOCAL,
//		g_globals.g_deviceOriginal);
//
//	//	Change image layout to be best target for transfer into.
//	//	TODO: should this be packaged up into one command buffer method?
//	{
//		vkcpp::CommandBuffer commandBuffer(g_globals.g_commandPoolOriginal);
//		commandBuffer.beginOneTimeSubmit();
//		vkcpp::ImageMemoryBarrier2 imageMemoryBarrier(
//			VK_IMAGE_LAYOUT_UNDEFINED,
//			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//			image_memory.m_image);
//		vkcpp::DependencyInfo dependencyInfo;
//		dependencyInfo.addImageMemoryBarrier(imageMemoryBarrier);
//		commandBuffer.cmdPipelineBarrier2(dependencyInfo);
//		commandBuffer.end();
//		g_globals.g_graphicsQueue.submit2Fenced(commandBuffer);
//	}
//
//	{
//		//	Copy bitmap bits from staging buffer into image memory.
//		//	TODO: should images remember their width and height?
//		vkcpp::CommandBuffer commandBuffer(g_globals.g_commandPoolOriginal);
//		commandBuffer.beginOneTimeSubmit();
//		commandBuffer.cmdCopyBufferToImage(
//			stagingBuffer_DeviceMemoryMapped.m_buffer,
//			image_memory.m_image,
//			g_width, g_height);
//		commandBuffer.end();
//		g_globals.g_graphicsQueue.submit2Fenced(commandBuffer);
//	}
//
//	{
//		//	Change image layout to whatever is appropriate for use.
//		vkcpp::CommandBuffer commandBuffer(g_globals.g_commandPoolOriginal);
//		commandBuffer.beginOneTimeSubmit();
//		vkcpp::ImageMemoryBarrier2 imageMemoryBarrier(
//			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//			VK_IMAGE_LAYOUT_GENERAL,
//			image_memory.m_image);
//		vkcpp::DependencyInfo dependencyInfo;
//		dependencyInfo.addImageMemoryBarrier(imageMemoryBarrier);
//		commandBuffer.cmdPipelineBarrier2(dependencyInfo);
//		commandBuffer.end();
//		g_globals.g_graphicsQueue.submit2Fenced(commandBuffer);
//	}
//
//	g_image_memory = std::move(image_memory);
//
//
//}



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



	//	Just a handy abbreviation
	const VkPipelineStageFlags bothFragmentTests
		= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
		| VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;


	renderPassCreateInfo.addSubpass()
		.addColorAttachmentReference(colorAttachmentReference)
		.setDepthStencilAttachmentReference(depthAttachmentReference);


	//	All depth stencil attachment writes have to finish before
	//	any new depth stencil attachment reads or writes.
	renderPassCreateInfo.addSubpassDependency(VK_SUBPASS_EXTERNAL, 0)
		.addSrc(bothFragmentTests, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)
		.addDst(bothFragmentTests, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT);

	renderPassCreateInfo.addSubpassDependency(VK_SUBPASS_EXTERNAL, 0)
		.addSrc(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
		.addDst(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);


	renderPassCreateInfo.addSubpass()
		.addColorAttachmentReference(colorAttachmentReference)
		.setDepthStencilAttachmentReference(depthAttachmentReference);

	renderPassCreateInfo.addSubpassDependency(0, 1)
		.addSrc(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
		.addDst(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

	return vkcpp::RenderPass(renderPassCreateInfo, device);
}


class Renderer {

public:

	vkcpp::RenderPass	m_renderPass;

	//	TODO:	pipelines should remember their layouts,
	//	or vice-versa, or both.
	vkcpp::PipelineLayout		m_pipelineLayout;

	vkcpp::PipelineLayout		m_pipelineLayout0;
	vkcpp::GraphicsPipeline		m_graphicsPipeline0;

	vkcpp::PipelineLayout		m_pipelineLayout1;
	vkcpp::GraphicsPipeline		m_graphicsPipeline1;


	//	TODO: where to put these?
	PointVertexDeviceBuffer	m_pointVertexDeviceBuffer0;
	PointVertexDeviceBuffer	m_pointVertexDeviceBuffer1;


	//	TODO: either the drawing frame or frame buffer should know the image extent
	void recordCommandBuffer(
		DrawingFrame& drawingFrame,
		const vkcpp::Framebuffer& framebufferArg,
		const VkExtent2D			imageExtent
	) {
		VkRenderPassBeginInfo vkRenderPassBeginInfo{};
		vkRenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		vkRenderPassBeginInfo.renderPass = m_renderPass;
		vkRenderPassBeginInfo.framebuffer = framebufferArg;
		vkRenderPassBeginInfo.renderArea.offset = { 0, 0 };
		vkRenderPassBeginInfo.renderArea.extent = imageExtent;

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f } };
		clearValues[1].depthStencil = { 1.0f, 0 };
		vkRenderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		vkRenderPassBeginInfo.pClearValues = clearValues.data();

		const int drawingFrameIndex = drawingFrame.m_index;

		UniformBufferMemory::updateUniformBuffer(drawingFrameIndex, imageExtent);

		vkcpp::CommandBuffer commandBuffer = drawingFrame.m_commandBuffer;
		commandBuffer.reset();
		commandBuffer.begin();

		commandBuffer.cmdBeginRenderPass(vkRenderPassBeginInfo);

		commandBuffer.cmdSetViewport(imageExtent);
		commandBuffer.cmdSetScissor(imageExtent);

		commandBuffer.cmdBindPipeline(m_graphicsPipeline0);
		commandBuffer.cmdBindDescriptorSet(m_pipelineLayout0,
			DescriptorSetWithBinding::getDescriptorSet(drawingFrameIndex));

		vkCmdSetDepthTestEnable(commandBuffer, VK_TRUE);
		m_pointVertexDeviceBuffer0.draw(commandBuffer);

		VkSubpassContents vkSubpassContents{};
		vkCmdNextSubpass(commandBuffer, vkSubpassContents);
		commandBuffer.cmdBindPipeline(m_graphicsPipeline1);
		commandBuffer.cmdBindDescriptorSet(m_pipelineLayout1,
			DescriptorSetWithBinding::getDescriptorSet(drawingFrameIndex));


		vkCmdSetDepthTestEnable(commandBuffer, VK_FALSE);
		m_pointVertexDeviceBuffer1.draw(commandBuffer);

		commandBuffer.cmdEndRenderPass();

		commandBuffer.end();

	}


};

Renderer theRenderer;


void VulkanStuff(HINSTANCE hInstance, HWND hWnd, Globals& globals) {

	DrawingFrame::setFrameCount(MagicValues::MAX_DRAWING_FRAMES_IN_FLIGHT);
	UniformBufferMemory::setUniformBufferMemoryCount(MagicValues::MAX_DRAWING_FRAMES_IN_FLIGHT);
	DescriptorSetWithBinding::setDescriptorSetCount(MagicValues::MAX_DRAWING_FRAMES_IN_FLIGHT);

	VkWin32SurfaceCreateInfoKHR vkWin32SurfaceCreateInfo{};
	vkWin32SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	vkWin32SurfaceCreateInfo.hwnd = hWnd;
	vkWin32SurfaceCreateInfo.hinstance = hInstance;
	vkcpp::Surface surfaceOriginal(
		vkWin32SurfaceCreateInfo,
		g_vulkanGpuAssets.vulkanInstance(),
		g_vulkanGpuAssets.physicalDevice()
	);

	VkSurfaceCapabilitiesKHR vkSurfaceCapabilities = surfaceOriginal.getSurfaceCapabilities();
	//std::vector<VkSurfaceFormatKHR> surfaceFormats = surfaceOriginal.getSurfaceFormats();
	//std::vector<VkPresentModeKHR> presentModes = surfaceOriginal.getSurfacePresentModes();

	const VkFormat swapchainImageFormat = VK_FORMAT_B8G8R8A8_SRGB;
	const VkColorSpaceKHR swapchainImageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	const VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

	vkcpp::RenderPass renderPass(createRenderPass(swapchainImageFormat, g_vulkanGpuAssets.m_device));

	vkcpp::Swapchain_FrameBuffers::setDevice(g_vulkanGpuAssets.m_device);

	vkcpp::SwapchainCreateInfo swapchainCreateInfo(
		surfaceOriginal,
		MagicValues::SWAP_CHAIN_IMAGE_COUNT,
		swapchainImageFormat,
		swapchainImageColorSpace,
		swapchainPresentMode
	);

	//	The swapchainCreateInfo does not hold the smart Surface object so
	//	we need to pass it in separately.
	vkcpp::Swapchain_FrameBuffers swapchain_frameBuffers(swapchainCreateInfo, surfaceOriginal);
	swapchain_frameBuffers.setRenderPass(renderPass);

	UniformBufferMemory::createUniformBufferMemorys(g_vulkanGpuAssets.m_device);

	PointVertexDeviceBuffer	pointVertexDeviceBuffer0(g_pointVertexBuffer0, g_vulkanGpuAssets.m_device);
	PointVertexDeviceBuffer	pointVertexDeviceBuffer1(g_pointVertexBuffer1, g_vulkanGpuAssets.m_device);


	for (const ShaderName& shaderName : g_shaderNames) {
		ShaderLibrary::createShaderModuleFromFile(
			shaderName.m_shaderName,
			shaderName.m_fileName,
			g_vulkanGpuAssets.m_device);
	}


	VkCommandPoolCreateInfo commandPoolCreateInfo{};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = MagicValues::GRAPHICS_QUEUE_FAMILY_INDEX;
	vkcpp::CommandPool commandPoolOriginal(commandPoolCreateInfo, g_vulkanGpuAssets.m_device);


	ImageLibrary::createImageMemoryViewFromFile(
		"statueImage", "c:/vulkan/statue.jpg",
		g_vulkanGpuAssets.m_device, commandPoolOriginal, g_vulkanGpuAssets.m_graphicsQueue);

	ImageLibrary::createImageMemoryViewFromFile(
		"spaceImage", "c:/vulkan/space.jpg",
		g_vulkanGpuAssets.m_device, commandPoolOriginal, g_vulkanGpuAssets.m_graphicsQueue);


	vkcpp::SamplerCreateInfo textureSamplerCreateInfo;
	vkcpp::Sampler textureSampler(textureSamplerCreateInfo, g_vulkanGpuAssets.m_device);

	vkcpp::DescriptorSetLayout descriptorSetLayoutOriginal =
		vkcpp::DescriptorSetLayout::create(
			g_descriptorSetLayoutBindings,
			g_vulkanGpuAssets.m_device);
	vkcpp::DescriptorPool descriptorPoolOriginal = createDescriptorPool(g_vulkanGpuAssets.m_device);

	vkcpp::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
	pipelineLayoutCreateInfo.addDescriptorSetLayout(descriptorSetLayoutOriginal);
	vkcpp::PipelineLayout pipelineLayout(pipelineLayoutCreateInfo, g_vulkanGpuAssets.m_device);

	vkcpp::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo;
	graphicsPipelineCreateInfo.addDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
	graphicsPipelineCreateInfo.addDynamicState(VK_DYNAMIC_STATE_SCISSOR);
	graphicsPipelineCreateInfo.addDynamicState(VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE);


	graphicsPipelineCreateInfo.addVertexBinding(
		Point::getVertexBinding(MagicValues::VERTEX_BINDING_INDEX));

	//	TODO: move pipeline creation to the renderer
	graphicsPipelineCreateInfo.setPipelineLayout(pipelineLayout);
	graphicsPipelineCreateInfo.setRenderPass(renderPass, 0);
	graphicsPipelineCreateInfo.addShaderModule(
		ShaderLibrary::shaderModule("vert4"), VK_SHADER_STAGE_VERTEX_BIT, "main");
	graphicsPipelineCreateInfo.addShaderModule(
		ShaderLibrary::shaderModule("textureFrag"), VK_SHADER_STAGE_FRAGMENT_BIT, "main");
	//graphicsPipelineCreateInfo.addShaderModule(
	//	ShaderLibrary::shaderModule("identityFrag"), VK_SHADER_STAGE_FRAGMENT_BIT, "main");

	vkcpp::GraphicsPipeline graphicsPipeline0(graphicsPipelineCreateInfo, g_vulkanGpuAssets.m_device);
	graphicsPipelineCreateInfo.setRenderPass(renderPass, 1);
	vkcpp::GraphicsPipeline graphicsPipeline1(graphicsPipelineCreateInfo, g_vulkanGpuAssets.m_device);

	DescriptorSetWithBinding::createDescriptorSets(
		descriptorSetLayoutOriginal,
		descriptorPoolOriginal,
		ImageLibrary::imageView("statueImage"),
		textureSampler);

	DrawingFrame::createDrawingFrames(
		g_vulkanGpuAssets.m_device,
		commandPoolOriginal
	);



#undef globals


	globals.g_hInstance = hInstance;
	globals.g_hWnd = hWnd;
	globals.g_surfaceOriginal = std::move(surfaceOriginal);


	globals.g_descriptorSetLayoutOriginal = std::move(descriptorSetLayoutOriginal);
	globals.g_descriptorPoolOriginal = std::move(descriptorPoolOriginal);

	globals.g_pointVertexDeviceBuffer0 = std::move(pointVertexDeviceBuffer0);
	globals.g_pointVertexDeviceBuffer1 = std::move(pointVertexDeviceBuffer1);

	theRenderer.m_renderPass = std::move(renderPass);
	theRenderer.m_pipelineLayout = std::move(pipelineLayout);
	theRenderer.m_pipelineLayout0 = theRenderer.m_pipelineLayout;
	theRenderer.m_graphicsPipeline0 = std::move(graphicsPipeline0);
	theRenderer.m_pipelineLayout1 = theRenderer.m_pipelineLayout;
	theRenderer.m_graphicsPipeline1 = std::move(graphicsPipeline1);


	globals.g_commandPoolOriginal = std::move(commandPoolOriginal);

	globals.g_swapchain_frameBuffers = std::move(swapchain_frameBuffers);

	globals.g_textureSampler = std::move(textureSampler);

}


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
	//	Should we draw?
	std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
	if (now < g_nextFrameTime) {
		return;
	}
	double frameInterval = (1.0 / 60.0);
	long long frameIntervalMicro = frameInterval * 1'000'000.0;
	g_nextFrameTime = std::chrono::high_resolution_clock::now() + std::chrono::microseconds(frameIntervalMicro);


	//	Is the swapchain available to draw?
	if (!globals.g_swapchain_frameBuffers.canDraw()) {
		return;
	}

	//	Is the drawing frame available to draw?
	DrawingFrame& currentDrawingFrame = DrawingFrame::getNextFrameToDraw();
	//	Wait for this drawing frame to be free
	//	TODO: does this need a warning timer?
	currentDrawingFrame.m_inFlightFence.wait();

	//	Need to grab the device from somewhere, might as well be from here.
	vkcpp::Device device = currentDrawingFrame.getDevice();

	//	Can we get a swapchain image to draw on?
	uint32_t	swapchainImageIndex;
	//	Try to get the next image available index.  Don't wait though.
	//	If no image available, just return.
	//	(I think) In some cases, this might return an index before the image
	//	is actually available.  The image available semaphore will be
	//	signaled when the image itself is actually available.
	VkResult vkResult = vkAcquireNextImageKHR(
		device,
		globals.g_swapchain_frameBuffers.vkSwapchain(),
		0,	//	0 timeout => don't wait
		currentDrawingFrame.m_swapchainImageAvailableSemaphore,
		VK_NULL_HANDLE,
		&swapchainImageIndex);
	if (vkResult == VK_NOT_READY) {
		return;
	}

	//	TODO: this is kind of clunky
	theRenderer.m_pointVertexDeviceBuffer0 = globals.g_pointVertexDeviceBuffer0;
	theRenderer.m_pointVertexDeviceBuffer1 = globals.g_pointVertexDeviceBuffer1;

	theRenderer.recordCommandBuffer(
		currentDrawingFrame,
		globals.g_swapchain_frameBuffers.getFrameBuffer(swapchainImageIndex),
		globals.g_swapchain_frameBuffers.getImageExtent());

	vkcpp::SubmitInfo2 submitInfo2;
	//	Command can proceed but wait for the image to
	//	actually be available before writing, i.e.,
	//	(COLOR_ATTACHMENT_OUTPUT) to the image.
	submitInfo2.addWaitSemaphore(
		currentDrawingFrame.m_swapchainImageAvailableSemaphore,
		vkcpp::PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT
	);

	submitInfo2.addCommandBuffer(currentDrawingFrame.m_commandBuffer);
	submitInfo2.addSignalSemaphore(currentDrawingFrame.m_renderFinishedSemaphore);


	//	Show that this drawing frame is now in use ..
	//	Fence will be opened when graphics queue is finished.
	currentDrawingFrame.m_inFlightFence.close();
	g_drawFrameDraws++;

	g_vulkanGpuAssets.m_graphicsQueue.submit2(submitInfo2, currentDrawingFrame.m_inFlightFence);

	//	TODO: Is this where we are supposed to add an image memory barrier
	//	to avoid the present after write hazard?


	vkcpp::PresentInfo presentInfo;
	presentInfo.addWaitSemaphore(currentDrawingFrame.m_renderFinishedSemaphore);
	presentInfo.addSwapchain(
		globals.g_swapchain_frameBuffers.vkSwapchain(),
		swapchainImageIndex
	);

	//	TODO: add timer to check for blocking call?
	vkResult = g_vulkanGpuAssets.m_presentationQueue.present(presentInfo);
	if (vkResult == VK_SUBOPTIMAL_KHR) {
		globals.g_swapchain_frameBuffers.stale();
	}

	DrawingFrame::advanceNextFrameToDrawIndex();


}

const int32_t	KEY_LEFT_ARROW = 37;
const int32_t	KEY_UP_ARROW = 38;
const int32_t	KEY_RIGHT_ARROW = 39;
const int32_t	KEY_DOWN_ARROW = 40;

const int32_t	KEY_W = 'W';
const int32_t	KEY_A = 'A';
const int32_t	KEY_S = 'S';
const int32_t	KEY_D = 'D';



void handleKeyDown(int32_t key) {

	std::cout << "key: " << key << "\n";

	switch (key) {

	case KEY_LEFT_ARROW: g_theCamera.lookCenterDelta(-0.1, 0.0, 0.0); break;
	case KEY_RIGHT_ARROW: g_theCamera.lookCenterDelta(0.1, 0.0, 0.0); break;

	case KEY_UP_ARROW: g_theCamera.lookCenterDelta(0.0, 0.1, 0.0); break;
	case KEY_DOWN_ARROW: g_theCamera.lookCenterDelta(0.0, -0.1, 0.0); break;

	case KEY_W:	g_theCamera.eyeDelta(0.0, 0.0, -0.1); break;
	case KEY_S:	g_theCamera.eyeDelta(0.0, 0.0, 0.1); break;


	}

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
		g_globals.g_swapchain_frameBuffers.stale();
		break;

	case WM_KEYDOWN:
		handleKeyDown(wParam);
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

	if (!hWnd) {
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

	//g_imageData.resize(bitmapInfo.bmiHeader.biSizeImage / sizeof(uint32_t));
	//g_width = bitmap.bmWidth;
	//g_height = bitmap.bmHeight;

	//returnCode = GetDIBits(
	//	commandWindowDC,
	//	bitmapHandle,
	//	0,
	//	windowHeight,
	//	g_imageData.data(),
	//	&bitmapInfo,
	//	DIB_RGB_COLORS
	//);

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

	HINSTANCE hInstance = GetModuleHandle(NULL);
	RegisterMyWindowClass(hInstance);
	HWND hWnd = CreateFirstWindow(hInstance);

	HWND commandBufferHwnd = createCommandBufferHwnd(hInstance, hWnd);


	Shape shape1 = g_pointVertexBuffer0.add(g_theCubeCenter);
	//shape1.addOffset(0.0, -0.5, 0.0);
	//shape1.scale(1.5, 1.5, 0.0);

	//Shape shape2 = g_pointVertexBuffer0.add(g_theSquareCenter);
	//shape2.scale(0.5, 0.5, 0.0);
	//shape2.addOffset(1.0, 0.0, -0.5);


	Shape shape3 = g_pointVertexBuffer1.add(g_theRightTriangle);
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
	g_vulkanGpuAssets.m_device.waitIdle();

}

int CaptureAnImage(HWND hWnd)
{

	// Retrieve the handle to a display device context for the client 
	// area of the window. 
	HDC clientDC = GetDC(hWnd);

	// Get the client area for size calculation.
	RECT windowClientRect;
	GetClientRect(hWnd, &windowClientRect);
	const int windowClientWidth = windowClientRect.right - windowClientRect.left;
	const int windowClientHeight = windowClientRect.bottom - windowClientRect.top;
	HBITMAP bitmapHandle = CreateCompatibleBitmap(clientDC, windowClientWidth, windowClientHeight);


	//	Now get the actual bitmap bits from the memory bitmap.
	BITMAP bitmap;
	GetObject(bitmapHandle, sizeof(BITMAP), &bitmap);

	BITMAPINFOHEADER   bi;

	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bitmap.bmWidth;
	bi.biHeight = bitmap.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	DWORD bitmapSize = ((bitmap.bmWidth * bi.biBitCount + 31) / 32) * 4 * bitmap.bmHeight;

	HANDLE hDIB = GlobalAlloc(GHND, bitmapSize);
	char* lpbitmap = (char*)GlobalLock(hDIB);

	// Gets the "bits" from the bitmap, and copies them into a buffer 
	// that's pointed to by lpbitmap.
	GetDIBits(
		clientDC,
		bitmapHandle,
		0,
		(UINT)bitmap.bmHeight,
		lpbitmap,
		(BITMAPINFO*)&bi,
		DIB_RGB_COLORS);

	//	makeImageFromBitmap();


		// Unlock and Free the DIB from the heap.
	GlobalUnlock(hDIB);
	GlobalFree(hDIB);
	DeleteObject(bitmapHandle);
	ReleaseDC(hWnd, clientDC);


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


