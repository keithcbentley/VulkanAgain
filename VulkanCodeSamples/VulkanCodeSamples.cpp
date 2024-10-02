// VulkanAgain.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pragmas.hpp"

#include <iostream>
#include <fstream>
#include <array>
#include <chrono>
#include <map>
#include <unordered_map>
#include <variant>
#include <bitset>

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

	static const uint32_t	TRANSFER_QUEUE_FAMILY_INDEX = 0;
	static const uint32_t	TRANSFER_QUEUE_COUNT = 1;


	//static const uint32_t	GRAPHICS_QUEUE_FAMILY_INDEX = 0;
	//static const uint32_t	GRAPHICS_QUEUE_INDEX = 0;

	//static const uint32_t	PRESENTATION_QUEUE_FAMILY_INDEX = 0;
	//static const uint32_t	PRESENTATION_QUEUE_INDEX = 0;




};







const wchar_t* szTitle = TEXT("Vulkan Code Samples");
const wchar_t* szWindowClass = TEXT("Vulkan Code Samples Class");

const int windowPositionX = 500;
const int windowPositionY = 500;
const int windowWidth = 800;
const int windowHeight = 600;




//class DrawingFrame {
//
//
//	void createCommandBuffer(vkcpp::CommandPool commandPool) {
//		m_commandBuffer = std::move(vkcpp::CommandBuffer(commandPool));
//	}
//
//	void createUniformBuffer() {
//		m_uniformBufferMemory = std::move(
//			vkcpp::Buffer_DeviceMemory(
//				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
//				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//				sizeof(ModelViewProjTransform),
//				m_device
//			));
//	}
//
//	void createSyncObjects() {
//		m_swapchainImageAvailableSemaphore = std::move(vkcpp::Semaphore(m_device));
//		m_renderFinishedSemaphore = std::move(vkcpp::Semaphore(m_device));
//		m_inFlightFence = std::move(vkcpp::Fence(m_device, VKCPP_FENCE_CREATE_OPENED));
//	}
//
//	void createDescriptorSet(
//		vkcpp::DescriptorPool		descriptorPool,
//		vkcpp::DescriptorSetLayout descriptorSetLayout,
//		vkcpp::ImageView textureImageView,
//		vkcpp::Sampler textureSampler
//	) {
//		vkcpp::DescriptorSet descriptorSet(descriptorSetLayout, descriptorPool);
//		vkcpp::DescriptorSetUpdater descriptorSetUpdater(descriptorSet);
//		descriptorSetUpdater.addWriteDescriptor(
//			MagicValues::UBO_DESCRIPTOR_BINDING_INDEX,
//			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
//			m_uniformBufferMemory.m_buffer,
//			sizeof(ModelViewProjTransform));
//		descriptorSetUpdater.addWriteDescriptor(
//			MagicValues::TEXTURE_DESCRIPTOR_BINDING_INDEX,
//			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
//			textureImageView,
//			textureSampler);
//		descriptorSetUpdater.updateDescriptorSets();
//		m_descriptorSet = std::move(descriptorSet);
//	}
//
//	vkcpp::Device m_device;	//	Just remember the device to make things easier.
//
//public:
//
//	vkcpp::Fence	m_inFlightFence;
//	vkcpp::Semaphore m_swapchainImageAvailableSemaphore;
//	vkcpp::Semaphore m_renderFinishedSemaphore;
//	vkcpp::CommandBuffer	m_commandBuffer;;
//	vkcpp::Buffer_DeviceMemory	m_uniformBufferMemory;
//	vkcpp::DescriptorSet	m_descriptorSet;
//
//	DrawingFrame() {};
//
//	DrawingFrame(const DrawingFrame&) = delete;
//	DrawingFrame& operator=(const DrawingFrame&) = delete;
//	DrawingFrame(DrawingFrame&&) = delete;
//	DrawingFrame& operator=(DrawingFrame&&) = delete;
//
//
//	void create(
//		vkcpp::Device device,
//		vkcpp::CommandPool commandPool,
//		vkcpp::DescriptorPool descriptorPool,
//		vkcpp::DescriptorSetLayout descriptorSetLayout,
//		vkcpp::ImageView textureImageView,
//		vkcpp::Sampler textureSampler
//	) {
//		m_device = device;
//		createUniformBuffer();
//		createCommandBuffer(commandPool);
//		createSyncObjects();
//		createDescriptorSet(
//			descriptorPool,
//			descriptorSetLayout,
//			textureImageView,
//			textureSampler);
//	}
//
//
//	vkcpp::Device getDevice() const {
//		return m_device;
//	}
//};


