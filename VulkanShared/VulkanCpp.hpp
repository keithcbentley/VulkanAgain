

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

	class Device;

	template<typename HandleArg_t, typename OwnerArg_t = VkDevice>
	class HandleWithOwner {

	public:
		using Handle_t = HandleArg_t;
		using Owner_t = OwnerArg_t;
		using DestroyFunc_t = void (*)(HandleArg_t, OwnerArg_t);

	private:

		void destroy() {
			if (m_pfnDestroy && m_handle) {
				(*m_pfnDestroy)(m_handle, m_owner);
			}
			makeEmpty();
		}


	protected:

		Handle_t	m_handle{};
		Owner_t	m_owner{};
		DestroyFunc_t	m_pfnDestroy = nullptr;

		void makeEmpty() {
			m_handle = Handle_t{};
			m_owner = Owner_t{};
			m_pfnDestroy = nullptr;
		}


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
			requires std::same_as<Owner_t, VkDevice>
		|| std::same_as<Owner_t, Device> {
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


		PhysicalDevice(VkPhysicalDevice vkPhysicalDevice)
			:m_vkPhysicalDevice(vkPhysicalDevice) {}

		operator VkPhysicalDevice() const { return m_vkPhysicalDevice; }

		uint32_t findMemoryTypeIndex(
			uint32_t usableMemoryIndexBits,
			VkMemoryPropertyFlags requiredProperties) const {
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

		operator VkInstanceCreateInfo* () = delete;

		VulkanInstanceCreateInfo() : VkInstanceCreateInfo{} {
			sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		}

		VkInstanceCreateInfo& construct() {
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
			vkDestroyInstance(vkInstance, nullptr);
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

		PhysicalDevice	m_physicalDevice;

		static void destroyFunc(VkSurfaceKHR vkSurface, VkInstance vkInstance) {
			vkDestroySurfaceKHR(vkInstance, vkSurface, nullptr);
		}

		Surface(
			VkSurfaceKHR vkSurface,
			VkInstance vkInstance,
			PhysicalDevice physicalDevice,
			DestroyFunc_t pfnDestroy)
			: HandleWithOwner(vkSurface, vkInstance, pfnDestroy)
			, m_physicalDevice(physicalDevice) {
		}

	public:

		Surface() {}
		Surface(
			const VkWin32SurfaceCreateInfoKHR& vkWin32SurfaceCreateInfo,
			VkInstance vkInstance,
			PhysicalDevice physicalDevice
		) {
			VkSurfaceKHR vkSurface;
			VkResult vkResult = vkCreateWin32SurfaceKHR(vkInstance, &vkWin32SurfaceCreateInfo, nullptr, &vkSurface);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			new(this)Surface(vkSurface, vkInstance, physicalDevice, &destroyFunc);
		}

		VkSurfaceCapabilitiesKHR getSurfaceCapabilities() const {

			VkSurfaceCapabilitiesKHR vkSurfaceCapabilities;
			VkResult vkResult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
				m_physicalDevice,
				m_handle,
				&vkSurfaceCapabilities);
			if (vkResult == VK_ERROR_UNKNOWN) {
				throw ShutdownException();
			}
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			return vkSurfaceCapabilities;
		}

		std::vector<VkSurfaceFormatKHR> getSurfaceFormats() {
			uint32_t	formatCount;
			VkResult vkResult = vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, *this, &formatCount, nullptr);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
			vkResult = vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, *this, &formatCount, surfaceFormats.data());
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			return surfaceFormats;
		}

		std::vector<VkPresentModeKHR> getSurfacePresentModes() {
			uint32_t presentModeCount;
			VkResult vkResult = vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, *this, &presentModeCount, nullptr);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			std::vector<VkPresentModeKHR> presentModes;
			vkResult = vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, *this, &presentModeCount, presentModes.data());
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

		std::vector<std::string>	m_extensionNames{};

		std::vector<const char*>	m_extensionStrings{};

	public:

		DeviceCreateInfo(const DeviceCreateInfo&) = delete;
		DeviceCreateInfo& operator=(const DeviceCreateInfo&) = delete;
		DeviceCreateInfo(DeviceCreateInfo&&) = delete;
		DeviceCreateInfo& operator=(DeviceCreateInfo&&) = delete;

		operator DeviceCreateInfo* () = delete;

		DeviceCreateInfo() : VkDeviceCreateInfo{} {
			sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		}

		DeviceCreateInfo& construct() {

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


		void addExtension(const char* extensionName) {
			m_extensionNames.push_back(extensionName);
		}


	};

	class Device : public HandleWithOwner<VkDevice, VkPhysicalDevice> {


		Device(VkDevice vkDevice, VkPhysicalDevice vkPhysicalDevice, DestroyFunc_t pfnDestroy)
			: HandleWithOwner(vkDevice, vkPhysicalDevice, pfnDestroy) {
		}

		static void destroy(VkDevice vkDevice, VkPhysicalDevice) {
			vkDestroyDevice(vkDevice, nullptr);
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


		uint32_t findMemoryTypeIndex(uint32_t usableMemoryIndexBits, VkMemoryPropertyFlags requiredProperties) {
			return getPhysicalDevice().findMemoryTypeIndex(usableMemoryIndexBits, requiredProperties);
		}


	};


	class DeviceMemory : public HandleWithOwner<VkDeviceMemory> {

		DeviceMemory(VkDeviceMemory vkDeviceMemory, VkDevice vkDevice, DestroyFunc_t pfnDestroy)
			: HandleWithOwner(vkDeviceMemory, vkDevice, pfnDestroy) {
		}

		static void destroy(VkDeviceMemory vkDeviceMemory, VkDevice vkDevice) {
			vkFreeMemory(vkDevice, vkDeviceMemory, nullptr);
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


	class Buffer : public HandleWithOwner<VkBuffer, Device> {

		static void destroy(VkBuffer vkBuffer, Device device) {
			vkDestroyBuffer(device, vkBuffer, nullptr);
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
			vkGetBufferMemoryRequirements(getVkDevice(), *this, &vkMemoryRequirements);
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


	class BufferAndDeviceMemoryMapped {

		BufferAndDeviceMemoryMapped(Buffer&& buffer, DeviceMemory&& deviceMemory, void* mappedMemory)
			: m_buffer(std::move(buffer))
			, m_deviceMemory(std::move(deviceMemory))
			, m_mappedMemory(mappedMemory) {
		}

	public:

		Buffer			m_buffer;
		DeviceMemory	m_deviceMemory;
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


		BufferAndDeviceMemoryMapped(
			VkDeviceSize size,
			VkBufferUsageFlags usage,
			VkMemoryPropertyFlags properties,
			Device device
		) {
			VkBufferCreateInfo vkBufferCreateInfo{};
			vkBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			vkBufferCreateInfo.size = size;
			vkBufferCreateInfo.usage = usage;
			vkBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			vkcpp::Buffer buffer(vkBufferCreateInfo, device);
			DeviceMemory deviceMemory = buffer.allocateDeviceMemory(properties);

			VkResult vkResult = vkBindBufferMemory(device, buffer, deviceMemory, 0);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}

			// TODO: should this be a method on DeviceMemory?
			void* mappedMemory;
			vkMapMemory(device, deviceMemory, 0, size, 0, &mappedMemory);

			new(this) BufferAndDeviceMemoryMapped(std::move(buffer), std::move(deviceMemory), mappedMemory);

		}

		void unmapMemory() {
			vkUnmapMemory(m_deviceMemory.getVkDevice(), m_deviceMemory);
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
			vkDestroyShaderModule(vkDevice, vkShaderModule, nullptr);
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


	class Swapchain : public HandleWithOwner<VkSwapchainKHR, Device> {

		Swapchain(VkSwapchainKHR vkSwapchain, Device device, DestroyFunc_t pfnDestroy, VkExtent2D vkSwapchainImageExtent)
			: HandleWithOwner(vkSwapchain, device, pfnDestroy)
			, m_vkSwapchainImageExtent(vkSwapchainImageExtent) {
		}

		static void destroy(VkSwapchainKHR vkSwapchain, Device device) {
			vkDestroySwapchainKHR(device, vkSwapchain, nullptr);
		}

	public:

		Swapchain() {}
		Swapchain(VkSwapchainCreateInfoKHR* vkSwapchainCreateInfo, Device device) {
			VkSwapchainKHR vkSwapchain;
			VkResult vkResult = vkCreateSwapchainKHR(device, vkSwapchainCreateInfo, nullptr, &vkSwapchain);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			new(this)Swapchain(vkSwapchain, device, &destroy, vkSwapchainCreateInfo->imageExtent);
		}

		VkExtent2D		m_vkSwapchainImageExtent = { .width = 0, .height = 0 };

		VkExtent2D imageExtent() const { return m_vkSwapchainImageExtent; }

		std::vector<VkImage> getImages() const {
			uint32_t swapchainImageCount;
			VkResult vkResult = vkGetSwapchainImagesKHR(getVkDevice(), *this, &swapchainImageCount, nullptr);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			std::vector<VkImage> swapchainImages(swapchainImageCount);
			vkResult = vkGetSwapchainImagesKHR(getVkDevice(), *this, &swapchainImageCount, swapchainImages.data());
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			return swapchainImages;
		}

	};


	class RenderPass : public HandleWithOwner<VkRenderPass> {

		RenderPass(VkRenderPass vkRenderPass, VkDevice vkDevice, DestroyFunc_t pfnDestroy)
			: HandleWithOwner(vkRenderPass, vkDevice, pfnDestroy) {
		}

		static void destroy(VkRenderPass vkRenderPass, VkDevice vkDevice) {
			vkDestroyRenderPass(vkDevice, vkRenderPass, nullptr);
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


	class CommandPool : public HandleWithOwner<VkCommandPool> {

		CommandPool(VkCommandPool vkCommandPool, VkDevice vkDevice, DestroyFunc_t pfnDestroy)
			: HandleWithOwner(vkCommandPool, vkDevice, pfnDestroy) {
		}

		static void destroy(VkCommandPool vkCommandPool, VkDevice vkDevice) {
			vkDestroyCommandPool(vkDevice, vkCommandPool, nullptr);
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

	class CommandBuffer : public HandleWithOwner<VkCommandBuffer, CommandPool> {

		CommandBuffer(VkCommandBuffer vkCommandBuffer, CommandPool commandPool, DestroyFunc_t pfnDestroy)
			: HandleWithOwner(vkCommandBuffer, commandPool, pfnDestroy) {
		}

		static void destroy(VkCommandBuffer vkCommandBuffer, CommandPool commandPool) {
			vkFreeCommandBuffers(commandPool.getVkDevice(), commandPool, 1, &vkCommandBuffer);
		}

	public:

		CommandBuffer() {}

		CommandBuffer(CommandPool commandPool) {
			VkCommandBufferAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandPool = commandPool;
			allocInfo.commandBufferCount = 1;
			VkCommandBuffer vkCommandBuffer;
			vkAllocateCommandBuffers(commandPool.getVkDevice(), &allocInfo, &vkCommandBuffer);
			new(this) CommandBuffer(vkCommandBuffer, commandPool, &destroy);
		}

		void beginOneTimeSubmit() {
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			vkBeginCommandBuffer(*this, &beginInfo);
		}


	};

	class DescriptorPool : public HandleWithOwner<VkDescriptorPool> {

		DescriptorPool(VkDescriptorPool vkDescriptorPool, VkDevice vkDevice, DestroyFunc_t pfnDestroy)
			: HandleWithOwner(vkDescriptorPool, vkDevice, pfnDestroy) {
		}

		static void destroy(VkDescriptorPool vkDescriptorPool, VkDevice vkDevice) {
			vkDestroyDescriptorPool(vkDevice, vkDescriptorPool, nullptr);
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
			vkDestroyDescriptorSetLayout(vkDevice, vkDescriptorSetLayout, nullptr);
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
			vkDestroySemaphore(vkDevice, vkSemaphore, nullptr);
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
			vkDestroyFence(vkDevice, vkFence, nullptr);
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
			vkDestroyPipelineLayout(vkDevice, vkPipelineLayout, nullptr);
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
			vkDestroyPipeline(vkDevice, vkPipeline, nullptr);
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


	class Image : public HandleWithOwner<VkImage, Device> {

		Image(VkImage vkImage, Device device, DestroyFunc_t pfnDestroy)
			: HandleWithOwner(vkImage, device, pfnDestroy) {

		}

		static void destroy(VkImage vkImage, Device device) {
			vkDestroyImage(device, vkImage, nullptr);
		}

	public:
		Image() {}

		Image(const VkImageCreateInfo& vkImageCreateInfo, Device device) {
			VkImage vkImage;
			VkResult vkResult = vkCreateImage(device, &vkImageCreateInfo, nullptr, &vkImage);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			new(this) Image(vkImage, device, &destroy);
		}

		VkMemoryRequirements  getMemoryRequirements() {
			VkMemoryRequirements vkMemoryRequirements;
			vkGetImageMemoryRequirements(getVkDevice(), *this, &vkMemoryRequirements);
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

	class ImageView : public HandleWithOwner<VkImageView> {

		ImageView(VkImageView vkImageView, VkDevice vkDevice, DestroyFunc_t pfnDestroy)
			: HandleWithOwner(vkImageView, vkDevice, pfnDestroy) {

		}

		static void destroy(VkImageView vkImageView, VkDevice vkDevice) {
			vkDestroyImageView(vkDevice, vkImageView, nullptr);
		}

	public:
		ImageView() {}

		ImageView(const VkImageViewCreateInfo& vkImageViewCreateInfo, VkDevice vkDevice) {
			VkImageView vkImageView;
			VkResult vkResult = vkCreateImageView(vkDevice, &vkImageViewCreateInfo, nullptr, &vkImageView);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			new(this) ImageView(vkImageView, vkDevice, &destroy);
		}


		static std::vector<VkImageView> createVkImageViews(
			std::vector<VkImage>& vkImages,
			VkImageViewCreateInfo& vkImageViewCreateInfo,
			Device device) {
			std::vector<VkImageView> vkImageViews(vkImages.size());
			int index = 0;
			for (VkImage vkImage : vkImages) {
				vkImageViewCreateInfo.image = vkImage;
				vkCreateImageView(device, &vkImageViewCreateInfo, nullptr, &vkImageViews[index]);
				++index;
			}
			return vkImageViews;
		}

	};


	class ImageAndDeviceMemory {

		ImageAndDeviceMemory(Image&& image, DeviceMemory&& deviceMemory)
			: m_image(std::move(image))
			, m_deviceMemory(std::move(deviceMemory)) {
		}

	public:

		Image			m_image;
		DeviceMemory	m_deviceMemory;

		ImageAndDeviceMemory() {}

		ImageAndDeviceMemory(const ImageAndDeviceMemory&) = delete;
		ImageAndDeviceMemory& operator=(const ImageAndDeviceMemory&) = delete;

		ImageAndDeviceMemory(ImageAndDeviceMemory&& other) noexcept
			: m_image(std::move(other.m_image))
			, m_deviceMemory(std::move(other.m_deviceMemory)) {
		}

		ImageAndDeviceMemory& operator=(ImageAndDeviceMemory&& other) noexcept {
			if (this == &other) {
				return *this;
			}
			(*this).~ImageAndDeviceMemory();
			new(this) ImageAndDeviceMemory(std::move(other));
			return *this;
		}


		ImageAndDeviceMemory(
			const VkImageCreateInfo& vkImageCreateInfo,
			VkMemoryPropertyFlags properties,
			Device device
		) {
			vkcpp::Image image(vkImageCreateInfo, device);
			DeviceMemory deviceMemory = image.allocateDeviceMemory(properties);

			VkResult vkResult = vkBindImageMemory(device, image, deviceMemory, 0);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}

			new(this) ImageAndDeviceMemory(std::move(image), std::move(deviceMemory));

		}

	};



	class SwapchainImageViewsFrameBuffers {

	public:
		static inline	Device		s_device;

		VkSwapchainCreateInfoKHR	m_vkSwapchainCreateInfo;
		vkcpp::Surface				m_surface;

		RenderPass	m_renderPass;

		Swapchain	m_swapchainOriginal;
		std::vector<VkImageView>	m_swapchainImageViews;
		std::vector<VkFramebuffer>	m_swapchainFrameBuffers;

	private:

		void makeEmpty() {
			m_swapchainImageViews.clear();
			m_swapchainFrameBuffers.clear();
		}


		void destroyFrameBuffers() {
			for (VkFramebuffer vkFrameBuffer : m_swapchainFrameBuffers) {
				vkDestroyFramebuffer(s_device, vkFrameBuffer, nullptr);
			}
			m_swapchainFrameBuffers.clear();
		}

		void destroyImageViews() {
			for (VkImageView vkImageView : m_swapchainImageViews) {
				vkDestroyImageView(s_device, vkImageView, nullptr);
			}
			m_swapchainImageViews.clear();
		}

		void destroy() {
			if (!s_device) {
				return;
			}
			if (m_swapchainOriginal) {
				vkDeviceWaitIdle(s_device);
				destroyFrameBuffers();
				destroyImageViews();
			}
		}

		static 	std::vector<VkImageView> createSwapchainImageViews(
			Swapchain	swapchain,
			VkFormat	swapchainImageFormat
		) {
			std::vector<VkImage> swapchainImages = swapchain.getImages();
			VkImageViewCreateInfo imageViewCreateInfo{};
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewCreateInfo.format = swapchainImageFormat;
			imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
			imageViewCreateInfo.subresourceRange.levelCount = 1;
			imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			imageViewCreateInfo.subresourceRange.layerCount = 1;
			return ImageView::createVkImageViews(swapchainImages, imageViewCreateInfo, swapchain.getOwner());
		}


		static std::vector<VkFramebuffer> createSwapchainFrameBuffers(
			std::vector<VkImageView> swapchainImageViews,
			VkExtent2D	swapchainExtent,
			VkRenderPass	vkRenderPass) {
			std::vector<VkFramebuffer>	swapchainFrameBuffers;

			swapchainFrameBuffers.resize(swapchainImageViews.size());
			for (size_t i = 0; i < swapchainImageViews.size(); i++) {
				VkImageView attachments[] = {
					swapchainImageViews[i]
				};

				VkFramebufferCreateInfo framebufferInfo{};
				framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				framebufferInfo.renderPass = vkRenderPass;
				framebufferInfo.attachmentCount = 1;
				framebufferInfo.pAttachments = attachments;
				framebufferInfo.width = swapchainExtent.width;
				framebufferInfo.height = swapchainExtent.height;
				framebufferInfo.layers = 1;

				if (vkCreateFramebuffer(s_device, &framebufferInfo, nullptr, &swapchainFrameBuffers[i]) != VK_SUCCESS) {
					throw std::runtime_error("failed to create framebuffer!");
				}
			}
			return swapchainFrameBuffers;
		}


		static Swapchain createSwapchain(
			VkSwapchainCreateInfoKHR& vkSwapchainCreateInfo,
			Surface surface
		) {
			const VkSurfaceCapabilitiesKHR vkSurfaceCapabilities = surface.getSurfaceCapabilities();
			const VkExtent2D swapchainExtent = vkSurfaceCapabilities.currentExtent;
			if (swapchainExtent.width == 0 || swapchainExtent.height == 0) {
				return Swapchain{};
			}

			vkSwapchainCreateInfo.surface = surface;
			vkSwapchainCreateInfo.imageExtent = swapchainExtent;
			vkSwapchainCreateInfo.preTransform = vkSurfaceCapabilities.currentTransform;

			return Swapchain(&vkSwapchainCreateInfo, s_device);
		}




	public:

		SwapchainImageViewsFrameBuffers() {}
		~SwapchainImageViewsFrameBuffers() {
			destroy();
		}

		SwapchainImageViewsFrameBuffers(
			const VkSwapchainCreateInfoKHR& vkSwapchainCreateInfo,
			Surface surface)
			: m_vkSwapchainCreateInfo(vkSwapchainCreateInfo)
			, m_surface(surface) {

		}

		SwapchainImageViewsFrameBuffers(const SwapchainImageViewsFrameBuffers&) = delete;
		SwapchainImageViewsFrameBuffers& operator=(const SwapchainImageViewsFrameBuffers&) = delete;

		SwapchainImageViewsFrameBuffers(SwapchainImageViewsFrameBuffers&& other) noexcept
			: m_vkSwapchainCreateInfo(std::move(other.m_vkSwapchainCreateInfo))
			, m_surface(std::move(other.m_surface))
			, m_renderPass(std::move(other.m_renderPass))
			, m_swapchainOriginal(std::move(other.m_swapchainOriginal))
			, m_swapchainImageViews(std::move(other.m_swapchainImageViews))
			, m_swapchainFrameBuffers(std::move(other.m_swapchainFrameBuffers)) {
			other.makeEmpty();
		}

		SwapchainImageViewsFrameBuffers& operator=(SwapchainImageViewsFrameBuffers&& other) noexcept {
			if (this == &other) {
				return *this;
			}
			(*this).~SwapchainImageViewsFrameBuffers();
			new(this) SwapchainImageViewsFrameBuffers(std::move(other));
			other.makeEmpty();
			return *this;
		}


		static void setDevice(Device device) {
			s_device = device;
		}


		operator bool() {
			return m_swapchainOriginal != nullptr;
		}

		VkSwapchainKHR vkSwapchain() {
			VkSwapchainKHR vkSwapchain = m_swapchainOriginal;
			return vkSwapchain;
		}

		bool canDraw() {
			VkSwapchainKHR vkSwapchain = m_swapchainOriginal;
			return vkSwapchain != nullptr;
		}

		VkExtent2D getImageExtent() const {
			return m_swapchainOriginal.imageExtent();
		}

		RenderPass getRenderPass() const {
			return m_renderPass;
		}

		VkFramebuffer getFrameBuffer(int index) {
			return m_swapchainFrameBuffers.at(index);
		}


		void setRenderPass(RenderPass renderPass) {
			m_renderPass = renderPass;
		}

		void recreateSwapchainImageViewsFrameBuffers() {

			if (!s_device) {
				return;
			}

			vkDeviceWaitIdle(s_device);
			destroyFrameBuffers();
			destroyImageViews();

			m_swapchainOriginal = std::move(createSwapchain(m_vkSwapchainCreateInfo, m_surface));
			if (!m_swapchainOriginal) {
				return;
			}
			m_swapchainImageViews = createSwapchainImageViews(m_swapchainOriginal, m_vkSwapchainCreateInfo.imageFormat);
			m_swapchainFrameBuffers = createSwapchainFrameBuffers(
				m_swapchainImageViews,
				m_vkSwapchainCreateInfo.imageExtent,
				m_renderPass);
		}

	};



}




