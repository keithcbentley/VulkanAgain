

#include <exception>
#include <vector>
#include <unordered_set>
#include <iostream>
#include <fstream>
#include <string>

#include <vulkan/vulkan.h>

#include "VulkanSynchronization2Only.h"


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

		uint32_t	m_vkVersionNumber{};

	public:

		VersionNumber() : m_vkVersionNumber(0) {}
		VersionNumber(uint32_t vkVersionNumber) : m_vkVersionNumber(vkVersionNumber) {}
		VersionNumber(uint32_t major, uint32_t minor, uint32_t patch)
			: m_vkVersionNumber(VK_MAKE_API_VERSION(0, major, minor, patch)) {
		}

		uint32_t	major() const { return VK_API_VERSION_MAJOR(m_vkVersionNumber); }
		uint32_t	minor() const { return VK_API_VERSION_MINOR(m_vkVersionNumber); }
		uint32_t	patch() const { return VK_API_VERSION_PATCH(m_vkVersionNumber); }
		uint32_t	variant() const { return VK_API_VERSION_VARIANT(m_vkVersionNumber); }

		static VersionNumber getVersionNumber() {
			uint32_t	vkVersionNumberersion;
			vkEnumerateInstanceVersion(&vkVersionNumberersion);
			return VersionNumber(vkVersionNumberersion);
		}

		operator uint32_t() const {
			return m_vkVersionNumber;
		}

		operator bool() const {
			return m_vkVersionNumber != 0;
		}

		int y;
		auto operator<=>(const VersionNumber& other) const {
			if (auto cmp = major() <=> other.major(); cmp != 0) {
				return cmp;
			}
			if (auto cmp = minor() <=> other.minor(); cmp != 0) {
				return cmp;
			}
			return patch() <=> other.patch();
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

		VkPhysicalDeviceFeatures2 getPhysicalDeviceFeatures2() {
			VkPhysicalDeviceFeatures2	vkPhysicalDeviceFeatures2{};
			vkPhysicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
			vkGetPhysicalDeviceFeatures2(m_vkPhysicalDevice, &vkPhysicalDeviceFeatures2);
			return vkPhysicalDeviceFeatures2;
		}

		VkPhysicalDeviceMemoryProperties getPhysicalDeviceMemoryProperties() {
			VkPhysicalDeviceMemoryProperties vkPhysicalDeviceMemoryProperties;
			vkGetPhysicalDeviceMemoryProperties(m_vkPhysicalDevice, &vkPhysicalDeviceMemoryProperties);
			return vkPhysicalDeviceMemoryProperties;
		}

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

		std::vector<const char*>	m_layerNamesVector{};
		std::vector<const char*>	m_extensionNamesVector{};

		VkApplicationInfo	m_vkApplicationInfo{};

	public:
		//	Somewhat arbitrary to make this no-move, no-copy.
		VulkanInstanceCreateInfo(const VulkanInstanceCreateInfo&) = delete;
		VulkanInstanceCreateInfo& operator=(const VulkanInstanceCreateInfo&) = delete;
		VulkanInstanceCreateInfo(VulkanInstanceCreateInfo&&) = delete;
		VulkanInstanceCreateInfo& operator=(VulkanInstanceCreateInfo&&) = delete;

		VkInstanceCreateInfo* operator& () = delete;	//	Make sure assemble is used.

		VulkanInstanceCreateInfo(VersionNumber versionNumber) : VkInstanceCreateInfo{} {
			sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			m_vkApplicationInfo.apiVersion = versionNumber;
		}

		VulkanInstanceCreateInfo()
			: VulkanInstanceCreateInfo(VersionNumber::getVersionNumber()) {
		}


		VkInstanceCreateInfo* assemble() {
			//	Need to convert from sets of layer names and extensions
			//	to vectors so that they can be used by the Vulkan api.
			m_layerNamesVector.clear();		// hygiene in case we are called twice
			ppEnabledLayerNames = nullptr;
			for (const std::string& layerName : m_layerNames) {
				m_layerNamesVector.push_back(layerName.c_str());
			}
			enabledLayerCount = static_cast<uint32_t>(m_layerNames.size());
			if (enabledLayerCount > 0) {
				ppEnabledLayerNames = m_layerNamesVector.data();
			}

			m_extensionNamesVector.clear();	// hygiene in case we are called twice
			ppEnabledExtensionNames = nullptr;
			for (const std::string& extensionName : m_extensionNames) {
				m_extensionNamesVector.push_back(extensionName.c_str());
			}
			enabledExtensionCount = static_cast<uint32_t>(m_extensionNames.size());
			if (enabledExtensionCount > 0) {
				ppEnabledExtensionNames = m_extensionNamesVector.data();
			}


			pApplicationInfo = &m_vkApplicationInfo;

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
		VulkanInstance(const VulkanInstance&) = delete;
		VulkanInstance& operator=(const VulkanInstance&) = delete;

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
			std::vector<VkPhysicalDevice> vkPhysicalDevices(physicalDeviceCount);
			vkResult = vkEnumeratePhysicalDevices(*this, &physicalDeviceCount, vkPhysicalDevices.data());
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			//	IMPORTANT: this is crazy.  If we don't call
			//	vkGetPhysicalDeviceQueueFamilyProperties to get the number
			//	of queue families, we can't create device queues later without
			//	the validation layer spitting out an error.  Just calling the function
			//	(and throwing away the value) seems to be sufficient.
			for (VkPhysicalDevice vkPhysicalDevice : vkPhysicalDevices) {
				uint32_t queueFamilyCount = 0;
				vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyCount, nullptr);

			}

			return vkPhysicalDevices;
		}

		PhysicalDevice getPhysicalDevice(int physicalDeviceIndex) const {
			std::vector<VkPhysicalDevice> vkPhysicalDevices = getAllPhysicalDevices();
			return PhysicalDevice(vkPhysicalDevices.at(physicalDeviceIndex));
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

			if (vkResult == VK_ERROR_SURFACE_LOST_KHR) {
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

		static const inline int	MAX_DEVICE_QUEUES = 8;

		static const inline std::array<float, MAX_DEVICE_QUEUES>
			s_queuePriorities{ 1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f };

	public:
		DeviceQueueCreateInfo(const DeviceQueueCreateInfo&) = delete;
		DeviceQueueCreateInfo& operator=(const DeviceQueueCreateInfo&) = delete;
		DeviceQueueCreateInfo(DeviceQueueCreateInfo&&) = delete;
		DeviceQueueCreateInfo& operator=(DeviceQueueCreateInfo&&) = delete;

		DeviceQueueCreateInfo(uint32_t queueFamilyIndexArg, int queueCountArg) : VkDeviceQueueCreateInfo{} {

			sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueFamilyIndex = queueFamilyIndexArg;
			queueCount = queueCountArg;
			pQueuePriorities = s_queuePriorities.data();
		}

	};
	static_assert(sizeof(DeviceQueueCreateInfo) == sizeof(VkDeviceQueueCreateInfo));

	//	TODO: do we need a better name for this? or a better location?
	struct AddDeviceQueueInfo {
		int	queueFamilyIndex;
		int	count;
	};

	class DeviceCreateInfo : public VkDeviceCreateInfo {

		std::vector<std::string>	m_extensionNames{};
		std::vector<const char*>	m_extensionStrings{};

		static const int MAX_DEVICE_QUEUE_FAMILIES = 8;
		std::array<int, MAX_DEVICE_QUEUE_FAMILIES> m_deviceQueueCounts{};
		std::vector<VkDeviceQueueCreateInfo>	m_deviceQueueCreateInfos;

		VkPhysicalDeviceSynchronization2Features m_sync2Features;



	public:
		//	Somewhat arbitrary to make this no-move, no-copy.
		DeviceCreateInfo(const DeviceCreateInfo&) = delete;
		DeviceCreateInfo& operator=(const DeviceCreateInfo&) = delete;
		DeviceCreateInfo(DeviceCreateInfo&&) = delete;
		DeviceCreateInfo& operator=(DeviceCreateInfo&&) = delete;

		VkDeviceCreateInfo* operator& () = delete;	//	Make sure assemble is used.

		DeviceCreateInfo() : VkDeviceCreateInfo{} {
			sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

			//VkStructureType                    sType;
			//const void* pNext;
			//VkDeviceCreateFlags                flags;
			//uint32_t                           queueCreateInfoCount;
			//const VkDeviceQueueCreateInfo* pQueueCreateInfos;
			//uint32_t                           enabledLayerCount;
			//const char* const* ppEnabledLayerNames;
			//uint32_t                           enabledExtensionCount;
			//const char* const* ppEnabledExtensionNames;
			//const VkPhysicalDeviceFeatures* pEnabledFeatures;

		}

		VkDeviceCreateInfo* assemble() {

			m_extensionStrings.clear();			//	Hygiene in case we are called more than once.
			ppEnabledExtensionNames = nullptr;
			for (const std::string& extensionName : m_extensionNames) {
				m_extensionStrings.push_back(extensionName.c_str());
			}
			enabledExtensionCount = static_cast<uint32_t>(m_extensionNames.size());
			if (enabledExtensionCount > 0) {
				ppEnabledExtensionNames = m_extensionStrings.data();
			}

			for (int deviceQueueFamilyIndex = 0;
				deviceQueueFamilyIndex < MAX_DEVICE_QUEUE_FAMILIES;
				deviceQueueFamilyIndex++) {
				if (m_deviceQueueCounts[deviceQueueFamilyIndex] > 0) {
					vkcpp::DeviceQueueCreateInfo deviceQueueCreateInfo(
						deviceQueueFamilyIndex, m_deviceQueueCounts[deviceQueueFamilyIndex]);
					m_deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);
				}
			}

			//	TODO: add hygiene in case we are called twice.
			queueCreateInfoCount = m_deviceQueueCreateInfos.size();
			if (queueCreateInfoCount > 0) {
				pQueueCreateInfos = m_deviceQueueCreateInfos.data();
			}

			// For some reason, this needs to be enabled through this structure.
			m_sync2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
			m_sync2Features.synchronization2 = TRUE;
			pNext = &m_sync2Features;

			return this;
		}


		void addExtension(const char* extensionName) {
			m_extensionNames.push_back(extensionName);
		}

		void addDeviceQueue(uint32_t deviceQueueFamilyIndex, int numberOfQueues) {
			m_deviceQueueCounts.at(deviceQueueFamilyIndex) += numberOfQueues;
		}

		void addDeviceQueues(const std::vector<AddDeviceQueueInfo>& addDeviceQueueInfos) {
			for (const AddDeviceQueueInfo& addDeviceQueueInfo : addDeviceQueueInfos) {
				addDeviceQueue(addDeviceQueueInfo.queueFamilyIndex, addDeviceQueueInfo.count);
			}
		}

	};

	class Queue;
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


		Queue getDeviceQueue(int deviceQueueFamily, int deviceQueueIndex) const;

		uint32_t findMemoryTypeIndex(uint32_t usableMemoryIndexBits, VkMemoryPropertyFlags requiredProperties) {
			return getPhysicalDevice().findMemoryTypeIndex(usableMemoryIndexBits, requiredProperties);
		}

		void waitIdle() {
			vkDeviceWaitIdle(*this);
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

		Semaphore(VkDevice vkDevice) {
			VkSemaphoreCreateInfo vkSemaphoreCreateInfo{};
			vkSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			new(this)Semaphore(vkSemaphoreCreateInfo, vkDevice);
		}


	};


	class Fence : public HandleWithOwner<VkFence> {
		//	Vulkan set/reset/signaled names are not really clear when it comes to fences.
		//	Use better open/closed terminology.
#define VKCPP_FENCE_CREATE_OPENED VK_FENCE_CREATE_SIGNALED_BIT

		Fence(VkFence vkFence, VkDevice vkDevice, DestroyFunc_t pfnDestroy)
			: HandleWithOwner(vkFence, vkDevice, pfnDestroy) {
		}

		static void destroy(VkFence vkFence, VkDevice vkDevice) {
			vkDestroyFence(vkDevice, vkFence, nullptr);
		}

	public:

		Fence() {}

		//	Kind of an exception to the argument ordering usually used.
		//	Flags are almost always 0, so make them optional.  Only
		//	the device is required.
		Fence(VkDevice vkDevice, VkFenceCreateFlags vkFenceCreateFlags = 0) {
			VkFenceCreateInfo vkFenceCreateInfo{};
			vkFenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			vkFenceCreateInfo.flags = vkFenceCreateFlags;
			VkFence vkFence;
			VkResult vkResult = vkCreateFence(vkDevice, &vkFenceCreateInfo, nullptr, &vkFence);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			new(this) Fence(vkFence, vkDevice, &destroy);
		}

		void close() {
			VkFence vkFence = *this;
			vkResetFences(m_owner, 1, &vkFence);
		}

		void wait() {
			VkFence vkFence = *this;
			vkWaitForFences(m_owner, 1, &vkFence, VK_TRUE, UINT64_MAX);
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

		DeviceMemory(
			VkMemoryRequirements vkMemoryRequirements,
			VkMemoryPropertyFlags vkRequiredMemoryPropertyFlags,
			Device	device
		) {
			VkMemoryAllocateInfo vkMemoryAllocateInfo{};
			vkMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			vkMemoryAllocateInfo.allocationSize = vkMemoryRequirements.size;
			vkMemoryAllocateInfo.memoryTypeIndex =
				device.findMemoryTypeIndex(vkMemoryRequirements.memoryTypeBits, vkRequiredMemoryPropertyFlags);
			new(this)DeviceMemory(vkMemoryAllocateInfo, device);
		}


	};


	class Buffer : public HandleWithOwner<VkBuffer, Device> {

		static void destroy(VkBuffer vkBuffer, Device device) {
			vkDestroyBuffer(device, vkBuffer, nullptr);
		}

		Buffer(VkBuffer vkBuffer, Device device, DestroyFunc_t pfnDestroy)
			: HandleWithOwner(vkBuffer, device, pfnDestroy) {
		}

		Buffer(const VkBufferCreateInfo& vkBufferCreateInfo, Device device) {
			VkBuffer vkBuffer;
			VkResult vkResult = vkCreateBuffer(device, &vkBufferCreateInfo, nullptr, &vkBuffer);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			new(this)Buffer(vkBuffer, device, &destroy);
		}

	public:

		Buffer() {}

		Buffer(
			VkBufferUsageFlags	vkBufferUsageFlags,
			VkDeviceSize		size,
			uint32_t			queueFamilyIndex,
			Device				device
		) {
			VkBufferCreateInfo vkBufferCreateInfo{};
			vkBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			vkBufferCreateInfo.usage = vkBufferUsageFlags;
			vkBufferCreateInfo.size = size;
			vkBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			uint32_t	queueFamilyIndexLocal = queueFamilyIndex;
			vkBufferCreateInfo.pQueueFamilyIndices = &queueFamilyIndexLocal;
			new(this)Buffer(vkBufferCreateInfo, device);

		}

		VkMemoryRequirements  getMemoryRequirements() {
			VkMemoryRequirements vkMemoryRequirements;
			vkGetBufferMemoryRequirements(getVkDevice(), *this, &vkMemoryRequirements);
			return vkMemoryRequirements;
		}

		DeviceMemory allocateDeviceMemory(VkMemoryPropertyFlags vkRequiredMemoryPropertyFlags) {
			VkMemoryRequirements vkMemoryRequirements = getMemoryRequirements();
			return DeviceMemory(vkMemoryRequirements, vkRequiredMemoryPropertyFlags, getOwner());
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
			VkBufferUsageFlags vkBufferUsageFlags,
			VkDeviceSize size,
			uint32_t	queueFamilyIndex,
			VkMemoryPropertyFlags vkMemoryPropertyFlags,
			Device device
		) {
			Buffer buffer(vkBufferUsageFlags, size, queueFamilyIndex, device);

			DeviceMemory deviceMemory = buffer.allocateDeviceMemory(vkMemoryPropertyFlags);

			VkResult vkResult = vkBindBufferMemory(device, buffer, deviceMemory, 0);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}

			// TODO: should this be a method on DeviceMemory?
			void* mappedMemory;
			vkResult = vkMapMemory(device, deviceMemory, 0, size, 0, &mappedMemory);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}

			new(this) Buffer_DeviceMemory(std::move(buffer), std::move(deviceMemory), mappedMemory);

		}

		Buffer_DeviceMemory(
			VkBufferUsageFlags vkBufferUsageFlags,
			int64_t size,
			uint32_t	queueFamilyIndex,
			VkMemoryPropertyFlags vkRequiredMemoryPropertyFlags,
			void* pSrcMem,
			Device device
		) {
			new(this)Buffer_DeviceMemory(
				vkBufferUsageFlags,
				size,
				queueFamilyIndex,
				vkRequiredMemoryPropertyFlags,
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


	class AttachmentDescription : public VkAttachmentDescription {

	public:

		AttachmentDescription()
			: VkAttachmentDescription{} {
		}

		static AttachmentDescription simpleColorAttachmentPresentDescription(
			VkFormat	colorAttachmentVkFormat
		) {
			AttachmentDescription	colorAttachmentDescription;
			//	Reasonable defaults
			colorAttachmentDescription.format = colorAttachmentVkFormat;
			colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			return colorAttachmentDescription;
		}

		static AttachmentDescription simpleColorAttachmentDescription(
			VkFormat	colorAttachmentVkFormat
		) {
			AttachmentDescription	colorAttachmentDescription;
			//	Reasonable defaults
			colorAttachmentDescription.format = colorAttachmentVkFormat;
			colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			return colorAttachmentDescription;
		}


		static AttachmentDescription simpleDepthAttachmentDescription() {
			AttachmentDescription depthAttachmentDescription;
			//	Reasonable defaults
			depthAttachmentDescription.format = VK_FORMAT_D32_SFLOAT;
			depthAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
			depthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			return depthAttachmentDescription;
		}


	};
	static_assert(sizeof(AttachmentDescription) == sizeof(VkAttachmentDescription));



	class SubpassDescription : public VkSubpassDescription {

		std::vector<VkAttachmentReference>	m_inputAttachmentReferences;
		std::vector<VkAttachmentReference>	m_colorAttachmentReferences;
		VkAttachmentReference				m_depthStencilAttachmentReference{};
		bool								m_haveDepthStencilAttachmentReference{};

		//typedef struct VkSubpassDescription {
		//	VkSubpassDescriptionFlags       flags;
		//	VkPipelineBindPoint             pipelineBindPoint;
		// 
		//	uint32_t                        inputAttachmentCount;
		//	const VkAttachmentReference* pInputAttachments;
		// 
		//	uint32_t                        colorAttachmentCount;
		//	const VkAttachmentReference* pColorAttachments;
		// 
		//	const VkAttachmentReference* pResolveAttachments;
		// 
		//	const VkAttachmentReference* pDepthStencilAttachment;
		// 
		//	uint32_t                        preserveAttachmentCount;
		//	const uint32_t* pPreserveAttachments;
		//} VkSubpassDescription;


	public:

		VkSubpassDescription* operator&() = delete;

		SubpassDescription()
			: VkSubpassDescription{} {
		}

		SubpassDescription& setPipelineBindPoint(VkPipelineBindPoint vkPipelineBindPoint) {
			pipelineBindPoint = vkPipelineBindPoint;
			return *this;
		}

		SubpassDescription& addInputAttachmentReference(VkAttachmentReference& vkAttachmentReference) {
			m_inputAttachmentReferences.push_back(vkAttachmentReference);
			return *this;
		}

		SubpassDescription& addColorAttachmentReference(VkAttachmentReference& vkAttachmentReference) {
			m_colorAttachmentReferences.push_back(vkAttachmentReference);
			return *this;
		}

		SubpassDescription& setDepthStencilAttachmentReference(VkAttachmentReference& vkDepthStencilAttachmentReference) {
			m_depthStencilAttachmentReference = vkDepthStencilAttachmentReference;
			m_haveDepthStencilAttachmentReference = true;
			return *this;
		}

		//static inline const std::array<uint32_t, 2> s_preserve = { 0,1 };

		VkSubpassDescription* assemble() {

			inputAttachmentCount = m_inputAttachmentReferences.size();
			if (inputAttachmentCount > 0) {
				pInputAttachments = m_inputAttachmentReferences.data();
			}

			colorAttachmentCount = m_colorAttachmentReferences.size();
			if (colorAttachmentCount > 0) {
				pColorAttachments = m_colorAttachmentReferences.data();
			}

			if (m_haveDepthStencilAttachmentReference) {
				pDepthStencilAttachment = &m_depthStencilAttachmentReference;
			}



			//	TODO:	need to figure out which attachments to preserve.
			//preserveAttachmentCount = 2;
			//pPreserveAttachments = s_preserve.data();


			return this;
		}

	};


	class SubpassDependency : public VkSubpassDependency {

	public:

		SubpassDependency()
			: VkSubpassDependency{} {
		}


		SubpassDependency& setDependency(
			uint32_t	srcSubpassArg,
			uint32_t	dstSubpassArg
		) {
			srcSubpass = srcSubpassArg;
			dstSubpass = dstSubpassArg;
			return *this;
		}

		SubpassDependency& setSrc(
			VkPipelineStageFlags    srcStageMaskArg,
			VkAccessFlags           srcAccessMaskArg
		) {
			srcStageMask = srcStageMaskArg;
			srcAccessMask = srcAccessMaskArg;
			return *this;
		}

		SubpassDependency& addSrc(
			VkPipelineStageFlags    srcStageMaskArg,
			VkAccessFlags           srcAccessMaskArg
		) {
			srcStageMask |= srcStageMaskArg;
			srcAccessMask |= srcAccessMaskArg;
			return *this;
		}


		SubpassDependency& setDst(
			VkPipelineStageFlags    dstStageMaskArg,
			VkAccessFlags           dstAccessMaskArg
		) {
			dstStageMask = dstStageMaskArg;
			dstAccessMask = dstAccessMaskArg;
			return *this;
		}

		SubpassDependency& addDst(
			VkPipelineStageFlags    dstStageMaskArg,
			VkAccessFlags           dstAccessMaskArg
		) {
			dstStageMask |= dstStageMaskArg;
			dstAccessMask |= dstAccessMaskArg;
			return *this;
		}

	};
	static_assert(sizeof(SubpassDependency) == sizeof(VkSubpassDependency));


	class RenderPassCreateInfo : public VkRenderPassCreateInfo {

		std::vector<VkAttachmentDescription>	m_attachmentDescriptions;

		std::vector<SubpassDescription> m_subpassDescriptions;
		std::vector<VkSubpassDescription>	m_vkSubpassDescriptions;

		std::vector<SubpassDependency> m_subpassDependencies;
		std::vector<VkSubpassDependency> m_vkSubpassDependencies;

	public:

		RenderPassCreateInfo()
			: VkRenderPassCreateInfo{} {
		}

		VkRenderPassCreateInfo* operator&() = delete;

		VkAttachmentReference addAttachment(
			const AttachmentDescription& attachmentDescription,
			VkImageLayout	imageLayout
		) {
			m_attachmentDescriptions.emplace_back(attachmentDescription);
			//	"attachment" really means "attachment index in the array of attachments"
			VkAttachmentReference attachmentReference{};
			attachmentReference.attachment = static_cast<uint32_t>(m_attachmentDescriptions.size() - 1);
			attachmentReference.layout = imageLayout;
			return attachmentReference;
		}

		SubpassDescription& addSubpass() {
			SubpassDescription& subpassDescription = m_subpassDescriptions.emplace_back();
			subpassDescription.setPipelineBindPoint(VK_PIPELINE_BIND_POINT_GRAPHICS);
			return subpassDescription;
		}

		SubpassDependency& addSubpassDependency(
			uint32_t	srcSubpassArg,
			uint32_t	dstSubpassArg
		) {
			SubpassDependency& subpassDependency = m_subpassDependencies.emplace_back();
			subpassDependency.setDependency(srcSubpassArg, dstSubpassArg);
			return subpassDependency;
		}


		VkRenderPassCreateInfo* assemble() {

			//	TODO: where do these magic layout transitions happen?

			sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

			pAttachments = nullptr;
			attachmentCount = static_cast<uint32_t>(m_attachmentDescriptions.size());
			if (attachmentCount > 0) {
				pAttachments = m_attachmentDescriptions.data();
			}

			pSubpasses = nullptr;
			m_vkSubpassDescriptions.clear();
			subpassCount = m_subpassDescriptions.size();
			if (subpassCount > 0) {
				for (SubpassDescription& subpassDescription : m_subpassDescriptions) {
					m_vkSubpassDescriptions.push_back(*subpassDescription.assemble());
				}
				pSubpasses = m_vkSubpassDescriptions.data();
			}

			pDependencies = nullptr;
			m_vkSubpassDependencies.clear();
			dependencyCount = m_subpassDependencies.size();
			if (dependencyCount > 0) {
				for (SubpassDependency& subpassDependency : m_subpassDependencies) {
					m_vkSubpassDependencies.push_back(subpassDependency);
				}
				pDependencies = m_vkSubpassDependencies.data();
			}

			return this;

		}


	};


	//	TODO: do we really need this yet?
	class RenderPassCreateInfo2 : public VkRenderPassCreateInfo2 {

	public:

		VkRenderPassCreateInfo2* operator&() = delete;

		VkRenderPassCreateInfo2* assemble() {
			return this;
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
		RenderPass(RenderPassCreateInfo& renderPassCreateInfo, VkDevice vkDevice) {
			VkRenderPass	vkRenderPass;
			VkResult vkResult = vkCreateRenderPass(vkDevice, renderPassCreateInfo.assemble(), nullptr, &vkRenderPass);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			new(this)RenderPass(vkRenderPass, vkDevice, &destroy);
		}

		//	TODO: do we really need this yet?
		//RenderPass(RenderPassCreateInfo2& renderPassCreateInfo2, VkDevice vkDevice) {
		//	VkRenderPass	vkRenderPass;
		//	VkResult vkResult = vkCreateRenderPass2(vkDevice, renderPassCreateInfo2.assemble(), nullptr, &vkRenderPass);
		//	if (vkResult != VK_SUCCESS) {
		//		throw Exception(vkResult);
		//	}
		//	new(this)RenderPass(vkRenderPass, vkDevice, &destroy);
		//}

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



	class ImageMemoryBarrier2 : public VkImageMemoryBarrier2 {

	public:
		ImageMemoryBarrier2(VkImageLayout oldLayoutArg, VkImageLayout newLayoutArg, VkImage imageArg)
			:VkImageMemoryBarrier2{} {
			sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;

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

			//	TODO: do we need a more general way to do this?
			//	Note that this grabs oldLayout and newLayout from the structure, not the args.
			if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
				srcStageMask = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
				srcAccessMask = 0;
				dstStageMask = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
				dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
				srcStageMask = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
				srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				dstStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
				dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			}
			else {
				throw std::invalid_argument("unsupported layout transition!");
			}
		}

	};
	static_assert(sizeof(ImageMemoryBarrier2) == sizeof(VkImageMemoryBarrier2));


	class DependencyInfo : public VkDependencyInfo {

		//	TODO: need to add the other dependency types.
		std::vector<ImageMemoryBarrier2>	m_imageMemoryBarriers;


	public:
		DependencyInfo(const DependencyInfo&) = delete;
		DependencyInfo& operator=(const DependencyInfo&) = delete;
		DependencyInfo(DependencyInfo&&) noexcept = delete;
		DependencyInfo& operator=(DependencyInfo&&) noexcept = delete;

		VkDependencyInfo* operator& () = delete;

		DependencyInfo()
			: VkDependencyInfo{} {
			sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		}

		void addImageMemoryBarrier(const ImageMemoryBarrier2& imageMemoryBarrier) {
			m_imageMemoryBarriers.push_back(imageMemoryBarrier);
		}

		VkDependencyInfo* assemble() {
			imageMemoryBarrierCount = m_imageMemoryBarriers.size();
			pImageMemoryBarriers = nullptr;
			if (imageMemoryBarrierCount > 0) {
				pImageMemoryBarriers = m_imageMemoryBarriers.data();
			}
			return this;
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

		CommandPool(
			VkCommandPoolCreateFlags	vkCommandPoolCreateFlags,
			uint32_t					queueFamilyIndex,
			VkDevice					vkDevice
		) {
			VkCommandPoolCreateInfo commandPoolCreateInfo{};
			commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			commandPoolCreateInfo.flags = vkCommandPoolCreateFlags;
			commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
			new(this)CommandPool(commandPoolCreateInfo, vkDevice);

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

		void cmdCopyBufferToImage(
			Buffer buffer,
			Image image,
			uint32_t	width,
			uint32_t	height
		) {
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

			vkCmdCopyBufferToImage(*this, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		}

		void cmdCopyBuffer(
			Buffer	srcBuffer,
			Buffer	dstBuffer,
			VkDeviceSize	size
		) {
			VkBufferCopy	vkBufferCopy{ .srcOffset = 0, .dstOffset = 0, .size = size };

			vkCmdCopyBuffer(*this, srcBuffer, dstBuffer, 1, &vkBufferCopy);
		}

		void cmdPipelineBarrier2(
			DependencyInfo& dependencyInfo
		) {
			vkCmdPipelineBarrier2(*this, dependencyInfo.assemble());
		}

	};


	//class SubmitInfo : public VkSubmitInfo {

	//	//	TODO: modify to handle more semaphores and command buffers?
	//	//	TODO: error checking? e.g., no command buffer?
	//	VkSemaphore	m_vkSemaphoreWait = nullptr;
	//	VkPipelineStageFlags	m_waitPipelineStageFlags{};
	//	VkCommandBuffer	m_commandBuffer = nullptr;
	//	VkSemaphore m_vkSemaphoreSignal = nullptr;


	//public:
	//	SubmitInfo()
	//		: VkSubmitInfo{} {
	//		sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	//	}

	//	SubmitInfo* operator&() = delete;

	//	void addWaitSemaphore(
	//		Semaphore	semaphore,
	//		VkPipelineStageFlags waitPipelineStateFlags) {
	//		m_vkSemaphoreWait = semaphore;
	//		m_waitPipelineStageFlags = waitPipelineStateFlags;
	//	}

	//	void addCommandBuffer(vkcpp::CommandBuffer commandBuffer) {
	//		m_commandBuffer = commandBuffer;
	//	}

	//	void addSignalSemaphore(vkcpp::Semaphore semaphore) {
	//		m_vkSemaphoreSignal = semaphore;
	//	}

	//	VkSubmitInfo* assemble() {
	//		if (m_vkSemaphoreWait) {
	//			waitSemaphoreCount = 1;
	//			pWaitSemaphores = &m_vkSemaphoreWait;
	//			pWaitDstStageMask = &m_waitPipelineStageFlags;
	//		}

	//		if (m_commandBuffer) {
	//			commandBufferCount = 1;
	//			pCommandBuffers = &m_commandBuffer;
	//		}

	//		if (m_vkSemaphoreSignal) {
	//			signalSemaphoreCount = 1;
	//			pSignalSemaphores = &m_vkSemaphoreSignal;
	//		}

	//		return this;

	//	}

	//};

	class SubmitInfo2 : public VkSubmitInfo2 {

		std::vector<VkSemaphoreSubmitInfo>		m_waitSemaphoreInfos;
		std::vector<VkCommandBufferSubmitInfo>	m_commandBufferInfos;
		std::vector<VkSemaphoreSubmitInfo>		m_signalSemaphoreInfos;


	public:

		SubmitInfo2()
			: VkSubmitInfo2{} {
			sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		}

		SubmitInfo2* operator&() = delete;

		void addCommandBuffer(CommandBuffer commandBuffer) {
			VkCommandBufferSubmitInfo	vkCommandBufferSubmitInfo{};
			vkCommandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
			vkCommandBufferSubmitInfo.commandBuffer = commandBuffer;
			m_commandBufferInfos.push_back(vkCommandBufferSubmitInfo);
		}

		void addWaitSemaphore(
			Semaphore semaphore,
			VkPipelineStageFlags2 waitPipelineStateFlags2
		) {
			VkSemaphoreSubmitInfo	vkSemaphoreSubmitInfo{};
			vkSemaphoreSubmitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
			vkSemaphoreSubmitInfo.semaphore = semaphore;
			vkSemaphoreSubmitInfo.stageMask = waitPipelineStateFlags2;
			m_waitSemaphoreInfos.push_back(vkSemaphoreSubmitInfo);
		}

		void addSignalSemaphore(Semaphore semaphore) {
			VkSemaphoreSubmitInfo	vkSemaphoreSubmitInfo{};
			vkSemaphoreSubmitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
			vkSemaphoreSubmitInfo.semaphore = semaphore;
			m_signalSemaphoreInfos.push_back(vkSemaphoreSubmitInfo);
		}


		SubmitInfo2* assemble() {

			waitSemaphoreInfoCount = m_waitSemaphoreInfos.size();
			pWaitSemaphoreInfos = nullptr;
			if (waitSemaphoreInfoCount > 0) {
				pWaitSemaphoreInfos = m_waitSemaphoreInfos.data();
			}

			commandBufferInfoCount = m_commandBufferInfos.size();
			pCommandBufferInfos = nullptr;
			if (commandBufferInfoCount > 0) {
				pCommandBufferInfos = m_commandBufferInfos.data();
			}

			signalSemaphoreInfoCount = m_signalSemaphoreInfos.size();
			pSignalSemaphoreInfos = nullptr;
			if (signalSemaphoreInfoCount > 0) {
				pSignalSemaphoreInfos = m_signalSemaphoreInfos.data();
			}

			return this;
		}

	};

	class PresentInfo : public VkPresentInfoKHR {

		//	TODO: modify to handle more semaphores?
		VkSemaphore	m_vkSemaphoreWait = nullptr;
		VkSwapchainKHR	m_vkSwapchain = nullptr;
		uint32_t		m_swapchainImageIndex = 0;

	public:

		PresentInfo()
			: VkPresentInfoKHR{} {
			sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		}

		PresentInfo* operator&() = delete;

		void addWaitSemaphore(vkcpp::Semaphore semaphore) {
			m_vkSemaphoreWait = semaphore;
		}

		void addSwapchain(
			VkSwapchainKHR vkSwapchain,
			int	swapchainImageIndex
		) {
			m_vkSwapchain = vkSwapchain;
			m_swapchainImageIndex = swapchainImageIndex;
		}

		VkPresentInfoKHR* assemble() {
			pWaitSemaphores = nullptr;	//	Hygiene
			if (m_vkSemaphoreWait) {
				waitSemaphoreCount = 1;
				pWaitSemaphores = &m_vkSemaphoreWait;
			}

			pSwapchains = nullptr;
			pImageIndices = nullptr;
			if (m_vkSwapchain) {
				swapchainCount = 1;
				pSwapchains = &m_vkSwapchain;
				pImageIndices = &m_swapchainImageIndex;
			}

			pResults = nullptr;

			return this;
		}

	};



	class Queue : public HandleWithOwner<VkQueue, Device> {

	public:

		Queue()
			: HandleWithOwner{} {
		}

		Queue(VkQueue vkQueue, Device device)
			: HandleWithOwner(vkQueue, device, nullptr) {
		}

		Queue(const Queue& other)
			: HandleWithOwner(other) {
		}

		Queue& operator=(const Queue& other) {
			HandleWithOwner::operator=(other);
			return *this;
		}

		void waitIdle() {
			vkQueueWaitIdle(*this);
		}

		//void submit(CommandBuffer commandBuffer) {
		//	SubmitInfo submitInfo;
		//	submitInfo.addCommandBuffer(commandBuffer);
		//	VkResult vkResult = vkQueueSubmit(*this, 1, submitInfo.assemble(), VK_NULL_HANDLE);
		//	if (vkResult != VK_SUCCESS) {
		//		throw Exception(vkResult);
		//	}
		//}

		//void submit(CommandBuffer commandBuffer, vkcpp::Fence fence) {
		//	SubmitInfo submitInfo;
		//	submitInfo.addCommandBuffer(commandBuffer);
		//	VkResult vkResult = vkQueueSubmit(*this, 1, submitInfo.assemble(), fence);
		//	if (vkResult != VK_SUCCESS) {
		//		throw Exception(vkResult);
		//	}
		//}


		//void submit(SubmitInfo& submitInfo, Fence fence) {
		//	VkResult vkResult = vkQueueSubmit(*this, 1, submitInfo.assemble(), fence);
		//	if (vkResult != VK_SUCCESS) {
		//		throw Exception(vkResult);
		//	}
		//}

		void submit2(CommandBuffer commandBuffer) const {
			SubmitInfo2 submitInfo2;
			submitInfo2.addCommandBuffer(commandBuffer);
			VkResult vkResult = vkQueueSubmit2(*this, 1, submitInfo2.assemble(), VK_NULL_HANDLE);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
		}

		void submit2(CommandBuffer commandBuffer, Fence fence) const {
			SubmitInfo2 submitInfo2;
			submitInfo2.addCommandBuffer(commandBuffer);
			VkResult vkResult = vkQueueSubmit2(*this, 1, submitInfo2.assemble(), fence);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
		}

		void submit2(SubmitInfo2& submitInfo2, Fence fence) const {
			VkResult vkResult = vkQueueSubmit2(*this, 1, submitInfo2.assemble(), fence);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
		}




		VkResult present(PresentInfo& presentInfo) {
			VkResult vkResult = vkQueuePresentKHR(*this, presentInfo.assemble());
			if (vkResult == VK_SUCCESS) {
				return vkResult;
			}
			if (vkResult == VK_SUBOPTIMAL_KHR) {
				return vkResult;
			}

			throw Exception(vkResult);
		}
	};

	Queue Device::getDeviceQueue(int deviceQueueFamily, int deviceQueueIndex) const {
		VkQueue vkQueue;
		vkGetDeviceQueue(m_handle, deviceQueueFamily, deviceQueueIndex, &vkQueue);
		if (vkQueue == nullptr) {
			throw Exception(VK_ERROR_UNKNOWN);
		}
		return Queue(vkQueue, *this);
	}


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
			pPoolSizes = nullptr;
			if (poolSizeCount > 0) {
				pPoolSizes = m_poolSizes.data();
			}

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
			pBindings = nullptr;
			if (bindingCount > 0) {
				pBindings = m_bindings.data();
			}
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

	class PipelineLayoutCreateInfo : public VkPipelineLayoutCreateInfo {

		std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;

	public:

		VkPipelineLayoutCreateInfo* operator&() = delete;

		PipelineLayoutCreateInfo()
			: VkPipelineLayoutCreateInfo() {
			sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		}

		void addDescriptorSetLayout(
			vkcpp::DescriptorSetLayout descriptorSetLayout
		) {
			m_descriptorSetLayouts.push_back(descriptorSetLayout);
		}

		VkPipelineLayoutCreateInfo* assemble() {

			setLayoutCount = (uint32_t)m_descriptorSetLayouts.size();
			pSetLayouts = nullptr;
			if (setLayoutCount > 0) {
				pSetLayouts = m_descriptorSetLayouts.data();
			}
			return this;

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
		PipelineLayout(PipelineLayoutCreateInfo& pipelineLayoutCreateInfo, VkDevice vkDevice) {
			VkPipelineLayout vkPipelineLayout;
			VkResult vkResult = vkCreatePipelineLayout(vkDevice, pipelineLayoutCreateInfo.assemble(), nullptr, &vkPipelineLayout);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			new(this)PipelineLayout(vkPipelineLayout, vkDevice, &destroy);
		}

	};

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

		//	Default create with reasonable settings.
		PipelineRasterizationStateCreateInfo()
			: VkPipelineRasterizationStateCreateInfo{} {
			sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			polygonMode = VK_POLYGON_MODE_FILL;
			lineWidth = 1.0f;
			//cullMode = VK_CULL_MODE_BACK_BIT;
			cullMode = VK_CULL_MODE_NONE;
			frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		}
	};

	class PipelineMultisampleStateCreateInfo : public VkPipelineMultisampleStateCreateInfo {

	public:

		//	Default create with reasonable settings.
		PipelineMultisampleStateCreateInfo()
			: VkPipelineMultisampleStateCreateInfo{} {
			sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			minSampleShading = 1.0f;
		}

	};


	class PipelineColorBlendAttachmentState : public VkPipelineColorBlendAttachmentState {

	public:

		//	Default create with reasonable settings.
		PipelineColorBlendAttachmentState()
			: VkPipelineColorBlendAttachmentState{} {
			colorWriteMask
				= VK_COLOR_COMPONENT_R_BIT
				| VK_COLOR_COMPONENT_G_BIT
				| VK_COLOR_COMPONENT_B_BIT
				| VK_COLOR_COMPONENT_A_BIT;
			blendEnable = VK_FALSE;
			srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
			dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			colorBlendOp = VK_BLEND_OP_ADD;
			srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			alphaBlendOp = VK_BLEND_OP_ADD;
		}
	};


	class PipelineColorBlendStateCreateInfo : public VkPipelineColorBlendStateCreateInfo {

	public:

		//	Default create with reasonable settings.
		PipelineColorBlendStateCreateInfo()
			: VkPipelineColorBlendStateCreateInfo{} {
			sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			logicOpEnable = VK_FALSE;
			logicOp = VK_LOGIC_OP_COPY;
		}
	};


	class VertexBinding {

	public:

		VkVertexInputBindingDescription m_vkVertexInputBindingDescription{};
		std::vector<VkVertexInputAttributeDescription>	m_vkVertexInputAttributeDescriptions;

		void addVertexInputAttributeDescription(
			int	bindingIndex,
			int	location,
			VkFormat vkFormat,
			uint32_t	offset
		) {
			VkVertexInputAttributeDescription vkVertexInputAttributeDescription{};
			vkVertexInputAttributeDescription.binding = bindingIndex;
			vkVertexInputAttributeDescription.location = location;
			vkVertexInputAttributeDescription.format = vkFormat;
			vkVertexInputAttributeDescription.offset = offset;
			m_vkVertexInputAttributeDescriptions.push_back(vkVertexInputAttributeDescription);
		}

	};

	class PipelineDepthStencilStateCreateInfo : public VkPipelineDepthStencilStateCreateInfo {

	public:

		PipelineDepthStencilStateCreateInfo()
			: VkPipelineDepthStencilStateCreateInfo{} {
			//	Reasonable defaults
			sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthTestEnable = VK_TRUE;
			depthWriteEnable = VK_TRUE;
			depthCompareOp = VK_COMPARE_OP_LESS;	// Less depth means in front of
			depthBoundsTestEnable = VK_FALSE;
			stencilTestEnable = VK_FALSE;
		}

	};
	static_assert(sizeof(PipelineDepthStencilStateCreateInfo) == sizeof(VkPipelineDepthStencilStateCreateInfo));


	class PipelineDynamicStateCreateInfo : public VkPipelineDynamicStateCreateInfo {

		std::vector<VkDynamicState> m_dynamicStates;

	public:

		VkPipelineDynamicStateCreateInfo* operator&() = delete;

		PipelineDynamicStateCreateInfo()
			: VkPipelineDynamicStateCreateInfo{} {
		}

		void addDynamicState(VkDynamicState vkDynamicState) {
			m_dynamicStates.push_back(vkDynamicState);
		}

		VkPipelineDynamicStateCreateInfo* assemble() {
			sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicStateCount = static_cast<uint32_t>(m_dynamicStates.size());
			pDynamicStates = nullptr;
			if (dynamicStateCount > 0) {
				pDynamicStates = m_dynamicStates.data();
			}
			return this;
		}



	};


	class GraphicsPipelineCreateInfo {

		std::vector<VkPipelineShaderStageCreateInfo> m_shaderStageCreateInfos;

		PipelineInputAssemblyStateCreateInfo m_inputAssemblyStateCreateInfo{ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST };

		VkPipelineVertexInputStateCreateInfo m_vkPipelineVertexInputStateCreateInfo{};
		std::vector<VkVertexInputBindingDescription>	m_vertexInputBindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> m_vertexInputAttributeDescriptions;


		PipelineDynamicStateCreateInfo m_pipelineDynamicStateCreateInfo;

		VkViewport m_viewport{};
		VkRect2D m_scissor{};

		VkPipelineViewportStateCreateInfo m_viewportState{};

		PipelineRasterizationStateCreateInfo m_pipelineRasterizationStateCreateInfo;
		PipelineMultisampleStateCreateInfo m_pipelineMultisampleStateCreateInfo;
		PipelineColorBlendAttachmentState m_pipelineColorBlendAttachmentState;
		PipelineColorBlendStateCreateInfo m_pipelineColorBlendStateCreateInfo;
		PipelineDepthStencilStateCreateInfo m_vkPipelineDepthStencilStateCreateInfo;

		VkGraphicsPipelineCreateInfo m_vkGraphicsPipelineCreateInfo{};

		PipelineLayout	m_pipelineLayout;
		RenderPass		m_renderPass;
		int				m_subpassNumber;

	public:

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


		void addVertexBinding(const VertexBinding& vertexBinding) {
			m_vertexInputBindingDescriptions.push_back(vertexBinding.m_vkVertexInputBindingDescription);
			for (const VkVertexInputAttributeDescription& vkVertexInputAttributeDescription
				: vertexBinding.m_vkVertexInputAttributeDescriptions) {
				m_vertexInputAttributeDescriptions.push_back(vkVertexInputAttributeDescription);
			}
		}

		void addDynamicState(VkDynamicState vkDynamicState) {
			m_pipelineDynamicStateCreateInfo.addDynamicState(vkDynamicState);
		}

		void setViewportExtent(VkExtent2D extent) {
			m_viewport.width = static_cast<float>(extent.width);
			m_viewport.height = static_cast<float>(extent.height);
		}

		void setScissorExtent(VkExtent2D extent) {
			m_scissor.extent = extent;
		}

		void setPipelineLayout(PipelineLayout pipelineLayout) {
			m_pipelineLayout = pipelineLayout;
		}

		void setRenderPass(RenderPass renderPass, int subpassNumber) {
			m_renderPass = renderPass;
			m_subpassNumber = subpassNumber;
		}


		//	A bit dodgy.  Returning pointer to internal member.
		//	Should just be used to create pipeline.
		VkGraphicsPipelineCreateInfo* assemble() {

			//	TODO: need to clear out create info in case we are called twice.

			//	Assemble pipeline create info
			m_vkGraphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

			//	Shaders
			m_vkGraphicsPipelineCreateInfo.stageCount = static_cast<uint32_t>(m_shaderStageCreateInfos.size());
			if (m_vkGraphicsPipelineCreateInfo.stageCount > 0) {
				m_vkGraphicsPipelineCreateInfo.pStages = m_shaderStageCreateInfos.data();
			}

			//	Just assemble this create structure in place.
			//	Doesn't seem to be a need to make the create structure independent right now.
			//	TODO: maybe make independent when more vertex info?  This is getting messy.
			m_vkPipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

			m_vkPipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(m_vertexInputBindingDescriptions.size());
			if (m_vkPipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount) {
				m_vkPipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = m_vertexInputBindingDescriptions.data();
			}

			m_vkPipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_vertexInputAttributeDescriptions.size());
			if (m_vkPipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount) {
				m_vkPipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = m_vertexInputAttributeDescriptions.data();
			}

			m_vkGraphicsPipelineCreateInfo.pVertexInputState = &m_vkPipelineVertexInputStateCreateInfo;

			m_vkGraphicsPipelineCreateInfo.pInputAssemblyState = &m_inputAssemblyStateCreateInfo;

			m_viewport.maxDepth = 1.0f;
			//TODO make the viewportState smarter?
			m_viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			m_viewportState.viewportCount = 1;
			m_viewportState.pViewports = &m_viewport;
			m_viewportState.scissorCount = 1;
			m_viewportState.pScissors = &m_scissor;
			m_vkGraphicsPipelineCreateInfo.pViewportState = &m_viewportState;

			m_vkGraphicsPipelineCreateInfo.pRasterizationState = &m_pipelineRasterizationStateCreateInfo;

			m_vkGraphicsPipelineCreateInfo.pMultisampleState = &m_pipelineMultisampleStateCreateInfo;

			m_vkGraphicsPipelineCreateInfo.pDepthStencilState = nullptr;

			m_pipelineColorBlendStateCreateInfo.attachmentCount = 1;
			m_pipelineColorBlendStateCreateInfo.pAttachments = &m_pipelineColorBlendAttachmentState;
			m_vkGraphicsPipelineCreateInfo.pColorBlendState = &m_pipelineColorBlendStateCreateInfo;

			m_vkGraphicsPipelineCreateInfo.pDynamicState = m_pipelineDynamicStateCreateInfo.assemble();


			m_vkGraphicsPipelineCreateInfo.pDepthStencilState = &m_vkPipelineDepthStencilStateCreateInfo;


			m_vkGraphicsPipelineCreateInfo.layout = m_pipelineLayout;
			m_vkGraphicsPipelineCreateInfo.renderPass = m_renderPass;
			m_vkGraphicsPipelineCreateInfo.subpass = m_subpassNumber;
			m_vkGraphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
			m_vkGraphicsPipelineCreateInfo.basePipelineIndex = -1; // Optional

			return &m_vkGraphicsPipelineCreateInfo;
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
		GraphicsPipeline(GraphicsPipelineCreateInfo& pipelineCreateInfo, VkDevice vkDevice) {
			VkPipeline vkPipeline;
			VkResult vkResult = vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, pipelineCreateInfo.assemble(), nullptr, &vkPipeline);
			if (vkResult != VK_SUCCESS) {
				throw Exception(vkResult);
			}
			new(this)GraphicsPipeline(vkPipeline, vkDevice, &destroy);
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

		//	TODO: need to start remembering some info about the image and how the
		//	imageview was created to make things easier to use later.
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
		bool m_swapchainUpToDate{};
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
				std::vector<VkImageView> attachments = {
					m_swapchainImageViews[i]
				};
				attachments.push_back(m_depthBuffer.m_imageView);

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
			const VkExtent2D surfaceExtent = vkSurfaceCapabilities.currentExtent;
			//	Can't make "real" swapchains with 0 width or height, e.g.,
			//	the window is minimized.  Return a "null" swapchain if
			//	this occurs.
			if (surfaceExtent.width == 0 || surfaceExtent.height == 0) {
				return Swapchain{};
			}

			vkSwapchainCreateInfo.surface = surface;
			vkSwapchainCreateInfo.imageExtent = surfaceExtent;
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


		VkSwapchainKHR vkSwapchain() {
			VkSwapchainKHR vkSwapchain = m_swapchain;	// Need to force a type conversion for return value.
			return vkSwapchain;
		}

		bool canDraw() {
			if (m_swapchain && m_swapchainUpToDate) {
				return true;
			}
			recreateSwapchainImageViewsFrameBuffers();
			return m_swapchain;
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
			m_swapchainUpToDate = false;
			if (!s_device) {
				return;
			}

			vkDeviceWaitIdle(s_device);
			destroyFrameBuffers();
			destroyImageViews();

			//	TODO: can we set the old swapchain to avoid this?
			//	Explicitly destroy the old swapchain for now.
			m_swapchain = std::move(vkcpp::Swapchain());

			m_swapchain = std::move(createSwapchain(m_vkSwapchainCreateInfo, m_surface));
			if (!m_swapchain) {
				return;
			}
			createSwapchainImageViews(m_vkSwapchainCreateInfo.imageFormat);
			m_depthBuffer = std::move(createDepthBuffer(m_vkSwapchainCreateInfo.imageExtent, s_device));
			createSwapchainFrameBuffers();
			m_swapchainUpToDate = true;
		}

		void stale() {
			m_swapchainUpToDate = false;
		}

		//void recreateIfStale() {
		//	if (!m_stale) {
		//		return;
		//	}
		//	recreateSwapchainImageViewsFrameBuffers();
		//	m_stale = false;
		//}

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