//class AllDrawingFrames {
//
//private:
//	std::vector<DrawingFrame> m_drawingFrames;
//
//public:
//
//	int m_nextFrameToDrawIndex = 0;
//
//
//	AllDrawingFrames() {}
//	~AllDrawingFrames() = default;
//
//	AllDrawingFrames(const AllDrawingFrames&) = delete;
//	AllDrawingFrames& operator=(const AllDrawingFrames&) = delete;
//
//	AllDrawingFrames(int frameCount) : m_drawingFrames(frameCount) {}
//
//	void advanceNextFrameToDraw() {
//		m_nextFrameToDrawIndex = (m_nextFrameToDrawIndex + 1) % m_drawingFrames.size();;
//	}
//
//	DrawingFrame& getNextFrameToDraw() {
//		return m_drawingFrames.at(m_nextFrameToDrawIndex);
//	}
//
//	int	frameCount() { return static_cast<int>(m_drawingFrames.size()); }
//
//	void createDrawingFrames(
//		vkcpp::Device device,
//		vkcpp::CommandPool commandPool,
//		vkcpp::DescriptorPool descriptorPool,
//		vkcpp::DescriptorSetLayout descriptorSetLayout,
//		vkcpp::ImageView textureImageView,
//		vkcpp::Sampler textureSampler
//	) {
//		for (DrawingFrame& drawingFrame : m_drawingFrames) {
//			drawingFrame.create(
//				device,
//				commandPool,
//				descriptorPool,
//				descriptorSetLayout,
//				textureImageView,
//				textureSampler
//			);
//		}
//	}
//
//};





vkcpp::VulkanInstance createVulkanInstance() {

	vkcpp::VulkanInstanceCreateInfo vulkanInstanceCreateInfo{};
	vulkanInstanceCreateInfo.addLayer("VK_LAYER_KHRONOS_validation");


	vulkanInstanceCreateInfo.addExtension("VK_EXT_debug_utils");
	//vulkanInstanceCreateInfo.addExtension("VK_KHR_surface");
	//vulkanInstanceCreateInfo.addExtension("VK_KHR_win32_surface");

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
	vkcpp::PhysicalDevice	g_physicalDevice;
	vkcpp::Device			g_deviceOriginal;

	//vkcpp::Queue				g_graphicsQueue;
	//vkcpp::Queue				g_presentationQueue;

	vkcpp::CommandPool		g_commandPoolOriginal;


};

Globals g_globals;

//AllDrawingFrames g_allDrawingFrames(MagicValues::MAX_FRAMES_IN_FLIGHT);






//void transitionImageLayout(
//	vkcpp::Image image,
//	VkFormat format,
//	VkImageLayout oldLayout,
//	VkImageLayout newLayout,
//	vkcpp::CommandPool commandPool,
//	vkcpp::Queue graphicsQueue
//) {
//	vkcpp::CommandBuffer commandBuffer(commandPool);
//	commandBuffer.beginOneTimeSubmit();
//
//	vkcpp::ImageMemoryBarrier2 imageMemoryBarrier(oldLayout, newLayout, image);
//	vkcpp::DependencyInfo dependencyInfo;
//	dependencyInfo.addImageMemoryBarrier(imageMemoryBarrier);
//	commandBuffer.cmdPipelineBarrier2(dependencyInfo);
//
//	commandBuffer.end();
//
//	vkcpp::Fence completedFence(commandPool.getVkDevice());
//	graphicsQueue.submit2(commandBuffer, completedFence);
//	completedFence.wait();
//
//}
//
//
//void copyBufferToImage(
//	vkcpp::Buffer buffer,
//	vkcpp::Image image,
//	int width,
//	int height,
//	vkcpp::CommandPool commandPool,
//	vkcpp::Queue graphicsQueue
//) {
//
//	vkcpp::CommandBuffer commandBuffer(commandPool);
//	commandBuffer.beginOneTimeSubmit();
//	commandBuffer.cmdCopyBufferToImage(buffer, image, width, height);
//	commandBuffer.end();
//
//	vkcpp::Fence completedFence(commandPool.getVkDevice());
//	graphicsQueue.submit2(commandBuffer, completedFence);
//	completedFence.wait();
//}



