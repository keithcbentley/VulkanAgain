

#include <exception>
#include <vector>
#include <unordered_set>
#include <iostream>
#include <fstream>
#include <string>

#include <vulkan/vulkan.h>

template<typename T>
concept DefaultCopyConstructible = requires (T t) {
	T(t);
};

template<typename T>
concept VirtualDestructor = requires{
	std::has_virtual_destructor_v<T>;
};

template<typename T>
concept DefaultCopyAssignable = requires (T t1, T t2) {
	t1 = t2;
};

template<typename T>
concept DefaultMoveConstructible = requires (T t) {
	T(std::move(t));
};

template<typename T>
concept DefaultMoveAssignable = requires (T t1, T t2) {
	t1 = std::move(t2);
};

template<typename T>
concept BoolConvertible = requires (T t) {
	t.operator bool();
};

template<typename T>
concept Cloneable = requires (T t) {
	t.clone();
};





namespace vkcpp {

	class Exception : public std::exception {
		VkResult	m_vkResult = VK_ERROR_UNKNOWN;

	public:
		Exception(VkResult vkResult) :m_vkResult(vkResult) {}

		Exception(const char* msg) : std::exception(msg) {}

	};



	class ShutdownException : public Exception {

	public:
		ShutdownException() : Exception(VK_NOT_READY) {}

	};

	class NullHandleException : public Exception {

	public:
		NullHandleException() : Exception(VK_INCOMPLETE) {}

	};

	template<typename HandleArg_t, typename OwnerArg_t = VkDevice>
	class HandleWithOwner {

	public:
		using Handle_t = HandleArg_t;
		using Owner_t = OwnerArg_t;
		using DestroyFunc_t = void (*)(HandleArg_t, OwnerArg_t);

	private:

		void destroy() {
			if (m_pfnDestroy) {
				(*m_pfnDestroy)(m_handle, m_owner);
			}
			makeEmpty();
		}


	protected:

		void makeEmpty() {
			m_handle = Handle_t{};
			m_owner = Owner_t();
			m_pfnDestroy = nullptr;
		}

		Handle_t	m_handle{};
		Owner_t	m_owner{};
		DestroyFunc_t	m_pfnDestroy = nullptr;

		HandleWithOwner() = default;
		~HandleWithOwner() { destroy(); }

		HandleWithOwner(Handle_t handle, Owner_t owner)
			: m_handle(handle)
			, m_owner(owner)
			, m_pfnDestroy(nullptr) {
		}

		HandleWithOwner(HandleArg_t handle, OwnerArg_t owner, DestroyFunc_t pfnDestroy)
			: m_handle(handle)
			, m_owner(owner)
			, m_pfnDestroy(pfnDestroy) {
		}

		HandleWithOwner(const HandleWithOwner& other)
			: m_handle(other.m_handle)
			, m_owner(other.m_owner)
			, m_pfnDestroy(nullptr) {
		}

		HandleWithOwner& operator=(const HandleWithOwner& other) {
			if (this == &other) {
				return *this;
			}
			(*this).~HandleWithOwner();
			new(this) HandleWithOwner(other);
			return *this;
		}

		HandleWithOwner(HandleWithOwner&& other) noexcept
			: m_handle(other.m_handle)
			, m_owner(other.m_owner)
			, m_pfnDestroy(other.m_pfnDestroy) {
			other.makeEmpty();
		}
		HandleWithOwner& operator=(HandleWithOwner&& other) noexcept {
			if (this == &other) {
				return *this;
			}
			(*this).~HandleWithOwner();
			new(this)HandleWithOwner(std::move(other));
			return *this;
		}

	public:

		operator bool() const { return !!m_handle; }
		operator Handle_t() const {
			if (!m_handle) {
				throw NullHandleException();
			}
			return m_handle;
		}
		OwnerArg_t getOwner() const {
			if (!m_owner) {
				throw NullHandleException();
			}
			return m_owner;
		}
		VkDevice getVkDevice() const
			requires std::same_as<Owner_t, VkDevice> {
			if (!m_owner) {
				throw NullHandleException();
			}
			return m_owner;
		}
	};




	class VersionNumber {
	public:
		uint32_t	m_version;

		VersionNumber(uint32_t version) :m_version(version) {}

		uint32_t	major() const { return VK_API_VERSION_MAJOR(m_version); }
		uint32_t	minor() const { return VK_API_VERSION_MINOR(m_version); }
		uint32_t	patch() const { return VK_API_VERSION_PATCH(m_version); }
		uint32_t	variant() const { return VK_API_VERSION_VARIANT(m_version); }

		static VersionNumber getVersionNumber() {
			uint32_t	version;
			vkEnumerateInstanceVersion(&version);
			return VersionNumber(version);
		}
	};


	class LayerProperties {

	public:

		static std::vector<VkLayerProperties> getAllInstanceLayerProperties() {
			uint32_t	instanceLayerCount;
			VkResult vkResult = vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			std::vector<VkLayerProperties> allInstanceLayerProperties(instanceLayerCount);
			vkResult = vkEnumerateInstanceLayerProperties(&instanceLayerCount, allInstanceLayerProperties.data());
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			return allInstanceLayerProperties;

		}
	};


	class InstanceExtensionProperties {

	public:

