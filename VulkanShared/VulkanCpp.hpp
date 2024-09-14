

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
concept BoolConvertible = requires (T t) {
	t.operator bool();
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


	template<typename Real_t, typename ActsLike_t>
		requires (sizeof(Real_t) == sizeof(ActsLike_t))
	ActsLike_t& wrapToRef(Real_t& real)
	{
		ActsLike_t* p = static_cast<ActsLike_t*>(&real);
		return *p;
	}

	class Extent2D : public VkExtent2D {};
	//	uint32_t    width;
	//	uint32_t    height;
	//} VkExtent2D;

	class Extent3D : public VkExtent3D {};
	//	uint32_t    width;
	//	uint32_t    height;
	//	uint32_t    depth;
	//} VkExtent3D;

	class Offset2D : public VkOffset2D {};
	//	int32_t    x;
	//	int32_t    y;
	//} VkOffset2D;

	class Offset3D : public VkOffset3D {};
	//	int32_t    x;
	//	int32_t    y;
	//	int32_t    z;
	//} VkOffset3D;

	class Rect2D : public VkRect2D {

	public:

		Rect2D()
			: VkRect2D{} {
		}

		Rect2D(VkOffset2D vkOffset2D, VkExtent2D vkExtent2D) {
			offset = vkOffset2D;
			extent = vkExtent2D;
		}

		Rect2D(VkExtent2D vkExtent2D) {
			offset.x = 0;
			offset.y = 0;
			extent = vkExtent2D;
		}



	};
	static_assert(sizeof(Rect2D) == sizeof(VkRect2D));
	template Rect2D& wrapToRef<VkRect2D, Rect2D>(VkRect2D&);



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
			VkMemoryPropertyFlags requiredProperties
		) const {
			VkPhysicalDeviceMemoryProperties memProperties;
			vkGetPhysicalDeviceMemoryProperties(m_vkPhysicalDevice, &memProperties);

			for (uint32_t index = 0; index < memProperties.memoryTypeCount; index++) {
				if ((usableMemoryIndexBits & (1 << index))
					&& (memProperties.memoryTypes[index].propertyFlags & requiredProperties) == requiredProperties) {
					return index;
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

		VkInstanceCreateInfo* operator& () = delete;

		VulkanInstanceCreateInfo() : VkInstanceCreateInfo{} {
			sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		}

		VkInstanceCreateInfo* assemble() {
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

			return this;

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
			VkResult vkResult = vkCreateInstance(vulkanInstanceCreateInfo.assemble(), nullptr, &vkInstance);
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

		DeviceCreateInfo* operator& () = delete;

		DeviceCreateInfo() : VkDeviceCreateInfo{} {
			sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		}

		VkDeviceCreateInfo* assemble() {

			m_extensionStrings.clear();
			ppEnabledExtensionNames = nullptr;
			for (const std::string& extensionName : m_extensionNames) {
				m_extensionStrings.push_back(extensionName.c_str());
			}
			enabledExtensionCount = static_cast<uint32_t>(m_extensionNames.size());
			if (enabledExtensionCount > 0) {
				ppEnabledExtensionNames = m_extensionStrings.data();
			}

			return this;
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
			VkResult vkResult = vkCreateDevice(physicalDevice, deviceCreateInfo.assemble(), nullptr, &vkDevice);
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


	class Buffer_DeviceMemory {

		Buffer_DeviceMemory(Buffer&& buffer, DeviceMemory&& deviceMemory, void* mappedMemory)
			: m_buffer(std::move(buffer))
			, m_deviceMemory(std::move(deviceMemory))
			, m_mappedMemory(mappedMemory) {
		}

	public:

		Buffer			m_buffer;
		DeviceMemory	m_deviceMemory;
		void* m_mappedMemory = nullptr;

		Buffer_DeviceMemory() {}

		Buffer_DeviceMemory(const Buffer_DeviceMemory&) = delete;
		Buffer_DeviceMemory& operator=(const Buffer_DeviceMemory&) = delete;

		Buffer_DeviceMemory(Buffer_DeviceMemory&& other) noexcept
			: m_buffer(std::move(other.m_buffer))
			, m_deviceMemory(std::move(other.m_deviceMemory))
			, m_mappedMemory(other.m_mappedMemory) {
		}

		Buffer_DeviceMemory& operator=(Buffer_DeviceMemory&& other) noexcept {
			if (this == &other) {
				return *this;
			}
			(*this).~Buffer_DeviceMemory();
			new(this) Buffer_DeviceMemory(std::move(other));
			return *this;
		}


		Buffer_DeviceMemory(
			VkBufferUsageFlags usage,
			VkMemoryPropertyFlags properties,
			int64_t size,
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

			new(this) Buffer_DeviceMemory(std::move(buffer), std::move(deviceMemory), mappedMemory);

		}

		Buffer_DeviceMemory(
			VkBufferUsageFlags usage,
			VkMemoryPropertyFlags properties,
			int64_t size,
			void* pSrcMem,
			Device device
		) {
			new(this)Buffer_DeviceMemory(
				usage,
				properties,
				size,
				device
			);
			memcpy(m_mappedMemory, pSrcMem, size);
			//unmapMemory();
		}


		void unmapMemory() {
			vkUnmapMemory(m_deviceMemory.getVkDevice(), m_deviceMemory);
			m_mappedMemory = nullptr;

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




	};

	class CommandBuffer : public HandleWithOwner<VkCommandBuffer, CommandPool> {

		static void destroy(VkCommandBuffer vkCommandBuffer, CommandPool commandPool) {
			vkFreeCommandBuffers(commandPool.getVkDevice(), commandPool, 1, &vkCommandBuffer);
		}

		CommandBuffer(
			VkCommandBuffer vkCommandBuffer,
			CommandPool commandPool,
			DestroyFunc_t pfnDestroy)
			: HandleWithOwner(vkCommandBuffer, commandPool, pfnDestroy) {
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

		static std::vector<CommandBuffer> allocateCommandBuffers(
			VkCommandBufferAllocateInfo& vkCommandBufferAllocateInfo,
			CommandPool	commandPool
		) {
			std::vector<VkCommandBuffer> vkCommandBuffers(vkCommandBufferAllocateInfo.commandBufferCount);
			VkResult vkResult = vkAllocateCommandBuffers(
				commandPool.getVkDevice(), &vkCommandBufferAllocateInfo, vkCommandBuffers.data());
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			std::vector<CommandBuffer> commandBuffers;
			for (VkCommandBuffer vkCommandBuffer : vkCommandBuffers) {
				//	Can't use emplace_back since full constructor is private.
				commandBuffers.push_back(std::move(CommandBuffer(vkCommandBuffer, commandPool, &destroy)));
			}
			return commandBuffers;
		}

		void reset() {
			VkResult vkResult = vkResetCommandBuffer(*this, 0);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
		}

		void begin() {
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

			VkResult vkResult = vkBeginCommandBuffer(*this, &beginInfo);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
		}


		void beginOneTimeSubmit() {
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			vkBeginCommandBuffer(*this, &beginInfo);
		}

		void end() {
			VkResult vkResult = vkEndCommandBuffer(*this);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}

		}

	};


	class DescriptorPoolCreateInfo : public VkDescriptorPoolCreateInfo {

		//	The map allows us to collect the size info in any order.
		//	When it's time to assemble, we'll put the collected info
		//	into the vector so that it's ready for the create call.
		std::map<VkDescriptorType, int>	m_poolSizesMap;
		std::vector<VkDescriptorPoolSize> m_poolSizes;

	public:

		//	Don't allow getting the pointer to the raw create info structure
		//	since it may not be assembled.  Must call assemble to get the pointer.
		VkDescriptorPoolCreateInfo* operator&() = delete;

		DescriptorPoolCreateInfo()
			: VkDescriptorPoolCreateInfo{} {
			sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		}

		void addDescriptorCount(VkDescriptorType vkDescriptorType, int count) {
			m_poolSizesMap[vkDescriptorType] += count;
		}

		VkDescriptorPoolCreateInfo* assemble() {
			m_poolSizes.clear();
			VkDescriptorPoolSize vkDescriptorPoolSize;
			for (std::pair<VkDescriptorType, int> kv : m_poolSizesMap) {
				vkDescriptorPoolSize.type = kv.first;
				vkDescriptorPoolSize.descriptorCount = kv.second;
				m_poolSizes.push_back(vkDescriptorPoolSize);
			}
			poolSizeCount = static_cast<uint32_t>(m_poolSizes.size());
			pPoolSizes = m_poolSizes.data();

			return this;
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
		DescriptorPool(DescriptorPoolCreateInfo& poolCreateInfo, VkDevice vkDevice) {
			VkDescriptorPool vkDescriptorPool;
			VkResult vkResult = vkCreateDescriptorPool(vkDevice, poolCreateInfo.assemble(), nullptr, &vkDescriptorPool);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			new(this)DescriptorPool(vkDescriptorPool, vkDevice, &destroy);
		}

	};

	class DescriptorSetLayoutCreateInfo : public VkDescriptorSetLayoutCreateInfo {

		std::vector<VkDescriptorSetLayoutBinding>	m_bindings;


	public:

		VkDescriptorSetLayoutCreateInfo* operator&() = delete;

		DescriptorSetLayoutCreateInfo()
			: VkDescriptorSetLayoutCreateInfo{} {
			sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		}

		DescriptorSetLayoutCreateInfo& addBinding(
			int bindingIndex,
			VkDescriptorType	vkDescriptorType,
			VkShaderStageFlags	vkShaderStageFlags
		) {
			VkDescriptorSetLayoutBinding layoutBinding{};
			layoutBinding.binding = bindingIndex;
			layoutBinding.descriptorType = vkDescriptorType;
			layoutBinding.descriptorCount = 1;
			layoutBinding.stageFlags = vkShaderStageFlags;
			layoutBinding.pImmutableSamplers = nullptr;

			m_bindings.push_back(layoutBinding);
			return *this;
		}

		VkDescriptorSetLayoutCreateInfo* assemble() {
			bindingCount = static_cast<uint32_t>(m_bindings.size());
			pBindings = m_bindings.data();
			return this;
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
		DescriptorSetLayout(DescriptorSetLayoutCreateInfo& descriptorSetLayoutCreateInfo, VkDevice vkDevice) {
			VkDescriptorSetLayout vkDescriptorSetLayout;
			VkResult vkResult = vkCreateDescriptorSetLayout(
				vkDevice,
				descriptorSetLayoutCreateInfo.assemble(),
				nullptr,
				&vkDescriptorSetLayout);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			new(this)DescriptorSetLayout(vkDescriptorSetLayout, vkDevice, &destroy);
		}


	};

	class DescriptorSet : public HandleWithOwner<VkDescriptorSet, DescriptorPool> {

		DescriptorSet(VkDescriptorSet vkDescriptorSet, DescriptorPool descriptorPool, DestroyFunc_t pfnDestroy)
			: HandleWithOwner(vkDescriptorSet, descriptorPool, pfnDestroy) {
		}

		static void destroy(VkDescriptorSet vkDescriptorSet, DescriptorPool descriptorPool) {
			vkFreeDescriptorSets(descriptorPool.getVkDevice(), descriptorPool, 1, &vkDescriptorSet);
		}

	public:

		DescriptorSet() {}

		DescriptorSet(DescriptorSetLayout descriptorSetLayout, DescriptorPool descriptorPool) {

			VkDescriptorSetLayout vkDescriptorSetLayout = descriptorSetLayout;
			VkDescriptorSetAllocateInfo vkDescriptorSetAllocateInfo{};
			vkDescriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			vkDescriptorSetAllocateInfo.descriptorPool = descriptorPool;
			vkDescriptorSetAllocateInfo.descriptorSetCount = 1;
			vkDescriptorSetAllocateInfo.pSetLayouts = &vkDescriptorSetLayout;

			VkDescriptorSet vkDescriptorSet;
			VkResult vkResult = vkAllocateDescriptorSets(
				descriptorPool.getVkDevice(),
				&vkDescriptorSetAllocateInfo,
				&vkDescriptorSet);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			new(this)DescriptorSet(vkDescriptorSet, descriptorPool, &destroy);
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

	class ImageCreateInfo : public VkImageCreateInfo {

	public:

		ImageCreateInfo(
			VkFormat vkFormat,
			VkImageUsageFlags vkUsage)
			: VkImageCreateInfo{} {
			sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			format = vkFormat;
			usage = vkUsage;

			imageType = VK_IMAGE_TYPE_2D;

			extent.depth = 1;
			mipLevels = 1;
			arrayLayers = 1;
			initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			samples = VK_SAMPLE_COUNT_1_BIT;
			sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		}

		ImageCreateInfo& setExtent(VkExtent2D  vkExtent2D) {
			extent.width = vkExtent2D.width;
			extent.height = vkExtent2D.height;
			return *this;
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

		Image(const ImageCreateInfo& imageCreateInfo, Device device) {
			VkImage vkImage;
			VkResult vkResult = vkCreateImage(device, &imageCreateInfo, nullptr, &vkImage);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			new(this) Image(vkImage, device, &destroy);
		}

		//static Image fromExisting(VkImage vkImage, Device device) {
		//	return Image(vkImage, device, nullptr);
		//}

		//static std::vector<Image> fromExisting(std::vector<VkImage>& vkImages, Device device) {
		//	std::vector<Image> images;
		//	for (VkImage vkImage : vkImages) {
		//		images.push_back(fromExisting(vkImage, device));
		//	}
		//	return images;
		//}

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

	class ImageViewCreateInfo : public VkImageViewCreateInfo {

	public:
		ImageViewCreateInfo(
			VkImageViewType vkImageViewType,
			VkFormat	vkFormat,
			VkImageAspectFlags aspectFlags)
			: VkImageViewCreateInfo{} {
			sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewType = vkImageViewType;
			format = vkFormat;
			subresourceRange.aspectMask = aspectFlags;
			components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.baseArrayLayer = 0;
			subresourceRange.layerCount = 1;
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


		static std::vector<ImageView> createImageViews(
			std::vector<VkImage>& vkImages,
			ImageViewCreateInfo& imageViewCreateInfo,
			Device device
		) {
			std::vector<ImageView> imageViews;
			for (VkImage vkImage : vkImages) {
				imageViewCreateInfo.image = vkImage;
				imageViews.emplace_back(imageViewCreateInfo, device);
			}
			return imageViews;
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

		VkExtent2D		m_vkSwapchainImageExtent = { .width = 0, .height = 0 };


	public:

		Swapchain() {}
		Swapchain(const VkSwapchainCreateInfoKHR& vkSwapchainCreateInfo, Device device) {
			VkSwapchainKHR vkSwapchain;
			VkResult vkResult = vkCreateSwapchainKHR(device, &vkSwapchainCreateInfo, nullptr, &vkSwapchain);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			new(this)Swapchain(vkSwapchain, device, &destroy, vkSwapchainCreateInfo.imageExtent);
		}

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



	class Image_Memory {

		Image_Memory(Image&& image, DeviceMemory&& deviceMemory)
			: m_image(std::move(image))
			, m_deviceMemory(std::move(deviceMemory)) {
		}

	public:

		Image			m_image;
		DeviceMemory	m_deviceMemory;

		Image_Memory() {}

		Image_Memory(const Image_Memory&) = delete;
		Image_Memory& operator=(const Image_Memory&) = delete;

		Image_Memory(Image_Memory&& other) noexcept
			: m_image(std::move(other.m_image))
			, m_deviceMemory(std::move(other.m_deviceMemory)) {
		}

		Image_Memory& operator=(Image_Memory&& other) noexcept {
			if (this == &other) {
				return *this;
			}
			(*this).~Image_Memory();
			new(this) Image_Memory(std::move(other));
			return *this;
		}


		Image_Memory(
			const ImageCreateInfo& imageCreateInfo,
			VkMemoryPropertyFlags properties,
			Device device
		) {
			Image image(imageCreateInfo, device);
			DeviceMemory deviceMemory = image.allocateDeviceMemory(properties);

			VkResult vkResult = vkBindImageMemory(device, image, deviceMemory, 0);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}

			new(this) Image_Memory(std::move(image), std::move(deviceMemory));

		}

		Image_Memory(
			VkExtent2D	vkExtent2D,
			VkFormat format,
			VkImageUsageFlags usage,
			VkMemoryPropertyFlags properties,
			Device device
		) {
			ImageCreateInfo imageCreateInfo(format, usage);
			imageCreateInfo.setExtent(vkExtent2D);
			new(this) Image_Memory(imageCreateInfo, properties, device);
		}

	};


	class Image_Memory_View {


	public:
		Image			m_image;
		DeviceMemory	m_deviceMemory;
		ImageView		m_imageView;

		Image_Memory_View() {}

		Image_Memory_View(
			Image&& image,
			DeviceMemory&& deviceMemory,
			ImageView&& imageView)
			: m_image(std::move(image))
			, m_deviceMemory(std::move(deviceMemory))
			, m_imageView(std::move(imageView)) {

		}

	};

	class SamplerCreateInfo : public VkSamplerCreateInfo {

	public:

		SamplerCreateInfo()
			: VkSamplerCreateInfo{}
		{
			sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			magFilter = VK_FILTER_LINEAR;
			minFilter = VK_FILTER_LINEAR;
			mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			anisotropyEnable = VK_FALSE;
			maxAnisotropy = 1.0f;
			borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
			unnormalizedCoordinates = VK_FALSE;
			compareEnable = VK_FALSE;
			compareOp = VK_COMPARE_OP_ALWAYS;
		}
	};

	class Sampler : public HandleWithOwner<VkSampler, Device> {

		Sampler(VkSampler sampler, Device device, DestroyFunc_t pfnDestroy)
			: HandleWithOwner(sampler, device, pfnDestroy) {
		}

		static void destroy(VkSampler sampler, Device device) {
			vkDestroySampler(device, sampler, nullptr);
		}

	public:

		Sampler() {}

		Sampler(const SamplerCreateInfo& samplerCreateInfo, Device device) {
			VkSampler vkSampler;
			VkResult vkResult = vkCreateSampler(device, &samplerCreateInfo, nullptr, &vkSampler);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			new(this) Sampler(vkSampler, device, &destroy);
		}


	};

	class Swapchain_ImageViews_FrameBuffers {

	public:
		static inline	Device		s_device;

		VkSwapchainCreateInfoKHR	m_vkSwapchainCreateInfo{};
		vkcpp::Surface				m_surface;

		RenderPass	m_renderPass;

		Swapchain	m_swapchain;
		std::vector<ImageView>		m_swapchainImageViews;
		std::vector<VkFramebuffer>	m_swapchainFrameBuffers;
		Image_Memory_View	m_depthBuffer;

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
			m_swapchainImageViews.clear();
		}

		void destroy() {
			if (!s_device) {
				return;
			}
			if (m_swapchain) {
				vkDeviceWaitIdle(s_device);
				destroyFrameBuffers();
				destroyImageViews();
			}
		}

		void createSwapchainImageViews(
			VkFormat	swapchainImageFormat
		) {
			std::vector<VkImage> swapchainImages = m_swapchain.getImages();
			ImageViewCreateInfo imageViewCreateInfo(
				VK_IMAGE_VIEW_TYPE_2D,
				swapchainImageFormat,
				VK_IMAGE_ASPECT_COLOR_BIT);
			m_swapchainImageViews = ImageView::createImageViews(swapchainImages, imageViewCreateInfo, m_swapchain.getOwner());
		}


		void createSwapchainFrameBuffers() {
			m_swapchainFrameBuffers.resize(m_swapchainImageViews.size());
			for (size_t i = 0; i < m_swapchainImageViews.size(); i++) {
				std::array<VkImageView, 2> attachments = {
					m_swapchainImageViews[i],
					m_depthBuffer.m_imageView
				};

				VkFramebufferCreateInfo framebufferInfo{};
				framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				framebufferInfo.renderPass = m_renderPass;
				framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
				framebufferInfo.pAttachments = attachments.data();
				framebufferInfo.width = m_swapchain.imageExtent().width;
				framebufferInfo.height = m_swapchain.imageExtent().height;
				framebufferInfo.layers = 1;

				VkResult vkResult = vkCreateFramebuffer(s_device, &framebufferInfo, nullptr, &m_swapchainFrameBuffers[i]);
				if (vkResult != VK_SUCCESS) {
					throw Exception(vkResult);
				}
			}
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

			return Swapchain(vkSwapchainCreateInfo, s_device);
		}




	public:

		Swapchain_ImageViews_FrameBuffers() {}
		~Swapchain_ImageViews_FrameBuffers() {
			destroy();
		}

		Swapchain_ImageViews_FrameBuffers(
			const VkSwapchainCreateInfoKHR& vkSwapchainCreateInfo,
			Surface surface)
			: m_vkSwapchainCreateInfo(vkSwapchainCreateInfo)
			, m_surface(surface) {

		}

		Swapchain_ImageViews_FrameBuffers(const Swapchain_ImageViews_FrameBuffers&) = delete;
		Swapchain_ImageViews_FrameBuffers& operator=(const Swapchain_ImageViews_FrameBuffers&) = delete;

		Swapchain_ImageViews_FrameBuffers(Swapchain_ImageViews_FrameBuffers&& other) noexcept
			: m_vkSwapchainCreateInfo(std::move(other.m_vkSwapchainCreateInfo))
			, m_surface(std::move(other.m_surface))
			, m_renderPass(std::move(other.m_renderPass))
			, m_swapchain(std::move(other.m_swapchain))
			, m_swapchainImageViews(std::move(other.m_swapchainImageViews))
			, m_swapchainFrameBuffers(std::move(other.m_swapchainFrameBuffers)) {
			other.makeEmpty();
		}

		Swapchain_ImageViews_FrameBuffers& operator=(Swapchain_ImageViews_FrameBuffers&& other) noexcept {
			if (this == &other) {
				return *this;
			}
			(*this).~Swapchain_ImageViews_FrameBuffers();
			new(this) Swapchain_ImageViews_FrameBuffers(std::move(other));
			other.makeEmpty();
			return *this;
		}


		static void setDevice(Device device) {
			s_device = device;
		}


		operator bool() {
			return !!m_swapchain;
		}

		VkSwapchainKHR vkSwapchain() {
			VkSwapchainKHR vkSwapchain = m_swapchain;	// Need to force a type conversion for return value.
			return vkSwapchain;
		}

		bool canDraw() {
			return !!(*this);
		}

		VkExtent2D getImageExtent() const {
			return m_swapchain.imageExtent();
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

			m_swapchain = std::move(createSwapchain(m_vkSwapchainCreateInfo, m_surface));
			if (!m_swapchain) {
				return;
			}
			createSwapchainImageViews(m_vkSwapchainCreateInfo.imageFormat);
			m_depthBuffer = std::move(createDepthBuffer(m_vkSwapchainCreateInfo.imageExtent, s_device));
			createSwapchainFrameBuffers();
		}

		Image_Memory_View createDepthBuffer(
			VkExtent2D vkExtent2D,
			vkcpp::Device device
		) {
			vkcpp::ImageCreateInfo imageCreateInfo(
				VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.setExtent(vkExtent2D);
			vkcpp::Image_Memory image_memory(
				imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, device);

			vkcpp::ImageViewCreateInfo imageViewCreateInfo(
				VK_IMAGE_VIEW_TYPE_2D,
				VK_FORMAT_D32_SFLOAT,
				VK_IMAGE_ASPECT_DEPTH_BIT);
			imageViewCreateInfo.image = image_memory.m_image;
			vkcpp::ImageView imageView(imageViewCreateInfo, device);

			return Image_Memory_View(
				std::move(image_memory.m_image),
				std::move(image_memory.m_deviceMemory),
				std::move(imageView));

		}



	};


	class DescriptorSetUpdater {

		union WriteDescriptorInfo {
			VkDescriptorBufferInfo	m_vkDescriptorBufferInfo;
			VkDescriptorImageInfo m_vkDescriptorImageInfo;

			WriteDescriptorInfo(const VkDescriptorBufferInfo& vkDescriptorBufferInfo)
				: m_vkDescriptorBufferInfo(vkDescriptorBufferInfo) {
			}

			WriteDescriptorInfo(const VkDescriptorImageInfo& vkDescriptorImageInfo)
				: m_vkDescriptorImageInfo(vkDescriptorImageInfo) {
			}

		};

		vkcpp::DescriptorSet	m_descriptorSet;
		std::vector<VkWriteDescriptorSet>	m_vkWriteDescriptorSets;
		std::vector<WriteDescriptorInfo>	m_writeDescriptorInfos;

	public:

		DescriptorSetUpdater(vkcpp::DescriptorSet descriptorSet)
			: m_descriptorSet(descriptorSet) {

		}

		void addWriteDescriptor(
			int	bindingIndex,
			VkDescriptorType vkDescriptorType,
			vkcpp::Buffer buffer,
			VkDeviceSize size) {

			const VkDescriptorBufferInfo* marker = reinterpret_cast<VkDescriptorBufferInfo*>(-1);

			VkWriteDescriptorSet	vkWriteDescriptorSet{};
			vkWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			vkWriteDescriptorSet.dstSet = m_descriptorSet;
			vkWriteDescriptorSet.dstBinding = bindingIndex;
			vkWriteDescriptorSet.dstArrayElement = 0;
			vkWriteDescriptorSet.descriptorType = vkDescriptorType;
			vkWriteDescriptorSet.descriptorCount = 1;
			vkWriteDescriptorSet.pBufferInfo = marker;

			VkDescriptorBufferInfo vkDescriptorBufferInfo{};
			vkDescriptorBufferInfo.buffer = buffer;
			vkDescriptorBufferInfo.offset = 0;
			vkDescriptorBufferInfo.range = size;

			m_vkWriteDescriptorSets.push_back(vkWriteDescriptorSet);
			m_writeDescriptorInfos.push_back(vkDescriptorBufferInfo);

		}

		void addWriteDescriptor(
			int	bindingIndex,
			VkDescriptorType vkDescriptorType,
			ImageView imageView,
			Sampler sampler) {

			const VkDescriptorImageInfo* marker = reinterpret_cast<VkDescriptorImageInfo*>(-1);

			VkWriteDescriptorSet	vkWriteDescriptorSet{};
			vkWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			vkWriteDescriptorSet.dstSet = m_descriptorSet;
			vkWriteDescriptorSet.dstBinding = bindingIndex;
			vkWriteDescriptorSet.dstArrayElement = 0;
			vkWriteDescriptorSet.descriptorType = vkDescriptorType;
			vkWriteDescriptorSet.descriptorCount = 1;
			vkWriteDescriptorSet.pImageInfo = marker;

			VkDescriptorImageInfo vkDescriptorImageInfo{};
			vkDescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			vkDescriptorImageInfo.imageView = imageView;
			vkDescriptorImageInfo.sampler = sampler;

			m_vkWriteDescriptorSets.push_back(vkWriteDescriptorSet);
			m_writeDescriptorInfos.push_back(vkDescriptorImageInfo);

		}



		void assemble() {
			int	index = 0;
			for (VkWriteDescriptorSet& vkWriteDescriptorSet : m_vkWriteDescriptorSets) {
				//	Update marked pointers to the proper address from the info array.
				if (vkWriteDescriptorSet.pBufferInfo) {
					vkWriteDescriptorSet.pBufferInfo = &(m_writeDescriptorInfos.at(index).m_vkDescriptorBufferInfo);
				}
				if (vkWriteDescriptorSet.pImageInfo) {
					vkWriteDescriptorSet.pImageInfo = &(m_writeDescriptorInfos.at(index).m_vkDescriptorImageInfo);
				}
				++index;
			}
		}

		void updateDescriptorSets() {
			assemble();
			vkUpdateDescriptorSets(
				m_descriptorSet.getOwner().getVkDevice(),
				static_cast<uint32_t>(m_vkWriteDescriptorSets.size()),
				m_vkWriteDescriptorSets.data(),
				0, nullptr);
		}

	};


}