//vkcpp::Image_Memory_View createTextureFromFile(
//	const char* fileName,
//	vkcpp::Device device,
//	vkcpp::CommandPool commandPool,
//	vkcpp::Queue graphicsQueue
//) {
//	const VkFormat targetFormat = VK_FORMAT_R8G8B8A8_SRGB;
//
//	int texWidth;
//	int texHeight;
//	int texChannels;
//
//	stbi_uc* pixels = stbi_load(fileName, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
//	if (!pixels) {
//		throw std::runtime_error("failed to load texture image!");
//	}
//
//	const VkDeviceSize imageSize = texWidth * texHeight * 4;
//
//	//	Make a device (gpu) staging buffer and copy the pixels into it.
//	vkcpp::Buffer_DeviceMemory stagingBuffer_DeviceMemoryMapped(
//		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
//		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//		imageSize,
//		pixels,
//		device);
//
//	stbi_image_free(pixels);	//	Don't need these anymore.  Pixels are on gpu now.
//
//	//	Make our target image and memory.
//	VkExtent2D texExtent;
//	texExtent.width = texWidth;
//	texExtent.height = texHeight;
//	vkcpp::Image_Memory textureImage_DeviceMemory(
//		texExtent,
//		targetFormat,
//		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
//		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//		device);
//
//	//	Change image format to be best target for transfer into.
//	transitionImageLayout(
//		textureImage_DeviceMemory.m_image,
//		targetFormat,
//		VK_IMAGE_LAYOUT_UNDEFINED,
//		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//		commandPool,
//		graphicsQueue);
//
//	//	Copy image pixels from staging buffer into image memory.
//	copyBufferToImage(
//		stagingBuffer_DeviceMemoryMapped.m_buffer,
//		textureImage_DeviceMemory.m_image,
//		texWidth,
//		texHeight,
//		commandPool,
//		graphicsQueue);
//
//	//	Now transition the image layout to best for shader images.
//	transitionImageLayout(
//		textureImage_DeviceMemory.m_image,
//		targetFormat,
//		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
//		commandPool,
//		graphicsQueue);
//
//	//	Shaders are accessed through image views, not directly from images.
//	vkcpp::ImageViewCreateInfo textureImageViewCreateInfo(
//		VK_IMAGE_VIEW_TYPE_2D,
//		targetFormat,
//		VK_IMAGE_ASPECT_COLOR_BIT);
//	textureImageViewCreateInfo.image = textureImage_DeviceMemory.m_image;
//	vkcpp::ImageView textureImageView(textureImageViewCreateInfo, device);
//
//	//	Package everything up for use as a shader.
//	return vkcpp::Image_Memory_View(
//		std::move(textureImage_DeviceMemory.m_image),
//		std::move(textureImage_DeviceMemory.m_deviceMemory),
//		std::move(textureImageView));
//}