		static std::vector<VkExtensionProperties> getAllInstanceExtensionProperties() {
			uint32_t	instanceExtensionCount;
			VkResult vkResult = vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			std::vector<VkExtensionProperties> allInstanceExtensionProperties = std::vector<VkExtensionProperties>(instanceExtensionCount);
			vkResult = vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, allInstanceExtensionProperties.data());
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			return allInstanceExtensionProperties;
		}

	};


	class DebugUtilsMessenger {

		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* /*pUserData*/) {
			if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
				std::cerr << "VERBOSE:\n";
			}
			else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
				std::cerr << "INFO:\n";
			}
			else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
				std::cerr << "WARNING:\n";
			}
			else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
				std::cerr << "ERROR:\n";
			}
			else {
				std::cerr << "OTHER: " << messageSeverity << "\n";
			}
			if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
				std::cerr << "MESSAGE_TYPE_GENERAL\n";
			}
			else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
				std::cerr << "MESSAGE_TYPE_VALIDATION\n";
			}
			else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
				std::cerr << "MESSAGE_TYPE_PERFORMANCE\n";
			}
			else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT) {
				std::cerr << "MESSAGE_TYPE_DEVICE_ADDRESS_BINDING\n";
			}
			else {
				std::cerr << "OTHER: " << messageType << "\n";
			}

			std::cerr << "  " << pCallbackData->pMessage << std::endl;
			std::cerr << "<<<<<<<<\n";
			return VK_FALSE;
		}

	public:

		DebugUtilsMessenger() = delete;
		DebugUtilsMessenger(const DebugUtilsMessenger&) = delete;
		DebugUtilsMessenger& operator=(const DebugUtilsMessenger&) = delete;
		DebugUtilsMessenger(DebugUtilsMessenger&&) = delete;
		DebugUtilsMessenger& operator=(DebugUtilsMessenger&&) = delete;

		static VkDebugUtilsMessengerCreateInfoEXT getCreateInfo() {
			VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{};
			debugMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			debugMessengerCreateInfo.pNext = nullptr;
			debugMessengerCreateInfo.flags = 0;
			debugMessengerCreateInfo.messageSeverity = 0
				| VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
				;
			debugMessengerCreateInfo.messageType = 0
				//| VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT
				;
			debugMessengerCreateInfo.pfnUserCallback = debugCallback;
			debugMessengerCreateInfo.pUserData = nullptr;
			return debugMessengerCreateInfo;
		}

	};


	class PhysicalDevice {
		VkPhysicalDevice	m_vkPhysicalDevice = nullptr;

		void destroy() {
			m_vkPhysicalDevice = nullptr;
		}

	public:

		PhysicalDevice() {}

		~PhysicalDevice() {
			destroy();
		}

		PhysicalDevice(const PhysicalDevice& other)
			: m_vkPhysicalDevice(other.m_vkPhysicalDevice) {
		}

		PhysicalDevice& operator=(const PhysicalDevice& other) {
			if (this == &other) {
				return *this;
			}
			m_vkPhysicalDevice = other.m_vkPhysicalDevice;
			return *this;
		}

		PhysicalDevice(PhysicalDevice&& other) noexcept
			: m_vkPhysicalDevice(other.m_vkPhysicalDevice) {
			other.m_vkPhysicalDevice = nullptr;
		}

		PhysicalDevice& operator=(PhysicalDevice&& other) noexcept {
			if (this == &other) {
				return *this;
			}
			m_vkPhysicalDevice = other.m_vkPhysicalDevice;
			other.m_vkPhysicalDevice;
			return *this;
		}


		PhysicalDevice(VkPhysicalDevice vkPhysicalDevice) :m_vkPhysicalDevice(vkPhysicalDevice) {}

		operator VkPhysicalDevice() { return m_vkPhysicalDevice; }

		uint32_t findMemoryTypeIndex(
			uint32_t usableMemoryIndexBits,
			VkMemoryPropertyFlags requiredProperties
		) {
			VkPhysicalDeviceMemoryProperties memProperties;
			vkGetPhysicalDeviceMemoryProperties(m_vkPhysicalDevice, &memProperties);

			for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
				if ((usableMemoryIndexBits & (1 << i))
					&& (memProperties.memoryTypes[i].propertyFlags & requiredProperties) == requiredProperties) {
					return i;
				}
			}
			throw std::runtime_error("failed to find suitable memory type!");
		}




		std::vector<VkQueueFamilyProperties>	getAllQueueFamilyProperties() const {
			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevice, &queueFamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> allQueueFamilyProperties(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevice, &queueFamilyCount, allQueueFamilyProperties.data());
			return allQueueFamilyProperties;
		}


	};


	class VulkanInstanceCreateInfo : public VkInstanceCreateInfo {

		std::unordered_set<std::string>	m_layerNames{};
		std::unordered_set<std::string>	m_extensionNames{};

		std::vector<const char*>	m_layerStrings{};
		std::vector<const char*>	m_extensionStrings{};



	public:
		VulkanInstanceCreateInfo(const VulkanInstanceCreateInfo&) = delete;
		VulkanInstanceCreateInfo& operator=(const VulkanInstanceCreateInfo&) = delete;
		VulkanInstanceCreateInfo(VulkanInstanceCreateInfo&&) = delete;
		VulkanInstanceCreateInfo& operator=(VulkanInstanceCreateInfo&&) = delete;


		VulkanInstanceCreateInfo() : VkInstanceCreateInfo{} {
			sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		}

		VulkanInstanceCreateInfo& construct() {
			m_layerStrings.clear();
			ppEnabledLayerNames = nullptr;
			for (const std::string& layerName : m_layerNames) {
				m_layerStrings.push_back(layerName.c_str());
			}
			enabledLayerCount = static_cast<uint32_t>(m_layerNames.size());
			if (enabledLayerCount > 0) {
				ppEnabledLayerNames = m_layerStrings.data();
			}

			m_extensionStrings.clear();
			ppEnabledExtensionNames = nullptr;
			for (const std::string& extensionName : m_extensionNames) {
				m_extensionStrings.push_back(extensionName.c_str());
			}
			enabledExtensionCount = static_cast<uint32_t>(m_extensionNames.size());
			if (enabledExtensionCount > 0) {
				ppEnabledExtensionNames = m_extensionStrings.data();
			}

			return *this;

		}

		void addLayer(const char* layerName) {
			m_layerNames.insert(layerName);
		}

		void addExtension(const char* extensionName) {
			m_extensionNames.insert(extensionName);
		}

	};

	class VulkanInstance : public HandleWithOwner<VkInstance, VkInstance> {

		VkDebugUtilsMessengerEXT	m_messenger = nullptr;

		VulkanInstance(VkInstance vkInstance, DestroyFunc_t pfnDestroy)
			: HandleWithOwner(vkInstance, vkInstance, pfnDestroy) {
		}

		static void destroy(VkInstance vkInstance, VkInstance) {
			if (vkInstance) {
				vkDestroyInstance(vkInstance, nullptr);
			}
		}

	public:

		VulkanInstance() {}
		VulkanInstance(VulkanInstanceCreateInfo& vulkanInstanceCreateInfo) {
			VkInstance vkInstance;
			VkResult vkResult = vkCreateInstance(&vulkanInstanceCreateInfo.construct(), nullptr, &vkInstance);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			new(this) VulkanInstance(vkInstance, &destroy);
		}

		~VulkanInstance() {
			if (m_messenger) {
				//	Compiler doesn't seem to mind since both function types return void.
				PFN_vkDestroyDebugUtilsMessengerEXT	pDestroyFunc =
					reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
						vkGetInstanceProcAddr(*this, "vkDestroyDebugUtilsMessengerEXT"));
				if (pDestroyFunc) {
					pDestroyFunc(*this, m_messenger, nullptr);
				}
				m_messenger = nullptr;
			}
		}

		VulkanInstance(const VulkanInstance&) = delete;
		VulkanInstance& operator=(const VulkanInstance&) = delete;

		VulkanInstance(VulkanInstance&& other) noexcept
			: HandleWithOwner(std::move(other)) {
			m_messenger = std::move(other.m_messenger);
			other.makeEmpty();
			other.m_messenger = nullptr;
		}

		VulkanInstance& operator=(VulkanInstance&& other) noexcept {
			if (this == &other) {
				return *this;
			}
			(*this).~VulkanInstance();
			new(this)VulkanInstance(std::move(other));
			return *this;
		}



		void createDebugMessenger() {
			//	There's something odd here.  I think the compiler complains here because vkGetInstanceProcAddrs returns a pointer to a function that
			//	returns a void, and that is being cast to a function pointer that has a non-void return type.  If the return function is cast to a
			//	function that also has a void return type, the compiler doesn't seem to mind.  It appears to ignore any differences in the parameter
			//	types of the functions.
#pragma warning(suppress:4191)
			PFN_vkCreateDebugUtilsMessengerEXT	pCreateFunc = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
				vkGetInstanceProcAddr(*this, "vkCreateDebugUtilsMessengerEXT"));
			if (pCreateFunc == nullptr) {
				throw Exception(VK_ERROR_EXTENSION_NOT_PRESENT);
			}

			VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = DebugUtilsMessenger::getCreateInfo();
			VkResult vkResult = (*pCreateFunc)(*this, &debugMessengerCreateInfo, nullptr, &m_messenger);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
		}

		std::vector<VkPhysicalDevice>  getAllPhysicalDevices() const {
			uint32_t	physicalDeviceCount = 0;
			VkResult vkResult = vkEnumeratePhysicalDevices(*this, &physicalDeviceCount, nullptr);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			std::vector<VkPhysicalDevice> allPhysicalDevices(physicalDeviceCount);
			vkResult = vkEnumeratePhysicalDevices(*this, &physicalDeviceCount, allPhysicalDevices.data());
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			return allPhysicalDevices;
		}

		PhysicalDevice getPhysicalDevice(int physicalDeviceIndex) const {
			std::vector<VkPhysicalDevice> physicalDevices = getAllPhysicalDevices();
			return PhysicalDevice(physicalDevices.at(physicalDeviceIndex));
		}

	};


	class Surface : public HandleWithOwner<VkSurfaceKHR, VkInstance> {

		static void destroyFunc(VkSurfaceKHR vkSurface, VkInstance vkInstance) {
			if (vkSurface && vkInstance) {
				vkDestroySurfaceKHR(vkInstance, vkSurface, nullptr);
			}
		}

		Surface(VkSurfaceKHR vkSurface, VkInstance vkInstance, DestroyFunc_t pfnDestroy)
			: HandleWithOwner(vkSurface, vkInstance, pfnDestroy) {
		}

	public:

		Surface() {}
		Surface(const VkWin32SurfaceCreateInfoKHR& vkWin32SurfaceCreateInfo, VkInstance vkInstance) {
			VkSurfaceKHR vkSurface;
			VkResult vkResult = vkCreateWin32SurfaceKHR(vkInstance, &vkWin32SurfaceCreateInfo, nullptr, &vkSurface);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			new(this)Surface(vkSurface, vkInstance, &destroyFunc);
		}

		VkSurfaceCapabilitiesKHR getSurfaceCapabilities(VkPhysicalDevice vkPhysicalDevice) const {

			VkSurfaceCapabilitiesKHR vkSurfaceCapabilities;
			VkResult vkResult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
				vkPhysicalDevice,
				m_handle,
				&vkSurfaceCapabilities
			);
			if (vkResult == VK_ERROR_UNKNOWN) {
				throw ShutdownException();
			}
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			return vkSurfaceCapabilities;
		}

		std::vector<VkSurfaceFormatKHR> getSurfaceFormats(VkPhysicalDevice vkPhysicalDevice) {
			uint32_t	formatCount;
			VkResult vkResult = vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, *this, &formatCount, nullptr);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
			vkResult = vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, *this, &formatCount, surfaceFormats.data());
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			return surfaceFormats;
		}

		std::vector<VkPresentModeKHR> getSurfacePresentModes(VkPhysicalDevice vkPhysicalDevice) {
			uint32_t presentModeCount;
			VkResult vkResult = vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, *this, &presentModeCount, nullptr);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			std::vector<VkPresentModeKHR> presentModes;
			vkResult = vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, *this, &presentModeCount, presentModes.data());
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			return presentModes;
		}

	};


	class DeviceQueueCreateInfo : public VkDeviceQueueCreateInfo {

		static inline const float s_queuePriority = 1.0f;

	public:
		DeviceQueueCreateInfo(const DeviceQueueCreateInfo&) = delete;
		DeviceQueueCreateInfo& operator=(const DeviceQueueCreateInfo&) = delete;
		DeviceQueueCreateInfo(DeviceQueueCreateInfo&&) = delete;
		DeviceQueueCreateInfo& operator=(DeviceQueueCreateInfo&&) = delete;

		DeviceQueueCreateInfo() : VkDeviceQueueCreateInfo{} {

			sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

			queueFamilyIndex = 0;
			queueCount = 1;
			pQueuePriorities = &s_queuePriority;

		}

	};


	class DeviceCreateInfo : public VkDeviceCreateInfo {

		std::vector<std::string>	m_layerNames{};
		std::vector<std::string>	m_extensionNames{};

		std::vector<const char*>	m_layerStrings{};
		std::vector<const char*>	m_extensionStrings{};

	public:

		DeviceCreateInfo(const DeviceCreateInfo&) = delete;
		DeviceCreateInfo& operator=(const DeviceCreateInfo&) = delete;
		DeviceCreateInfo(DeviceCreateInfo&&) = delete;
		DeviceCreateInfo& operator=(DeviceCreateInfo&&) = delete;

		DeviceCreateInfo() : VkDeviceCreateInfo{} {
			sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		}

		DeviceCreateInfo& construct() {

			m_layerStrings.clear();
			ppEnabledLayerNames = nullptr;
			for (const std::string& layerName : m_layerNames) {
				m_layerStrings.push_back(layerName.c_str());
			}
			enabledLayerCount = static_cast<uint32_t>(m_layerNames.size());
			if (enabledLayerCount > 0) {
				ppEnabledLayerNames = m_layerStrings.data();
			}

			m_extensionStrings.clear();
			ppEnabledExtensionNames = nullptr;
			for (const std::string& extensionName : m_extensionNames) {
				m_extensionStrings.push_back(extensionName.c_str());
			}
			enabledExtensionCount = static_cast<uint32_t>(m_extensionNames.size());
			if (enabledExtensionCount > 0) {
				ppEnabledExtensionNames = m_extensionStrings.data();
			}

			return *this;
		}

		void addLayer(const char* layerName) {
			m_layerNames.push_back(layerName);
		}

		void addExtension(const char* extensionName) {
			m_extensionNames.push_back(extensionName);
		}


	};

	class Device : public HandleWithOwner<VkDevice, VkPhysicalDevice> {


		Device(VkDevice vkDevice, VkPhysicalDevice vkPhysicalDevice, DestroyFunc_t pfnDestroy)
			: HandleWithOwner(vkDevice, vkPhysicalDevice, pfnDestroy) {
		}

		static void destroy(VkDevice vkDevice, VkPhysicalDevice) {
			if (vkDevice) {
				vkDestroyDevice(vkDevice, nullptr);
			}
		}


	public:

		Device() {}
		Device(DeviceCreateInfo& deviceCreateInfo, PhysicalDevice physicalDevice) {
			VkDevice vkDevice;
			VkResult vkResult = vkCreateDevice(physicalDevice, &deviceCreateInfo.construct(), nullptr, &vkDevice);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			new(this) Device(vkDevice, physicalDevice, &destroy);
		}

		PhysicalDevice getPhysicalDevice() {
			return PhysicalDevice(getOwner());
		}

		VkQueue getDeviceQueue(int deviceQueueFamily, int deviceQueueIndex) const {
			VkQueue vkQueue;
			vkGetDeviceQueue(m_handle, deviceQueueFamily, deviceQueueIndex, &vkQueue);
			if (vkQueue == nullptr) {
				throw Exception(VK_ERROR_UNKNOWN);
			}
			return vkQueue;
		}


		std::vector<VkImageView> createImageViews(
			std::vector<VkImage>& images,
			VkImageViewCreateInfo* vkImageViewCreateInfo
		) const {
			std::vector<VkImageView> imageViews(images.size());
			int imageIndex = 0;
			for (VkImage imageIn : images) {
				vkImageViewCreateInfo->image = imageIn;
				vkCreateImageView(m_handle, vkImageViewCreateInfo, nullptr, &imageViews[imageIndex]);
				++imageIndex;
			}
			return imageViews;
		}


		uint32_t findMemoryTypeIndex(uint32_t usableMemoryIndexBits, VkMemoryPropertyFlags requiredProperties) {
			return getPhysicalDevice().findMemoryTypeIndex(usableMemoryIndexBits, requiredProperties);
		}


	};


	class DeviceMemory : public HandleWithOwner<VkDeviceMemory> {

		DeviceMemory(VkDeviceMemory vkDeviceMemory, VkDevice vkDevice, DestroyFunc_t pfnDestroy)
			: HandleWithOwner(vkDeviceMemory, vkDevice, pfnDestroy) {
		}

		static void destroy(VkDeviceMemory vkDeviceMemory, VkDevice vkDevice) {
			if (vkDeviceMemory && vkDevice) {
				vkFreeMemory(vkDevice, vkDeviceMemory, nullptr);
			}
		}

	public:

		DeviceMemory() {}
		DeviceMemory(const VkMemoryAllocateInfo& vkMemoryAllocateInfo, VkDevice vkDevice) {
			VkDeviceMemory vkDeviceMemory;
			VkResult vkResult = vkAllocateMemory(vkDevice, &vkMemoryAllocateInfo, nullptr, &vkDeviceMemory);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			new(this)DeviceMemory(vkDeviceMemory, vkDevice, &destroy);
		}

	};



	class Test : public HandleWithOwner<VkBuffer, Device> {

	public:
		Test() {}

	};

	class Buffer : public HandleWithOwner<VkBuffer, Device> {

		static void destroy(VkBuffer vkBuffer, Device device) {
			if (vkBuffer && device) {
				vkDestroyBuffer(device, vkBuffer, nullptr);
			}
		}

		Buffer(VkBuffer vkBuffer, Device device, DestroyFunc_t pfnDestroy)
			: HandleWithOwner(vkBuffer, device, pfnDestroy) {
		}

	public:

		Buffer() {}
		Buffer(VkBufferCreateInfo& vkBufferCreateInfo, Device device) {
			VkBuffer vkBuffer;
			VkResult vkResult = vkCreateBuffer(device, &vkBufferCreateInfo, nullptr, &vkBuffer);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			new(this)Buffer(vkBuffer, device, &destroy);
		}

		VkMemoryRequirements  getMemoryRequirements() {
			VkMemoryRequirements vkMemoryRequirements;
			vkGetBufferMemoryRequirements(getOwner(), *this, &vkMemoryRequirements);
			return vkMemoryRequirements;
		}

		DeviceMemory allocateDeviceMemory(VkMemoryPropertyFlags vkRequiredProperties) {
			VkMemoryRequirements vkMemoryRequirements = getMemoryRequirements();

			VkMemoryAllocateInfo vkMemoryAllocateInfo{};
			vkMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			vkMemoryAllocateInfo.allocationSize = vkMemoryRequirements.size;
			vkMemoryAllocateInfo.memoryTypeIndex =
				getOwner().findMemoryTypeIndex(vkMemoryRequirements.memoryTypeBits, vkRequiredProperties);
			return vkcpp::DeviceMemory(vkMemoryAllocateInfo, getOwner());
		}



	};


	class ShaderModule : public HandleWithOwner<VkShaderModule> {

		static std::vector<char> readFile(const std::string& filename) {
			std::ifstream file(filename, std::ios::ate | std::ios::binary);

			if (!file.is_open()) {
				throw std::runtime_error("failed to open file!");
			}
			size_t fileSize = (size_t)file.tellg();
			std::vector<char> buffer(fileSize);
			file.seekg(0);
			file.read(buffer.data(), fileSize);
			file.close();

			return buffer;
		}

		static void destroy(VkShaderModule vkShaderModule, VkDevice vkDevice) {
			if (vkShaderModule && vkDevice) {
				vkDestroyShaderModule(vkDevice, vkShaderModule, nullptr);
			}
		}

		ShaderModule(VkShaderModule vkShaderModule, VkDevice vkDevice, DestroyFunc_t pfnDestroy)
			: HandleWithOwner<VkShaderModule>(vkShaderModule, vkDevice, pfnDestroy) {
		}



	public:

		ShaderModule() {}

		static ShaderModule createShaderModuleFromFile(const char* fileName, VkDevice vkDevice) {
			auto fragShaderCode = readFile(fileName);
			VkShaderModuleCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = fragShaderCode.size();
			createInfo.pCode = reinterpret_cast<const uint32_t*>(fragShaderCode.data());
			VkShaderModule vkShaderModule;
			VkResult vkResult = vkCreateShaderModule(vkDevice, &createInfo, nullptr, &vkShaderModule);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			return ShaderModule(vkShaderModule, vkDevice, &destroy);
		}

	};


	class SwapChain : public HandleWithOwner<VkSwapchainKHR> {

		SwapChain(VkSwapchainKHR vkSwapChain, VkDevice vkDevice, DestroyFunc_t pfnDestroy, VkExtent2D vkSwapChainImageExtent)
			: HandleWithOwner(vkSwapChain, vkDevice, pfnDestroy)
			, m_vkSwapChainImageExtent(vkSwapChainImageExtent) {
		}

		static void destroy(VkSwapchainKHR vkSwapChain, VkDevice vkDevice) {
			if (vkSwapChain) {
				vkDestroySwapchainKHR(vkDevice, vkSwapChain, nullptr);
			}
		}

	public:

		SwapChain() {}
		SwapChain(VkSwapchainCreateInfoKHR* vkSwapChainCreateInfo, VkDevice vkDevice) {
			VkSwapchainKHR vkSwapChain;
			VkResult vkResult = vkCreateSwapchainKHR(vkDevice, vkSwapChainCreateInfo, nullptr, &vkSwapChain);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			new(this)SwapChain(vkSwapChain, vkDevice, &destroy, vkSwapChainCreateInfo->imageExtent);
		}

		VkExtent2D		m_vkSwapChainImageExtent = { .width = 0, .height = 0 };

		VkExtent2D imageExtent() const { return m_vkSwapChainImageExtent; }

		std::vector<VkImage> getImages() const {
			uint32_t swapChainImageCount;
			VkResult vkResult = vkGetSwapchainImagesKHR(getOwner(), *this, &swapChainImageCount, nullptr);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			std::vector<VkImage> swapChainImages(swapChainImageCount);
			vkResult = vkGetSwapchainImagesKHR(getOwner(), *this, &swapChainImageCount, swapChainImages.data());
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			return swapChainImages;
		}

	};


	class RenderPass : public HandleWithOwner<VkRenderPass> {

		RenderPass(VkRenderPass vkRenderPass, VkDevice vkDevice, DestroyFunc_t pfnDestroy)
			: HandleWithOwner(vkRenderPass, vkDevice, pfnDestroy) {
		}

		static void destroy(VkRenderPass vkRenderPass, VkDevice vkDevice) {
			if (vkRenderPass && vkDevice) {
				vkDestroyRenderPass(vkDevice, vkRenderPass, nullptr);
			}
		}

	public:

		RenderPass() {}
		RenderPass(VkRenderPassCreateInfo& vkRenderPassCreateInfo, VkDevice vkDevice) {
			VkRenderPass	vkRenderPass;
			VkResult vkResult = vkCreateRenderPass(vkDevice, &vkRenderPassCreateInfo, nullptr, &vkRenderPass);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			new(this)RenderPass(vkRenderPass, vkDevice, &destroy);
		}

	};


	class SwapChainImageViewsFrameBuffers {

	public:
		static inline	Device		s_device;

		//	Info needed to create and recreate actual instance info
		PhysicalDevice	m_physicalDevice;
		Surface	m_surface;
		RenderPass	m_renderPass;
		int				m_minImageCount = 0;
		VkFormat		m_vkFormat = VK_FORMAT_UNDEFINED;

		//	Actual instance info
		SwapChain	m_swapChainOriginal;

		std::vector<VkImageView>	m_swapChainImageViews;

		std::vector<VkFramebuffer>	m_swapChainFrameBuffers;

	private:

		void makeEmpty() {
			m_swapChainImageViews.clear();
			m_swapChainFrameBuffers.clear();
		}

	public:

		SwapChainImageViewsFrameBuffers() {}

		SwapChainImageViewsFrameBuffers(const SwapChainImageViewsFrameBuffers&) = delete;
		SwapChainImageViewsFrameBuffers& operator=(const SwapChainImageViewsFrameBuffers&) = delete;

		SwapChainImageViewsFrameBuffers(SwapChainImageViewsFrameBuffers&& other) noexcept
			: m_physicalDevice(std::move(other.m_physicalDevice))
			, m_surface(std::move(other.m_surface))
			, m_renderPass(std::move(other.m_renderPass))
			, m_minImageCount(std::move(other.m_minImageCount))
			, m_vkFormat(std::move(other.m_vkFormat))
			, m_swapChainOriginal(std::move(other.m_swapChainOriginal))
			, m_swapChainImageViews(std::move(other.m_swapChainImageViews))
			, m_swapChainFrameBuffers(std::move(other.m_swapChainFrameBuffers)) {
			other.makeEmpty();
		}

		SwapChainImageViewsFrameBuffers& operator=(SwapChainImageViewsFrameBuffers&& other) noexcept {
			if (this == &other) {
				return *this;
			}
			(*this).~SwapChainImageViewsFrameBuffers();
			new(this) SwapChainImageViewsFrameBuffers(std::move(other));
			other.makeEmpty();
			return *this;
		}


		void destroyFrameBuffers() {
			for (VkFramebuffer vkFrameBuffer : m_swapChainFrameBuffers) {
				vkDestroyFramebuffer(s_device, vkFrameBuffer, nullptr);
			}
			m_swapChainFrameBuffers.clear();
		}

		void destroyImageViews() {
			for (VkImageView vkImageView : m_swapChainImageViews) {
				vkDestroyImageView(s_device, vkImageView, nullptr);
			}
			m_swapChainImageViews.clear();
		}

		void destroy() {
			if (m_swapChainOriginal) {
				vkDeviceWaitIdle(s_device);
				destroyFrameBuffers();
				destroyImageViews();
			}
		}

		static 	std::vector<VkImageView> createSwapChainImageViews(
			const SwapChain& swapChain,
			VkFormat			swapChainImageFormat
		) {
			std::vector<VkImage> swapChainImages = swapChain.getImages();
			VkImageViewCreateInfo imageViewCreateInfo{};
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewCreateInfo.format = swapChainImageFormat;
			imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
			imageViewCreateInfo.subresourceRange.levelCount = 1;
			imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			imageViewCreateInfo.subresourceRange.layerCount = 1;
			return s_device.createImageViews(swapChainImages, &imageViewCreateInfo);
		}


		static std::vector<VkFramebuffer> createSwapChainFrameBuffers(
			std::vector<VkImageView> swapChainImageViews,
			VkExtent2D	swapChainExtent,
			VkRenderPass	vkRenderPass) {
			std::vector<VkFramebuffer>	swapChainFrameBuffers;

			swapChainFrameBuffers.resize(swapChainImageViews.size());
			for (size_t i = 0; i < swapChainImageViews.size(); i++) {
				VkImageView attachments[] = {
					swapChainImageViews[i]
				};

				VkFramebufferCreateInfo framebufferInfo{};
				framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				framebufferInfo.renderPass = vkRenderPass;
				framebufferInfo.attachmentCount = 1;
				framebufferInfo.pAttachments = attachments;
				framebufferInfo.width = swapChainExtent.width;
				framebufferInfo.height = swapChainExtent.height;
				framebufferInfo.layers = 1;

				if (vkCreateFramebuffer(s_device, &framebufferInfo, nullptr, &swapChainFrameBuffers[i]) != VK_SUCCESS) {
					throw std::runtime_error("failed to create framebuffer!");
				}
			}
			return swapChainFrameBuffers;
		}

		static SwapChain createSwapChain(
			Surface surface,
			VkExtent2D						swapChainExtent,
			uint32_t						swapChainMinImageCount,
			VkFormat						swapChainImageFormat,
			VkSurfaceTransformFlagBitsKHR	swapChainPreTransform
		) {
			const VkColorSpaceKHR swapChainImageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
			const VkPresentModeKHR swapChainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

			VkSwapchainCreateInfoKHR swapChainCreateInfo{};
			swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			swapChainCreateInfo.surface = surface;
			swapChainCreateInfo.minImageCount = swapChainMinImageCount;
			swapChainCreateInfo.imageFormat = swapChainImageFormat;
			swapChainCreateInfo.imageColorSpace = swapChainImageColorSpace;
			swapChainCreateInfo.imageExtent = swapChainExtent;
			swapChainCreateInfo.imageArrayLayers = 1;
			swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			swapChainCreateInfo.queueFamilyIndexCount = 0;
			swapChainCreateInfo.pQueueFamilyIndices = nullptr;
			swapChainCreateInfo.preTransform = swapChainPreTransform;
			swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			swapChainCreateInfo.presentMode = swapChainPresentMode;
			swapChainCreateInfo.clipped = VK_TRUE;
			swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

			return SwapChain(&swapChainCreateInfo, s_device);
		}


	public:

		static void setDevice(Device device) {
			s_device = device;
		}

		~SwapChainImageViewsFrameBuffers() {
			destroy();
		}

		operator bool() {
			return m_swapChainOriginal != nullptr;
		}

		VkSwapchainKHR vkSwapchain() {
			VkSwapchainKHR vkSwapChain = m_swapChainOriginal;
			return vkSwapChain;
		}

		bool canDraw() {
			VkSwapchainKHR vkSwapChain = m_swapChainOriginal;
			return vkSwapChain != nullptr;
		}

		VkExtent2D getImageExtent() const {
			return m_swapChainOriginal.imageExtent();
		}

		RenderPass getRenderPass() const {
			return m_renderPass;
		}

		VkFramebuffer getFrameBuffer(int index) {
			return m_swapChainFrameBuffers.at(index);
		}

		void setPhysicalDevice(PhysicalDevice physicalDevice) {
			m_physicalDevice = physicalDevice;
		}

		void setSurface(Surface surface) {
			m_surface = surface;
		}

		void setMinImageCount(int minImageCount) {
			m_minImageCount = minImageCount;
		}

		void setRenderPass(RenderPass renderPass) {
			m_renderPass = renderPass;
		}

		void setFormat(VkFormat vkFormat) {
			m_vkFormat = vkFormat;
		}

		void recreateSwapChainImageViewsFrameBuffers() {

			if (!s_device) {
				return;
			}

			vkDeviceWaitIdle(s_device);
			destroyFrameBuffers();
			destroyImageViews();


			VkSurfaceCapabilitiesKHR vkSurfaceCapabilities = m_surface.getSurfaceCapabilities(m_physicalDevice);
			VkExtent2D swapChainExtent = vkSurfaceCapabilities.currentExtent;
			if (swapChainExtent.width == 0 || swapChainExtent.height == 0) {
				return;
			}

			const VkSurfaceTransformFlagBitsKHR swapChainPreTransform = vkSurfaceCapabilities.currentTransform;

			m_swapChainOriginal = std::move(createSwapChain(
				m_surface, swapChainExtent, m_minImageCount, m_vkFormat, swapChainPreTransform));
			m_swapChainImageViews = createSwapChainImageViews(m_swapChainOriginal, m_vkFormat);
			m_swapChainFrameBuffers = createSwapChainFrameBuffers(m_swapChainImageViews, swapChainExtent, m_renderPass);

		}

	};


	class CommandPool : public HandleWithOwner<VkCommandPool> {

		CommandPool(VkCommandPool vkCommandPool, VkDevice vkDevice, DestroyFunc_t pfnDestroy)
			: HandleWithOwner(vkCommandPool, vkDevice, pfnDestroy) {
		}

		static void destroy(VkCommandPool vkCommandPool, VkDevice vkDevice) {
			if (vkCommandPool && vkDevice) {
				vkDestroyCommandPool(vkDevice, vkCommandPool, nullptr);
			}
		}


	public:

		CommandPool() {}
		CommandPool(const VkCommandPoolCreateInfo& commandPoolCreateInfo, VkDevice vkDevice) {
			VkCommandPool vkCommandPool;
			VkResult vkResult = vkCreateCommandPool(
				vkDevice,
				&commandPoolCreateInfo,
				nullptr,
				&vkCommandPool);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			new(this)CommandPool(vkCommandPool, vkDevice, &destroy);
		}

		std::vector<VkCommandBuffer> allocateCommandBuffers(
			VkCommandBufferAllocateInfo& vkCommandBufferAllocateInfo
		) const {
			std::vector<VkCommandBuffer> commandBuffers(vkCommandBufferAllocateInfo.commandBufferCount);
			VkResult vkResult = vkAllocateCommandBuffers(m_owner, &vkCommandBufferAllocateInfo, commandBuffers.data());
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			return commandBuffers;
		}



	};

	//class CommandBuffer : public HandleWithOwner<VkCommandBuffer> {

	//	CommandBuffer(VkCommandBuffer vkCommandBuffer, VkDevice vkDevice)
	//		: HandleWithOwner(vkDevice, vkCommandBuffer) {
	//	}

	//	void destroy(VkCommandBuffer vkCommandBuffer, VkDevice vkDevice) {
	//		if (vkCommandBuffer) {
	//			vkFreeCommandBuffers(vkDevice, 1, &)
	//		}
	//	}

	//public:

	//	CommandBuffer() {}

	//	CommandBu
	//};

	class DescriptorPool : public HandleWithOwner<VkDescriptorPool> {

		DescriptorPool(VkDescriptorPool vkDescriptorPool, VkDevice vkDevice, DestroyFunc_t pfnDestroy)
			: HandleWithOwner(vkDescriptorPool, vkDevice, pfnDestroy) {
		}

		static void destroy(VkDescriptorPool vkDescriptorPool, VkDevice vkDevice) {
			if (vkDescriptorPool && vkDevice) {
				vkDestroyDescriptorPool(vkDevice, vkDescriptorPool, nullptr);
			}
		}


	public:

		DescriptorPool() {}
		DescriptorPool(const VkDescriptorPoolCreateInfo& poolCreateInfo, VkDevice vkDevice) {
			VkDescriptorPool vkDescriptorPool;
			VkResult vkResult = vkCreateDescriptorPool(vkDevice, &poolCreateInfo, nullptr, &vkDescriptorPool);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			new(this)DescriptorPool(vkDescriptorPool, vkDevice, &destroy);
		}

	};


	class DescriptorSetLayout : public HandleWithOwner<VkDescriptorSetLayout> {

		DescriptorSetLayout(VkDescriptorSetLayout vkDescriptorSetLayout, VkDevice vkDevice, DestroyFunc_t pfnDestroy)
			: HandleWithOwner(vkDescriptorSetLayout, vkDevice, pfnDestroy) {
		}

		static void destroy(VkDescriptorSetLayout vkDescriptorSetLayout, VkDevice vkDevice) {
			if (vkDescriptorSetLayout && vkDevice) {
				vkDestroyDescriptorSetLayout(vkDevice, vkDescriptorSetLayout, nullptr);
			}
		}

	public:

		DescriptorSetLayout() {}
		DescriptorSetLayout(VkDescriptorSetLayoutCreateInfo& descriptorSetLayoutCreateInfo, VkDevice vkDevice) {
			VkDescriptorSetLayout vkDescriptorSetLayout;
			VkResult vkResult = vkCreateDescriptorSetLayout(vkDevice, &descriptorSetLayoutCreateInfo, nullptr, &vkDescriptorSetLayout);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			new(this)DescriptorSetLayout(vkDescriptorSetLayout, vkDevice, &destroy);
		}


	};


	class Semaphore : public HandleWithOwner<VkSemaphore> {

		Semaphore(VkSemaphore vkSemaphore, VkDevice vkDevice, DestroyFunc_t pfnDestroy)
			: HandleWithOwner(vkSemaphore, vkDevice, pfnDestroy) {
		}

		static void destroy(VkSemaphore vkSemaphore, VkDevice vkDevice) {
			if (vkSemaphore && vkDevice) {
				vkDestroySemaphore(vkDevice, vkSemaphore, nullptr);
			}
		}

	public:

		Semaphore() {}
		Semaphore(const VkSemaphoreCreateInfo& vkSemaphoreCreateInfo, VkDevice vkDevice) {
			VkSemaphore vkSemaphore;
			VkResult vkResult = vkCreateSemaphore(vkDevice, &vkSemaphoreCreateInfo, nullptr, &vkSemaphore);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			new(this)Semaphore(vkSemaphore, vkDevice, &destroy);
		}
	};


	class Fence : public HandleWithOwner<VkFence> {

		Fence(VkFence vkFence, VkDevice vkDevice, DestroyFunc_t pfnDestroy)
			: HandleWithOwner(vkFence, vkDevice, pfnDestroy) {
		}

		static void destroy(VkFence vkFence, VkDevice vkDevice) {
			if (vkFence && vkDevice) {
				vkDestroyFence(vkDevice, vkFence, nullptr);
			}
		}

	public:

		Fence() {}
		Fence(const VkFenceCreateInfo& vkFenceCreateInfo, VkDevice vkDevice) {
			VkFence vkFence;
			VkResult vkResult = vkCreateFence(vkDevice, &vkFenceCreateInfo, nullptr, &vkFence);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			new(this) Fence(vkFence, vkDevice, &destroy);
		}

		void reset() {
			VkFence vkFence = *this;
			vkResetFences(m_owner, 1, &vkFence);
		}

		void wait() {
			VkFence vkFence = *this;
			vkWaitForFences(m_owner, 1, &vkFence, VK_TRUE, UINT64_MAX);
		}

	};


	class PipelineLayout : public HandleWithOwner<VkPipelineLayout> {

		PipelineLayout(VkPipelineLayout vkPipelineLayout, VkDevice vkDevice, DestroyFunc_t pfnDestroy)
			: HandleWithOwner(vkPipelineLayout, vkDevice, pfnDestroy) {
		}

		static void destroy(VkPipelineLayout vkPipelineLayout, VkDevice vkDevice) {
			if (vkPipelineLayout) {
				vkDestroyPipelineLayout(vkDevice, vkPipelineLayout, nullptr);
			}
		}

	public:

		PipelineLayout() {}
		PipelineLayout(VkPipelineLayoutCreateInfo& pipelineLayoutCreateInfo, VkDevice vkDevice) {
			VkPipelineLayout vkPipelineLayout;
			VkResult vkResult = vkCreatePipelineLayout(vkDevice, &pipelineLayoutCreateInfo, nullptr, &vkPipelineLayout);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			new(this)PipelineLayout(vkPipelineLayout, vkDevice, &destroy);
		}


	};


	class GraphicsPipeline : public HandleWithOwner<VkPipeline> {

		GraphicsPipeline(VkPipeline vkPipeline, VkDevice vkDevice, DestroyFunc_t pfnDestroy)
			: HandleWithOwner(vkPipeline, vkDevice, pfnDestroy) {
		}

		static void destroy(VkPipeline vkPipeline, VkDevice vkDevice) {
			if (vkPipeline) {
				vkDestroyPipeline(vkDevice, vkPipeline, nullptr);
			}
		}

	public:

		GraphicsPipeline() {}
		GraphicsPipeline(VkGraphicsPipelineCreateInfo& pipelineCreateInfo, VkDevice vkDevice) {
			VkPipeline vkPipeline;
			VkResult vkResult = vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &vkPipeline);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			new(this)GraphicsPipeline(vkPipeline, vkDevice, &destroy);
		}


	};


	class Image : public HandleWithOwner<VkImage> {

		Image(VkImage vkImage, VkDevice vkDevice, DestroyFunc_t pfnDestroy)
			: HandleWithOwner(vkImage, vkDevice, pfnDestroy) {

		}

		static void destroy(VkImage vkImage, VkDevice vkDevice) {
			if (vkImage) {
				vkDestroyImage(vkDevice, vkImage, nullptr);
			}
		}

	public:
		Image() {}

		Image(const VkImageCreateInfo& vkImageCreateInfo, VkDevice vkDevice) {
			VkImage vkImage;
			VkResult vkResult = vkCreateImage(vkDevice, &vkImageCreateInfo, nullptr, &vkImage);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			new(this) Image(vkImage, vkDevice, &destroy);
		}
	};



}