void VulkanStuff(HINSTANCE hInstance, HWND hWnd, Globals& globals) {

	vkcpp::VulkanInstance vulkanInstance = createVulkanInstance();
	vulkanInstance.createDebugMessenger();

	vkcpp::PhysicalDevice physicalDevice = vulkanInstance.getPhysicalDevice(0);


	vkcpp::DeviceCreateInfo deviceCreateInfo;
	//	deviceCreateInfo.addExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		//deviceCreateInfo.addDeviceQueue(MagicValues::GRAPHICS_QUEUE_FAMILY_INDEX, 1);
		//deviceCreateInfo.addDeviceQueue(MagicValues::PRESENTATION_QUEUE_FAMILY_INDEX, 1);

	VkPhysicalDeviceFeatures2 vkPhysicalDeviceFeatures2 = physicalDevice.getPhysicalDeviceFeatures2();
	deviceCreateInfo.pNext = &vkPhysicalDeviceFeatures2;

	vkcpp::Device deviceOriginal(deviceCreateInfo, physicalDevice);

	//vkcpp::Queue graphicsQueue = deviceOriginal.getDeviceQueue(
	//	MagicValues::GRAPHICS_QUEUE_FAMILY_INDEX,
	//	MagicValues::GRAPHICS_QUEUE_INDEX);

	//vkcpp::Queue presentationQueue = deviceOriginal.getDeviceQueue(
	//	MagicValues::PRESENTATION_QUEUE_FAMILY_INDEX,
	//	MagicValues::PRESENTATION_QUEUE_INDEX);





	//VkCommandPoolCreateInfo commandPoolCreateInfo{};
	//commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	//commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	//commandPoolCreateInfo.queueFamilyIndex = MagicValues::GRAPHICS_QUEUE_FAMILY_INDEX;
	//vkcpp::CommandPool commandPoolOriginal(commandPoolCreateInfo, deviceOriginal);



#undef globals

	globals.g_hInstance = hInstance;
	globals.g_hWnd = hWnd;

	globals.g_vulkanInstance = std::move(vulkanInstance);
	globals.g_physicalDevice = std::move(physicalDevice);
	globals.g_deviceOriginal = std::move(deviceOriginal);



	//	globals.g_commandPoolOriginal = std::move(commandPoolOriginal);


}


#define valueStringPair(name) name,#name

std::string BitFieldNamesAppender(
	uint32_t bitField,
	const std::unordered_map<uint32_t, const char*>& bitFieldNames
) {

	if (bitField == 0) {
		return "No bits set";
	}

	std::string	names;
	bool	needSeparator = false;
	for (auto& pair : bitFieldNames) {
		if (bitField & pair.first) {
			if (needSeparator) {
				names.append(" | ");
			}
			names.append(pair.second);
			needSeparator = true;
		}
	}

	return names;
}


std::string MemoryPropertyFlagsString(VkMemoryPropertyFlags bitField) {

	static const std::unordered_map<VkMemoryPropertyFlags, const char*>
		s_bitFieldNames{
			{valueStringPair(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)},
			{valueStringPair(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)},
			{valueStringPair(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)},
			{valueStringPair(VK_MEMORY_PROPERTY_HOST_CACHED_BIT)},
			{valueStringPair(VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)},
			{valueStringPair(VK_MEMORY_PROPERTY_PROTECTED_BIT)},
			{valueStringPair(VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD)},
			{valueStringPair(VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD)},
			{valueStringPair(VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV)} };

	if (bitField == 0) {
		return "No bits set. (Host only memory?)";
	}

	return BitFieldNamesAppender(bitField, s_bitFieldNames);
}

std::string MemoryHeapFlagsString(VkMemoryHeapFlags bitField) {

	static const std::unordered_map<VkMemoryHeapFlags, const char*>
		s_bitFieldNames{
			{valueStringPair(VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)},
			{valueStringPair(VK_MEMORY_HEAP_MULTI_INSTANCE_BIT)} };

	return BitFieldNamesAppender(bitField, s_bitFieldNames);

}

std::string DeviceQueueFlagsString(VkMemoryHeapFlags bitField) {

	static const std::unordered_map<VkMemoryHeapFlags, const char*>
		s_bitFieldNames{
			{valueStringPair(VK_QUEUE_GRAPHICS_BIT)},
			{valueStringPair(VK_QUEUE_COMPUTE_BIT)},
			{valueStringPair(VK_QUEUE_TRANSFER_BIT)},
			{valueStringPair(VK_QUEUE_SPARSE_BINDING_BIT)},
			{valueStringPair(VK_QUEUE_PROTECTED_BIT)},
			{valueStringPair(VK_QUEUE_VIDEO_DECODE_BIT_KHR)},
			{valueStringPair(VK_QUEUE_VIDEO_ENCODE_BIT_KHR)},
			{valueStringPair(VK_QUEUE_OPTICAL_FLOW_BIT_NV)} };

	return BitFieldNamesAppender(bitField, s_bitFieldNames);

}



void DumpPhysicalDeviceMemoryInfo(vkcpp::PhysicalDevice physicalDevice) {

	std::cout << "PhysicalDeviceMemoryInfo\n";

	VkPhysicalDeviceMemoryProperties vkPhysicalDeviceMemoryProperties
		= physicalDevice.getPhysicalDeviceMemoryProperties();

	std::cout << "  memoryTypes (" << vkPhysicalDeviceMemoryProperties.memoryTypeCount << ")\n";
	for (int i = 0; i < vkPhysicalDeviceMemoryProperties.memoryTypeCount; i++) {
		std::cout << "    " << i;
		std::cout << "  memoryType propertyFlags: ";
		std::cout << MemoryPropertyFlagsString(vkPhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags);
		std::cout << "\n";
		std::cout << "       heapIndex: " << vkPhysicalDeviceMemoryProperties.memoryTypes[i].heapIndex;
		std::cout << "\n";
	}

	std::cout << "  memoryHeaps (" << vkPhysicalDeviceMemoryProperties.memoryHeapCount << ")\n";

	for (int i = 0; i < vkPhysicalDeviceMemoryProperties.memoryHeapCount; i++) {
		std::cout << "    " << i;
		std::cout << "  memoryHeap size:" << vkPhysicalDeviceMemoryProperties.memoryHeaps[i].size;
		std::cout << "\n";
		std::cout << "       heapFlags: ";
		std::cout << MemoryHeapFlagsString(vkPhysicalDeviceMemoryProperties.memoryHeaps[i].flags);
		std::cout << "\n";
	}


}

void DumpPhysicalDeviceQueueInfo(vkcpp::PhysicalDevice physicalDevice) {

	std::cout << "PhysicalDeviceQueueInfo\n";
	std::vector<VkQueueFamilyProperties> allQueueFamilyProperties = physicalDevice.getAllQueueFamilyProperties();
	int	index = 0;
	for (const VkQueueFamilyProperties& vkQueueFamilyProperties : allQueueFamilyProperties) {
		std::cout << "  Queue family index: " << index << "\n";
		std::cout << "     flags: "
			<< DeviceQueueFlagsString(vkQueueFamilyProperties.queueFlags) << "\n";
		std::cout << "     count: " << vkQueueFamilyProperties.queueCount << "\n";
		std::cout << "     timestampValidBits: " << vkQueueFamilyProperties.timestampValidBits << "\n";
		//		std::cout << "     minImageTransferGranularity: " << vkQueueFamilyProperties.minImageTransferGranularity;
		index++;
	}

}

std::chrono::high_resolution_clock::time_point g_nextFrameTime = std::chrono::high_resolution_clock::now();


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
		//		std::cout << "WM_WINDOWPOSCHANGED\n";
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

	while (!done) {
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT) {
				done = true;
				break;
			}

			//	Placeholder for keyboard handling.
			if (msg.message == WM_KEYDOWN) {
				if (msg.wParam == 'A') {
				}
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

	}
	return;
}


int main()
{
	std::cout << "Hello World!\n";


	std::cout.imbue(std::locale(""));



	HINSTANCE hInstance = GetModuleHandle(NULL);
	RegisterMyWindowClass(hInstance);
	HWND hWnd = CreateFirstWindow(hInstance);


	VulkanStuff(hInstance, hWnd, g_globals);
	DumpPhysicalDeviceMemoryInfo(g_globals.g_physicalDevice);
	DumpPhysicalDeviceQueueInfo(g_globals.g_physicalDevice);

	MessageLoop(g_globals);

	//	Wait for device to be idle before exiting and cleaning up globals.
	g_globals.g_deviceOriginal.waitIdle();

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


