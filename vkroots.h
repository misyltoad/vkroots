// The vkroots.h header is licensed under Apache-2.0 OR MIT
// as it was generated from the Vulkan Registry, which is licensed
// under the same license.

#pragma once

#include <vulkan/vk_layer.h>
#include <vulkan/vulkan.h>

#include <cstring>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <type_traits>
#include <memory>
#include <cassert>
#include <vector>
#include <utility>
#include <optional>
#include <string_view>
#include <array>

#define VKROOTS_VERSION_MAJOR 0
#define VKROOTS_VERSION_MINOR 1
#define VKROOTS_VERSION_PATCH 0

#define VKROOTS_VERSION VK_MAKE_API_VERSION(0, VKROOTS_VERSION_MAJOR, VKROOTS_VERSION_MINOR, VKROOTS_VERSION_PATCH)

namespace vkroots {

  // Consistency!
  using PFN_vkGetPhysicalDeviceProcAddr = PFN_GetPhysicalDeviceProcAddr;

  class VkInstanceDispatch;
  class VkPhysicalDeviceDispatch;
  class VkDeviceDispatch;

  class NoOverrides { static constexpr bool IsNoOverrides = true; };

  struct VkStructHeader {
    VkStructureType sType;
    void*           pNext;
  };

  template <VkStructureType SType, typename Type, typename AnyStruct>
  Type* FindInChain(AnyStruct* obj) {
    for (const VkStructHeader* header = reinterpret_cast<const VkStructHeader*>(obj); header; header = reinterpret_cast<const VkStructHeader*>(header->pNext)) {
      if (header->sType == SType)
        return reinterpret_cast<Type*>(header);
    }
    return nullptr;
  }

  namespace tables {

    template <typename T>
    class RawPointer {
    public:
      RawPointer(T* value) : m_value(value) {}
            T* get()       { return m_value; }
      const T* get() const { return m_value; }
    private:
      T* m_value;
    };

    template <typename Object, typename DispatchType, typename DispatchPtr>
    class VkDispatchTableMap {
    public:
      const DispatchType* insert(Object obj, DispatchPtr ptr) {
        assert(obj);
        auto lock = std::unique_lock(m_mutex);
        const DispatchType* val = ptr.get();
        m_map.insert(std::make_pair(obj, std::move(ptr)));
        return val;
      }
      void remove(Object obj) {
        assert(obj);
        auto lock = std::unique_lock(m_mutex);
        m_map.erase(obj);
      }
      const DispatchType* find(Object obj) const {
        if (!obj) return nullptr;
        auto lock = std::unique_lock(m_mutex);
        auto iter = m_map.find(obj);
        if (iter == m_map.end())
          return nullptr;
        return iter->second.get();
      }
    private:
      std::unordered_map<Object, DispatchPtr> m_map;
      mutable std::mutex m_mutex;
    };

    // All our dispatchables...
    inline VkDispatchTableMap<VkInstance,       VkInstanceDispatch,       std::unique_ptr<const VkInstanceDispatch>>       InstanceDispatches;
    inline VkDispatchTableMap<VkInstance,       VkPhysicalDeviceDispatch, std::unique_ptr<const VkPhysicalDeviceDispatch>> PhysicalDeviceInstanceDispatches;
    inline VkDispatchTableMap<VkPhysicalDevice, VkPhysicalDeviceDispatch, RawPointer     <const VkPhysicalDeviceDispatch>> PhysicalDeviceDispatches;
    inline VkDispatchTableMap<VkDevice,         VkDeviceDispatch,         std::unique_ptr<const VkDeviceDispatch>>         DeviceDispatches;
    inline VkDispatchTableMap<VkQueue,          VkDeviceDispatch,         RawPointer     <const VkDeviceDispatch>>         QueueDispatches;
    inline VkDispatchTableMap<VkCommandBuffer,  VkDeviceDispatch,         RawPointer     <const VkDeviceDispatch>>         CommandBufferDispatches;

    static inline const VkInstanceDispatch*       LookupInstanceDispatch      (VkInstance instance)             { return InstanceDispatches.find(instance); }
    static inline const VkPhysicalDeviceDispatch* LookupPhysicalDeviceDispatch(VkInstance instance)             { return PhysicalDeviceInstanceDispatches.find(instance); }
    static inline const VkPhysicalDeviceDispatch* LookupPhysicalDeviceDispatch(VkPhysicalDevice physicalDevice) { return PhysicalDeviceDispatches.find(physicalDevice); }
    static inline const VkDeviceDispatch*         LookupDeviceDispatch        (VkDevice device)                 { return DeviceDispatches.find(device); }
    static inline const VkDeviceDispatch*         LookupDeviceDispatch        (VkQueue device)                  { return QueueDispatches.find(device); }
    static inline const VkDeviceDispatch*         LookupDeviceDispatch        (VkCommandBuffer cmdBuffer)       { return CommandBufferDispatches.find(cmdBuffer); }

    static inline void CreateDispatchTable(PFN_vkGetInstanceProcAddr nextInstanceProcAddr, PFN_GetPhysicalDeviceProcAddr nextPhysDevProcAddr, VkInstance instance);
    static inline void CreateDispatchTable(const VkDeviceCreateInfo* pCreateInfo, PFN_vkGetDeviceProcAddr nextProcAddr, VkPhysicalDevice physicalDevice, VkDevice device);
    static inline void DestroyDispatchTable(VkInstance instance);
    static inline void DestroyDispatchTable(VkDevice device);
  }

  struct VkInstanceProcAddrFuncs {
    PFN_vkGetInstanceProcAddr NextGetInstanceProcAddr;
    PFN_vkGetPhysicalDeviceProcAddr NextGetPhysicalDeviceProcAddr;
  };

  static inline VkResult GetProcAddrs(const VkInstanceCreateInfo* pInfo, VkInstanceProcAddrFuncs *pOutFuncs) {
    const void* pNext = (const void*) pInfo;
    const VkLayerInstanceCreateInfo* layerInfo;
    while ((layerInfo = FindInChain<VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO, const VkLayerInstanceCreateInfo>(pNext)) && layerInfo->function != VK_LAYER_LINK_INFO)
      pNext = layerInfo->pNext;
    assert(layerInfo);
    if (!layerInfo)
      return VK_ERROR_INITIALIZATION_FAILED;
    *pOutFuncs = VkInstanceProcAddrFuncs{ layerInfo->u.pLayerInfo->pfnNextGetInstanceProcAddr, layerInfo->u.pLayerInfo->pfnNextGetPhysicalDeviceProcAddr };
    // Josh:
    // It really sucks that we have to advance this ourselves given the const situation here... 
    VkLayerInstanceCreateInfo* layerInfoMutable = const_cast<VkLayerInstanceCreateInfo *>(layerInfo);
    layerInfoMutable->u.pLayerInfo = layerInfoMutable->u.pLayerInfo->pNext;
    return VK_SUCCESS;
  }

  static inline VkResult GetProcAddrs(const VkDeviceCreateInfo* pInfo, PFN_vkGetDeviceProcAddr *pOutAddr) {
    const void* pNext = (const void*) pInfo;
    const VkLayerDeviceCreateInfo* layerInfo;
    while ((layerInfo = FindInChain<VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO, const VkLayerDeviceCreateInfo>(pNext)) && layerInfo->function != VK_LAYER_LINK_INFO)
      pNext = layerInfo->pNext;
    assert(layerInfo);
    if (!layerInfo)
      return VK_ERROR_INITIALIZATION_FAILED;
    *pOutAddr = layerInfo->u.pLayerInfo->pfnNextGetDeviceProcAddr;
    // Josh:
    // It really sucks that we have to advance this ourselves given the const situation here... 
    VkLayerDeviceCreateInfo* layerInfoMutable = const_cast<VkLayerDeviceCreateInfo *>(layerInfo);
    layerInfoMutable->u.pLayerInfo = layerInfoMutable->u.pLayerInfo->pNext;
    return VK_SUCCESS;
  }

}
namespace vkroots {
  class VkInstanceDispatch {
  public:
    VkInstanceDispatch(PFN_vkGetInstanceProcAddr NextGetInstanceProcAddr, VkInstance instance) {
      this->Instance = instance;
#ifdef VK_USE_PLATFORM_ANDROID_KHR
      CreateAndroidSurfaceKHR = (PFN_vkCreateAndroidSurfaceKHR) NextGetInstanceProcAddr(instance, "vkCreateAndroidSurfaceKHR");
#endif
      CreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT) NextGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
      CreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) NextGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
      CreateDevice = (PFN_vkCreateDevice) NextGetInstanceProcAddr(instance, "vkCreateDevice");
#ifdef VK_USE_PLATFORM_DIRECTFB_EXT
      CreateDirectFBSurfaceEXT = (PFN_vkCreateDirectFBSurfaceEXT) NextGetInstanceProcAddr(instance, "vkCreateDirectFBSurfaceEXT");
#endif
      CreateDisplayModeKHR = (PFN_vkCreateDisplayModeKHR) NextGetInstanceProcAddr(instance, "vkCreateDisplayModeKHR");
      CreateDisplayPlaneSurfaceKHR = (PFN_vkCreateDisplayPlaneSurfaceKHR) NextGetInstanceProcAddr(instance, "vkCreateDisplayPlaneSurfaceKHR");
      CreateHeadlessSurfaceEXT = (PFN_vkCreateHeadlessSurfaceEXT) NextGetInstanceProcAddr(instance, "vkCreateHeadlessSurfaceEXT");
#ifdef VK_USE_PLATFORM_IOS_MVK
      CreateIOSSurfaceMVK = (PFN_vkCreateIOSSurfaceMVK) NextGetInstanceProcAddr(instance, "vkCreateIOSSurfaceMVK");
#endif
#ifdef VK_USE_PLATFORM_FUCHSIA
      CreateImagePipeSurfaceFUCHSIA = (PFN_vkCreateImagePipeSurfaceFUCHSIA) NextGetInstanceProcAddr(instance, "vkCreateImagePipeSurfaceFUCHSIA");
#endif
      CreateInstance = (PFN_vkCreateInstance) NextGetInstanceProcAddr(instance, "vkCreateInstance");
#ifdef VK_USE_PLATFORM_MACOS_MVK
      CreateMacOSSurfaceMVK = (PFN_vkCreateMacOSSurfaceMVK) NextGetInstanceProcAddr(instance, "vkCreateMacOSSurfaceMVK");
#endif
#ifdef VK_USE_PLATFORM_METAL_EXT
      CreateMetalSurfaceEXT = (PFN_vkCreateMetalSurfaceEXT) NextGetInstanceProcAddr(instance, "vkCreateMetalSurfaceEXT");
#endif
#ifdef VK_USE_PLATFORM_SCREEN_QNX
      CreateScreenSurfaceQNX = (PFN_vkCreateScreenSurfaceQNX) NextGetInstanceProcAddr(instance, "vkCreateScreenSurfaceQNX");
#endif
#ifdef VK_USE_PLATFORM_GGP
      CreateStreamDescriptorSurfaceGGP = (PFN_vkCreateStreamDescriptorSurfaceGGP) NextGetInstanceProcAddr(instance, "vkCreateStreamDescriptorSurfaceGGP");
#endif
#ifdef VK_USE_PLATFORM_VI_NN
      CreateViSurfaceNN = (PFN_vkCreateViSurfaceNN) NextGetInstanceProcAddr(instance, "vkCreateViSurfaceNN");
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
      CreateWaylandSurfaceKHR = (PFN_vkCreateWaylandSurfaceKHR) NextGetInstanceProcAddr(instance, "vkCreateWaylandSurfaceKHR");
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
      CreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR) NextGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
      CreateXcbSurfaceKHR = (PFN_vkCreateXcbSurfaceKHR) NextGetInstanceProcAddr(instance, "vkCreateXcbSurfaceKHR");
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
      CreateXlibSurfaceKHR = (PFN_vkCreateXlibSurfaceKHR) NextGetInstanceProcAddr(instance, "vkCreateXlibSurfaceKHR");
#endif
      DebugReportMessageEXT = (PFN_vkDebugReportMessageEXT) NextGetInstanceProcAddr(instance, "vkDebugReportMessageEXT");
      DestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT) NextGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
      DestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) NextGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
      DestroyInstanceReal = (PFN_vkDestroyInstance) NextGetInstanceProcAddr(instance, "vkDestroyInstance");
      DestroyInstance = (PFN_vkDestroyInstance) DestroyInstanceWrapper;
      DestroySurfaceKHR = (PFN_vkDestroySurfaceKHR) NextGetInstanceProcAddr(instance, "vkDestroySurfaceKHR");
      EnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties) NextGetInstanceProcAddr(instance, "vkEnumerateDeviceExtensionProperties");
      EnumerateDeviceLayerProperties = (PFN_vkEnumerateDeviceLayerProperties) NextGetInstanceProcAddr(instance, "vkEnumerateDeviceLayerProperties");
      EnumeratePhysicalDeviceGroups = (PFN_vkEnumeratePhysicalDeviceGroups) NextGetInstanceProcAddr(instance, "vkEnumeratePhysicalDeviceGroups");
      EnumeratePhysicalDeviceGroupsKHR = (PFN_vkEnumeratePhysicalDeviceGroupsKHR) NextGetInstanceProcAddr(instance, "vkEnumeratePhysicalDeviceGroupsKHR");
      EnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices) NextGetInstanceProcAddr(instance, "vkEnumeratePhysicalDevices");
      GetDisplayModePropertiesKHR = (PFN_vkGetDisplayModePropertiesKHR) NextGetInstanceProcAddr(instance, "vkGetDisplayModePropertiesKHR");
      GetDisplayPlaneCapabilitiesKHR = (PFN_vkGetDisplayPlaneCapabilitiesKHR) NextGetInstanceProcAddr(instance, "vkGetDisplayPlaneCapabilitiesKHR");
      GetDisplayPlaneSupportedDisplaysKHR = (PFN_vkGetDisplayPlaneSupportedDisplaysKHR) NextGetInstanceProcAddr(instance, "vkGetDisplayPlaneSupportedDisplaysKHR");
      GetInstanceProcAddr = NextGetInstanceProcAddr;
      GetPhysicalDeviceDisplayPlanePropertiesKHR = (PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR) NextGetInstanceProcAddr(instance, "vkGetPhysicalDeviceDisplayPlanePropertiesKHR");
      GetPhysicalDeviceDisplayPropertiesKHR = (PFN_vkGetPhysicalDeviceDisplayPropertiesKHR) NextGetInstanceProcAddr(instance, "vkGetPhysicalDeviceDisplayPropertiesKHR");
      GetPhysicalDeviceExternalBufferProperties = (PFN_vkGetPhysicalDeviceExternalBufferProperties) NextGetInstanceProcAddr(instance, "vkGetPhysicalDeviceExternalBufferProperties");
      GetPhysicalDeviceExternalFenceProperties = (PFN_vkGetPhysicalDeviceExternalFenceProperties) NextGetInstanceProcAddr(instance, "vkGetPhysicalDeviceExternalFenceProperties");
      GetPhysicalDeviceExternalSemaphoreProperties = (PFN_vkGetPhysicalDeviceExternalSemaphoreProperties) NextGetInstanceProcAddr(instance, "vkGetPhysicalDeviceExternalSemaphoreProperties");
      GetPhysicalDeviceFeatures = (PFN_vkGetPhysicalDeviceFeatures) NextGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures");
      GetPhysicalDeviceFeatures2 = (PFN_vkGetPhysicalDeviceFeatures2) NextGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures2");
      GetPhysicalDeviceFormatProperties = (PFN_vkGetPhysicalDeviceFormatProperties) NextGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFormatProperties");
      GetPhysicalDeviceFormatProperties2 = (PFN_vkGetPhysicalDeviceFormatProperties2) NextGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFormatProperties2");
      GetPhysicalDeviceImageFormatProperties = (PFN_vkGetPhysicalDeviceImageFormatProperties) NextGetInstanceProcAddr(instance, "vkGetPhysicalDeviceImageFormatProperties");
      GetPhysicalDeviceImageFormatProperties2 = (PFN_vkGetPhysicalDeviceImageFormatProperties2) NextGetInstanceProcAddr(instance, "vkGetPhysicalDeviceImageFormatProperties2");
      GetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties) NextGetInstanceProcAddr(instance, "vkGetPhysicalDeviceMemoryProperties");
      GetPhysicalDeviceMemoryProperties2 = (PFN_vkGetPhysicalDeviceMemoryProperties2) NextGetInstanceProcAddr(instance, "vkGetPhysicalDeviceMemoryProperties2");
      GetPhysicalDevicePresentRectanglesKHR = (PFN_vkGetPhysicalDevicePresentRectanglesKHR) NextGetInstanceProcAddr(instance, "vkGetPhysicalDevicePresentRectanglesKHR");
      GetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties) NextGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties");
      GetPhysicalDeviceProperties2 = (PFN_vkGetPhysicalDeviceProperties2) NextGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties2");
      GetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties) NextGetInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties");
      GetPhysicalDeviceQueueFamilyProperties2 = (PFN_vkGetPhysicalDeviceQueueFamilyProperties2) NextGetInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties2");
      GetPhysicalDeviceSparseImageFormatProperties = (PFN_vkGetPhysicalDeviceSparseImageFormatProperties) NextGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSparseImageFormatProperties");
      GetPhysicalDeviceSparseImageFormatProperties2 = (PFN_vkGetPhysicalDeviceSparseImageFormatProperties2) NextGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSparseImageFormatProperties2");
      GetPhysicalDeviceSurfaceCapabilities2KHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR) NextGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceCapabilities2KHR");
      GetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR) NextGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
      GetPhysicalDeviceSurfaceFormats2KHR = (PFN_vkGetPhysicalDeviceSurfaceFormats2KHR) NextGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceFormats2KHR");
      GetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR) NextGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceFormatsKHR");
      GetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR) NextGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfacePresentModesKHR");
      GetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR) NextGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceSupportKHR");
      GetPhysicalDeviceToolProperties = (PFN_vkGetPhysicalDeviceToolProperties) NextGetInstanceProcAddr(instance, "vkGetPhysicalDeviceToolProperties");
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
      GetPhysicalDeviceWaylandPresentationSupportKHR = (PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR) NextGetInstanceProcAddr(instance, "vkGetPhysicalDeviceWaylandPresentationSupportKHR");
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
      GetPhysicalDeviceWin32PresentationSupportKHR = (PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR) NextGetInstanceProcAddr(instance, "vkGetPhysicalDeviceWin32PresentationSupportKHR");
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
      GetPhysicalDeviceXcbPresentationSupportKHR = (PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR) NextGetInstanceProcAddr(instance, "vkGetPhysicalDeviceXcbPresentationSupportKHR");
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
      GetPhysicalDeviceXlibPresentationSupportKHR = (PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR) NextGetInstanceProcAddr(instance, "vkGetPhysicalDeviceXlibPresentationSupportKHR");
#endif
      SubmitDebugUtilsMessageEXT = (PFN_vkSubmitDebugUtilsMessageEXT) NextGetInstanceProcAddr(instance, "vkSubmitDebugUtilsMessageEXT");
    }

    mutable uint64_t UserData = 0;
    VkInstance Instance;
#ifdef VK_USE_PLATFORM_ANDROID_KHR
    PFN_vkCreateAndroidSurfaceKHR CreateAndroidSurfaceKHR;
#endif
    PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallbackEXT;
    PFN_vkCreateDebugUtilsMessengerEXT CreateDebugUtilsMessengerEXT;
    PFN_vkCreateDevice CreateDevice;
#ifdef VK_USE_PLATFORM_DIRECTFB_EXT
    PFN_vkCreateDirectFBSurfaceEXT CreateDirectFBSurfaceEXT;
#endif
    PFN_vkCreateDisplayModeKHR CreateDisplayModeKHR;
    PFN_vkCreateDisplayPlaneSurfaceKHR CreateDisplayPlaneSurfaceKHR;
    PFN_vkCreateHeadlessSurfaceEXT CreateHeadlessSurfaceEXT;
#ifdef VK_USE_PLATFORM_IOS_MVK
    PFN_vkCreateIOSSurfaceMVK CreateIOSSurfaceMVK;
#endif
#ifdef VK_USE_PLATFORM_FUCHSIA
    PFN_vkCreateImagePipeSurfaceFUCHSIA CreateImagePipeSurfaceFUCHSIA;
#endif
    PFN_vkCreateInstance CreateInstance;
#ifdef VK_USE_PLATFORM_MACOS_MVK
    PFN_vkCreateMacOSSurfaceMVK CreateMacOSSurfaceMVK;
#endif
#ifdef VK_USE_PLATFORM_METAL_EXT
    PFN_vkCreateMetalSurfaceEXT CreateMetalSurfaceEXT;
#endif
#ifdef VK_USE_PLATFORM_SCREEN_QNX
    PFN_vkCreateScreenSurfaceQNX CreateScreenSurfaceQNX;
#endif
#ifdef VK_USE_PLATFORM_GGP
    PFN_vkCreateStreamDescriptorSurfaceGGP CreateStreamDescriptorSurfaceGGP;
#endif
#ifdef VK_USE_PLATFORM_VI_NN
    PFN_vkCreateViSurfaceNN CreateViSurfaceNN;
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    PFN_vkCreateWaylandSurfaceKHR CreateWaylandSurfaceKHR;
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkCreateWin32SurfaceKHR CreateWin32SurfaceKHR;
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
    PFN_vkCreateXcbSurfaceKHR CreateXcbSurfaceKHR;
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
    PFN_vkCreateXlibSurfaceKHR CreateXlibSurfaceKHR;
#endif
    PFN_vkDebugReportMessageEXT DebugReportMessageEXT;
    PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallbackEXT;
    PFN_vkDestroyDebugUtilsMessengerEXT DestroyDebugUtilsMessengerEXT;
    PFN_vkDestroyInstance DestroyInstance;
    PFN_vkDestroySurfaceKHR DestroySurfaceKHR;
    PFN_vkEnumerateDeviceExtensionProperties EnumerateDeviceExtensionProperties;
    PFN_vkEnumerateDeviceLayerProperties EnumerateDeviceLayerProperties;
    PFN_vkEnumeratePhysicalDeviceGroups EnumeratePhysicalDeviceGroups;
    PFN_vkEnumeratePhysicalDeviceGroupsKHR EnumeratePhysicalDeviceGroupsKHR;
    PFN_vkEnumeratePhysicalDevices EnumeratePhysicalDevices;
    PFN_vkGetDisplayModePropertiesKHR GetDisplayModePropertiesKHR;
    PFN_vkGetDisplayPlaneCapabilitiesKHR GetDisplayPlaneCapabilitiesKHR;
    PFN_vkGetDisplayPlaneSupportedDisplaysKHR GetDisplayPlaneSupportedDisplaysKHR;
    PFN_vkGetInstanceProcAddr GetInstanceProcAddr;
    PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR GetPhysicalDeviceDisplayPlanePropertiesKHR;
    PFN_vkGetPhysicalDeviceDisplayPropertiesKHR GetPhysicalDeviceDisplayPropertiesKHR;
    PFN_vkGetPhysicalDeviceExternalBufferProperties GetPhysicalDeviceExternalBufferProperties;
    PFN_vkGetPhysicalDeviceExternalFenceProperties GetPhysicalDeviceExternalFenceProperties;
    PFN_vkGetPhysicalDeviceExternalSemaphoreProperties GetPhysicalDeviceExternalSemaphoreProperties;
    PFN_vkGetPhysicalDeviceFeatures GetPhysicalDeviceFeatures;
    PFN_vkGetPhysicalDeviceFeatures2 GetPhysicalDeviceFeatures2;
    PFN_vkGetPhysicalDeviceFormatProperties GetPhysicalDeviceFormatProperties;
    PFN_vkGetPhysicalDeviceFormatProperties2 GetPhysicalDeviceFormatProperties2;
    PFN_vkGetPhysicalDeviceImageFormatProperties GetPhysicalDeviceImageFormatProperties;
    PFN_vkGetPhysicalDeviceImageFormatProperties2 GetPhysicalDeviceImageFormatProperties2;
    PFN_vkGetPhysicalDeviceMemoryProperties GetPhysicalDeviceMemoryProperties;
    PFN_vkGetPhysicalDeviceMemoryProperties2 GetPhysicalDeviceMemoryProperties2;
    PFN_vkGetPhysicalDevicePresentRectanglesKHR GetPhysicalDevicePresentRectanglesKHR;
    PFN_vkGetPhysicalDeviceProperties GetPhysicalDeviceProperties;
    PFN_vkGetPhysicalDeviceProperties2 GetPhysicalDeviceProperties2;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties GetPhysicalDeviceQueueFamilyProperties;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties2 GetPhysicalDeviceQueueFamilyProperties2;
    PFN_vkGetPhysicalDeviceSparseImageFormatProperties GetPhysicalDeviceSparseImageFormatProperties;
    PFN_vkGetPhysicalDeviceSparseImageFormatProperties2 GetPhysicalDeviceSparseImageFormatProperties2;
    PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR GetPhysicalDeviceSurfaceCapabilities2KHR;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR GetPhysicalDeviceSurfaceCapabilitiesKHR;
    PFN_vkGetPhysicalDeviceSurfaceFormats2KHR GetPhysicalDeviceSurfaceFormats2KHR;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR GetPhysicalDeviceSurfaceFormatsKHR;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR GetPhysicalDeviceSurfacePresentModesKHR;
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR GetPhysicalDeviceSurfaceSupportKHR;
    PFN_vkGetPhysicalDeviceToolProperties GetPhysicalDeviceToolProperties;
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR GetPhysicalDeviceWaylandPresentationSupportKHR;
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR GetPhysicalDeviceWin32PresentationSupportKHR;
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
    PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR GetPhysicalDeviceXcbPresentationSupportKHR;
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
    PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR GetPhysicalDeviceXlibPresentationSupportKHR;
#endif
    PFN_vkSubmitDebugUtilsMessageEXT SubmitDebugUtilsMessageEXT;
  private:
    PFN_vkDestroyInstance DestroyInstanceReal;
    static void DestroyInstanceWrapper(VkInstance object, const VkAllocationCallbacks* pAllocator) {
      auto dispatch = vkroots::tables::LookupInstanceDispatch(object);
      auto destroyFunc = dispatch->DestroyInstanceReal;
      vkroots::tables::DestroyDispatchTable(object);
      destroyFunc(object, pAllocator);
    }
  };

  class VkPhysicalDeviceDispatch {
  public:
    VkPhysicalDeviceDispatch(PFN_vkGetPhysicalDeviceProcAddr NextGetPhysicalDeviceProcAddr, VkInstance instance, const VkInstanceDispatch* pInstanceDispatch) {
      this->Instance = instance;
      this->pInstanceDispatch = pInstanceDispatch;
      this->GetPhysicalDeviceProcAddr = NextGetPhysicalDeviceProcAddr;
      AcquireDrmDisplayEXT = (PFN_vkAcquireDrmDisplayEXT) NextGetPhysicalDeviceProcAddr(instance, "vkAcquireDrmDisplayEXT");
#ifdef VK_USE_PLATFORM_WIN32_KHR
      AcquireWinrtDisplayNV = (PFN_vkAcquireWinrtDisplayNV) NextGetPhysicalDeviceProcAddr(instance, "vkAcquireWinrtDisplayNV");
#endif
#ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
      AcquireXlibDisplayEXT = (PFN_vkAcquireXlibDisplayEXT) NextGetPhysicalDeviceProcAddr(instance, "vkAcquireXlibDisplayEXT");
#endif
      EnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR = (PFN_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR) NextGetPhysicalDeviceProcAddr(instance, "vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR");
      GetDisplayModeProperties2KHR = (PFN_vkGetDisplayModeProperties2KHR) NextGetPhysicalDeviceProcAddr(instance, "vkGetDisplayModeProperties2KHR");
      GetDisplayPlaneCapabilities2KHR = (PFN_vkGetDisplayPlaneCapabilities2KHR) NextGetPhysicalDeviceProcAddr(instance, "vkGetDisplayPlaneCapabilities2KHR");
      GetDrmDisplayEXT = (PFN_vkGetDrmDisplayEXT) NextGetPhysicalDeviceProcAddr(instance, "vkGetDrmDisplayEXT");
      GetPhysicalDeviceCalibrateableTimeDomainsEXT = (PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT) NextGetPhysicalDeviceProcAddr(instance, "vkGetPhysicalDeviceCalibrateableTimeDomainsEXT");
      GetPhysicalDeviceCooperativeMatrixPropertiesNV = (PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV) NextGetPhysicalDeviceProcAddr(instance, "vkGetPhysicalDeviceCooperativeMatrixPropertiesNV");
#ifdef VK_USE_PLATFORM_DIRECTFB_EXT
      GetPhysicalDeviceDirectFBPresentationSupportEXT = (PFN_vkGetPhysicalDeviceDirectFBPresentationSupportEXT) NextGetPhysicalDeviceProcAddr(instance, "vkGetPhysicalDeviceDirectFBPresentationSupportEXT");
#endif
      GetPhysicalDeviceDisplayPlaneProperties2KHR = (PFN_vkGetPhysicalDeviceDisplayPlaneProperties2KHR) NextGetPhysicalDeviceProcAddr(instance, "vkGetPhysicalDeviceDisplayPlaneProperties2KHR");
      GetPhysicalDeviceDisplayProperties2KHR = (PFN_vkGetPhysicalDeviceDisplayProperties2KHR) NextGetPhysicalDeviceProcAddr(instance, "vkGetPhysicalDeviceDisplayProperties2KHR");
      GetPhysicalDeviceExternalBufferPropertiesKHR = (PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR) NextGetPhysicalDeviceProcAddr(instance, "vkGetPhysicalDeviceExternalBufferPropertiesKHR");
      GetPhysicalDeviceExternalFencePropertiesKHR = (PFN_vkGetPhysicalDeviceExternalFencePropertiesKHR) NextGetPhysicalDeviceProcAddr(instance, "vkGetPhysicalDeviceExternalFencePropertiesKHR");
      GetPhysicalDeviceExternalImageFormatPropertiesNV = (PFN_vkGetPhysicalDeviceExternalImageFormatPropertiesNV) NextGetPhysicalDeviceProcAddr(instance, "vkGetPhysicalDeviceExternalImageFormatPropertiesNV");
      GetPhysicalDeviceExternalSemaphorePropertiesKHR = (PFN_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR) NextGetPhysicalDeviceProcAddr(instance, "vkGetPhysicalDeviceExternalSemaphorePropertiesKHR");
      GetPhysicalDeviceFeatures2KHR = (PFN_vkGetPhysicalDeviceFeatures2KHR) NextGetPhysicalDeviceProcAddr(instance, "vkGetPhysicalDeviceFeatures2KHR");
      GetPhysicalDeviceFormatProperties2KHR = (PFN_vkGetPhysicalDeviceFormatProperties2KHR) NextGetPhysicalDeviceProcAddr(instance, "vkGetPhysicalDeviceFormatProperties2KHR");
      GetPhysicalDeviceFragmentShadingRatesKHR = (PFN_vkGetPhysicalDeviceFragmentShadingRatesKHR) NextGetPhysicalDeviceProcAddr(instance, "vkGetPhysicalDeviceFragmentShadingRatesKHR");
      GetPhysicalDeviceImageFormatProperties2KHR = (PFN_vkGetPhysicalDeviceImageFormatProperties2KHR) NextGetPhysicalDeviceProcAddr(instance, "vkGetPhysicalDeviceImageFormatProperties2KHR");
      GetPhysicalDeviceMemoryProperties2KHR = (PFN_vkGetPhysicalDeviceMemoryProperties2KHR) NextGetPhysicalDeviceProcAddr(instance, "vkGetPhysicalDeviceMemoryProperties2KHR");
      GetPhysicalDeviceMultisamplePropertiesEXT = (PFN_vkGetPhysicalDeviceMultisamplePropertiesEXT) NextGetPhysicalDeviceProcAddr(instance, "vkGetPhysicalDeviceMultisamplePropertiesEXT");
      GetPhysicalDeviceProperties2KHR = (PFN_vkGetPhysicalDeviceProperties2KHR) NextGetPhysicalDeviceProcAddr(instance, "vkGetPhysicalDeviceProperties2KHR");
      GetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR = (PFN_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR) NextGetPhysicalDeviceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR");
      GetPhysicalDeviceQueueFamilyProperties2KHR = (PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR) NextGetPhysicalDeviceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties2KHR");
#ifdef VK_USE_PLATFORM_SCREEN_QNX
      GetPhysicalDeviceScreenPresentationSupportQNX = (PFN_vkGetPhysicalDeviceScreenPresentationSupportQNX) NextGetPhysicalDeviceProcAddr(instance, "vkGetPhysicalDeviceScreenPresentationSupportQNX");
#endif
      GetPhysicalDeviceSparseImageFormatProperties2KHR = (PFN_vkGetPhysicalDeviceSparseImageFormatProperties2KHR) NextGetPhysicalDeviceProcAddr(instance, "vkGetPhysicalDeviceSparseImageFormatProperties2KHR");
      GetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV = (PFN_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV) NextGetPhysicalDeviceProcAddr(instance, "vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV");
      GetPhysicalDeviceSurfaceCapabilities2EXT = (PFN_vkGetPhysicalDeviceSurfaceCapabilities2EXT) NextGetPhysicalDeviceProcAddr(instance, "vkGetPhysicalDeviceSurfaceCapabilities2EXT");
#ifdef VK_USE_PLATFORM_WIN32_KHR
      GetPhysicalDeviceSurfacePresentModes2EXT = (PFN_vkGetPhysicalDeviceSurfacePresentModes2EXT) NextGetPhysicalDeviceProcAddr(instance, "vkGetPhysicalDeviceSurfacePresentModes2EXT");
#endif
      GetPhysicalDeviceToolPropertiesEXT = (PFN_vkGetPhysicalDeviceToolPropertiesEXT) NextGetPhysicalDeviceProcAddr(instance, "vkGetPhysicalDeviceToolPropertiesEXT");
#ifdef VK_ENABLE_BETA_EXTENSIONS
      GetPhysicalDeviceVideoCapabilitiesKHR = (PFN_vkGetPhysicalDeviceVideoCapabilitiesKHR) NextGetPhysicalDeviceProcAddr(instance, "vkGetPhysicalDeviceVideoCapabilitiesKHR");
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
      GetPhysicalDeviceVideoFormatPropertiesKHR = (PFN_vkGetPhysicalDeviceVideoFormatPropertiesKHR) NextGetPhysicalDeviceProcAddr(instance, "vkGetPhysicalDeviceVideoFormatPropertiesKHR");
#endif
#ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
      GetRandROutputDisplayEXT = (PFN_vkGetRandROutputDisplayEXT) NextGetPhysicalDeviceProcAddr(instance, "vkGetRandROutputDisplayEXT");
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
      GetWinrtDisplayNV = (PFN_vkGetWinrtDisplayNV) NextGetPhysicalDeviceProcAddr(instance, "vkGetWinrtDisplayNV");
#endif
      ReleaseDisplayEXT = (PFN_vkReleaseDisplayEXT) NextGetPhysicalDeviceProcAddr(instance, "vkReleaseDisplayEXT");
    }

    mutable uint64_t UserData = 0;
    VkInstance Instance;
    const VkInstanceDispatch* pInstanceDispatch;
    PFN_GetPhysicalDeviceProcAddr GetPhysicalDeviceProcAddr;
    PFN_vkAcquireDrmDisplayEXT AcquireDrmDisplayEXT;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkAcquireWinrtDisplayNV AcquireWinrtDisplayNV;
#endif
#ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
    PFN_vkAcquireXlibDisplayEXT AcquireXlibDisplayEXT;
#endif
    PFN_vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR EnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR;
    PFN_vkGetDisplayModeProperties2KHR GetDisplayModeProperties2KHR;
    PFN_vkGetDisplayPlaneCapabilities2KHR GetDisplayPlaneCapabilities2KHR;
    PFN_vkGetDrmDisplayEXT GetDrmDisplayEXT;
    PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT GetPhysicalDeviceCalibrateableTimeDomainsEXT;
    PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesNV GetPhysicalDeviceCooperativeMatrixPropertiesNV;
#ifdef VK_USE_PLATFORM_DIRECTFB_EXT
    PFN_vkGetPhysicalDeviceDirectFBPresentationSupportEXT GetPhysicalDeviceDirectFBPresentationSupportEXT;
#endif
    PFN_vkGetPhysicalDeviceDisplayPlaneProperties2KHR GetPhysicalDeviceDisplayPlaneProperties2KHR;
    PFN_vkGetPhysicalDeviceDisplayProperties2KHR GetPhysicalDeviceDisplayProperties2KHR;
    PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR GetPhysicalDeviceExternalBufferPropertiesKHR;
    PFN_vkGetPhysicalDeviceExternalFencePropertiesKHR GetPhysicalDeviceExternalFencePropertiesKHR;
    PFN_vkGetPhysicalDeviceExternalImageFormatPropertiesNV GetPhysicalDeviceExternalImageFormatPropertiesNV;
    PFN_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR GetPhysicalDeviceExternalSemaphorePropertiesKHR;
    PFN_vkGetPhysicalDeviceFeatures2KHR GetPhysicalDeviceFeatures2KHR;
    PFN_vkGetPhysicalDeviceFormatProperties2KHR GetPhysicalDeviceFormatProperties2KHR;
    PFN_vkGetPhysicalDeviceFragmentShadingRatesKHR GetPhysicalDeviceFragmentShadingRatesKHR;
    PFN_vkGetPhysicalDeviceImageFormatProperties2KHR GetPhysicalDeviceImageFormatProperties2KHR;
    PFN_vkGetPhysicalDeviceMemoryProperties2KHR GetPhysicalDeviceMemoryProperties2KHR;
    PFN_vkGetPhysicalDeviceMultisamplePropertiesEXT GetPhysicalDeviceMultisamplePropertiesEXT;
    PFN_vkGetPhysicalDeviceProperties2KHR GetPhysicalDeviceProperties2KHR;
    PFN_vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR GetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR GetPhysicalDeviceQueueFamilyProperties2KHR;
#ifdef VK_USE_PLATFORM_SCREEN_QNX
    PFN_vkGetPhysicalDeviceScreenPresentationSupportQNX GetPhysicalDeviceScreenPresentationSupportQNX;
#endif
    PFN_vkGetPhysicalDeviceSparseImageFormatProperties2KHR GetPhysicalDeviceSparseImageFormatProperties2KHR;
    PFN_vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV GetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV;
    PFN_vkGetPhysicalDeviceSurfaceCapabilities2EXT GetPhysicalDeviceSurfaceCapabilities2EXT;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkGetPhysicalDeviceSurfacePresentModes2EXT GetPhysicalDeviceSurfacePresentModes2EXT;
#endif
    PFN_vkGetPhysicalDeviceToolPropertiesEXT GetPhysicalDeviceToolPropertiesEXT;
#ifdef VK_ENABLE_BETA_EXTENSIONS
    PFN_vkGetPhysicalDeviceVideoCapabilitiesKHR GetPhysicalDeviceVideoCapabilitiesKHR;
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
    PFN_vkGetPhysicalDeviceVideoFormatPropertiesKHR GetPhysicalDeviceVideoFormatPropertiesKHR;
#endif
#ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
    PFN_vkGetRandROutputDisplayEXT GetRandROutputDisplayEXT;
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkGetWinrtDisplayNV GetWinrtDisplayNV;
#endif
    PFN_vkReleaseDisplayEXT ReleaseDisplayEXT;
  private:
  };

  namespace tables {
    static inline const VkInstanceDispatch* LookupInstanceDispatch(VkPhysicalDevice physicalDevice) { return PhysicalDeviceDispatches.find(physicalDevice)->pInstanceDispatch; }
  }
  class VkDeviceDispatch {
  public:
    VkDeviceDispatch(PFN_vkGetDeviceProcAddr NextGetDeviceProcAddr, VkDevice device, VkPhysicalDevice PhysicalDevice, const VkPhysicalDeviceDispatch* pPhysicalDeviceDispatch, const VkDeviceCreateInfo* pCreateInfo) {
      this->PhysicalDevice = PhysicalDevice;
      this->Device = device;
      this->pPhysicalDeviceDispatch = pPhysicalDeviceDispatch;
      for (uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; i++) {
        VkDeviceQueueCreateInfo queueInfo = pCreateInfo->pQueueCreateInfos[i];
        queueInfo.pNext = nullptr;
        DeviceQueueInfos.push_back(queueInfo);
      }
#ifdef VK_USE_PLATFORM_WIN32_KHR
      AcquireFullScreenExclusiveModeEXT = (PFN_vkAcquireFullScreenExclusiveModeEXT) NextGetDeviceProcAddr(device, "vkAcquireFullScreenExclusiveModeEXT");
#endif
#ifdef VKROOTS_PLATFORM_UNSUPPORTED
      AcquireImageANDROID = (PFN_vkAcquireImageANDROID) NextGetDeviceProcAddr(device, "vkAcquireImageANDROID");
#endif
      AcquireNextImage2KHR = (PFN_vkAcquireNextImage2KHR) NextGetDeviceProcAddr(device, "vkAcquireNextImage2KHR");
      AcquireNextImageKHR = (PFN_vkAcquireNextImageKHR) NextGetDeviceProcAddr(device, "vkAcquireNextImageKHR");
      AcquirePerformanceConfigurationINTEL = (PFN_vkAcquirePerformanceConfigurationINTEL) NextGetDeviceProcAddr(device, "vkAcquirePerformanceConfigurationINTEL");
      AcquireProfilingLockKHR = (PFN_vkAcquireProfilingLockKHR) NextGetDeviceProcAddr(device, "vkAcquireProfilingLockKHR");
      AllocateCommandBuffers = (PFN_vkAllocateCommandBuffers) NextGetDeviceProcAddr(device, "vkAllocateCommandBuffers");
      AllocateDescriptorSets = (PFN_vkAllocateDescriptorSets) NextGetDeviceProcAddr(device, "vkAllocateDescriptorSets");
      AllocateMemory = (PFN_vkAllocateMemory) NextGetDeviceProcAddr(device, "vkAllocateMemory");
      BeginCommandBuffer = (PFN_vkBeginCommandBuffer) NextGetDeviceProcAddr(device, "vkBeginCommandBuffer");
      BindAccelerationStructureMemoryNV = (PFN_vkBindAccelerationStructureMemoryNV) NextGetDeviceProcAddr(device, "vkBindAccelerationStructureMemoryNV");
      BindBufferMemory = (PFN_vkBindBufferMemory) NextGetDeviceProcAddr(device, "vkBindBufferMemory");
      BindBufferMemory2 = (PFN_vkBindBufferMemory2) NextGetDeviceProcAddr(device, "vkBindBufferMemory2");
      BindBufferMemory2KHR = (PFN_vkBindBufferMemory2KHR) NextGetDeviceProcAddr(device, "vkBindBufferMemory2KHR");
      BindImageMemory = (PFN_vkBindImageMemory) NextGetDeviceProcAddr(device, "vkBindImageMemory");
      BindImageMemory2 = (PFN_vkBindImageMemory2) NextGetDeviceProcAddr(device, "vkBindImageMemory2");
      BindImageMemory2KHR = (PFN_vkBindImageMemory2KHR) NextGetDeviceProcAddr(device, "vkBindImageMemory2KHR");
#ifdef VK_ENABLE_BETA_EXTENSIONS
      BindVideoSessionMemoryKHR = (PFN_vkBindVideoSessionMemoryKHR) NextGetDeviceProcAddr(device, "vkBindVideoSessionMemoryKHR");
#endif
      BuildAccelerationStructuresKHR = (PFN_vkBuildAccelerationStructuresKHR) NextGetDeviceProcAddr(device, "vkBuildAccelerationStructuresKHR");
      CmdBeginConditionalRenderingEXT = (PFN_vkCmdBeginConditionalRenderingEXT) NextGetDeviceProcAddr(device, "vkCmdBeginConditionalRenderingEXT");
      CmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT) NextGetDeviceProcAddr(device, "vkCmdBeginDebugUtilsLabelEXT");
      CmdBeginQuery = (PFN_vkCmdBeginQuery) NextGetDeviceProcAddr(device, "vkCmdBeginQuery");
      CmdBeginQueryIndexedEXT = (PFN_vkCmdBeginQueryIndexedEXT) NextGetDeviceProcAddr(device, "vkCmdBeginQueryIndexedEXT");
      CmdBeginRenderPass = (PFN_vkCmdBeginRenderPass) NextGetDeviceProcAddr(device, "vkCmdBeginRenderPass");
      CmdBeginRenderPass2 = (PFN_vkCmdBeginRenderPass2) NextGetDeviceProcAddr(device, "vkCmdBeginRenderPass2");
      CmdBeginRenderPass2KHR = (PFN_vkCmdBeginRenderPass2KHR) NextGetDeviceProcAddr(device, "vkCmdBeginRenderPass2KHR");
      CmdBeginRendering = (PFN_vkCmdBeginRendering) NextGetDeviceProcAddr(device, "vkCmdBeginRendering");
      CmdBeginRenderingKHR = (PFN_vkCmdBeginRenderingKHR) NextGetDeviceProcAddr(device, "vkCmdBeginRenderingKHR");
      CmdBeginTransformFeedbackEXT = (PFN_vkCmdBeginTransformFeedbackEXT) NextGetDeviceProcAddr(device, "vkCmdBeginTransformFeedbackEXT");
#ifdef VK_ENABLE_BETA_EXTENSIONS
      CmdBeginVideoCodingKHR = (PFN_vkCmdBeginVideoCodingKHR) NextGetDeviceProcAddr(device, "vkCmdBeginVideoCodingKHR");
#endif
      CmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets) NextGetDeviceProcAddr(device, "vkCmdBindDescriptorSets");
      CmdBindIndexBuffer = (PFN_vkCmdBindIndexBuffer) NextGetDeviceProcAddr(device, "vkCmdBindIndexBuffer");
      CmdBindInvocationMaskHUAWEI = (PFN_vkCmdBindInvocationMaskHUAWEI) NextGetDeviceProcAddr(device, "vkCmdBindInvocationMaskHUAWEI");
      CmdBindPipeline = (PFN_vkCmdBindPipeline) NextGetDeviceProcAddr(device, "vkCmdBindPipeline");
      CmdBindPipelineShaderGroupNV = (PFN_vkCmdBindPipelineShaderGroupNV) NextGetDeviceProcAddr(device, "vkCmdBindPipelineShaderGroupNV");
      CmdBindShadingRateImageNV = (PFN_vkCmdBindShadingRateImageNV) NextGetDeviceProcAddr(device, "vkCmdBindShadingRateImageNV");
      CmdBindTransformFeedbackBuffersEXT = (PFN_vkCmdBindTransformFeedbackBuffersEXT) NextGetDeviceProcAddr(device, "vkCmdBindTransformFeedbackBuffersEXT");
      CmdBindVertexBuffers = (PFN_vkCmdBindVertexBuffers) NextGetDeviceProcAddr(device, "vkCmdBindVertexBuffers");
      CmdBindVertexBuffers2 = (PFN_vkCmdBindVertexBuffers2) NextGetDeviceProcAddr(device, "vkCmdBindVertexBuffers2");
      CmdBindVertexBuffers2EXT = (PFN_vkCmdBindVertexBuffers2EXT) NextGetDeviceProcAddr(device, "vkCmdBindVertexBuffers2EXT");
      CmdBlitImage = (PFN_vkCmdBlitImage) NextGetDeviceProcAddr(device, "vkCmdBlitImage");
      CmdBlitImage2 = (PFN_vkCmdBlitImage2) NextGetDeviceProcAddr(device, "vkCmdBlitImage2");
      CmdBlitImage2KHR = (PFN_vkCmdBlitImage2KHR) NextGetDeviceProcAddr(device, "vkCmdBlitImage2KHR");
      CmdBuildAccelerationStructureNV = (PFN_vkCmdBuildAccelerationStructureNV) NextGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructureNV");
      CmdBuildAccelerationStructuresIndirectKHR = (PFN_vkCmdBuildAccelerationStructuresIndirectKHR) NextGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresIndirectKHR");
      CmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR) NextGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR");
      CmdClearAttachments = (PFN_vkCmdClearAttachments) NextGetDeviceProcAddr(device, "vkCmdClearAttachments");
      CmdClearColorImage = (PFN_vkCmdClearColorImage) NextGetDeviceProcAddr(device, "vkCmdClearColorImage");
      CmdClearDepthStencilImage = (PFN_vkCmdClearDepthStencilImage) NextGetDeviceProcAddr(device, "vkCmdClearDepthStencilImage");
#ifdef VK_ENABLE_BETA_EXTENSIONS
      CmdControlVideoCodingKHR = (PFN_vkCmdControlVideoCodingKHR) NextGetDeviceProcAddr(device, "vkCmdControlVideoCodingKHR");
#endif
      CmdCopyAccelerationStructureKHR = (PFN_vkCmdCopyAccelerationStructureKHR) NextGetDeviceProcAddr(device, "vkCmdCopyAccelerationStructureKHR");
      CmdCopyAccelerationStructureNV = (PFN_vkCmdCopyAccelerationStructureNV) NextGetDeviceProcAddr(device, "vkCmdCopyAccelerationStructureNV");
      CmdCopyAccelerationStructureToMemoryKHR = (PFN_vkCmdCopyAccelerationStructureToMemoryKHR) NextGetDeviceProcAddr(device, "vkCmdCopyAccelerationStructureToMemoryKHR");
      CmdCopyBuffer = (PFN_vkCmdCopyBuffer) NextGetDeviceProcAddr(device, "vkCmdCopyBuffer");
      CmdCopyBuffer2 = (PFN_vkCmdCopyBuffer2) NextGetDeviceProcAddr(device, "vkCmdCopyBuffer2");
      CmdCopyBuffer2KHR = (PFN_vkCmdCopyBuffer2KHR) NextGetDeviceProcAddr(device, "vkCmdCopyBuffer2KHR");
      CmdCopyBufferToImage = (PFN_vkCmdCopyBufferToImage) NextGetDeviceProcAddr(device, "vkCmdCopyBufferToImage");
      CmdCopyBufferToImage2 = (PFN_vkCmdCopyBufferToImage2) NextGetDeviceProcAddr(device, "vkCmdCopyBufferToImage2");
      CmdCopyBufferToImage2KHR = (PFN_vkCmdCopyBufferToImage2KHR) NextGetDeviceProcAddr(device, "vkCmdCopyBufferToImage2KHR");
      CmdCopyImage = (PFN_vkCmdCopyImage) NextGetDeviceProcAddr(device, "vkCmdCopyImage");
      CmdCopyImage2 = (PFN_vkCmdCopyImage2) NextGetDeviceProcAddr(device, "vkCmdCopyImage2");
      CmdCopyImage2KHR = (PFN_vkCmdCopyImage2KHR) NextGetDeviceProcAddr(device, "vkCmdCopyImage2KHR");
      CmdCopyImageToBuffer = (PFN_vkCmdCopyImageToBuffer) NextGetDeviceProcAddr(device, "vkCmdCopyImageToBuffer");
      CmdCopyImageToBuffer2 = (PFN_vkCmdCopyImageToBuffer2) NextGetDeviceProcAddr(device, "vkCmdCopyImageToBuffer2");
      CmdCopyImageToBuffer2KHR = (PFN_vkCmdCopyImageToBuffer2KHR) NextGetDeviceProcAddr(device, "vkCmdCopyImageToBuffer2KHR");
      CmdCopyMemoryToAccelerationStructureKHR = (PFN_vkCmdCopyMemoryToAccelerationStructureKHR) NextGetDeviceProcAddr(device, "vkCmdCopyMemoryToAccelerationStructureKHR");
      CmdCopyQueryPoolResults = (PFN_vkCmdCopyQueryPoolResults) NextGetDeviceProcAddr(device, "vkCmdCopyQueryPoolResults");
#ifdef VKROOTS_PLATFORM_UNSUPPORTED
      CmdCuLaunchKernelNVX = (PFN_vkCmdCuLaunchKernelNVX) NextGetDeviceProcAddr(device, "vkCmdCuLaunchKernelNVX");
#endif
      CmdDebugMarkerBeginEXT = (PFN_vkCmdDebugMarkerBeginEXT) NextGetDeviceProcAddr(device, "vkCmdDebugMarkerBeginEXT");
      CmdDebugMarkerEndEXT = (PFN_vkCmdDebugMarkerEndEXT) NextGetDeviceProcAddr(device, "vkCmdDebugMarkerEndEXT");
      CmdDebugMarkerInsertEXT = (PFN_vkCmdDebugMarkerInsertEXT) NextGetDeviceProcAddr(device, "vkCmdDebugMarkerInsertEXT");
#ifdef VK_ENABLE_BETA_EXTENSIONS
      CmdDecodeVideoKHR = (PFN_vkCmdDecodeVideoKHR) NextGetDeviceProcAddr(device, "vkCmdDecodeVideoKHR");
#endif
      CmdDispatch = (PFN_vkCmdDispatch) NextGetDeviceProcAddr(device, "vkCmdDispatch");
      CmdDispatchBase = (PFN_vkCmdDispatchBase) NextGetDeviceProcAddr(device, "vkCmdDispatchBase");
      CmdDispatchBaseKHR = (PFN_vkCmdDispatchBaseKHR) NextGetDeviceProcAddr(device, "vkCmdDispatchBaseKHR");
      CmdDispatchIndirect = (PFN_vkCmdDispatchIndirect) NextGetDeviceProcAddr(device, "vkCmdDispatchIndirect");
      CmdDraw = (PFN_vkCmdDraw) NextGetDeviceProcAddr(device, "vkCmdDraw");
      CmdDrawIndexed = (PFN_vkCmdDrawIndexed) NextGetDeviceProcAddr(device, "vkCmdDrawIndexed");
      CmdDrawIndexedIndirect = (PFN_vkCmdDrawIndexedIndirect) NextGetDeviceProcAddr(device, "vkCmdDrawIndexedIndirect");
      CmdDrawIndexedIndirectCount = (PFN_vkCmdDrawIndexedIndirectCount) NextGetDeviceProcAddr(device, "vkCmdDrawIndexedIndirectCount");
      CmdDrawIndexedIndirectCountAMD = (PFN_vkCmdDrawIndexedIndirectCountAMD) NextGetDeviceProcAddr(device, "vkCmdDrawIndexedIndirectCountAMD");
      CmdDrawIndexedIndirectCountKHR = (PFN_vkCmdDrawIndexedIndirectCountKHR) NextGetDeviceProcAddr(device, "vkCmdDrawIndexedIndirectCountKHR");
      CmdDrawIndirect = (PFN_vkCmdDrawIndirect) NextGetDeviceProcAddr(device, "vkCmdDrawIndirect");
      CmdDrawIndirectByteCountEXT = (PFN_vkCmdDrawIndirectByteCountEXT) NextGetDeviceProcAddr(device, "vkCmdDrawIndirectByteCountEXT");
      CmdDrawIndirectCount = (PFN_vkCmdDrawIndirectCount) NextGetDeviceProcAddr(device, "vkCmdDrawIndirectCount");
      CmdDrawIndirectCountAMD = (PFN_vkCmdDrawIndirectCountAMD) NextGetDeviceProcAddr(device, "vkCmdDrawIndirectCountAMD");
      CmdDrawIndirectCountKHR = (PFN_vkCmdDrawIndirectCountKHR) NextGetDeviceProcAddr(device, "vkCmdDrawIndirectCountKHR");
      CmdDrawMeshTasksIndirectCountNV = (PFN_vkCmdDrawMeshTasksIndirectCountNV) NextGetDeviceProcAddr(device, "vkCmdDrawMeshTasksIndirectCountNV");
      CmdDrawMeshTasksIndirectNV = (PFN_vkCmdDrawMeshTasksIndirectNV) NextGetDeviceProcAddr(device, "vkCmdDrawMeshTasksIndirectNV");
      CmdDrawMeshTasksNV = (PFN_vkCmdDrawMeshTasksNV) NextGetDeviceProcAddr(device, "vkCmdDrawMeshTasksNV");
      CmdDrawMultiEXT = (PFN_vkCmdDrawMultiEXT) NextGetDeviceProcAddr(device, "vkCmdDrawMultiEXT");
      CmdDrawMultiIndexedEXT = (PFN_vkCmdDrawMultiIndexedEXT) NextGetDeviceProcAddr(device, "vkCmdDrawMultiIndexedEXT");
#ifdef VK_ENABLE_BETA_EXTENSIONS
      CmdEncodeVideoKHR = (PFN_vkCmdEncodeVideoKHR) NextGetDeviceProcAddr(device, "vkCmdEncodeVideoKHR");
#endif
      CmdEndConditionalRenderingEXT = (PFN_vkCmdEndConditionalRenderingEXT) NextGetDeviceProcAddr(device, "vkCmdEndConditionalRenderingEXT");
      CmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT) NextGetDeviceProcAddr(device, "vkCmdEndDebugUtilsLabelEXT");
      CmdEndQuery = (PFN_vkCmdEndQuery) NextGetDeviceProcAddr(device, "vkCmdEndQuery");
      CmdEndQueryIndexedEXT = (PFN_vkCmdEndQueryIndexedEXT) NextGetDeviceProcAddr(device, "vkCmdEndQueryIndexedEXT");
      CmdEndRenderPass = (PFN_vkCmdEndRenderPass) NextGetDeviceProcAddr(device, "vkCmdEndRenderPass");
      CmdEndRenderPass2 = (PFN_vkCmdEndRenderPass2) NextGetDeviceProcAddr(device, "vkCmdEndRenderPass2");
      CmdEndRenderPass2KHR = (PFN_vkCmdEndRenderPass2KHR) NextGetDeviceProcAddr(device, "vkCmdEndRenderPass2KHR");
      CmdEndRendering = (PFN_vkCmdEndRendering) NextGetDeviceProcAddr(device, "vkCmdEndRendering");
      CmdEndRenderingKHR = (PFN_vkCmdEndRenderingKHR) NextGetDeviceProcAddr(device, "vkCmdEndRenderingKHR");
      CmdEndTransformFeedbackEXT = (PFN_vkCmdEndTransformFeedbackEXT) NextGetDeviceProcAddr(device, "vkCmdEndTransformFeedbackEXT");
#ifdef VK_ENABLE_BETA_EXTENSIONS
      CmdEndVideoCodingKHR = (PFN_vkCmdEndVideoCodingKHR) NextGetDeviceProcAddr(device, "vkCmdEndVideoCodingKHR");
#endif
      CmdExecuteCommands = (PFN_vkCmdExecuteCommands) NextGetDeviceProcAddr(device, "vkCmdExecuteCommands");
      CmdExecuteGeneratedCommandsNV = (PFN_vkCmdExecuteGeneratedCommandsNV) NextGetDeviceProcAddr(device, "vkCmdExecuteGeneratedCommandsNV");
      CmdFillBuffer = (PFN_vkCmdFillBuffer) NextGetDeviceProcAddr(device, "vkCmdFillBuffer");
      CmdInsertDebugUtilsLabelEXT = (PFN_vkCmdInsertDebugUtilsLabelEXT) NextGetDeviceProcAddr(device, "vkCmdInsertDebugUtilsLabelEXT");
      CmdNextSubpass = (PFN_vkCmdNextSubpass) NextGetDeviceProcAddr(device, "vkCmdNextSubpass");
      CmdNextSubpass2 = (PFN_vkCmdNextSubpass2) NextGetDeviceProcAddr(device, "vkCmdNextSubpass2");
      CmdNextSubpass2KHR = (PFN_vkCmdNextSubpass2KHR) NextGetDeviceProcAddr(device, "vkCmdNextSubpass2KHR");
      CmdPipelineBarrier = (PFN_vkCmdPipelineBarrier) NextGetDeviceProcAddr(device, "vkCmdPipelineBarrier");
      CmdPipelineBarrier2 = (PFN_vkCmdPipelineBarrier2) NextGetDeviceProcAddr(device, "vkCmdPipelineBarrier2");
      CmdPipelineBarrier2KHR = (PFN_vkCmdPipelineBarrier2KHR) NextGetDeviceProcAddr(device, "vkCmdPipelineBarrier2KHR");
      CmdPreprocessGeneratedCommandsNV = (PFN_vkCmdPreprocessGeneratedCommandsNV) NextGetDeviceProcAddr(device, "vkCmdPreprocessGeneratedCommandsNV");
      CmdPushConstants = (PFN_vkCmdPushConstants) NextGetDeviceProcAddr(device, "vkCmdPushConstants");
      CmdPushDescriptorSetKHR = (PFN_vkCmdPushDescriptorSetKHR) NextGetDeviceProcAddr(device, "vkCmdPushDescriptorSetKHR");
      CmdPushDescriptorSetWithTemplateKHR = (PFN_vkCmdPushDescriptorSetWithTemplateKHR) NextGetDeviceProcAddr(device, "vkCmdPushDescriptorSetWithTemplateKHR");
      CmdResetEvent = (PFN_vkCmdResetEvent) NextGetDeviceProcAddr(device, "vkCmdResetEvent");
      CmdResetEvent2 = (PFN_vkCmdResetEvent2) NextGetDeviceProcAddr(device, "vkCmdResetEvent2");
      CmdResetEvent2KHR = (PFN_vkCmdResetEvent2KHR) NextGetDeviceProcAddr(device, "vkCmdResetEvent2KHR");
      CmdResetQueryPool = (PFN_vkCmdResetQueryPool) NextGetDeviceProcAddr(device, "vkCmdResetQueryPool");
      CmdResolveImage = (PFN_vkCmdResolveImage) NextGetDeviceProcAddr(device, "vkCmdResolveImage");
      CmdResolveImage2 = (PFN_vkCmdResolveImage2) NextGetDeviceProcAddr(device, "vkCmdResolveImage2");
      CmdResolveImage2KHR = (PFN_vkCmdResolveImage2KHR) NextGetDeviceProcAddr(device, "vkCmdResolveImage2KHR");
      CmdSetBlendConstants = (PFN_vkCmdSetBlendConstants) NextGetDeviceProcAddr(device, "vkCmdSetBlendConstants");
      CmdSetCheckpointNV = (PFN_vkCmdSetCheckpointNV) NextGetDeviceProcAddr(device, "vkCmdSetCheckpointNV");
      CmdSetCoarseSampleOrderNV = (PFN_vkCmdSetCoarseSampleOrderNV) NextGetDeviceProcAddr(device, "vkCmdSetCoarseSampleOrderNV");
      CmdSetColorWriteEnableEXT = (PFN_vkCmdSetColorWriteEnableEXT) NextGetDeviceProcAddr(device, "vkCmdSetColorWriteEnableEXT");
      CmdSetCullMode = (PFN_vkCmdSetCullMode) NextGetDeviceProcAddr(device, "vkCmdSetCullMode");
      CmdSetCullModeEXT = (PFN_vkCmdSetCullModeEXT) NextGetDeviceProcAddr(device, "vkCmdSetCullModeEXT");
      CmdSetDepthBias = (PFN_vkCmdSetDepthBias) NextGetDeviceProcAddr(device, "vkCmdSetDepthBias");
      CmdSetDepthBiasEnable = (PFN_vkCmdSetDepthBiasEnable) NextGetDeviceProcAddr(device, "vkCmdSetDepthBiasEnable");
      CmdSetDepthBiasEnableEXT = (PFN_vkCmdSetDepthBiasEnableEXT) NextGetDeviceProcAddr(device, "vkCmdSetDepthBiasEnableEXT");
      CmdSetDepthBounds = (PFN_vkCmdSetDepthBounds) NextGetDeviceProcAddr(device, "vkCmdSetDepthBounds");
      CmdSetDepthBoundsTestEnable = (PFN_vkCmdSetDepthBoundsTestEnable) NextGetDeviceProcAddr(device, "vkCmdSetDepthBoundsTestEnable");
      CmdSetDepthBoundsTestEnableEXT = (PFN_vkCmdSetDepthBoundsTestEnableEXT) NextGetDeviceProcAddr(device, "vkCmdSetDepthBoundsTestEnableEXT");
      CmdSetDepthCompareOp = (PFN_vkCmdSetDepthCompareOp) NextGetDeviceProcAddr(device, "vkCmdSetDepthCompareOp");
      CmdSetDepthCompareOpEXT = (PFN_vkCmdSetDepthCompareOpEXT) NextGetDeviceProcAddr(device, "vkCmdSetDepthCompareOpEXT");
      CmdSetDepthTestEnable = (PFN_vkCmdSetDepthTestEnable) NextGetDeviceProcAddr(device, "vkCmdSetDepthTestEnable");
      CmdSetDepthTestEnableEXT = (PFN_vkCmdSetDepthTestEnableEXT) NextGetDeviceProcAddr(device, "vkCmdSetDepthTestEnableEXT");
      CmdSetDepthWriteEnable = (PFN_vkCmdSetDepthWriteEnable) NextGetDeviceProcAddr(device, "vkCmdSetDepthWriteEnable");
      CmdSetDepthWriteEnableEXT = (PFN_vkCmdSetDepthWriteEnableEXT) NextGetDeviceProcAddr(device, "vkCmdSetDepthWriteEnableEXT");
      CmdSetDeviceMask = (PFN_vkCmdSetDeviceMask) NextGetDeviceProcAddr(device, "vkCmdSetDeviceMask");
      CmdSetDeviceMaskKHR = (PFN_vkCmdSetDeviceMaskKHR) NextGetDeviceProcAddr(device, "vkCmdSetDeviceMaskKHR");
      CmdSetDiscardRectangleEXT = (PFN_vkCmdSetDiscardRectangleEXT) NextGetDeviceProcAddr(device, "vkCmdSetDiscardRectangleEXT");
      CmdSetEvent = (PFN_vkCmdSetEvent) NextGetDeviceProcAddr(device, "vkCmdSetEvent");
      CmdSetEvent2 = (PFN_vkCmdSetEvent2) NextGetDeviceProcAddr(device, "vkCmdSetEvent2");
      CmdSetEvent2KHR = (PFN_vkCmdSetEvent2KHR) NextGetDeviceProcAddr(device, "vkCmdSetEvent2KHR");
      CmdSetExclusiveScissorNV = (PFN_vkCmdSetExclusiveScissorNV) NextGetDeviceProcAddr(device, "vkCmdSetExclusiveScissorNV");
      CmdSetFragmentShadingRateEnumNV = (PFN_vkCmdSetFragmentShadingRateEnumNV) NextGetDeviceProcAddr(device, "vkCmdSetFragmentShadingRateEnumNV");
      CmdSetFragmentShadingRateKHR = (PFN_vkCmdSetFragmentShadingRateKHR) NextGetDeviceProcAddr(device, "vkCmdSetFragmentShadingRateKHR");
      CmdSetFrontFace = (PFN_vkCmdSetFrontFace) NextGetDeviceProcAddr(device, "vkCmdSetFrontFace");
      CmdSetFrontFaceEXT = (PFN_vkCmdSetFrontFaceEXT) NextGetDeviceProcAddr(device, "vkCmdSetFrontFaceEXT");
      CmdSetLineStippleEXT = (PFN_vkCmdSetLineStippleEXT) NextGetDeviceProcAddr(device, "vkCmdSetLineStippleEXT");
      CmdSetLineWidth = (PFN_vkCmdSetLineWidth) NextGetDeviceProcAddr(device, "vkCmdSetLineWidth");
      CmdSetLogicOpEXT = (PFN_vkCmdSetLogicOpEXT) NextGetDeviceProcAddr(device, "vkCmdSetLogicOpEXT");
      CmdSetPatchControlPointsEXT = (PFN_vkCmdSetPatchControlPointsEXT) NextGetDeviceProcAddr(device, "vkCmdSetPatchControlPointsEXT");
      CmdSetPerformanceMarkerINTEL = (PFN_vkCmdSetPerformanceMarkerINTEL) NextGetDeviceProcAddr(device, "vkCmdSetPerformanceMarkerINTEL");
      CmdSetPerformanceOverrideINTEL = (PFN_vkCmdSetPerformanceOverrideINTEL) NextGetDeviceProcAddr(device, "vkCmdSetPerformanceOverrideINTEL");
      CmdSetPerformanceStreamMarkerINTEL = (PFN_vkCmdSetPerformanceStreamMarkerINTEL) NextGetDeviceProcAddr(device, "vkCmdSetPerformanceStreamMarkerINTEL");
      CmdSetPrimitiveRestartEnable = (PFN_vkCmdSetPrimitiveRestartEnable) NextGetDeviceProcAddr(device, "vkCmdSetPrimitiveRestartEnable");
      CmdSetPrimitiveRestartEnableEXT = (PFN_vkCmdSetPrimitiveRestartEnableEXT) NextGetDeviceProcAddr(device, "vkCmdSetPrimitiveRestartEnableEXT");
      CmdSetPrimitiveTopology = (PFN_vkCmdSetPrimitiveTopology) NextGetDeviceProcAddr(device, "vkCmdSetPrimitiveTopology");
      CmdSetPrimitiveTopologyEXT = (PFN_vkCmdSetPrimitiveTopologyEXT) NextGetDeviceProcAddr(device, "vkCmdSetPrimitiveTopologyEXT");
      CmdSetRasterizerDiscardEnable = (PFN_vkCmdSetRasterizerDiscardEnable) NextGetDeviceProcAddr(device, "vkCmdSetRasterizerDiscardEnable");
      CmdSetRasterizerDiscardEnableEXT = (PFN_vkCmdSetRasterizerDiscardEnableEXT) NextGetDeviceProcAddr(device, "vkCmdSetRasterizerDiscardEnableEXT");
      CmdSetRayTracingPipelineStackSizeKHR = (PFN_vkCmdSetRayTracingPipelineStackSizeKHR) NextGetDeviceProcAddr(device, "vkCmdSetRayTracingPipelineStackSizeKHR");
      CmdSetSampleLocationsEXT = (PFN_vkCmdSetSampleLocationsEXT) NextGetDeviceProcAddr(device, "vkCmdSetSampleLocationsEXT");
      CmdSetScissor = (PFN_vkCmdSetScissor) NextGetDeviceProcAddr(device, "vkCmdSetScissor");
      CmdSetScissorWithCount = (PFN_vkCmdSetScissorWithCount) NextGetDeviceProcAddr(device, "vkCmdSetScissorWithCount");
      CmdSetScissorWithCountEXT = (PFN_vkCmdSetScissorWithCountEXT) NextGetDeviceProcAddr(device, "vkCmdSetScissorWithCountEXT");
      CmdSetStencilCompareMask = (PFN_vkCmdSetStencilCompareMask) NextGetDeviceProcAddr(device, "vkCmdSetStencilCompareMask");
      CmdSetStencilOp = (PFN_vkCmdSetStencilOp) NextGetDeviceProcAddr(device, "vkCmdSetStencilOp");
      CmdSetStencilOpEXT = (PFN_vkCmdSetStencilOpEXT) NextGetDeviceProcAddr(device, "vkCmdSetStencilOpEXT");
      CmdSetStencilReference = (PFN_vkCmdSetStencilReference) NextGetDeviceProcAddr(device, "vkCmdSetStencilReference");
      CmdSetStencilTestEnable = (PFN_vkCmdSetStencilTestEnable) NextGetDeviceProcAddr(device, "vkCmdSetStencilTestEnable");
      CmdSetStencilTestEnableEXT = (PFN_vkCmdSetStencilTestEnableEXT) NextGetDeviceProcAddr(device, "vkCmdSetStencilTestEnableEXT");
      CmdSetStencilWriteMask = (PFN_vkCmdSetStencilWriteMask) NextGetDeviceProcAddr(device, "vkCmdSetStencilWriteMask");
      CmdSetVertexInputEXT = (PFN_vkCmdSetVertexInputEXT) NextGetDeviceProcAddr(device, "vkCmdSetVertexInputEXT");
      CmdSetViewport = (PFN_vkCmdSetViewport) NextGetDeviceProcAddr(device, "vkCmdSetViewport");
      CmdSetViewportShadingRatePaletteNV = (PFN_vkCmdSetViewportShadingRatePaletteNV) NextGetDeviceProcAddr(device, "vkCmdSetViewportShadingRatePaletteNV");
      CmdSetViewportWScalingNV = (PFN_vkCmdSetViewportWScalingNV) NextGetDeviceProcAddr(device, "vkCmdSetViewportWScalingNV");
      CmdSetViewportWithCount = (PFN_vkCmdSetViewportWithCount) NextGetDeviceProcAddr(device, "vkCmdSetViewportWithCount");
      CmdSetViewportWithCountEXT = (PFN_vkCmdSetViewportWithCountEXT) NextGetDeviceProcAddr(device, "vkCmdSetViewportWithCountEXT");
      CmdSubpassShadingHUAWEI = (PFN_vkCmdSubpassShadingHUAWEI) NextGetDeviceProcAddr(device, "vkCmdSubpassShadingHUAWEI");
      CmdTraceRaysIndirect2KHR = (PFN_vkCmdTraceRaysIndirect2KHR) NextGetDeviceProcAddr(device, "vkCmdTraceRaysIndirect2KHR");
      CmdTraceRaysIndirectKHR = (PFN_vkCmdTraceRaysIndirectKHR) NextGetDeviceProcAddr(device, "vkCmdTraceRaysIndirectKHR");
      CmdTraceRaysKHR = (PFN_vkCmdTraceRaysKHR) NextGetDeviceProcAddr(device, "vkCmdTraceRaysKHR");
      CmdTraceRaysNV = (PFN_vkCmdTraceRaysNV) NextGetDeviceProcAddr(device, "vkCmdTraceRaysNV");
      CmdUpdateBuffer = (PFN_vkCmdUpdateBuffer) NextGetDeviceProcAddr(device, "vkCmdUpdateBuffer");
      CmdWaitEvents = (PFN_vkCmdWaitEvents) NextGetDeviceProcAddr(device, "vkCmdWaitEvents");
      CmdWaitEvents2 = (PFN_vkCmdWaitEvents2) NextGetDeviceProcAddr(device, "vkCmdWaitEvents2");
      CmdWaitEvents2KHR = (PFN_vkCmdWaitEvents2KHR) NextGetDeviceProcAddr(device, "vkCmdWaitEvents2KHR");
      CmdWriteAccelerationStructuresPropertiesKHR = (PFN_vkCmdWriteAccelerationStructuresPropertiesKHR) NextGetDeviceProcAddr(device, "vkCmdWriteAccelerationStructuresPropertiesKHR");
      CmdWriteAccelerationStructuresPropertiesNV = (PFN_vkCmdWriteAccelerationStructuresPropertiesNV) NextGetDeviceProcAddr(device, "vkCmdWriteAccelerationStructuresPropertiesNV");
      CmdWriteBufferMarker2AMD = (PFN_vkCmdWriteBufferMarker2AMD) NextGetDeviceProcAddr(device, "vkCmdWriteBufferMarker2AMD");
      CmdWriteBufferMarkerAMD = (PFN_vkCmdWriteBufferMarkerAMD) NextGetDeviceProcAddr(device, "vkCmdWriteBufferMarkerAMD");
      CmdWriteTimestamp = (PFN_vkCmdWriteTimestamp) NextGetDeviceProcAddr(device, "vkCmdWriteTimestamp");
      CmdWriteTimestamp2 = (PFN_vkCmdWriteTimestamp2) NextGetDeviceProcAddr(device, "vkCmdWriteTimestamp2");
      CmdWriteTimestamp2KHR = (PFN_vkCmdWriteTimestamp2KHR) NextGetDeviceProcAddr(device, "vkCmdWriteTimestamp2KHR");
      CompileDeferredNV = (PFN_vkCompileDeferredNV) NextGetDeviceProcAddr(device, "vkCompileDeferredNV");
      CopyAccelerationStructureKHR = (PFN_vkCopyAccelerationStructureKHR) NextGetDeviceProcAddr(device, "vkCopyAccelerationStructureKHR");
      CopyAccelerationStructureToMemoryKHR = (PFN_vkCopyAccelerationStructureToMemoryKHR) NextGetDeviceProcAddr(device, "vkCopyAccelerationStructureToMemoryKHR");
      CopyMemoryToAccelerationStructureKHR = (PFN_vkCopyMemoryToAccelerationStructureKHR) NextGetDeviceProcAddr(device, "vkCopyMemoryToAccelerationStructureKHR");
      CreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR) NextGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR");
      CreateAccelerationStructureNV = (PFN_vkCreateAccelerationStructureNV) NextGetDeviceProcAddr(device, "vkCreateAccelerationStructureNV");
      CreateBuffer = (PFN_vkCreateBuffer) NextGetDeviceProcAddr(device, "vkCreateBuffer");
#ifdef VK_USE_PLATFORM_FUCHSIA
      CreateBufferCollectionFUCHSIA = (PFN_vkCreateBufferCollectionFUCHSIA) NextGetDeviceProcAddr(device, "vkCreateBufferCollectionFUCHSIA");
#endif
      CreateBufferView = (PFN_vkCreateBufferView) NextGetDeviceProcAddr(device, "vkCreateBufferView");
      CreateCommandPool = (PFN_vkCreateCommandPool) NextGetDeviceProcAddr(device, "vkCreateCommandPool");
      CreateComputePipelines = (PFN_vkCreateComputePipelines) NextGetDeviceProcAddr(device, "vkCreateComputePipelines");
#ifdef VKROOTS_PLATFORM_UNSUPPORTED
      CreateCuFunctionNVX = (PFN_vkCreateCuFunctionNVX) NextGetDeviceProcAddr(device, "vkCreateCuFunctionNVX");
#endif
#ifdef VKROOTS_PLATFORM_UNSUPPORTED
      CreateCuModuleNVX = (PFN_vkCreateCuModuleNVX) NextGetDeviceProcAddr(device, "vkCreateCuModuleNVX");
#endif
      CreateDeferredOperationKHR = (PFN_vkCreateDeferredOperationKHR) NextGetDeviceProcAddr(device, "vkCreateDeferredOperationKHR");
      CreateDescriptorPool = (PFN_vkCreateDescriptorPool) NextGetDeviceProcAddr(device, "vkCreateDescriptorPool");
      CreateDescriptorSetLayout = (PFN_vkCreateDescriptorSetLayout) NextGetDeviceProcAddr(device, "vkCreateDescriptorSetLayout");
      CreateDescriptorUpdateTemplate = (PFN_vkCreateDescriptorUpdateTemplate) NextGetDeviceProcAddr(device, "vkCreateDescriptorUpdateTemplate");
      CreateDescriptorUpdateTemplateKHR = (PFN_vkCreateDescriptorUpdateTemplateKHR) NextGetDeviceProcAddr(device, "vkCreateDescriptorUpdateTemplateKHR");
      CreateEvent = (PFN_vkCreateEvent) NextGetDeviceProcAddr(device, "vkCreateEvent");
      CreateFence = (PFN_vkCreateFence) NextGetDeviceProcAddr(device, "vkCreateFence");
      CreateFramebuffer = (PFN_vkCreateFramebuffer) NextGetDeviceProcAddr(device, "vkCreateFramebuffer");
      CreateGraphicsPipelines = (PFN_vkCreateGraphicsPipelines) NextGetDeviceProcAddr(device, "vkCreateGraphicsPipelines");
      CreateImage = (PFN_vkCreateImage) NextGetDeviceProcAddr(device, "vkCreateImage");
      CreateImageView = (PFN_vkCreateImageView) NextGetDeviceProcAddr(device, "vkCreateImageView");
      CreateIndirectCommandsLayoutNV = (PFN_vkCreateIndirectCommandsLayoutNV) NextGetDeviceProcAddr(device, "vkCreateIndirectCommandsLayoutNV");
      CreatePipelineCache = (PFN_vkCreatePipelineCache) NextGetDeviceProcAddr(device, "vkCreatePipelineCache");
      CreatePipelineLayout = (PFN_vkCreatePipelineLayout) NextGetDeviceProcAddr(device, "vkCreatePipelineLayout");
      CreatePrivateDataSlot = (PFN_vkCreatePrivateDataSlot) NextGetDeviceProcAddr(device, "vkCreatePrivateDataSlot");
      CreatePrivateDataSlotEXT = (PFN_vkCreatePrivateDataSlotEXT) NextGetDeviceProcAddr(device, "vkCreatePrivateDataSlotEXT");
      CreateQueryPool = (PFN_vkCreateQueryPool) NextGetDeviceProcAddr(device, "vkCreateQueryPool");
      CreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR) NextGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR");
      CreateRayTracingPipelinesNV = (PFN_vkCreateRayTracingPipelinesNV) NextGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesNV");
      CreateRenderPass = (PFN_vkCreateRenderPass) NextGetDeviceProcAddr(device, "vkCreateRenderPass");
      CreateRenderPass2 = (PFN_vkCreateRenderPass2) NextGetDeviceProcAddr(device, "vkCreateRenderPass2");
      CreateRenderPass2KHR = (PFN_vkCreateRenderPass2KHR) NextGetDeviceProcAddr(device, "vkCreateRenderPass2KHR");
      CreateSampler = (PFN_vkCreateSampler) NextGetDeviceProcAddr(device, "vkCreateSampler");
      CreateSamplerYcbcrConversion = (PFN_vkCreateSamplerYcbcrConversion) NextGetDeviceProcAddr(device, "vkCreateSamplerYcbcrConversion");
      CreateSamplerYcbcrConversionKHR = (PFN_vkCreateSamplerYcbcrConversionKHR) NextGetDeviceProcAddr(device, "vkCreateSamplerYcbcrConversionKHR");
      CreateSemaphore = (PFN_vkCreateSemaphore) NextGetDeviceProcAddr(device, "vkCreateSemaphore");
      CreateShaderModule = (PFN_vkCreateShaderModule) NextGetDeviceProcAddr(device, "vkCreateShaderModule");
      CreateSharedSwapchainsKHR = (PFN_vkCreateSharedSwapchainsKHR) NextGetDeviceProcAddr(device, "vkCreateSharedSwapchainsKHR");
      CreateSwapchainKHR = (PFN_vkCreateSwapchainKHR) NextGetDeviceProcAddr(device, "vkCreateSwapchainKHR");
      CreateValidationCacheEXT = (PFN_vkCreateValidationCacheEXT) NextGetDeviceProcAddr(device, "vkCreateValidationCacheEXT");
#ifdef VK_ENABLE_BETA_EXTENSIONS
      CreateVideoSessionKHR = (PFN_vkCreateVideoSessionKHR) NextGetDeviceProcAddr(device, "vkCreateVideoSessionKHR");
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
      CreateVideoSessionParametersKHR = (PFN_vkCreateVideoSessionParametersKHR) NextGetDeviceProcAddr(device, "vkCreateVideoSessionParametersKHR");
#endif
      DebugMarkerSetObjectNameEXT = (PFN_vkDebugMarkerSetObjectNameEXT) NextGetDeviceProcAddr(device, "vkDebugMarkerSetObjectNameEXT");
      DebugMarkerSetObjectTagEXT = (PFN_vkDebugMarkerSetObjectTagEXT) NextGetDeviceProcAddr(device, "vkDebugMarkerSetObjectTagEXT");
      DeferredOperationJoinKHR = (PFN_vkDeferredOperationJoinKHR) NextGetDeviceProcAddr(device, "vkDeferredOperationJoinKHR");
      DestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR) NextGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR");
      DestroyAccelerationStructureNV = (PFN_vkDestroyAccelerationStructureNV) NextGetDeviceProcAddr(device, "vkDestroyAccelerationStructureNV");
      DestroyBuffer = (PFN_vkDestroyBuffer) NextGetDeviceProcAddr(device, "vkDestroyBuffer");
#ifdef VK_USE_PLATFORM_FUCHSIA
      DestroyBufferCollectionFUCHSIA = (PFN_vkDestroyBufferCollectionFUCHSIA) NextGetDeviceProcAddr(device, "vkDestroyBufferCollectionFUCHSIA");
#endif
      DestroyBufferView = (PFN_vkDestroyBufferView) NextGetDeviceProcAddr(device, "vkDestroyBufferView");
      DestroyCommandPool = (PFN_vkDestroyCommandPool) NextGetDeviceProcAddr(device, "vkDestroyCommandPool");
#ifdef VKROOTS_PLATFORM_UNSUPPORTED
      DestroyCuFunctionNVX = (PFN_vkDestroyCuFunctionNVX) NextGetDeviceProcAddr(device, "vkDestroyCuFunctionNVX");
#endif
#ifdef VKROOTS_PLATFORM_UNSUPPORTED
      DestroyCuModuleNVX = (PFN_vkDestroyCuModuleNVX) NextGetDeviceProcAddr(device, "vkDestroyCuModuleNVX");
#endif
      DestroyDeferredOperationKHR = (PFN_vkDestroyDeferredOperationKHR) NextGetDeviceProcAddr(device, "vkDestroyDeferredOperationKHR");
      DestroyDescriptorPool = (PFN_vkDestroyDescriptorPool) NextGetDeviceProcAddr(device, "vkDestroyDescriptorPool");
      DestroyDescriptorSetLayout = (PFN_vkDestroyDescriptorSetLayout) NextGetDeviceProcAddr(device, "vkDestroyDescriptorSetLayout");
      DestroyDescriptorUpdateTemplate = (PFN_vkDestroyDescriptorUpdateTemplate) NextGetDeviceProcAddr(device, "vkDestroyDescriptorUpdateTemplate");
      DestroyDescriptorUpdateTemplateKHR = (PFN_vkDestroyDescriptorUpdateTemplateKHR) NextGetDeviceProcAddr(device, "vkDestroyDescriptorUpdateTemplateKHR");
      DestroyDeviceReal = (PFN_vkDestroyDevice) NextGetDeviceProcAddr(device, "vkDestroyDevice");
      DestroyDevice = (PFN_vkDestroyDevice) DestroyDeviceWrapper;
      DestroyEvent = (PFN_vkDestroyEvent) NextGetDeviceProcAddr(device, "vkDestroyEvent");
      DestroyFence = (PFN_vkDestroyFence) NextGetDeviceProcAddr(device, "vkDestroyFence");
      DestroyFramebuffer = (PFN_vkDestroyFramebuffer) NextGetDeviceProcAddr(device, "vkDestroyFramebuffer");
      DestroyImage = (PFN_vkDestroyImage) NextGetDeviceProcAddr(device, "vkDestroyImage");
      DestroyImageView = (PFN_vkDestroyImageView) NextGetDeviceProcAddr(device, "vkDestroyImageView");
      DestroyIndirectCommandsLayoutNV = (PFN_vkDestroyIndirectCommandsLayoutNV) NextGetDeviceProcAddr(device, "vkDestroyIndirectCommandsLayoutNV");
      DestroyPipeline = (PFN_vkDestroyPipeline) NextGetDeviceProcAddr(device, "vkDestroyPipeline");
      DestroyPipelineCache = (PFN_vkDestroyPipelineCache) NextGetDeviceProcAddr(device, "vkDestroyPipelineCache");
      DestroyPipelineLayout = (PFN_vkDestroyPipelineLayout) NextGetDeviceProcAddr(device, "vkDestroyPipelineLayout");
      DestroyPrivateDataSlot = (PFN_vkDestroyPrivateDataSlot) NextGetDeviceProcAddr(device, "vkDestroyPrivateDataSlot");
      DestroyPrivateDataSlotEXT = (PFN_vkDestroyPrivateDataSlotEXT) NextGetDeviceProcAddr(device, "vkDestroyPrivateDataSlotEXT");
      DestroyQueryPool = (PFN_vkDestroyQueryPool) NextGetDeviceProcAddr(device, "vkDestroyQueryPool");
      DestroyRenderPass = (PFN_vkDestroyRenderPass) NextGetDeviceProcAddr(device, "vkDestroyRenderPass");
      DestroySampler = (PFN_vkDestroySampler) NextGetDeviceProcAddr(device, "vkDestroySampler");
      DestroySamplerYcbcrConversion = (PFN_vkDestroySamplerYcbcrConversion) NextGetDeviceProcAddr(device, "vkDestroySamplerYcbcrConversion");
      DestroySamplerYcbcrConversionKHR = (PFN_vkDestroySamplerYcbcrConversionKHR) NextGetDeviceProcAddr(device, "vkDestroySamplerYcbcrConversionKHR");
      DestroySemaphore = (PFN_vkDestroySemaphore) NextGetDeviceProcAddr(device, "vkDestroySemaphore");
      DestroyShaderModule = (PFN_vkDestroyShaderModule) NextGetDeviceProcAddr(device, "vkDestroyShaderModule");
      DestroySwapchainKHR = (PFN_vkDestroySwapchainKHR) NextGetDeviceProcAddr(device, "vkDestroySwapchainKHR");
      DestroyValidationCacheEXT = (PFN_vkDestroyValidationCacheEXT) NextGetDeviceProcAddr(device, "vkDestroyValidationCacheEXT");
#ifdef VK_ENABLE_BETA_EXTENSIONS
      DestroyVideoSessionKHR = (PFN_vkDestroyVideoSessionKHR) NextGetDeviceProcAddr(device, "vkDestroyVideoSessionKHR");
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
      DestroyVideoSessionParametersKHR = (PFN_vkDestroyVideoSessionParametersKHR) NextGetDeviceProcAddr(device, "vkDestroyVideoSessionParametersKHR");
#endif
      DeviceWaitIdle = (PFN_vkDeviceWaitIdle) NextGetDeviceProcAddr(device, "vkDeviceWaitIdle");
      DisplayPowerControlEXT = (PFN_vkDisplayPowerControlEXT) NextGetDeviceProcAddr(device, "vkDisplayPowerControlEXT");
      EndCommandBuffer = (PFN_vkEndCommandBuffer) NextGetDeviceProcAddr(device, "vkEndCommandBuffer");
#ifdef VK_USE_PLATFORM_METAL_EXT
      ExportMetalObjectsEXT = (PFN_vkExportMetalObjectsEXT) NextGetDeviceProcAddr(device, "vkExportMetalObjectsEXT");
#endif
      FlushMappedMemoryRanges = (PFN_vkFlushMappedMemoryRanges) NextGetDeviceProcAddr(device, "vkFlushMappedMemoryRanges");
      FreeCommandBuffers = (PFN_vkFreeCommandBuffers) NextGetDeviceProcAddr(device, "vkFreeCommandBuffers");
      FreeDescriptorSets = (PFN_vkFreeDescriptorSets) NextGetDeviceProcAddr(device, "vkFreeDescriptorSets");
      FreeMemory = (PFN_vkFreeMemory) NextGetDeviceProcAddr(device, "vkFreeMemory");
      GetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR) NextGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR");
      GetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR) NextGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR");
      GetAccelerationStructureHandleNV = (PFN_vkGetAccelerationStructureHandleNV) NextGetDeviceProcAddr(device, "vkGetAccelerationStructureHandleNV");
      GetAccelerationStructureMemoryRequirementsNV = (PFN_vkGetAccelerationStructureMemoryRequirementsNV) NextGetDeviceProcAddr(device, "vkGetAccelerationStructureMemoryRequirementsNV");
#ifdef VK_USE_PLATFORM_ANDROID_KHR
      GetAndroidHardwareBufferPropertiesANDROID = (PFN_vkGetAndroidHardwareBufferPropertiesANDROID) NextGetDeviceProcAddr(device, "vkGetAndroidHardwareBufferPropertiesANDROID");
#endif
#ifdef VK_USE_PLATFORM_FUCHSIA
      GetBufferCollectionPropertiesFUCHSIA = (PFN_vkGetBufferCollectionPropertiesFUCHSIA) NextGetDeviceProcAddr(device, "vkGetBufferCollectionPropertiesFUCHSIA");
#endif
      GetBufferDeviceAddress = (PFN_vkGetBufferDeviceAddress) NextGetDeviceProcAddr(device, "vkGetBufferDeviceAddress");
      GetBufferDeviceAddressEXT = (PFN_vkGetBufferDeviceAddressEXT) NextGetDeviceProcAddr(device, "vkGetBufferDeviceAddressEXT");
      GetBufferDeviceAddressKHR = (PFN_vkGetBufferDeviceAddressKHR) NextGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR");
      GetBufferMemoryRequirements = (PFN_vkGetBufferMemoryRequirements) NextGetDeviceProcAddr(device, "vkGetBufferMemoryRequirements");
      GetBufferMemoryRequirements2 = (PFN_vkGetBufferMemoryRequirements2) NextGetDeviceProcAddr(device, "vkGetBufferMemoryRequirements2");
      GetBufferMemoryRequirements2KHR = (PFN_vkGetBufferMemoryRequirements2KHR) NextGetDeviceProcAddr(device, "vkGetBufferMemoryRequirements2KHR");
      GetBufferOpaqueCaptureAddress = (PFN_vkGetBufferOpaqueCaptureAddress) NextGetDeviceProcAddr(device, "vkGetBufferOpaqueCaptureAddress");
      GetBufferOpaqueCaptureAddressKHR = (PFN_vkGetBufferOpaqueCaptureAddressKHR) NextGetDeviceProcAddr(device, "vkGetBufferOpaqueCaptureAddressKHR");
      GetCalibratedTimestampsEXT = (PFN_vkGetCalibratedTimestampsEXT) NextGetDeviceProcAddr(device, "vkGetCalibratedTimestampsEXT");
      GetDeferredOperationMaxConcurrencyKHR = (PFN_vkGetDeferredOperationMaxConcurrencyKHR) NextGetDeviceProcAddr(device, "vkGetDeferredOperationMaxConcurrencyKHR");
      GetDeferredOperationResultKHR = (PFN_vkGetDeferredOperationResultKHR) NextGetDeviceProcAddr(device, "vkGetDeferredOperationResultKHR");
      GetDescriptorSetHostMappingVALVE = (PFN_vkGetDescriptorSetHostMappingVALVE) NextGetDeviceProcAddr(device, "vkGetDescriptorSetHostMappingVALVE");
      GetDescriptorSetLayoutHostMappingInfoVALVE = (PFN_vkGetDescriptorSetLayoutHostMappingInfoVALVE) NextGetDeviceProcAddr(device, "vkGetDescriptorSetLayoutHostMappingInfoVALVE");
      GetDescriptorSetLayoutSupport = (PFN_vkGetDescriptorSetLayoutSupport) NextGetDeviceProcAddr(device, "vkGetDescriptorSetLayoutSupport");
      GetDescriptorSetLayoutSupportKHR = (PFN_vkGetDescriptorSetLayoutSupportKHR) NextGetDeviceProcAddr(device, "vkGetDescriptorSetLayoutSupportKHR");
      GetDeviceAccelerationStructureCompatibilityKHR = (PFN_vkGetDeviceAccelerationStructureCompatibilityKHR) NextGetDeviceProcAddr(device, "vkGetDeviceAccelerationStructureCompatibilityKHR");
      GetDeviceBufferMemoryRequirements = (PFN_vkGetDeviceBufferMemoryRequirements) NextGetDeviceProcAddr(device, "vkGetDeviceBufferMemoryRequirements");
      GetDeviceBufferMemoryRequirementsKHR = (PFN_vkGetDeviceBufferMemoryRequirementsKHR) NextGetDeviceProcAddr(device, "vkGetDeviceBufferMemoryRequirementsKHR");
      GetDeviceGroupPeerMemoryFeatures = (PFN_vkGetDeviceGroupPeerMemoryFeatures) NextGetDeviceProcAddr(device, "vkGetDeviceGroupPeerMemoryFeatures");
      GetDeviceGroupPeerMemoryFeaturesKHR = (PFN_vkGetDeviceGroupPeerMemoryFeaturesKHR) NextGetDeviceProcAddr(device, "vkGetDeviceGroupPeerMemoryFeaturesKHR");
      GetDeviceGroupPresentCapabilitiesKHR = (PFN_vkGetDeviceGroupPresentCapabilitiesKHR) NextGetDeviceProcAddr(device, "vkGetDeviceGroupPresentCapabilitiesKHR");
#ifdef VK_USE_PLATFORM_WIN32_KHR
      GetDeviceGroupSurfacePresentModes2EXT = (PFN_vkGetDeviceGroupSurfacePresentModes2EXT) NextGetDeviceProcAddr(device, "vkGetDeviceGroupSurfacePresentModes2EXT");
#endif
      GetDeviceGroupSurfacePresentModesKHR = (PFN_vkGetDeviceGroupSurfacePresentModesKHR) NextGetDeviceProcAddr(device, "vkGetDeviceGroupSurfacePresentModesKHR");
      GetDeviceImageMemoryRequirements = (PFN_vkGetDeviceImageMemoryRequirements) NextGetDeviceProcAddr(device, "vkGetDeviceImageMemoryRequirements");
      GetDeviceImageMemoryRequirementsKHR = (PFN_vkGetDeviceImageMemoryRequirementsKHR) NextGetDeviceProcAddr(device, "vkGetDeviceImageMemoryRequirementsKHR");
      GetDeviceImageSparseMemoryRequirements = (PFN_vkGetDeviceImageSparseMemoryRequirements) NextGetDeviceProcAddr(device, "vkGetDeviceImageSparseMemoryRequirements");
      GetDeviceImageSparseMemoryRequirementsKHR = (PFN_vkGetDeviceImageSparseMemoryRequirementsKHR) NextGetDeviceProcAddr(device, "vkGetDeviceImageSparseMemoryRequirementsKHR");
      GetDeviceMemoryCommitment = (PFN_vkGetDeviceMemoryCommitment) NextGetDeviceProcAddr(device, "vkGetDeviceMemoryCommitment");
      GetDeviceMemoryOpaqueCaptureAddress = (PFN_vkGetDeviceMemoryOpaqueCaptureAddress) NextGetDeviceProcAddr(device, "vkGetDeviceMemoryOpaqueCaptureAddress");
      GetDeviceMemoryOpaqueCaptureAddressKHR = (PFN_vkGetDeviceMemoryOpaqueCaptureAddressKHR) NextGetDeviceProcAddr(device, "vkGetDeviceMemoryOpaqueCaptureAddressKHR");
      GetDeviceProcAddr = NextGetDeviceProcAddr;
      GetDeviceQueue = (PFN_vkGetDeviceQueue) NextGetDeviceProcAddr(device, "vkGetDeviceQueue");
      GetDeviceQueue2 = (PFN_vkGetDeviceQueue2) NextGetDeviceProcAddr(device, "vkGetDeviceQueue2");
      GetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI = (PFN_vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI) NextGetDeviceProcAddr(device, "vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI");
      GetEventStatus = (PFN_vkGetEventStatus) NextGetDeviceProcAddr(device, "vkGetEventStatus");
      GetFenceFdKHR = (PFN_vkGetFenceFdKHR) NextGetDeviceProcAddr(device, "vkGetFenceFdKHR");
      GetFenceStatus = (PFN_vkGetFenceStatus) NextGetDeviceProcAddr(device, "vkGetFenceStatus");
#ifdef VK_USE_PLATFORM_WIN32_KHR
      GetFenceWin32HandleKHR = (PFN_vkGetFenceWin32HandleKHR) NextGetDeviceProcAddr(device, "vkGetFenceWin32HandleKHR");
#endif
      GetGeneratedCommandsMemoryRequirementsNV = (PFN_vkGetGeneratedCommandsMemoryRequirementsNV) NextGetDeviceProcAddr(device, "vkGetGeneratedCommandsMemoryRequirementsNV");
      GetImageDrmFormatModifierPropertiesEXT = (PFN_vkGetImageDrmFormatModifierPropertiesEXT) NextGetDeviceProcAddr(device, "vkGetImageDrmFormatModifierPropertiesEXT");
      GetImageMemoryRequirements = (PFN_vkGetImageMemoryRequirements) NextGetDeviceProcAddr(device, "vkGetImageMemoryRequirements");
      GetImageMemoryRequirements2 = (PFN_vkGetImageMemoryRequirements2) NextGetDeviceProcAddr(device, "vkGetImageMemoryRequirements2");
      GetImageMemoryRequirements2KHR = (PFN_vkGetImageMemoryRequirements2KHR) NextGetDeviceProcAddr(device, "vkGetImageMemoryRequirements2KHR");
      GetImageSparseMemoryRequirements = (PFN_vkGetImageSparseMemoryRequirements) NextGetDeviceProcAddr(device, "vkGetImageSparseMemoryRequirements");
      GetImageSparseMemoryRequirements2 = (PFN_vkGetImageSparseMemoryRequirements2) NextGetDeviceProcAddr(device, "vkGetImageSparseMemoryRequirements2");
      GetImageSparseMemoryRequirements2KHR = (PFN_vkGetImageSparseMemoryRequirements2KHR) NextGetDeviceProcAddr(device, "vkGetImageSparseMemoryRequirements2KHR");
      GetImageSubresourceLayout = (PFN_vkGetImageSubresourceLayout) NextGetDeviceProcAddr(device, "vkGetImageSubresourceLayout");
      GetImageSubresourceLayout2EXT = (PFN_vkGetImageSubresourceLayout2EXT) NextGetDeviceProcAddr(device, "vkGetImageSubresourceLayout2EXT");
#ifdef VKROOTS_PLATFORM_UNSUPPORTED
      GetImageViewAddressNVX = (PFN_vkGetImageViewAddressNVX) NextGetDeviceProcAddr(device, "vkGetImageViewAddressNVX");
#endif
#ifdef VKROOTS_PLATFORM_UNSUPPORTED
      GetImageViewHandleNVX = (PFN_vkGetImageViewHandleNVX) NextGetDeviceProcAddr(device, "vkGetImageViewHandleNVX");
#endif
#ifdef VK_USE_PLATFORM_ANDROID_KHR
      GetMemoryAndroidHardwareBufferANDROID = (PFN_vkGetMemoryAndroidHardwareBufferANDROID) NextGetDeviceProcAddr(device, "vkGetMemoryAndroidHardwareBufferANDROID");
#endif
      GetMemoryFdKHR = (PFN_vkGetMemoryFdKHR) NextGetDeviceProcAddr(device, "vkGetMemoryFdKHR");
      GetMemoryFdPropertiesKHR = (PFN_vkGetMemoryFdPropertiesKHR) NextGetDeviceProcAddr(device, "vkGetMemoryFdPropertiesKHR");
      GetMemoryHostPointerPropertiesEXT = (PFN_vkGetMemoryHostPointerPropertiesEXT) NextGetDeviceProcAddr(device, "vkGetMemoryHostPointerPropertiesEXT");
      GetMemoryRemoteAddressNV = (PFN_vkGetMemoryRemoteAddressNV) NextGetDeviceProcAddr(device, "vkGetMemoryRemoteAddressNV");
#ifdef VK_USE_PLATFORM_WIN32_KHR
      GetMemoryWin32HandleKHR = (PFN_vkGetMemoryWin32HandleKHR) NextGetDeviceProcAddr(device, "vkGetMemoryWin32HandleKHR");
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
      GetMemoryWin32HandleNV = (PFN_vkGetMemoryWin32HandleNV) NextGetDeviceProcAddr(device, "vkGetMemoryWin32HandleNV");
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
      GetMemoryWin32HandlePropertiesKHR = (PFN_vkGetMemoryWin32HandlePropertiesKHR) NextGetDeviceProcAddr(device, "vkGetMemoryWin32HandlePropertiesKHR");
#endif
#ifdef VK_USE_PLATFORM_FUCHSIA
      GetMemoryZirconHandleFUCHSIA = (PFN_vkGetMemoryZirconHandleFUCHSIA) NextGetDeviceProcAddr(device, "vkGetMemoryZirconHandleFUCHSIA");
#endif
#ifdef VK_USE_PLATFORM_FUCHSIA
      GetMemoryZirconHandlePropertiesFUCHSIA = (PFN_vkGetMemoryZirconHandlePropertiesFUCHSIA) NextGetDeviceProcAddr(device, "vkGetMemoryZirconHandlePropertiesFUCHSIA");
#endif
      GetPastPresentationTimingGOOGLE = (PFN_vkGetPastPresentationTimingGOOGLE) NextGetDeviceProcAddr(device, "vkGetPastPresentationTimingGOOGLE");
      GetPerformanceParameterINTEL = (PFN_vkGetPerformanceParameterINTEL) NextGetDeviceProcAddr(device, "vkGetPerformanceParameterINTEL");
      GetPipelineCacheData = (PFN_vkGetPipelineCacheData) NextGetDeviceProcAddr(device, "vkGetPipelineCacheData");
      GetPipelineExecutableInternalRepresentationsKHR = (PFN_vkGetPipelineExecutableInternalRepresentationsKHR) NextGetDeviceProcAddr(device, "vkGetPipelineExecutableInternalRepresentationsKHR");
      GetPipelineExecutablePropertiesKHR = (PFN_vkGetPipelineExecutablePropertiesKHR) NextGetDeviceProcAddr(device, "vkGetPipelineExecutablePropertiesKHR");
      GetPipelineExecutableStatisticsKHR = (PFN_vkGetPipelineExecutableStatisticsKHR) NextGetDeviceProcAddr(device, "vkGetPipelineExecutableStatisticsKHR");
      GetPipelinePropertiesEXT = (PFN_vkGetPipelinePropertiesEXT) NextGetDeviceProcAddr(device, "vkGetPipelinePropertiesEXT");
      GetPrivateData = (PFN_vkGetPrivateData) NextGetDeviceProcAddr(device, "vkGetPrivateData");
      GetPrivateDataEXT = (PFN_vkGetPrivateDataEXT) NextGetDeviceProcAddr(device, "vkGetPrivateDataEXT");
      GetQueryPoolResults = (PFN_vkGetQueryPoolResults) NextGetDeviceProcAddr(device, "vkGetQueryPoolResults");
      GetQueueCheckpointData2NV = (PFN_vkGetQueueCheckpointData2NV) NextGetDeviceProcAddr(device, "vkGetQueueCheckpointData2NV");
      GetQueueCheckpointDataNV = (PFN_vkGetQueueCheckpointDataNV) NextGetDeviceProcAddr(device, "vkGetQueueCheckpointDataNV");
      GetRayTracingCaptureReplayShaderGroupHandlesKHR = (PFN_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR) NextGetDeviceProcAddr(device, "vkGetRayTracingCaptureReplayShaderGroupHandlesKHR");
      GetRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR) NextGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR");
      GetRayTracingShaderGroupHandlesNV = (PFN_vkGetRayTracingShaderGroupHandlesNV) NextGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesNV");
      GetRayTracingShaderGroupStackSizeKHR = (PFN_vkGetRayTracingShaderGroupStackSizeKHR) NextGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupStackSizeKHR");
      GetRefreshCycleDurationGOOGLE = (PFN_vkGetRefreshCycleDurationGOOGLE) NextGetDeviceProcAddr(device, "vkGetRefreshCycleDurationGOOGLE");
      GetRenderAreaGranularity = (PFN_vkGetRenderAreaGranularity) NextGetDeviceProcAddr(device, "vkGetRenderAreaGranularity");
      GetSemaphoreCounterValue = (PFN_vkGetSemaphoreCounterValue) NextGetDeviceProcAddr(device, "vkGetSemaphoreCounterValue");
      GetSemaphoreCounterValueKHR = (PFN_vkGetSemaphoreCounterValueKHR) NextGetDeviceProcAddr(device, "vkGetSemaphoreCounterValueKHR");
      GetSemaphoreFdKHR = (PFN_vkGetSemaphoreFdKHR) NextGetDeviceProcAddr(device, "vkGetSemaphoreFdKHR");
#ifdef VK_USE_PLATFORM_WIN32_KHR
      GetSemaphoreWin32HandleKHR = (PFN_vkGetSemaphoreWin32HandleKHR) NextGetDeviceProcAddr(device, "vkGetSemaphoreWin32HandleKHR");
#endif
#ifdef VK_USE_PLATFORM_FUCHSIA
      GetSemaphoreZirconHandleFUCHSIA = (PFN_vkGetSemaphoreZirconHandleFUCHSIA) NextGetDeviceProcAddr(device, "vkGetSemaphoreZirconHandleFUCHSIA");
#endif
      GetShaderInfoAMD = (PFN_vkGetShaderInfoAMD) NextGetDeviceProcAddr(device, "vkGetShaderInfoAMD");
      GetShaderModuleCreateInfoIdentifierEXT = (PFN_vkGetShaderModuleCreateInfoIdentifierEXT) NextGetDeviceProcAddr(device, "vkGetShaderModuleCreateInfoIdentifierEXT");
      GetShaderModuleIdentifierEXT = (PFN_vkGetShaderModuleIdentifierEXT) NextGetDeviceProcAddr(device, "vkGetShaderModuleIdentifierEXT");
      GetSwapchainCounterEXT = (PFN_vkGetSwapchainCounterEXT) NextGetDeviceProcAddr(device, "vkGetSwapchainCounterEXT");
#ifdef VKROOTS_PLATFORM_UNSUPPORTED
      GetSwapchainGrallocUsage2ANDROID = (PFN_vkGetSwapchainGrallocUsage2ANDROID) NextGetDeviceProcAddr(device, "vkGetSwapchainGrallocUsage2ANDROID");
#endif
#ifdef VKROOTS_PLATFORM_UNSUPPORTED
      GetSwapchainGrallocUsageANDROID = (PFN_vkGetSwapchainGrallocUsageANDROID) NextGetDeviceProcAddr(device, "vkGetSwapchainGrallocUsageANDROID");
#endif
      GetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR) NextGetDeviceProcAddr(device, "vkGetSwapchainImagesKHR");
      GetSwapchainStatusKHR = (PFN_vkGetSwapchainStatusKHR) NextGetDeviceProcAddr(device, "vkGetSwapchainStatusKHR");
      GetValidationCacheDataEXT = (PFN_vkGetValidationCacheDataEXT) NextGetDeviceProcAddr(device, "vkGetValidationCacheDataEXT");
#ifdef VK_ENABLE_BETA_EXTENSIONS
      GetVideoSessionMemoryRequirementsKHR = (PFN_vkGetVideoSessionMemoryRequirementsKHR) NextGetDeviceProcAddr(device, "vkGetVideoSessionMemoryRequirementsKHR");
#endif
      ImportFenceFdKHR = (PFN_vkImportFenceFdKHR) NextGetDeviceProcAddr(device, "vkImportFenceFdKHR");
#ifdef VK_USE_PLATFORM_WIN32_KHR
      ImportFenceWin32HandleKHR = (PFN_vkImportFenceWin32HandleKHR) NextGetDeviceProcAddr(device, "vkImportFenceWin32HandleKHR");
#endif
      ImportSemaphoreFdKHR = (PFN_vkImportSemaphoreFdKHR) NextGetDeviceProcAddr(device, "vkImportSemaphoreFdKHR");
#ifdef VK_USE_PLATFORM_WIN32_KHR
      ImportSemaphoreWin32HandleKHR = (PFN_vkImportSemaphoreWin32HandleKHR) NextGetDeviceProcAddr(device, "vkImportSemaphoreWin32HandleKHR");
#endif
#ifdef VK_USE_PLATFORM_FUCHSIA
      ImportSemaphoreZirconHandleFUCHSIA = (PFN_vkImportSemaphoreZirconHandleFUCHSIA) NextGetDeviceProcAddr(device, "vkImportSemaphoreZirconHandleFUCHSIA");
#endif
      InitializePerformanceApiINTEL = (PFN_vkInitializePerformanceApiINTEL) NextGetDeviceProcAddr(device, "vkInitializePerformanceApiINTEL");
      InvalidateMappedMemoryRanges = (PFN_vkInvalidateMappedMemoryRanges) NextGetDeviceProcAddr(device, "vkInvalidateMappedMemoryRanges");
      MapMemory = (PFN_vkMapMemory) NextGetDeviceProcAddr(device, "vkMapMemory");
      MergePipelineCaches = (PFN_vkMergePipelineCaches) NextGetDeviceProcAddr(device, "vkMergePipelineCaches");
      MergeValidationCachesEXT = (PFN_vkMergeValidationCachesEXT) NextGetDeviceProcAddr(device, "vkMergeValidationCachesEXT");
      QueueBeginDebugUtilsLabelEXT = (PFN_vkQueueBeginDebugUtilsLabelEXT) NextGetDeviceProcAddr(device, "vkQueueBeginDebugUtilsLabelEXT");
      QueueBindSparse = (PFN_vkQueueBindSparse) NextGetDeviceProcAddr(device, "vkQueueBindSparse");
      QueueEndDebugUtilsLabelEXT = (PFN_vkQueueEndDebugUtilsLabelEXT) NextGetDeviceProcAddr(device, "vkQueueEndDebugUtilsLabelEXT");
      QueueInsertDebugUtilsLabelEXT = (PFN_vkQueueInsertDebugUtilsLabelEXT) NextGetDeviceProcAddr(device, "vkQueueInsertDebugUtilsLabelEXT");
      QueuePresentKHR = (PFN_vkQueuePresentKHR) NextGetDeviceProcAddr(device, "vkQueuePresentKHR");
      QueueSetPerformanceConfigurationINTEL = (PFN_vkQueueSetPerformanceConfigurationINTEL) NextGetDeviceProcAddr(device, "vkQueueSetPerformanceConfigurationINTEL");
#ifdef VKROOTS_PLATFORM_UNSUPPORTED
      QueueSignalReleaseImageANDROID = (PFN_vkQueueSignalReleaseImageANDROID) NextGetDeviceProcAddr(device, "vkQueueSignalReleaseImageANDROID");
#endif
      QueueSubmit = (PFN_vkQueueSubmit) NextGetDeviceProcAddr(device, "vkQueueSubmit");
      QueueSubmit2 = (PFN_vkQueueSubmit2) NextGetDeviceProcAddr(device, "vkQueueSubmit2");
      QueueSubmit2KHR = (PFN_vkQueueSubmit2KHR) NextGetDeviceProcAddr(device, "vkQueueSubmit2KHR");
      QueueWaitIdle = (PFN_vkQueueWaitIdle) NextGetDeviceProcAddr(device, "vkQueueWaitIdle");
      RegisterDeviceEventEXT = (PFN_vkRegisterDeviceEventEXT) NextGetDeviceProcAddr(device, "vkRegisterDeviceEventEXT");
      RegisterDisplayEventEXT = (PFN_vkRegisterDisplayEventEXT) NextGetDeviceProcAddr(device, "vkRegisterDisplayEventEXT");
#ifdef VK_USE_PLATFORM_WIN32_KHR
      ReleaseFullScreenExclusiveModeEXT = (PFN_vkReleaseFullScreenExclusiveModeEXT) NextGetDeviceProcAddr(device, "vkReleaseFullScreenExclusiveModeEXT");
#endif
      ReleasePerformanceConfigurationINTEL = (PFN_vkReleasePerformanceConfigurationINTEL) NextGetDeviceProcAddr(device, "vkReleasePerformanceConfigurationINTEL");
      ReleaseProfilingLockKHR = (PFN_vkReleaseProfilingLockKHR) NextGetDeviceProcAddr(device, "vkReleaseProfilingLockKHR");
      ResetCommandBuffer = (PFN_vkResetCommandBuffer) NextGetDeviceProcAddr(device, "vkResetCommandBuffer");
      ResetCommandPool = (PFN_vkResetCommandPool) NextGetDeviceProcAddr(device, "vkResetCommandPool");
      ResetDescriptorPool = (PFN_vkResetDescriptorPool) NextGetDeviceProcAddr(device, "vkResetDescriptorPool");
      ResetEvent = (PFN_vkResetEvent) NextGetDeviceProcAddr(device, "vkResetEvent");
      ResetFences = (PFN_vkResetFences) NextGetDeviceProcAddr(device, "vkResetFences");
      ResetQueryPool = (PFN_vkResetQueryPool) NextGetDeviceProcAddr(device, "vkResetQueryPool");
      ResetQueryPoolEXT = (PFN_vkResetQueryPoolEXT) NextGetDeviceProcAddr(device, "vkResetQueryPoolEXT");
#ifdef VK_USE_PLATFORM_FUCHSIA
      SetBufferCollectionBufferConstraintsFUCHSIA = (PFN_vkSetBufferCollectionBufferConstraintsFUCHSIA) NextGetDeviceProcAddr(device, "vkSetBufferCollectionBufferConstraintsFUCHSIA");
#endif
#ifdef VK_USE_PLATFORM_FUCHSIA
      SetBufferCollectionImageConstraintsFUCHSIA = (PFN_vkSetBufferCollectionImageConstraintsFUCHSIA) NextGetDeviceProcAddr(device, "vkSetBufferCollectionImageConstraintsFUCHSIA");
#endif
      SetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT) NextGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT");
      SetDebugUtilsObjectTagEXT = (PFN_vkSetDebugUtilsObjectTagEXT) NextGetDeviceProcAddr(device, "vkSetDebugUtilsObjectTagEXT");
      SetDeviceMemoryPriorityEXT = (PFN_vkSetDeviceMemoryPriorityEXT) NextGetDeviceProcAddr(device, "vkSetDeviceMemoryPriorityEXT");
      SetEvent = (PFN_vkSetEvent) NextGetDeviceProcAddr(device, "vkSetEvent");
      SetHdrMetadataEXT = (PFN_vkSetHdrMetadataEXT) NextGetDeviceProcAddr(device, "vkSetHdrMetadataEXT");
      SetLocalDimmingAMD = (PFN_vkSetLocalDimmingAMD) NextGetDeviceProcAddr(device, "vkSetLocalDimmingAMD");
      SetPrivateData = (PFN_vkSetPrivateData) NextGetDeviceProcAddr(device, "vkSetPrivateData");
      SetPrivateDataEXT = (PFN_vkSetPrivateDataEXT) NextGetDeviceProcAddr(device, "vkSetPrivateDataEXT");
      SignalSemaphore = (PFN_vkSignalSemaphore) NextGetDeviceProcAddr(device, "vkSignalSemaphore");
      SignalSemaphoreKHR = (PFN_vkSignalSemaphoreKHR) NextGetDeviceProcAddr(device, "vkSignalSemaphoreKHR");
      TrimCommandPool = (PFN_vkTrimCommandPool) NextGetDeviceProcAddr(device, "vkTrimCommandPool");
      TrimCommandPoolKHR = (PFN_vkTrimCommandPoolKHR) NextGetDeviceProcAddr(device, "vkTrimCommandPoolKHR");
      UninitializePerformanceApiINTEL = (PFN_vkUninitializePerformanceApiINTEL) NextGetDeviceProcAddr(device, "vkUninitializePerformanceApiINTEL");
      UnmapMemory = (PFN_vkUnmapMemory) NextGetDeviceProcAddr(device, "vkUnmapMemory");
      UpdateDescriptorSetWithTemplate = (PFN_vkUpdateDescriptorSetWithTemplate) NextGetDeviceProcAddr(device, "vkUpdateDescriptorSetWithTemplate");
      UpdateDescriptorSetWithTemplateKHR = (PFN_vkUpdateDescriptorSetWithTemplateKHR) NextGetDeviceProcAddr(device, "vkUpdateDescriptorSetWithTemplateKHR");
      UpdateDescriptorSets = (PFN_vkUpdateDescriptorSets) NextGetDeviceProcAddr(device, "vkUpdateDescriptorSets");
#ifdef VK_ENABLE_BETA_EXTENSIONS
      UpdateVideoSessionParametersKHR = (PFN_vkUpdateVideoSessionParametersKHR) NextGetDeviceProcAddr(device, "vkUpdateVideoSessionParametersKHR");
#endif
      WaitForFences = (PFN_vkWaitForFences) NextGetDeviceProcAddr(device, "vkWaitForFences");
      WaitForPresentKHR = (PFN_vkWaitForPresentKHR) NextGetDeviceProcAddr(device, "vkWaitForPresentKHR");
      WaitSemaphores = (PFN_vkWaitSemaphores) NextGetDeviceProcAddr(device, "vkWaitSemaphores");
      WaitSemaphoresKHR = (PFN_vkWaitSemaphoresKHR) NextGetDeviceProcAddr(device, "vkWaitSemaphoresKHR");
      WriteAccelerationStructuresPropertiesKHR = (PFN_vkWriteAccelerationStructuresPropertiesKHR) NextGetDeviceProcAddr(device, "vkWriteAccelerationStructuresPropertiesKHR");
    }

    mutable uint64_t UserData = 0;
    VkDevice Device;
    VkPhysicalDevice PhysicalDevice;
    const VkPhysicalDeviceDispatch* pPhysicalDeviceDispatch;
    std::vector<VkDeviceQueueCreateInfo> DeviceQueueInfos;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkAcquireFullScreenExclusiveModeEXT AcquireFullScreenExclusiveModeEXT;
#endif
#ifdef VKROOTS_PLATFORM_UNSUPPORTED
    PFN_vkAcquireImageANDROID AcquireImageANDROID;
#endif
    PFN_vkAcquireNextImage2KHR AcquireNextImage2KHR;
    PFN_vkAcquireNextImageKHR AcquireNextImageKHR;
    PFN_vkAcquirePerformanceConfigurationINTEL AcquirePerformanceConfigurationINTEL;
    PFN_vkAcquireProfilingLockKHR AcquireProfilingLockKHR;
    PFN_vkAllocateCommandBuffers AllocateCommandBuffers;
    PFN_vkAllocateDescriptorSets AllocateDescriptorSets;
    PFN_vkAllocateMemory AllocateMemory;
    PFN_vkBeginCommandBuffer BeginCommandBuffer;
    PFN_vkBindAccelerationStructureMemoryNV BindAccelerationStructureMemoryNV;
    PFN_vkBindBufferMemory BindBufferMemory;
    PFN_vkBindBufferMemory2 BindBufferMemory2;
    PFN_vkBindBufferMemory2KHR BindBufferMemory2KHR;
    PFN_vkBindImageMemory BindImageMemory;
    PFN_vkBindImageMemory2 BindImageMemory2;
    PFN_vkBindImageMemory2KHR BindImageMemory2KHR;
#ifdef VK_ENABLE_BETA_EXTENSIONS
    PFN_vkBindVideoSessionMemoryKHR BindVideoSessionMemoryKHR;
#endif
    PFN_vkBuildAccelerationStructuresKHR BuildAccelerationStructuresKHR;
    PFN_vkCmdBeginConditionalRenderingEXT CmdBeginConditionalRenderingEXT;
    PFN_vkCmdBeginDebugUtilsLabelEXT CmdBeginDebugUtilsLabelEXT;
    PFN_vkCmdBeginQuery CmdBeginQuery;
    PFN_vkCmdBeginQueryIndexedEXT CmdBeginQueryIndexedEXT;
    PFN_vkCmdBeginRenderPass CmdBeginRenderPass;
    PFN_vkCmdBeginRenderPass2 CmdBeginRenderPass2;
    PFN_vkCmdBeginRenderPass2KHR CmdBeginRenderPass2KHR;
    PFN_vkCmdBeginRendering CmdBeginRendering;
    PFN_vkCmdBeginRenderingKHR CmdBeginRenderingKHR;
    PFN_vkCmdBeginTransformFeedbackEXT CmdBeginTransformFeedbackEXT;
#ifdef VK_ENABLE_BETA_EXTENSIONS
    PFN_vkCmdBeginVideoCodingKHR CmdBeginVideoCodingKHR;
#endif
    PFN_vkCmdBindDescriptorSets CmdBindDescriptorSets;
    PFN_vkCmdBindIndexBuffer CmdBindIndexBuffer;
    PFN_vkCmdBindInvocationMaskHUAWEI CmdBindInvocationMaskHUAWEI;
    PFN_vkCmdBindPipeline CmdBindPipeline;
    PFN_vkCmdBindPipelineShaderGroupNV CmdBindPipelineShaderGroupNV;
    PFN_vkCmdBindShadingRateImageNV CmdBindShadingRateImageNV;
    PFN_vkCmdBindTransformFeedbackBuffersEXT CmdBindTransformFeedbackBuffersEXT;
    PFN_vkCmdBindVertexBuffers CmdBindVertexBuffers;
    PFN_vkCmdBindVertexBuffers2 CmdBindVertexBuffers2;
    PFN_vkCmdBindVertexBuffers2EXT CmdBindVertexBuffers2EXT;
    PFN_vkCmdBlitImage CmdBlitImage;
    PFN_vkCmdBlitImage2 CmdBlitImage2;
    PFN_vkCmdBlitImage2KHR CmdBlitImage2KHR;
    PFN_vkCmdBuildAccelerationStructureNV CmdBuildAccelerationStructureNV;
    PFN_vkCmdBuildAccelerationStructuresIndirectKHR CmdBuildAccelerationStructuresIndirectKHR;
    PFN_vkCmdBuildAccelerationStructuresKHR CmdBuildAccelerationStructuresKHR;
    PFN_vkCmdClearAttachments CmdClearAttachments;
    PFN_vkCmdClearColorImage CmdClearColorImage;
    PFN_vkCmdClearDepthStencilImage CmdClearDepthStencilImage;
#ifdef VK_ENABLE_BETA_EXTENSIONS
    PFN_vkCmdControlVideoCodingKHR CmdControlVideoCodingKHR;
#endif
    PFN_vkCmdCopyAccelerationStructureKHR CmdCopyAccelerationStructureKHR;
    PFN_vkCmdCopyAccelerationStructureNV CmdCopyAccelerationStructureNV;
    PFN_vkCmdCopyAccelerationStructureToMemoryKHR CmdCopyAccelerationStructureToMemoryKHR;
    PFN_vkCmdCopyBuffer CmdCopyBuffer;
    PFN_vkCmdCopyBuffer2 CmdCopyBuffer2;
    PFN_vkCmdCopyBuffer2KHR CmdCopyBuffer2KHR;
    PFN_vkCmdCopyBufferToImage CmdCopyBufferToImage;
    PFN_vkCmdCopyBufferToImage2 CmdCopyBufferToImage2;
    PFN_vkCmdCopyBufferToImage2KHR CmdCopyBufferToImage2KHR;
    PFN_vkCmdCopyImage CmdCopyImage;
    PFN_vkCmdCopyImage2 CmdCopyImage2;
    PFN_vkCmdCopyImage2KHR CmdCopyImage2KHR;
    PFN_vkCmdCopyImageToBuffer CmdCopyImageToBuffer;
    PFN_vkCmdCopyImageToBuffer2 CmdCopyImageToBuffer2;
    PFN_vkCmdCopyImageToBuffer2KHR CmdCopyImageToBuffer2KHR;
    PFN_vkCmdCopyMemoryToAccelerationStructureKHR CmdCopyMemoryToAccelerationStructureKHR;
    PFN_vkCmdCopyQueryPoolResults CmdCopyQueryPoolResults;
#ifdef VKROOTS_PLATFORM_UNSUPPORTED
    PFN_vkCmdCuLaunchKernelNVX CmdCuLaunchKernelNVX;
#endif
    PFN_vkCmdDebugMarkerBeginEXT CmdDebugMarkerBeginEXT;
    PFN_vkCmdDebugMarkerEndEXT CmdDebugMarkerEndEXT;
    PFN_vkCmdDebugMarkerInsertEXT CmdDebugMarkerInsertEXT;
#ifdef VK_ENABLE_BETA_EXTENSIONS
    PFN_vkCmdDecodeVideoKHR CmdDecodeVideoKHR;
#endif
    PFN_vkCmdDispatch CmdDispatch;
    PFN_vkCmdDispatchBase CmdDispatchBase;
    PFN_vkCmdDispatchBaseKHR CmdDispatchBaseKHR;
    PFN_vkCmdDispatchIndirect CmdDispatchIndirect;
    PFN_vkCmdDraw CmdDraw;
    PFN_vkCmdDrawIndexed CmdDrawIndexed;
    PFN_vkCmdDrawIndexedIndirect CmdDrawIndexedIndirect;
    PFN_vkCmdDrawIndexedIndirectCount CmdDrawIndexedIndirectCount;
    PFN_vkCmdDrawIndexedIndirectCountAMD CmdDrawIndexedIndirectCountAMD;
    PFN_vkCmdDrawIndexedIndirectCountKHR CmdDrawIndexedIndirectCountKHR;
    PFN_vkCmdDrawIndirect CmdDrawIndirect;
    PFN_vkCmdDrawIndirectByteCountEXT CmdDrawIndirectByteCountEXT;
    PFN_vkCmdDrawIndirectCount CmdDrawIndirectCount;
    PFN_vkCmdDrawIndirectCountAMD CmdDrawIndirectCountAMD;
    PFN_vkCmdDrawIndirectCountKHR CmdDrawIndirectCountKHR;
    PFN_vkCmdDrawMeshTasksIndirectCountNV CmdDrawMeshTasksIndirectCountNV;
    PFN_vkCmdDrawMeshTasksIndirectNV CmdDrawMeshTasksIndirectNV;
    PFN_vkCmdDrawMeshTasksNV CmdDrawMeshTasksNV;
    PFN_vkCmdDrawMultiEXT CmdDrawMultiEXT;
    PFN_vkCmdDrawMultiIndexedEXT CmdDrawMultiIndexedEXT;
#ifdef VK_ENABLE_BETA_EXTENSIONS
    PFN_vkCmdEncodeVideoKHR CmdEncodeVideoKHR;
#endif
    PFN_vkCmdEndConditionalRenderingEXT CmdEndConditionalRenderingEXT;
    PFN_vkCmdEndDebugUtilsLabelEXT CmdEndDebugUtilsLabelEXT;
    PFN_vkCmdEndQuery CmdEndQuery;
    PFN_vkCmdEndQueryIndexedEXT CmdEndQueryIndexedEXT;
    PFN_vkCmdEndRenderPass CmdEndRenderPass;
    PFN_vkCmdEndRenderPass2 CmdEndRenderPass2;
    PFN_vkCmdEndRenderPass2KHR CmdEndRenderPass2KHR;
    PFN_vkCmdEndRendering CmdEndRendering;
    PFN_vkCmdEndRenderingKHR CmdEndRenderingKHR;
    PFN_vkCmdEndTransformFeedbackEXT CmdEndTransformFeedbackEXT;
#ifdef VK_ENABLE_BETA_EXTENSIONS
    PFN_vkCmdEndVideoCodingKHR CmdEndVideoCodingKHR;
#endif
    PFN_vkCmdExecuteCommands CmdExecuteCommands;
    PFN_vkCmdExecuteGeneratedCommandsNV CmdExecuteGeneratedCommandsNV;
    PFN_vkCmdFillBuffer CmdFillBuffer;
    PFN_vkCmdInsertDebugUtilsLabelEXT CmdInsertDebugUtilsLabelEXT;
    PFN_vkCmdNextSubpass CmdNextSubpass;
    PFN_vkCmdNextSubpass2 CmdNextSubpass2;
    PFN_vkCmdNextSubpass2KHR CmdNextSubpass2KHR;
    PFN_vkCmdPipelineBarrier CmdPipelineBarrier;
    PFN_vkCmdPipelineBarrier2 CmdPipelineBarrier2;
    PFN_vkCmdPipelineBarrier2KHR CmdPipelineBarrier2KHR;
    PFN_vkCmdPreprocessGeneratedCommandsNV CmdPreprocessGeneratedCommandsNV;
    PFN_vkCmdPushConstants CmdPushConstants;
    PFN_vkCmdPushDescriptorSetKHR CmdPushDescriptorSetKHR;
    PFN_vkCmdPushDescriptorSetWithTemplateKHR CmdPushDescriptorSetWithTemplateKHR;
    PFN_vkCmdResetEvent CmdResetEvent;
    PFN_vkCmdResetEvent2 CmdResetEvent2;
    PFN_vkCmdResetEvent2KHR CmdResetEvent2KHR;
    PFN_vkCmdResetQueryPool CmdResetQueryPool;
    PFN_vkCmdResolveImage CmdResolveImage;
    PFN_vkCmdResolveImage2 CmdResolveImage2;
    PFN_vkCmdResolveImage2KHR CmdResolveImage2KHR;
    PFN_vkCmdSetBlendConstants CmdSetBlendConstants;
    PFN_vkCmdSetCheckpointNV CmdSetCheckpointNV;
    PFN_vkCmdSetCoarseSampleOrderNV CmdSetCoarseSampleOrderNV;
    PFN_vkCmdSetColorWriteEnableEXT CmdSetColorWriteEnableEXT;
    PFN_vkCmdSetCullMode CmdSetCullMode;
    PFN_vkCmdSetCullModeEXT CmdSetCullModeEXT;
    PFN_vkCmdSetDepthBias CmdSetDepthBias;
    PFN_vkCmdSetDepthBiasEnable CmdSetDepthBiasEnable;
    PFN_vkCmdSetDepthBiasEnableEXT CmdSetDepthBiasEnableEXT;
    PFN_vkCmdSetDepthBounds CmdSetDepthBounds;
    PFN_vkCmdSetDepthBoundsTestEnable CmdSetDepthBoundsTestEnable;
    PFN_vkCmdSetDepthBoundsTestEnableEXT CmdSetDepthBoundsTestEnableEXT;
    PFN_vkCmdSetDepthCompareOp CmdSetDepthCompareOp;
    PFN_vkCmdSetDepthCompareOpEXT CmdSetDepthCompareOpEXT;
    PFN_vkCmdSetDepthTestEnable CmdSetDepthTestEnable;
    PFN_vkCmdSetDepthTestEnableEXT CmdSetDepthTestEnableEXT;
    PFN_vkCmdSetDepthWriteEnable CmdSetDepthWriteEnable;
    PFN_vkCmdSetDepthWriteEnableEXT CmdSetDepthWriteEnableEXT;
    PFN_vkCmdSetDeviceMask CmdSetDeviceMask;
    PFN_vkCmdSetDeviceMaskKHR CmdSetDeviceMaskKHR;
    PFN_vkCmdSetDiscardRectangleEXT CmdSetDiscardRectangleEXT;
    PFN_vkCmdSetEvent CmdSetEvent;
    PFN_vkCmdSetEvent2 CmdSetEvent2;
    PFN_vkCmdSetEvent2KHR CmdSetEvent2KHR;
    PFN_vkCmdSetExclusiveScissorNV CmdSetExclusiveScissorNV;
    PFN_vkCmdSetFragmentShadingRateEnumNV CmdSetFragmentShadingRateEnumNV;
    PFN_vkCmdSetFragmentShadingRateKHR CmdSetFragmentShadingRateKHR;
    PFN_vkCmdSetFrontFace CmdSetFrontFace;
    PFN_vkCmdSetFrontFaceEXT CmdSetFrontFaceEXT;
    PFN_vkCmdSetLineStippleEXT CmdSetLineStippleEXT;
    PFN_vkCmdSetLineWidth CmdSetLineWidth;
    PFN_vkCmdSetLogicOpEXT CmdSetLogicOpEXT;
    PFN_vkCmdSetPatchControlPointsEXT CmdSetPatchControlPointsEXT;
    PFN_vkCmdSetPerformanceMarkerINTEL CmdSetPerformanceMarkerINTEL;
    PFN_vkCmdSetPerformanceOverrideINTEL CmdSetPerformanceOverrideINTEL;
    PFN_vkCmdSetPerformanceStreamMarkerINTEL CmdSetPerformanceStreamMarkerINTEL;
    PFN_vkCmdSetPrimitiveRestartEnable CmdSetPrimitiveRestartEnable;
    PFN_vkCmdSetPrimitiveRestartEnableEXT CmdSetPrimitiveRestartEnableEXT;
    PFN_vkCmdSetPrimitiveTopology CmdSetPrimitiveTopology;
    PFN_vkCmdSetPrimitiveTopologyEXT CmdSetPrimitiveTopologyEXT;
    PFN_vkCmdSetRasterizerDiscardEnable CmdSetRasterizerDiscardEnable;
    PFN_vkCmdSetRasterizerDiscardEnableEXT CmdSetRasterizerDiscardEnableEXT;
    PFN_vkCmdSetRayTracingPipelineStackSizeKHR CmdSetRayTracingPipelineStackSizeKHR;
    PFN_vkCmdSetSampleLocationsEXT CmdSetSampleLocationsEXT;
    PFN_vkCmdSetScissor CmdSetScissor;
    PFN_vkCmdSetScissorWithCount CmdSetScissorWithCount;
    PFN_vkCmdSetScissorWithCountEXT CmdSetScissorWithCountEXT;
    PFN_vkCmdSetStencilCompareMask CmdSetStencilCompareMask;
    PFN_vkCmdSetStencilOp CmdSetStencilOp;
    PFN_vkCmdSetStencilOpEXT CmdSetStencilOpEXT;
    PFN_vkCmdSetStencilReference CmdSetStencilReference;
    PFN_vkCmdSetStencilTestEnable CmdSetStencilTestEnable;
    PFN_vkCmdSetStencilTestEnableEXT CmdSetStencilTestEnableEXT;
    PFN_vkCmdSetStencilWriteMask CmdSetStencilWriteMask;
    PFN_vkCmdSetVertexInputEXT CmdSetVertexInputEXT;
    PFN_vkCmdSetViewport CmdSetViewport;
    PFN_vkCmdSetViewportShadingRatePaletteNV CmdSetViewportShadingRatePaletteNV;
    PFN_vkCmdSetViewportWScalingNV CmdSetViewportWScalingNV;
    PFN_vkCmdSetViewportWithCount CmdSetViewportWithCount;
    PFN_vkCmdSetViewportWithCountEXT CmdSetViewportWithCountEXT;
    PFN_vkCmdSubpassShadingHUAWEI CmdSubpassShadingHUAWEI;
    PFN_vkCmdTraceRaysIndirect2KHR CmdTraceRaysIndirect2KHR;
    PFN_vkCmdTraceRaysIndirectKHR CmdTraceRaysIndirectKHR;
    PFN_vkCmdTraceRaysKHR CmdTraceRaysKHR;
    PFN_vkCmdTraceRaysNV CmdTraceRaysNV;
    PFN_vkCmdUpdateBuffer CmdUpdateBuffer;
    PFN_vkCmdWaitEvents CmdWaitEvents;
    PFN_vkCmdWaitEvents2 CmdWaitEvents2;
    PFN_vkCmdWaitEvents2KHR CmdWaitEvents2KHR;
    PFN_vkCmdWriteAccelerationStructuresPropertiesKHR CmdWriteAccelerationStructuresPropertiesKHR;
    PFN_vkCmdWriteAccelerationStructuresPropertiesNV CmdWriteAccelerationStructuresPropertiesNV;
    PFN_vkCmdWriteBufferMarker2AMD CmdWriteBufferMarker2AMD;
    PFN_vkCmdWriteBufferMarkerAMD CmdWriteBufferMarkerAMD;
    PFN_vkCmdWriteTimestamp CmdWriteTimestamp;
    PFN_vkCmdWriteTimestamp2 CmdWriteTimestamp2;
    PFN_vkCmdWriteTimestamp2KHR CmdWriteTimestamp2KHR;
    PFN_vkCompileDeferredNV CompileDeferredNV;
    PFN_vkCopyAccelerationStructureKHR CopyAccelerationStructureKHR;
    PFN_vkCopyAccelerationStructureToMemoryKHR CopyAccelerationStructureToMemoryKHR;
    PFN_vkCopyMemoryToAccelerationStructureKHR CopyMemoryToAccelerationStructureKHR;
    PFN_vkCreateAccelerationStructureKHR CreateAccelerationStructureKHR;
    PFN_vkCreateAccelerationStructureNV CreateAccelerationStructureNV;
    PFN_vkCreateBuffer CreateBuffer;
#ifdef VK_USE_PLATFORM_FUCHSIA
    PFN_vkCreateBufferCollectionFUCHSIA CreateBufferCollectionFUCHSIA;
#endif
    PFN_vkCreateBufferView CreateBufferView;
    PFN_vkCreateCommandPool CreateCommandPool;
    PFN_vkCreateComputePipelines CreateComputePipelines;
#ifdef VKROOTS_PLATFORM_UNSUPPORTED
    PFN_vkCreateCuFunctionNVX CreateCuFunctionNVX;
#endif
#ifdef VKROOTS_PLATFORM_UNSUPPORTED
    PFN_vkCreateCuModuleNVX CreateCuModuleNVX;
#endif
    PFN_vkCreateDeferredOperationKHR CreateDeferredOperationKHR;
    PFN_vkCreateDescriptorPool CreateDescriptorPool;
    PFN_vkCreateDescriptorSetLayout CreateDescriptorSetLayout;
    PFN_vkCreateDescriptorUpdateTemplate CreateDescriptorUpdateTemplate;
    PFN_vkCreateDescriptorUpdateTemplateKHR CreateDescriptorUpdateTemplateKHR;
    PFN_vkCreateEvent CreateEvent;
    PFN_vkCreateFence CreateFence;
    PFN_vkCreateFramebuffer CreateFramebuffer;
    PFN_vkCreateGraphicsPipelines CreateGraphicsPipelines;
    PFN_vkCreateImage CreateImage;
    PFN_vkCreateImageView CreateImageView;
    PFN_vkCreateIndirectCommandsLayoutNV CreateIndirectCommandsLayoutNV;
    PFN_vkCreatePipelineCache CreatePipelineCache;
    PFN_vkCreatePipelineLayout CreatePipelineLayout;
    PFN_vkCreatePrivateDataSlot CreatePrivateDataSlot;
    PFN_vkCreatePrivateDataSlotEXT CreatePrivateDataSlotEXT;
    PFN_vkCreateQueryPool CreateQueryPool;
    PFN_vkCreateRayTracingPipelinesKHR CreateRayTracingPipelinesKHR;
    PFN_vkCreateRayTracingPipelinesNV CreateRayTracingPipelinesNV;
    PFN_vkCreateRenderPass CreateRenderPass;
    PFN_vkCreateRenderPass2 CreateRenderPass2;
    PFN_vkCreateRenderPass2KHR CreateRenderPass2KHR;
    PFN_vkCreateSampler CreateSampler;
    PFN_vkCreateSamplerYcbcrConversion CreateSamplerYcbcrConversion;
    PFN_vkCreateSamplerYcbcrConversionKHR CreateSamplerYcbcrConversionKHR;
    PFN_vkCreateSemaphore CreateSemaphore;
    PFN_vkCreateShaderModule CreateShaderModule;
    PFN_vkCreateSharedSwapchainsKHR CreateSharedSwapchainsKHR;
    PFN_vkCreateSwapchainKHR CreateSwapchainKHR;
    PFN_vkCreateValidationCacheEXT CreateValidationCacheEXT;
#ifdef VK_ENABLE_BETA_EXTENSIONS
    PFN_vkCreateVideoSessionKHR CreateVideoSessionKHR;
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
    PFN_vkCreateVideoSessionParametersKHR CreateVideoSessionParametersKHR;
#endif
    PFN_vkDebugMarkerSetObjectNameEXT DebugMarkerSetObjectNameEXT;
    PFN_vkDebugMarkerSetObjectTagEXT DebugMarkerSetObjectTagEXT;
    PFN_vkDeferredOperationJoinKHR DeferredOperationJoinKHR;
    PFN_vkDestroyAccelerationStructureKHR DestroyAccelerationStructureKHR;
    PFN_vkDestroyAccelerationStructureNV DestroyAccelerationStructureNV;
    PFN_vkDestroyBuffer DestroyBuffer;
#ifdef VK_USE_PLATFORM_FUCHSIA
    PFN_vkDestroyBufferCollectionFUCHSIA DestroyBufferCollectionFUCHSIA;
#endif
    PFN_vkDestroyBufferView DestroyBufferView;
    PFN_vkDestroyCommandPool DestroyCommandPool;
#ifdef VKROOTS_PLATFORM_UNSUPPORTED
    PFN_vkDestroyCuFunctionNVX DestroyCuFunctionNVX;
#endif
#ifdef VKROOTS_PLATFORM_UNSUPPORTED
    PFN_vkDestroyCuModuleNVX DestroyCuModuleNVX;
#endif
    PFN_vkDestroyDeferredOperationKHR DestroyDeferredOperationKHR;
    PFN_vkDestroyDescriptorPool DestroyDescriptorPool;
    PFN_vkDestroyDescriptorSetLayout DestroyDescriptorSetLayout;
    PFN_vkDestroyDescriptorUpdateTemplate DestroyDescriptorUpdateTemplate;
    PFN_vkDestroyDescriptorUpdateTemplateKHR DestroyDescriptorUpdateTemplateKHR;
    PFN_vkDestroyDevice DestroyDevice;
    PFN_vkDestroyEvent DestroyEvent;
    PFN_vkDestroyFence DestroyFence;
    PFN_vkDestroyFramebuffer DestroyFramebuffer;
    PFN_vkDestroyImage DestroyImage;
    PFN_vkDestroyImageView DestroyImageView;
    PFN_vkDestroyIndirectCommandsLayoutNV DestroyIndirectCommandsLayoutNV;
    PFN_vkDestroyPipeline DestroyPipeline;
    PFN_vkDestroyPipelineCache DestroyPipelineCache;
    PFN_vkDestroyPipelineLayout DestroyPipelineLayout;
    PFN_vkDestroyPrivateDataSlot DestroyPrivateDataSlot;
    PFN_vkDestroyPrivateDataSlotEXT DestroyPrivateDataSlotEXT;
    PFN_vkDestroyQueryPool DestroyQueryPool;
    PFN_vkDestroyRenderPass DestroyRenderPass;
    PFN_vkDestroySampler DestroySampler;
    PFN_vkDestroySamplerYcbcrConversion DestroySamplerYcbcrConversion;
    PFN_vkDestroySamplerYcbcrConversionKHR DestroySamplerYcbcrConversionKHR;
    PFN_vkDestroySemaphore DestroySemaphore;
    PFN_vkDestroyShaderModule DestroyShaderModule;
    PFN_vkDestroySwapchainKHR DestroySwapchainKHR;
    PFN_vkDestroyValidationCacheEXT DestroyValidationCacheEXT;
#ifdef VK_ENABLE_BETA_EXTENSIONS
    PFN_vkDestroyVideoSessionKHR DestroyVideoSessionKHR;
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
    PFN_vkDestroyVideoSessionParametersKHR DestroyVideoSessionParametersKHR;
#endif
    PFN_vkDeviceWaitIdle DeviceWaitIdle;
    PFN_vkDisplayPowerControlEXT DisplayPowerControlEXT;
    PFN_vkEndCommandBuffer EndCommandBuffer;
#ifdef VK_USE_PLATFORM_METAL_EXT
    PFN_vkExportMetalObjectsEXT ExportMetalObjectsEXT;
#endif
    PFN_vkFlushMappedMemoryRanges FlushMappedMemoryRanges;
    PFN_vkFreeCommandBuffers FreeCommandBuffers;
    PFN_vkFreeDescriptorSets FreeDescriptorSets;
    PFN_vkFreeMemory FreeMemory;
    PFN_vkGetAccelerationStructureBuildSizesKHR GetAccelerationStructureBuildSizesKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR GetAccelerationStructureDeviceAddressKHR;
    PFN_vkGetAccelerationStructureHandleNV GetAccelerationStructureHandleNV;
    PFN_vkGetAccelerationStructureMemoryRequirementsNV GetAccelerationStructureMemoryRequirementsNV;
#ifdef VK_USE_PLATFORM_ANDROID_KHR
    PFN_vkGetAndroidHardwareBufferPropertiesANDROID GetAndroidHardwareBufferPropertiesANDROID;
#endif
#ifdef VK_USE_PLATFORM_FUCHSIA
    PFN_vkGetBufferCollectionPropertiesFUCHSIA GetBufferCollectionPropertiesFUCHSIA;
#endif
    PFN_vkGetBufferDeviceAddress GetBufferDeviceAddress;
    PFN_vkGetBufferDeviceAddressEXT GetBufferDeviceAddressEXT;
    PFN_vkGetBufferDeviceAddressKHR GetBufferDeviceAddressKHR;
    PFN_vkGetBufferMemoryRequirements GetBufferMemoryRequirements;
    PFN_vkGetBufferMemoryRequirements2 GetBufferMemoryRequirements2;
    PFN_vkGetBufferMemoryRequirements2KHR GetBufferMemoryRequirements2KHR;
    PFN_vkGetBufferOpaqueCaptureAddress GetBufferOpaqueCaptureAddress;
    PFN_vkGetBufferOpaqueCaptureAddressKHR GetBufferOpaqueCaptureAddressKHR;
    PFN_vkGetCalibratedTimestampsEXT GetCalibratedTimestampsEXT;
    PFN_vkGetDeferredOperationMaxConcurrencyKHR GetDeferredOperationMaxConcurrencyKHR;
    PFN_vkGetDeferredOperationResultKHR GetDeferredOperationResultKHR;
    PFN_vkGetDescriptorSetHostMappingVALVE GetDescriptorSetHostMappingVALVE;
    PFN_vkGetDescriptorSetLayoutHostMappingInfoVALVE GetDescriptorSetLayoutHostMappingInfoVALVE;
    PFN_vkGetDescriptorSetLayoutSupport GetDescriptorSetLayoutSupport;
    PFN_vkGetDescriptorSetLayoutSupportKHR GetDescriptorSetLayoutSupportKHR;
    PFN_vkGetDeviceAccelerationStructureCompatibilityKHR GetDeviceAccelerationStructureCompatibilityKHR;
    PFN_vkGetDeviceBufferMemoryRequirements GetDeviceBufferMemoryRequirements;
    PFN_vkGetDeviceBufferMemoryRequirementsKHR GetDeviceBufferMemoryRequirementsKHR;
    PFN_vkGetDeviceGroupPeerMemoryFeatures GetDeviceGroupPeerMemoryFeatures;
    PFN_vkGetDeviceGroupPeerMemoryFeaturesKHR GetDeviceGroupPeerMemoryFeaturesKHR;
    PFN_vkGetDeviceGroupPresentCapabilitiesKHR GetDeviceGroupPresentCapabilitiesKHR;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkGetDeviceGroupSurfacePresentModes2EXT GetDeviceGroupSurfacePresentModes2EXT;
#endif
    PFN_vkGetDeviceGroupSurfacePresentModesKHR GetDeviceGroupSurfacePresentModesKHR;
    PFN_vkGetDeviceImageMemoryRequirements GetDeviceImageMemoryRequirements;
    PFN_vkGetDeviceImageMemoryRequirementsKHR GetDeviceImageMemoryRequirementsKHR;
    PFN_vkGetDeviceImageSparseMemoryRequirements GetDeviceImageSparseMemoryRequirements;
    PFN_vkGetDeviceImageSparseMemoryRequirementsKHR GetDeviceImageSparseMemoryRequirementsKHR;
    PFN_vkGetDeviceMemoryCommitment GetDeviceMemoryCommitment;
    PFN_vkGetDeviceMemoryOpaqueCaptureAddress GetDeviceMemoryOpaqueCaptureAddress;
    PFN_vkGetDeviceMemoryOpaqueCaptureAddressKHR GetDeviceMemoryOpaqueCaptureAddressKHR;
    PFN_vkGetDeviceProcAddr GetDeviceProcAddr;
    PFN_vkGetDeviceQueue GetDeviceQueue;
    PFN_vkGetDeviceQueue2 GetDeviceQueue2;
    PFN_vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI GetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI;
    PFN_vkGetEventStatus GetEventStatus;
    PFN_vkGetFenceFdKHR GetFenceFdKHR;
    PFN_vkGetFenceStatus GetFenceStatus;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkGetFenceWin32HandleKHR GetFenceWin32HandleKHR;
#endif
    PFN_vkGetGeneratedCommandsMemoryRequirementsNV GetGeneratedCommandsMemoryRequirementsNV;
    PFN_vkGetImageDrmFormatModifierPropertiesEXT GetImageDrmFormatModifierPropertiesEXT;
    PFN_vkGetImageMemoryRequirements GetImageMemoryRequirements;
    PFN_vkGetImageMemoryRequirements2 GetImageMemoryRequirements2;
    PFN_vkGetImageMemoryRequirements2KHR GetImageMemoryRequirements2KHR;
    PFN_vkGetImageSparseMemoryRequirements GetImageSparseMemoryRequirements;
    PFN_vkGetImageSparseMemoryRequirements2 GetImageSparseMemoryRequirements2;
    PFN_vkGetImageSparseMemoryRequirements2KHR GetImageSparseMemoryRequirements2KHR;
    PFN_vkGetImageSubresourceLayout GetImageSubresourceLayout;
    PFN_vkGetImageSubresourceLayout2EXT GetImageSubresourceLayout2EXT;
#ifdef VKROOTS_PLATFORM_UNSUPPORTED
    PFN_vkGetImageViewAddressNVX GetImageViewAddressNVX;
#endif
#ifdef VKROOTS_PLATFORM_UNSUPPORTED
    PFN_vkGetImageViewHandleNVX GetImageViewHandleNVX;
#endif
#ifdef VK_USE_PLATFORM_ANDROID_KHR
    PFN_vkGetMemoryAndroidHardwareBufferANDROID GetMemoryAndroidHardwareBufferANDROID;
#endif
    PFN_vkGetMemoryFdKHR GetMemoryFdKHR;
    PFN_vkGetMemoryFdPropertiesKHR GetMemoryFdPropertiesKHR;
    PFN_vkGetMemoryHostPointerPropertiesEXT GetMemoryHostPointerPropertiesEXT;
    PFN_vkGetMemoryRemoteAddressNV GetMemoryRemoteAddressNV;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkGetMemoryWin32HandleKHR GetMemoryWin32HandleKHR;
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkGetMemoryWin32HandleNV GetMemoryWin32HandleNV;
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkGetMemoryWin32HandlePropertiesKHR GetMemoryWin32HandlePropertiesKHR;
#endif
#ifdef VK_USE_PLATFORM_FUCHSIA
    PFN_vkGetMemoryZirconHandleFUCHSIA GetMemoryZirconHandleFUCHSIA;
#endif
#ifdef VK_USE_PLATFORM_FUCHSIA
    PFN_vkGetMemoryZirconHandlePropertiesFUCHSIA GetMemoryZirconHandlePropertiesFUCHSIA;
#endif
    PFN_vkGetPastPresentationTimingGOOGLE GetPastPresentationTimingGOOGLE;
    PFN_vkGetPerformanceParameterINTEL GetPerformanceParameterINTEL;
    PFN_vkGetPipelineCacheData GetPipelineCacheData;
    PFN_vkGetPipelineExecutableInternalRepresentationsKHR GetPipelineExecutableInternalRepresentationsKHR;
    PFN_vkGetPipelineExecutablePropertiesKHR GetPipelineExecutablePropertiesKHR;
    PFN_vkGetPipelineExecutableStatisticsKHR GetPipelineExecutableStatisticsKHR;
    PFN_vkGetPipelinePropertiesEXT GetPipelinePropertiesEXT;
    PFN_vkGetPrivateData GetPrivateData;
    PFN_vkGetPrivateDataEXT GetPrivateDataEXT;
    PFN_vkGetQueryPoolResults GetQueryPoolResults;
    PFN_vkGetQueueCheckpointData2NV GetQueueCheckpointData2NV;
    PFN_vkGetQueueCheckpointDataNV GetQueueCheckpointDataNV;
    PFN_vkGetRayTracingCaptureReplayShaderGroupHandlesKHR GetRayTracingCaptureReplayShaderGroupHandlesKHR;
    PFN_vkGetRayTracingShaderGroupHandlesKHR GetRayTracingShaderGroupHandlesKHR;
    PFN_vkGetRayTracingShaderGroupHandlesNV GetRayTracingShaderGroupHandlesNV;
    PFN_vkGetRayTracingShaderGroupStackSizeKHR GetRayTracingShaderGroupStackSizeKHR;
    PFN_vkGetRefreshCycleDurationGOOGLE GetRefreshCycleDurationGOOGLE;
    PFN_vkGetRenderAreaGranularity GetRenderAreaGranularity;
    PFN_vkGetSemaphoreCounterValue GetSemaphoreCounterValue;
    PFN_vkGetSemaphoreCounterValueKHR GetSemaphoreCounterValueKHR;
    PFN_vkGetSemaphoreFdKHR GetSemaphoreFdKHR;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkGetSemaphoreWin32HandleKHR GetSemaphoreWin32HandleKHR;
#endif
#ifdef VK_USE_PLATFORM_FUCHSIA
    PFN_vkGetSemaphoreZirconHandleFUCHSIA GetSemaphoreZirconHandleFUCHSIA;
#endif
    PFN_vkGetShaderInfoAMD GetShaderInfoAMD;
    PFN_vkGetShaderModuleCreateInfoIdentifierEXT GetShaderModuleCreateInfoIdentifierEXT;
    PFN_vkGetShaderModuleIdentifierEXT GetShaderModuleIdentifierEXT;
    PFN_vkGetSwapchainCounterEXT GetSwapchainCounterEXT;
#ifdef VKROOTS_PLATFORM_UNSUPPORTED
    PFN_vkGetSwapchainGrallocUsage2ANDROID GetSwapchainGrallocUsage2ANDROID;
#endif
#ifdef VKROOTS_PLATFORM_UNSUPPORTED
    PFN_vkGetSwapchainGrallocUsageANDROID GetSwapchainGrallocUsageANDROID;
#endif
    PFN_vkGetSwapchainImagesKHR GetSwapchainImagesKHR;
    PFN_vkGetSwapchainStatusKHR GetSwapchainStatusKHR;
    PFN_vkGetValidationCacheDataEXT GetValidationCacheDataEXT;
#ifdef VK_ENABLE_BETA_EXTENSIONS
    PFN_vkGetVideoSessionMemoryRequirementsKHR GetVideoSessionMemoryRequirementsKHR;
#endif
    PFN_vkImportFenceFdKHR ImportFenceFdKHR;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkImportFenceWin32HandleKHR ImportFenceWin32HandleKHR;
#endif
    PFN_vkImportSemaphoreFdKHR ImportSemaphoreFdKHR;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkImportSemaphoreWin32HandleKHR ImportSemaphoreWin32HandleKHR;
#endif
#ifdef VK_USE_PLATFORM_FUCHSIA
    PFN_vkImportSemaphoreZirconHandleFUCHSIA ImportSemaphoreZirconHandleFUCHSIA;
#endif
    PFN_vkInitializePerformanceApiINTEL InitializePerformanceApiINTEL;
    PFN_vkInvalidateMappedMemoryRanges InvalidateMappedMemoryRanges;
    PFN_vkMapMemory MapMemory;
    PFN_vkMergePipelineCaches MergePipelineCaches;
    PFN_vkMergeValidationCachesEXT MergeValidationCachesEXT;
    PFN_vkQueueBeginDebugUtilsLabelEXT QueueBeginDebugUtilsLabelEXT;
    PFN_vkQueueBindSparse QueueBindSparse;
    PFN_vkQueueEndDebugUtilsLabelEXT QueueEndDebugUtilsLabelEXT;
    PFN_vkQueueInsertDebugUtilsLabelEXT QueueInsertDebugUtilsLabelEXT;
    PFN_vkQueuePresentKHR QueuePresentKHR;
    PFN_vkQueueSetPerformanceConfigurationINTEL QueueSetPerformanceConfigurationINTEL;
#ifdef VKROOTS_PLATFORM_UNSUPPORTED
    PFN_vkQueueSignalReleaseImageANDROID QueueSignalReleaseImageANDROID;
#endif
    PFN_vkQueueSubmit QueueSubmit;
    PFN_vkQueueSubmit2 QueueSubmit2;
    PFN_vkQueueSubmit2KHR QueueSubmit2KHR;
    PFN_vkQueueWaitIdle QueueWaitIdle;
    PFN_vkRegisterDeviceEventEXT RegisterDeviceEventEXT;
    PFN_vkRegisterDisplayEventEXT RegisterDisplayEventEXT;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkReleaseFullScreenExclusiveModeEXT ReleaseFullScreenExclusiveModeEXT;
#endif
    PFN_vkReleasePerformanceConfigurationINTEL ReleasePerformanceConfigurationINTEL;
    PFN_vkReleaseProfilingLockKHR ReleaseProfilingLockKHR;
    PFN_vkResetCommandBuffer ResetCommandBuffer;
    PFN_vkResetCommandPool ResetCommandPool;
    PFN_vkResetDescriptorPool ResetDescriptorPool;
    PFN_vkResetEvent ResetEvent;
    PFN_vkResetFences ResetFences;
    PFN_vkResetQueryPool ResetQueryPool;
    PFN_vkResetQueryPoolEXT ResetQueryPoolEXT;
#ifdef VK_USE_PLATFORM_FUCHSIA
    PFN_vkSetBufferCollectionBufferConstraintsFUCHSIA SetBufferCollectionBufferConstraintsFUCHSIA;
#endif
#ifdef VK_USE_PLATFORM_FUCHSIA
    PFN_vkSetBufferCollectionImageConstraintsFUCHSIA SetBufferCollectionImageConstraintsFUCHSIA;
#endif
    PFN_vkSetDebugUtilsObjectNameEXT SetDebugUtilsObjectNameEXT;
    PFN_vkSetDebugUtilsObjectTagEXT SetDebugUtilsObjectTagEXT;
    PFN_vkSetDeviceMemoryPriorityEXT SetDeviceMemoryPriorityEXT;
    PFN_vkSetEvent SetEvent;
    PFN_vkSetHdrMetadataEXT SetHdrMetadataEXT;
    PFN_vkSetLocalDimmingAMD SetLocalDimmingAMD;
    PFN_vkSetPrivateData SetPrivateData;
    PFN_vkSetPrivateDataEXT SetPrivateDataEXT;
    PFN_vkSignalSemaphore SignalSemaphore;
    PFN_vkSignalSemaphoreKHR SignalSemaphoreKHR;
    PFN_vkTrimCommandPool TrimCommandPool;
    PFN_vkTrimCommandPoolKHR TrimCommandPoolKHR;
    PFN_vkUninitializePerformanceApiINTEL UninitializePerformanceApiINTEL;
    PFN_vkUnmapMemory UnmapMemory;
    PFN_vkUpdateDescriptorSetWithTemplate UpdateDescriptorSetWithTemplate;
    PFN_vkUpdateDescriptorSetWithTemplateKHR UpdateDescriptorSetWithTemplateKHR;
    PFN_vkUpdateDescriptorSets UpdateDescriptorSets;
#ifdef VK_ENABLE_BETA_EXTENSIONS
    PFN_vkUpdateVideoSessionParametersKHR UpdateVideoSessionParametersKHR;
#endif
    PFN_vkWaitForFences WaitForFences;
    PFN_vkWaitForPresentKHR WaitForPresentKHR;
    PFN_vkWaitSemaphores WaitSemaphores;
    PFN_vkWaitSemaphoresKHR WaitSemaphoresKHR;
    PFN_vkWriteAccelerationStructuresPropertiesKHR WriteAccelerationStructuresPropertiesKHR;
  private:
    PFN_vkDestroyDevice DestroyDeviceReal;
    static void DestroyDeviceWrapper(VkDevice object, const VkAllocationCallbacks* pAllocator) {
      auto dispatch = vkroots::tables::LookupDeviceDispatch(object);
      auto destroyFunc = dispatch->DestroyDeviceReal;
      vkroots::tables::DestroyDispatchTable(object);
      destroyFunc(object, pAllocator);
    }
  };

#ifdef VK_USE_PLATFORM_ANDROID_KHR
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateAndroidSurfaceKHR(VkInstance instance, const VkAndroidSurfaceCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(instance);
    VkResult ret = InstanceOverrides::CreateAndroidSurfaceKHR(dispatch, instance, pCreateInfo, pAllocator, pSurface);
    return ret;
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugReportCallbackEXT *pCallback) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(instance);
    VkResult ret = InstanceOverrides::CreateDebugReportCallbackEXT(dispatch, instance, pCreateInfo, pAllocator, pCallback);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pMessenger) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(instance);
    VkResult ret = InstanceOverrides::CreateDebugUtilsMessengerEXT(dispatch, instance, pCreateInfo, pAllocator, pMessenger);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDevice *pDevice) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    PFN_vkGetDeviceProcAddr deviceProcAddr;
    VkResult procAddrRes = GetProcAddrs(pCreateInfo, &deviceProcAddr);
    if (procAddrRes != VK_SUCCESS)
      return procAddrRes;
    VkResult ret = InstanceOverrides::CreateDevice(dispatch, physicalDevice, pCreateInfo, pAllocator, pDevice);
    if (ret == VK_SUCCESS)
      tables::CreateDispatchTable(pCreateInfo, deviceProcAddr, physicalDevice, *pDevice);
    return ret;
  }

#ifdef VK_USE_PLATFORM_DIRECTFB_EXT
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateDirectFBSurfaceEXT(VkInstance instance, const VkDirectFBSurfaceCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(instance);
    VkResult ret = InstanceOverrides::CreateDirectFBSurfaceEXT(dispatch, instance, pCreateInfo, pAllocator, pSurface);
    return ret;
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateDisplayModeKHR(VkPhysicalDevice physicalDevice, VkDisplayKHR display, const VkDisplayModeCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDisplayModeKHR *pMode) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    VkResult ret = InstanceOverrides::CreateDisplayModeKHR(dispatch, physicalDevice, display, pCreateInfo, pAllocator, pMode);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateDisplayPlaneSurfaceKHR(VkInstance instance, const VkDisplaySurfaceCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(instance);
    VkResult ret = InstanceOverrides::CreateDisplayPlaneSurfaceKHR(dispatch, instance, pCreateInfo, pAllocator, pSurface);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateHeadlessSurfaceEXT(VkInstance instance, const VkHeadlessSurfaceCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(instance);
    VkResult ret = InstanceOverrides::CreateHeadlessSurfaceEXT(dispatch, instance, pCreateInfo, pAllocator, pSurface);
    return ret;
  }

#ifdef VK_USE_PLATFORM_IOS_MVK
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateIOSSurfaceMVK(VkInstance instance, const VkIOSSurfaceCreateInfoMVK *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(instance);
    VkResult ret = InstanceOverrides::CreateIOSSurfaceMVK(dispatch, instance, pCreateInfo, pAllocator, pSurface);
    return ret;
  }

#endif
#ifdef VK_USE_PLATFORM_FUCHSIA
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateImagePipeSurfaceFUCHSIA(VkInstance instance, const VkImagePipeSurfaceCreateInfoFUCHSIA *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(instance);
    VkResult ret = InstanceOverrides::CreateImagePipeSurfaceFUCHSIA(dispatch, instance, pCreateInfo, pAllocator, pSurface);
    return ret;
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateInstance(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkInstance *pInstance) {
    VkInstanceProcAddrFuncs instanceProcAddrFuncs;
    VkResult procAddrRes = GetProcAddrs(pCreateInfo, &instanceProcAddrFuncs);
    if (procAddrRes != VK_SUCCESS)
      return procAddrRes;
    PFN_vkCreateInstance createInstanceProc = (PFN_vkCreateInstance) instanceProcAddrFuncs.NextGetInstanceProcAddr(NULL, "vkCreateInstance");
    VkResult ret = InstanceOverrides::CreateInstance(createInstanceProc, pCreateInfo, pAllocator, pInstance);
    if (ret == VK_SUCCESS)
      tables::CreateDispatchTable(instanceProcAddrFuncs.NextGetInstanceProcAddr, instanceProcAddrFuncs.NextGetPhysicalDeviceProcAddr, *pInstance);
    return ret;
  }

#ifdef VK_USE_PLATFORM_MACOS_MVK
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateMacOSSurfaceMVK(VkInstance instance, const VkMacOSSurfaceCreateInfoMVK *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(instance);
    VkResult ret = InstanceOverrides::CreateMacOSSurfaceMVK(dispatch, instance, pCreateInfo, pAllocator, pSurface);
    return ret;
  }

#endif
#ifdef VK_USE_PLATFORM_METAL_EXT
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateMetalSurfaceEXT(VkInstance instance, const VkMetalSurfaceCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(instance);
    VkResult ret = InstanceOverrides::CreateMetalSurfaceEXT(dispatch, instance, pCreateInfo, pAllocator, pSurface);
    return ret;
  }

#endif
#ifdef VK_USE_PLATFORM_SCREEN_QNX
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateScreenSurfaceQNX(VkInstance instance, const VkScreenSurfaceCreateInfoQNX *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(instance);
    VkResult ret = InstanceOverrides::CreateScreenSurfaceQNX(dispatch, instance, pCreateInfo, pAllocator, pSurface);
    return ret;
  }

#endif
#ifdef VK_USE_PLATFORM_GGP
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateStreamDescriptorSurfaceGGP(VkInstance instance, const VkStreamDescriptorSurfaceCreateInfoGGP *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(instance);
    VkResult ret = InstanceOverrides::CreateStreamDescriptorSurfaceGGP(dispatch, instance, pCreateInfo, pAllocator, pSurface);
    return ret;
  }

#endif
#ifdef VK_USE_PLATFORM_VI_NN
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateViSurfaceNN(VkInstance instance, const VkViSurfaceCreateInfoNN *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(instance);
    VkResult ret = InstanceOverrides::CreateViSurfaceNN(dispatch, instance, pCreateInfo, pAllocator, pSurface);
    return ret;
  }

#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateWaylandSurfaceKHR(VkInstance instance, const VkWaylandSurfaceCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(instance);
    VkResult ret = InstanceOverrides::CreateWaylandSurfaceKHR(dispatch, instance, pCreateInfo, pAllocator, pSurface);
    return ret;
  }

#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateWin32SurfaceKHR(VkInstance instance, const VkWin32SurfaceCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(instance);
    VkResult ret = InstanceOverrides::CreateWin32SurfaceKHR(dispatch, instance, pCreateInfo, pAllocator, pSurface);
    return ret;
  }

#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateXcbSurfaceKHR(VkInstance instance, const VkXcbSurfaceCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(instance);
    VkResult ret = InstanceOverrides::CreateXcbSurfaceKHR(dispatch, instance, pCreateInfo, pAllocator, pSurface);
    return ret;
  }

#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateXlibSurfaceKHR(VkInstance instance, const VkXlibSurfaceCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(instance);
    VkResult ret = InstanceOverrides::CreateXlibSurfaceKHR(dispatch, instance, pCreateInfo, pAllocator, pSurface);
    return ret;
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DebugReportMessageEXT(VkInstance instance, VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char *pLayerPrefix, const char *pMessage) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(instance);
    InstanceOverrides::DebugReportMessageEXT(dispatch, instance, flags, objectType, object, location, messageCode, pLayerPrefix, pMessage);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks *pAllocator) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(instance);
    InstanceOverrides::DestroyDebugReportCallbackEXT(dispatch, instance, callback, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks *pAllocator) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(instance);
    InstanceOverrides::DestroyDebugUtilsMessengerEXT(dispatch, instance, messenger, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(instance);
    InstanceOverrides::DestroyInstance(dispatch, instance, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks *pAllocator) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(instance);
    InstanceOverrides::DestroySurfaceKHR(dispatch, instance, surface, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_EnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char *pLayerName, uint32_t *pPropertyCount, VkExtensionProperties *pProperties) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    VkResult ret = InstanceOverrides::EnumerateDeviceExtensionProperties(dispatch, physicalDevice, pLayerName, pPropertyCount, pProperties);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_EnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount, VkLayerProperties *pProperties) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    VkResult ret = InstanceOverrides::EnumerateDeviceLayerProperties(dispatch, physicalDevice, pPropertyCount, pProperties);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_EnumeratePhysicalDeviceGroups(VkInstance instance, uint32_t *pPhysicalDeviceGroupCount, VkPhysicalDeviceGroupProperties *pPhysicalDeviceGroupProperties) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(instance);
    VkResult ret = InstanceOverrides::EnumeratePhysicalDeviceGroups(dispatch, instance, pPhysicalDeviceGroupCount, pPhysicalDeviceGroupProperties);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_EnumeratePhysicalDeviceGroupsKHR(VkInstance instance, uint32_t *pPhysicalDeviceGroupCount, VkPhysicalDeviceGroupProperties *pPhysicalDeviceGroupProperties) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(instance);
    VkResult ret = InstanceOverrides::EnumeratePhysicalDeviceGroupsKHR(dispatch, instance, pPhysicalDeviceGroupCount, pPhysicalDeviceGroupProperties);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_EnumeratePhysicalDevices(VkInstance instance, uint32_t *pPhysicalDeviceCount, VkPhysicalDevice *pPhysicalDevices) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(instance);
    VkResult ret = InstanceOverrides::EnumeratePhysicalDevices(dispatch, instance, pPhysicalDeviceCount, pPhysicalDevices);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetDisplayModePropertiesKHR(VkPhysicalDevice physicalDevice, VkDisplayKHR display, uint32_t *pPropertyCount, VkDisplayModePropertiesKHR *pProperties) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    VkResult ret = InstanceOverrides::GetDisplayModePropertiesKHR(dispatch, physicalDevice, display, pPropertyCount, pProperties);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetDisplayPlaneCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkDisplayModeKHR mode, uint32_t planeIndex, VkDisplayPlaneCapabilitiesKHR *pCapabilities) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    VkResult ret = InstanceOverrides::GetDisplayPlaneCapabilitiesKHR(dispatch, physicalDevice, mode, planeIndex, pCapabilities);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetDisplayPlaneSupportedDisplaysKHR(VkPhysicalDevice physicalDevice, uint32_t planeIndex, uint32_t *pDisplayCount, VkDisplayKHR *pDisplays) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    VkResult ret = InstanceOverrides::GetDisplayPlaneSupportedDisplaysKHR(dispatch, physicalDevice, planeIndex, pDisplayCount, pDisplays);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static PFN_vkVoidFunction wrap_GetInstanceProcAddr(VkInstance instance, const char *pName) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(instance);
    PFN_vkVoidFunction ret = InstanceOverrides::GetInstanceProcAddr(dispatch, instance, pName);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetPhysicalDeviceDisplayPlanePropertiesKHR(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount, VkDisplayPlanePropertiesKHR *pProperties) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    VkResult ret = InstanceOverrides::GetPhysicalDeviceDisplayPlanePropertiesKHR(dispatch, physicalDevice, pPropertyCount, pProperties);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetPhysicalDeviceDisplayPropertiesKHR(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount, VkDisplayPropertiesKHR *pProperties) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    VkResult ret = InstanceOverrides::GetPhysicalDeviceDisplayPropertiesKHR(dispatch, physicalDevice, pPropertyCount, pProperties);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetPhysicalDeviceExternalBufferProperties(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalBufferInfo *pExternalBufferInfo, VkExternalBufferProperties *pExternalBufferProperties) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    InstanceOverrides::GetPhysicalDeviceExternalBufferProperties(dispatch, physicalDevice, pExternalBufferInfo, pExternalBufferProperties);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetPhysicalDeviceExternalFenceProperties(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalFenceInfo *pExternalFenceInfo, VkExternalFenceProperties *pExternalFenceProperties) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    InstanceOverrides::GetPhysicalDeviceExternalFenceProperties(dispatch, physicalDevice, pExternalFenceInfo, pExternalFenceProperties);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetPhysicalDeviceExternalSemaphoreProperties(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalSemaphoreInfo *pExternalSemaphoreInfo, VkExternalSemaphoreProperties *pExternalSemaphoreProperties) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    InstanceOverrides::GetPhysicalDeviceExternalSemaphoreProperties(dispatch, physicalDevice, pExternalSemaphoreInfo, pExternalSemaphoreProperties);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures *pFeatures) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    InstanceOverrides::GetPhysicalDeviceFeatures(dispatch, physicalDevice, pFeatures);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetPhysicalDeviceFeatures2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2 *pFeatures) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    InstanceOverrides::GetPhysicalDeviceFeatures2(dispatch, physicalDevice, pFeatures);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties *pFormatProperties) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    InstanceOverrides::GetPhysicalDeviceFormatProperties(dispatch, physicalDevice, format, pFormatProperties);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetPhysicalDeviceFormatProperties2(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties2 *pFormatProperties) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    InstanceOverrides::GetPhysicalDeviceFormatProperties2(dispatch, physicalDevice, format, pFormatProperties);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkImageFormatProperties *pImageFormatProperties) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    VkResult ret = InstanceOverrides::GetPhysicalDeviceImageFormatProperties(dispatch, physicalDevice, format, type, tiling, usage, flags, pImageFormatProperties);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetPhysicalDeviceImageFormatProperties2(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo, VkImageFormatProperties2 *pImageFormatProperties) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    VkResult ret = InstanceOverrides::GetPhysicalDeviceImageFormatProperties2(dispatch, physicalDevice, pImageFormatInfo, pImageFormatProperties);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties *pMemoryProperties) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    InstanceOverrides::GetPhysicalDeviceMemoryProperties(dispatch, physicalDevice, pMemoryProperties);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetPhysicalDeviceMemoryProperties2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties2 *pMemoryProperties) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    InstanceOverrides::GetPhysicalDeviceMemoryProperties2(dispatch, physicalDevice, pMemoryProperties);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetPhysicalDevicePresentRectanglesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t *pRectCount, VkRect2D *pRects) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    VkResult ret = InstanceOverrides::GetPhysicalDevicePresentRectanglesKHR(dispatch, physicalDevice, surface, pRectCount, pRects);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties *pProperties) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    InstanceOverrides::GetPhysicalDeviceProperties(dispatch, physicalDevice, pProperties);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetPhysicalDeviceProperties2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties2 *pProperties) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    InstanceOverrides::GetPhysicalDeviceProperties2(dispatch, physicalDevice, pProperties);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount, VkQueueFamilyProperties *pQueueFamilyProperties) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    InstanceOverrides::GetPhysicalDeviceQueueFamilyProperties(dispatch, physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetPhysicalDeviceQueueFamilyProperties2(VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount, VkQueueFamilyProperties2 *pQueueFamilyProperties) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    InstanceOverrides::GetPhysicalDeviceQueueFamilyProperties2(dispatch, physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetPhysicalDeviceSparseImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkImageTiling tiling, uint32_t *pPropertyCount, VkSparseImageFormatProperties *pProperties) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    InstanceOverrides::GetPhysicalDeviceSparseImageFormatProperties(dispatch, physicalDevice, format, type, samples, usage, tiling, pPropertyCount, pProperties);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetPhysicalDeviceSparseImageFormatProperties2(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSparseImageFormatInfo2 *pFormatInfo, uint32_t *pPropertyCount, VkSparseImageFormatProperties2 *pProperties) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    InstanceOverrides::GetPhysicalDeviceSparseImageFormatProperties2(dispatch, physicalDevice, pFormatInfo, pPropertyCount, pProperties);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetPhysicalDeviceSurfaceCapabilities2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo, VkSurfaceCapabilities2KHR *pSurfaceCapabilities) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    VkResult ret = InstanceOverrides::GetPhysicalDeviceSurfaceCapabilities2KHR(dispatch, physicalDevice, pSurfaceInfo, pSurfaceCapabilities);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR *pSurfaceCapabilities) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    VkResult ret = InstanceOverrides::GetPhysicalDeviceSurfaceCapabilitiesKHR(dispatch, physicalDevice, surface, pSurfaceCapabilities);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetPhysicalDeviceSurfaceFormats2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo, uint32_t *pSurfaceFormatCount, VkSurfaceFormat2KHR *pSurfaceFormats) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    VkResult ret = InstanceOverrides::GetPhysicalDeviceSurfaceFormats2KHR(dispatch, physicalDevice, pSurfaceInfo, pSurfaceFormatCount, pSurfaceFormats);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t *pSurfaceFormatCount, VkSurfaceFormatKHR *pSurfaceFormats) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    VkResult ret = InstanceOverrides::GetPhysicalDeviceSurfaceFormatsKHR(dispatch, physicalDevice, surface, pSurfaceFormatCount, pSurfaceFormats);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t *pPresentModeCount, VkPresentModeKHR *pPresentModes) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    VkResult ret = InstanceOverrides::GetPhysicalDeviceSurfacePresentModesKHR(dispatch, physicalDevice, surface, pPresentModeCount, pPresentModes);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkBool32 *pSupported) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    VkResult ret = InstanceOverrides::GetPhysicalDeviceSurfaceSupportKHR(dispatch, physicalDevice, queueFamilyIndex, surface, pSupported);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetPhysicalDeviceToolProperties(VkPhysicalDevice physicalDevice, uint32_t *pToolCount, VkPhysicalDeviceToolProperties *pToolProperties) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    VkResult ret = InstanceOverrides::GetPhysicalDeviceToolProperties(dispatch, physicalDevice, pToolCount, pToolProperties);
    return ret;
  }

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkBool32 wrap_GetPhysicalDeviceWaylandPresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, struct wl_display *display) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    VkBool32 ret = InstanceOverrides::GetPhysicalDeviceWaylandPresentationSupportKHR(dispatch, physicalDevice, queueFamilyIndex, display);
    return ret;
  }

#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkBool32 wrap_GetPhysicalDeviceWin32PresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    VkBool32 ret = InstanceOverrides::GetPhysicalDeviceWin32PresentationSupportKHR(dispatch, physicalDevice, queueFamilyIndex);
    return ret;
  }

#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkBool32 wrap_GetPhysicalDeviceXcbPresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, xcb_connection_t *connection, xcb_visualid_t visual_id) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    VkBool32 ret = InstanceOverrides::GetPhysicalDeviceXcbPresentationSupportKHR(dispatch, physicalDevice, queueFamilyIndex, connection, visual_id);
    return ret;
  }

#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkBool32 wrap_GetPhysicalDeviceXlibPresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, Display *dpy, VisualID visualID) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    VkBool32 ret = InstanceOverrides::GetPhysicalDeviceXlibPresentationSupportKHR(dispatch, physicalDevice, queueFamilyIndex, dpy, visualID);
    return ret;
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_SubmitDebugUtilsMessageEXT(VkInstance instance, VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(instance);
    InstanceOverrides::SubmitDebugUtilsMessageEXT(dispatch, instance, messageSeverity, messageTypes, pCallbackData);
  }


  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult implicit_wrap_CreateInstance(
    const VkInstanceCreateInfo*  pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
          VkInstance*            pInstance) {
    VkInstanceProcAddrFuncs instanceProcAddrFuncs;
    VkResult procAddrRes = GetProcAddrs(pCreateInfo, &instanceProcAddrFuncs);
    if (procAddrRes != VK_SUCCESS)
      return procAddrRes;
    PFN_vkCreateInstance createInstanceProc = (PFN_vkCreateInstance) instanceProcAddrFuncs.NextGetInstanceProcAddr(nullptr, "vkCreateInstance");
    VkResult ret = createInstanceProc(pCreateInfo, pAllocator, pInstance);
    if (ret == VK_SUCCESS)
      tables::CreateDispatchTable(instanceProcAddrFuncs.NextGetInstanceProcAddr, instanceProcAddrFuncs.NextGetPhysicalDeviceProcAddr, *pInstance);
    return ret;
  }


  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void implicit_wrap_DestroyInstance(
          VkInstance             instance,
    const VkAllocationCallbacks* pAllocator) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(instance);
    // Implemented in the Dispatch class, goes to DestroyInstanceWrapper.
    // Make sure we call ours here.
    dispatch->DestroyInstance(instance, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_AcquireDrmDisplayEXT(VkPhysicalDevice physicalDevice, int32_t drmFd, VkDisplayKHR display) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    VkResult ret = PhysicalDeviceOverrides::AcquireDrmDisplayEXT(dispatch, physicalDevice, drmFd, display);
    return ret;
  }

#ifdef VK_USE_PLATFORM_WIN32_KHR
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_AcquireWinrtDisplayNV(VkPhysicalDevice physicalDevice, VkDisplayKHR display) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    VkResult ret = PhysicalDeviceOverrides::AcquireWinrtDisplayNV(dispatch, physicalDevice, display);
    return ret;
  }

#endif
#ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_AcquireXlibDisplayEXT(VkPhysicalDevice physicalDevice, Display *dpy, VkDisplayKHR display) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    VkResult ret = PhysicalDeviceOverrides::AcquireXlibDisplayEXT(dispatch, physicalDevice, dpy, display);
    return ret;
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_EnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, uint32_t *pCounterCount, VkPerformanceCounterKHR *pCounters, VkPerformanceCounterDescriptionKHR *pCounterDescriptions) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    VkResult ret = PhysicalDeviceOverrides::EnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR(dispatch, physicalDevice, queueFamilyIndex, pCounterCount, pCounters, pCounterDescriptions);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetDisplayModeProperties2KHR(VkPhysicalDevice physicalDevice, VkDisplayKHR display, uint32_t *pPropertyCount, VkDisplayModeProperties2KHR *pProperties) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    VkResult ret = PhysicalDeviceOverrides::GetDisplayModeProperties2KHR(dispatch, physicalDevice, display, pPropertyCount, pProperties);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetDisplayPlaneCapabilities2KHR(VkPhysicalDevice physicalDevice, const VkDisplayPlaneInfo2KHR *pDisplayPlaneInfo, VkDisplayPlaneCapabilities2KHR *pCapabilities) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    VkResult ret = PhysicalDeviceOverrides::GetDisplayPlaneCapabilities2KHR(dispatch, physicalDevice, pDisplayPlaneInfo, pCapabilities);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetDrmDisplayEXT(VkPhysicalDevice physicalDevice, int32_t drmFd, uint32_t connectorId, VkDisplayKHR *display) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    VkResult ret = PhysicalDeviceOverrides::GetDrmDisplayEXT(dispatch, physicalDevice, drmFd, connectorId, display);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetPhysicalDeviceCalibrateableTimeDomainsEXT(VkPhysicalDevice physicalDevice, uint32_t *pTimeDomainCount, VkTimeDomainEXT *pTimeDomains) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    VkResult ret = PhysicalDeviceOverrides::GetPhysicalDeviceCalibrateableTimeDomainsEXT(dispatch, physicalDevice, pTimeDomainCount, pTimeDomains);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetPhysicalDeviceCooperativeMatrixPropertiesNV(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount, VkCooperativeMatrixPropertiesNV *pProperties) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    VkResult ret = PhysicalDeviceOverrides::GetPhysicalDeviceCooperativeMatrixPropertiesNV(dispatch, physicalDevice, pPropertyCount, pProperties);
    return ret;
  }

#ifdef VK_USE_PLATFORM_DIRECTFB_EXT
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkBool32 wrap_GetPhysicalDeviceDirectFBPresentationSupportEXT(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, IDirectFB *dfb) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    VkBool32 ret = PhysicalDeviceOverrides::GetPhysicalDeviceDirectFBPresentationSupportEXT(dispatch, physicalDevice, queueFamilyIndex, dfb);
    return ret;
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetPhysicalDeviceDisplayPlaneProperties2KHR(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount, VkDisplayPlaneProperties2KHR *pProperties) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    VkResult ret = PhysicalDeviceOverrides::GetPhysicalDeviceDisplayPlaneProperties2KHR(dispatch, physicalDevice, pPropertyCount, pProperties);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetPhysicalDeviceDisplayProperties2KHR(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount, VkDisplayProperties2KHR *pProperties) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    VkResult ret = PhysicalDeviceOverrides::GetPhysicalDeviceDisplayProperties2KHR(dispatch, physicalDevice, pPropertyCount, pProperties);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetPhysicalDeviceExternalBufferPropertiesKHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalBufferInfo *pExternalBufferInfo, VkExternalBufferProperties *pExternalBufferProperties) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    PhysicalDeviceOverrides::GetPhysicalDeviceExternalBufferPropertiesKHR(dispatch, physicalDevice, pExternalBufferInfo, pExternalBufferProperties);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetPhysicalDeviceExternalFencePropertiesKHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalFenceInfo *pExternalFenceInfo, VkExternalFenceProperties *pExternalFenceProperties) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    PhysicalDeviceOverrides::GetPhysicalDeviceExternalFencePropertiesKHR(dispatch, physicalDevice, pExternalFenceInfo, pExternalFenceProperties);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetPhysicalDeviceExternalImageFormatPropertiesNV(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkExternalMemoryHandleTypeFlagsNV externalHandleType, VkExternalImageFormatPropertiesNV *pExternalImageFormatProperties) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    VkResult ret = PhysicalDeviceOverrides::GetPhysicalDeviceExternalImageFormatPropertiesNV(dispatch, physicalDevice, format, type, tiling, usage, flags, externalHandleType, pExternalImageFormatProperties);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetPhysicalDeviceExternalSemaphorePropertiesKHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalSemaphoreInfo *pExternalSemaphoreInfo, VkExternalSemaphoreProperties *pExternalSemaphoreProperties) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    PhysicalDeviceOverrides::GetPhysicalDeviceExternalSemaphorePropertiesKHR(dispatch, physicalDevice, pExternalSemaphoreInfo, pExternalSemaphoreProperties);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetPhysicalDeviceFeatures2KHR(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2 *pFeatures) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    PhysicalDeviceOverrides::GetPhysicalDeviceFeatures2KHR(dispatch, physicalDevice, pFeatures);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetPhysicalDeviceFormatProperties2KHR(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties2 *pFormatProperties) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    PhysicalDeviceOverrides::GetPhysicalDeviceFormatProperties2KHR(dispatch, physicalDevice, format, pFormatProperties);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetPhysicalDeviceFragmentShadingRatesKHR(VkPhysicalDevice physicalDevice, uint32_t *pFragmentShadingRateCount, VkPhysicalDeviceFragmentShadingRateKHR *pFragmentShadingRates) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    VkResult ret = PhysicalDeviceOverrides::GetPhysicalDeviceFragmentShadingRatesKHR(dispatch, physicalDevice, pFragmentShadingRateCount, pFragmentShadingRates);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetPhysicalDeviceImageFormatProperties2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo, VkImageFormatProperties2 *pImageFormatProperties) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    VkResult ret = PhysicalDeviceOverrides::GetPhysicalDeviceImageFormatProperties2KHR(dispatch, physicalDevice, pImageFormatInfo, pImageFormatProperties);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetPhysicalDeviceMemoryProperties2KHR(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties2 *pMemoryProperties) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    PhysicalDeviceOverrides::GetPhysicalDeviceMemoryProperties2KHR(dispatch, physicalDevice, pMemoryProperties);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetPhysicalDeviceMultisamplePropertiesEXT(VkPhysicalDevice physicalDevice, VkSampleCountFlagBits samples, VkMultisamplePropertiesEXT *pMultisampleProperties) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    PhysicalDeviceOverrides::GetPhysicalDeviceMultisamplePropertiesEXT(dispatch, physicalDevice, samples, pMultisampleProperties);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetPhysicalDeviceProperties2KHR(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties2 *pProperties) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    PhysicalDeviceOverrides::GetPhysicalDeviceProperties2KHR(dispatch, physicalDevice, pProperties);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR(VkPhysicalDevice physicalDevice, const VkQueryPoolPerformanceCreateInfoKHR *pPerformanceQueryCreateInfo, uint32_t *pNumPasses) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    PhysicalDeviceOverrides::GetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR(dispatch, physicalDevice, pPerformanceQueryCreateInfo, pNumPasses);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetPhysicalDeviceQueueFamilyProperties2KHR(VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount, VkQueueFamilyProperties2 *pQueueFamilyProperties) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    PhysicalDeviceOverrides::GetPhysicalDeviceQueueFamilyProperties2KHR(dispatch, physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
  }

#ifdef VK_USE_PLATFORM_SCREEN_QNX
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkBool32 wrap_GetPhysicalDeviceScreenPresentationSupportQNX(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, struct _screen_window *window) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    VkBool32 ret = PhysicalDeviceOverrides::GetPhysicalDeviceScreenPresentationSupportQNX(dispatch, physicalDevice, queueFamilyIndex, window);
    return ret;
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetPhysicalDeviceSparseImageFormatProperties2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSparseImageFormatInfo2 *pFormatInfo, uint32_t *pPropertyCount, VkSparseImageFormatProperties2 *pProperties) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    PhysicalDeviceOverrides::GetPhysicalDeviceSparseImageFormatProperties2KHR(dispatch, physicalDevice, pFormatInfo, pPropertyCount, pProperties);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV(VkPhysicalDevice physicalDevice, uint32_t *pCombinationCount, VkFramebufferMixedSamplesCombinationNV *pCombinations) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    VkResult ret = PhysicalDeviceOverrides::GetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV(dispatch, physicalDevice, pCombinationCount, pCombinations);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetPhysicalDeviceSurfaceCapabilities2EXT(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilities2EXT *pSurfaceCapabilities) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    VkResult ret = PhysicalDeviceOverrides::GetPhysicalDeviceSurfaceCapabilities2EXT(dispatch, physicalDevice, surface, pSurfaceCapabilities);
    return ret;
  }

#ifdef VK_USE_PLATFORM_WIN32_KHR
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetPhysicalDeviceSurfacePresentModes2EXT(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo, uint32_t *pPresentModeCount, VkPresentModeKHR *pPresentModes) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    VkResult ret = PhysicalDeviceOverrides::GetPhysicalDeviceSurfacePresentModes2EXT(dispatch, physicalDevice, pSurfaceInfo, pPresentModeCount, pPresentModes);
    return ret;
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetPhysicalDeviceToolPropertiesEXT(VkPhysicalDevice physicalDevice, uint32_t *pToolCount, VkPhysicalDeviceToolProperties *pToolProperties) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    VkResult ret = PhysicalDeviceOverrides::GetPhysicalDeviceToolPropertiesEXT(dispatch, physicalDevice, pToolCount, pToolProperties);
    return ret;
  }

#ifdef VK_ENABLE_BETA_EXTENSIONS
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetPhysicalDeviceVideoCapabilitiesKHR(VkPhysicalDevice physicalDevice, const VkVideoProfileKHR *pVideoProfile, VkVideoCapabilitiesKHR *pCapabilities) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    VkResult ret = PhysicalDeviceOverrides::GetPhysicalDeviceVideoCapabilitiesKHR(dispatch, physicalDevice, pVideoProfile, pCapabilities);
    return ret;
  }

#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetPhysicalDeviceVideoFormatPropertiesKHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceVideoFormatInfoKHR *pVideoFormatInfo, uint32_t *pVideoFormatPropertyCount, VkVideoFormatPropertiesKHR *pVideoFormatProperties) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    VkResult ret = PhysicalDeviceOverrides::GetPhysicalDeviceVideoFormatPropertiesKHR(dispatch, physicalDevice, pVideoFormatInfo, pVideoFormatPropertyCount, pVideoFormatProperties);
    return ret;
  }

#endif
#ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetRandROutputDisplayEXT(VkPhysicalDevice physicalDevice, Display *dpy, RROutput rrOutput, VkDisplayKHR *pDisplay) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    VkResult ret = PhysicalDeviceOverrides::GetRandROutputDisplayEXT(dispatch, physicalDevice, dpy, rrOutput, pDisplay);
    return ret;
  }

#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetWinrtDisplayNV(VkPhysicalDevice physicalDevice, uint32_t deviceRelativeId, VkDisplayKHR *pDisplay) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    VkResult ret = PhysicalDeviceOverrides::GetWinrtDisplayNV(dispatch, physicalDevice, deviceRelativeId, pDisplay);
    return ret;
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_ReleaseDisplayEXT(VkPhysicalDevice physicalDevice, VkDisplayKHR display) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    VkResult ret = PhysicalDeviceOverrides::ReleaseDisplayEXT(dispatch, physicalDevice, display);
    return ret;
  }


  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult implicit_wrap_CreateDevice(
            VkPhysicalDevice       physicalDevice,
      const VkDeviceCreateInfo*    pCreateInfo,
      const VkAllocationCallbacks* pAllocator,
            VkDevice*              pDevice) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    PFN_vkGetDeviceProcAddr deviceProcAddr;
    VkResult procAddrRes = GetProcAddrs(pCreateInfo, &deviceProcAddr);
    if (procAddrRes != VK_SUCCESS)
      return procAddrRes;
    VkResult ret = dispatch->CreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
    if (ret == VK_SUCCESS)
      tables::CreateDispatchTable(pCreateInfo, deviceProcAddr, physicalDevice, *pDevice);
    return ret;
  }

#ifdef VK_USE_PLATFORM_WIN32_KHR
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_AcquireFullScreenExclusiveModeEXT(VkDevice device, VkSwapchainKHR swapchain) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::AcquireFullScreenExclusiveModeEXT(dispatch, device, swapchain);
    return ret;
  }

#endif
#ifdef VKROOTS_PLATFORM_UNSUPPORTED
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_AcquireImageANDROID(VkDevice device, VkImage image, int nativeFenceFd, VkSemaphore semaphore, VkFence fence) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::AcquireImageANDROID(dispatch, device, image, nativeFenceFd, semaphore, fence);
    return ret;
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_AcquireNextImage2KHR(VkDevice device, const VkAcquireNextImageInfoKHR *pAcquireInfo, uint32_t *pImageIndex) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::AcquireNextImage2KHR(dispatch, device, pAcquireInfo, pImageIndex);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_AcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::AcquireNextImageKHR(dispatch, device, swapchain, timeout, semaphore, fence, pImageIndex);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_AcquirePerformanceConfigurationINTEL(VkDevice device, const VkPerformanceConfigurationAcquireInfoINTEL *pAcquireInfo, VkPerformanceConfigurationINTEL *pConfiguration) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::AcquirePerformanceConfigurationINTEL(dispatch, device, pAcquireInfo, pConfiguration);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_AcquireProfilingLockKHR(VkDevice device, const VkAcquireProfilingLockInfoKHR *pInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::AcquireProfilingLockKHR(dispatch, device, pInfo);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_AllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo *pAllocateInfo, VkCommandBuffer *pCommandBuffers) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::AllocateCommandBuffers(dispatch, device, pAllocateInfo, pCommandBuffers);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_AllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo *pAllocateInfo, VkDescriptorSet *pDescriptorSets) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::AllocateDescriptorSets(dispatch, device, pAllocateInfo, pDescriptorSets);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_AllocateMemory(VkDevice device, const VkMemoryAllocateInfo *pAllocateInfo, const VkAllocationCallbacks *pAllocator, VkDeviceMemory *pMemory) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::AllocateMemory(dispatch, device, pAllocateInfo, pAllocator, pMemory);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_BeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo *pBeginInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    VkResult ret = DeviceOverrides::BeginCommandBuffer(dispatch, commandBuffer, pBeginInfo);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_BindAccelerationStructureMemoryNV(VkDevice device, uint32_t bindInfoCount, const VkBindAccelerationStructureMemoryInfoNV *pBindInfos) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::BindAccelerationStructureMemoryNV(dispatch, device, bindInfoCount, pBindInfos);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_BindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::BindBufferMemory(dispatch, device, buffer, memory, memoryOffset);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_BindBufferMemory2(VkDevice device, uint32_t bindInfoCount, const VkBindBufferMemoryInfo *pBindInfos) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::BindBufferMemory2(dispatch, device, bindInfoCount, pBindInfos);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_BindBufferMemory2KHR(VkDevice device, uint32_t bindInfoCount, const VkBindBufferMemoryInfo *pBindInfos) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::BindBufferMemory2KHR(dispatch, device, bindInfoCount, pBindInfos);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_BindImageMemory(VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::BindImageMemory(dispatch, device, image, memory, memoryOffset);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_BindImageMemory2(VkDevice device, uint32_t bindInfoCount, const VkBindImageMemoryInfo *pBindInfos) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::BindImageMemory2(dispatch, device, bindInfoCount, pBindInfos);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_BindImageMemory2KHR(VkDevice device, uint32_t bindInfoCount, const VkBindImageMemoryInfo *pBindInfos) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::BindImageMemory2KHR(dispatch, device, bindInfoCount, pBindInfos);
    return ret;
  }

#ifdef VK_ENABLE_BETA_EXTENSIONS
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_BindVideoSessionMemoryKHR(VkDevice device, VkVideoSessionKHR videoSession, uint32_t videoSessionBindMemoryCount, const VkVideoBindMemoryKHR *pVideoSessionBindMemories) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::BindVideoSessionMemoryKHR(dispatch, device, videoSession, videoSessionBindMemoryCount, pVideoSessionBindMemories);
    return ret;
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_BuildAccelerationStructuresKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR *pInfos, const VkAccelerationStructureBuildRangeInfoKHR * const*ppBuildRangeInfos) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::BuildAccelerationStructuresKHR(dispatch, device, deferredOperation, infoCount, pInfos, ppBuildRangeInfos);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdBeginConditionalRenderingEXT(VkCommandBuffer commandBuffer, const VkConditionalRenderingBeginInfoEXT *pConditionalRenderingBegin) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdBeginConditionalRenderingEXT(dispatch, commandBuffer, pConditionalRenderingBegin);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdBeginDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT *pLabelInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdBeginDebugUtilsLabelEXT(dispatch, commandBuffer, pLabelInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdBeginQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdBeginQuery(dispatch, commandBuffer, queryPool, query, flags);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdBeginQueryIndexedEXT(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags, uint32_t index) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdBeginQueryIndexedEXT(dispatch, commandBuffer, queryPool, query, flags, index);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin, VkSubpassContents contents) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdBeginRenderPass(dispatch, commandBuffer, pRenderPassBegin, contents);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdBeginRenderPass2(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin, const VkSubpassBeginInfo *pSubpassBeginInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdBeginRenderPass2(dispatch, commandBuffer, pRenderPassBegin, pSubpassBeginInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdBeginRenderPass2KHR(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin, const VkSubpassBeginInfo *pSubpassBeginInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdBeginRenderPass2KHR(dispatch, commandBuffer, pRenderPassBegin, pSubpassBeginInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdBeginRendering(VkCommandBuffer commandBuffer, const VkRenderingInfo *pRenderingInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdBeginRendering(dispatch, commandBuffer, pRenderingInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdBeginRenderingKHR(VkCommandBuffer commandBuffer, const VkRenderingInfo *pRenderingInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdBeginRenderingKHR(dispatch, commandBuffer, pRenderingInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdBeginTransformFeedbackEXT(VkCommandBuffer commandBuffer, uint32_t firstCounterBuffer, uint32_t counterBufferCount, const VkBuffer *pCounterBuffers, const VkDeviceSize *pCounterBufferOffsets) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdBeginTransformFeedbackEXT(dispatch, commandBuffer, firstCounterBuffer, counterBufferCount, pCounterBuffers, pCounterBufferOffsets);
  }

#ifdef VK_ENABLE_BETA_EXTENSIONS
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdBeginVideoCodingKHR(VkCommandBuffer commandBuffer, const VkVideoBeginCodingInfoKHR *pBeginInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdBeginVideoCodingKHR(dispatch, commandBuffer, pBeginInfo);
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdBindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet *pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t *pDynamicOffsets) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdBindDescriptorSets(dispatch, commandBuffer, pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdBindIndexBuffer(dispatch, commandBuffer, buffer, offset, indexType);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdBindInvocationMaskHUAWEI(VkCommandBuffer commandBuffer, VkImageView imageView, VkImageLayout imageLayout) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdBindInvocationMaskHUAWEI(dispatch, commandBuffer, imageView, imageLayout);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdBindPipeline(dispatch, commandBuffer, pipelineBindPoint, pipeline);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdBindPipelineShaderGroupNV(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline, uint32_t groupIndex) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdBindPipelineShaderGroupNV(dispatch, commandBuffer, pipelineBindPoint, pipeline, groupIndex);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdBindShadingRateImageNV(VkCommandBuffer commandBuffer, VkImageView imageView, VkImageLayout imageLayout) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdBindShadingRateImageNV(dispatch, commandBuffer, imageView, imageLayout);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdBindTransformFeedbackBuffersEXT(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer *pBuffers, const VkDeviceSize *pOffsets, const VkDeviceSize *pSizes) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdBindTransformFeedbackBuffersEXT(dispatch, commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets, pSizes);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdBindVertexBuffers(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer *pBuffers, const VkDeviceSize *pOffsets) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdBindVertexBuffers(dispatch, commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdBindVertexBuffers2(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer *pBuffers, const VkDeviceSize *pOffsets, const VkDeviceSize *pSizes, const VkDeviceSize *pStrides) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdBindVertexBuffers2(dispatch, commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets, pSizes, pStrides);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdBindVertexBuffers2EXT(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer *pBuffers, const VkDeviceSize *pOffsets, const VkDeviceSize *pSizes, const VkDeviceSize *pStrides) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdBindVertexBuffers2EXT(dispatch, commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets, pSizes, pStrides);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit *pRegions, VkFilter filter) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdBlitImage(dispatch, commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdBlitImage2(VkCommandBuffer commandBuffer, const VkBlitImageInfo2 *pBlitImageInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdBlitImage2(dispatch, commandBuffer, pBlitImageInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdBlitImage2KHR(VkCommandBuffer commandBuffer, const VkBlitImageInfo2 *pBlitImageInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdBlitImage2KHR(dispatch, commandBuffer, pBlitImageInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdBuildAccelerationStructureNV(VkCommandBuffer commandBuffer, const VkAccelerationStructureInfoNV *pInfo, VkBuffer instanceData, VkDeviceSize instanceOffset, VkBool32 update, VkAccelerationStructureNV dst, VkAccelerationStructureNV src, VkBuffer scratch, VkDeviceSize scratchOffset) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdBuildAccelerationStructureNV(dispatch, commandBuffer, pInfo, instanceData, instanceOffset, update, dst, src, scratch, scratchOffset);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdBuildAccelerationStructuresIndirectKHR(VkCommandBuffer commandBuffer, uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR *pInfos, const VkDeviceAddress *pIndirectDeviceAddresses, const uint32_t *pIndirectStrides, const uint32_t * const*ppMaxPrimitiveCounts) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdBuildAccelerationStructuresIndirectKHR(dispatch, commandBuffer, infoCount, pInfos, pIndirectDeviceAddresses, pIndirectStrides, ppMaxPrimitiveCounts);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdBuildAccelerationStructuresKHR(VkCommandBuffer commandBuffer, uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR *pInfos, const VkAccelerationStructureBuildRangeInfoKHR * const*ppBuildRangeInfos) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdBuildAccelerationStructuresKHR(dispatch, commandBuffer, infoCount, pInfos, ppBuildRangeInfos);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdClearAttachments(VkCommandBuffer commandBuffer, uint32_t attachmentCount, const VkClearAttachment *pAttachments, uint32_t rectCount, const VkClearRect *pRects) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdClearAttachments(dispatch, commandBuffer, attachmentCount, pAttachments, rectCount, pRects);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearColorValue *pColor, uint32_t rangeCount, const VkImageSubresourceRange *pRanges) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdClearColorImage(dispatch, commandBuffer, image, imageLayout, pColor, rangeCount, pRanges);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdClearDepthStencilImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue *pDepthStencil, uint32_t rangeCount, const VkImageSubresourceRange *pRanges) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdClearDepthStencilImage(dispatch, commandBuffer, image, imageLayout, pDepthStencil, rangeCount, pRanges);
  }

#ifdef VK_ENABLE_BETA_EXTENSIONS
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdControlVideoCodingKHR(VkCommandBuffer commandBuffer, const VkVideoCodingControlInfoKHR *pCodingControlInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdControlVideoCodingKHR(dispatch, commandBuffer, pCodingControlInfo);
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdCopyAccelerationStructureKHR(VkCommandBuffer commandBuffer, const VkCopyAccelerationStructureInfoKHR *pInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdCopyAccelerationStructureKHR(dispatch, commandBuffer, pInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdCopyAccelerationStructureNV(VkCommandBuffer commandBuffer, VkAccelerationStructureNV dst, VkAccelerationStructureNV src, VkCopyAccelerationStructureModeKHR mode) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdCopyAccelerationStructureNV(dispatch, commandBuffer, dst, src, mode);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdCopyAccelerationStructureToMemoryKHR(VkCommandBuffer commandBuffer, const VkCopyAccelerationStructureToMemoryInfoKHR *pInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdCopyAccelerationStructureToMemoryKHR(dispatch, commandBuffer, pInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy *pRegions) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdCopyBuffer(dispatch, commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdCopyBuffer2(VkCommandBuffer commandBuffer, const VkCopyBufferInfo2 *pCopyBufferInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdCopyBuffer2(dispatch, commandBuffer, pCopyBufferInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdCopyBuffer2KHR(VkCommandBuffer commandBuffer, const VkCopyBufferInfo2 *pCopyBufferInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdCopyBuffer2KHR(dispatch, commandBuffer, pCopyBufferInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy *pRegions) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdCopyBufferToImage(dispatch, commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdCopyBufferToImage2(VkCommandBuffer commandBuffer, const VkCopyBufferToImageInfo2 *pCopyBufferToImageInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdCopyBufferToImage2(dispatch, commandBuffer, pCopyBufferToImageInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdCopyBufferToImage2KHR(VkCommandBuffer commandBuffer, const VkCopyBufferToImageInfo2 *pCopyBufferToImageInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdCopyBufferToImage2KHR(dispatch, commandBuffer, pCopyBufferToImageInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy *pRegions) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdCopyImage(dispatch, commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdCopyImage2(VkCommandBuffer commandBuffer, const VkCopyImageInfo2 *pCopyImageInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdCopyImage2(dispatch, commandBuffer, pCopyImageInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdCopyImage2KHR(VkCommandBuffer commandBuffer, const VkCopyImageInfo2 *pCopyImageInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdCopyImage2KHR(dispatch, commandBuffer, pCopyImageInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy *pRegions) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdCopyImageToBuffer(dispatch, commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdCopyImageToBuffer2(VkCommandBuffer commandBuffer, const VkCopyImageToBufferInfo2 *pCopyImageToBufferInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdCopyImageToBuffer2(dispatch, commandBuffer, pCopyImageToBufferInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdCopyImageToBuffer2KHR(VkCommandBuffer commandBuffer, const VkCopyImageToBufferInfo2 *pCopyImageToBufferInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdCopyImageToBuffer2KHR(dispatch, commandBuffer, pCopyImageToBufferInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdCopyMemoryToAccelerationStructureKHR(VkCommandBuffer commandBuffer, const VkCopyMemoryToAccelerationStructureInfoKHR *pInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdCopyMemoryToAccelerationStructureKHR(dispatch, commandBuffer, pInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdCopyQueryPoolResults(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride, VkQueryResultFlags flags) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdCopyQueryPoolResults(dispatch, commandBuffer, queryPool, firstQuery, queryCount, dstBuffer, dstOffset, stride, flags);
  }

#ifdef VKROOTS_PLATFORM_UNSUPPORTED
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdCuLaunchKernelNVX(VkCommandBuffer commandBuffer, const VkCuLaunchInfoNVX *pLaunchInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdCuLaunchKernelNVX(dispatch, commandBuffer, pLaunchInfo);
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdDebugMarkerBeginEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT *pMarkerInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdDebugMarkerBeginEXT(dispatch, commandBuffer, pMarkerInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdDebugMarkerEndEXT(VkCommandBuffer commandBuffer) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdDebugMarkerEndEXT(dispatch, commandBuffer);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdDebugMarkerInsertEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT *pMarkerInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdDebugMarkerInsertEXT(dispatch, commandBuffer, pMarkerInfo);
  }

#ifdef VK_ENABLE_BETA_EXTENSIONS
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdDecodeVideoKHR(VkCommandBuffer commandBuffer, const VkVideoDecodeInfoKHR *pFrameInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdDecodeVideoKHR(dispatch, commandBuffer, pFrameInfo);
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdDispatch(VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdDispatch(dispatch, commandBuffer, groupCountX, groupCountY, groupCountZ);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdDispatchBase(VkCommandBuffer commandBuffer, uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdDispatchBase(dispatch, commandBuffer, baseGroupX, baseGroupY, baseGroupZ, groupCountX, groupCountY, groupCountZ);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdDispatchBaseKHR(VkCommandBuffer commandBuffer, uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdDispatchBaseKHR(dispatch, commandBuffer, baseGroupX, baseGroupY, baseGroupZ, groupCountX, groupCountY, groupCountZ);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdDispatchIndirect(dispatch, commandBuffer, buffer, offset);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdDraw(dispatch, commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdDrawIndexed(dispatch, commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdDrawIndexedIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdDrawIndexedIndirect(dispatch, commandBuffer, buffer, offset, drawCount, stride);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdDrawIndexedIndirectCount(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdDrawIndexedIndirectCount(dispatch, commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdDrawIndexedIndirectCountAMD(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdDrawIndexedIndirectCountAMD(dispatch, commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdDrawIndexedIndirectCountKHR(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdDrawIndexedIndirectCountKHR(dispatch, commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdDrawIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdDrawIndirect(dispatch, commandBuffer, buffer, offset, drawCount, stride);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdDrawIndirectByteCountEXT(VkCommandBuffer commandBuffer, uint32_t instanceCount, uint32_t firstInstance, VkBuffer counterBuffer, VkDeviceSize counterBufferOffset, uint32_t counterOffset, uint32_t vertexStride) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdDrawIndirectByteCountEXT(dispatch, commandBuffer, instanceCount, firstInstance, counterBuffer, counterBufferOffset, counterOffset, vertexStride);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdDrawIndirectCount(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdDrawIndirectCount(dispatch, commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdDrawIndirectCountAMD(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdDrawIndirectCountAMD(dispatch, commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdDrawIndirectCountKHR(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdDrawIndirectCountKHR(dispatch, commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdDrawMeshTasksIndirectCountNV(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdDrawMeshTasksIndirectCountNV(dispatch, commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdDrawMeshTasksIndirectNV(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdDrawMeshTasksIndirectNV(dispatch, commandBuffer, buffer, offset, drawCount, stride);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdDrawMeshTasksNV(VkCommandBuffer commandBuffer, uint32_t taskCount, uint32_t firstTask) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdDrawMeshTasksNV(dispatch, commandBuffer, taskCount, firstTask);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdDrawMultiEXT(VkCommandBuffer commandBuffer, uint32_t drawCount, const VkMultiDrawInfoEXT *pVertexInfo, uint32_t instanceCount, uint32_t firstInstance, uint32_t stride) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdDrawMultiEXT(dispatch, commandBuffer, drawCount, pVertexInfo, instanceCount, firstInstance, stride);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdDrawMultiIndexedEXT(VkCommandBuffer commandBuffer, uint32_t drawCount, const VkMultiDrawIndexedInfoEXT *pIndexInfo, uint32_t instanceCount, uint32_t firstInstance, uint32_t stride, const int32_t *pVertexOffset) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdDrawMultiIndexedEXT(dispatch, commandBuffer, drawCount, pIndexInfo, instanceCount, firstInstance, stride, pVertexOffset);
  }

#ifdef VK_ENABLE_BETA_EXTENSIONS
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdEncodeVideoKHR(VkCommandBuffer commandBuffer, const VkVideoEncodeInfoKHR *pEncodeInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdEncodeVideoKHR(dispatch, commandBuffer, pEncodeInfo);
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdEndConditionalRenderingEXT(VkCommandBuffer commandBuffer) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdEndConditionalRenderingEXT(dispatch, commandBuffer);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdEndDebugUtilsLabelEXT(VkCommandBuffer commandBuffer) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdEndDebugUtilsLabelEXT(dispatch, commandBuffer);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdEndQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdEndQuery(dispatch, commandBuffer, queryPool, query);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdEndQueryIndexedEXT(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, uint32_t index) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdEndQueryIndexedEXT(dispatch, commandBuffer, queryPool, query, index);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdEndRenderPass(VkCommandBuffer commandBuffer) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdEndRenderPass(dispatch, commandBuffer);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdEndRenderPass2(VkCommandBuffer commandBuffer, const VkSubpassEndInfo *pSubpassEndInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdEndRenderPass2(dispatch, commandBuffer, pSubpassEndInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdEndRenderPass2KHR(VkCommandBuffer commandBuffer, const VkSubpassEndInfo *pSubpassEndInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdEndRenderPass2KHR(dispatch, commandBuffer, pSubpassEndInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdEndRendering(VkCommandBuffer commandBuffer) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdEndRendering(dispatch, commandBuffer);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdEndRenderingKHR(VkCommandBuffer commandBuffer) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdEndRenderingKHR(dispatch, commandBuffer);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdEndTransformFeedbackEXT(VkCommandBuffer commandBuffer, uint32_t firstCounterBuffer, uint32_t counterBufferCount, const VkBuffer *pCounterBuffers, const VkDeviceSize *pCounterBufferOffsets) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdEndTransformFeedbackEXT(dispatch, commandBuffer, firstCounterBuffer, counterBufferCount, pCounterBuffers, pCounterBufferOffsets);
  }

#ifdef VK_ENABLE_BETA_EXTENSIONS
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdEndVideoCodingKHR(VkCommandBuffer commandBuffer, const VkVideoEndCodingInfoKHR *pEndCodingInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdEndVideoCodingKHR(dispatch, commandBuffer, pEndCodingInfo);
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdExecuteCommands(VkCommandBuffer commandBuffer, uint32_t commandBufferCount, const VkCommandBuffer *pCommandBuffers) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdExecuteCommands(dispatch, commandBuffer, commandBufferCount, pCommandBuffers);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdExecuteGeneratedCommandsNV(VkCommandBuffer commandBuffer, VkBool32 isPreprocessed, const VkGeneratedCommandsInfoNV *pGeneratedCommandsInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdExecuteGeneratedCommandsNV(dispatch, commandBuffer, isPreprocessed, pGeneratedCommandsInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdFillBuffer(dispatch, commandBuffer, dstBuffer, dstOffset, size, data);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdInsertDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT *pLabelInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdInsertDebugUtilsLabelEXT(dispatch, commandBuffer, pLabelInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdNextSubpass(VkCommandBuffer commandBuffer, VkSubpassContents contents) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdNextSubpass(dispatch, commandBuffer, contents);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdNextSubpass2(VkCommandBuffer commandBuffer, const VkSubpassBeginInfo *pSubpassBeginInfo, const VkSubpassEndInfo *pSubpassEndInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdNextSubpass2(dispatch, commandBuffer, pSubpassBeginInfo, pSubpassEndInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdNextSubpass2KHR(VkCommandBuffer commandBuffer, const VkSubpassBeginInfo *pSubpassBeginInfo, const VkSubpassEndInfo *pSubpassEndInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdNextSubpass2KHR(dispatch, commandBuffer, pSubpassBeginInfo, pSubpassEndInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdPipelineBarrier(dispatch, commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdPipelineBarrier2(VkCommandBuffer commandBuffer, const VkDependencyInfo *pDependencyInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdPipelineBarrier2(dispatch, commandBuffer, pDependencyInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdPipelineBarrier2KHR(VkCommandBuffer commandBuffer, const VkDependencyInfo *pDependencyInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdPipelineBarrier2KHR(dispatch, commandBuffer, pDependencyInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdPreprocessGeneratedCommandsNV(VkCommandBuffer commandBuffer, const VkGeneratedCommandsInfoNV *pGeneratedCommandsInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdPreprocessGeneratedCommandsNV(dispatch, commandBuffer, pGeneratedCommandsInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdPushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void *pValues) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdPushConstants(dispatch, commandBuffer, layout, stageFlags, offset, size, pValues);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdPushDescriptorSetKHR(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t set, uint32_t descriptorWriteCount, const VkWriteDescriptorSet *pDescriptorWrites) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdPushDescriptorSetKHR(dispatch, commandBuffer, pipelineBindPoint, layout, set, descriptorWriteCount, pDescriptorWrites);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdPushDescriptorSetWithTemplateKHR(VkCommandBuffer commandBuffer, VkDescriptorUpdateTemplate descriptorUpdateTemplate, VkPipelineLayout layout, uint32_t set, const void *pData) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdPushDescriptorSetWithTemplateKHR(dispatch, commandBuffer, descriptorUpdateTemplate, layout, set, pData);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdResetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdResetEvent(dispatch, commandBuffer, event, stageMask);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdResetEvent2(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags2 stageMask) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdResetEvent2(dispatch, commandBuffer, event, stageMask);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdResetEvent2KHR(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags2 stageMask) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdResetEvent2KHR(dispatch, commandBuffer, event, stageMask);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdResetQueryPool(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdResetQueryPool(dispatch, commandBuffer, queryPool, firstQuery, queryCount);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdResolveImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve *pRegions) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdResolveImage(dispatch, commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdResolveImage2(VkCommandBuffer commandBuffer, const VkResolveImageInfo2 *pResolveImageInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdResolveImage2(dispatch, commandBuffer, pResolveImageInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdResolveImage2KHR(VkCommandBuffer commandBuffer, const VkResolveImageInfo2 *pResolveImageInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdResolveImage2KHR(dispatch, commandBuffer, pResolveImageInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetBlendConstants(VkCommandBuffer commandBuffer, const float blendConstants[4]) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetBlendConstants(dispatch, commandBuffer, blendConstants);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetCheckpointNV(VkCommandBuffer commandBuffer, const void *pCheckpointMarker) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetCheckpointNV(dispatch, commandBuffer, pCheckpointMarker);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetCoarseSampleOrderNV(VkCommandBuffer commandBuffer, VkCoarseSampleOrderTypeNV sampleOrderType, uint32_t customSampleOrderCount, const VkCoarseSampleOrderCustomNV *pCustomSampleOrders) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetCoarseSampleOrderNV(dispatch, commandBuffer, sampleOrderType, customSampleOrderCount, pCustomSampleOrders);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetColorWriteEnableEXT(VkCommandBuffer commandBuffer, uint32_t attachmentCount, const VkBool32 *pColorWriteEnables) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetColorWriteEnableEXT(dispatch, commandBuffer, attachmentCount, pColorWriteEnables);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetCullMode(VkCommandBuffer commandBuffer, VkCullModeFlags cullMode) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetCullMode(dispatch, commandBuffer, cullMode);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetCullModeEXT(VkCommandBuffer commandBuffer, VkCullModeFlags cullMode) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetCullModeEXT(dispatch, commandBuffer, cullMode);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetDepthBias(VkCommandBuffer commandBuffer, float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetDepthBias(dispatch, commandBuffer, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetDepthBiasEnable(VkCommandBuffer commandBuffer, VkBool32 depthBiasEnable) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetDepthBiasEnable(dispatch, commandBuffer, depthBiasEnable);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetDepthBiasEnableEXT(VkCommandBuffer commandBuffer, VkBool32 depthBiasEnable) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetDepthBiasEnableEXT(dispatch, commandBuffer, depthBiasEnable);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetDepthBounds(VkCommandBuffer commandBuffer, float minDepthBounds, float maxDepthBounds) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetDepthBounds(dispatch, commandBuffer, minDepthBounds, maxDepthBounds);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetDepthBoundsTestEnable(VkCommandBuffer commandBuffer, VkBool32 depthBoundsTestEnable) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetDepthBoundsTestEnable(dispatch, commandBuffer, depthBoundsTestEnable);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetDepthBoundsTestEnableEXT(VkCommandBuffer commandBuffer, VkBool32 depthBoundsTestEnable) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetDepthBoundsTestEnableEXT(dispatch, commandBuffer, depthBoundsTestEnable);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetDepthCompareOp(VkCommandBuffer commandBuffer, VkCompareOp depthCompareOp) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetDepthCompareOp(dispatch, commandBuffer, depthCompareOp);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetDepthCompareOpEXT(VkCommandBuffer commandBuffer, VkCompareOp depthCompareOp) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetDepthCompareOpEXT(dispatch, commandBuffer, depthCompareOp);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetDepthTestEnable(VkCommandBuffer commandBuffer, VkBool32 depthTestEnable) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetDepthTestEnable(dispatch, commandBuffer, depthTestEnable);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetDepthTestEnableEXT(VkCommandBuffer commandBuffer, VkBool32 depthTestEnable) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetDepthTestEnableEXT(dispatch, commandBuffer, depthTestEnable);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetDepthWriteEnable(VkCommandBuffer commandBuffer, VkBool32 depthWriteEnable) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetDepthWriteEnable(dispatch, commandBuffer, depthWriteEnable);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetDepthWriteEnableEXT(VkCommandBuffer commandBuffer, VkBool32 depthWriteEnable) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetDepthWriteEnableEXT(dispatch, commandBuffer, depthWriteEnable);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetDeviceMask(VkCommandBuffer commandBuffer, uint32_t deviceMask) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetDeviceMask(dispatch, commandBuffer, deviceMask);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetDeviceMaskKHR(VkCommandBuffer commandBuffer, uint32_t deviceMask) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetDeviceMaskKHR(dispatch, commandBuffer, deviceMask);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetDiscardRectangleEXT(VkCommandBuffer commandBuffer, uint32_t firstDiscardRectangle, uint32_t discardRectangleCount, const VkRect2D *pDiscardRectangles) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetDiscardRectangleEXT(dispatch, commandBuffer, firstDiscardRectangle, discardRectangleCount, pDiscardRectangles);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetEvent(dispatch, commandBuffer, event, stageMask);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetEvent2(VkCommandBuffer commandBuffer, VkEvent event, const VkDependencyInfo *pDependencyInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetEvent2(dispatch, commandBuffer, event, pDependencyInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetEvent2KHR(VkCommandBuffer commandBuffer, VkEvent event, const VkDependencyInfo *pDependencyInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetEvent2KHR(dispatch, commandBuffer, event, pDependencyInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetExclusiveScissorNV(VkCommandBuffer commandBuffer, uint32_t firstExclusiveScissor, uint32_t exclusiveScissorCount, const VkRect2D *pExclusiveScissors) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetExclusiveScissorNV(dispatch, commandBuffer, firstExclusiveScissor, exclusiveScissorCount, pExclusiveScissors);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetFragmentShadingRateEnumNV(VkCommandBuffer commandBuffer, VkFragmentShadingRateNV shadingRate, const VkFragmentShadingRateCombinerOpKHR combinerOps[2]) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetFragmentShadingRateEnumNV(dispatch, commandBuffer, shadingRate, combinerOps);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetFragmentShadingRateKHR(VkCommandBuffer commandBuffer, const VkExtent2D *pFragmentSize, const VkFragmentShadingRateCombinerOpKHR combinerOps[2]) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetFragmentShadingRateKHR(dispatch, commandBuffer, pFragmentSize, combinerOps);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetFrontFace(VkCommandBuffer commandBuffer, VkFrontFace frontFace) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetFrontFace(dispatch, commandBuffer, frontFace);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetFrontFaceEXT(VkCommandBuffer commandBuffer, VkFrontFace frontFace) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetFrontFaceEXT(dispatch, commandBuffer, frontFace);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetLineStippleEXT(VkCommandBuffer commandBuffer, uint32_t lineStippleFactor, uint16_t lineStipplePattern) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetLineStippleEXT(dispatch, commandBuffer, lineStippleFactor, lineStipplePattern);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetLineWidth(VkCommandBuffer commandBuffer, float lineWidth) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetLineWidth(dispatch, commandBuffer, lineWidth);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetLogicOpEXT(VkCommandBuffer commandBuffer, VkLogicOp logicOp) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetLogicOpEXT(dispatch, commandBuffer, logicOp);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetPatchControlPointsEXT(VkCommandBuffer commandBuffer, uint32_t patchControlPoints) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetPatchControlPointsEXT(dispatch, commandBuffer, patchControlPoints);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CmdSetPerformanceMarkerINTEL(VkCommandBuffer commandBuffer, const VkPerformanceMarkerInfoINTEL *pMarkerInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    VkResult ret = DeviceOverrides::CmdSetPerformanceMarkerINTEL(dispatch, commandBuffer, pMarkerInfo);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CmdSetPerformanceOverrideINTEL(VkCommandBuffer commandBuffer, const VkPerformanceOverrideInfoINTEL *pOverrideInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    VkResult ret = DeviceOverrides::CmdSetPerformanceOverrideINTEL(dispatch, commandBuffer, pOverrideInfo);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CmdSetPerformanceStreamMarkerINTEL(VkCommandBuffer commandBuffer, const VkPerformanceStreamMarkerInfoINTEL *pMarkerInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    VkResult ret = DeviceOverrides::CmdSetPerformanceStreamMarkerINTEL(dispatch, commandBuffer, pMarkerInfo);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetPrimitiveRestartEnable(VkCommandBuffer commandBuffer, VkBool32 primitiveRestartEnable) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetPrimitiveRestartEnable(dispatch, commandBuffer, primitiveRestartEnable);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetPrimitiveRestartEnableEXT(VkCommandBuffer commandBuffer, VkBool32 primitiveRestartEnable) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetPrimitiveRestartEnableEXT(dispatch, commandBuffer, primitiveRestartEnable);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetPrimitiveTopology(VkCommandBuffer commandBuffer, VkPrimitiveTopology primitiveTopology) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetPrimitiveTopology(dispatch, commandBuffer, primitiveTopology);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetPrimitiveTopologyEXT(VkCommandBuffer commandBuffer, VkPrimitiveTopology primitiveTopology) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetPrimitiveTopologyEXT(dispatch, commandBuffer, primitiveTopology);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetRasterizerDiscardEnable(VkCommandBuffer commandBuffer, VkBool32 rasterizerDiscardEnable) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetRasterizerDiscardEnable(dispatch, commandBuffer, rasterizerDiscardEnable);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetRasterizerDiscardEnableEXT(VkCommandBuffer commandBuffer, VkBool32 rasterizerDiscardEnable) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetRasterizerDiscardEnableEXT(dispatch, commandBuffer, rasterizerDiscardEnable);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetRayTracingPipelineStackSizeKHR(VkCommandBuffer commandBuffer, uint32_t pipelineStackSize) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetRayTracingPipelineStackSizeKHR(dispatch, commandBuffer, pipelineStackSize);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetSampleLocationsEXT(VkCommandBuffer commandBuffer, const VkSampleLocationsInfoEXT *pSampleLocationsInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetSampleLocationsEXT(dispatch, commandBuffer, pSampleLocationsInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetScissor(VkCommandBuffer commandBuffer, uint32_t firstScissor, uint32_t scissorCount, const VkRect2D *pScissors) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetScissor(dispatch, commandBuffer, firstScissor, scissorCount, pScissors);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetScissorWithCount(VkCommandBuffer commandBuffer, uint32_t scissorCount, const VkRect2D *pScissors) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetScissorWithCount(dispatch, commandBuffer, scissorCount, pScissors);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetScissorWithCountEXT(VkCommandBuffer commandBuffer, uint32_t scissorCount, const VkRect2D *pScissors) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetScissorWithCountEXT(dispatch, commandBuffer, scissorCount, pScissors);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetStencilCompareMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t compareMask) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetStencilCompareMask(dispatch, commandBuffer, faceMask, compareMask);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetStencilOp(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, VkStencilOp failOp, VkStencilOp passOp, VkStencilOp depthFailOp, VkCompareOp compareOp) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetStencilOp(dispatch, commandBuffer, faceMask, failOp, passOp, depthFailOp, compareOp);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetStencilOpEXT(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, VkStencilOp failOp, VkStencilOp passOp, VkStencilOp depthFailOp, VkCompareOp compareOp) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetStencilOpEXT(dispatch, commandBuffer, faceMask, failOp, passOp, depthFailOp, compareOp);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetStencilReference(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t reference) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetStencilReference(dispatch, commandBuffer, faceMask, reference);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetStencilTestEnable(VkCommandBuffer commandBuffer, VkBool32 stencilTestEnable) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetStencilTestEnable(dispatch, commandBuffer, stencilTestEnable);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetStencilTestEnableEXT(VkCommandBuffer commandBuffer, VkBool32 stencilTestEnable) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetStencilTestEnableEXT(dispatch, commandBuffer, stencilTestEnable);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetStencilWriteMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t writeMask) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetStencilWriteMask(dispatch, commandBuffer, faceMask, writeMask);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetVertexInputEXT(VkCommandBuffer commandBuffer, uint32_t vertexBindingDescriptionCount, const VkVertexInputBindingDescription2EXT *pVertexBindingDescriptions, uint32_t vertexAttributeDescriptionCount, const VkVertexInputAttributeDescription2EXT *pVertexAttributeDescriptions) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetVertexInputEXT(dispatch, commandBuffer, vertexBindingDescriptionCount, pVertexBindingDescriptions, vertexAttributeDescriptionCount, pVertexAttributeDescriptions);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetViewport(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewport *pViewports) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetViewport(dispatch, commandBuffer, firstViewport, viewportCount, pViewports);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetViewportShadingRatePaletteNV(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkShadingRatePaletteNV *pShadingRatePalettes) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetViewportShadingRatePaletteNV(dispatch, commandBuffer, firstViewport, viewportCount, pShadingRatePalettes);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetViewportWScalingNV(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewportWScalingNV *pViewportWScalings) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetViewportWScalingNV(dispatch, commandBuffer, firstViewport, viewportCount, pViewportWScalings);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetViewportWithCount(VkCommandBuffer commandBuffer, uint32_t viewportCount, const VkViewport *pViewports) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetViewportWithCount(dispatch, commandBuffer, viewportCount, pViewports);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSetViewportWithCountEXT(VkCommandBuffer commandBuffer, uint32_t viewportCount, const VkViewport *pViewports) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSetViewportWithCountEXT(dispatch, commandBuffer, viewportCount, pViewports);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdSubpassShadingHUAWEI(VkCommandBuffer commandBuffer) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdSubpassShadingHUAWEI(dispatch, commandBuffer);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdTraceRaysIndirect2KHR(VkCommandBuffer commandBuffer, VkDeviceAddress indirectDeviceAddress) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdTraceRaysIndirect2KHR(dispatch, commandBuffer, indirectDeviceAddress);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdTraceRaysIndirectKHR(VkCommandBuffer commandBuffer, const VkStridedDeviceAddressRegionKHR *pRaygenShaderBindingTable, const VkStridedDeviceAddressRegionKHR *pMissShaderBindingTable, const VkStridedDeviceAddressRegionKHR *pHitShaderBindingTable, const VkStridedDeviceAddressRegionKHR *pCallableShaderBindingTable, VkDeviceAddress indirectDeviceAddress) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdTraceRaysIndirectKHR(dispatch, commandBuffer, pRaygenShaderBindingTable, pMissShaderBindingTable, pHitShaderBindingTable, pCallableShaderBindingTable, indirectDeviceAddress);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdTraceRaysKHR(VkCommandBuffer commandBuffer, const VkStridedDeviceAddressRegionKHR *pRaygenShaderBindingTable, const VkStridedDeviceAddressRegionKHR *pMissShaderBindingTable, const VkStridedDeviceAddressRegionKHR *pHitShaderBindingTable, const VkStridedDeviceAddressRegionKHR *pCallableShaderBindingTable, uint32_t width, uint32_t height, uint32_t depth) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdTraceRaysKHR(dispatch, commandBuffer, pRaygenShaderBindingTable, pMissShaderBindingTable, pHitShaderBindingTable, pCallableShaderBindingTable, width, height, depth);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdTraceRaysNV(VkCommandBuffer commandBuffer, VkBuffer raygenShaderBindingTableBuffer, VkDeviceSize raygenShaderBindingOffset, VkBuffer missShaderBindingTableBuffer, VkDeviceSize missShaderBindingOffset, VkDeviceSize missShaderBindingStride, VkBuffer hitShaderBindingTableBuffer, VkDeviceSize hitShaderBindingOffset, VkDeviceSize hitShaderBindingStride, VkBuffer callableShaderBindingTableBuffer, VkDeviceSize callableShaderBindingOffset, VkDeviceSize callableShaderBindingStride, uint32_t width, uint32_t height, uint32_t depth) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdTraceRaysNV(dispatch, commandBuffer, raygenShaderBindingTableBuffer, raygenShaderBindingOffset, missShaderBindingTableBuffer, missShaderBindingOffset, missShaderBindingStride, hitShaderBindingTableBuffer, hitShaderBindingOffset, hitShaderBindingStride, callableShaderBindingTableBuffer, callableShaderBindingOffset, callableShaderBindingStride, width, height, depth);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdUpdateBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void *pData) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdUpdateBuffer(dispatch, commandBuffer, dstBuffer, dstOffset, dataSize, pData);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdWaitEvents(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent *pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdWaitEvents(dispatch, commandBuffer, eventCount, pEvents, srcStageMask, dstStageMask, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdWaitEvents2(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent *pEvents, const VkDependencyInfo *pDependencyInfos) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdWaitEvents2(dispatch, commandBuffer, eventCount, pEvents, pDependencyInfos);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdWaitEvents2KHR(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent *pEvents, const VkDependencyInfo *pDependencyInfos) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdWaitEvents2KHR(dispatch, commandBuffer, eventCount, pEvents, pDependencyInfos);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdWriteAccelerationStructuresPropertiesKHR(VkCommandBuffer commandBuffer, uint32_t accelerationStructureCount, const VkAccelerationStructureKHR *pAccelerationStructures, VkQueryType queryType, VkQueryPool queryPool, uint32_t firstQuery) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdWriteAccelerationStructuresPropertiesKHR(dispatch, commandBuffer, accelerationStructureCount, pAccelerationStructures, queryType, queryPool, firstQuery);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdWriteAccelerationStructuresPropertiesNV(VkCommandBuffer commandBuffer, uint32_t accelerationStructureCount, const VkAccelerationStructureNV *pAccelerationStructures, VkQueryType queryType, VkQueryPool queryPool, uint32_t firstQuery) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdWriteAccelerationStructuresPropertiesNV(dispatch, commandBuffer, accelerationStructureCount, pAccelerationStructures, queryType, queryPool, firstQuery);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdWriteBufferMarker2AMD(VkCommandBuffer commandBuffer, VkPipelineStageFlags2 stage, VkBuffer dstBuffer, VkDeviceSize dstOffset, uint32_t marker) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdWriteBufferMarker2AMD(dispatch, commandBuffer, stage, dstBuffer, dstOffset, marker);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdWriteBufferMarkerAMD(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkBuffer dstBuffer, VkDeviceSize dstOffset, uint32_t marker) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdWriteBufferMarkerAMD(dispatch, commandBuffer, pipelineStage, dstBuffer, dstOffset, marker);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t query) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdWriteTimestamp(dispatch, commandBuffer, pipelineStage, queryPool, query);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdWriteTimestamp2(VkCommandBuffer commandBuffer, VkPipelineStageFlags2 stage, VkQueryPool queryPool, uint32_t query) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdWriteTimestamp2(dispatch, commandBuffer, stage, queryPool, query);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_CmdWriteTimestamp2KHR(VkCommandBuffer commandBuffer, VkPipelineStageFlags2 stage, VkQueryPool queryPool, uint32_t query) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    DeviceOverrides::CmdWriteTimestamp2KHR(dispatch, commandBuffer, stage, queryPool, query);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CompileDeferredNV(VkDevice device, VkPipeline pipeline, uint32_t shader) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CompileDeferredNV(dispatch, device, pipeline, shader);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CopyAccelerationStructureKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, const VkCopyAccelerationStructureInfoKHR *pInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CopyAccelerationStructureKHR(dispatch, device, deferredOperation, pInfo);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CopyAccelerationStructureToMemoryKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, const VkCopyAccelerationStructureToMemoryInfoKHR *pInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CopyAccelerationStructureToMemoryKHR(dispatch, device, deferredOperation, pInfo);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CopyMemoryToAccelerationStructureKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, const VkCopyMemoryToAccelerationStructureInfoKHR *pInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CopyMemoryToAccelerationStructureKHR(dispatch, device, deferredOperation, pInfo);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateAccelerationStructureKHR(VkDevice device, const VkAccelerationStructureCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkAccelerationStructureKHR *pAccelerationStructure) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateAccelerationStructureKHR(dispatch, device, pCreateInfo, pAllocator, pAccelerationStructure);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateAccelerationStructureNV(VkDevice device, const VkAccelerationStructureCreateInfoNV *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkAccelerationStructureNV *pAccelerationStructure) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateAccelerationStructureNV(dispatch, device, pCreateInfo, pAllocator, pAccelerationStructure);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateBuffer(VkDevice device, const VkBufferCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkBuffer *pBuffer) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateBuffer(dispatch, device, pCreateInfo, pAllocator, pBuffer);
    return ret;
  }

#ifdef VK_USE_PLATFORM_FUCHSIA
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateBufferCollectionFUCHSIA(VkDevice device, const VkBufferCollectionCreateInfoFUCHSIA *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkBufferCollectionFUCHSIA *pCollection) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateBufferCollectionFUCHSIA(dispatch, device, pCreateInfo, pAllocator, pCollection);
    return ret;
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateBufferView(VkDevice device, const VkBufferViewCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkBufferView *pView) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateBufferView(dispatch, device, pCreateInfo, pAllocator, pView);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkCommandPool *pCommandPool) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateCommandPool(dispatch, device, pCreateInfo, pAllocator, pCommandPool);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateComputePipelines(dispatch, device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
    return ret;
  }

#ifdef VKROOTS_PLATFORM_UNSUPPORTED
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateCuFunctionNVX(VkDevice device, const VkCuFunctionCreateInfoNVX *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkCuFunctionNVX *pFunction) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateCuFunctionNVX(dispatch, device, pCreateInfo, pAllocator, pFunction);
    return ret;
  }

#endif
#ifdef VKROOTS_PLATFORM_UNSUPPORTED
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateCuModuleNVX(VkDevice device, const VkCuModuleCreateInfoNVX *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkCuModuleNVX *pModule) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateCuModuleNVX(dispatch, device, pCreateInfo, pAllocator, pModule);
    return ret;
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateDeferredOperationKHR(VkDevice device, const VkAllocationCallbacks *pAllocator, VkDeferredOperationKHR *pDeferredOperation) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateDeferredOperationKHR(dispatch, device, pAllocator, pDeferredOperation);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorPool *pDescriptorPool) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateDescriptorPool(dispatch, device, pCreateInfo, pAllocator, pDescriptorPool);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorSetLayout *pSetLayout) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateDescriptorSetLayout(dispatch, device, pCreateInfo, pAllocator, pSetLayout);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateDescriptorUpdateTemplate(VkDevice device, const VkDescriptorUpdateTemplateCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorUpdateTemplate *pDescriptorUpdateTemplate) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateDescriptorUpdateTemplate(dispatch, device, pCreateInfo, pAllocator, pDescriptorUpdateTemplate);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateDescriptorUpdateTemplateKHR(VkDevice device, const VkDescriptorUpdateTemplateCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorUpdateTemplate *pDescriptorUpdateTemplate) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateDescriptorUpdateTemplateKHR(dispatch, device, pCreateInfo, pAllocator, pDescriptorUpdateTemplate);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateEvent(VkDevice device, const VkEventCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkEvent *pEvent) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateEvent(dispatch, device, pCreateInfo, pAllocator, pEvent);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateFence(VkDevice device, const VkFenceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkFence *pFence) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateFence(dispatch, device, pCreateInfo, pAllocator, pFence);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkFramebuffer *pFramebuffer) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateFramebuffer(dispatch, device, pCreateInfo, pAllocator, pFramebuffer);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateGraphicsPipelines(dispatch, device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateImage(VkDevice device, const VkImageCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkImage *pImage) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateImage(dispatch, device, pCreateInfo, pAllocator, pImage);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateImageView(VkDevice device, const VkImageViewCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkImageView *pView) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateImageView(dispatch, device, pCreateInfo, pAllocator, pView);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateIndirectCommandsLayoutNV(VkDevice device, const VkIndirectCommandsLayoutCreateInfoNV *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkIndirectCommandsLayoutNV *pIndirectCommandsLayout) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateIndirectCommandsLayoutNV(dispatch, device, pCreateInfo, pAllocator, pIndirectCommandsLayout);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreatePipelineCache(VkDevice device, const VkPipelineCacheCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkPipelineCache *pPipelineCache) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreatePipelineCache(dispatch, device, pCreateInfo, pAllocator, pPipelineCache);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkPipelineLayout *pPipelineLayout) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreatePipelineLayout(dispatch, device, pCreateInfo, pAllocator, pPipelineLayout);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreatePrivateDataSlot(VkDevice device, const VkPrivateDataSlotCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkPrivateDataSlot *pPrivateDataSlot) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreatePrivateDataSlot(dispatch, device, pCreateInfo, pAllocator, pPrivateDataSlot);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreatePrivateDataSlotEXT(VkDevice device, const VkPrivateDataSlotCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkPrivateDataSlot *pPrivateDataSlot) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreatePrivateDataSlotEXT(dispatch, device, pCreateInfo, pAllocator, pPrivateDataSlot);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateQueryPool(VkDevice device, const VkQueryPoolCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkQueryPool *pQueryPool) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateQueryPool(dispatch, device, pCreateInfo, pAllocator, pQueryPool);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateRayTracingPipelinesKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkRayTracingPipelineCreateInfoKHR *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateRayTracingPipelinesKHR(dispatch, device, deferredOperation, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateRayTracingPipelinesNV(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkRayTracingPipelineCreateInfoNV *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateRayTracingPipelinesNV(dispatch, device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateRenderPass(VkDevice device, const VkRenderPassCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateRenderPass(dispatch, device, pCreateInfo, pAllocator, pRenderPass);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateRenderPass2(VkDevice device, const VkRenderPassCreateInfo2 *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateRenderPass2(dispatch, device, pCreateInfo, pAllocator, pRenderPass);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateRenderPass2KHR(VkDevice device, const VkRenderPassCreateInfo2 *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateRenderPass2KHR(dispatch, device, pCreateInfo, pAllocator, pRenderPass);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateSampler(VkDevice device, const VkSamplerCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSampler *pSampler) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateSampler(dispatch, device, pCreateInfo, pAllocator, pSampler);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateSamplerYcbcrConversion(VkDevice device, const VkSamplerYcbcrConversionCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSamplerYcbcrConversion *pYcbcrConversion) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateSamplerYcbcrConversion(dispatch, device, pCreateInfo, pAllocator, pYcbcrConversion);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateSamplerYcbcrConversionKHR(VkDevice device, const VkSamplerYcbcrConversionCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSamplerYcbcrConversion *pYcbcrConversion) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateSamplerYcbcrConversionKHR(dispatch, device, pCreateInfo, pAllocator, pYcbcrConversion);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSemaphore *pSemaphore) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateSemaphore(dispatch, device, pCreateInfo, pAllocator, pSemaphore);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkShaderModule *pShaderModule) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateShaderModule(dispatch, device, pCreateInfo, pAllocator, pShaderModule);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateSharedSwapchainsKHR(VkDevice device, uint32_t swapchainCount, const VkSwapchainCreateInfoKHR *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkSwapchainKHR *pSwapchains) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateSharedSwapchainsKHR(dispatch, device, swapchainCount, pCreateInfos, pAllocator, pSwapchains);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSwapchainKHR *pSwapchain) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateSwapchainKHR(dispatch, device, pCreateInfo, pAllocator, pSwapchain);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateValidationCacheEXT(VkDevice device, const VkValidationCacheCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkValidationCacheEXT *pValidationCache) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateValidationCacheEXT(dispatch, device, pCreateInfo, pAllocator, pValidationCache);
    return ret;
  }

#ifdef VK_ENABLE_BETA_EXTENSIONS
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateVideoSessionKHR(VkDevice device, const VkVideoSessionCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkVideoSessionKHR *pVideoSession) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateVideoSessionKHR(dispatch, device, pCreateInfo, pAllocator, pVideoSession);
    return ret;
  }

#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_CreateVideoSessionParametersKHR(VkDevice device, const VkVideoSessionParametersCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkVideoSessionParametersKHR *pVideoSessionParameters) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::CreateVideoSessionParametersKHR(dispatch, device, pCreateInfo, pAllocator, pVideoSessionParameters);
    return ret;
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_DebugMarkerSetObjectNameEXT(VkDevice device, const VkDebugMarkerObjectNameInfoEXT *pNameInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::DebugMarkerSetObjectNameEXT(dispatch, device, pNameInfo);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_DebugMarkerSetObjectTagEXT(VkDevice device, const VkDebugMarkerObjectTagInfoEXT *pTagInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::DebugMarkerSetObjectTagEXT(dispatch, device, pTagInfo);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_DeferredOperationJoinKHR(VkDevice device, VkDeferredOperationKHR operation) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::DeferredOperationJoinKHR(dispatch, device, operation);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyAccelerationStructureKHR(VkDevice device, VkAccelerationStructureKHR accelerationStructure, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroyAccelerationStructureKHR(dispatch, device, accelerationStructure, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyAccelerationStructureNV(VkDevice device, VkAccelerationStructureNV accelerationStructure, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroyAccelerationStructureNV(dispatch, device, accelerationStructure, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyBuffer(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroyBuffer(dispatch, device, buffer, pAllocator);
  }

#ifdef VK_USE_PLATFORM_FUCHSIA
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyBufferCollectionFUCHSIA(VkDevice device, VkBufferCollectionFUCHSIA collection, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroyBufferCollectionFUCHSIA(dispatch, device, collection, pAllocator);
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyBufferView(VkDevice device, VkBufferView bufferView, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroyBufferView(dispatch, device, bufferView, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyCommandPool(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroyCommandPool(dispatch, device, commandPool, pAllocator);
  }

#ifdef VKROOTS_PLATFORM_UNSUPPORTED
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyCuFunctionNVX(VkDevice device, VkCuFunctionNVX function, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroyCuFunctionNVX(dispatch, device, function, pAllocator);
  }

#endif
#ifdef VKROOTS_PLATFORM_UNSUPPORTED
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyCuModuleNVX(VkDevice device, VkCuModuleNVX module, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroyCuModuleNVX(dispatch, device, module, pAllocator);
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyDeferredOperationKHR(VkDevice device, VkDeferredOperationKHR operation, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroyDeferredOperationKHR(dispatch, device, operation, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroyDescriptorPool(dispatch, device, descriptorPool, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroyDescriptorSetLayout(dispatch, device, descriptorSetLayout, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyDescriptorUpdateTemplate(VkDevice device, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroyDescriptorUpdateTemplate(dispatch, device, descriptorUpdateTemplate, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyDescriptorUpdateTemplateKHR(VkDevice device, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroyDescriptorUpdateTemplateKHR(dispatch, device, descriptorUpdateTemplate, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroyDevice(dispatch, device, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyEvent(VkDevice device, VkEvent event, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroyEvent(dispatch, device, event, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyFence(VkDevice device, VkFence fence, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroyFence(dispatch, device, fence, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroyFramebuffer(dispatch, device, framebuffer, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroyImage(dispatch, device, image, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyImageView(VkDevice device, VkImageView imageView, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroyImageView(dispatch, device, imageView, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyIndirectCommandsLayoutNV(VkDevice device, VkIndirectCommandsLayoutNV indirectCommandsLayout, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroyIndirectCommandsLayoutNV(dispatch, device, indirectCommandsLayout, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroyPipeline(dispatch, device, pipeline, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyPipelineCache(VkDevice device, VkPipelineCache pipelineCache, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroyPipelineCache(dispatch, device, pipelineCache, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroyPipelineLayout(dispatch, device, pipelineLayout, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyPrivateDataSlot(VkDevice device, VkPrivateDataSlot privateDataSlot, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroyPrivateDataSlot(dispatch, device, privateDataSlot, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyPrivateDataSlotEXT(VkDevice device, VkPrivateDataSlot privateDataSlot, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroyPrivateDataSlotEXT(dispatch, device, privateDataSlot, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyQueryPool(VkDevice device, VkQueryPool queryPool, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroyQueryPool(dispatch, device, queryPool, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroyRenderPass(dispatch, device, renderPass, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroySampler(dispatch, device, sampler, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroySamplerYcbcrConversion(VkDevice device, VkSamplerYcbcrConversion ycbcrConversion, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroySamplerYcbcrConversion(dispatch, device, ycbcrConversion, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroySamplerYcbcrConversionKHR(VkDevice device, VkSamplerYcbcrConversion ycbcrConversion, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroySamplerYcbcrConversionKHR(dispatch, device, ycbcrConversion, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroySemaphore(VkDevice device, VkSemaphore semaphore, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroySemaphore(dispatch, device, semaphore, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyShaderModule(VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroyShaderModule(dispatch, device, shaderModule, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroySwapchainKHR(dispatch, device, swapchain, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyValidationCacheEXT(VkDevice device, VkValidationCacheEXT validationCache, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroyValidationCacheEXT(dispatch, device, validationCache, pAllocator);
  }

#ifdef VK_ENABLE_BETA_EXTENSIONS
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyVideoSessionKHR(VkDevice device, VkVideoSessionKHR videoSession, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroyVideoSessionKHR(dispatch, device, videoSession, pAllocator);
  }

#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_DestroyVideoSessionParametersKHR(VkDevice device, VkVideoSessionParametersKHR videoSessionParameters, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::DestroyVideoSessionParametersKHR(dispatch, device, videoSessionParameters, pAllocator);
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_DeviceWaitIdle(VkDevice device) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::DeviceWaitIdle(dispatch, device);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_DisplayPowerControlEXT(VkDevice device, VkDisplayKHR display, const VkDisplayPowerInfoEXT *pDisplayPowerInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::DisplayPowerControlEXT(dispatch, device, display, pDisplayPowerInfo);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_EndCommandBuffer(VkCommandBuffer commandBuffer) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    VkResult ret = DeviceOverrides::EndCommandBuffer(dispatch, commandBuffer);
    return ret;
  }

#ifdef VK_USE_PLATFORM_METAL_EXT
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_ExportMetalObjectsEXT(VkDevice device, VkExportMetalObjectsInfoEXT *pMetalObjectsInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::ExportMetalObjectsEXT(dispatch, device, pMetalObjectsInfo);
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_FlushMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange *pMemoryRanges) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::FlushMappedMemoryRanges(dispatch, device, memoryRangeCount, pMemoryRanges);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_FreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer *pCommandBuffers) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::FreeCommandBuffers(dispatch, device, commandPool, commandBufferCount, pCommandBuffers);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_FreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet *pDescriptorSets) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::FreeDescriptorSets(dispatch, device, descriptorPool, descriptorSetCount, pDescriptorSets);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_FreeMemory(VkDevice device, VkDeviceMemory memory, const VkAllocationCallbacks *pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::FreeMemory(dispatch, device, memory, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetAccelerationStructureBuildSizesKHR(VkDevice device, VkAccelerationStructureBuildTypeKHR buildType, const VkAccelerationStructureBuildGeometryInfoKHR *pBuildInfo, const uint32_t *pMaxPrimitiveCounts, VkAccelerationStructureBuildSizesInfoKHR *pSizeInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetAccelerationStructureBuildSizesKHR(dispatch, device, buildType, pBuildInfo, pMaxPrimitiveCounts, pSizeInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkDeviceAddress wrap_GetAccelerationStructureDeviceAddressKHR(VkDevice device, const VkAccelerationStructureDeviceAddressInfoKHR *pInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkDeviceAddress ret = DeviceOverrides::GetAccelerationStructureDeviceAddressKHR(dispatch, device, pInfo);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetAccelerationStructureHandleNV(VkDevice device, VkAccelerationStructureNV accelerationStructure, size_t dataSize, void *pData) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetAccelerationStructureHandleNV(dispatch, device, accelerationStructure, dataSize, pData);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetAccelerationStructureMemoryRequirementsNV(VkDevice device, const VkAccelerationStructureMemoryRequirementsInfoNV *pInfo, VkMemoryRequirements2KHR *pMemoryRequirements) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetAccelerationStructureMemoryRequirementsNV(dispatch, device, pInfo, pMemoryRequirements);
  }

#ifdef VK_USE_PLATFORM_ANDROID_KHR
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetAndroidHardwareBufferPropertiesANDROID(VkDevice device, const struct AHardwareBuffer *buffer, VkAndroidHardwareBufferPropertiesANDROID *pProperties) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetAndroidHardwareBufferPropertiesANDROID(dispatch, device, buffer, pProperties);
    return ret;
  }

#endif
#ifdef VK_USE_PLATFORM_FUCHSIA
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetBufferCollectionPropertiesFUCHSIA(VkDevice device, VkBufferCollectionFUCHSIA collection, VkBufferCollectionPropertiesFUCHSIA *pProperties) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetBufferCollectionPropertiesFUCHSIA(dispatch, device, collection, pProperties);
    return ret;
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkDeviceAddress wrap_GetBufferDeviceAddress(VkDevice device, const VkBufferDeviceAddressInfo *pInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkDeviceAddress ret = DeviceOverrides::GetBufferDeviceAddress(dispatch, device, pInfo);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkDeviceAddress wrap_GetBufferDeviceAddressEXT(VkDevice device, const VkBufferDeviceAddressInfo *pInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkDeviceAddress ret = DeviceOverrides::GetBufferDeviceAddressEXT(dispatch, device, pInfo);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkDeviceAddress wrap_GetBufferDeviceAddressKHR(VkDevice device, const VkBufferDeviceAddressInfo *pInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkDeviceAddress ret = DeviceOverrides::GetBufferDeviceAddressKHR(dispatch, device, pInfo);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetBufferMemoryRequirements(VkDevice device, VkBuffer buffer, VkMemoryRequirements *pMemoryRequirements) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetBufferMemoryRequirements(dispatch, device, buffer, pMemoryRequirements);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetBufferMemoryRequirements2(VkDevice device, const VkBufferMemoryRequirementsInfo2 *pInfo, VkMemoryRequirements2 *pMemoryRequirements) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetBufferMemoryRequirements2(dispatch, device, pInfo, pMemoryRequirements);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetBufferMemoryRequirements2KHR(VkDevice device, const VkBufferMemoryRequirementsInfo2 *pInfo, VkMemoryRequirements2 *pMemoryRequirements) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetBufferMemoryRequirements2KHR(dispatch, device, pInfo, pMemoryRequirements);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static uint64_t wrap_GetBufferOpaqueCaptureAddress(VkDevice device, const VkBufferDeviceAddressInfo *pInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    uint64_t ret = DeviceOverrides::GetBufferOpaqueCaptureAddress(dispatch, device, pInfo);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static uint64_t wrap_GetBufferOpaqueCaptureAddressKHR(VkDevice device, const VkBufferDeviceAddressInfo *pInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    uint64_t ret = DeviceOverrides::GetBufferOpaqueCaptureAddressKHR(dispatch, device, pInfo);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetCalibratedTimestampsEXT(VkDevice device, uint32_t timestampCount, const VkCalibratedTimestampInfoEXT *pTimestampInfos, uint64_t *pTimestamps, uint64_t *pMaxDeviation) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetCalibratedTimestampsEXT(dispatch, device, timestampCount, pTimestampInfos, pTimestamps, pMaxDeviation);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static uint32_t wrap_GetDeferredOperationMaxConcurrencyKHR(VkDevice device, VkDeferredOperationKHR operation) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    uint32_t ret = DeviceOverrides::GetDeferredOperationMaxConcurrencyKHR(dispatch, device, operation);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetDeferredOperationResultKHR(VkDevice device, VkDeferredOperationKHR operation) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetDeferredOperationResultKHR(dispatch, device, operation);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetDescriptorSetHostMappingVALVE(VkDevice device, VkDescriptorSet descriptorSet, void **ppData) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetDescriptorSetHostMappingVALVE(dispatch, device, descriptorSet, ppData);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetDescriptorSetLayoutHostMappingInfoVALVE(VkDevice device, const VkDescriptorSetBindingReferenceVALVE *pBindingReference, VkDescriptorSetLayoutHostMappingInfoVALVE *pHostMapping) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetDescriptorSetLayoutHostMappingInfoVALVE(dispatch, device, pBindingReference, pHostMapping);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetDescriptorSetLayoutSupport(VkDevice device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo, VkDescriptorSetLayoutSupport *pSupport) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetDescriptorSetLayoutSupport(dispatch, device, pCreateInfo, pSupport);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetDescriptorSetLayoutSupportKHR(VkDevice device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo, VkDescriptorSetLayoutSupport *pSupport) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetDescriptorSetLayoutSupportKHR(dispatch, device, pCreateInfo, pSupport);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetDeviceAccelerationStructureCompatibilityKHR(VkDevice device, const VkAccelerationStructureVersionInfoKHR *pVersionInfo, VkAccelerationStructureCompatibilityKHR *pCompatibility) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetDeviceAccelerationStructureCompatibilityKHR(dispatch, device, pVersionInfo, pCompatibility);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetDeviceBufferMemoryRequirements(VkDevice device, const VkDeviceBufferMemoryRequirements *pInfo, VkMemoryRequirements2 *pMemoryRequirements) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetDeviceBufferMemoryRequirements(dispatch, device, pInfo, pMemoryRequirements);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetDeviceBufferMemoryRequirementsKHR(VkDevice device, const VkDeviceBufferMemoryRequirements *pInfo, VkMemoryRequirements2 *pMemoryRequirements) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetDeviceBufferMemoryRequirementsKHR(dispatch, device, pInfo, pMemoryRequirements);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetDeviceGroupPeerMemoryFeatures(VkDevice device, uint32_t heapIndex, uint32_t localDeviceIndex, uint32_t remoteDeviceIndex, VkPeerMemoryFeatureFlags *pPeerMemoryFeatures) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetDeviceGroupPeerMemoryFeatures(dispatch, device, heapIndex, localDeviceIndex, remoteDeviceIndex, pPeerMemoryFeatures);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetDeviceGroupPeerMemoryFeaturesKHR(VkDevice device, uint32_t heapIndex, uint32_t localDeviceIndex, uint32_t remoteDeviceIndex, VkPeerMemoryFeatureFlags *pPeerMemoryFeatures) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetDeviceGroupPeerMemoryFeaturesKHR(dispatch, device, heapIndex, localDeviceIndex, remoteDeviceIndex, pPeerMemoryFeatures);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetDeviceGroupPresentCapabilitiesKHR(VkDevice device, VkDeviceGroupPresentCapabilitiesKHR *pDeviceGroupPresentCapabilities) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetDeviceGroupPresentCapabilitiesKHR(dispatch, device, pDeviceGroupPresentCapabilities);
    return ret;
  }

#ifdef VK_USE_PLATFORM_WIN32_KHR
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetDeviceGroupSurfacePresentModes2EXT(VkDevice device, const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo, VkDeviceGroupPresentModeFlagsKHR *pModes) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetDeviceGroupSurfacePresentModes2EXT(dispatch, device, pSurfaceInfo, pModes);
    return ret;
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetDeviceGroupSurfacePresentModesKHR(VkDevice device, VkSurfaceKHR surface, VkDeviceGroupPresentModeFlagsKHR *pModes) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetDeviceGroupSurfacePresentModesKHR(dispatch, device, surface, pModes);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetDeviceImageMemoryRequirements(VkDevice device, const VkDeviceImageMemoryRequirements *pInfo, VkMemoryRequirements2 *pMemoryRequirements) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetDeviceImageMemoryRequirements(dispatch, device, pInfo, pMemoryRequirements);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetDeviceImageMemoryRequirementsKHR(VkDevice device, const VkDeviceImageMemoryRequirements *pInfo, VkMemoryRequirements2 *pMemoryRequirements) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetDeviceImageMemoryRequirementsKHR(dispatch, device, pInfo, pMemoryRequirements);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetDeviceImageSparseMemoryRequirements(VkDevice device, const VkDeviceImageMemoryRequirements *pInfo, uint32_t *pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements2 *pSparseMemoryRequirements) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetDeviceImageSparseMemoryRequirements(dispatch, device, pInfo, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetDeviceImageSparseMemoryRequirementsKHR(VkDevice device, const VkDeviceImageMemoryRequirements *pInfo, uint32_t *pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements2 *pSparseMemoryRequirements) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetDeviceImageSparseMemoryRequirementsKHR(dispatch, device, pInfo, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetDeviceMemoryCommitment(VkDevice device, VkDeviceMemory memory, VkDeviceSize *pCommittedMemoryInBytes) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetDeviceMemoryCommitment(dispatch, device, memory, pCommittedMemoryInBytes);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static uint64_t wrap_GetDeviceMemoryOpaqueCaptureAddress(VkDevice device, const VkDeviceMemoryOpaqueCaptureAddressInfo *pInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    uint64_t ret = DeviceOverrides::GetDeviceMemoryOpaqueCaptureAddress(dispatch, device, pInfo);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static uint64_t wrap_GetDeviceMemoryOpaqueCaptureAddressKHR(VkDevice device, const VkDeviceMemoryOpaqueCaptureAddressInfo *pInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    uint64_t ret = DeviceOverrides::GetDeviceMemoryOpaqueCaptureAddressKHR(dispatch, device, pInfo);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue *pQueue) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetDeviceQueue(dispatch, device, queueFamilyIndex, queueIndex, pQueue);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetDeviceQueue2(VkDevice device, const VkDeviceQueueInfo2 *pQueueInfo, VkQueue *pQueue) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetDeviceQueue2(dispatch, device, pQueueInfo, pQueue);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI(VkDevice device, VkRenderPass renderpass, VkExtent2D *pMaxWorkgroupSize) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI(dispatch, device, renderpass, pMaxWorkgroupSize);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetEventStatus(VkDevice device, VkEvent event) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetEventStatus(dispatch, device, event);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetFenceFdKHR(VkDevice device, const VkFenceGetFdInfoKHR *pGetFdInfo, int *pFd) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetFenceFdKHR(dispatch, device, pGetFdInfo, pFd);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetFenceStatus(VkDevice device, VkFence fence) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetFenceStatus(dispatch, device, fence);
    return ret;
  }

#ifdef VK_USE_PLATFORM_WIN32_KHR
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetFenceWin32HandleKHR(VkDevice device, const VkFenceGetWin32HandleInfoKHR *pGetWin32HandleInfo, HANDLE *pHandle) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetFenceWin32HandleKHR(dispatch, device, pGetWin32HandleInfo, pHandle);
    return ret;
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetGeneratedCommandsMemoryRequirementsNV(VkDevice device, const VkGeneratedCommandsMemoryRequirementsInfoNV *pInfo, VkMemoryRequirements2 *pMemoryRequirements) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetGeneratedCommandsMemoryRequirementsNV(dispatch, device, pInfo, pMemoryRequirements);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetImageDrmFormatModifierPropertiesEXT(VkDevice device, VkImage image, VkImageDrmFormatModifierPropertiesEXT *pProperties) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetImageDrmFormatModifierPropertiesEXT(dispatch, device, image, pProperties);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetImageMemoryRequirements(VkDevice device, VkImage image, VkMemoryRequirements *pMemoryRequirements) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetImageMemoryRequirements(dispatch, device, image, pMemoryRequirements);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetImageMemoryRequirements2(VkDevice device, const VkImageMemoryRequirementsInfo2 *pInfo, VkMemoryRequirements2 *pMemoryRequirements) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetImageMemoryRequirements2(dispatch, device, pInfo, pMemoryRequirements);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetImageMemoryRequirements2KHR(VkDevice device, const VkImageMemoryRequirementsInfo2 *pInfo, VkMemoryRequirements2 *pMemoryRequirements) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetImageMemoryRequirements2KHR(dispatch, device, pInfo, pMemoryRequirements);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetImageSparseMemoryRequirements(VkDevice device, VkImage image, uint32_t *pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements *pSparseMemoryRequirements) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetImageSparseMemoryRequirements(dispatch, device, image, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetImageSparseMemoryRequirements2(VkDevice device, const VkImageSparseMemoryRequirementsInfo2 *pInfo, uint32_t *pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements2 *pSparseMemoryRequirements) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetImageSparseMemoryRequirements2(dispatch, device, pInfo, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetImageSparseMemoryRequirements2KHR(VkDevice device, const VkImageSparseMemoryRequirementsInfo2 *pInfo, uint32_t *pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements2 *pSparseMemoryRequirements) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetImageSparseMemoryRequirements2KHR(dispatch, device, pInfo, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetImageSubresourceLayout(VkDevice device, VkImage image, const VkImageSubresource *pSubresource, VkSubresourceLayout *pLayout) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetImageSubresourceLayout(dispatch, device, image, pSubresource, pLayout);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetImageSubresourceLayout2EXT(VkDevice device, VkImage image, const VkImageSubresource2EXT *pSubresource, VkSubresourceLayout2EXT *pLayout) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetImageSubresourceLayout2EXT(dispatch, device, image, pSubresource, pLayout);
  }

#ifdef VKROOTS_PLATFORM_UNSUPPORTED
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetImageViewAddressNVX(VkDevice device, VkImageView imageView, VkImageViewAddressPropertiesNVX *pProperties) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetImageViewAddressNVX(dispatch, device, imageView, pProperties);
    return ret;
  }

#endif
#ifdef VKROOTS_PLATFORM_UNSUPPORTED
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static uint32_t wrap_GetImageViewHandleNVX(VkDevice device, const VkImageViewHandleInfoNVX *pInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    uint32_t ret = DeviceOverrides::GetImageViewHandleNVX(dispatch, device, pInfo);
    return ret;
  }

#endif
#ifdef VK_USE_PLATFORM_ANDROID_KHR
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetMemoryAndroidHardwareBufferANDROID(VkDevice device, const VkMemoryGetAndroidHardwareBufferInfoANDROID *pInfo, struct AHardwareBuffer **pBuffer) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetMemoryAndroidHardwareBufferANDROID(dispatch, device, pInfo, pBuffer);
    return ret;
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetMemoryFdKHR(VkDevice device, const VkMemoryGetFdInfoKHR *pGetFdInfo, int *pFd) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetMemoryFdKHR(dispatch, device, pGetFdInfo, pFd);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetMemoryFdPropertiesKHR(VkDevice device, VkExternalMemoryHandleTypeFlagBits handleType, int fd, VkMemoryFdPropertiesKHR *pMemoryFdProperties) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetMemoryFdPropertiesKHR(dispatch, device, handleType, fd, pMemoryFdProperties);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetMemoryHostPointerPropertiesEXT(VkDevice device, VkExternalMemoryHandleTypeFlagBits handleType, const void *pHostPointer, VkMemoryHostPointerPropertiesEXT *pMemoryHostPointerProperties) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetMemoryHostPointerPropertiesEXT(dispatch, device, handleType, pHostPointer, pMemoryHostPointerProperties);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetMemoryRemoteAddressNV(VkDevice device, const VkMemoryGetRemoteAddressInfoNV *pMemoryGetRemoteAddressInfo, VkRemoteAddressNV *pAddress) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetMemoryRemoteAddressNV(dispatch, device, pMemoryGetRemoteAddressInfo, pAddress);
    return ret;
  }

#ifdef VK_USE_PLATFORM_WIN32_KHR
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetMemoryWin32HandleKHR(VkDevice device, const VkMemoryGetWin32HandleInfoKHR *pGetWin32HandleInfo, HANDLE *pHandle) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetMemoryWin32HandleKHR(dispatch, device, pGetWin32HandleInfo, pHandle);
    return ret;
  }

#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetMemoryWin32HandleNV(VkDevice device, VkDeviceMemory memory, VkExternalMemoryHandleTypeFlagsNV handleType, HANDLE *pHandle) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetMemoryWin32HandleNV(dispatch, device, memory, handleType, pHandle);
    return ret;
  }

#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetMemoryWin32HandlePropertiesKHR(VkDevice device, VkExternalMemoryHandleTypeFlagBits handleType, HANDLE handle, VkMemoryWin32HandlePropertiesKHR *pMemoryWin32HandleProperties) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetMemoryWin32HandlePropertiesKHR(dispatch, device, handleType, handle, pMemoryWin32HandleProperties);
    return ret;
  }

#endif
#ifdef VK_USE_PLATFORM_FUCHSIA
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetMemoryZirconHandleFUCHSIA(VkDevice device, const VkMemoryGetZirconHandleInfoFUCHSIA *pGetZirconHandleInfo, zx_handle_t *pZirconHandle) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetMemoryZirconHandleFUCHSIA(dispatch, device, pGetZirconHandleInfo, pZirconHandle);
    return ret;
  }

#endif
#ifdef VK_USE_PLATFORM_FUCHSIA
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetMemoryZirconHandlePropertiesFUCHSIA(VkDevice device, VkExternalMemoryHandleTypeFlagBits handleType, zx_handle_t zirconHandle, VkMemoryZirconHandlePropertiesFUCHSIA *pMemoryZirconHandleProperties) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetMemoryZirconHandlePropertiesFUCHSIA(dispatch, device, handleType, zirconHandle, pMemoryZirconHandleProperties);
    return ret;
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetPastPresentationTimingGOOGLE(VkDevice device, VkSwapchainKHR swapchain, uint32_t *pPresentationTimingCount, VkPastPresentationTimingGOOGLE *pPresentationTimings) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetPastPresentationTimingGOOGLE(dispatch, device, swapchain, pPresentationTimingCount, pPresentationTimings);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetPerformanceParameterINTEL(VkDevice device, VkPerformanceParameterTypeINTEL parameter, VkPerformanceValueINTEL *pValue) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetPerformanceParameterINTEL(dispatch, device, parameter, pValue);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetPipelineCacheData(VkDevice device, VkPipelineCache pipelineCache, size_t *pDataSize, void *pData) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetPipelineCacheData(dispatch, device, pipelineCache, pDataSize, pData);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetPipelineExecutableInternalRepresentationsKHR(VkDevice device, const VkPipelineExecutableInfoKHR *pExecutableInfo, uint32_t *pInternalRepresentationCount, VkPipelineExecutableInternalRepresentationKHR *pInternalRepresentations) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetPipelineExecutableInternalRepresentationsKHR(dispatch, device, pExecutableInfo, pInternalRepresentationCount, pInternalRepresentations);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetPipelineExecutablePropertiesKHR(VkDevice device, const VkPipelineInfoKHR *pPipelineInfo, uint32_t *pExecutableCount, VkPipelineExecutablePropertiesKHR *pProperties) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetPipelineExecutablePropertiesKHR(dispatch, device, pPipelineInfo, pExecutableCount, pProperties);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetPipelineExecutableStatisticsKHR(VkDevice device, const VkPipelineExecutableInfoKHR *pExecutableInfo, uint32_t *pStatisticCount, VkPipelineExecutableStatisticKHR *pStatistics) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetPipelineExecutableStatisticsKHR(dispatch, device, pExecutableInfo, pStatisticCount, pStatistics);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetPipelinePropertiesEXT(VkDevice device, const VkPipelineInfoEXT *pPipelineInfo, VkBaseOutStructure *pPipelineProperties) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetPipelinePropertiesEXT(dispatch, device, pPipelineInfo, pPipelineProperties);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetPrivateData(VkDevice device, VkObjectType objectType, uint64_t objectHandle, VkPrivateDataSlot privateDataSlot, uint64_t *pData) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetPrivateData(dispatch, device, objectType, objectHandle, privateDataSlot, pData);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetPrivateDataEXT(VkDevice device, VkObjectType objectType, uint64_t objectHandle, VkPrivateDataSlot privateDataSlot, uint64_t *pData) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetPrivateDataEXT(dispatch, device, objectType, objectHandle, privateDataSlot, pData);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetQueryPoolResults(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, size_t dataSize, void *pData, VkDeviceSize stride, VkQueryResultFlags flags) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetQueryPoolResults(dispatch, device, queryPool, firstQuery, queryCount, dataSize, pData, stride, flags);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetQueueCheckpointData2NV(VkQueue queue, uint32_t *pCheckpointDataCount, VkCheckpointData2NV *pCheckpointData) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(queue);
    DeviceOverrides::GetQueueCheckpointData2NV(dispatch, queue, pCheckpointDataCount, pCheckpointData);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetQueueCheckpointDataNV(VkQueue queue, uint32_t *pCheckpointDataCount, VkCheckpointDataNV *pCheckpointData) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(queue);
    DeviceOverrides::GetQueueCheckpointDataNV(dispatch, queue, pCheckpointDataCount, pCheckpointData);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetRayTracingCaptureReplayShaderGroupHandlesKHR(VkDevice device, VkPipeline pipeline, uint32_t firstGroup, uint32_t groupCount, size_t dataSize, void *pData) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetRayTracingCaptureReplayShaderGroupHandlesKHR(dispatch, device, pipeline, firstGroup, groupCount, dataSize, pData);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetRayTracingShaderGroupHandlesKHR(VkDevice device, VkPipeline pipeline, uint32_t firstGroup, uint32_t groupCount, size_t dataSize, void *pData) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetRayTracingShaderGroupHandlesKHR(dispatch, device, pipeline, firstGroup, groupCount, dataSize, pData);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetRayTracingShaderGroupHandlesNV(VkDevice device, VkPipeline pipeline, uint32_t firstGroup, uint32_t groupCount, size_t dataSize, void *pData) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetRayTracingShaderGroupHandlesNV(dispatch, device, pipeline, firstGroup, groupCount, dataSize, pData);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkDeviceSize wrap_GetRayTracingShaderGroupStackSizeKHR(VkDevice device, VkPipeline pipeline, uint32_t group, VkShaderGroupShaderKHR groupShader) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkDeviceSize ret = DeviceOverrides::GetRayTracingShaderGroupStackSizeKHR(dispatch, device, pipeline, group, groupShader);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetRefreshCycleDurationGOOGLE(VkDevice device, VkSwapchainKHR swapchain, VkRefreshCycleDurationGOOGLE *pDisplayTimingProperties) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetRefreshCycleDurationGOOGLE(dispatch, device, swapchain, pDisplayTimingProperties);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetRenderAreaGranularity(VkDevice device, VkRenderPass renderPass, VkExtent2D *pGranularity) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetRenderAreaGranularity(dispatch, device, renderPass, pGranularity);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetSemaphoreCounterValue(VkDevice device, VkSemaphore semaphore, uint64_t *pValue) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetSemaphoreCounterValue(dispatch, device, semaphore, pValue);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetSemaphoreCounterValueKHR(VkDevice device, VkSemaphore semaphore, uint64_t *pValue) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetSemaphoreCounterValueKHR(dispatch, device, semaphore, pValue);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetSemaphoreFdKHR(VkDevice device, const VkSemaphoreGetFdInfoKHR *pGetFdInfo, int *pFd) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetSemaphoreFdKHR(dispatch, device, pGetFdInfo, pFd);
    return ret;
  }

#ifdef VK_USE_PLATFORM_WIN32_KHR
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetSemaphoreWin32HandleKHR(VkDevice device, const VkSemaphoreGetWin32HandleInfoKHR *pGetWin32HandleInfo, HANDLE *pHandle) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetSemaphoreWin32HandleKHR(dispatch, device, pGetWin32HandleInfo, pHandle);
    return ret;
  }

#endif
#ifdef VK_USE_PLATFORM_FUCHSIA
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetSemaphoreZirconHandleFUCHSIA(VkDevice device, const VkSemaphoreGetZirconHandleInfoFUCHSIA *pGetZirconHandleInfo, zx_handle_t *pZirconHandle) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetSemaphoreZirconHandleFUCHSIA(dispatch, device, pGetZirconHandleInfo, pZirconHandle);
    return ret;
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetShaderInfoAMD(VkDevice device, VkPipeline pipeline, VkShaderStageFlagBits shaderStage, VkShaderInfoTypeAMD infoType, size_t *pInfoSize, void *pInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetShaderInfoAMD(dispatch, device, pipeline, shaderStage, infoType, pInfoSize, pInfo);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetShaderModuleCreateInfoIdentifierEXT(VkDevice device, const VkShaderModuleCreateInfo *pCreateInfo, VkShaderModuleIdentifierEXT *pIdentifier) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetShaderModuleCreateInfoIdentifierEXT(dispatch, device, pCreateInfo, pIdentifier);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_GetShaderModuleIdentifierEXT(VkDevice device, VkShaderModule shaderModule, VkShaderModuleIdentifierEXT *pIdentifier) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::GetShaderModuleIdentifierEXT(dispatch, device, shaderModule, pIdentifier);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetSwapchainCounterEXT(VkDevice device, VkSwapchainKHR swapchain, VkSurfaceCounterFlagBitsEXT counter, uint64_t *pCounterValue) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetSwapchainCounterEXT(dispatch, device, swapchain, counter, pCounterValue);
    return ret;
  }

#ifdef VKROOTS_PLATFORM_UNSUPPORTED
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetSwapchainGrallocUsage2ANDROID(VkDevice device, VkFormat format, VkImageUsageFlags imageUsage, VkSwapchainImageUsageFlagsANDROID swapchainImageUsage, uint64_t *grallocConsumerUsage, uint64_t *grallocProducerUsage) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetSwapchainGrallocUsage2ANDROID(dispatch, device, format, imageUsage, swapchainImageUsage, grallocConsumerUsage, grallocProducerUsage);
    return ret;
  }

#endif
#ifdef VKROOTS_PLATFORM_UNSUPPORTED
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetSwapchainGrallocUsageANDROID(VkDevice device, VkFormat format, VkImageUsageFlags imageUsage, int *grallocUsage) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetSwapchainGrallocUsageANDROID(dispatch, device, format, imageUsage, grallocUsage);
    return ret;
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t *pSwapchainImageCount, VkImage *pSwapchainImages) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetSwapchainImagesKHR(dispatch, device, swapchain, pSwapchainImageCount, pSwapchainImages);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetSwapchainStatusKHR(VkDevice device, VkSwapchainKHR swapchain) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetSwapchainStatusKHR(dispatch, device, swapchain);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetValidationCacheDataEXT(VkDevice device, VkValidationCacheEXT validationCache, size_t *pDataSize, void *pData) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetValidationCacheDataEXT(dispatch, device, validationCache, pDataSize, pData);
    return ret;
  }

#ifdef VK_ENABLE_BETA_EXTENSIONS
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_GetVideoSessionMemoryRequirementsKHR(VkDevice device, VkVideoSessionKHR videoSession, uint32_t *pVideoSessionMemoryRequirementsCount, VkVideoGetMemoryPropertiesKHR *pVideoSessionMemoryRequirements) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::GetVideoSessionMemoryRequirementsKHR(dispatch, device, videoSession, pVideoSessionMemoryRequirementsCount, pVideoSessionMemoryRequirements);
    return ret;
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_ImportFenceFdKHR(VkDevice device, const VkImportFenceFdInfoKHR *pImportFenceFdInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::ImportFenceFdKHR(dispatch, device, pImportFenceFdInfo);
    return ret;
  }

#ifdef VK_USE_PLATFORM_WIN32_KHR
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_ImportFenceWin32HandleKHR(VkDevice device, const VkImportFenceWin32HandleInfoKHR *pImportFenceWin32HandleInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::ImportFenceWin32HandleKHR(dispatch, device, pImportFenceWin32HandleInfo);
    return ret;
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_ImportSemaphoreFdKHR(VkDevice device, const VkImportSemaphoreFdInfoKHR *pImportSemaphoreFdInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::ImportSemaphoreFdKHR(dispatch, device, pImportSemaphoreFdInfo);
    return ret;
  }

#ifdef VK_USE_PLATFORM_WIN32_KHR
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_ImportSemaphoreWin32HandleKHR(VkDevice device, const VkImportSemaphoreWin32HandleInfoKHR *pImportSemaphoreWin32HandleInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::ImportSemaphoreWin32HandleKHR(dispatch, device, pImportSemaphoreWin32HandleInfo);
    return ret;
  }

#endif
#ifdef VK_USE_PLATFORM_FUCHSIA
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_ImportSemaphoreZirconHandleFUCHSIA(VkDevice device, const VkImportSemaphoreZirconHandleInfoFUCHSIA *pImportSemaphoreZirconHandleInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::ImportSemaphoreZirconHandleFUCHSIA(dispatch, device, pImportSemaphoreZirconHandleInfo);
    return ret;
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_InitializePerformanceApiINTEL(VkDevice device, const VkInitializePerformanceApiInfoINTEL *pInitializeInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::InitializePerformanceApiINTEL(dispatch, device, pInitializeInfo);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_InvalidateMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange *pMemoryRanges) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::InvalidateMappedMemoryRanges(dispatch, device, memoryRangeCount, pMemoryRanges);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_MapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void **ppData) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::MapMemory(dispatch, device, memory, offset, size, flags, ppData);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_MergePipelineCaches(VkDevice device, VkPipelineCache dstCache, uint32_t srcCacheCount, const VkPipelineCache *pSrcCaches) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::MergePipelineCaches(dispatch, device, dstCache, srcCacheCount, pSrcCaches);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_MergeValidationCachesEXT(VkDevice device, VkValidationCacheEXT dstCache, uint32_t srcCacheCount, const VkValidationCacheEXT *pSrcCaches) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::MergeValidationCachesEXT(dispatch, device, dstCache, srcCacheCount, pSrcCaches);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_QueueBeginDebugUtilsLabelEXT(VkQueue queue, const VkDebugUtilsLabelEXT *pLabelInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(queue);
    DeviceOverrides::QueueBeginDebugUtilsLabelEXT(dispatch, queue, pLabelInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_QueueBindSparse(VkQueue queue, uint32_t bindInfoCount, const VkBindSparseInfo *pBindInfo, VkFence fence) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(queue);
    VkResult ret = DeviceOverrides::QueueBindSparse(dispatch, queue, bindInfoCount, pBindInfo, fence);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_QueueEndDebugUtilsLabelEXT(VkQueue queue) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(queue);
    DeviceOverrides::QueueEndDebugUtilsLabelEXT(dispatch, queue);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_QueueInsertDebugUtilsLabelEXT(VkQueue queue, const VkDebugUtilsLabelEXT *pLabelInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(queue);
    DeviceOverrides::QueueInsertDebugUtilsLabelEXT(dispatch, queue, pLabelInfo);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_QueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *pPresentInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(queue);
    VkResult ret = DeviceOverrides::QueuePresentKHR(dispatch, queue, pPresentInfo);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_QueueSetPerformanceConfigurationINTEL(VkQueue queue, VkPerformanceConfigurationINTEL configuration) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(queue);
    VkResult ret = DeviceOverrides::QueueSetPerformanceConfigurationINTEL(dispatch, queue, configuration);
    return ret;
  }

#ifdef VKROOTS_PLATFORM_UNSUPPORTED
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_QueueSignalReleaseImageANDROID(VkQueue queue, uint32_t waitSemaphoreCount, const VkSemaphore *pWaitSemaphores, VkImage image, int *pNativeFenceFd) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(queue);
    VkResult ret = DeviceOverrides::QueueSignalReleaseImageANDROID(dispatch, queue, waitSemaphoreCount, pWaitSemaphores, image, pNativeFenceFd);
    return ret;
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_QueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo *pSubmits, VkFence fence) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(queue);
    VkResult ret = DeviceOverrides::QueueSubmit(dispatch, queue, submitCount, pSubmits, fence);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_QueueSubmit2(VkQueue queue, uint32_t submitCount, const VkSubmitInfo2 *pSubmits, VkFence fence) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(queue);
    VkResult ret = DeviceOverrides::QueueSubmit2(dispatch, queue, submitCount, pSubmits, fence);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_QueueSubmit2KHR(VkQueue queue, uint32_t submitCount, const VkSubmitInfo2 *pSubmits, VkFence fence) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(queue);
    VkResult ret = DeviceOverrides::QueueSubmit2KHR(dispatch, queue, submitCount, pSubmits, fence);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_QueueWaitIdle(VkQueue queue) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(queue);
    VkResult ret = DeviceOverrides::QueueWaitIdle(dispatch, queue);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_RegisterDeviceEventEXT(VkDevice device, const VkDeviceEventInfoEXT *pDeviceEventInfo, const VkAllocationCallbacks *pAllocator, VkFence *pFence) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::RegisterDeviceEventEXT(dispatch, device, pDeviceEventInfo, pAllocator, pFence);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_RegisterDisplayEventEXT(VkDevice device, VkDisplayKHR display, const VkDisplayEventInfoEXT *pDisplayEventInfo, const VkAllocationCallbacks *pAllocator, VkFence *pFence) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::RegisterDisplayEventEXT(dispatch, device, display, pDisplayEventInfo, pAllocator, pFence);
    return ret;
  }

#ifdef VK_USE_PLATFORM_WIN32_KHR
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_ReleaseFullScreenExclusiveModeEXT(VkDevice device, VkSwapchainKHR swapchain) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::ReleaseFullScreenExclusiveModeEXT(dispatch, device, swapchain);
    return ret;
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_ReleasePerformanceConfigurationINTEL(VkDevice device, VkPerformanceConfigurationINTEL configuration) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::ReleasePerformanceConfigurationINTEL(dispatch, device, configuration);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_ReleaseProfilingLockKHR(VkDevice device) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::ReleaseProfilingLockKHR(dispatch, device);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_ResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(commandBuffer);
    VkResult ret = DeviceOverrides::ResetCommandBuffer(dispatch, commandBuffer, flags);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_ResetCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::ResetCommandPool(dispatch, device, commandPool, flags);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_ResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorPoolResetFlags flags) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::ResetDescriptorPool(dispatch, device, descriptorPool, flags);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_ResetEvent(VkDevice device, VkEvent event) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::ResetEvent(dispatch, device, event);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_ResetFences(VkDevice device, uint32_t fenceCount, const VkFence *pFences) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::ResetFences(dispatch, device, fenceCount, pFences);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_ResetQueryPool(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::ResetQueryPool(dispatch, device, queryPool, firstQuery, queryCount);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_ResetQueryPoolEXT(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::ResetQueryPoolEXT(dispatch, device, queryPool, firstQuery, queryCount);
  }

#ifdef VK_USE_PLATFORM_FUCHSIA
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_SetBufferCollectionBufferConstraintsFUCHSIA(VkDevice device, VkBufferCollectionFUCHSIA collection, const VkBufferConstraintsInfoFUCHSIA *pBufferConstraintsInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::SetBufferCollectionBufferConstraintsFUCHSIA(dispatch, device, collection, pBufferConstraintsInfo);
    return ret;
  }

#endif
#ifdef VK_USE_PLATFORM_FUCHSIA
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_SetBufferCollectionImageConstraintsFUCHSIA(VkDevice device, VkBufferCollectionFUCHSIA collection, const VkImageConstraintsInfoFUCHSIA *pImageConstraintsInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::SetBufferCollectionImageConstraintsFUCHSIA(dispatch, device, collection, pImageConstraintsInfo);
    return ret;
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_SetDebugUtilsObjectNameEXT(VkDevice device, const VkDebugUtilsObjectNameInfoEXT *pNameInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::SetDebugUtilsObjectNameEXT(dispatch, device, pNameInfo);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_SetDebugUtilsObjectTagEXT(VkDevice device, const VkDebugUtilsObjectTagInfoEXT *pTagInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::SetDebugUtilsObjectTagEXT(dispatch, device, pTagInfo);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_SetDeviceMemoryPriorityEXT(VkDevice device, VkDeviceMemory memory, float priority) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::SetDeviceMemoryPriorityEXT(dispatch, device, memory, priority);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_SetEvent(VkDevice device, VkEvent event) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::SetEvent(dispatch, device, event);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_SetHdrMetadataEXT(VkDevice device, uint32_t swapchainCount, const VkSwapchainKHR *pSwapchains, const VkHdrMetadataEXT *pMetadata) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::SetHdrMetadataEXT(dispatch, device, swapchainCount, pSwapchains, pMetadata);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_SetLocalDimmingAMD(VkDevice device, VkSwapchainKHR swapChain, VkBool32 localDimmingEnable) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::SetLocalDimmingAMD(dispatch, device, swapChain, localDimmingEnable);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_SetPrivateData(VkDevice device, VkObjectType objectType, uint64_t objectHandle, VkPrivateDataSlot privateDataSlot, uint64_t data) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::SetPrivateData(dispatch, device, objectType, objectHandle, privateDataSlot, data);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_SetPrivateDataEXT(VkDevice device, VkObjectType objectType, uint64_t objectHandle, VkPrivateDataSlot privateDataSlot, uint64_t data) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::SetPrivateDataEXT(dispatch, device, objectType, objectHandle, privateDataSlot, data);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_SignalSemaphore(VkDevice device, const VkSemaphoreSignalInfo *pSignalInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::SignalSemaphore(dispatch, device, pSignalInfo);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_SignalSemaphoreKHR(VkDevice device, const VkSemaphoreSignalInfo *pSignalInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::SignalSemaphoreKHR(dispatch, device, pSignalInfo);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_TrimCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolTrimFlags flags) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::TrimCommandPool(dispatch, device, commandPool, flags);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_TrimCommandPoolKHR(VkDevice device, VkCommandPool commandPool, VkCommandPoolTrimFlags flags) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::TrimCommandPoolKHR(dispatch, device, commandPool, flags);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_UninitializePerformanceApiINTEL(VkDevice device) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::UninitializePerformanceApiINTEL(dispatch, device);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_UnmapMemory(VkDevice device, VkDeviceMemory memory) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::UnmapMemory(dispatch, device, memory);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_UpdateDescriptorSetWithTemplate(VkDevice device, VkDescriptorSet descriptorSet, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const void *pData) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::UpdateDescriptorSetWithTemplate(dispatch, device, descriptorSet, descriptorUpdateTemplate, pData);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_UpdateDescriptorSetWithTemplateKHR(VkDevice device, VkDescriptorSet descriptorSet, VkDescriptorUpdateTemplate descriptorUpdateTemplate, const void *pData) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::UpdateDescriptorSetWithTemplateKHR(dispatch, device, descriptorSet, descriptorUpdateTemplate, pData);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void wrap_UpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet *pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet *pDescriptorCopies) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    DeviceOverrides::UpdateDescriptorSets(dispatch, device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
  }

#ifdef VK_ENABLE_BETA_EXTENSIONS
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_UpdateVideoSessionParametersKHR(VkDevice device, VkVideoSessionParametersKHR videoSessionParameters, const VkVideoSessionParametersUpdateInfoKHR *pUpdateInfo) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::UpdateVideoSessionParametersKHR(dispatch, device, videoSessionParameters, pUpdateInfo);
    return ret;
  }

#endif
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_WaitForFences(VkDevice device, uint32_t fenceCount, const VkFence *pFences, VkBool32 waitAll, uint64_t timeout) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::WaitForFences(dispatch, device, fenceCount, pFences, waitAll, timeout);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_WaitForPresentKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t presentId, uint64_t timeout) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::WaitForPresentKHR(dispatch, device, swapchain, presentId, timeout);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_WaitSemaphores(VkDevice device, const VkSemaphoreWaitInfo *pWaitInfo, uint64_t timeout) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::WaitSemaphores(dispatch, device, pWaitInfo, timeout);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_WaitSemaphoresKHR(VkDevice device, const VkSemaphoreWaitInfo *pWaitInfo, uint64_t timeout) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::WaitSemaphoresKHR(dispatch, device, pWaitInfo, timeout);
    return ret;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult wrap_WriteAccelerationStructuresPropertiesKHR(VkDevice device, uint32_t accelerationStructureCount, const VkAccelerationStructureKHR *pAccelerationStructures, VkQueryType queryType, size_t dataSize, void *pData, size_t stride) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    VkResult ret = DeviceOverrides::WriteAccelerationStructuresPropertiesKHR(dispatch, device, accelerationStructureCount, pAccelerationStructures, queryType, dataSize, pData, stride);
    return ret;
  }


  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void implicit_wrap_DestroyDevice(
          VkDevice               device,
    const VkAllocationCallbacks* pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    // Implemented in the Dispatch class, goes to DestroyDeviceWrapper.
    // Make sure we call ours here.
    dispatch->DestroyDevice(device, pAllocator);
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static PFN_vkVoidFunction GetInstanceProcAddr(VkInstance instance, const char* name) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(instance);
#ifdef VK_USE_PLATFORM_ANDROID_KHR
    constexpr bool HasCreateAndroidSurfaceKHR = requires(const InstanceOverrides& t) { &InstanceOverrides::CreateAndroidSurfaceKHR; };
    if constexpr (HasCreateAndroidSurfaceKHR) {
      if (!std::strcmp("vkCreateAndroidSurfaceKHR", name))
        return (PFN_vkVoidFunction) &wrap_CreateAndroidSurfaceKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasCreateDebugReportCallbackEXT = requires(const InstanceOverrides& t) { &InstanceOverrides::CreateDebugReportCallbackEXT; };
    if constexpr (HasCreateDebugReportCallbackEXT) {
      if (!std::strcmp("vkCreateDebugReportCallbackEXT", name))
        return (PFN_vkVoidFunction) &wrap_CreateDebugReportCallbackEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateDebugUtilsMessengerEXT = requires(const InstanceOverrides& t) { &InstanceOverrides::CreateDebugUtilsMessengerEXT; };
    if constexpr (HasCreateDebugUtilsMessengerEXT) {
      if (!std::strcmp("vkCreateDebugUtilsMessengerEXT", name))
        return (PFN_vkVoidFunction) &wrap_CreateDebugUtilsMessengerEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateDevice = requires(const InstanceOverrides& t) { &InstanceOverrides::CreateDevice; };
    if constexpr (HasCreateDevice) {
      if (!std::strcmp("vkCreateDevice", name))
        return (PFN_vkVoidFunction) &wrap_CreateDevice<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
    else {
      if (!std::is_base_of<NoOverrides, DeviceOverrides>::value && !std::strcmp("vkCreateDevice", name))
        return (PFN_vkVoidFunction) &implicit_wrap_CreateDevice<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VK_USE_PLATFORM_DIRECTFB_EXT
    constexpr bool HasCreateDirectFBSurfaceEXT = requires(const InstanceOverrides& t) { &InstanceOverrides::CreateDirectFBSurfaceEXT; };
    if constexpr (HasCreateDirectFBSurfaceEXT) {
      if (!std::strcmp("vkCreateDirectFBSurfaceEXT", name))
        return (PFN_vkVoidFunction) &wrap_CreateDirectFBSurfaceEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasCreateDisplayModeKHR = requires(const InstanceOverrides& t) { &InstanceOverrides::CreateDisplayModeKHR; };
    if constexpr (HasCreateDisplayModeKHR) {
      if (!std::strcmp("vkCreateDisplayModeKHR", name))
        return (PFN_vkVoidFunction) &wrap_CreateDisplayModeKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateDisplayPlaneSurfaceKHR = requires(const InstanceOverrides& t) { &InstanceOverrides::CreateDisplayPlaneSurfaceKHR; };
    if constexpr (HasCreateDisplayPlaneSurfaceKHR) {
      if (!std::strcmp("vkCreateDisplayPlaneSurfaceKHR", name))
        return (PFN_vkVoidFunction) &wrap_CreateDisplayPlaneSurfaceKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateHeadlessSurfaceEXT = requires(const InstanceOverrides& t) { &InstanceOverrides::CreateHeadlessSurfaceEXT; };
    if constexpr (HasCreateHeadlessSurfaceEXT) {
      if (!std::strcmp("vkCreateHeadlessSurfaceEXT", name))
        return (PFN_vkVoidFunction) &wrap_CreateHeadlessSurfaceEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VK_USE_PLATFORM_IOS_MVK
    constexpr bool HasCreateIOSSurfaceMVK = requires(const InstanceOverrides& t) { &InstanceOverrides::CreateIOSSurfaceMVK; };
    if constexpr (HasCreateIOSSurfaceMVK) {
      if (!std::strcmp("vkCreateIOSSurfaceMVK", name))
        return (PFN_vkVoidFunction) &wrap_CreateIOSSurfaceMVK<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

#ifdef VK_USE_PLATFORM_FUCHSIA
    constexpr bool HasCreateImagePipeSurfaceFUCHSIA = requires(const InstanceOverrides& t) { &InstanceOverrides::CreateImagePipeSurfaceFUCHSIA; };
    if constexpr (HasCreateImagePipeSurfaceFUCHSIA) {
      if (!std::strcmp("vkCreateImagePipeSurfaceFUCHSIA", name))
        return (PFN_vkVoidFunction) &wrap_CreateImagePipeSurfaceFUCHSIA<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasCreateInstance = requires(const InstanceOverrides& t) { &InstanceOverrides::CreateInstance; };
    if constexpr (HasCreateInstance) {
      if (!std::strcmp("vkCreateInstance", name))
        return (PFN_vkVoidFunction) &wrap_CreateInstance<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
    else {
      if (!std::strcmp("vkCreateInstance", name))
        return (PFN_vkVoidFunction) &implicit_wrap_CreateInstance<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VK_USE_PLATFORM_MACOS_MVK
    constexpr bool HasCreateMacOSSurfaceMVK = requires(const InstanceOverrides& t) { &InstanceOverrides::CreateMacOSSurfaceMVK; };
    if constexpr (HasCreateMacOSSurfaceMVK) {
      if (!std::strcmp("vkCreateMacOSSurfaceMVK", name))
        return (PFN_vkVoidFunction) &wrap_CreateMacOSSurfaceMVK<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

#ifdef VK_USE_PLATFORM_METAL_EXT
    constexpr bool HasCreateMetalSurfaceEXT = requires(const InstanceOverrides& t) { &InstanceOverrides::CreateMetalSurfaceEXT; };
    if constexpr (HasCreateMetalSurfaceEXT) {
      if (!std::strcmp("vkCreateMetalSurfaceEXT", name))
        return (PFN_vkVoidFunction) &wrap_CreateMetalSurfaceEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

#ifdef VK_USE_PLATFORM_SCREEN_QNX
    constexpr bool HasCreateScreenSurfaceQNX = requires(const InstanceOverrides& t) { &InstanceOverrides::CreateScreenSurfaceQNX; };
    if constexpr (HasCreateScreenSurfaceQNX) {
      if (!std::strcmp("vkCreateScreenSurfaceQNX", name))
        return (PFN_vkVoidFunction) &wrap_CreateScreenSurfaceQNX<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

#ifdef VK_USE_PLATFORM_GGP
    constexpr bool HasCreateStreamDescriptorSurfaceGGP = requires(const InstanceOverrides& t) { &InstanceOverrides::CreateStreamDescriptorSurfaceGGP; };
    if constexpr (HasCreateStreamDescriptorSurfaceGGP) {
      if (!std::strcmp("vkCreateStreamDescriptorSurfaceGGP", name))
        return (PFN_vkVoidFunction) &wrap_CreateStreamDescriptorSurfaceGGP<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

#ifdef VK_USE_PLATFORM_VI_NN
    constexpr bool HasCreateViSurfaceNN = requires(const InstanceOverrides& t) { &InstanceOverrides::CreateViSurfaceNN; };
    if constexpr (HasCreateViSurfaceNN) {
      if (!std::strcmp("vkCreateViSurfaceNN", name))
        return (PFN_vkVoidFunction) &wrap_CreateViSurfaceNN<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    constexpr bool HasCreateWaylandSurfaceKHR = requires(const InstanceOverrides& t) { &InstanceOverrides::CreateWaylandSurfaceKHR; };
    if constexpr (HasCreateWaylandSurfaceKHR) {
      if (!std::strcmp("vkCreateWaylandSurfaceKHR", name))
        return (PFN_vkVoidFunction) &wrap_CreateWaylandSurfaceKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

#ifdef VK_USE_PLATFORM_WIN32_KHR
    constexpr bool HasCreateWin32SurfaceKHR = requires(const InstanceOverrides& t) { &InstanceOverrides::CreateWin32SurfaceKHR; };
    if constexpr (HasCreateWin32SurfaceKHR) {
      if (!std::strcmp("vkCreateWin32SurfaceKHR", name))
        return (PFN_vkVoidFunction) &wrap_CreateWin32SurfaceKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

#ifdef VK_USE_PLATFORM_XCB_KHR
    constexpr bool HasCreateXcbSurfaceKHR = requires(const InstanceOverrides& t) { &InstanceOverrides::CreateXcbSurfaceKHR; };
    if constexpr (HasCreateXcbSurfaceKHR) {
      if (!std::strcmp("vkCreateXcbSurfaceKHR", name))
        return (PFN_vkVoidFunction) &wrap_CreateXcbSurfaceKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

#ifdef VK_USE_PLATFORM_XLIB_KHR
    constexpr bool HasCreateXlibSurfaceKHR = requires(const InstanceOverrides& t) { &InstanceOverrides::CreateXlibSurfaceKHR; };
    if constexpr (HasCreateXlibSurfaceKHR) {
      if (!std::strcmp("vkCreateXlibSurfaceKHR", name))
        return (PFN_vkVoidFunction) &wrap_CreateXlibSurfaceKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasDebugReportMessageEXT = requires(const InstanceOverrides& t) { &InstanceOverrides::DebugReportMessageEXT; };
    if constexpr (HasDebugReportMessageEXT) {
      if (!std::strcmp("vkDebugReportMessageEXT", name))
        return (PFN_vkVoidFunction) &wrap_DebugReportMessageEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDestroyDebugReportCallbackEXT = requires(const InstanceOverrides& t) { &InstanceOverrides::DestroyDebugReportCallbackEXT; };
    if constexpr (HasDestroyDebugReportCallbackEXT) {
      if (!std::strcmp("vkDestroyDebugReportCallbackEXT", name))
        return (PFN_vkVoidFunction) &wrap_DestroyDebugReportCallbackEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDestroyDebugUtilsMessengerEXT = requires(const InstanceOverrides& t) { &InstanceOverrides::DestroyDebugUtilsMessengerEXT; };
    if constexpr (HasDestroyDebugUtilsMessengerEXT) {
      if (!std::strcmp("vkDestroyDebugUtilsMessengerEXT", name))
        return (PFN_vkVoidFunction) &wrap_DestroyDebugUtilsMessengerEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDestroyInstance = requires(const InstanceOverrides& t) { &InstanceOverrides::DestroyInstance; };
    if constexpr (HasDestroyInstance) {
      if (!std::strcmp("vkDestroyInstance", name))
        return (PFN_vkVoidFunction) &wrap_DestroyInstance<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
    else {
      if (!std::strcmp("vkDestroyInstance", name))
        return (PFN_vkVoidFunction) &implicit_wrap_DestroyInstance<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDestroySurfaceKHR = requires(const InstanceOverrides& t) { &InstanceOverrides::DestroySurfaceKHR; };
    if constexpr (HasDestroySurfaceKHR) {
      if (!std::strcmp("vkDestroySurfaceKHR", name))
        return (PFN_vkVoidFunction) &wrap_DestroySurfaceKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasEnumerateDeviceExtensionProperties = requires(const InstanceOverrides& t) { &InstanceOverrides::EnumerateDeviceExtensionProperties; };
    if constexpr (HasEnumerateDeviceExtensionProperties) {
      if (!std::strcmp("vkEnumerateDeviceExtensionProperties", name))
        return (PFN_vkVoidFunction) &wrap_EnumerateDeviceExtensionProperties<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasEnumerateDeviceLayerProperties = requires(const InstanceOverrides& t) { &InstanceOverrides::EnumerateDeviceLayerProperties; };
    if constexpr (HasEnumerateDeviceLayerProperties) {
      if (!std::strcmp("vkEnumerateDeviceLayerProperties", name))
        return (PFN_vkVoidFunction) &wrap_EnumerateDeviceLayerProperties<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasEnumeratePhysicalDeviceGroups = requires(const InstanceOverrides& t) { &InstanceOverrides::EnumeratePhysicalDeviceGroups; };
    if constexpr (HasEnumeratePhysicalDeviceGroups) {
      if (!std::strcmp("vkEnumeratePhysicalDeviceGroups", name))
        return (PFN_vkVoidFunction) &wrap_EnumeratePhysicalDeviceGroups<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasEnumeratePhysicalDeviceGroupsKHR = requires(const InstanceOverrides& t) { &InstanceOverrides::EnumeratePhysicalDeviceGroupsKHR; };
    if constexpr (HasEnumeratePhysicalDeviceGroupsKHR) {
      if (!std::strcmp("vkEnumeratePhysicalDeviceGroupsKHR", name))
        return (PFN_vkVoidFunction) &wrap_EnumeratePhysicalDeviceGroupsKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasEnumeratePhysicalDevices = requires(const InstanceOverrides& t) { &InstanceOverrides::EnumeratePhysicalDevices; };
    if constexpr (HasEnumeratePhysicalDevices) {
      if (!std::strcmp("vkEnumeratePhysicalDevices", name))
        return (PFN_vkVoidFunction) &wrap_EnumeratePhysicalDevices<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetDisplayModePropertiesKHR = requires(const InstanceOverrides& t) { &InstanceOverrides::GetDisplayModePropertiesKHR; };
    if constexpr (HasGetDisplayModePropertiesKHR) {
      if (!std::strcmp("vkGetDisplayModePropertiesKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetDisplayModePropertiesKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetDisplayPlaneCapabilitiesKHR = requires(const InstanceOverrides& t) { &InstanceOverrides::GetDisplayPlaneCapabilitiesKHR; };
    if constexpr (HasGetDisplayPlaneCapabilitiesKHR) {
      if (!std::strcmp("vkGetDisplayPlaneCapabilitiesKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetDisplayPlaneCapabilitiesKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetDisplayPlaneSupportedDisplaysKHR = requires(const InstanceOverrides& t) { &InstanceOverrides::GetDisplayPlaneSupportedDisplaysKHR; };
    if constexpr (HasGetDisplayPlaneSupportedDisplaysKHR) {
      if (!std::strcmp("vkGetDisplayPlaneSupportedDisplaysKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetDisplayPlaneSupportedDisplaysKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    if (!std::strcmp("vkGetInstanceProcAddr", name))
      return (PFN_vkVoidFunction) &GetInstanceProcAddr<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;

    constexpr bool HasGetPhysicalDeviceDisplayPlanePropertiesKHR = requires(const InstanceOverrides& t) { &InstanceOverrides::GetPhysicalDeviceDisplayPlanePropertiesKHR; };
    if constexpr (HasGetPhysicalDeviceDisplayPlanePropertiesKHR) {
      if (!std::strcmp("vkGetPhysicalDeviceDisplayPlanePropertiesKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceDisplayPlanePropertiesKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceDisplayPropertiesKHR = requires(const InstanceOverrides& t) { &InstanceOverrides::GetPhysicalDeviceDisplayPropertiesKHR; };
    if constexpr (HasGetPhysicalDeviceDisplayPropertiesKHR) {
      if (!std::strcmp("vkGetPhysicalDeviceDisplayPropertiesKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceDisplayPropertiesKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceExternalBufferProperties = requires(const InstanceOverrides& t) { &InstanceOverrides::GetPhysicalDeviceExternalBufferProperties; };
    if constexpr (HasGetPhysicalDeviceExternalBufferProperties) {
      if (!std::strcmp("vkGetPhysicalDeviceExternalBufferProperties", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceExternalBufferProperties<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceExternalFenceProperties = requires(const InstanceOverrides& t) { &InstanceOverrides::GetPhysicalDeviceExternalFenceProperties; };
    if constexpr (HasGetPhysicalDeviceExternalFenceProperties) {
      if (!std::strcmp("vkGetPhysicalDeviceExternalFenceProperties", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceExternalFenceProperties<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceExternalSemaphoreProperties = requires(const InstanceOverrides& t) { &InstanceOverrides::GetPhysicalDeviceExternalSemaphoreProperties; };
    if constexpr (HasGetPhysicalDeviceExternalSemaphoreProperties) {
      if (!std::strcmp("vkGetPhysicalDeviceExternalSemaphoreProperties", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceExternalSemaphoreProperties<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceFeatures = requires(const InstanceOverrides& t) { &InstanceOverrides::GetPhysicalDeviceFeatures; };
    if constexpr (HasGetPhysicalDeviceFeatures) {
      if (!std::strcmp("vkGetPhysicalDeviceFeatures", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceFeatures<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceFeatures2 = requires(const InstanceOverrides& t) { &InstanceOverrides::GetPhysicalDeviceFeatures2; };
    if constexpr (HasGetPhysicalDeviceFeatures2) {
      if (!std::strcmp("vkGetPhysicalDeviceFeatures2", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceFeatures2<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceFormatProperties = requires(const InstanceOverrides& t) { &InstanceOverrides::GetPhysicalDeviceFormatProperties; };
    if constexpr (HasGetPhysicalDeviceFormatProperties) {
      if (!std::strcmp("vkGetPhysicalDeviceFormatProperties", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceFormatProperties<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceFormatProperties2 = requires(const InstanceOverrides& t) { &InstanceOverrides::GetPhysicalDeviceFormatProperties2; };
    if constexpr (HasGetPhysicalDeviceFormatProperties2) {
      if (!std::strcmp("vkGetPhysicalDeviceFormatProperties2", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceFormatProperties2<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceImageFormatProperties = requires(const InstanceOverrides& t) { &InstanceOverrides::GetPhysicalDeviceImageFormatProperties; };
    if constexpr (HasGetPhysicalDeviceImageFormatProperties) {
      if (!std::strcmp("vkGetPhysicalDeviceImageFormatProperties", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceImageFormatProperties<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceImageFormatProperties2 = requires(const InstanceOverrides& t) { &InstanceOverrides::GetPhysicalDeviceImageFormatProperties2; };
    if constexpr (HasGetPhysicalDeviceImageFormatProperties2) {
      if (!std::strcmp("vkGetPhysicalDeviceImageFormatProperties2", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceImageFormatProperties2<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceMemoryProperties = requires(const InstanceOverrides& t) { &InstanceOverrides::GetPhysicalDeviceMemoryProperties; };
    if constexpr (HasGetPhysicalDeviceMemoryProperties) {
      if (!std::strcmp("vkGetPhysicalDeviceMemoryProperties", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceMemoryProperties<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceMemoryProperties2 = requires(const InstanceOverrides& t) { &InstanceOverrides::GetPhysicalDeviceMemoryProperties2; };
    if constexpr (HasGetPhysicalDeviceMemoryProperties2) {
      if (!std::strcmp("vkGetPhysicalDeviceMemoryProperties2", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceMemoryProperties2<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDevicePresentRectanglesKHR = requires(const InstanceOverrides& t) { &InstanceOverrides::GetPhysicalDevicePresentRectanglesKHR; };
    if constexpr (HasGetPhysicalDevicePresentRectanglesKHR) {
      if (!std::strcmp("vkGetPhysicalDevicePresentRectanglesKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDevicePresentRectanglesKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceProperties = requires(const InstanceOverrides& t) { &InstanceOverrides::GetPhysicalDeviceProperties; };
    if constexpr (HasGetPhysicalDeviceProperties) {
      if (!std::strcmp("vkGetPhysicalDeviceProperties", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceProperties<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceProperties2 = requires(const InstanceOverrides& t) { &InstanceOverrides::GetPhysicalDeviceProperties2; };
    if constexpr (HasGetPhysicalDeviceProperties2) {
      if (!std::strcmp("vkGetPhysicalDeviceProperties2", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceProperties2<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceQueueFamilyProperties = requires(const InstanceOverrides& t) { &InstanceOverrides::GetPhysicalDeviceQueueFamilyProperties; };
    if constexpr (HasGetPhysicalDeviceQueueFamilyProperties) {
      if (!std::strcmp("vkGetPhysicalDeviceQueueFamilyProperties", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceQueueFamilyProperties<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceQueueFamilyProperties2 = requires(const InstanceOverrides& t) { &InstanceOverrides::GetPhysicalDeviceQueueFamilyProperties2; };
    if constexpr (HasGetPhysicalDeviceQueueFamilyProperties2) {
      if (!std::strcmp("vkGetPhysicalDeviceQueueFamilyProperties2", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceQueueFamilyProperties2<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceSparseImageFormatProperties = requires(const InstanceOverrides& t) { &InstanceOverrides::GetPhysicalDeviceSparseImageFormatProperties; };
    if constexpr (HasGetPhysicalDeviceSparseImageFormatProperties) {
      if (!std::strcmp("vkGetPhysicalDeviceSparseImageFormatProperties", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceSparseImageFormatProperties<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceSparseImageFormatProperties2 = requires(const InstanceOverrides& t) { &InstanceOverrides::GetPhysicalDeviceSparseImageFormatProperties2; };
    if constexpr (HasGetPhysicalDeviceSparseImageFormatProperties2) {
      if (!std::strcmp("vkGetPhysicalDeviceSparseImageFormatProperties2", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceSparseImageFormatProperties2<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceSurfaceCapabilities2KHR = requires(const InstanceOverrides& t) { &InstanceOverrides::GetPhysicalDeviceSurfaceCapabilities2KHR; };
    if constexpr (HasGetPhysicalDeviceSurfaceCapabilities2KHR) {
      if (!std::strcmp("vkGetPhysicalDeviceSurfaceCapabilities2KHR", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceSurfaceCapabilities2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceSurfaceCapabilitiesKHR = requires(const InstanceOverrides& t) { &InstanceOverrides::GetPhysicalDeviceSurfaceCapabilitiesKHR; };
    if constexpr (HasGetPhysicalDeviceSurfaceCapabilitiesKHR) {
      if (!std::strcmp("vkGetPhysicalDeviceSurfaceCapabilitiesKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceSurfaceCapabilitiesKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceSurfaceFormats2KHR = requires(const InstanceOverrides& t) { &InstanceOverrides::GetPhysicalDeviceSurfaceFormats2KHR; };
    if constexpr (HasGetPhysicalDeviceSurfaceFormats2KHR) {
      if (!std::strcmp("vkGetPhysicalDeviceSurfaceFormats2KHR", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceSurfaceFormats2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceSurfaceFormatsKHR = requires(const InstanceOverrides& t) { &InstanceOverrides::GetPhysicalDeviceSurfaceFormatsKHR; };
    if constexpr (HasGetPhysicalDeviceSurfaceFormatsKHR) {
      if (!std::strcmp("vkGetPhysicalDeviceSurfaceFormatsKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceSurfaceFormatsKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceSurfacePresentModesKHR = requires(const InstanceOverrides& t) { &InstanceOverrides::GetPhysicalDeviceSurfacePresentModesKHR; };
    if constexpr (HasGetPhysicalDeviceSurfacePresentModesKHR) {
      if (!std::strcmp("vkGetPhysicalDeviceSurfacePresentModesKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceSurfacePresentModesKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceSurfaceSupportKHR = requires(const InstanceOverrides& t) { &InstanceOverrides::GetPhysicalDeviceSurfaceSupportKHR; };
    if constexpr (HasGetPhysicalDeviceSurfaceSupportKHR) {
      if (!std::strcmp("vkGetPhysicalDeviceSurfaceSupportKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceSurfaceSupportKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceToolProperties = requires(const InstanceOverrides& t) { &InstanceOverrides::GetPhysicalDeviceToolProperties; };
    if constexpr (HasGetPhysicalDeviceToolProperties) {
      if (!std::strcmp("vkGetPhysicalDeviceToolProperties", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceToolProperties<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    constexpr bool HasGetPhysicalDeviceWaylandPresentationSupportKHR = requires(const InstanceOverrides& t) { &InstanceOverrides::GetPhysicalDeviceWaylandPresentationSupportKHR; };
    if constexpr (HasGetPhysicalDeviceWaylandPresentationSupportKHR) {
      if (!std::strcmp("vkGetPhysicalDeviceWaylandPresentationSupportKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceWaylandPresentationSupportKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

#ifdef VK_USE_PLATFORM_WIN32_KHR
    constexpr bool HasGetPhysicalDeviceWin32PresentationSupportKHR = requires(const InstanceOverrides& t) { &InstanceOverrides::GetPhysicalDeviceWin32PresentationSupportKHR; };
    if constexpr (HasGetPhysicalDeviceWin32PresentationSupportKHR) {
      if (!std::strcmp("vkGetPhysicalDeviceWin32PresentationSupportKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceWin32PresentationSupportKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

#ifdef VK_USE_PLATFORM_XCB_KHR
    constexpr bool HasGetPhysicalDeviceXcbPresentationSupportKHR = requires(const InstanceOverrides& t) { &InstanceOverrides::GetPhysicalDeviceXcbPresentationSupportKHR; };
    if constexpr (HasGetPhysicalDeviceXcbPresentationSupportKHR) {
      if (!std::strcmp("vkGetPhysicalDeviceXcbPresentationSupportKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceXcbPresentationSupportKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

#ifdef VK_USE_PLATFORM_XLIB_KHR
    constexpr bool HasGetPhysicalDeviceXlibPresentationSupportKHR = requires(const InstanceOverrides& t) { &InstanceOverrides::GetPhysicalDeviceXlibPresentationSupportKHR; };
    if constexpr (HasGetPhysicalDeviceXlibPresentationSupportKHR) {
      if (!std::strcmp("vkGetPhysicalDeviceXlibPresentationSupportKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceXlibPresentationSupportKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasSubmitDebugUtilsMessageEXT = requires(const InstanceOverrides& t) { &InstanceOverrides::SubmitDebugUtilsMessageEXT; };
    if constexpr (HasSubmitDebugUtilsMessageEXT) {
      if (!std::strcmp("vkSubmitDebugUtilsMessageEXT", name))
        return (PFN_vkVoidFunction) &wrap_SubmitDebugUtilsMessageEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    if (dispatch)
      return dispatch->GetInstanceProcAddr(instance, name);
    else
      return NULL;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static PFN_vkVoidFunction GetPhysicalDeviceProcAddr(VkInstance instance, const char* name) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(instance);
    constexpr bool HasAcquireDrmDisplayEXT = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::AcquireDrmDisplayEXT; };
    if constexpr (HasAcquireDrmDisplayEXT) {
      if (!std::strcmp("vkAcquireDrmDisplayEXT", name))
        return (PFN_vkVoidFunction) &wrap_AcquireDrmDisplayEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VK_USE_PLATFORM_WIN32_KHR
    constexpr bool HasAcquireWinrtDisplayNV = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::AcquireWinrtDisplayNV; };
    if constexpr (HasAcquireWinrtDisplayNV) {
      if (!std::strcmp("vkAcquireWinrtDisplayNV", name))
        return (PFN_vkVoidFunction) &wrap_AcquireWinrtDisplayNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

#ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
    constexpr bool HasAcquireXlibDisplayEXT = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::AcquireXlibDisplayEXT; };
    if constexpr (HasAcquireXlibDisplayEXT) {
      if (!std::strcmp("vkAcquireXlibDisplayEXT", name))
        return (PFN_vkVoidFunction) &wrap_AcquireXlibDisplayEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::EnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR; };
    if constexpr (HasEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR) {
      if (!std::strcmp("vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR", name))
        return (PFN_vkVoidFunction) &wrap_EnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetDisplayModeProperties2KHR = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::GetDisplayModeProperties2KHR; };
    if constexpr (HasGetDisplayModeProperties2KHR) {
      if (!std::strcmp("vkGetDisplayModeProperties2KHR", name))
        return (PFN_vkVoidFunction) &wrap_GetDisplayModeProperties2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetDisplayPlaneCapabilities2KHR = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::GetDisplayPlaneCapabilities2KHR; };
    if constexpr (HasGetDisplayPlaneCapabilities2KHR) {
      if (!std::strcmp("vkGetDisplayPlaneCapabilities2KHR", name))
        return (PFN_vkVoidFunction) &wrap_GetDisplayPlaneCapabilities2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetDrmDisplayEXT = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::GetDrmDisplayEXT; };
    if constexpr (HasGetDrmDisplayEXT) {
      if (!std::strcmp("vkGetDrmDisplayEXT", name))
        return (PFN_vkVoidFunction) &wrap_GetDrmDisplayEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceCalibrateableTimeDomainsEXT = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::GetPhysicalDeviceCalibrateableTimeDomainsEXT; };
    if constexpr (HasGetPhysicalDeviceCalibrateableTimeDomainsEXT) {
      if (!std::strcmp("vkGetPhysicalDeviceCalibrateableTimeDomainsEXT", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceCalibrateableTimeDomainsEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceCooperativeMatrixPropertiesNV = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::GetPhysicalDeviceCooperativeMatrixPropertiesNV; };
    if constexpr (HasGetPhysicalDeviceCooperativeMatrixPropertiesNV) {
      if (!std::strcmp("vkGetPhysicalDeviceCooperativeMatrixPropertiesNV", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceCooperativeMatrixPropertiesNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VK_USE_PLATFORM_DIRECTFB_EXT
    constexpr bool HasGetPhysicalDeviceDirectFBPresentationSupportEXT = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::GetPhysicalDeviceDirectFBPresentationSupportEXT; };
    if constexpr (HasGetPhysicalDeviceDirectFBPresentationSupportEXT) {
      if (!std::strcmp("vkGetPhysicalDeviceDirectFBPresentationSupportEXT", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceDirectFBPresentationSupportEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasGetPhysicalDeviceDisplayPlaneProperties2KHR = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::GetPhysicalDeviceDisplayPlaneProperties2KHR; };
    if constexpr (HasGetPhysicalDeviceDisplayPlaneProperties2KHR) {
      if (!std::strcmp("vkGetPhysicalDeviceDisplayPlaneProperties2KHR", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceDisplayPlaneProperties2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceDisplayProperties2KHR = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::GetPhysicalDeviceDisplayProperties2KHR; };
    if constexpr (HasGetPhysicalDeviceDisplayProperties2KHR) {
      if (!std::strcmp("vkGetPhysicalDeviceDisplayProperties2KHR", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceDisplayProperties2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceExternalBufferPropertiesKHR = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::GetPhysicalDeviceExternalBufferPropertiesKHR; };
    if constexpr (HasGetPhysicalDeviceExternalBufferPropertiesKHR) {
      if (!std::strcmp("vkGetPhysicalDeviceExternalBufferPropertiesKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceExternalBufferPropertiesKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceExternalFencePropertiesKHR = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::GetPhysicalDeviceExternalFencePropertiesKHR; };
    if constexpr (HasGetPhysicalDeviceExternalFencePropertiesKHR) {
      if (!std::strcmp("vkGetPhysicalDeviceExternalFencePropertiesKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceExternalFencePropertiesKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceExternalImageFormatPropertiesNV = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::GetPhysicalDeviceExternalImageFormatPropertiesNV; };
    if constexpr (HasGetPhysicalDeviceExternalImageFormatPropertiesNV) {
      if (!std::strcmp("vkGetPhysicalDeviceExternalImageFormatPropertiesNV", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceExternalImageFormatPropertiesNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceExternalSemaphorePropertiesKHR = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::GetPhysicalDeviceExternalSemaphorePropertiesKHR; };
    if constexpr (HasGetPhysicalDeviceExternalSemaphorePropertiesKHR) {
      if (!std::strcmp("vkGetPhysicalDeviceExternalSemaphorePropertiesKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceExternalSemaphorePropertiesKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceFeatures2KHR = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::GetPhysicalDeviceFeatures2KHR; };
    if constexpr (HasGetPhysicalDeviceFeatures2KHR) {
      if (!std::strcmp("vkGetPhysicalDeviceFeatures2KHR", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceFeatures2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceFormatProperties2KHR = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::GetPhysicalDeviceFormatProperties2KHR; };
    if constexpr (HasGetPhysicalDeviceFormatProperties2KHR) {
      if (!std::strcmp("vkGetPhysicalDeviceFormatProperties2KHR", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceFormatProperties2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceFragmentShadingRatesKHR = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::GetPhysicalDeviceFragmentShadingRatesKHR; };
    if constexpr (HasGetPhysicalDeviceFragmentShadingRatesKHR) {
      if (!std::strcmp("vkGetPhysicalDeviceFragmentShadingRatesKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceFragmentShadingRatesKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceImageFormatProperties2KHR = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::GetPhysicalDeviceImageFormatProperties2KHR; };
    if constexpr (HasGetPhysicalDeviceImageFormatProperties2KHR) {
      if (!std::strcmp("vkGetPhysicalDeviceImageFormatProperties2KHR", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceImageFormatProperties2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceMemoryProperties2KHR = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::GetPhysicalDeviceMemoryProperties2KHR; };
    if constexpr (HasGetPhysicalDeviceMemoryProperties2KHR) {
      if (!std::strcmp("vkGetPhysicalDeviceMemoryProperties2KHR", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceMemoryProperties2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceMultisamplePropertiesEXT = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::GetPhysicalDeviceMultisamplePropertiesEXT; };
    if constexpr (HasGetPhysicalDeviceMultisamplePropertiesEXT) {
      if (!std::strcmp("vkGetPhysicalDeviceMultisamplePropertiesEXT", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceMultisamplePropertiesEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceProperties2KHR = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::GetPhysicalDeviceProperties2KHR; };
    if constexpr (HasGetPhysicalDeviceProperties2KHR) {
      if (!std::strcmp("vkGetPhysicalDeviceProperties2KHR", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceProperties2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::GetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR; };
    if constexpr (HasGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR) {
      if (!std::strcmp("vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceQueueFamilyProperties2KHR = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::GetPhysicalDeviceQueueFamilyProperties2KHR; };
    if constexpr (HasGetPhysicalDeviceQueueFamilyProperties2KHR) {
      if (!std::strcmp("vkGetPhysicalDeviceQueueFamilyProperties2KHR", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceQueueFamilyProperties2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VK_USE_PLATFORM_SCREEN_QNX
    constexpr bool HasGetPhysicalDeviceScreenPresentationSupportQNX = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::GetPhysicalDeviceScreenPresentationSupportQNX; };
    if constexpr (HasGetPhysicalDeviceScreenPresentationSupportQNX) {
      if (!std::strcmp("vkGetPhysicalDeviceScreenPresentationSupportQNX", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceScreenPresentationSupportQNX<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasGetPhysicalDeviceSparseImageFormatProperties2KHR = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::GetPhysicalDeviceSparseImageFormatProperties2KHR; };
    if constexpr (HasGetPhysicalDeviceSparseImageFormatProperties2KHR) {
      if (!std::strcmp("vkGetPhysicalDeviceSparseImageFormatProperties2KHR", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceSparseImageFormatProperties2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::GetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV; };
    if constexpr (HasGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV) {
      if (!std::strcmp("vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPhysicalDeviceSurfaceCapabilities2EXT = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::GetPhysicalDeviceSurfaceCapabilities2EXT; };
    if constexpr (HasGetPhysicalDeviceSurfaceCapabilities2EXT) {
      if (!std::strcmp("vkGetPhysicalDeviceSurfaceCapabilities2EXT", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceSurfaceCapabilities2EXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VK_USE_PLATFORM_WIN32_KHR
    constexpr bool HasGetPhysicalDeviceSurfacePresentModes2EXT = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::GetPhysicalDeviceSurfacePresentModes2EXT; };
    if constexpr (HasGetPhysicalDeviceSurfacePresentModes2EXT) {
      if (!std::strcmp("vkGetPhysicalDeviceSurfacePresentModes2EXT", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceSurfacePresentModes2EXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasGetPhysicalDeviceToolPropertiesEXT = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::GetPhysicalDeviceToolPropertiesEXT; };
    if constexpr (HasGetPhysicalDeviceToolPropertiesEXT) {
      if (!std::strcmp("vkGetPhysicalDeviceToolPropertiesEXT", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceToolPropertiesEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VK_ENABLE_BETA_EXTENSIONS
    constexpr bool HasGetPhysicalDeviceVideoCapabilitiesKHR = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::GetPhysicalDeviceVideoCapabilitiesKHR; };
    if constexpr (HasGetPhysicalDeviceVideoCapabilitiesKHR) {
      if (!std::strcmp("vkGetPhysicalDeviceVideoCapabilitiesKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceVideoCapabilitiesKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

#ifdef VK_ENABLE_BETA_EXTENSIONS
    constexpr bool HasGetPhysicalDeviceVideoFormatPropertiesKHR = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::GetPhysicalDeviceVideoFormatPropertiesKHR; };
    if constexpr (HasGetPhysicalDeviceVideoFormatPropertiesKHR) {
      if (!std::strcmp("vkGetPhysicalDeviceVideoFormatPropertiesKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetPhysicalDeviceVideoFormatPropertiesKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

#ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
    constexpr bool HasGetRandROutputDisplayEXT = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::GetRandROutputDisplayEXT; };
    if constexpr (HasGetRandROutputDisplayEXT) {
      if (!std::strcmp("vkGetRandROutputDisplayEXT", name))
        return (PFN_vkVoidFunction) &wrap_GetRandROutputDisplayEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

#ifdef VK_USE_PLATFORM_WIN32_KHR
    constexpr bool HasGetWinrtDisplayNV = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::GetWinrtDisplayNV; };
    if constexpr (HasGetWinrtDisplayNV) {
      if (!std::strcmp("vkGetWinrtDisplayNV", name))
        return (PFN_vkVoidFunction) &wrap_GetWinrtDisplayNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasReleaseDisplayEXT = requires(const PhysicalDeviceOverrides& t) { &PhysicalDeviceOverrides::ReleaseDisplayEXT; };
    if constexpr (HasReleaseDisplayEXT) {
      if (!std::strcmp("vkReleaseDisplayEXT", name))
        return (PFN_vkVoidFunction) &wrap_ReleaseDisplayEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    if constexpr (!std::is_base_of<NoOverrides, PhysicalDeviceOverrides>::value || !std::is_base_of<NoOverrides, DeviceOverrides>::value) {
      if (!std::strcmp("vk_layerGetPhysicalDeviceProcAddr", name))
        return (PFN_vkVoidFunction) &GetPhysicalDeviceProcAddr<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    if (dispatch)
      return dispatch->GetPhysicalDeviceProcAddr(instance, name);
    else
      return NULL;
  }

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static PFN_vkVoidFunction GetDeviceProcAddr(VkDevice device, const char* name) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
#ifdef VK_USE_PLATFORM_WIN32_KHR
    constexpr bool HasAcquireFullScreenExclusiveModeEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::AcquireFullScreenExclusiveModeEXT; };
    if constexpr (HasAcquireFullScreenExclusiveModeEXT) {
      if (!std::strcmp("vkAcquireFullScreenExclusiveModeEXT", name))
        return (PFN_vkVoidFunction) &wrap_AcquireFullScreenExclusiveModeEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

#ifdef VKROOTS_PLATFORM_UNSUPPORTED
    constexpr bool HasAcquireImageANDROID = requires(const DeviceOverrides& t) { &DeviceOverrides::AcquireImageANDROID; };
    if constexpr (HasAcquireImageANDROID) {
      if (!std::strcmp("vkAcquireImageANDROID", name))
        return (PFN_vkVoidFunction) &wrap_AcquireImageANDROID<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasAcquireNextImage2KHR = requires(const DeviceOverrides& t) { &DeviceOverrides::AcquireNextImage2KHR; };
    if constexpr (HasAcquireNextImage2KHR) {
      if (!std::strcmp("vkAcquireNextImage2KHR", name))
        return (PFN_vkVoidFunction) &wrap_AcquireNextImage2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasAcquireNextImageKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::AcquireNextImageKHR; };
    if constexpr (HasAcquireNextImageKHR) {
      if (!std::strcmp("vkAcquireNextImageKHR", name))
        return (PFN_vkVoidFunction) &wrap_AcquireNextImageKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasAcquirePerformanceConfigurationINTEL = requires(const DeviceOverrides& t) { &DeviceOverrides::AcquirePerformanceConfigurationINTEL; };
    if constexpr (HasAcquirePerformanceConfigurationINTEL) {
      if (!std::strcmp("vkAcquirePerformanceConfigurationINTEL", name))
        return (PFN_vkVoidFunction) &wrap_AcquirePerformanceConfigurationINTEL<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasAcquireProfilingLockKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::AcquireProfilingLockKHR; };
    if constexpr (HasAcquireProfilingLockKHR) {
      if (!std::strcmp("vkAcquireProfilingLockKHR", name))
        return (PFN_vkVoidFunction) &wrap_AcquireProfilingLockKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasAllocateCommandBuffers = requires(const DeviceOverrides& t) { &DeviceOverrides::AllocateCommandBuffers; };
    if constexpr (HasAllocateCommandBuffers) {
      if (!std::strcmp("vkAllocateCommandBuffers", name))
        return (PFN_vkVoidFunction) &wrap_AllocateCommandBuffers<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasAllocateDescriptorSets = requires(const DeviceOverrides& t) { &DeviceOverrides::AllocateDescriptorSets; };
    if constexpr (HasAllocateDescriptorSets) {
      if (!std::strcmp("vkAllocateDescriptorSets", name))
        return (PFN_vkVoidFunction) &wrap_AllocateDescriptorSets<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasAllocateMemory = requires(const DeviceOverrides& t) { &DeviceOverrides::AllocateMemory; };
    if constexpr (HasAllocateMemory) {
      if (!std::strcmp("vkAllocateMemory", name))
        return (PFN_vkVoidFunction) &wrap_AllocateMemory<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasBeginCommandBuffer = requires(const DeviceOverrides& t) { &DeviceOverrides::BeginCommandBuffer; };
    if constexpr (HasBeginCommandBuffer) {
      if (!std::strcmp("vkBeginCommandBuffer", name))
        return (PFN_vkVoidFunction) &wrap_BeginCommandBuffer<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasBindAccelerationStructureMemoryNV = requires(const DeviceOverrides& t) { &DeviceOverrides::BindAccelerationStructureMemoryNV; };
    if constexpr (HasBindAccelerationStructureMemoryNV) {
      if (!std::strcmp("vkBindAccelerationStructureMemoryNV", name))
        return (PFN_vkVoidFunction) &wrap_BindAccelerationStructureMemoryNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasBindBufferMemory = requires(const DeviceOverrides& t) { &DeviceOverrides::BindBufferMemory; };
    if constexpr (HasBindBufferMemory) {
      if (!std::strcmp("vkBindBufferMemory", name))
        return (PFN_vkVoidFunction) &wrap_BindBufferMemory<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasBindBufferMemory2 = requires(const DeviceOverrides& t) { &DeviceOverrides::BindBufferMemory2; };
    if constexpr (HasBindBufferMemory2) {
      if (!std::strcmp("vkBindBufferMemory2", name))
        return (PFN_vkVoidFunction) &wrap_BindBufferMemory2<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasBindBufferMemory2KHR = requires(const DeviceOverrides& t) { &DeviceOverrides::BindBufferMemory2KHR; };
    if constexpr (HasBindBufferMemory2KHR) {
      if (!std::strcmp("vkBindBufferMemory2KHR", name))
        return (PFN_vkVoidFunction) &wrap_BindBufferMemory2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasBindImageMemory = requires(const DeviceOverrides& t) { &DeviceOverrides::BindImageMemory; };
    if constexpr (HasBindImageMemory) {
      if (!std::strcmp("vkBindImageMemory", name))
        return (PFN_vkVoidFunction) &wrap_BindImageMemory<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasBindImageMemory2 = requires(const DeviceOverrides& t) { &DeviceOverrides::BindImageMemory2; };
    if constexpr (HasBindImageMemory2) {
      if (!std::strcmp("vkBindImageMemory2", name))
        return (PFN_vkVoidFunction) &wrap_BindImageMemory2<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasBindImageMemory2KHR = requires(const DeviceOverrides& t) { &DeviceOverrides::BindImageMemory2KHR; };
    if constexpr (HasBindImageMemory2KHR) {
      if (!std::strcmp("vkBindImageMemory2KHR", name))
        return (PFN_vkVoidFunction) &wrap_BindImageMemory2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VK_ENABLE_BETA_EXTENSIONS
    constexpr bool HasBindVideoSessionMemoryKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::BindVideoSessionMemoryKHR; };
    if constexpr (HasBindVideoSessionMemoryKHR) {
      if (!std::strcmp("vkBindVideoSessionMemoryKHR", name))
        return (PFN_vkVoidFunction) &wrap_BindVideoSessionMemoryKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasBuildAccelerationStructuresKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::BuildAccelerationStructuresKHR; };
    if constexpr (HasBuildAccelerationStructuresKHR) {
      if (!std::strcmp("vkBuildAccelerationStructuresKHR", name))
        return (PFN_vkVoidFunction) &wrap_BuildAccelerationStructuresKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdBeginConditionalRenderingEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdBeginConditionalRenderingEXT; };
    if constexpr (HasCmdBeginConditionalRenderingEXT) {
      if (!std::strcmp("vkCmdBeginConditionalRenderingEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdBeginConditionalRenderingEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdBeginDebugUtilsLabelEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdBeginDebugUtilsLabelEXT; };
    if constexpr (HasCmdBeginDebugUtilsLabelEXT) {
      if (!std::strcmp("vkCmdBeginDebugUtilsLabelEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdBeginDebugUtilsLabelEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdBeginQuery = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdBeginQuery; };
    if constexpr (HasCmdBeginQuery) {
      if (!std::strcmp("vkCmdBeginQuery", name))
        return (PFN_vkVoidFunction) &wrap_CmdBeginQuery<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdBeginQueryIndexedEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdBeginQueryIndexedEXT; };
    if constexpr (HasCmdBeginQueryIndexedEXT) {
      if (!std::strcmp("vkCmdBeginQueryIndexedEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdBeginQueryIndexedEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdBeginRenderPass = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdBeginRenderPass; };
    if constexpr (HasCmdBeginRenderPass) {
      if (!std::strcmp("vkCmdBeginRenderPass", name))
        return (PFN_vkVoidFunction) &wrap_CmdBeginRenderPass<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdBeginRenderPass2 = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdBeginRenderPass2; };
    if constexpr (HasCmdBeginRenderPass2) {
      if (!std::strcmp("vkCmdBeginRenderPass2", name))
        return (PFN_vkVoidFunction) &wrap_CmdBeginRenderPass2<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdBeginRenderPass2KHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdBeginRenderPass2KHR; };
    if constexpr (HasCmdBeginRenderPass2KHR) {
      if (!std::strcmp("vkCmdBeginRenderPass2KHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdBeginRenderPass2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdBeginRendering = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdBeginRendering; };
    if constexpr (HasCmdBeginRendering) {
      if (!std::strcmp("vkCmdBeginRendering", name))
        return (PFN_vkVoidFunction) &wrap_CmdBeginRendering<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdBeginRenderingKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdBeginRenderingKHR; };
    if constexpr (HasCmdBeginRenderingKHR) {
      if (!std::strcmp("vkCmdBeginRenderingKHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdBeginRenderingKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdBeginTransformFeedbackEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdBeginTransformFeedbackEXT; };
    if constexpr (HasCmdBeginTransformFeedbackEXT) {
      if (!std::strcmp("vkCmdBeginTransformFeedbackEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdBeginTransformFeedbackEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VK_ENABLE_BETA_EXTENSIONS
    constexpr bool HasCmdBeginVideoCodingKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdBeginVideoCodingKHR; };
    if constexpr (HasCmdBeginVideoCodingKHR) {
      if (!std::strcmp("vkCmdBeginVideoCodingKHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdBeginVideoCodingKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasCmdBindDescriptorSets = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdBindDescriptorSets; };
    if constexpr (HasCmdBindDescriptorSets) {
      if (!std::strcmp("vkCmdBindDescriptorSets", name))
        return (PFN_vkVoidFunction) &wrap_CmdBindDescriptorSets<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdBindIndexBuffer = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdBindIndexBuffer; };
    if constexpr (HasCmdBindIndexBuffer) {
      if (!std::strcmp("vkCmdBindIndexBuffer", name))
        return (PFN_vkVoidFunction) &wrap_CmdBindIndexBuffer<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdBindInvocationMaskHUAWEI = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdBindInvocationMaskHUAWEI; };
    if constexpr (HasCmdBindInvocationMaskHUAWEI) {
      if (!std::strcmp("vkCmdBindInvocationMaskHUAWEI", name))
        return (PFN_vkVoidFunction) &wrap_CmdBindInvocationMaskHUAWEI<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdBindPipeline = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdBindPipeline; };
    if constexpr (HasCmdBindPipeline) {
      if (!std::strcmp("vkCmdBindPipeline", name))
        return (PFN_vkVoidFunction) &wrap_CmdBindPipeline<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdBindPipelineShaderGroupNV = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdBindPipelineShaderGroupNV; };
    if constexpr (HasCmdBindPipelineShaderGroupNV) {
      if (!std::strcmp("vkCmdBindPipelineShaderGroupNV", name))
        return (PFN_vkVoidFunction) &wrap_CmdBindPipelineShaderGroupNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdBindShadingRateImageNV = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdBindShadingRateImageNV; };
    if constexpr (HasCmdBindShadingRateImageNV) {
      if (!std::strcmp("vkCmdBindShadingRateImageNV", name))
        return (PFN_vkVoidFunction) &wrap_CmdBindShadingRateImageNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdBindTransformFeedbackBuffersEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdBindTransformFeedbackBuffersEXT; };
    if constexpr (HasCmdBindTransformFeedbackBuffersEXT) {
      if (!std::strcmp("vkCmdBindTransformFeedbackBuffersEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdBindTransformFeedbackBuffersEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdBindVertexBuffers = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdBindVertexBuffers; };
    if constexpr (HasCmdBindVertexBuffers) {
      if (!std::strcmp("vkCmdBindVertexBuffers", name))
        return (PFN_vkVoidFunction) &wrap_CmdBindVertexBuffers<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdBindVertexBuffers2 = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdBindVertexBuffers2; };
    if constexpr (HasCmdBindVertexBuffers2) {
      if (!std::strcmp("vkCmdBindVertexBuffers2", name))
        return (PFN_vkVoidFunction) &wrap_CmdBindVertexBuffers2<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdBindVertexBuffers2EXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdBindVertexBuffers2EXT; };
    if constexpr (HasCmdBindVertexBuffers2EXT) {
      if (!std::strcmp("vkCmdBindVertexBuffers2EXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdBindVertexBuffers2EXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdBlitImage = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdBlitImage; };
    if constexpr (HasCmdBlitImage) {
      if (!std::strcmp("vkCmdBlitImage", name))
        return (PFN_vkVoidFunction) &wrap_CmdBlitImage<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdBlitImage2 = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdBlitImage2; };
    if constexpr (HasCmdBlitImage2) {
      if (!std::strcmp("vkCmdBlitImage2", name))
        return (PFN_vkVoidFunction) &wrap_CmdBlitImage2<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdBlitImage2KHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdBlitImage2KHR; };
    if constexpr (HasCmdBlitImage2KHR) {
      if (!std::strcmp("vkCmdBlitImage2KHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdBlitImage2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdBuildAccelerationStructureNV = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdBuildAccelerationStructureNV; };
    if constexpr (HasCmdBuildAccelerationStructureNV) {
      if (!std::strcmp("vkCmdBuildAccelerationStructureNV", name))
        return (PFN_vkVoidFunction) &wrap_CmdBuildAccelerationStructureNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdBuildAccelerationStructuresIndirectKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdBuildAccelerationStructuresIndirectKHR; };
    if constexpr (HasCmdBuildAccelerationStructuresIndirectKHR) {
      if (!std::strcmp("vkCmdBuildAccelerationStructuresIndirectKHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdBuildAccelerationStructuresIndirectKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdBuildAccelerationStructuresKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdBuildAccelerationStructuresKHR; };
    if constexpr (HasCmdBuildAccelerationStructuresKHR) {
      if (!std::strcmp("vkCmdBuildAccelerationStructuresKHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdBuildAccelerationStructuresKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdClearAttachments = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdClearAttachments; };
    if constexpr (HasCmdClearAttachments) {
      if (!std::strcmp("vkCmdClearAttachments", name))
        return (PFN_vkVoidFunction) &wrap_CmdClearAttachments<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdClearColorImage = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdClearColorImage; };
    if constexpr (HasCmdClearColorImage) {
      if (!std::strcmp("vkCmdClearColorImage", name))
        return (PFN_vkVoidFunction) &wrap_CmdClearColorImage<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdClearDepthStencilImage = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdClearDepthStencilImage; };
    if constexpr (HasCmdClearDepthStencilImage) {
      if (!std::strcmp("vkCmdClearDepthStencilImage", name))
        return (PFN_vkVoidFunction) &wrap_CmdClearDepthStencilImage<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VK_ENABLE_BETA_EXTENSIONS
    constexpr bool HasCmdControlVideoCodingKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdControlVideoCodingKHR; };
    if constexpr (HasCmdControlVideoCodingKHR) {
      if (!std::strcmp("vkCmdControlVideoCodingKHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdControlVideoCodingKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasCmdCopyAccelerationStructureKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdCopyAccelerationStructureKHR; };
    if constexpr (HasCmdCopyAccelerationStructureKHR) {
      if (!std::strcmp("vkCmdCopyAccelerationStructureKHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdCopyAccelerationStructureKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdCopyAccelerationStructureNV = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdCopyAccelerationStructureNV; };
    if constexpr (HasCmdCopyAccelerationStructureNV) {
      if (!std::strcmp("vkCmdCopyAccelerationStructureNV", name))
        return (PFN_vkVoidFunction) &wrap_CmdCopyAccelerationStructureNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdCopyAccelerationStructureToMemoryKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdCopyAccelerationStructureToMemoryKHR; };
    if constexpr (HasCmdCopyAccelerationStructureToMemoryKHR) {
      if (!std::strcmp("vkCmdCopyAccelerationStructureToMemoryKHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdCopyAccelerationStructureToMemoryKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdCopyBuffer = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdCopyBuffer; };
    if constexpr (HasCmdCopyBuffer) {
      if (!std::strcmp("vkCmdCopyBuffer", name))
        return (PFN_vkVoidFunction) &wrap_CmdCopyBuffer<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdCopyBuffer2 = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdCopyBuffer2; };
    if constexpr (HasCmdCopyBuffer2) {
      if (!std::strcmp("vkCmdCopyBuffer2", name))
        return (PFN_vkVoidFunction) &wrap_CmdCopyBuffer2<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdCopyBuffer2KHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdCopyBuffer2KHR; };
    if constexpr (HasCmdCopyBuffer2KHR) {
      if (!std::strcmp("vkCmdCopyBuffer2KHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdCopyBuffer2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdCopyBufferToImage = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdCopyBufferToImage; };
    if constexpr (HasCmdCopyBufferToImage) {
      if (!std::strcmp("vkCmdCopyBufferToImage", name))
        return (PFN_vkVoidFunction) &wrap_CmdCopyBufferToImage<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdCopyBufferToImage2 = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdCopyBufferToImage2; };
    if constexpr (HasCmdCopyBufferToImage2) {
      if (!std::strcmp("vkCmdCopyBufferToImage2", name))
        return (PFN_vkVoidFunction) &wrap_CmdCopyBufferToImage2<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdCopyBufferToImage2KHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdCopyBufferToImage2KHR; };
    if constexpr (HasCmdCopyBufferToImage2KHR) {
      if (!std::strcmp("vkCmdCopyBufferToImage2KHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdCopyBufferToImage2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdCopyImage = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdCopyImage; };
    if constexpr (HasCmdCopyImage) {
      if (!std::strcmp("vkCmdCopyImage", name))
        return (PFN_vkVoidFunction) &wrap_CmdCopyImage<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdCopyImage2 = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdCopyImage2; };
    if constexpr (HasCmdCopyImage2) {
      if (!std::strcmp("vkCmdCopyImage2", name))
        return (PFN_vkVoidFunction) &wrap_CmdCopyImage2<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdCopyImage2KHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdCopyImage2KHR; };
    if constexpr (HasCmdCopyImage2KHR) {
      if (!std::strcmp("vkCmdCopyImage2KHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdCopyImage2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdCopyImageToBuffer = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdCopyImageToBuffer; };
    if constexpr (HasCmdCopyImageToBuffer) {
      if (!std::strcmp("vkCmdCopyImageToBuffer", name))
        return (PFN_vkVoidFunction) &wrap_CmdCopyImageToBuffer<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdCopyImageToBuffer2 = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdCopyImageToBuffer2; };
    if constexpr (HasCmdCopyImageToBuffer2) {
      if (!std::strcmp("vkCmdCopyImageToBuffer2", name))
        return (PFN_vkVoidFunction) &wrap_CmdCopyImageToBuffer2<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdCopyImageToBuffer2KHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdCopyImageToBuffer2KHR; };
    if constexpr (HasCmdCopyImageToBuffer2KHR) {
      if (!std::strcmp("vkCmdCopyImageToBuffer2KHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdCopyImageToBuffer2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdCopyMemoryToAccelerationStructureKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdCopyMemoryToAccelerationStructureKHR; };
    if constexpr (HasCmdCopyMemoryToAccelerationStructureKHR) {
      if (!std::strcmp("vkCmdCopyMemoryToAccelerationStructureKHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdCopyMemoryToAccelerationStructureKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdCopyQueryPoolResults = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdCopyQueryPoolResults; };
    if constexpr (HasCmdCopyQueryPoolResults) {
      if (!std::strcmp("vkCmdCopyQueryPoolResults", name))
        return (PFN_vkVoidFunction) &wrap_CmdCopyQueryPoolResults<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VKROOTS_PLATFORM_UNSUPPORTED
    constexpr bool HasCmdCuLaunchKernelNVX = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdCuLaunchKernelNVX; };
    if constexpr (HasCmdCuLaunchKernelNVX) {
      if (!std::strcmp("vkCmdCuLaunchKernelNVX", name))
        return (PFN_vkVoidFunction) &wrap_CmdCuLaunchKernelNVX<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasCmdDebugMarkerBeginEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdDebugMarkerBeginEXT; };
    if constexpr (HasCmdDebugMarkerBeginEXT) {
      if (!std::strcmp("vkCmdDebugMarkerBeginEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdDebugMarkerBeginEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdDebugMarkerEndEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdDebugMarkerEndEXT; };
    if constexpr (HasCmdDebugMarkerEndEXT) {
      if (!std::strcmp("vkCmdDebugMarkerEndEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdDebugMarkerEndEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdDebugMarkerInsertEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdDebugMarkerInsertEXT; };
    if constexpr (HasCmdDebugMarkerInsertEXT) {
      if (!std::strcmp("vkCmdDebugMarkerInsertEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdDebugMarkerInsertEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VK_ENABLE_BETA_EXTENSIONS
    constexpr bool HasCmdDecodeVideoKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdDecodeVideoKHR; };
    if constexpr (HasCmdDecodeVideoKHR) {
      if (!std::strcmp("vkCmdDecodeVideoKHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdDecodeVideoKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasCmdDispatch = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdDispatch; };
    if constexpr (HasCmdDispatch) {
      if (!std::strcmp("vkCmdDispatch", name))
        return (PFN_vkVoidFunction) &wrap_CmdDispatch<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdDispatchBase = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdDispatchBase; };
    if constexpr (HasCmdDispatchBase) {
      if (!std::strcmp("vkCmdDispatchBase", name))
        return (PFN_vkVoidFunction) &wrap_CmdDispatchBase<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdDispatchBaseKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdDispatchBaseKHR; };
    if constexpr (HasCmdDispatchBaseKHR) {
      if (!std::strcmp("vkCmdDispatchBaseKHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdDispatchBaseKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdDispatchIndirect = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdDispatchIndirect; };
    if constexpr (HasCmdDispatchIndirect) {
      if (!std::strcmp("vkCmdDispatchIndirect", name))
        return (PFN_vkVoidFunction) &wrap_CmdDispatchIndirect<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdDraw = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdDraw; };
    if constexpr (HasCmdDraw) {
      if (!std::strcmp("vkCmdDraw", name))
        return (PFN_vkVoidFunction) &wrap_CmdDraw<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdDrawIndexed = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdDrawIndexed; };
    if constexpr (HasCmdDrawIndexed) {
      if (!std::strcmp("vkCmdDrawIndexed", name))
        return (PFN_vkVoidFunction) &wrap_CmdDrawIndexed<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdDrawIndexedIndirect = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdDrawIndexedIndirect; };
    if constexpr (HasCmdDrawIndexedIndirect) {
      if (!std::strcmp("vkCmdDrawIndexedIndirect", name))
        return (PFN_vkVoidFunction) &wrap_CmdDrawIndexedIndirect<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdDrawIndexedIndirectCount = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdDrawIndexedIndirectCount; };
    if constexpr (HasCmdDrawIndexedIndirectCount) {
      if (!std::strcmp("vkCmdDrawIndexedIndirectCount", name))
        return (PFN_vkVoidFunction) &wrap_CmdDrawIndexedIndirectCount<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdDrawIndexedIndirectCountAMD = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdDrawIndexedIndirectCountAMD; };
    if constexpr (HasCmdDrawIndexedIndirectCountAMD) {
      if (!std::strcmp("vkCmdDrawIndexedIndirectCountAMD", name))
        return (PFN_vkVoidFunction) &wrap_CmdDrawIndexedIndirectCountAMD<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdDrawIndexedIndirectCountKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdDrawIndexedIndirectCountKHR; };
    if constexpr (HasCmdDrawIndexedIndirectCountKHR) {
      if (!std::strcmp("vkCmdDrawIndexedIndirectCountKHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdDrawIndexedIndirectCountKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdDrawIndirect = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdDrawIndirect; };
    if constexpr (HasCmdDrawIndirect) {
      if (!std::strcmp("vkCmdDrawIndirect", name))
        return (PFN_vkVoidFunction) &wrap_CmdDrawIndirect<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdDrawIndirectByteCountEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdDrawIndirectByteCountEXT; };
    if constexpr (HasCmdDrawIndirectByteCountEXT) {
      if (!std::strcmp("vkCmdDrawIndirectByteCountEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdDrawIndirectByteCountEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdDrawIndirectCount = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdDrawIndirectCount; };
    if constexpr (HasCmdDrawIndirectCount) {
      if (!std::strcmp("vkCmdDrawIndirectCount", name))
        return (PFN_vkVoidFunction) &wrap_CmdDrawIndirectCount<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdDrawIndirectCountAMD = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdDrawIndirectCountAMD; };
    if constexpr (HasCmdDrawIndirectCountAMD) {
      if (!std::strcmp("vkCmdDrawIndirectCountAMD", name))
        return (PFN_vkVoidFunction) &wrap_CmdDrawIndirectCountAMD<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdDrawIndirectCountKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdDrawIndirectCountKHR; };
    if constexpr (HasCmdDrawIndirectCountKHR) {
      if (!std::strcmp("vkCmdDrawIndirectCountKHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdDrawIndirectCountKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdDrawMeshTasksIndirectCountNV = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdDrawMeshTasksIndirectCountNV; };
    if constexpr (HasCmdDrawMeshTasksIndirectCountNV) {
      if (!std::strcmp("vkCmdDrawMeshTasksIndirectCountNV", name))
        return (PFN_vkVoidFunction) &wrap_CmdDrawMeshTasksIndirectCountNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdDrawMeshTasksIndirectNV = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdDrawMeshTasksIndirectNV; };
    if constexpr (HasCmdDrawMeshTasksIndirectNV) {
      if (!std::strcmp("vkCmdDrawMeshTasksIndirectNV", name))
        return (PFN_vkVoidFunction) &wrap_CmdDrawMeshTasksIndirectNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdDrawMeshTasksNV = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdDrawMeshTasksNV; };
    if constexpr (HasCmdDrawMeshTasksNV) {
      if (!std::strcmp("vkCmdDrawMeshTasksNV", name))
        return (PFN_vkVoidFunction) &wrap_CmdDrawMeshTasksNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdDrawMultiEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdDrawMultiEXT; };
    if constexpr (HasCmdDrawMultiEXT) {
      if (!std::strcmp("vkCmdDrawMultiEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdDrawMultiEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdDrawMultiIndexedEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdDrawMultiIndexedEXT; };
    if constexpr (HasCmdDrawMultiIndexedEXT) {
      if (!std::strcmp("vkCmdDrawMultiIndexedEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdDrawMultiIndexedEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VK_ENABLE_BETA_EXTENSIONS
    constexpr bool HasCmdEncodeVideoKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdEncodeVideoKHR; };
    if constexpr (HasCmdEncodeVideoKHR) {
      if (!std::strcmp("vkCmdEncodeVideoKHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdEncodeVideoKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasCmdEndConditionalRenderingEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdEndConditionalRenderingEXT; };
    if constexpr (HasCmdEndConditionalRenderingEXT) {
      if (!std::strcmp("vkCmdEndConditionalRenderingEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdEndConditionalRenderingEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdEndDebugUtilsLabelEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdEndDebugUtilsLabelEXT; };
    if constexpr (HasCmdEndDebugUtilsLabelEXT) {
      if (!std::strcmp("vkCmdEndDebugUtilsLabelEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdEndDebugUtilsLabelEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdEndQuery = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdEndQuery; };
    if constexpr (HasCmdEndQuery) {
      if (!std::strcmp("vkCmdEndQuery", name))
        return (PFN_vkVoidFunction) &wrap_CmdEndQuery<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdEndQueryIndexedEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdEndQueryIndexedEXT; };
    if constexpr (HasCmdEndQueryIndexedEXT) {
      if (!std::strcmp("vkCmdEndQueryIndexedEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdEndQueryIndexedEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdEndRenderPass = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdEndRenderPass; };
    if constexpr (HasCmdEndRenderPass) {
      if (!std::strcmp("vkCmdEndRenderPass", name))
        return (PFN_vkVoidFunction) &wrap_CmdEndRenderPass<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdEndRenderPass2 = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdEndRenderPass2; };
    if constexpr (HasCmdEndRenderPass2) {
      if (!std::strcmp("vkCmdEndRenderPass2", name))
        return (PFN_vkVoidFunction) &wrap_CmdEndRenderPass2<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdEndRenderPass2KHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdEndRenderPass2KHR; };
    if constexpr (HasCmdEndRenderPass2KHR) {
      if (!std::strcmp("vkCmdEndRenderPass2KHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdEndRenderPass2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdEndRendering = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdEndRendering; };
    if constexpr (HasCmdEndRendering) {
      if (!std::strcmp("vkCmdEndRendering", name))
        return (PFN_vkVoidFunction) &wrap_CmdEndRendering<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdEndRenderingKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdEndRenderingKHR; };
    if constexpr (HasCmdEndRenderingKHR) {
      if (!std::strcmp("vkCmdEndRenderingKHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdEndRenderingKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdEndTransformFeedbackEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdEndTransformFeedbackEXT; };
    if constexpr (HasCmdEndTransformFeedbackEXT) {
      if (!std::strcmp("vkCmdEndTransformFeedbackEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdEndTransformFeedbackEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VK_ENABLE_BETA_EXTENSIONS
    constexpr bool HasCmdEndVideoCodingKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdEndVideoCodingKHR; };
    if constexpr (HasCmdEndVideoCodingKHR) {
      if (!std::strcmp("vkCmdEndVideoCodingKHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdEndVideoCodingKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasCmdExecuteCommands = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdExecuteCommands; };
    if constexpr (HasCmdExecuteCommands) {
      if (!std::strcmp("vkCmdExecuteCommands", name))
        return (PFN_vkVoidFunction) &wrap_CmdExecuteCommands<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdExecuteGeneratedCommandsNV = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdExecuteGeneratedCommandsNV; };
    if constexpr (HasCmdExecuteGeneratedCommandsNV) {
      if (!std::strcmp("vkCmdExecuteGeneratedCommandsNV", name))
        return (PFN_vkVoidFunction) &wrap_CmdExecuteGeneratedCommandsNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdFillBuffer = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdFillBuffer; };
    if constexpr (HasCmdFillBuffer) {
      if (!std::strcmp("vkCmdFillBuffer", name))
        return (PFN_vkVoidFunction) &wrap_CmdFillBuffer<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdInsertDebugUtilsLabelEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdInsertDebugUtilsLabelEXT; };
    if constexpr (HasCmdInsertDebugUtilsLabelEXT) {
      if (!std::strcmp("vkCmdInsertDebugUtilsLabelEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdInsertDebugUtilsLabelEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdNextSubpass = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdNextSubpass; };
    if constexpr (HasCmdNextSubpass) {
      if (!std::strcmp("vkCmdNextSubpass", name))
        return (PFN_vkVoidFunction) &wrap_CmdNextSubpass<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdNextSubpass2 = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdNextSubpass2; };
    if constexpr (HasCmdNextSubpass2) {
      if (!std::strcmp("vkCmdNextSubpass2", name))
        return (PFN_vkVoidFunction) &wrap_CmdNextSubpass2<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdNextSubpass2KHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdNextSubpass2KHR; };
    if constexpr (HasCmdNextSubpass2KHR) {
      if (!std::strcmp("vkCmdNextSubpass2KHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdNextSubpass2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdPipelineBarrier = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdPipelineBarrier; };
    if constexpr (HasCmdPipelineBarrier) {
      if (!std::strcmp("vkCmdPipelineBarrier", name))
        return (PFN_vkVoidFunction) &wrap_CmdPipelineBarrier<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdPipelineBarrier2 = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdPipelineBarrier2; };
    if constexpr (HasCmdPipelineBarrier2) {
      if (!std::strcmp("vkCmdPipelineBarrier2", name))
        return (PFN_vkVoidFunction) &wrap_CmdPipelineBarrier2<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdPipelineBarrier2KHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdPipelineBarrier2KHR; };
    if constexpr (HasCmdPipelineBarrier2KHR) {
      if (!std::strcmp("vkCmdPipelineBarrier2KHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdPipelineBarrier2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdPreprocessGeneratedCommandsNV = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdPreprocessGeneratedCommandsNV; };
    if constexpr (HasCmdPreprocessGeneratedCommandsNV) {
      if (!std::strcmp("vkCmdPreprocessGeneratedCommandsNV", name))
        return (PFN_vkVoidFunction) &wrap_CmdPreprocessGeneratedCommandsNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdPushConstants = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdPushConstants; };
    if constexpr (HasCmdPushConstants) {
      if (!std::strcmp("vkCmdPushConstants", name))
        return (PFN_vkVoidFunction) &wrap_CmdPushConstants<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdPushDescriptorSetKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdPushDescriptorSetKHR; };
    if constexpr (HasCmdPushDescriptorSetKHR) {
      if (!std::strcmp("vkCmdPushDescriptorSetKHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdPushDescriptorSetKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdPushDescriptorSetWithTemplateKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdPushDescriptorSetWithTemplateKHR; };
    if constexpr (HasCmdPushDescriptorSetWithTemplateKHR) {
      if (!std::strcmp("vkCmdPushDescriptorSetWithTemplateKHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdPushDescriptorSetWithTemplateKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdResetEvent = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdResetEvent; };
    if constexpr (HasCmdResetEvent) {
      if (!std::strcmp("vkCmdResetEvent", name))
        return (PFN_vkVoidFunction) &wrap_CmdResetEvent<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdResetEvent2 = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdResetEvent2; };
    if constexpr (HasCmdResetEvent2) {
      if (!std::strcmp("vkCmdResetEvent2", name))
        return (PFN_vkVoidFunction) &wrap_CmdResetEvent2<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdResetEvent2KHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdResetEvent2KHR; };
    if constexpr (HasCmdResetEvent2KHR) {
      if (!std::strcmp("vkCmdResetEvent2KHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdResetEvent2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdResetQueryPool = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdResetQueryPool; };
    if constexpr (HasCmdResetQueryPool) {
      if (!std::strcmp("vkCmdResetQueryPool", name))
        return (PFN_vkVoidFunction) &wrap_CmdResetQueryPool<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdResolveImage = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdResolveImage; };
    if constexpr (HasCmdResolveImage) {
      if (!std::strcmp("vkCmdResolveImage", name))
        return (PFN_vkVoidFunction) &wrap_CmdResolveImage<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdResolveImage2 = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdResolveImage2; };
    if constexpr (HasCmdResolveImage2) {
      if (!std::strcmp("vkCmdResolveImage2", name))
        return (PFN_vkVoidFunction) &wrap_CmdResolveImage2<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdResolveImage2KHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdResolveImage2KHR; };
    if constexpr (HasCmdResolveImage2KHR) {
      if (!std::strcmp("vkCmdResolveImage2KHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdResolveImage2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetBlendConstants = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetBlendConstants; };
    if constexpr (HasCmdSetBlendConstants) {
      if (!std::strcmp("vkCmdSetBlendConstants", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetBlendConstants<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetCheckpointNV = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetCheckpointNV; };
    if constexpr (HasCmdSetCheckpointNV) {
      if (!std::strcmp("vkCmdSetCheckpointNV", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetCheckpointNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetCoarseSampleOrderNV = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetCoarseSampleOrderNV; };
    if constexpr (HasCmdSetCoarseSampleOrderNV) {
      if (!std::strcmp("vkCmdSetCoarseSampleOrderNV", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetCoarseSampleOrderNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetColorWriteEnableEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetColorWriteEnableEXT; };
    if constexpr (HasCmdSetColorWriteEnableEXT) {
      if (!std::strcmp("vkCmdSetColorWriteEnableEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetColorWriteEnableEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetCullMode = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetCullMode; };
    if constexpr (HasCmdSetCullMode) {
      if (!std::strcmp("vkCmdSetCullMode", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetCullMode<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetCullModeEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetCullModeEXT; };
    if constexpr (HasCmdSetCullModeEXT) {
      if (!std::strcmp("vkCmdSetCullModeEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetCullModeEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetDepthBias = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetDepthBias; };
    if constexpr (HasCmdSetDepthBias) {
      if (!std::strcmp("vkCmdSetDepthBias", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetDepthBias<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetDepthBiasEnable = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetDepthBiasEnable; };
    if constexpr (HasCmdSetDepthBiasEnable) {
      if (!std::strcmp("vkCmdSetDepthBiasEnable", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetDepthBiasEnable<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetDepthBiasEnableEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetDepthBiasEnableEXT; };
    if constexpr (HasCmdSetDepthBiasEnableEXT) {
      if (!std::strcmp("vkCmdSetDepthBiasEnableEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetDepthBiasEnableEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetDepthBounds = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetDepthBounds; };
    if constexpr (HasCmdSetDepthBounds) {
      if (!std::strcmp("vkCmdSetDepthBounds", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetDepthBounds<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetDepthBoundsTestEnable = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetDepthBoundsTestEnable; };
    if constexpr (HasCmdSetDepthBoundsTestEnable) {
      if (!std::strcmp("vkCmdSetDepthBoundsTestEnable", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetDepthBoundsTestEnable<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetDepthBoundsTestEnableEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetDepthBoundsTestEnableEXT; };
    if constexpr (HasCmdSetDepthBoundsTestEnableEXT) {
      if (!std::strcmp("vkCmdSetDepthBoundsTestEnableEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetDepthBoundsTestEnableEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetDepthCompareOp = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetDepthCompareOp; };
    if constexpr (HasCmdSetDepthCompareOp) {
      if (!std::strcmp("vkCmdSetDepthCompareOp", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetDepthCompareOp<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetDepthCompareOpEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetDepthCompareOpEXT; };
    if constexpr (HasCmdSetDepthCompareOpEXT) {
      if (!std::strcmp("vkCmdSetDepthCompareOpEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetDepthCompareOpEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetDepthTestEnable = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetDepthTestEnable; };
    if constexpr (HasCmdSetDepthTestEnable) {
      if (!std::strcmp("vkCmdSetDepthTestEnable", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetDepthTestEnable<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetDepthTestEnableEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetDepthTestEnableEXT; };
    if constexpr (HasCmdSetDepthTestEnableEXT) {
      if (!std::strcmp("vkCmdSetDepthTestEnableEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetDepthTestEnableEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetDepthWriteEnable = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetDepthWriteEnable; };
    if constexpr (HasCmdSetDepthWriteEnable) {
      if (!std::strcmp("vkCmdSetDepthWriteEnable", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetDepthWriteEnable<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetDepthWriteEnableEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetDepthWriteEnableEXT; };
    if constexpr (HasCmdSetDepthWriteEnableEXT) {
      if (!std::strcmp("vkCmdSetDepthWriteEnableEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetDepthWriteEnableEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetDeviceMask = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetDeviceMask; };
    if constexpr (HasCmdSetDeviceMask) {
      if (!std::strcmp("vkCmdSetDeviceMask", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetDeviceMask<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetDeviceMaskKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetDeviceMaskKHR; };
    if constexpr (HasCmdSetDeviceMaskKHR) {
      if (!std::strcmp("vkCmdSetDeviceMaskKHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetDeviceMaskKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetDiscardRectangleEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetDiscardRectangleEXT; };
    if constexpr (HasCmdSetDiscardRectangleEXT) {
      if (!std::strcmp("vkCmdSetDiscardRectangleEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetDiscardRectangleEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetEvent = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetEvent; };
    if constexpr (HasCmdSetEvent) {
      if (!std::strcmp("vkCmdSetEvent", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetEvent<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetEvent2 = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetEvent2; };
    if constexpr (HasCmdSetEvent2) {
      if (!std::strcmp("vkCmdSetEvent2", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetEvent2<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetEvent2KHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetEvent2KHR; };
    if constexpr (HasCmdSetEvent2KHR) {
      if (!std::strcmp("vkCmdSetEvent2KHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetEvent2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetExclusiveScissorNV = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetExclusiveScissorNV; };
    if constexpr (HasCmdSetExclusiveScissorNV) {
      if (!std::strcmp("vkCmdSetExclusiveScissorNV", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetExclusiveScissorNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetFragmentShadingRateEnumNV = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetFragmentShadingRateEnumNV; };
    if constexpr (HasCmdSetFragmentShadingRateEnumNV) {
      if (!std::strcmp("vkCmdSetFragmentShadingRateEnumNV", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetFragmentShadingRateEnumNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetFragmentShadingRateKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetFragmentShadingRateKHR; };
    if constexpr (HasCmdSetFragmentShadingRateKHR) {
      if (!std::strcmp("vkCmdSetFragmentShadingRateKHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetFragmentShadingRateKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetFrontFace = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetFrontFace; };
    if constexpr (HasCmdSetFrontFace) {
      if (!std::strcmp("vkCmdSetFrontFace", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetFrontFace<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetFrontFaceEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetFrontFaceEXT; };
    if constexpr (HasCmdSetFrontFaceEXT) {
      if (!std::strcmp("vkCmdSetFrontFaceEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetFrontFaceEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetLineStippleEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetLineStippleEXT; };
    if constexpr (HasCmdSetLineStippleEXT) {
      if (!std::strcmp("vkCmdSetLineStippleEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetLineStippleEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetLineWidth = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetLineWidth; };
    if constexpr (HasCmdSetLineWidth) {
      if (!std::strcmp("vkCmdSetLineWidth", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetLineWidth<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetLogicOpEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetLogicOpEXT; };
    if constexpr (HasCmdSetLogicOpEXT) {
      if (!std::strcmp("vkCmdSetLogicOpEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetLogicOpEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetPatchControlPointsEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetPatchControlPointsEXT; };
    if constexpr (HasCmdSetPatchControlPointsEXT) {
      if (!std::strcmp("vkCmdSetPatchControlPointsEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetPatchControlPointsEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetPerformanceMarkerINTEL = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetPerformanceMarkerINTEL; };
    if constexpr (HasCmdSetPerformanceMarkerINTEL) {
      if (!std::strcmp("vkCmdSetPerformanceMarkerINTEL", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetPerformanceMarkerINTEL<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetPerformanceOverrideINTEL = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetPerformanceOverrideINTEL; };
    if constexpr (HasCmdSetPerformanceOverrideINTEL) {
      if (!std::strcmp("vkCmdSetPerformanceOverrideINTEL", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetPerformanceOverrideINTEL<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetPerformanceStreamMarkerINTEL = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetPerformanceStreamMarkerINTEL; };
    if constexpr (HasCmdSetPerformanceStreamMarkerINTEL) {
      if (!std::strcmp("vkCmdSetPerformanceStreamMarkerINTEL", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetPerformanceStreamMarkerINTEL<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetPrimitiveRestartEnable = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetPrimitiveRestartEnable; };
    if constexpr (HasCmdSetPrimitiveRestartEnable) {
      if (!std::strcmp("vkCmdSetPrimitiveRestartEnable", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetPrimitiveRestartEnable<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetPrimitiveRestartEnableEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetPrimitiveRestartEnableEXT; };
    if constexpr (HasCmdSetPrimitiveRestartEnableEXT) {
      if (!std::strcmp("vkCmdSetPrimitiveRestartEnableEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetPrimitiveRestartEnableEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetPrimitiveTopology = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetPrimitiveTopology; };
    if constexpr (HasCmdSetPrimitiveTopology) {
      if (!std::strcmp("vkCmdSetPrimitiveTopology", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetPrimitiveTopology<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetPrimitiveTopologyEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetPrimitiveTopologyEXT; };
    if constexpr (HasCmdSetPrimitiveTopologyEXT) {
      if (!std::strcmp("vkCmdSetPrimitiveTopologyEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetPrimitiveTopologyEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetRasterizerDiscardEnable = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetRasterizerDiscardEnable; };
    if constexpr (HasCmdSetRasterizerDiscardEnable) {
      if (!std::strcmp("vkCmdSetRasterizerDiscardEnable", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetRasterizerDiscardEnable<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetRasterizerDiscardEnableEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetRasterizerDiscardEnableEXT; };
    if constexpr (HasCmdSetRasterizerDiscardEnableEXT) {
      if (!std::strcmp("vkCmdSetRasterizerDiscardEnableEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetRasterizerDiscardEnableEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetRayTracingPipelineStackSizeKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetRayTracingPipelineStackSizeKHR; };
    if constexpr (HasCmdSetRayTracingPipelineStackSizeKHR) {
      if (!std::strcmp("vkCmdSetRayTracingPipelineStackSizeKHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetRayTracingPipelineStackSizeKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetSampleLocationsEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetSampleLocationsEXT; };
    if constexpr (HasCmdSetSampleLocationsEXT) {
      if (!std::strcmp("vkCmdSetSampleLocationsEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetSampleLocationsEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetScissor = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetScissor; };
    if constexpr (HasCmdSetScissor) {
      if (!std::strcmp("vkCmdSetScissor", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetScissor<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetScissorWithCount = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetScissorWithCount; };
    if constexpr (HasCmdSetScissorWithCount) {
      if (!std::strcmp("vkCmdSetScissorWithCount", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetScissorWithCount<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetScissorWithCountEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetScissorWithCountEXT; };
    if constexpr (HasCmdSetScissorWithCountEXT) {
      if (!std::strcmp("vkCmdSetScissorWithCountEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetScissorWithCountEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetStencilCompareMask = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetStencilCompareMask; };
    if constexpr (HasCmdSetStencilCompareMask) {
      if (!std::strcmp("vkCmdSetStencilCompareMask", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetStencilCompareMask<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetStencilOp = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetStencilOp; };
    if constexpr (HasCmdSetStencilOp) {
      if (!std::strcmp("vkCmdSetStencilOp", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetStencilOp<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetStencilOpEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetStencilOpEXT; };
    if constexpr (HasCmdSetStencilOpEXT) {
      if (!std::strcmp("vkCmdSetStencilOpEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetStencilOpEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetStencilReference = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetStencilReference; };
    if constexpr (HasCmdSetStencilReference) {
      if (!std::strcmp("vkCmdSetStencilReference", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetStencilReference<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetStencilTestEnable = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetStencilTestEnable; };
    if constexpr (HasCmdSetStencilTestEnable) {
      if (!std::strcmp("vkCmdSetStencilTestEnable", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetStencilTestEnable<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetStencilTestEnableEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetStencilTestEnableEXT; };
    if constexpr (HasCmdSetStencilTestEnableEXT) {
      if (!std::strcmp("vkCmdSetStencilTestEnableEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetStencilTestEnableEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetStencilWriteMask = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetStencilWriteMask; };
    if constexpr (HasCmdSetStencilWriteMask) {
      if (!std::strcmp("vkCmdSetStencilWriteMask", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetStencilWriteMask<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetVertexInputEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetVertexInputEXT; };
    if constexpr (HasCmdSetVertexInputEXT) {
      if (!std::strcmp("vkCmdSetVertexInputEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetVertexInputEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetViewport = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetViewport; };
    if constexpr (HasCmdSetViewport) {
      if (!std::strcmp("vkCmdSetViewport", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetViewport<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetViewportShadingRatePaletteNV = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetViewportShadingRatePaletteNV; };
    if constexpr (HasCmdSetViewportShadingRatePaletteNV) {
      if (!std::strcmp("vkCmdSetViewportShadingRatePaletteNV", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetViewportShadingRatePaletteNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetViewportWScalingNV = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetViewportWScalingNV; };
    if constexpr (HasCmdSetViewportWScalingNV) {
      if (!std::strcmp("vkCmdSetViewportWScalingNV", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetViewportWScalingNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetViewportWithCount = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetViewportWithCount; };
    if constexpr (HasCmdSetViewportWithCount) {
      if (!std::strcmp("vkCmdSetViewportWithCount", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetViewportWithCount<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSetViewportWithCountEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSetViewportWithCountEXT; };
    if constexpr (HasCmdSetViewportWithCountEXT) {
      if (!std::strcmp("vkCmdSetViewportWithCountEXT", name))
        return (PFN_vkVoidFunction) &wrap_CmdSetViewportWithCountEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdSubpassShadingHUAWEI = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdSubpassShadingHUAWEI; };
    if constexpr (HasCmdSubpassShadingHUAWEI) {
      if (!std::strcmp("vkCmdSubpassShadingHUAWEI", name))
        return (PFN_vkVoidFunction) &wrap_CmdSubpassShadingHUAWEI<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdTraceRaysIndirect2KHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdTraceRaysIndirect2KHR; };
    if constexpr (HasCmdTraceRaysIndirect2KHR) {
      if (!std::strcmp("vkCmdTraceRaysIndirect2KHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdTraceRaysIndirect2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdTraceRaysIndirectKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdTraceRaysIndirectKHR; };
    if constexpr (HasCmdTraceRaysIndirectKHR) {
      if (!std::strcmp("vkCmdTraceRaysIndirectKHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdTraceRaysIndirectKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdTraceRaysKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdTraceRaysKHR; };
    if constexpr (HasCmdTraceRaysKHR) {
      if (!std::strcmp("vkCmdTraceRaysKHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdTraceRaysKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdTraceRaysNV = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdTraceRaysNV; };
    if constexpr (HasCmdTraceRaysNV) {
      if (!std::strcmp("vkCmdTraceRaysNV", name))
        return (PFN_vkVoidFunction) &wrap_CmdTraceRaysNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdUpdateBuffer = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdUpdateBuffer; };
    if constexpr (HasCmdUpdateBuffer) {
      if (!std::strcmp("vkCmdUpdateBuffer", name))
        return (PFN_vkVoidFunction) &wrap_CmdUpdateBuffer<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdWaitEvents = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdWaitEvents; };
    if constexpr (HasCmdWaitEvents) {
      if (!std::strcmp("vkCmdWaitEvents", name))
        return (PFN_vkVoidFunction) &wrap_CmdWaitEvents<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdWaitEvents2 = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdWaitEvents2; };
    if constexpr (HasCmdWaitEvents2) {
      if (!std::strcmp("vkCmdWaitEvents2", name))
        return (PFN_vkVoidFunction) &wrap_CmdWaitEvents2<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdWaitEvents2KHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdWaitEvents2KHR; };
    if constexpr (HasCmdWaitEvents2KHR) {
      if (!std::strcmp("vkCmdWaitEvents2KHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdWaitEvents2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdWriteAccelerationStructuresPropertiesKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdWriteAccelerationStructuresPropertiesKHR; };
    if constexpr (HasCmdWriteAccelerationStructuresPropertiesKHR) {
      if (!std::strcmp("vkCmdWriteAccelerationStructuresPropertiesKHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdWriteAccelerationStructuresPropertiesKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdWriteAccelerationStructuresPropertiesNV = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdWriteAccelerationStructuresPropertiesNV; };
    if constexpr (HasCmdWriteAccelerationStructuresPropertiesNV) {
      if (!std::strcmp("vkCmdWriteAccelerationStructuresPropertiesNV", name))
        return (PFN_vkVoidFunction) &wrap_CmdWriteAccelerationStructuresPropertiesNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdWriteBufferMarker2AMD = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdWriteBufferMarker2AMD; };
    if constexpr (HasCmdWriteBufferMarker2AMD) {
      if (!std::strcmp("vkCmdWriteBufferMarker2AMD", name))
        return (PFN_vkVoidFunction) &wrap_CmdWriteBufferMarker2AMD<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdWriteBufferMarkerAMD = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdWriteBufferMarkerAMD; };
    if constexpr (HasCmdWriteBufferMarkerAMD) {
      if (!std::strcmp("vkCmdWriteBufferMarkerAMD", name))
        return (PFN_vkVoidFunction) &wrap_CmdWriteBufferMarkerAMD<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdWriteTimestamp = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdWriteTimestamp; };
    if constexpr (HasCmdWriteTimestamp) {
      if (!std::strcmp("vkCmdWriteTimestamp", name))
        return (PFN_vkVoidFunction) &wrap_CmdWriteTimestamp<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdWriteTimestamp2 = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdWriteTimestamp2; };
    if constexpr (HasCmdWriteTimestamp2) {
      if (!std::strcmp("vkCmdWriteTimestamp2", name))
        return (PFN_vkVoidFunction) &wrap_CmdWriteTimestamp2<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCmdWriteTimestamp2KHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CmdWriteTimestamp2KHR; };
    if constexpr (HasCmdWriteTimestamp2KHR) {
      if (!std::strcmp("vkCmdWriteTimestamp2KHR", name))
        return (PFN_vkVoidFunction) &wrap_CmdWriteTimestamp2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCompileDeferredNV = requires(const DeviceOverrides& t) { &DeviceOverrides::CompileDeferredNV; };
    if constexpr (HasCompileDeferredNV) {
      if (!std::strcmp("vkCompileDeferredNV", name))
        return (PFN_vkVoidFunction) &wrap_CompileDeferredNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCopyAccelerationStructureKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CopyAccelerationStructureKHR; };
    if constexpr (HasCopyAccelerationStructureKHR) {
      if (!std::strcmp("vkCopyAccelerationStructureKHR", name))
        return (PFN_vkVoidFunction) &wrap_CopyAccelerationStructureKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCopyAccelerationStructureToMemoryKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CopyAccelerationStructureToMemoryKHR; };
    if constexpr (HasCopyAccelerationStructureToMemoryKHR) {
      if (!std::strcmp("vkCopyAccelerationStructureToMemoryKHR", name))
        return (PFN_vkVoidFunction) &wrap_CopyAccelerationStructureToMemoryKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCopyMemoryToAccelerationStructureKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CopyMemoryToAccelerationStructureKHR; };
    if constexpr (HasCopyMemoryToAccelerationStructureKHR) {
      if (!std::strcmp("vkCopyMemoryToAccelerationStructureKHR", name))
        return (PFN_vkVoidFunction) &wrap_CopyMemoryToAccelerationStructureKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateAccelerationStructureKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateAccelerationStructureKHR; };
    if constexpr (HasCreateAccelerationStructureKHR) {
      if (!std::strcmp("vkCreateAccelerationStructureKHR", name))
        return (PFN_vkVoidFunction) &wrap_CreateAccelerationStructureKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateAccelerationStructureNV = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateAccelerationStructureNV; };
    if constexpr (HasCreateAccelerationStructureNV) {
      if (!std::strcmp("vkCreateAccelerationStructureNV", name))
        return (PFN_vkVoidFunction) &wrap_CreateAccelerationStructureNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateBuffer = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateBuffer; };
    if constexpr (HasCreateBuffer) {
      if (!std::strcmp("vkCreateBuffer", name))
        return (PFN_vkVoidFunction) &wrap_CreateBuffer<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VK_USE_PLATFORM_FUCHSIA
    constexpr bool HasCreateBufferCollectionFUCHSIA = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateBufferCollectionFUCHSIA; };
    if constexpr (HasCreateBufferCollectionFUCHSIA) {
      if (!std::strcmp("vkCreateBufferCollectionFUCHSIA", name))
        return (PFN_vkVoidFunction) &wrap_CreateBufferCollectionFUCHSIA<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasCreateBufferView = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateBufferView; };
    if constexpr (HasCreateBufferView) {
      if (!std::strcmp("vkCreateBufferView", name))
        return (PFN_vkVoidFunction) &wrap_CreateBufferView<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateCommandPool = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateCommandPool; };
    if constexpr (HasCreateCommandPool) {
      if (!std::strcmp("vkCreateCommandPool", name))
        return (PFN_vkVoidFunction) &wrap_CreateCommandPool<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateComputePipelines = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateComputePipelines; };
    if constexpr (HasCreateComputePipelines) {
      if (!std::strcmp("vkCreateComputePipelines", name))
        return (PFN_vkVoidFunction) &wrap_CreateComputePipelines<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VKROOTS_PLATFORM_UNSUPPORTED
    constexpr bool HasCreateCuFunctionNVX = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateCuFunctionNVX; };
    if constexpr (HasCreateCuFunctionNVX) {
      if (!std::strcmp("vkCreateCuFunctionNVX", name))
        return (PFN_vkVoidFunction) &wrap_CreateCuFunctionNVX<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

#ifdef VKROOTS_PLATFORM_UNSUPPORTED
    constexpr bool HasCreateCuModuleNVX = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateCuModuleNVX; };
    if constexpr (HasCreateCuModuleNVX) {
      if (!std::strcmp("vkCreateCuModuleNVX", name))
        return (PFN_vkVoidFunction) &wrap_CreateCuModuleNVX<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasCreateDeferredOperationKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateDeferredOperationKHR; };
    if constexpr (HasCreateDeferredOperationKHR) {
      if (!std::strcmp("vkCreateDeferredOperationKHR", name))
        return (PFN_vkVoidFunction) &wrap_CreateDeferredOperationKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateDescriptorPool = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateDescriptorPool; };
    if constexpr (HasCreateDescriptorPool) {
      if (!std::strcmp("vkCreateDescriptorPool", name))
        return (PFN_vkVoidFunction) &wrap_CreateDescriptorPool<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateDescriptorSetLayout = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateDescriptorSetLayout; };
    if constexpr (HasCreateDescriptorSetLayout) {
      if (!std::strcmp("vkCreateDescriptorSetLayout", name))
        return (PFN_vkVoidFunction) &wrap_CreateDescriptorSetLayout<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateDescriptorUpdateTemplate = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateDescriptorUpdateTemplate; };
    if constexpr (HasCreateDescriptorUpdateTemplate) {
      if (!std::strcmp("vkCreateDescriptorUpdateTemplate", name))
        return (PFN_vkVoidFunction) &wrap_CreateDescriptorUpdateTemplate<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateDescriptorUpdateTemplateKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateDescriptorUpdateTemplateKHR; };
    if constexpr (HasCreateDescriptorUpdateTemplateKHR) {
      if (!std::strcmp("vkCreateDescriptorUpdateTemplateKHR", name))
        return (PFN_vkVoidFunction) &wrap_CreateDescriptorUpdateTemplateKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateEvent = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateEvent; };
    if constexpr (HasCreateEvent) {
      if (!std::strcmp("vkCreateEvent", name))
        return (PFN_vkVoidFunction) &wrap_CreateEvent<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateFence = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateFence; };
    if constexpr (HasCreateFence) {
      if (!std::strcmp("vkCreateFence", name))
        return (PFN_vkVoidFunction) &wrap_CreateFence<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateFramebuffer = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateFramebuffer; };
    if constexpr (HasCreateFramebuffer) {
      if (!std::strcmp("vkCreateFramebuffer", name))
        return (PFN_vkVoidFunction) &wrap_CreateFramebuffer<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateGraphicsPipelines = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateGraphicsPipelines; };
    if constexpr (HasCreateGraphicsPipelines) {
      if (!std::strcmp("vkCreateGraphicsPipelines", name))
        return (PFN_vkVoidFunction) &wrap_CreateGraphicsPipelines<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateImage = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateImage; };
    if constexpr (HasCreateImage) {
      if (!std::strcmp("vkCreateImage", name))
        return (PFN_vkVoidFunction) &wrap_CreateImage<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateImageView = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateImageView; };
    if constexpr (HasCreateImageView) {
      if (!std::strcmp("vkCreateImageView", name))
        return (PFN_vkVoidFunction) &wrap_CreateImageView<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateIndirectCommandsLayoutNV = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateIndirectCommandsLayoutNV; };
    if constexpr (HasCreateIndirectCommandsLayoutNV) {
      if (!std::strcmp("vkCreateIndirectCommandsLayoutNV", name))
        return (PFN_vkVoidFunction) &wrap_CreateIndirectCommandsLayoutNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreatePipelineCache = requires(const DeviceOverrides& t) { &DeviceOverrides::CreatePipelineCache; };
    if constexpr (HasCreatePipelineCache) {
      if (!std::strcmp("vkCreatePipelineCache", name))
        return (PFN_vkVoidFunction) &wrap_CreatePipelineCache<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreatePipelineLayout = requires(const DeviceOverrides& t) { &DeviceOverrides::CreatePipelineLayout; };
    if constexpr (HasCreatePipelineLayout) {
      if (!std::strcmp("vkCreatePipelineLayout", name))
        return (PFN_vkVoidFunction) &wrap_CreatePipelineLayout<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreatePrivateDataSlot = requires(const DeviceOverrides& t) { &DeviceOverrides::CreatePrivateDataSlot; };
    if constexpr (HasCreatePrivateDataSlot) {
      if (!std::strcmp("vkCreatePrivateDataSlot", name))
        return (PFN_vkVoidFunction) &wrap_CreatePrivateDataSlot<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreatePrivateDataSlotEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CreatePrivateDataSlotEXT; };
    if constexpr (HasCreatePrivateDataSlotEXT) {
      if (!std::strcmp("vkCreatePrivateDataSlotEXT", name))
        return (PFN_vkVoidFunction) &wrap_CreatePrivateDataSlotEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateQueryPool = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateQueryPool; };
    if constexpr (HasCreateQueryPool) {
      if (!std::strcmp("vkCreateQueryPool", name))
        return (PFN_vkVoidFunction) &wrap_CreateQueryPool<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateRayTracingPipelinesKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateRayTracingPipelinesKHR; };
    if constexpr (HasCreateRayTracingPipelinesKHR) {
      if (!std::strcmp("vkCreateRayTracingPipelinesKHR", name))
        return (PFN_vkVoidFunction) &wrap_CreateRayTracingPipelinesKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateRayTracingPipelinesNV = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateRayTracingPipelinesNV; };
    if constexpr (HasCreateRayTracingPipelinesNV) {
      if (!std::strcmp("vkCreateRayTracingPipelinesNV", name))
        return (PFN_vkVoidFunction) &wrap_CreateRayTracingPipelinesNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateRenderPass = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateRenderPass; };
    if constexpr (HasCreateRenderPass) {
      if (!std::strcmp("vkCreateRenderPass", name))
        return (PFN_vkVoidFunction) &wrap_CreateRenderPass<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateRenderPass2 = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateRenderPass2; };
    if constexpr (HasCreateRenderPass2) {
      if (!std::strcmp("vkCreateRenderPass2", name))
        return (PFN_vkVoidFunction) &wrap_CreateRenderPass2<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateRenderPass2KHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateRenderPass2KHR; };
    if constexpr (HasCreateRenderPass2KHR) {
      if (!std::strcmp("vkCreateRenderPass2KHR", name))
        return (PFN_vkVoidFunction) &wrap_CreateRenderPass2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateSampler = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateSampler; };
    if constexpr (HasCreateSampler) {
      if (!std::strcmp("vkCreateSampler", name))
        return (PFN_vkVoidFunction) &wrap_CreateSampler<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateSamplerYcbcrConversion = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateSamplerYcbcrConversion; };
    if constexpr (HasCreateSamplerYcbcrConversion) {
      if (!std::strcmp("vkCreateSamplerYcbcrConversion", name))
        return (PFN_vkVoidFunction) &wrap_CreateSamplerYcbcrConversion<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateSamplerYcbcrConversionKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateSamplerYcbcrConversionKHR; };
    if constexpr (HasCreateSamplerYcbcrConversionKHR) {
      if (!std::strcmp("vkCreateSamplerYcbcrConversionKHR", name))
        return (PFN_vkVoidFunction) &wrap_CreateSamplerYcbcrConversionKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateSemaphore = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateSemaphore; };
    if constexpr (HasCreateSemaphore) {
      if (!std::strcmp("vkCreateSemaphore", name))
        return (PFN_vkVoidFunction) &wrap_CreateSemaphore<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateShaderModule = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateShaderModule; };
    if constexpr (HasCreateShaderModule) {
      if (!std::strcmp("vkCreateShaderModule", name))
        return (PFN_vkVoidFunction) &wrap_CreateShaderModule<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateSharedSwapchainsKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateSharedSwapchainsKHR; };
    if constexpr (HasCreateSharedSwapchainsKHR) {
      if (!std::strcmp("vkCreateSharedSwapchainsKHR", name))
        return (PFN_vkVoidFunction) &wrap_CreateSharedSwapchainsKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateSwapchainKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateSwapchainKHR; };
    if constexpr (HasCreateSwapchainKHR) {
      if (!std::strcmp("vkCreateSwapchainKHR", name))
        return (PFN_vkVoidFunction) &wrap_CreateSwapchainKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasCreateValidationCacheEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateValidationCacheEXT; };
    if constexpr (HasCreateValidationCacheEXT) {
      if (!std::strcmp("vkCreateValidationCacheEXT", name))
        return (PFN_vkVoidFunction) &wrap_CreateValidationCacheEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VK_ENABLE_BETA_EXTENSIONS
    constexpr bool HasCreateVideoSessionKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateVideoSessionKHR; };
    if constexpr (HasCreateVideoSessionKHR) {
      if (!std::strcmp("vkCreateVideoSessionKHR", name))
        return (PFN_vkVoidFunction) &wrap_CreateVideoSessionKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

#ifdef VK_ENABLE_BETA_EXTENSIONS
    constexpr bool HasCreateVideoSessionParametersKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::CreateVideoSessionParametersKHR; };
    if constexpr (HasCreateVideoSessionParametersKHR) {
      if (!std::strcmp("vkCreateVideoSessionParametersKHR", name))
        return (PFN_vkVoidFunction) &wrap_CreateVideoSessionParametersKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasDebugMarkerSetObjectNameEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::DebugMarkerSetObjectNameEXT; };
    if constexpr (HasDebugMarkerSetObjectNameEXT) {
      if (!std::strcmp("vkDebugMarkerSetObjectNameEXT", name))
        return (PFN_vkVoidFunction) &wrap_DebugMarkerSetObjectNameEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDebugMarkerSetObjectTagEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::DebugMarkerSetObjectTagEXT; };
    if constexpr (HasDebugMarkerSetObjectTagEXT) {
      if (!std::strcmp("vkDebugMarkerSetObjectTagEXT", name))
        return (PFN_vkVoidFunction) &wrap_DebugMarkerSetObjectTagEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDeferredOperationJoinKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::DeferredOperationJoinKHR; };
    if constexpr (HasDeferredOperationJoinKHR) {
      if (!std::strcmp("vkDeferredOperationJoinKHR", name))
        return (PFN_vkVoidFunction) &wrap_DeferredOperationJoinKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDestroyAccelerationStructureKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroyAccelerationStructureKHR; };
    if constexpr (HasDestroyAccelerationStructureKHR) {
      if (!std::strcmp("vkDestroyAccelerationStructureKHR", name))
        return (PFN_vkVoidFunction) &wrap_DestroyAccelerationStructureKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDestroyAccelerationStructureNV = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroyAccelerationStructureNV; };
    if constexpr (HasDestroyAccelerationStructureNV) {
      if (!std::strcmp("vkDestroyAccelerationStructureNV", name))
        return (PFN_vkVoidFunction) &wrap_DestroyAccelerationStructureNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDestroyBuffer = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroyBuffer; };
    if constexpr (HasDestroyBuffer) {
      if (!std::strcmp("vkDestroyBuffer", name))
        return (PFN_vkVoidFunction) &wrap_DestroyBuffer<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VK_USE_PLATFORM_FUCHSIA
    constexpr bool HasDestroyBufferCollectionFUCHSIA = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroyBufferCollectionFUCHSIA; };
    if constexpr (HasDestroyBufferCollectionFUCHSIA) {
      if (!std::strcmp("vkDestroyBufferCollectionFUCHSIA", name))
        return (PFN_vkVoidFunction) &wrap_DestroyBufferCollectionFUCHSIA<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasDestroyBufferView = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroyBufferView; };
    if constexpr (HasDestroyBufferView) {
      if (!std::strcmp("vkDestroyBufferView", name))
        return (PFN_vkVoidFunction) &wrap_DestroyBufferView<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDestroyCommandPool = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroyCommandPool; };
    if constexpr (HasDestroyCommandPool) {
      if (!std::strcmp("vkDestroyCommandPool", name))
        return (PFN_vkVoidFunction) &wrap_DestroyCommandPool<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VKROOTS_PLATFORM_UNSUPPORTED
    constexpr bool HasDestroyCuFunctionNVX = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroyCuFunctionNVX; };
    if constexpr (HasDestroyCuFunctionNVX) {
      if (!std::strcmp("vkDestroyCuFunctionNVX", name))
        return (PFN_vkVoidFunction) &wrap_DestroyCuFunctionNVX<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

#ifdef VKROOTS_PLATFORM_UNSUPPORTED
    constexpr bool HasDestroyCuModuleNVX = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroyCuModuleNVX; };
    if constexpr (HasDestroyCuModuleNVX) {
      if (!std::strcmp("vkDestroyCuModuleNVX", name))
        return (PFN_vkVoidFunction) &wrap_DestroyCuModuleNVX<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasDestroyDeferredOperationKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroyDeferredOperationKHR; };
    if constexpr (HasDestroyDeferredOperationKHR) {
      if (!std::strcmp("vkDestroyDeferredOperationKHR", name))
        return (PFN_vkVoidFunction) &wrap_DestroyDeferredOperationKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDestroyDescriptorPool = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroyDescriptorPool; };
    if constexpr (HasDestroyDescriptorPool) {
      if (!std::strcmp("vkDestroyDescriptorPool", name))
        return (PFN_vkVoidFunction) &wrap_DestroyDescriptorPool<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDestroyDescriptorSetLayout = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroyDescriptorSetLayout; };
    if constexpr (HasDestroyDescriptorSetLayout) {
      if (!std::strcmp("vkDestroyDescriptorSetLayout", name))
        return (PFN_vkVoidFunction) &wrap_DestroyDescriptorSetLayout<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDestroyDescriptorUpdateTemplate = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroyDescriptorUpdateTemplate; };
    if constexpr (HasDestroyDescriptorUpdateTemplate) {
      if (!std::strcmp("vkDestroyDescriptorUpdateTemplate", name))
        return (PFN_vkVoidFunction) &wrap_DestroyDescriptorUpdateTemplate<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDestroyDescriptorUpdateTemplateKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroyDescriptorUpdateTemplateKHR; };
    if constexpr (HasDestroyDescriptorUpdateTemplateKHR) {
      if (!std::strcmp("vkDestroyDescriptorUpdateTemplateKHR", name))
        return (PFN_vkVoidFunction) &wrap_DestroyDescriptorUpdateTemplateKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDestroyDevice = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroyDevice; };
    if constexpr (HasDestroyDevice) {
      if (!std::strcmp("vkDestroyDevice", name))
        return (PFN_vkVoidFunction) &wrap_DestroyDevice<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
    else {
      if (!std::strcmp("vkDestroyDevice", name))
        return (PFN_vkVoidFunction) &implicit_wrap_DestroyDevice<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDestroyEvent = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroyEvent; };
    if constexpr (HasDestroyEvent) {
      if (!std::strcmp("vkDestroyEvent", name))
        return (PFN_vkVoidFunction) &wrap_DestroyEvent<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDestroyFence = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroyFence; };
    if constexpr (HasDestroyFence) {
      if (!std::strcmp("vkDestroyFence", name))
        return (PFN_vkVoidFunction) &wrap_DestroyFence<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDestroyFramebuffer = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroyFramebuffer; };
    if constexpr (HasDestroyFramebuffer) {
      if (!std::strcmp("vkDestroyFramebuffer", name))
        return (PFN_vkVoidFunction) &wrap_DestroyFramebuffer<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDestroyImage = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroyImage; };
    if constexpr (HasDestroyImage) {
      if (!std::strcmp("vkDestroyImage", name))
        return (PFN_vkVoidFunction) &wrap_DestroyImage<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDestroyImageView = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroyImageView; };
    if constexpr (HasDestroyImageView) {
      if (!std::strcmp("vkDestroyImageView", name))
        return (PFN_vkVoidFunction) &wrap_DestroyImageView<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDestroyIndirectCommandsLayoutNV = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroyIndirectCommandsLayoutNV; };
    if constexpr (HasDestroyIndirectCommandsLayoutNV) {
      if (!std::strcmp("vkDestroyIndirectCommandsLayoutNV", name))
        return (PFN_vkVoidFunction) &wrap_DestroyIndirectCommandsLayoutNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDestroyPipeline = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroyPipeline; };
    if constexpr (HasDestroyPipeline) {
      if (!std::strcmp("vkDestroyPipeline", name))
        return (PFN_vkVoidFunction) &wrap_DestroyPipeline<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDestroyPipelineCache = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroyPipelineCache; };
    if constexpr (HasDestroyPipelineCache) {
      if (!std::strcmp("vkDestroyPipelineCache", name))
        return (PFN_vkVoidFunction) &wrap_DestroyPipelineCache<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDestroyPipelineLayout = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroyPipelineLayout; };
    if constexpr (HasDestroyPipelineLayout) {
      if (!std::strcmp("vkDestroyPipelineLayout", name))
        return (PFN_vkVoidFunction) &wrap_DestroyPipelineLayout<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDestroyPrivateDataSlot = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroyPrivateDataSlot; };
    if constexpr (HasDestroyPrivateDataSlot) {
      if (!std::strcmp("vkDestroyPrivateDataSlot", name))
        return (PFN_vkVoidFunction) &wrap_DestroyPrivateDataSlot<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDestroyPrivateDataSlotEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroyPrivateDataSlotEXT; };
    if constexpr (HasDestroyPrivateDataSlotEXT) {
      if (!std::strcmp("vkDestroyPrivateDataSlotEXT", name))
        return (PFN_vkVoidFunction) &wrap_DestroyPrivateDataSlotEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDestroyQueryPool = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroyQueryPool; };
    if constexpr (HasDestroyQueryPool) {
      if (!std::strcmp("vkDestroyQueryPool", name))
        return (PFN_vkVoidFunction) &wrap_DestroyQueryPool<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDestroyRenderPass = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroyRenderPass; };
    if constexpr (HasDestroyRenderPass) {
      if (!std::strcmp("vkDestroyRenderPass", name))
        return (PFN_vkVoidFunction) &wrap_DestroyRenderPass<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDestroySampler = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroySampler; };
    if constexpr (HasDestroySampler) {
      if (!std::strcmp("vkDestroySampler", name))
        return (PFN_vkVoidFunction) &wrap_DestroySampler<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDestroySamplerYcbcrConversion = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroySamplerYcbcrConversion; };
    if constexpr (HasDestroySamplerYcbcrConversion) {
      if (!std::strcmp("vkDestroySamplerYcbcrConversion", name))
        return (PFN_vkVoidFunction) &wrap_DestroySamplerYcbcrConversion<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDestroySamplerYcbcrConversionKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroySamplerYcbcrConversionKHR; };
    if constexpr (HasDestroySamplerYcbcrConversionKHR) {
      if (!std::strcmp("vkDestroySamplerYcbcrConversionKHR", name))
        return (PFN_vkVoidFunction) &wrap_DestroySamplerYcbcrConversionKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDestroySemaphore = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroySemaphore; };
    if constexpr (HasDestroySemaphore) {
      if (!std::strcmp("vkDestroySemaphore", name))
        return (PFN_vkVoidFunction) &wrap_DestroySemaphore<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDestroyShaderModule = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroyShaderModule; };
    if constexpr (HasDestroyShaderModule) {
      if (!std::strcmp("vkDestroyShaderModule", name))
        return (PFN_vkVoidFunction) &wrap_DestroyShaderModule<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDestroySwapchainKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroySwapchainKHR; };
    if constexpr (HasDestroySwapchainKHR) {
      if (!std::strcmp("vkDestroySwapchainKHR", name))
        return (PFN_vkVoidFunction) &wrap_DestroySwapchainKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDestroyValidationCacheEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroyValidationCacheEXT; };
    if constexpr (HasDestroyValidationCacheEXT) {
      if (!std::strcmp("vkDestroyValidationCacheEXT", name))
        return (PFN_vkVoidFunction) &wrap_DestroyValidationCacheEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VK_ENABLE_BETA_EXTENSIONS
    constexpr bool HasDestroyVideoSessionKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroyVideoSessionKHR; };
    if constexpr (HasDestroyVideoSessionKHR) {
      if (!std::strcmp("vkDestroyVideoSessionKHR", name))
        return (PFN_vkVoidFunction) &wrap_DestroyVideoSessionKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

#ifdef VK_ENABLE_BETA_EXTENSIONS
    constexpr bool HasDestroyVideoSessionParametersKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::DestroyVideoSessionParametersKHR; };
    if constexpr (HasDestroyVideoSessionParametersKHR) {
      if (!std::strcmp("vkDestroyVideoSessionParametersKHR", name))
        return (PFN_vkVoidFunction) &wrap_DestroyVideoSessionParametersKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasDeviceWaitIdle = requires(const DeviceOverrides& t) { &DeviceOverrides::DeviceWaitIdle; };
    if constexpr (HasDeviceWaitIdle) {
      if (!std::strcmp("vkDeviceWaitIdle", name))
        return (PFN_vkVoidFunction) &wrap_DeviceWaitIdle<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasDisplayPowerControlEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::DisplayPowerControlEXT; };
    if constexpr (HasDisplayPowerControlEXT) {
      if (!std::strcmp("vkDisplayPowerControlEXT", name))
        return (PFN_vkVoidFunction) &wrap_DisplayPowerControlEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasEndCommandBuffer = requires(const DeviceOverrides& t) { &DeviceOverrides::EndCommandBuffer; };
    if constexpr (HasEndCommandBuffer) {
      if (!std::strcmp("vkEndCommandBuffer", name))
        return (PFN_vkVoidFunction) &wrap_EndCommandBuffer<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VK_USE_PLATFORM_METAL_EXT
    constexpr bool HasExportMetalObjectsEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::ExportMetalObjectsEXT; };
    if constexpr (HasExportMetalObjectsEXT) {
      if (!std::strcmp("vkExportMetalObjectsEXT", name))
        return (PFN_vkVoidFunction) &wrap_ExportMetalObjectsEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasFlushMappedMemoryRanges = requires(const DeviceOverrides& t) { &DeviceOverrides::FlushMappedMemoryRanges; };
    if constexpr (HasFlushMappedMemoryRanges) {
      if (!std::strcmp("vkFlushMappedMemoryRanges", name))
        return (PFN_vkVoidFunction) &wrap_FlushMappedMemoryRanges<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasFreeCommandBuffers = requires(const DeviceOverrides& t) { &DeviceOverrides::FreeCommandBuffers; };
    if constexpr (HasFreeCommandBuffers) {
      if (!std::strcmp("vkFreeCommandBuffers", name))
        return (PFN_vkVoidFunction) &wrap_FreeCommandBuffers<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasFreeDescriptorSets = requires(const DeviceOverrides& t) { &DeviceOverrides::FreeDescriptorSets; };
    if constexpr (HasFreeDescriptorSets) {
      if (!std::strcmp("vkFreeDescriptorSets", name))
        return (PFN_vkVoidFunction) &wrap_FreeDescriptorSets<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasFreeMemory = requires(const DeviceOverrides& t) { &DeviceOverrides::FreeMemory; };
    if constexpr (HasFreeMemory) {
      if (!std::strcmp("vkFreeMemory", name))
        return (PFN_vkVoidFunction) &wrap_FreeMemory<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetAccelerationStructureBuildSizesKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetAccelerationStructureBuildSizesKHR; };
    if constexpr (HasGetAccelerationStructureBuildSizesKHR) {
      if (!std::strcmp("vkGetAccelerationStructureBuildSizesKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetAccelerationStructureBuildSizesKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetAccelerationStructureDeviceAddressKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetAccelerationStructureDeviceAddressKHR; };
    if constexpr (HasGetAccelerationStructureDeviceAddressKHR) {
      if (!std::strcmp("vkGetAccelerationStructureDeviceAddressKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetAccelerationStructureDeviceAddressKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetAccelerationStructureHandleNV = requires(const DeviceOverrides& t) { &DeviceOverrides::GetAccelerationStructureHandleNV; };
    if constexpr (HasGetAccelerationStructureHandleNV) {
      if (!std::strcmp("vkGetAccelerationStructureHandleNV", name))
        return (PFN_vkVoidFunction) &wrap_GetAccelerationStructureHandleNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetAccelerationStructureMemoryRequirementsNV = requires(const DeviceOverrides& t) { &DeviceOverrides::GetAccelerationStructureMemoryRequirementsNV; };
    if constexpr (HasGetAccelerationStructureMemoryRequirementsNV) {
      if (!std::strcmp("vkGetAccelerationStructureMemoryRequirementsNV", name))
        return (PFN_vkVoidFunction) &wrap_GetAccelerationStructureMemoryRequirementsNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VK_USE_PLATFORM_ANDROID_KHR
    constexpr bool HasGetAndroidHardwareBufferPropertiesANDROID = requires(const DeviceOverrides& t) { &DeviceOverrides::GetAndroidHardwareBufferPropertiesANDROID; };
    if constexpr (HasGetAndroidHardwareBufferPropertiesANDROID) {
      if (!std::strcmp("vkGetAndroidHardwareBufferPropertiesANDROID", name))
        return (PFN_vkVoidFunction) &wrap_GetAndroidHardwareBufferPropertiesANDROID<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

#ifdef VK_USE_PLATFORM_FUCHSIA
    constexpr bool HasGetBufferCollectionPropertiesFUCHSIA = requires(const DeviceOverrides& t) { &DeviceOverrides::GetBufferCollectionPropertiesFUCHSIA; };
    if constexpr (HasGetBufferCollectionPropertiesFUCHSIA) {
      if (!std::strcmp("vkGetBufferCollectionPropertiesFUCHSIA", name))
        return (PFN_vkVoidFunction) &wrap_GetBufferCollectionPropertiesFUCHSIA<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasGetBufferDeviceAddress = requires(const DeviceOverrides& t) { &DeviceOverrides::GetBufferDeviceAddress; };
    if constexpr (HasGetBufferDeviceAddress) {
      if (!std::strcmp("vkGetBufferDeviceAddress", name))
        return (PFN_vkVoidFunction) &wrap_GetBufferDeviceAddress<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetBufferDeviceAddressEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::GetBufferDeviceAddressEXT; };
    if constexpr (HasGetBufferDeviceAddressEXT) {
      if (!std::strcmp("vkGetBufferDeviceAddressEXT", name))
        return (PFN_vkVoidFunction) &wrap_GetBufferDeviceAddressEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetBufferDeviceAddressKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetBufferDeviceAddressKHR; };
    if constexpr (HasGetBufferDeviceAddressKHR) {
      if (!std::strcmp("vkGetBufferDeviceAddressKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetBufferDeviceAddressKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetBufferMemoryRequirements = requires(const DeviceOverrides& t) { &DeviceOverrides::GetBufferMemoryRequirements; };
    if constexpr (HasGetBufferMemoryRequirements) {
      if (!std::strcmp("vkGetBufferMemoryRequirements", name))
        return (PFN_vkVoidFunction) &wrap_GetBufferMemoryRequirements<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetBufferMemoryRequirements2 = requires(const DeviceOverrides& t) { &DeviceOverrides::GetBufferMemoryRequirements2; };
    if constexpr (HasGetBufferMemoryRequirements2) {
      if (!std::strcmp("vkGetBufferMemoryRequirements2", name))
        return (PFN_vkVoidFunction) &wrap_GetBufferMemoryRequirements2<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetBufferMemoryRequirements2KHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetBufferMemoryRequirements2KHR; };
    if constexpr (HasGetBufferMemoryRequirements2KHR) {
      if (!std::strcmp("vkGetBufferMemoryRequirements2KHR", name))
        return (PFN_vkVoidFunction) &wrap_GetBufferMemoryRequirements2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetBufferOpaqueCaptureAddress = requires(const DeviceOverrides& t) { &DeviceOverrides::GetBufferOpaqueCaptureAddress; };
    if constexpr (HasGetBufferOpaqueCaptureAddress) {
      if (!std::strcmp("vkGetBufferOpaqueCaptureAddress", name))
        return (PFN_vkVoidFunction) &wrap_GetBufferOpaqueCaptureAddress<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetBufferOpaqueCaptureAddressKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetBufferOpaqueCaptureAddressKHR; };
    if constexpr (HasGetBufferOpaqueCaptureAddressKHR) {
      if (!std::strcmp("vkGetBufferOpaqueCaptureAddressKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetBufferOpaqueCaptureAddressKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetCalibratedTimestampsEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::GetCalibratedTimestampsEXT; };
    if constexpr (HasGetCalibratedTimestampsEXT) {
      if (!std::strcmp("vkGetCalibratedTimestampsEXT", name))
        return (PFN_vkVoidFunction) &wrap_GetCalibratedTimestampsEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetDeferredOperationMaxConcurrencyKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetDeferredOperationMaxConcurrencyKHR; };
    if constexpr (HasGetDeferredOperationMaxConcurrencyKHR) {
      if (!std::strcmp("vkGetDeferredOperationMaxConcurrencyKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetDeferredOperationMaxConcurrencyKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetDeferredOperationResultKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetDeferredOperationResultKHR; };
    if constexpr (HasGetDeferredOperationResultKHR) {
      if (!std::strcmp("vkGetDeferredOperationResultKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetDeferredOperationResultKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetDescriptorSetHostMappingVALVE = requires(const DeviceOverrides& t) { &DeviceOverrides::GetDescriptorSetHostMappingVALVE; };
    if constexpr (HasGetDescriptorSetHostMappingVALVE) {
      if (!std::strcmp("vkGetDescriptorSetHostMappingVALVE", name))
        return (PFN_vkVoidFunction) &wrap_GetDescriptorSetHostMappingVALVE<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetDescriptorSetLayoutHostMappingInfoVALVE = requires(const DeviceOverrides& t) { &DeviceOverrides::GetDescriptorSetLayoutHostMappingInfoVALVE; };
    if constexpr (HasGetDescriptorSetLayoutHostMappingInfoVALVE) {
      if (!std::strcmp("vkGetDescriptorSetLayoutHostMappingInfoVALVE", name))
        return (PFN_vkVoidFunction) &wrap_GetDescriptorSetLayoutHostMappingInfoVALVE<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetDescriptorSetLayoutSupport = requires(const DeviceOverrides& t) { &DeviceOverrides::GetDescriptorSetLayoutSupport; };
    if constexpr (HasGetDescriptorSetLayoutSupport) {
      if (!std::strcmp("vkGetDescriptorSetLayoutSupport", name))
        return (PFN_vkVoidFunction) &wrap_GetDescriptorSetLayoutSupport<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetDescriptorSetLayoutSupportKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetDescriptorSetLayoutSupportKHR; };
    if constexpr (HasGetDescriptorSetLayoutSupportKHR) {
      if (!std::strcmp("vkGetDescriptorSetLayoutSupportKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetDescriptorSetLayoutSupportKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetDeviceAccelerationStructureCompatibilityKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetDeviceAccelerationStructureCompatibilityKHR; };
    if constexpr (HasGetDeviceAccelerationStructureCompatibilityKHR) {
      if (!std::strcmp("vkGetDeviceAccelerationStructureCompatibilityKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetDeviceAccelerationStructureCompatibilityKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetDeviceBufferMemoryRequirements = requires(const DeviceOverrides& t) { &DeviceOverrides::GetDeviceBufferMemoryRequirements; };
    if constexpr (HasGetDeviceBufferMemoryRequirements) {
      if (!std::strcmp("vkGetDeviceBufferMemoryRequirements", name))
        return (PFN_vkVoidFunction) &wrap_GetDeviceBufferMemoryRequirements<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetDeviceBufferMemoryRequirementsKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetDeviceBufferMemoryRequirementsKHR; };
    if constexpr (HasGetDeviceBufferMemoryRequirementsKHR) {
      if (!std::strcmp("vkGetDeviceBufferMemoryRequirementsKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetDeviceBufferMemoryRequirementsKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetDeviceGroupPeerMemoryFeatures = requires(const DeviceOverrides& t) { &DeviceOverrides::GetDeviceGroupPeerMemoryFeatures; };
    if constexpr (HasGetDeviceGroupPeerMemoryFeatures) {
      if (!std::strcmp("vkGetDeviceGroupPeerMemoryFeatures", name))
        return (PFN_vkVoidFunction) &wrap_GetDeviceGroupPeerMemoryFeatures<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetDeviceGroupPeerMemoryFeaturesKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetDeviceGroupPeerMemoryFeaturesKHR; };
    if constexpr (HasGetDeviceGroupPeerMemoryFeaturesKHR) {
      if (!std::strcmp("vkGetDeviceGroupPeerMemoryFeaturesKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetDeviceGroupPeerMemoryFeaturesKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetDeviceGroupPresentCapabilitiesKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetDeviceGroupPresentCapabilitiesKHR; };
    if constexpr (HasGetDeviceGroupPresentCapabilitiesKHR) {
      if (!std::strcmp("vkGetDeviceGroupPresentCapabilitiesKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetDeviceGroupPresentCapabilitiesKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VK_USE_PLATFORM_WIN32_KHR
    constexpr bool HasGetDeviceGroupSurfacePresentModes2EXT = requires(const DeviceOverrides& t) { &DeviceOverrides::GetDeviceGroupSurfacePresentModes2EXT; };
    if constexpr (HasGetDeviceGroupSurfacePresentModes2EXT) {
      if (!std::strcmp("vkGetDeviceGroupSurfacePresentModes2EXT", name))
        return (PFN_vkVoidFunction) &wrap_GetDeviceGroupSurfacePresentModes2EXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasGetDeviceGroupSurfacePresentModesKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetDeviceGroupSurfacePresentModesKHR; };
    if constexpr (HasGetDeviceGroupSurfacePresentModesKHR) {
      if (!std::strcmp("vkGetDeviceGroupSurfacePresentModesKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetDeviceGroupSurfacePresentModesKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetDeviceImageMemoryRequirements = requires(const DeviceOverrides& t) { &DeviceOverrides::GetDeviceImageMemoryRequirements; };
    if constexpr (HasGetDeviceImageMemoryRequirements) {
      if (!std::strcmp("vkGetDeviceImageMemoryRequirements", name))
        return (PFN_vkVoidFunction) &wrap_GetDeviceImageMemoryRequirements<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetDeviceImageMemoryRequirementsKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetDeviceImageMemoryRequirementsKHR; };
    if constexpr (HasGetDeviceImageMemoryRequirementsKHR) {
      if (!std::strcmp("vkGetDeviceImageMemoryRequirementsKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetDeviceImageMemoryRequirementsKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetDeviceImageSparseMemoryRequirements = requires(const DeviceOverrides& t) { &DeviceOverrides::GetDeviceImageSparseMemoryRequirements; };
    if constexpr (HasGetDeviceImageSparseMemoryRequirements) {
      if (!std::strcmp("vkGetDeviceImageSparseMemoryRequirements", name))
        return (PFN_vkVoidFunction) &wrap_GetDeviceImageSparseMemoryRequirements<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetDeviceImageSparseMemoryRequirementsKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetDeviceImageSparseMemoryRequirementsKHR; };
    if constexpr (HasGetDeviceImageSparseMemoryRequirementsKHR) {
      if (!std::strcmp("vkGetDeviceImageSparseMemoryRequirementsKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetDeviceImageSparseMemoryRequirementsKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetDeviceMemoryCommitment = requires(const DeviceOverrides& t) { &DeviceOverrides::GetDeviceMemoryCommitment; };
    if constexpr (HasGetDeviceMemoryCommitment) {
      if (!std::strcmp("vkGetDeviceMemoryCommitment", name))
        return (PFN_vkVoidFunction) &wrap_GetDeviceMemoryCommitment<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetDeviceMemoryOpaqueCaptureAddress = requires(const DeviceOverrides& t) { &DeviceOverrides::GetDeviceMemoryOpaqueCaptureAddress; };
    if constexpr (HasGetDeviceMemoryOpaqueCaptureAddress) {
      if (!std::strcmp("vkGetDeviceMemoryOpaqueCaptureAddress", name))
        return (PFN_vkVoidFunction) &wrap_GetDeviceMemoryOpaqueCaptureAddress<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetDeviceMemoryOpaqueCaptureAddressKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetDeviceMemoryOpaqueCaptureAddressKHR; };
    if constexpr (HasGetDeviceMemoryOpaqueCaptureAddressKHR) {
      if (!std::strcmp("vkGetDeviceMemoryOpaqueCaptureAddressKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetDeviceMemoryOpaqueCaptureAddressKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    if (!std::strcmp("vkGetDeviceProcAddr", name))
      return (PFN_vkVoidFunction) &GetDeviceProcAddr<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;

    constexpr bool HasGetDeviceQueue = requires(const DeviceOverrides& t) { &DeviceOverrides::GetDeviceQueue; };
    if constexpr (HasGetDeviceQueue) {
      if (!std::strcmp("vkGetDeviceQueue", name))
        return (PFN_vkVoidFunction) &wrap_GetDeviceQueue<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetDeviceQueue2 = requires(const DeviceOverrides& t) { &DeviceOverrides::GetDeviceQueue2; };
    if constexpr (HasGetDeviceQueue2) {
      if (!std::strcmp("vkGetDeviceQueue2", name))
        return (PFN_vkVoidFunction) &wrap_GetDeviceQueue2<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI = requires(const DeviceOverrides& t) { &DeviceOverrides::GetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI; };
    if constexpr (HasGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI) {
      if (!std::strcmp("vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI", name))
        return (PFN_vkVoidFunction) &wrap_GetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetEventStatus = requires(const DeviceOverrides& t) { &DeviceOverrides::GetEventStatus; };
    if constexpr (HasGetEventStatus) {
      if (!std::strcmp("vkGetEventStatus", name))
        return (PFN_vkVoidFunction) &wrap_GetEventStatus<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetFenceFdKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetFenceFdKHR; };
    if constexpr (HasGetFenceFdKHR) {
      if (!std::strcmp("vkGetFenceFdKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetFenceFdKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetFenceStatus = requires(const DeviceOverrides& t) { &DeviceOverrides::GetFenceStatus; };
    if constexpr (HasGetFenceStatus) {
      if (!std::strcmp("vkGetFenceStatus", name))
        return (PFN_vkVoidFunction) &wrap_GetFenceStatus<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VK_USE_PLATFORM_WIN32_KHR
    constexpr bool HasGetFenceWin32HandleKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetFenceWin32HandleKHR; };
    if constexpr (HasGetFenceWin32HandleKHR) {
      if (!std::strcmp("vkGetFenceWin32HandleKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetFenceWin32HandleKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasGetGeneratedCommandsMemoryRequirementsNV = requires(const DeviceOverrides& t) { &DeviceOverrides::GetGeneratedCommandsMemoryRequirementsNV; };
    if constexpr (HasGetGeneratedCommandsMemoryRequirementsNV) {
      if (!std::strcmp("vkGetGeneratedCommandsMemoryRequirementsNV", name))
        return (PFN_vkVoidFunction) &wrap_GetGeneratedCommandsMemoryRequirementsNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetImageDrmFormatModifierPropertiesEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::GetImageDrmFormatModifierPropertiesEXT; };
    if constexpr (HasGetImageDrmFormatModifierPropertiesEXT) {
      if (!std::strcmp("vkGetImageDrmFormatModifierPropertiesEXT", name))
        return (PFN_vkVoidFunction) &wrap_GetImageDrmFormatModifierPropertiesEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetImageMemoryRequirements = requires(const DeviceOverrides& t) { &DeviceOverrides::GetImageMemoryRequirements; };
    if constexpr (HasGetImageMemoryRequirements) {
      if (!std::strcmp("vkGetImageMemoryRequirements", name))
        return (PFN_vkVoidFunction) &wrap_GetImageMemoryRequirements<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetImageMemoryRequirements2 = requires(const DeviceOverrides& t) { &DeviceOverrides::GetImageMemoryRequirements2; };
    if constexpr (HasGetImageMemoryRequirements2) {
      if (!std::strcmp("vkGetImageMemoryRequirements2", name))
        return (PFN_vkVoidFunction) &wrap_GetImageMemoryRequirements2<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetImageMemoryRequirements2KHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetImageMemoryRequirements2KHR; };
    if constexpr (HasGetImageMemoryRequirements2KHR) {
      if (!std::strcmp("vkGetImageMemoryRequirements2KHR", name))
        return (PFN_vkVoidFunction) &wrap_GetImageMemoryRequirements2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetImageSparseMemoryRequirements = requires(const DeviceOverrides& t) { &DeviceOverrides::GetImageSparseMemoryRequirements; };
    if constexpr (HasGetImageSparseMemoryRequirements) {
      if (!std::strcmp("vkGetImageSparseMemoryRequirements", name))
        return (PFN_vkVoidFunction) &wrap_GetImageSparseMemoryRequirements<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetImageSparseMemoryRequirements2 = requires(const DeviceOverrides& t) { &DeviceOverrides::GetImageSparseMemoryRequirements2; };
    if constexpr (HasGetImageSparseMemoryRequirements2) {
      if (!std::strcmp("vkGetImageSparseMemoryRequirements2", name))
        return (PFN_vkVoidFunction) &wrap_GetImageSparseMemoryRequirements2<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetImageSparseMemoryRequirements2KHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetImageSparseMemoryRequirements2KHR; };
    if constexpr (HasGetImageSparseMemoryRequirements2KHR) {
      if (!std::strcmp("vkGetImageSparseMemoryRequirements2KHR", name))
        return (PFN_vkVoidFunction) &wrap_GetImageSparseMemoryRequirements2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetImageSubresourceLayout = requires(const DeviceOverrides& t) { &DeviceOverrides::GetImageSubresourceLayout; };
    if constexpr (HasGetImageSubresourceLayout) {
      if (!std::strcmp("vkGetImageSubresourceLayout", name))
        return (PFN_vkVoidFunction) &wrap_GetImageSubresourceLayout<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetImageSubresourceLayout2EXT = requires(const DeviceOverrides& t) { &DeviceOverrides::GetImageSubresourceLayout2EXT; };
    if constexpr (HasGetImageSubresourceLayout2EXT) {
      if (!std::strcmp("vkGetImageSubresourceLayout2EXT", name))
        return (PFN_vkVoidFunction) &wrap_GetImageSubresourceLayout2EXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VKROOTS_PLATFORM_UNSUPPORTED
    constexpr bool HasGetImageViewAddressNVX = requires(const DeviceOverrides& t) { &DeviceOverrides::GetImageViewAddressNVX; };
    if constexpr (HasGetImageViewAddressNVX) {
      if (!std::strcmp("vkGetImageViewAddressNVX", name))
        return (PFN_vkVoidFunction) &wrap_GetImageViewAddressNVX<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

#ifdef VKROOTS_PLATFORM_UNSUPPORTED
    constexpr bool HasGetImageViewHandleNVX = requires(const DeviceOverrides& t) { &DeviceOverrides::GetImageViewHandleNVX; };
    if constexpr (HasGetImageViewHandleNVX) {
      if (!std::strcmp("vkGetImageViewHandleNVX", name))
        return (PFN_vkVoidFunction) &wrap_GetImageViewHandleNVX<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

#ifdef VK_USE_PLATFORM_ANDROID_KHR
    constexpr bool HasGetMemoryAndroidHardwareBufferANDROID = requires(const DeviceOverrides& t) { &DeviceOverrides::GetMemoryAndroidHardwareBufferANDROID; };
    if constexpr (HasGetMemoryAndroidHardwareBufferANDROID) {
      if (!std::strcmp("vkGetMemoryAndroidHardwareBufferANDROID", name))
        return (PFN_vkVoidFunction) &wrap_GetMemoryAndroidHardwareBufferANDROID<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasGetMemoryFdKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetMemoryFdKHR; };
    if constexpr (HasGetMemoryFdKHR) {
      if (!std::strcmp("vkGetMemoryFdKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetMemoryFdKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetMemoryFdPropertiesKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetMemoryFdPropertiesKHR; };
    if constexpr (HasGetMemoryFdPropertiesKHR) {
      if (!std::strcmp("vkGetMemoryFdPropertiesKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetMemoryFdPropertiesKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetMemoryHostPointerPropertiesEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::GetMemoryHostPointerPropertiesEXT; };
    if constexpr (HasGetMemoryHostPointerPropertiesEXT) {
      if (!std::strcmp("vkGetMemoryHostPointerPropertiesEXT", name))
        return (PFN_vkVoidFunction) &wrap_GetMemoryHostPointerPropertiesEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetMemoryRemoteAddressNV = requires(const DeviceOverrides& t) { &DeviceOverrides::GetMemoryRemoteAddressNV; };
    if constexpr (HasGetMemoryRemoteAddressNV) {
      if (!std::strcmp("vkGetMemoryRemoteAddressNV", name))
        return (PFN_vkVoidFunction) &wrap_GetMemoryRemoteAddressNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VK_USE_PLATFORM_WIN32_KHR
    constexpr bool HasGetMemoryWin32HandleKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetMemoryWin32HandleKHR; };
    if constexpr (HasGetMemoryWin32HandleKHR) {
      if (!std::strcmp("vkGetMemoryWin32HandleKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetMemoryWin32HandleKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

#ifdef VK_USE_PLATFORM_WIN32_KHR
    constexpr bool HasGetMemoryWin32HandleNV = requires(const DeviceOverrides& t) { &DeviceOverrides::GetMemoryWin32HandleNV; };
    if constexpr (HasGetMemoryWin32HandleNV) {
      if (!std::strcmp("vkGetMemoryWin32HandleNV", name))
        return (PFN_vkVoidFunction) &wrap_GetMemoryWin32HandleNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

#ifdef VK_USE_PLATFORM_WIN32_KHR
    constexpr bool HasGetMemoryWin32HandlePropertiesKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetMemoryWin32HandlePropertiesKHR; };
    if constexpr (HasGetMemoryWin32HandlePropertiesKHR) {
      if (!std::strcmp("vkGetMemoryWin32HandlePropertiesKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetMemoryWin32HandlePropertiesKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

#ifdef VK_USE_PLATFORM_FUCHSIA
    constexpr bool HasGetMemoryZirconHandleFUCHSIA = requires(const DeviceOverrides& t) { &DeviceOverrides::GetMemoryZirconHandleFUCHSIA; };
    if constexpr (HasGetMemoryZirconHandleFUCHSIA) {
      if (!std::strcmp("vkGetMemoryZirconHandleFUCHSIA", name))
        return (PFN_vkVoidFunction) &wrap_GetMemoryZirconHandleFUCHSIA<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

#ifdef VK_USE_PLATFORM_FUCHSIA
    constexpr bool HasGetMemoryZirconHandlePropertiesFUCHSIA = requires(const DeviceOverrides& t) { &DeviceOverrides::GetMemoryZirconHandlePropertiesFUCHSIA; };
    if constexpr (HasGetMemoryZirconHandlePropertiesFUCHSIA) {
      if (!std::strcmp("vkGetMemoryZirconHandlePropertiesFUCHSIA", name))
        return (PFN_vkVoidFunction) &wrap_GetMemoryZirconHandlePropertiesFUCHSIA<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasGetPastPresentationTimingGOOGLE = requires(const DeviceOverrides& t) { &DeviceOverrides::GetPastPresentationTimingGOOGLE; };
    if constexpr (HasGetPastPresentationTimingGOOGLE) {
      if (!std::strcmp("vkGetPastPresentationTimingGOOGLE", name))
        return (PFN_vkVoidFunction) &wrap_GetPastPresentationTimingGOOGLE<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPerformanceParameterINTEL = requires(const DeviceOverrides& t) { &DeviceOverrides::GetPerformanceParameterINTEL; };
    if constexpr (HasGetPerformanceParameterINTEL) {
      if (!std::strcmp("vkGetPerformanceParameterINTEL", name))
        return (PFN_vkVoidFunction) &wrap_GetPerformanceParameterINTEL<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPipelineCacheData = requires(const DeviceOverrides& t) { &DeviceOverrides::GetPipelineCacheData; };
    if constexpr (HasGetPipelineCacheData) {
      if (!std::strcmp("vkGetPipelineCacheData", name))
        return (PFN_vkVoidFunction) &wrap_GetPipelineCacheData<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPipelineExecutableInternalRepresentationsKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetPipelineExecutableInternalRepresentationsKHR; };
    if constexpr (HasGetPipelineExecutableInternalRepresentationsKHR) {
      if (!std::strcmp("vkGetPipelineExecutableInternalRepresentationsKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetPipelineExecutableInternalRepresentationsKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPipelineExecutablePropertiesKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetPipelineExecutablePropertiesKHR; };
    if constexpr (HasGetPipelineExecutablePropertiesKHR) {
      if (!std::strcmp("vkGetPipelineExecutablePropertiesKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetPipelineExecutablePropertiesKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPipelineExecutableStatisticsKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetPipelineExecutableStatisticsKHR; };
    if constexpr (HasGetPipelineExecutableStatisticsKHR) {
      if (!std::strcmp("vkGetPipelineExecutableStatisticsKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetPipelineExecutableStatisticsKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPipelinePropertiesEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::GetPipelinePropertiesEXT; };
    if constexpr (HasGetPipelinePropertiesEXT) {
      if (!std::strcmp("vkGetPipelinePropertiesEXT", name))
        return (PFN_vkVoidFunction) &wrap_GetPipelinePropertiesEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPrivateData = requires(const DeviceOverrides& t) { &DeviceOverrides::GetPrivateData; };
    if constexpr (HasGetPrivateData) {
      if (!std::strcmp("vkGetPrivateData", name))
        return (PFN_vkVoidFunction) &wrap_GetPrivateData<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetPrivateDataEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::GetPrivateDataEXT; };
    if constexpr (HasGetPrivateDataEXT) {
      if (!std::strcmp("vkGetPrivateDataEXT", name))
        return (PFN_vkVoidFunction) &wrap_GetPrivateDataEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetQueryPoolResults = requires(const DeviceOverrides& t) { &DeviceOverrides::GetQueryPoolResults; };
    if constexpr (HasGetQueryPoolResults) {
      if (!std::strcmp("vkGetQueryPoolResults", name))
        return (PFN_vkVoidFunction) &wrap_GetQueryPoolResults<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetQueueCheckpointData2NV = requires(const DeviceOverrides& t) { &DeviceOverrides::GetQueueCheckpointData2NV; };
    if constexpr (HasGetQueueCheckpointData2NV) {
      if (!std::strcmp("vkGetQueueCheckpointData2NV", name))
        return (PFN_vkVoidFunction) &wrap_GetQueueCheckpointData2NV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetQueueCheckpointDataNV = requires(const DeviceOverrides& t) { &DeviceOverrides::GetQueueCheckpointDataNV; };
    if constexpr (HasGetQueueCheckpointDataNV) {
      if (!std::strcmp("vkGetQueueCheckpointDataNV", name))
        return (PFN_vkVoidFunction) &wrap_GetQueueCheckpointDataNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetRayTracingCaptureReplayShaderGroupHandlesKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetRayTracingCaptureReplayShaderGroupHandlesKHR; };
    if constexpr (HasGetRayTracingCaptureReplayShaderGroupHandlesKHR) {
      if (!std::strcmp("vkGetRayTracingCaptureReplayShaderGroupHandlesKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetRayTracingCaptureReplayShaderGroupHandlesKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetRayTracingShaderGroupHandlesKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetRayTracingShaderGroupHandlesKHR; };
    if constexpr (HasGetRayTracingShaderGroupHandlesKHR) {
      if (!std::strcmp("vkGetRayTracingShaderGroupHandlesKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetRayTracingShaderGroupHandlesKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetRayTracingShaderGroupHandlesNV = requires(const DeviceOverrides& t) { &DeviceOverrides::GetRayTracingShaderGroupHandlesNV; };
    if constexpr (HasGetRayTracingShaderGroupHandlesNV) {
      if (!std::strcmp("vkGetRayTracingShaderGroupHandlesNV", name))
        return (PFN_vkVoidFunction) &wrap_GetRayTracingShaderGroupHandlesNV<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetRayTracingShaderGroupStackSizeKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetRayTracingShaderGroupStackSizeKHR; };
    if constexpr (HasGetRayTracingShaderGroupStackSizeKHR) {
      if (!std::strcmp("vkGetRayTracingShaderGroupStackSizeKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetRayTracingShaderGroupStackSizeKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetRefreshCycleDurationGOOGLE = requires(const DeviceOverrides& t) { &DeviceOverrides::GetRefreshCycleDurationGOOGLE; };
    if constexpr (HasGetRefreshCycleDurationGOOGLE) {
      if (!std::strcmp("vkGetRefreshCycleDurationGOOGLE", name))
        return (PFN_vkVoidFunction) &wrap_GetRefreshCycleDurationGOOGLE<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetRenderAreaGranularity = requires(const DeviceOverrides& t) { &DeviceOverrides::GetRenderAreaGranularity; };
    if constexpr (HasGetRenderAreaGranularity) {
      if (!std::strcmp("vkGetRenderAreaGranularity", name))
        return (PFN_vkVoidFunction) &wrap_GetRenderAreaGranularity<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetSemaphoreCounterValue = requires(const DeviceOverrides& t) { &DeviceOverrides::GetSemaphoreCounterValue; };
    if constexpr (HasGetSemaphoreCounterValue) {
      if (!std::strcmp("vkGetSemaphoreCounterValue", name))
        return (PFN_vkVoidFunction) &wrap_GetSemaphoreCounterValue<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetSemaphoreCounterValueKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetSemaphoreCounterValueKHR; };
    if constexpr (HasGetSemaphoreCounterValueKHR) {
      if (!std::strcmp("vkGetSemaphoreCounterValueKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetSemaphoreCounterValueKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetSemaphoreFdKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetSemaphoreFdKHR; };
    if constexpr (HasGetSemaphoreFdKHR) {
      if (!std::strcmp("vkGetSemaphoreFdKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetSemaphoreFdKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VK_USE_PLATFORM_WIN32_KHR
    constexpr bool HasGetSemaphoreWin32HandleKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetSemaphoreWin32HandleKHR; };
    if constexpr (HasGetSemaphoreWin32HandleKHR) {
      if (!std::strcmp("vkGetSemaphoreWin32HandleKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetSemaphoreWin32HandleKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

#ifdef VK_USE_PLATFORM_FUCHSIA
    constexpr bool HasGetSemaphoreZirconHandleFUCHSIA = requires(const DeviceOverrides& t) { &DeviceOverrides::GetSemaphoreZirconHandleFUCHSIA; };
    if constexpr (HasGetSemaphoreZirconHandleFUCHSIA) {
      if (!std::strcmp("vkGetSemaphoreZirconHandleFUCHSIA", name))
        return (PFN_vkVoidFunction) &wrap_GetSemaphoreZirconHandleFUCHSIA<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasGetShaderInfoAMD = requires(const DeviceOverrides& t) { &DeviceOverrides::GetShaderInfoAMD; };
    if constexpr (HasGetShaderInfoAMD) {
      if (!std::strcmp("vkGetShaderInfoAMD", name))
        return (PFN_vkVoidFunction) &wrap_GetShaderInfoAMD<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetShaderModuleCreateInfoIdentifierEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::GetShaderModuleCreateInfoIdentifierEXT; };
    if constexpr (HasGetShaderModuleCreateInfoIdentifierEXT) {
      if (!std::strcmp("vkGetShaderModuleCreateInfoIdentifierEXT", name))
        return (PFN_vkVoidFunction) &wrap_GetShaderModuleCreateInfoIdentifierEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetShaderModuleIdentifierEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::GetShaderModuleIdentifierEXT; };
    if constexpr (HasGetShaderModuleIdentifierEXT) {
      if (!std::strcmp("vkGetShaderModuleIdentifierEXT", name))
        return (PFN_vkVoidFunction) &wrap_GetShaderModuleIdentifierEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetSwapchainCounterEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::GetSwapchainCounterEXT; };
    if constexpr (HasGetSwapchainCounterEXT) {
      if (!std::strcmp("vkGetSwapchainCounterEXT", name))
        return (PFN_vkVoidFunction) &wrap_GetSwapchainCounterEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VKROOTS_PLATFORM_UNSUPPORTED
    constexpr bool HasGetSwapchainGrallocUsage2ANDROID = requires(const DeviceOverrides& t) { &DeviceOverrides::GetSwapchainGrallocUsage2ANDROID; };
    if constexpr (HasGetSwapchainGrallocUsage2ANDROID) {
      if (!std::strcmp("vkGetSwapchainGrallocUsage2ANDROID", name))
        return (PFN_vkVoidFunction) &wrap_GetSwapchainGrallocUsage2ANDROID<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

#ifdef VKROOTS_PLATFORM_UNSUPPORTED
    constexpr bool HasGetSwapchainGrallocUsageANDROID = requires(const DeviceOverrides& t) { &DeviceOverrides::GetSwapchainGrallocUsageANDROID; };
    if constexpr (HasGetSwapchainGrallocUsageANDROID) {
      if (!std::strcmp("vkGetSwapchainGrallocUsageANDROID", name))
        return (PFN_vkVoidFunction) &wrap_GetSwapchainGrallocUsageANDROID<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasGetSwapchainImagesKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetSwapchainImagesKHR; };
    if constexpr (HasGetSwapchainImagesKHR) {
      if (!std::strcmp("vkGetSwapchainImagesKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetSwapchainImagesKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetSwapchainStatusKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetSwapchainStatusKHR; };
    if constexpr (HasGetSwapchainStatusKHR) {
      if (!std::strcmp("vkGetSwapchainStatusKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetSwapchainStatusKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasGetValidationCacheDataEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::GetValidationCacheDataEXT; };
    if constexpr (HasGetValidationCacheDataEXT) {
      if (!std::strcmp("vkGetValidationCacheDataEXT", name))
        return (PFN_vkVoidFunction) &wrap_GetValidationCacheDataEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VK_ENABLE_BETA_EXTENSIONS
    constexpr bool HasGetVideoSessionMemoryRequirementsKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::GetVideoSessionMemoryRequirementsKHR; };
    if constexpr (HasGetVideoSessionMemoryRequirementsKHR) {
      if (!std::strcmp("vkGetVideoSessionMemoryRequirementsKHR", name))
        return (PFN_vkVoidFunction) &wrap_GetVideoSessionMemoryRequirementsKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasImportFenceFdKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::ImportFenceFdKHR; };
    if constexpr (HasImportFenceFdKHR) {
      if (!std::strcmp("vkImportFenceFdKHR", name))
        return (PFN_vkVoidFunction) &wrap_ImportFenceFdKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VK_USE_PLATFORM_WIN32_KHR
    constexpr bool HasImportFenceWin32HandleKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::ImportFenceWin32HandleKHR; };
    if constexpr (HasImportFenceWin32HandleKHR) {
      if (!std::strcmp("vkImportFenceWin32HandleKHR", name))
        return (PFN_vkVoidFunction) &wrap_ImportFenceWin32HandleKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasImportSemaphoreFdKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::ImportSemaphoreFdKHR; };
    if constexpr (HasImportSemaphoreFdKHR) {
      if (!std::strcmp("vkImportSemaphoreFdKHR", name))
        return (PFN_vkVoidFunction) &wrap_ImportSemaphoreFdKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VK_USE_PLATFORM_WIN32_KHR
    constexpr bool HasImportSemaphoreWin32HandleKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::ImportSemaphoreWin32HandleKHR; };
    if constexpr (HasImportSemaphoreWin32HandleKHR) {
      if (!std::strcmp("vkImportSemaphoreWin32HandleKHR", name))
        return (PFN_vkVoidFunction) &wrap_ImportSemaphoreWin32HandleKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

#ifdef VK_USE_PLATFORM_FUCHSIA
    constexpr bool HasImportSemaphoreZirconHandleFUCHSIA = requires(const DeviceOverrides& t) { &DeviceOverrides::ImportSemaphoreZirconHandleFUCHSIA; };
    if constexpr (HasImportSemaphoreZirconHandleFUCHSIA) {
      if (!std::strcmp("vkImportSemaphoreZirconHandleFUCHSIA", name))
        return (PFN_vkVoidFunction) &wrap_ImportSemaphoreZirconHandleFUCHSIA<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasInitializePerformanceApiINTEL = requires(const DeviceOverrides& t) { &DeviceOverrides::InitializePerformanceApiINTEL; };
    if constexpr (HasInitializePerformanceApiINTEL) {
      if (!std::strcmp("vkInitializePerformanceApiINTEL", name))
        return (PFN_vkVoidFunction) &wrap_InitializePerformanceApiINTEL<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasInvalidateMappedMemoryRanges = requires(const DeviceOverrides& t) { &DeviceOverrides::InvalidateMappedMemoryRanges; };
    if constexpr (HasInvalidateMappedMemoryRanges) {
      if (!std::strcmp("vkInvalidateMappedMemoryRanges", name))
        return (PFN_vkVoidFunction) &wrap_InvalidateMappedMemoryRanges<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasMapMemory = requires(const DeviceOverrides& t) { &DeviceOverrides::MapMemory; };
    if constexpr (HasMapMemory) {
      if (!std::strcmp("vkMapMemory", name))
        return (PFN_vkVoidFunction) &wrap_MapMemory<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasMergePipelineCaches = requires(const DeviceOverrides& t) { &DeviceOverrides::MergePipelineCaches; };
    if constexpr (HasMergePipelineCaches) {
      if (!std::strcmp("vkMergePipelineCaches", name))
        return (PFN_vkVoidFunction) &wrap_MergePipelineCaches<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasMergeValidationCachesEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::MergeValidationCachesEXT; };
    if constexpr (HasMergeValidationCachesEXT) {
      if (!std::strcmp("vkMergeValidationCachesEXT", name))
        return (PFN_vkVoidFunction) &wrap_MergeValidationCachesEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasQueueBeginDebugUtilsLabelEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::QueueBeginDebugUtilsLabelEXT; };
    if constexpr (HasQueueBeginDebugUtilsLabelEXT) {
      if (!std::strcmp("vkQueueBeginDebugUtilsLabelEXT", name))
        return (PFN_vkVoidFunction) &wrap_QueueBeginDebugUtilsLabelEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasQueueBindSparse = requires(const DeviceOverrides& t) { &DeviceOverrides::QueueBindSparse; };
    if constexpr (HasQueueBindSparse) {
      if (!std::strcmp("vkQueueBindSparse", name))
        return (PFN_vkVoidFunction) &wrap_QueueBindSparse<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasQueueEndDebugUtilsLabelEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::QueueEndDebugUtilsLabelEXT; };
    if constexpr (HasQueueEndDebugUtilsLabelEXT) {
      if (!std::strcmp("vkQueueEndDebugUtilsLabelEXT", name))
        return (PFN_vkVoidFunction) &wrap_QueueEndDebugUtilsLabelEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasQueueInsertDebugUtilsLabelEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::QueueInsertDebugUtilsLabelEXT; };
    if constexpr (HasQueueInsertDebugUtilsLabelEXT) {
      if (!std::strcmp("vkQueueInsertDebugUtilsLabelEXT", name))
        return (PFN_vkVoidFunction) &wrap_QueueInsertDebugUtilsLabelEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasQueuePresentKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::QueuePresentKHR; };
    if constexpr (HasQueuePresentKHR) {
      if (!std::strcmp("vkQueuePresentKHR", name))
        return (PFN_vkVoidFunction) &wrap_QueuePresentKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasQueueSetPerformanceConfigurationINTEL = requires(const DeviceOverrides& t) { &DeviceOverrides::QueueSetPerformanceConfigurationINTEL; };
    if constexpr (HasQueueSetPerformanceConfigurationINTEL) {
      if (!std::strcmp("vkQueueSetPerformanceConfigurationINTEL", name))
        return (PFN_vkVoidFunction) &wrap_QueueSetPerformanceConfigurationINTEL<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VKROOTS_PLATFORM_UNSUPPORTED
    constexpr bool HasQueueSignalReleaseImageANDROID = requires(const DeviceOverrides& t) { &DeviceOverrides::QueueSignalReleaseImageANDROID; };
    if constexpr (HasQueueSignalReleaseImageANDROID) {
      if (!std::strcmp("vkQueueSignalReleaseImageANDROID", name))
        return (PFN_vkVoidFunction) &wrap_QueueSignalReleaseImageANDROID<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasQueueSubmit = requires(const DeviceOverrides& t) { &DeviceOverrides::QueueSubmit; };
    if constexpr (HasQueueSubmit) {
      if (!std::strcmp("vkQueueSubmit", name))
        return (PFN_vkVoidFunction) &wrap_QueueSubmit<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasQueueSubmit2 = requires(const DeviceOverrides& t) { &DeviceOverrides::QueueSubmit2; };
    if constexpr (HasQueueSubmit2) {
      if (!std::strcmp("vkQueueSubmit2", name))
        return (PFN_vkVoidFunction) &wrap_QueueSubmit2<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasQueueSubmit2KHR = requires(const DeviceOverrides& t) { &DeviceOverrides::QueueSubmit2KHR; };
    if constexpr (HasQueueSubmit2KHR) {
      if (!std::strcmp("vkQueueSubmit2KHR", name))
        return (PFN_vkVoidFunction) &wrap_QueueSubmit2KHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasQueueWaitIdle = requires(const DeviceOverrides& t) { &DeviceOverrides::QueueWaitIdle; };
    if constexpr (HasQueueWaitIdle) {
      if (!std::strcmp("vkQueueWaitIdle", name))
        return (PFN_vkVoidFunction) &wrap_QueueWaitIdle<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasRegisterDeviceEventEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::RegisterDeviceEventEXT; };
    if constexpr (HasRegisterDeviceEventEXT) {
      if (!std::strcmp("vkRegisterDeviceEventEXT", name))
        return (PFN_vkVoidFunction) &wrap_RegisterDeviceEventEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasRegisterDisplayEventEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::RegisterDisplayEventEXT; };
    if constexpr (HasRegisterDisplayEventEXT) {
      if (!std::strcmp("vkRegisterDisplayEventEXT", name))
        return (PFN_vkVoidFunction) &wrap_RegisterDisplayEventEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VK_USE_PLATFORM_WIN32_KHR
    constexpr bool HasReleaseFullScreenExclusiveModeEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::ReleaseFullScreenExclusiveModeEXT; };
    if constexpr (HasReleaseFullScreenExclusiveModeEXT) {
      if (!std::strcmp("vkReleaseFullScreenExclusiveModeEXT", name))
        return (PFN_vkVoidFunction) &wrap_ReleaseFullScreenExclusiveModeEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasReleasePerformanceConfigurationINTEL = requires(const DeviceOverrides& t) { &DeviceOverrides::ReleasePerformanceConfigurationINTEL; };
    if constexpr (HasReleasePerformanceConfigurationINTEL) {
      if (!std::strcmp("vkReleasePerformanceConfigurationINTEL", name))
        return (PFN_vkVoidFunction) &wrap_ReleasePerformanceConfigurationINTEL<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasReleaseProfilingLockKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::ReleaseProfilingLockKHR; };
    if constexpr (HasReleaseProfilingLockKHR) {
      if (!std::strcmp("vkReleaseProfilingLockKHR", name))
        return (PFN_vkVoidFunction) &wrap_ReleaseProfilingLockKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasResetCommandBuffer = requires(const DeviceOverrides& t) { &DeviceOverrides::ResetCommandBuffer; };
    if constexpr (HasResetCommandBuffer) {
      if (!std::strcmp("vkResetCommandBuffer", name))
        return (PFN_vkVoidFunction) &wrap_ResetCommandBuffer<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasResetCommandPool = requires(const DeviceOverrides& t) { &DeviceOverrides::ResetCommandPool; };
    if constexpr (HasResetCommandPool) {
      if (!std::strcmp("vkResetCommandPool", name))
        return (PFN_vkVoidFunction) &wrap_ResetCommandPool<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasResetDescriptorPool = requires(const DeviceOverrides& t) { &DeviceOverrides::ResetDescriptorPool; };
    if constexpr (HasResetDescriptorPool) {
      if (!std::strcmp("vkResetDescriptorPool", name))
        return (PFN_vkVoidFunction) &wrap_ResetDescriptorPool<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasResetEvent = requires(const DeviceOverrides& t) { &DeviceOverrides::ResetEvent; };
    if constexpr (HasResetEvent) {
      if (!std::strcmp("vkResetEvent", name))
        return (PFN_vkVoidFunction) &wrap_ResetEvent<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasResetFences = requires(const DeviceOverrides& t) { &DeviceOverrides::ResetFences; };
    if constexpr (HasResetFences) {
      if (!std::strcmp("vkResetFences", name))
        return (PFN_vkVoidFunction) &wrap_ResetFences<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasResetQueryPool = requires(const DeviceOverrides& t) { &DeviceOverrides::ResetQueryPool; };
    if constexpr (HasResetQueryPool) {
      if (!std::strcmp("vkResetQueryPool", name))
        return (PFN_vkVoidFunction) &wrap_ResetQueryPool<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasResetQueryPoolEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::ResetQueryPoolEXT; };
    if constexpr (HasResetQueryPoolEXT) {
      if (!std::strcmp("vkResetQueryPoolEXT", name))
        return (PFN_vkVoidFunction) &wrap_ResetQueryPoolEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VK_USE_PLATFORM_FUCHSIA
    constexpr bool HasSetBufferCollectionBufferConstraintsFUCHSIA = requires(const DeviceOverrides& t) { &DeviceOverrides::SetBufferCollectionBufferConstraintsFUCHSIA; };
    if constexpr (HasSetBufferCollectionBufferConstraintsFUCHSIA) {
      if (!std::strcmp("vkSetBufferCollectionBufferConstraintsFUCHSIA", name))
        return (PFN_vkVoidFunction) &wrap_SetBufferCollectionBufferConstraintsFUCHSIA<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

#ifdef VK_USE_PLATFORM_FUCHSIA
    constexpr bool HasSetBufferCollectionImageConstraintsFUCHSIA = requires(const DeviceOverrides& t) { &DeviceOverrides::SetBufferCollectionImageConstraintsFUCHSIA; };
    if constexpr (HasSetBufferCollectionImageConstraintsFUCHSIA) {
      if (!std::strcmp("vkSetBufferCollectionImageConstraintsFUCHSIA", name))
        return (PFN_vkVoidFunction) &wrap_SetBufferCollectionImageConstraintsFUCHSIA<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasSetDebugUtilsObjectNameEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::SetDebugUtilsObjectNameEXT; };
    if constexpr (HasSetDebugUtilsObjectNameEXT) {
      if (!std::strcmp("vkSetDebugUtilsObjectNameEXT", name))
        return (PFN_vkVoidFunction) &wrap_SetDebugUtilsObjectNameEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasSetDebugUtilsObjectTagEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::SetDebugUtilsObjectTagEXT; };
    if constexpr (HasSetDebugUtilsObjectTagEXT) {
      if (!std::strcmp("vkSetDebugUtilsObjectTagEXT", name))
        return (PFN_vkVoidFunction) &wrap_SetDebugUtilsObjectTagEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasSetDeviceMemoryPriorityEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::SetDeviceMemoryPriorityEXT; };
    if constexpr (HasSetDeviceMemoryPriorityEXT) {
      if (!std::strcmp("vkSetDeviceMemoryPriorityEXT", name))
        return (PFN_vkVoidFunction) &wrap_SetDeviceMemoryPriorityEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasSetEvent = requires(const DeviceOverrides& t) { &DeviceOverrides::SetEvent; };
    if constexpr (HasSetEvent) {
      if (!std::strcmp("vkSetEvent", name))
        return (PFN_vkVoidFunction) &wrap_SetEvent<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasSetHdrMetadataEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::SetHdrMetadataEXT; };
    if constexpr (HasSetHdrMetadataEXT) {
      if (!std::strcmp("vkSetHdrMetadataEXT", name))
        return (PFN_vkVoidFunction) &wrap_SetHdrMetadataEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasSetLocalDimmingAMD = requires(const DeviceOverrides& t) { &DeviceOverrides::SetLocalDimmingAMD; };
    if constexpr (HasSetLocalDimmingAMD) {
      if (!std::strcmp("vkSetLocalDimmingAMD", name))
        return (PFN_vkVoidFunction) &wrap_SetLocalDimmingAMD<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasSetPrivateData = requires(const DeviceOverrides& t) { &DeviceOverrides::SetPrivateData; };
    if constexpr (HasSetPrivateData) {
      if (!std::strcmp("vkSetPrivateData", name))
        return (PFN_vkVoidFunction) &wrap_SetPrivateData<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasSetPrivateDataEXT = requires(const DeviceOverrides& t) { &DeviceOverrides::SetPrivateDataEXT; };
    if constexpr (HasSetPrivateDataEXT) {
      if (!std::strcmp("vkSetPrivateDataEXT", name))
        return (PFN_vkVoidFunction) &wrap_SetPrivateDataEXT<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasSignalSemaphore = requires(const DeviceOverrides& t) { &DeviceOverrides::SignalSemaphore; };
    if constexpr (HasSignalSemaphore) {
      if (!std::strcmp("vkSignalSemaphore", name))
        return (PFN_vkVoidFunction) &wrap_SignalSemaphore<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasSignalSemaphoreKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::SignalSemaphoreKHR; };
    if constexpr (HasSignalSemaphoreKHR) {
      if (!std::strcmp("vkSignalSemaphoreKHR", name))
        return (PFN_vkVoidFunction) &wrap_SignalSemaphoreKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasTrimCommandPool = requires(const DeviceOverrides& t) { &DeviceOverrides::TrimCommandPool; };
    if constexpr (HasTrimCommandPool) {
      if (!std::strcmp("vkTrimCommandPool", name))
        return (PFN_vkVoidFunction) &wrap_TrimCommandPool<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasTrimCommandPoolKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::TrimCommandPoolKHR; };
    if constexpr (HasTrimCommandPoolKHR) {
      if (!std::strcmp("vkTrimCommandPoolKHR", name))
        return (PFN_vkVoidFunction) &wrap_TrimCommandPoolKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasUninitializePerformanceApiINTEL = requires(const DeviceOverrides& t) { &DeviceOverrides::UninitializePerformanceApiINTEL; };
    if constexpr (HasUninitializePerformanceApiINTEL) {
      if (!std::strcmp("vkUninitializePerformanceApiINTEL", name))
        return (PFN_vkVoidFunction) &wrap_UninitializePerformanceApiINTEL<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasUnmapMemory = requires(const DeviceOverrides& t) { &DeviceOverrides::UnmapMemory; };
    if constexpr (HasUnmapMemory) {
      if (!std::strcmp("vkUnmapMemory", name))
        return (PFN_vkVoidFunction) &wrap_UnmapMemory<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasUpdateDescriptorSetWithTemplate = requires(const DeviceOverrides& t) { &DeviceOverrides::UpdateDescriptorSetWithTemplate; };
    if constexpr (HasUpdateDescriptorSetWithTemplate) {
      if (!std::strcmp("vkUpdateDescriptorSetWithTemplate", name))
        return (PFN_vkVoidFunction) &wrap_UpdateDescriptorSetWithTemplate<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasUpdateDescriptorSetWithTemplateKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::UpdateDescriptorSetWithTemplateKHR; };
    if constexpr (HasUpdateDescriptorSetWithTemplateKHR) {
      if (!std::strcmp("vkUpdateDescriptorSetWithTemplateKHR", name))
        return (PFN_vkVoidFunction) &wrap_UpdateDescriptorSetWithTemplateKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasUpdateDescriptorSets = requires(const DeviceOverrides& t) { &DeviceOverrides::UpdateDescriptorSets; };
    if constexpr (HasUpdateDescriptorSets) {
      if (!std::strcmp("vkUpdateDescriptorSets", name))
        return (PFN_vkVoidFunction) &wrap_UpdateDescriptorSets<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

#ifdef VK_ENABLE_BETA_EXTENSIONS
    constexpr bool HasUpdateVideoSessionParametersKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::UpdateVideoSessionParametersKHR; };
    if constexpr (HasUpdateVideoSessionParametersKHR) {
      if (!std::strcmp("vkUpdateVideoSessionParametersKHR", name))
        return (PFN_vkVoidFunction) &wrap_UpdateVideoSessionParametersKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }
#endif

    constexpr bool HasWaitForFences = requires(const DeviceOverrides& t) { &DeviceOverrides::WaitForFences; };
    if constexpr (HasWaitForFences) {
      if (!std::strcmp("vkWaitForFences", name))
        return (PFN_vkVoidFunction) &wrap_WaitForFences<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasWaitForPresentKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::WaitForPresentKHR; };
    if constexpr (HasWaitForPresentKHR) {
      if (!std::strcmp("vkWaitForPresentKHR", name))
        return (PFN_vkVoidFunction) &wrap_WaitForPresentKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasWaitSemaphores = requires(const DeviceOverrides& t) { &DeviceOverrides::WaitSemaphores; };
    if constexpr (HasWaitSemaphores) {
      if (!std::strcmp("vkWaitSemaphores", name))
        return (PFN_vkVoidFunction) &wrap_WaitSemaphores<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasWaitSemaphoresKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::WaitSemaphoresKHR; };
    if constexpr (HasWaitSemaphoresKHR) {
      if (!std::strcmp("vkWaitSemaphoresKHR", name))
        return (PFN_vkVoidFunction) &wrap_WaitSemaphoresKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    constexpr bool HasWriteAccelerationStructuresPropertiesKHR = requires(const DeviceOverrides& t) { &DeviceOverrides::WriteAccelerationStructuresPropertiesKHR; };
    if constexpr (HasWriteAccelerationStructuresPropertiesKHR) {
      if (!std::strcmp("vkWriteAccelerationStructuresPropertiesKHR", name))
        return (PFN_vkVoidFunction) &wrap_WriteAccelerationStructuresPropertiesKHR<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    }

    if (dispatch)
      return dispatch->GetDeviceProcAddr(device, name);
    else
      return NULL;
  }

  namespace helpers {
    template <typename EnumType>
    constexpr const char* enumString(EnumType type);

    template <> constexpr const char* enumString<VkAccelerationStructureBuildTypeKHR>(VkAccelerationStructureBuildTypeKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_ACCELERATION_STRUCTURE_BUILD_TYPE_HOST_KHR";
        case static_cast<uint64_t>(1): return "VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR";
        case static_cast<uint64_t>(2): return "VK_ACCELERATION_STRUCTURE_BUILD_TYPE_HOST_OR_DEVICE_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_ACCELERATION_STRUCTURE_BUILD_TYPE_KHR_MAX_ENUM";
        default: return "VkAccelerationStructureBuildTypeKHR_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkAccelerationStructureCompatibilityKHR>(VkAccelerationStructureCompatibilityKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_ACCELERATION_STRUCTURE_COMPATIBILITY_COMPATIBLE_KHR";
        case static_cast<uint64_t>(1): return "VK_ACCELERATION_STRUCTURE_COMPATIBILITY_INCOMPATIBLE_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_ACCELERATION_STRUCTURE_COMPATIBILITY_KHR_MAX_ENUM";
        default: return "VkAccelerationStructureCompatibilityKHR_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkAccelerationStructureCreateFlagBitsKHR>(VkAccelerationStructureCreateFlagBitsKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_ACCELERATION_STRUCTURE_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_ACCELERATION_STRUCTURE_CREATE_FLAG_BITS_KHR_MAX_ENUM";
        case static_cast<uint64_t>(4): return "VK_ACCELERATION_STRUCTURE_CREATE_MOTION_BIT_NV";
        default: return "VkAccelerationStructureCreateFlagBitsKHR_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkAccelerationStructureMemoryRequirementsTypeNV>(VkAccelerationStructureMemoryRequirementsTypeNV type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV";
        case static_cast<uint64_t>(1): return "VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV";
        case static_cast<uint64_t>(2): return "VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_UPDATE_SCRATCH_NV";
        case static_cast<uint64_t>(2147483647): return "VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_NV_MAX_ENUM";
        default: return "VkAccelerationStructureMemoryRequirementsTypeNV_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkAccelerationStructureMotionInstanceTypeNV>(VkAccelerationStructureMotionInstanceTypeNV type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_ACCELERATION_STRUCTURE_MOTION_INSTANCE_TYPE_STATIC_NV";
        case static_cast<uint64_t>(1): return "VK_ACCELERATION_STRUCTURE_MOTION_INSTANCE_TYPE_MATRIX_MOTION_NV";
        case static_cast<uint64_t>(2): return "VK_ACCELERATION_STRUCTURE_MOTION_INSTANCE_TYPE_SRT_MOTION_NV";
        case static_cast<uint64_t>(2147483647): return "VK_ACCELERATION_STRUCTURE_MOTION_INSTANCE_TYPE_NV_MAX_ENUM";
        default: return "VkAccelerationStructureMotionInstanceTypeNV_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkAccelerationStructureTypeKHR>(VkAccelerationStructureTypeKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR";
        case static_cast<uint64_t>(1): return "VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR";
        case static_cast<uint64_t>(2): return "VK_ACCELERATION_STRUCTURE_TYPE_GENERIC_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_ACCELERATION_STRUCTURE_TYPE_KHR_MAX_ENUM";
        default: return "VkAccelerationStructureTypeKHR_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkAccessFlagBits>(VkAccessFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_ACCESS_INDIRECT_COMMAND_READ_BIT";
        case static_cast<uint64_t>(2): return "VK_ACCESS_INDEX_READ_BIT";
        case static_cast<uint64_t>(4): return "VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT";
        case static_cast<uint64_t>(8): return "VK_ACCESS_UNIFORM_READ_BIT";
        case static_cast<uint64_t>(16): return "VK_ACCESS_INPUT_ATTACHMENT_READ_BIT";
        case static_cast<uint64_t>(32): return "VK_ACCESS_SHADER_READ_BIT";
        case static_cast<uint64_t>(64): return "VK_ACCESS_SHADER_WRITE_BIT";
        case static_cast<uint64_t>(128): return "VK_ACCESS_COLOR_ATTACHMENT_READ_BIT";
        case static_cast<uint64_t>(256): return "VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT";
        case static_cast<uint64_t>(512): return "VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT";
        case static_cast<uint64_t>(1024): return "VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT";
        case static_cast<uint64_t>(2048): return "VK_ACCESS_TRANSFER_READ_BIT";
        case static_cast<uint64_t>(4096): return "VK_ACCESS_TRANSFER_WRITE_BIT";
        case static_cast<uint64_t>(8192): return "VK_ACCESS_HOST_READ_BIT";
        case static_cast<uint64_t>(16384): return "VK_ACCESS_HOST_WRITE_BIT";
        case static_cast<uint64_t>(32768): return "VK_ACCESS_MEMORY_READ_BIT";
        case static_cast<uint64_t>(65536): return "VK_ACCESS_MEMORY_WRITE_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_ACCESS_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(0): return "VK_ACCESS_NONE";
        case static_cast<uint64_t>(33554432): return "VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT";
        case static_cast<uint64_t>(67108864): return "VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT";
        case static_cast<uint64_t>(134217728): return "VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT";
        case static_cast<uint64_t>(1048576): return "VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT";
        case static_cast<uint64_t>(524288): return "VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT";
        case static_cast<uint64_t>(16777216): return "VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT";
        case static_cast<uint64_t>(8388608): return "VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR";
        case static_cast<uint64_t>(131072): return "VK_ACCESS_COMMAND_PREPROCESS_READ_BIT_NV";
        case static_cast<uint64_t>(262144): return "VK_ACCESS_COMMAND_PREPROCESS_WRITE_BIT_NV";
        case static_cast<uint64_t>(2097152): return "VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR";
        case static_cast<uint64_t>(4194304): return "VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR";
        default: return "VkAccessFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkAttachmentDescriptionFlagBits>(VkAttachmentDescriptionFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_ATTACHMENT_DESCRIPTION_FLAG_BITS_MAX_ENUM";
        default: return "VkAttachmentDescriptionFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkAttachmentLoadOp>(VkAttachmentLoadOp type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_ATTACHMENT_LOAD_OP_LOAD";
        case static_cast<uint64_t>(1): return "VK_ATTACHMENT_LOAD_OP_CLEAR";
        case static_cast<uint64_t>(2): return "VK_ATTACHMENT_LOAD_OP_DONT_CARE";
        case static_cast<uint64_t>(2147483647): return "VK_ATTACHMENT_LOAD_OP_MAX_ENUM";
        case static_cast<uint64_t>(1000400000): return "VK_ATTACHMENT_LOAD_OP_NONE_EXT";
        default: return "VkAttachmentLoadOp_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkAttachmentStoreOp>(VkAttachmentStoreOp type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_ATTACHMENT_STORE_OP_STORE";
        case static_cast<uint64_t>(1): return "VK_ATTACHMENT_STORE_OP_DONT_CARE";
        case static_cast<uint64_t>(2147483647): return "VK_ATTACHMENT_STORE_OP_MAX_ENUM";
        case static_cast<uint64_t>(1000301000): return "VK_ATTACHMENT_STORE_OP_NONE";
        default: return "VkAttachmentStoreOp_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkBlendFactor>(VkBlendFactor type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_BLEND_FACTOR_ZERO";
        case static_cast<uint64_t>(1): return "VK_BLEND_FACTOR_ONE";
        case static_cast<uint64_t>(2): return "VK_BLEND_FACTOR_SRC_COLOR";
        case static_cast<uint64_t>(3): return "VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR";
        case static_cast<uint64_t>(4): return "VK_BLEND_FACTOR_DST_COLOR";
        case static_cast<uint64_t>(5): return "VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR";
        case static_cast<uint64_t>(6): return "VK_BLEND_FACTOR_SRC_ALPHA";
        case static_cast<uint64_t>(7): return "VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA";
        case static_cast<uint64_t>(8): return "VK_BLEND_FACTOR_DST_ALPHA";
        case static_cast<uint64_t>(9): return "VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA";
        case static_cast<uint64_t>(10): return "VK_BLEND_FACTOR_CONSTANT_COLOR";
        case static_cast<uint64_t>(11): return "VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR";
        case static_cast<uint64_t>(12): return "VK_BLEND_FACTOR_CONSTANT_ALPHA";
        case static_cast<uint64_t>(13): return "VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA";
        case static_cast<uint64_t>(14): return "VK_BLEND_FACTOR_SRC_ALPHA_SATURATE";
        case static_cast<uint64_t>(15): return "VK_BLEND_FACTOR_SRC1_COLOR";
        case static_cast<uint64_t>(16): return "VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR";
        case static_cast<uint64_t>(17): return "VK_BLEND_FACTOR_SRC1_ALPHA";
        case static_cast<uint64_t>(18): return "VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA";
        case static_cast<uint64_t>(2147483647): return "VK_BLEND_FACTOR_MAX_ENUM";
        default: return "VkBlendFactor_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkBlendOp>(VkBlendOp type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_BLEND_OP_ADD";
        case static_cast<uint64_t>(1): return "VK_BLEND_OP_SUBTRACT";
        case static_cast<uint64_t>(2): return "VK_BLEND_OP_REVERSE_SUBTRACT";
        case static_cast<uint64_t>(3): return "VK_BLEND_OP_MIN";
        case static_cast<uint64_t>(4): return "VK_BLEND_OP_MAX";
        case static_cast<uint64_t>(2147483647): return "VK_BLEND_OP_MAX_ENUM";
        case static_cast<uint64_t>(1000148000): return "VK_BLEND_OP_ZERO_EXT";
        case static_cast<uint64_t>(1000148001): return "VK_BLEND_OP_SRC_EXT";
        case static_cast<uint64_t>(1000148002): return "VK_BLEND_OP_DST_EXT";
        case static_cast<uint64_t>(1000148003): return "VK_BLEND_OP_SRC_OVER_EXT";
        case static_cast<uint64_t>(1000148004): return "VK_BLEND_OP_DST_OVER_EXT";
        case static_cast<uint64_t>(1000148005): return "VK_BLEND_OP_SRC_IN_EXT";
        case static_cast<uint64_t>(1000148006): return "VK_BLEND_OP_DST_IN_EXT";
        case static_cast<uint64_t>(1000148007): return "VK_BLEND_OP_SRC_OUT_EXT";
        case static_cast<uint64_t>(1000148008): return "VK_BLEND_OP_DST_OUT_EXT";
        case static_cast<uint64_t>(1000148009): return "VK_BLEND_OP_SRC_ATOP_EXT";
        case static_cast<uint64_t>(1000148010): return "VK_BLEND_OP_DST_ATOP_EXT";
        case static_cast<uint64_t>(1000148011): return "VK_BLEND_OP_XOR_EXT";
        case static_cast<uint64_t>(1000148012): return "VK_BLEND_OP_MULTIPLY_EXT";
        case static_cast<uint64_t>(1000148013): return "VK_BLEND_OP_SCREEN_EXT";
        case static_cast<uint64_t>(1000148014): return "VK_BLEND_OP_OVERLAY_EXT";
        case static_cast<uint64_t>(1000148015): return "VK_BLEND_OP_DARKEN_EXT";
        case static_cast<uint64_t>(1000148016): return "VK_BLEND_OP_LIGHTEN_EXT";
        case static_cast<uint64_t>(1000148017): return "VK_BLEND_OP_COLORDODGE_EXT";
        case static_cast<uint64_t>(1000148018): return "VK_BLEND_OP_COLORBURN_EXT";
        case static_cast<uint64_t>(1000148019): return "VK_BLEND_OP_HARDLIGHT_EXT";
        case static_cast<uint64_t>(1000148020): return "VK_BLEND_OP_SOFTLIGHT_EXT";
        case static_cast<uint64_t>(1000148021): return "VK_BLEND_OP_DIFFERENCE_EXT";
        case static_cast<uint64_t>(1000148022): return "VK_BLEND_OP_EXCLUSION_EXT";
        case static_cast<uint64_t>(1000148023): return "VK_BLEND_OP_INVERT_EXT";
        case static_cast<uint64_t>(1000148024): return "VK_BLEND_OP_INVERT_RGB_EXT";
        case static_cast<uint64_t>(1000148025): return "VK_BLEND_OP_LINEARDODGE_EXT";
        case static_cast<uint64_t>(1000148026): return "VK_BLEND_OP_LINEARBURN_EXT";
        case static_cast<uint64_t>(1000148027): return "VK_BLEND_OP_VIVIDLIGHT_EXT";
        case static_cast<uint64_t>(1000148028): return "VK_BLEND_OP_LINEARLIGHT_EXT";
        case static_cast<uint64_t>(1000148029): return "VK_BLEND_OP_PINLIGHT_EXT";
        case static_cast<uint64_t>(1000148030): return "VK_BLEND_OP_HARDMIX_EXT";
        case static_cast<uint64_t>(1000148031): return "VK_BLEND_OP_HSL_HUE_EXT";
        case static_cast<uint64_t>(1000148032): return "VK_BLEND_OP_HSL_SATURATION_EXT";
        case static_cast<uint64_t>(1000148033): return "VK_BLEND_OP_HSL_COLOR_EXT";
        case static_cast<uint64_t>(1000148034): return "VK_BLEND_OP_HSL_LUMINOSITY_EXT";
        case static_cast<uint64_t>(1000148035): return "VK_BLEND_OP_PLUS_EXT";
        case static_cast<uint64_t>(1000148036): return "VK_BLEND_OP_PLUS_CLAMPED_EXT";
        case static_cast<uint64_t>(1000148037): return "VK_BLEND_OP_PLUS_CLAMPED_ALPHA_EXT";
        case static_cast<uint64_t>(1000148038): return "VK_BLEND_OP_PLUS_DARKER_EXT";
        case static_cast<uint64_t>(1000148039): return "VK_BLEND_OP_MINUS_EXT";
        case static_cast<uint64_t>(1000148040): return "VK_BLEND_OP_MINUS_CLAMPED_EXT";
        case static_cast<uint64_t>(1000148041): return "VK_BLEND_OP_CONTRAST_EXT";
        case static_cast<uint64_t>(1000148042): return "VK_BLEND_OP_INVERT_OVG_EXT";
        case static_cast<uint64_t>(1000148043): return "VK_BLEND_OP_RED_EXT";
        case static_cast<uint64_t>(1000148044): return "VK_BLEND_OP_GREEN_EXT";
        case static_cast<uint64_t>(1000148045): return "VK_BLEND_OP_BLUE_EXT";
        default: return "VkBlendOp_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkBlendOverlapEXT>(VkBlendOverlapEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_BLEND_OVERLAP_UNCORRELATED_EXT";
        case static_cast<uint64_t>(1): return "VK_BLEND_OVERLAP_DISJOINT_EXT";
        case static_cast<uint64_t>(2): return "VK_BLEND_OVERLAP_CONJOINT_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_BLEND_OVERLAP_EXT_MAX_ENUM";
        default: return "VkBlendOverlapEXT_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkBorderColor>(VkBorderColor type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK";
        case static_cast<uint64_t>(1): return "VK_BORDER_COLOR_INT_TRANSPARENT_BLACK";
        case static_cast<uint64_t>(2): return "VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK";
        case static_cast<uint64_t>(3): return "VK_BORDER_COLOR_INT_OPAQUE_BLACK";
        case static_cast<uint64_t>(4): return "VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE";
        case static_cast<uint64_t>(5): return "VK_BORDER_COLOR_INT_OPAQUE_WHITE";
        case static_cast<uint64_t>(2147483647): return "VK_BORDER_COLOR_MAX_ENUM";
        case static_cast<uint64_t>(1000287003): return "VK_BORDER_COLOR_FLOAT_CUSTOM_EXT";
        case static_cast<uint64_t>(1000287004): return "VK_BORDER_COLOR_INT_CUSTOM_EXT";
        default: return "VkBorderColor_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkBufferCreateFlagBits>(VkBufferCreateFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_BUFFER_CREATE_SPARSE_BINDING_BIT";
        case static_cast<uint64_t>(2): return "VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT";
        case static_cast<uint64_t>(4): return "VK_BUFFER_CREATE_SPARSE_ALIASED_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_BUFFER_CREATE_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(8): return "VK_BUFFER_CREATE_PROTECTED_BIT";
        case static_cast<uint64_t>(16): return "VK_BUFFER_CREATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT";
        default: return "VkBufferCreateFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkBufferUsageFlagBits>(VkBufferUsageFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_BUFFER_USAGE_TRANSFER_SRC_BIT";
        case static_cast<uint64_t>(2): return "VK_BUFFER_USAGE_TRANSFER_DST_BIT";
        case static_cast<uint64_t>(4): return "VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT";
        case static_cast<uint64_t>(8): return "VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT";
        case static_cast<uint64_t>(16): return "VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT";
        case static_cast<uint64_t>(32): return "VK_BUFFER_USAGE_STORAGE_BUFFER_BIT";
        case static_cast<uint64_t>(64): return "VK_BUFFER_USAGE_INDEX_BUFFER_BIT";
        case static_cast<uint64_t>(128): return "VK_BUFFER_USAGE_VERTEX_BUFFER_BIT";
        case static_cast<uint64_t>(256): return "VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(131072): return "VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT";
        case static_cast<uint64_t>(8192): return "VK_BUFFER_USAGE_VIDEO_DECODE_SRC_BIT_KHR";
        case static_cast<uint64_t>(16384): return "VK_BUFFER_USAGE_VIDEO_DECODE_DST_BIT_KHR";
        case static_cast<uint64_t>(2048): return "VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT";
        case static_cast<uint64_t>(4096): return "VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT";
        case static_cast<uint64_t>(512): return "VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT";
        case static_cast<uint64_t>(32768): return "VK_BUFFER_USAGE_VIDEO_ENCODE_DST_BIT_KHR";
        case static_cast<uint64_t>(65536): return "VK_BUFFER_USAGE_VIDEO_ENCODE_SRC_BIT_KHR";
        case static_cast<uint64_t>(524288): return "VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR";
        case static_cast<uint64_t>(1048576): return "VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR";
        case static_cast<uint64_t>(1024): return "VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR";
        default: return "VkBufferUsageFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkBuildAccelerationStructureFlagBitsKHR>(VkBuildAccelerationStructureFlagBitsKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR";
        case static_cast<uint64_t>(2): return "VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR";
        case static_cast<uint64_t>(4): return "VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR";
        case static_cast<uint64_t>(8): return "VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR";
        case static_cast<uint64_t>(16): return "VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_BUILD_ACCELERATION_STRUCTURE_FLAG_BITS_KHR_MAX_ENUM";
        case static_cast<uint64_t>(32): return "VK_BUILD_ACCELERATION_STRUCTURE_MOTION_BIT_NV";
        default: return "VkBuildAccelerationStructureFlagBitsKHR_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkBuildAccelerationStructureModeKHR>(VkBuildAccelerationStructureModeKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR";
        case static_cast<uint64_t>(1): return "VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_BUILD_ACCELERATION_STRUCTURE_MODE_KHR_MAX_ENUM";
        default: return "VkBuildAccelerationStructureModeKHR_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkChromaLocation>(VkChromaLocation type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_CHROMA_LOCATION_COSITED_EVEN";
        case static_cast<uint64_t>(1): return "VK_CHROMA_LOCATION_MIDPOINT";
        case static_cast<uint64_t>(2147483647): return "VK_CHROMA_LOCATION_MAX_ENUM";
        default: return "VkChromaLocation_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkCoarseSampleOrderTypeNV>(VkCoarseSampleOrderTypeNV type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_COARSE_SAMPLE_ORDER_TYPE_DEFAULT_NV";
        case static_cast<uint64_t>(1): return "VK_COARSE_SAMPLE_ORDER_TYPE_CUSTOM_NV";
        case static_cast<uint64_t>(2): return "VK_COARSE_SAMPLE_ORDER_TYPE_PIXEL_MAJOR_NV";
        case static_cast<uint64_t>(3): return "VK_COARSE_SAMPLE_ORDER_TYPE_SAMPLE_MAJOR_NV";
        case static_cast<uint64_t>(2147483647): return "VK_COARSE_SAMPLE_ORDER_TYPE_NV_MAX_ENUM";
        default: return "VkCoarseSampleOrderTypeNV_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkColorComponentFlagBits>(VkColorComponentFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_COLOR_COMPONENT_R_BIT";
        case static_cast<uint64_t>(2): return "VK_COLOR_COMPONENT_G_BIT";
        case static_cast<uint64_t>(4): return "VK_COLOR_COMPONENT_B_BIT";
        case static_cast<uint64_t>(8): return "VK_COLOR_COMPONENT_A_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_COLOR_COMPONENT_FLAG_BITS_MAX_ENUM";
        default: return "VkColorComponentFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkColorSpaceKHR>(VkColorSpaceKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_COLOR_SPACE_SRGB_NONLINEAR_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_COLOR_SPACE_KHR_MAX_ENUM";
        case static_cast<uint64_t>(1000104001): return "VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT";
        case static_cast<uint64_t>(1000104002): return "VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT";
        case static_cast<uint64_t>(1000104003): return "VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT";
        case static_cast<uint64_t>(1000104004): return "VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT";
        case static_cast<uint64_t>(1000104005): return "VK_COLOR_SPACE_BT709_LINEAR_EXT";
        case static_cast<uint64_t>(1000104006): return "VK_COLOR_SPACE_BT709_NONLINEAR_EXT";
        case static_cast<uint64_t>(1000104007): return "VK_COLOR_SPACE_BT2020_LINEAR_EXT";
        case static_cast<uint64_t>(1000104008): return "VK_COLOR_SPACE_HDR10_ST2084_EXT";
        case static_cast<uint64_t>(1000104009): return "VK_COLOR_SPACE_DOLBYVISION_EXT";
        case static_cast<uint64_t>(1000104010): return "VK_COLOR_SPACE_HDR10_HLG_EXT";
        case static_cast<uint64_t>(1000104011): return "VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT";
        case static_cast<uint64_t>(1000104012): return "VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT";
        case static_cast<uint64_t>(1000104013): return "VK_COLOR_SPACE_PASS_THROUGH_EXT";
        case static_cast<uint64_t>(1000104014): return "VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT";
        case static_cast<uint64_t>(1000213000): return "VK_COLOR_SPACE_DISPLAY_NATIVE_AMD";
        default: return "VkColorSpaceKHR_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkCommandBufferLevel>(VkCommandBufferLevel type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_COMMAND_BUFFER_LEVEL_PRIMARY";
        case static_cast<uint64_t>(1): return "VK_COMMAND_BUFFER_LEVEL_SECONDARY";
        case static_cast<uint64_t>(2147483647): return "VK_COMMAND_BUFFER_LEVEL_MAX_ENUM";
        default: return "VkCommandBufferLevel_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkCommandBufferResetFlagBits>(VkCommandBufferResetFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_COMMAND_BUFFER_RESET_FLAG_BITS_MAX_ENUM";
        default: return "VkCommandBufferResetFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkCommandBufferUsageFlagBits>(VkCommandBufferUsageFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT";
        case static_cast<uint64_t>(2): return "VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT";
        case static_cast<uint64_t>(4): return "VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_COMMAND_BUFFER_USAGE_FLAG_BITS_MAX_ENUM";
        default: return "VkCommandBufferUsageFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkCommandPoolCreateFlagBits>(VkCommandPoolCreateFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_COMMAND_POOL_CREATE_TRANSIENT_BIT";
        case static_cast<uint64_t>(2): return "VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_COMMAND_POOL_CREATE_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(4): return "VK_COMMAND_POOL_CREATE_PROTECTED_BIT";
        default: return "VkCommandPoolCreateFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkCommandPoolResetFlagBits>(VkCommandPoolResetFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_COMMAND_POOL_RESET_FLAG_BITS_MAX_ENUM";
        default: return "VkCommandPoolResetFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkCompareOp>(VkCompareOp type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_COMPARE_OP_NEVER";
        case static_cast<uint64_t>(1): return "VK_COMPARE_OP_LESS";
        case static_cast<uint64_t>(2): return "VK_COMPARE_OP_EQUAL";
        case static_cast<uint64_t>(3): return "VK_COMPARE_OP_LESS_OR_EQUAL";
        case static_cast<uint64_t>(4): return "VK_COMPARE_OP_GREATER";
        case static_cast<uint64_t>(5): return "VK_COMPARE_OP_NOT_EQUAL";
        case static_cast<uint64_t>(6): return "VK_COMPARE_OP_GREATER_OR_EQUAL";
        case static_cast<uint64_t>(7): return "VK_COMPARE_OP_ALWAYS";
        case static_cast<uint64_t>(2147483647): return "VK_COMPARE_OP_MAX_ENUM";
        default: return "VkCompareOp_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkComponentSwizzle>(VkComponentSwizzle type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_COMPONENT_SWIZZLE_IDENTITY";
        case static_cast<uint64_t>(1): return "VK_COMPONENT_SWIZZLE_ZERO";
        case static_cast<uint64_t>(2): return "VK_COMPONENT_SWIZZLE_ONE";
        case static_cast<uint64_t>(3): return "VK_COMPONENT_SWIZZLE_R";
        case static_cast<uint64_t>(4): return "VK_COMPONENT_SWIZZLE_G";
        case static_cast<uint64_t>(5): return "VK_COMPONENT_SWIZZLE_B";
        case static_cast<uint64_t>(6): return "VK_COMPONENT_SWIZZLE_A";
        case static_cast<uint64_t>(2147483647): return "VK_COMPONENT_SWIZZLE_MAX_ENUM";
        default: return "VkComponentSwizzle_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkComponentTypeNV>(VkComponentTypeNV type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_COMPONENT_TYPE_FLOAT16_NV";
        case static_cast<uint64_t>(1): return "VK_COMPONENT_TYPE_FLOAT32_NV";
        case static_cast<uint64_t>(2): return "VK_COMPONENT_TYPE_FLOAT64_NV";
        case static_cast<uint64_t>(3): return "VK_COMPONENT_TYPE_SINT8_NV";
        case static_cast<uint64_t>(4): return "VK_COMPONENT_TYPE_SINT16_NV";
        case static_cast<uint64_t>(5): return "VK_COMPONENT_TYPE_SINT32_NV";
        case static_cast<uint64_t>(6): return "VK_COMPONENT_TYPE_SINT64_NV";
        case static_cast<uint64_t>(7): return "VK_COMPONENT_TYPE_UINT8_NV";
        case static_cast<uint64_t>(8): return "VK_COMPONENT_TYPE_UINT16_NV";
        case static_cast<uint64_t>(9): return "VK_COMPONENT_TYPE_UINT32_NV";
        case static_cast<uint64_t>(10): return "VK_COMPONENT_TYPE_UINT64_NV";
        case static_cast<uint64_t>(2147483647): return "VK_COMPONENT_TYPE_NV_MAX_ENUM";
        default: return "VkComponentTypeNV_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkCompositeAlphaFlagBitsKHR>(VkCompositeAlphaFlagBitsKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR";
        case static_cast<uint64_t>(2): return "VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR";
        case static_cast<uint64_t>(4): return "VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR";
        case static_cast<uint64_t>(8): return "VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_COMPOSITE_ALPHA_FLAG_BITS_KHR_MAX_ENUM";
        default: return "VkCompositeAlphaFlagBitsKHR_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkConditionalRenderingFlagBitsEXT>(VkConditionalRenderingFlagBitsEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_CONDITIONAL_RENDERING_INVERTED_BIT_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_CONDITIONAL_RENDERING_FLAG_BITS_EXT_MAX_ENUM";
        default: return "VkConditionalRenderingFlagBitsEXT_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkConservativeRasterizationModeEXT>(VkConservativeRasterizationModeEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_CONSERVATIVE_RASTERIZATION_MODE_DISABLED_EXT";
        case static_cast<uint64_t>(1): return "VK_CONSERVATIVE_RASTERIZATION_MODE_OVERESTIMATE_EXT";
        case static_cast<uint64_t>(2): return "VK_CONSERVATIVE_RASTERIZATION_MODE_UNDERESTIMATE_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_CONSERVATIVE_RASTERIZATION_MODE_EXT_MAX_ENUM";
        default: return "VkConservativeRasterizationModeEXT_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkCopyAccelerationStructureModeKHR>(VkCopyAccelerationStructureModeKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_COPY_ACCELERATION_STRUCTURE_MODE_CLONE_KHR";
        case static_cast<uint64_t>(1): return "VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR";
        case static_cast<uint64_t>(2): return "VK_COPY_ACCELERATION_STRUCTURE_MODE_SERIALIZE_KHR";
        case static_cast<uint64_t>(3): return "VK_COPY_ACCELERATION_STRUCTURE_MODE_DESERIALIZE_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_COPY_ACCELERATION_STRUCTURE_MODE_KHR_MAX_ENUM";
        default: return "VkCopyAccelerationStructureModeKHR_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkCoverageModulationModeNV>(VkCoverageModulationModeNV type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_COVERAGE_MODULATION_MODE_NONE_NV";
        case static_cast<uint64_t>(1): return "VK_COVERAGE_MODULATION_MODE_RGB_NV";
        case static_cast<uint64_t>(2): return "VK_COVERAGE_MODULATION_MODE_ALPHA_NV";
        case static_cast<uint64_t>(3): return "VK_COVERAGE_MODULATION_MODE_RGBA_NV";
        case static_cast<uint64_t>(2147483647): return "VK_COVERAGE_MODULATION_MODE_NV_MAX_ENUM";
        default: return "VkCoverageModulationModeNV_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkCoverageReductionModeNV>(VkCoverageReductionModeNV type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_COVERAGE_REDUCTION_MODE_MERGE_NV";
        case static_cast<uint64_t>(1): return "VK_COVERAGE_REDUCTION_MODE_TRUNCATE_NV";
        case static_cast<uint64_t>(2147483647): return "VK_COVERAGE_REDUCTION_MODE_NV_MAX_ENUM";
        default: return "VkCoverageReductionModeNV_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkCullModeFlagBits>(VkCullModeFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_CULL_MODE_NONE";
        case static_cast<uint64_t>(1): return "VK_CULL_MODE_FRONT_BIT";
        case static_cast<uint64_t>(2): return "VK_CULL_MODE_BACK_BIT";
        case static_cast<uint64_t>(3): return "VK_CULL_MODE_FRONT_AND_BACK";
        case static_cast<uint64_t>(2147483647): return "VK_CULL_MODE_FLAG_BITS_MAX_ENUM";
        default: return "VkCullModeFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkDebugReportFlagBitsEXT>(VkDebugReportFlagBitsEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_DEBUG_REPORT_INFORMATION_BIT_EXT";
        case static_cast<uint64_t>(2): return "VK_DEBUG_REPORT_WARNING_BIT_EXT";
        case static_cast<uint64_t>(4): return "VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT";
        case static_cast<uint64_t>(8): return "VK_DEBUG_REPORT_ERROR_BIT_EXT";
        case static_cast<uint64_t>(16): return "VK_DEBUG_REPORT_DEBUG_BIT_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_DEBUG_REPORT_FLAG_BITS_EXT_MAX_ENUM";
        default: return "VkDebugReportFlagBitsEXT_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkDebugReportObjectTypeEXT>(VkDebugReportObjectTypeEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT";
        case static_cast<uint64_t>(1): return "VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT";
        case static_cast<uint64_t>(2): return "VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT";
        case static_cast<uint64_t>(3): return "VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT";
        case static_cast<uint64_t>(4): return "VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT";
        case static_cast<uint64_t>(5): return "VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT";
        case static_cast<uint64_t>(6): return "VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT";
        case static_cast<uint64_t>(7): return "VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT";
        case static_cast<uint64_t>(8): return "VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT";
        case static_cast<uint64_t>(9): return "VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT";
        case static_cast<uint64_t>(10): return "VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT";
        case static_cast<uint64_t>(11): return "VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT";
        case static_cast<uint64_t>(12): return "VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT";
        case static_cast<uint64_t>(13): return "VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT";
        case static_cast<uint64_t>(14): return "VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT";
        case static_cast<uint64_t>(15): return "VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT";
        case static_cast<uint64_t>(16): return "VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT";
        case static_cast<uint64_t>(17): return "VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT";
        case static_cast<uint64_t>(18): return "VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT";
        case static_cast<uint64_t>(19): return "VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT";
        case static_cast<uint64_t>(20): return "VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT";
        case static_cast<uint64_t>(21): return "VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT";
        case static_cast<uint64_t>(22): return "VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT";
        case static_cast<uint64_t>(23): return "VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT";
        case static_cast<uint64_t>(24): return "VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT";
        case static_cast<uint64_t>(25): return "VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT";
        case static_cast<uint64_t>(26): return "VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT";
        case static_cast<uint64_t>(27): return "VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT";
        case static_cast<uint64_t>(28): return "VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT_EXT";
        case static_cast<uint64_t>(29): return "VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_KHR_EXT";
        case static_cast<uint64_t>(30): return "VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_MODE_KHR_EXT";
        case static_cast<uint64_t>(33): return "VK_DEBUG_REPORT_OBJECT_TYPE_VALIDATION_CACHE_EXT_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_DEBUG_REPORT_OBJECT_TYPE_EXT_MAX_ENUM";
        case static_cast<uint64_t>(1000156000): return "VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION_EXT";
        case static_cast<uint64_t>(1000085000): return "VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_EXT";
        case static_cast<uint64_t>(1000165000): return "VK_DEBUG_REPORT_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV_EXT";
        case static_cast<uint64_t>(1000366000): return "VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_COLLECTION_FUCHSIA_EXT";
        case static_cast<uint64_t>(1000150000): return "VK_DEBUG_REPORT_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR_EXT";
        default: return "VkDebugReportObjectTypeEXT_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkDebugUtilsMessageSeverityFlagBitsEXT>(VkDebugUtilsMessageSeverityFlagBitsEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT";
        case static_cast<uint64_t>(16): return "VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT";
        case static_cast<uint64_t>(256): return "VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT";
        case static_cast<uint64_t>(4096): return "VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_EXT_MAX_ENUM";
        default: return "VkDebugUtilsMessageSeverityFlagBitsEXT_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkDebugUtilsMessageTypeFlagBitsEXT>(VkDebugUtilsMessageTypeFlagBitsEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT";
        case static_cast<uint64_t>(2): return "VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT";
        case static_cast<uint64_t>(4): return "VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_DEBUG_UTILS_MESSAGE_TYPE_FLAG_BITS_EXT_MAX_ENUM";
        default: return "VkDebugUtilsMessageTypeFlagBitsEXT_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkDependencyFlagBits>(VkDependencyFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_DEPENDENCY_BY_REGION_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_DEPENDENCY_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(4): return "VK_DEPENDENCY_DEVICE_GROUP_BIT";
        case static_cast<uint64_t>(2): return "VK_DEPENDENCY_VIEW_LOCAL_BIT";
        default: return "VkDependencyFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkDescriptorBindingFlagBits>(VkDescriptorBindingFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT";
        case static_cast<uint64_t>(2): return "VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT";
        case static_cast<uint64_t>(4): return "VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT";
        case static_cast<uint64_t>(8): return "VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_DESCRIPTOR_BINDING_FLAG_BITS_MAX_ENUM";
        default: return "VkDescriptorBindingFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkDescriptorPoolCreateFlagBits>(VkDescriptorPoolCreateFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_DESCRIPTOR_POOL_CREATE_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(2): return "VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT";
        case static_cast<uint64_t>(4): return "VK_DESCRIPTOR_POOL_CREATE_HOST_ONLY_BIT_VALVE";
        default: return "VkDescriptorPoolCreateFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkDescriptorSetLayoutCreateFlagBits>(VkDescriptorSetLayoutCreateFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(2147483647): return "VK_DESCRIPTOR_SET_LAYOUT_CREATE_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(2): return "VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT";
        case static_cast<uint64_t>(1): return "VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR";
        case static_cast<uint64_t>(4): return "VK_DESCRIPTOR_SET_LAYOUT_CREATE_HOST_ONLY_POOL_BIT_VALVE";
        default: return "VkDescriptorSetLayoutCreateFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkDescriptorType>(VkDescriptorType type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_DESCRIPTOR_TYPE_SAMPLER";
        case static_cast<uint64_t>(1): return "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER";
        case static_cast<uint64_t>(2): return "VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE";
        case static_cast<uint64_t>(3): return "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE";
        case static_cast<uint64_t>(4): return "VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER";
        case static_cast<uint64_t>(5): return "VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER";
        case static_cast<uint64_t>(6): return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER";
        case static_cast<uint64_t>(7): return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER";
        case static_cast<uint64_t>(8): return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC";
        case static_cast<uint64_t>(9): return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC";
        case static_cast<uint64_t>(10): return "VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT";
        case static_cast<uint64_t>(2147483647): return "VK_DESCRIPTOR_TYPE_MAX_ENUM";
        case static_cast<uint64_t>(1000138000): return "VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK";
        case static_cast<uint64_t>(1000165000): return "VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV";
        case static_cast<uint64_t>(1000351000): return "VK_DESCRIPTOR_TYPE_MUTABLE_VALVE";
        case static_cast<uint64_t>(1000150000): return "VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR";
        default: return "VkDescriptorType_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkDescriptorUpdateTemplateType>(VkDescriptorUpdateTemplateType type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET";
        case static_cast<uint64_t>(2147483647): return "VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_MAX_ENUM";
        case static_cast<uint64_t>(1): return "VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_PUSH_DESCRIPTORS_KHR";
        default: return "VkDescriptorUpdateTemplateType_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkDeviceDiagnosticsConfigFlagBitsNV>(VkDeviceDiagnosticsConfigFlagBitsNV type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_DEBUG_INFO_BIT_NV";
        case static_cast<uint64_t>(2): return "VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_RESOURCE_TRACKING_BIT_NV";
        case static_cast<uint64_t>(4): return "VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_AUTOMATIC_CHECKPOINTS_BIT_NV";
        case static_cast<uint64_t>(8): return "VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_ERROR_REPORTING_BIT_NV";
        case static_cast<uint64_t>(2147483647): return "VK_DEVICE_DIAGNOSTICS_CONFIG_FLAG_BITS_NV_MAX_ENUM";
        default: return "VkDeviceDiagnosticsConfigFlagBitsNV_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkDeviceEventTypeEXT>(VkDeviceEventTypeEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_DEVICE_EVENT_TYPE_DISPLAY_HOTPLUG_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_DEVICE_EVENT_TYPE_EXT_MAX_ENUM";
        default: return "VkDeviceEventTypeEXT_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkDeviceGroupPresentModeFlagBitsKHR>(VkDeviceGroupPresentModeFlagBitsKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_DEVICE_GROUP_PRESENT_MODE_LOCAL_BIT_KHR";
        case static_cast<uint64_t>(2): return "VK_DEVICE_GROUP_PRESENT_MODE_REMOTE_BIT_KHR";
        case static_cast<uint64_t>(4): return "VK_DEVICE_GROUP_PRESENT_MODE_SUM_BIT_KHR";
        case static_cast<uint64_t>(8): return "VK_DEVICE_GROUP_PRESENT_MODE_LOCAL_MULTI_DEVICE_BIT_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_DEVICE_GROUP_PRESENT_MODE_FLAG_BITS_KHR_MAX_ENUM";
        default: return "VkDeviceGroupPresentModeFlagBitsKHR_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkDeviceMemoryReportEventTypeEXT>(VkDeviceMemoryReportEventTypeEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_DEVICE_MEMORY_REPORT_EVENT_TYPE_ALLOCATE_EXT";
        case static_cast<uint64_t>(1): return "VK_DEVICE_MEMORY_REPORT_EVENT_TYPE_FREE_EXT";
        case static_cast<uint64_t>(2): return "VK_DEVICE_MEMORY_REPORT_EVENT_TYPE_IMPORT_EXT";
        case static_cast<uint64_t>(3): return "VK_DEVICE_MEMORY_REPORT_EVENT_TYPE_UNIMPORT_EXT";
        case static_cast<uint64_t>(4): return "VK_DEVICE_MEMORY_REPORT_EVENT_TYPE_ALLOCATION_FAILED_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_DEVICE_MEMORY_REPORT_EVENT_TYPE_EXT_MAX_ENUM";
        default: return "VkDeviceMemoryReportEventTypeEXT_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkDeviceQueueCreateFlagBits>(VkDeviceQueueCreateFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(2147483647): return "VK_DEVICE_QUEUE_CREATE_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(1): return "VK_DEVICE_QUEUE_CREATE_PROTECTED_BIT";
        default: return "VkDeviceQueueCreateFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkDiscardRectangleModeEXT>(VkDiscardRectangleModeEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_DISCARD_RECTANGLE_MODE_INCLUSIVE_EXT";
        case static_cast<uint64_t>(1): return "VK_DISCARD_RECTANGLE_MODE_EXCLUSIVE_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_DISCARD_RECTANGLE_MODE_EXT_MAX_ENUM";
        default: return "VkDiscardRectangleModeEXT_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkDisplayEventTypeEXT>(VkDisplayEventTypeEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_DISPLAY_EVENT_TYPE_FIRST_PIXEL_OUT_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_DISPLAY_EVENT_TYPE_EXT_MAX_ENUM";
        default: return "VkDisplayEventTypeEXT_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkDisplayPlaneAlphaFlagBitsKHR>(VkDisplayPlaneAlphaFlagBitsKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR";
        case static_cast<uint64_t>(2): return "VK_DISPLAY_PLANE_ALPHA_GLOBAL_BIT_KHR";
        case static_cast<uint64_t>(4): return "VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_BIT_KHR";
        case static_cast<uint64_t>(8): return "VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_PREMULTIPLIED_BIT_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_DISPLAY_PLANE_ALPHA_FLAG_BITS_KHR_MAX_ENUM";
        default: return "VkDisplayPlaneAlphaFlagBitsKHR_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkDisplayPowerStateEXT>(VkDisplayPowerStateEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_DISPLAY_POWER_STATE_OFF_EXT";
        case static_cast<uint64_t>(1): return "VK_DISPLAY_POWER_STATE_SUSPEND_EXT";
        case static_cast<uint64_t>(2): return "VK_DISPLAY_POWER_STATE_ON_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_DISPLAY_POWER_STATE_EXT_MAX_ENUM";
        default: return "VkDisplayPowerStateEXT_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkDriverId>(VkDriverId type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_DRIVER_ID_AMD_PROPRIETARY";
        case static_cast<uint64_t>(2): return "VK_DRIVER_ID_AMD_OPEN_SOURCE";
        case static_cast<uint64_t>(3): return "VK_DRIVER_ID_MESA_RADV";
        case static_cast<uint64_t>(4): return "VK_DRIVER_ID_NVIDIA_PROPRIETARY";
        case static_cast<uint64_t>(5): return "VK_DRIVER_ID_INTEL_PROPRIETARY_WINDOWS";
        case static_cast<uint64_t>(6): return "VK_DRIVER_ID_INTEL_OPEN_SOURCE_MESA";
        case static_cast<uint64_t>(7): return "VK_DRIVER_ID_IMAGINATION_PROPRIETARY";
        case static_cast<uint64_t>(8): return "VK_DRIVER_ID_QUALCOMM_PROPRIETARY";
        case static_cast<uint64_t>(9): return "VK_DRIVER_ID_ARM_PROPRIETARY";
        case static_cast<uint64_t>(10): return "VK_DRIVER_ID_GOOGLE_SWIFTSHADER";
        case static_cast<uint64_t>(11): return "VK_DRIVER_ID_GGP_PROPRIETARY";
        case static_cast<uint64_t>(12): return "VK_DRIVER_ID_BROADCOM_PROPRIETARY";
        case static_cast<uint64_t>(13): return "VK_DRIVER_ID_MESA_LLVMPIPE";
        case static_cast<uint64_t>(14): return "VK_DRIVER_ID_MOLTENVK";
        case static_cast<uint64_t>(15): return "VK_DRIVER_ID_COREAVI_PROPRIETARY";
        case static_cast<uint64_t>(16): return "VK_DRIVER_ID_JUICE_PROPRIETARY";
        case static_cast<uint64_t>(17): return "VK_DRIVER_ID_VERISILICON_PROPRIETARY";
        case static_cast<uint64_t>(18): return "VK_DRIVER_ID_MESA_TURNIP";
        case static_cast<uint64_t>(19): return "VK_DRIVER_ID_MESA_V3DV";
        case static_cast<uint64_t>(20): return "VK_DRIVER_ID_MESA_PANVK";
        case static_cast<uint64_t>(21): return "VK_DRIVER_ID_SAMSUNG_PROPRIETARY";
        case static_cast<uint64_t>(22): return "VK_DRIVER_ID_MESA_VENUS";
        case static_cast<uint64_t>(23): return "VK_DRIVER_ID_MESA_DOZEN";
        case static_cast<uint64_t>(2147483647): return "VK_DRIVER_ID_MAX_ENUM";
        default: return "VkDriverId_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkDynamicState>(VkDynamicState type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_DYNAMIC_STATE_VIEWPORT";
        case static_cast<uint64_t>(1): return "VK_DYNAMIC_STATE_SCISSOR";
        case static_cast<uint64_t>(2): return "VK_DYNAMIC_STATE_LINE_WIDTH";
        case static_cast<uint64_t>(3): return "VK_DYNAMIC_STATE_DEPTH_BIAS";
        case static_cast<uint64_t>(4): return "VK_DYNAMIC_STATE_BLEND_CONSTANTS";
        case static_cast<uint64_t>(5): return "VK_DYNAMIC_STATE_DEPTH_BOUNDS";
        case static_cast<uint64_t>(6): return "VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK";
        case static_cast<uint64_t>(7): return "VK_DYNAMIC_STATE_STENCIL_WRITE_MASK";
        case static_cast<uint64_t>(8): return "VK_DYNAMIC_STATE_STENCIL_REFERENCE";
        case static_cast<uint64_t>(2147483647): return "VK_DYNAMIC_STATE_MAX_ENUM";
        case static_cast<uint64_t>(1000267000): return "VK_DYNAMIC_STATE_CULL_MODE";
        case static_cast<uint64_t>(1000267001): return "VK_DYNAMIC_STATE_FRONT_FACE";
        case static_cast<uint64_t>(1000267002): return "VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY";
        case static_cast<uint64_t>(1000267003): return "VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT";
        case static_cast<uint64_t>(1000267004): return "VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT";
        case static_cast<uint64_t>(1000267005): return "VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE";
        case static_cast<uint64_t>(1000267006): return "VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE";
        case static_cast<uint64_t>(1000267007): return "VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE";
        case static_cast<uint64_t>(1000267008): return "VK_DYNAMIC_STATE_DEPTH_COMPARE_OP";
        case static_cast<uint64_t>(1000267009): return "VK_DYNAMIC_STATE_DEPTH_BOUNDS_TEST_ENABLE";
        case static_cast<uint64_t>(1000267010): return "VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE";
        case static_cast<uint64_t>(1000267011): return "VK_DYNAMIC_STATE_STENCIL_OP";
        case static_cast<uint64_t>(1000377001): return "VK_DYNAMIC_STATE_RASTERIZER_DISCARD_ENABLE";
        case static_cast<uint64_t>(1000377002): return "VK_DYNAMIC_STATE_DEPTH_BIAS_ENABLE";
        case static_cast<uint64_t>(1000377004): return "VK_DYNAMIC_STATE_PRIMITIVE_RESTART_ENABLE";
        case static_cast<uint64_t>(1000087000): return "VK_DYNAMIC_STATE_VIEWPORT_W_SCALING_NV";
        case static_cast<uint64_t>(1000099000): return "VK_DYNAMIC_STATE_DISCARD_RECTANGLE_EXT";
        case static_cast<uint64_t>(1000143000): return "VK_DYNAMIC_STATE_SAMPLE_LOCATIONS_EXT";
        case static_cast<uint64_t>(1000164004): return "VK_DYNAMIC_STATE_VIEWPORT_SHADING_RATE_PALETTE_NV";
        case static_cast<uint64_t>(1000164006): return "VK_DYNAMIC_STATE_VIEWPORT_COARSE_SAMPLE_ORDER_NV";
        case static_cast<uint64_t>(1000205001): return "VK_DYNAMIC_STATE_EXCLUSIVE_SCISSOR_NV";
        case static_cast<uint64_t>(1000226000): return "VK_DYNAMIC_STATE_FRAGMENT_SHADING_RATE_KHR";
        case static_cast<uint64_t>(1000259000): return "VK_DYNAMIC_STATE_LINE_STIPPLE_EXT";
        case static_cast<uint64_t>(1000352000): return "VK_DYNAMIC_STATE_VERTEX_INPUT_EXT";
        case static_cast<uint64_t>(1000377000): return "VK_DYNAMIC_STATE_PATCH_CONTROL_POINTS_EXT";
        case static_cast<uint64_t>(1000377003): return "VK_DYNAMIC_STATE_LOGIC_OP_EXT";
        case static_cast<uint64_t>(1000381000): return "VK_DYNAMIC_STATE_COLOR_WRITE_ENABLE_EXT";
        case static_cast<uint64_t>(1000347000): return "VK_DYNAMIC_STATE_RAY_TRACING_PIPELINE_STACK_SIZE_KHR";
        default: return "VkDynamicState_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkEventCreateFlagBits>(VkEventCreateFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(2147483647): return "VK_EVENT_CREATE_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(1): return "VK_EVENT_CREATE_DEVICE_ONLY_BIT";
        default: return "VkEventCreateFlagBits_UNKNOWN";
      }
    }

#ifdef VK_USE_PLATFORM_METAL_EXT
    template <> constexpr const char* enumString<VkExportMetalObjectTypeFlagBitsEXT>(VkExportMetalObjectTypeFlagBitsEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_EXPORT_METAL_OBJECT_TYPE_METAL_DEVICE_BIT_EXT";
        case static_cast<uint64_t>(2): return "VK_EXPORT_METAL_OBJECT_TYPE_METAL_COMMAND_QUEUE_BIT_EXT";
        case static_cast<uint64_t>(4): return "VK_EXPORT_METAL_OBJECT_TYPE_METAL_BUFFER_BIT_EXT";
        case static_cast<uint64_t>(8): return "VK_EXPORT_METAL_OBJECT_TYPE_METAL_TEXTURE_BIT_EXT";
        case static_cast<uint64_t>(16): return "VK_EXPORT_METAL_OBJECT_TYPE_METAL_IOSURFACE_BIT_EXT";
        case static_cast<uint64_t>(32): return "VK_EXPORT_METAL_OBJECT_TYPE_METAL_SHARED_EVENT_BIT_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_EXPORT_METAL_OBJECT_TYPE_FLAG_BITS_EXT_MAX_ENUM";
        default: return "VkExportMetalObjectTypeFlagBitsEXT_UNKNOWN";
      }
    }
#endif

    template <> constexpr const char* enumString<VkExternalFenceFeatureFlagBits>(VkExternalFenceFeatureFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_EXTERNAL_FENCE_FEATURE_EXPORTABLE_BIT";
        case static_cast<uint64_t>(2): return "VK_EXTERNAL_FENCE_FEATURE_IMPORTABLE_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_EXTERNAL_FENCE_FEATURE_FLAG_BITS_MAX_ENUM";
        default: return "VkExternalFenceFeatureFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkExternalFenceHandleTypeFlagBits>(VkExternalFenceHandleTypeFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_FD_BIT";
        case static_cast<uint64_t>(2): return "VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_BIT";
        case static_cast<uint64_t>(4): return "VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT";
        case static_cast<uint64_t>(8): return "VK_EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_EXTERNAL_FENCE_HANDLE_TYPE_FLAG_BITS_MAX_ENUM";
        default: return "VkExternalFenceHandleTypeFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkExternalMemoryFeatureFlagBits>(VkExternalMemoryFeatureFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT";
        case static_cast<uint64_t>(2): return "VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT";
        case static_cast<uint64_t>(4): return "VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_EXTERNAL_MEMORY_FEATURE_FLAG_BITS_MAX_ENUM";
        default: return "VkExternalMemoryFeatureFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkExternalMemoryFeatureFlagBitsNV>(VkExternalMemoryFeatureFlagBitsNV type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT_NV";
        case static_cast<uint64_t>(2): return "VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT_NV";
        case static_cast<uint64_t>(4): return "VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT_NV";
        case static_cast<uint64_t>(2147483647): return "VK_EXTERNAL_MEMORY_FEATURE_FLAG_BITS_NV_MAX_ENUM";
        default: return "VkExternalMemoryFeatureFlagBitsNV_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkExternalMemoryHandleTypeFlagBits>(VkExternalMemoryHandleTypeFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT";
        case static_cast<uint64_t>(2): return "VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT";
        case static_cast<uint64_t>(4): return "VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT";
        case static_cast<uint64_t>(8): return "VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT";
        case static_cast<uint64_t>(16): return "VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_KMT_BIT";
        case static_cast<uint64_t>(32): return "VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_HEAP_BIT";
        case static_cast<uint64_t>(64): return "VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_EXTERNAL_MEMORY_HANDLE_TYPE_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(512): return "VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT";
        case static_cast<uint64_t>(1024): return "VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID";
        case static_cast<uint64_t>(128): return "VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT";
        case static_cast<uint64_t>(256): return "VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_MAPPED_FOREIGN_MEMORY_BIT_EXT";
        case static_cast<uint64_t>(2048): return "VK_EXTERNAL_MEMORY_HANDLE_TYPE_ZIRCON_VMO_BIT_FUCHSIA";
        case static_cast<uint64_t>(4096): return "VK_EXTERNAL_MEMORY_HANDLE_TYPE_RDMA_ADDRESS_BIT_NV";
        default: return "VkExternalMemoryHandleTypeFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkExternalMemoryHandleTypeFlagBitsNV>(VkExternalMemoryHandleTypeFlagBitsNV type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_NV";
        case static_cast<uint64_t>(2): return "VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_NV";
        case static_cast<uint64_t>(4): return "VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_IMAGE_BIT_NV";
        case static_cast<uint64_t>(8): return "VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_IMAGE_KMT_BIT_NV";
        case static_cast<uint64_t>(2147483647): return "VK_EXTERNAL_MEMORY_HANDLE_TYPE_FLAG_BITS_NV_MAX_ENUM";
        default: return "VkExternalMemoryHandleTypeFlagBitsNV_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkExternalSemaphoreFeatureFlagBits>(VkExternalSemaphoreFeatureFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_EXTERNAL_SEMAPHORE_FEATURE_EXPORTABLE_BIT";
        case static_cast<uint64_t>(2): return "VK_EXTERNAL_SEMAPHORE_FEATURE_IMPORTABLE_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_EXTERNAL_SEMAPHORE_FEATURE_FLAG_BITS_MAX_ENUM";
        default: return "VkExternalSemaphoreFeatureFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkExternalSemaphoreHandleTypeFlagBits>(VkExternalSemaphoreHandleTypeFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT";
        case static_cast<uint64_t>(2): return "VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT";
        case static_cast<uint64_t>(4): return "VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT";
        case static_cast<uint64_t>(8): return "VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_D3D12_FENCE_BIT";
        case static_cast<uint64_t>(16): return "VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(128): return "VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_ZIRCON_EVENT_BIT_FUCHSIA";
        default: return "VkExternalSemaphoreHandleTypeFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkFenceCreateFlagBits>(VkFenceCreateFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_FENCE_CREATE_SIGNALED_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_FENCE_CREATE_FLAG_BITS_MAX_ENUM";
        default: return "VkFenceCreateFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkFenceImportFlagBits>(VkFenceImportFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_FENCE_IMPORT_TEMPORARY_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_FENCE_IMPORT_FLAG_BITS_MAX_ENUM";
        default: return "VkFenceImportFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkFilter>(VkFilter type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_FILTER_NEAREST";
        case static_cast<uint64_t>(1): return "VK_FILTER_LINEAR";
        case static_cast<uint64_t>(2147483647): return "VK_FILTER_MAX_ENUM";
        case static_cast<uint64_t>(1000015000): return "VK_FILTER_CUBIC_EXT";
        default: return "VkFilter_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkFormat>(VkFormat type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_FORMAT_UNDEFINED";
        case static_cast<uint64_t>(1): return "VK_FORMAT_R4G4_UNORM_PACK8";
        case static_cast<uint64_t>(2): return "VK_FORMAT_R4G4B4A4_UNORM_PACK16";
        case static_cast<uint64_t>(3): return "VK_FORMAT_B4G4R4A4_UNORM_PACK16";
        case static_cast<uint64_t>(4): return "VK_FORMAT_R5G6B5_UNORM_PACK16";
        case static_cast<uint64_t>(5): return "VK_FORMAT_B5G6R5_UNORM_PACK16";
        case static_cast<uint64_t>(6): return "VK_FORMAT_R5G5B5A1_UNORM_PACK16";
        case static_cast<uint64_t>(7): return "VK_FORMAT_B5G5R5A1_UNORM_PACK16";
        case static_cast<uint64_t>(8): return "VK_FORMAT_A1R5G5B5_UNORM_PACK16";
        case static_cast<uint64_t>(9): return "VK_FORMAT_R8_UNORM";
        case static_cast<uint64_t>(10): return "VK_FORMAT_R8_SNORM";
        case static_cast<uint64_t>(11): return "VK_FORMAT_R8_USCALED";
        case static_cast<uint64_t>(12): return "VK_FORMAT_R8_SSCALED";
        case static_cast<uint64_t>(13): return "VK_FORMAT_R8_UINT";
        case static_cast<uint64_t>(14): return "VK_FORMAT_R8_SINT";
        case static_cast<uint64_t>(15): return "VK_FORMAT_R8_SRGB";
        case static_cast<uint64_t>(16): return "VK_FORMAT_R8G8_UNORM";
        case static_cast<uint64_t>(17): return "VK_FORMAT_R8G8_SNORM";
        case static_cast<uint64_t>(18): return "VK_FORMAT_R8G8_USCALED";
        case static_cast<uint64_t>(19): return "VK_FORMAT_R8G8_SSCALED";
        case static_cast<uint64_t>(20): return "VK_FORMAT_R8G8_UINT";
        case static_cast<uint64_t>(21): return "VK_FORMAT_R8G8_SINT";
        case static_cast<uint64_t>(22): return "VK_FORMAT_R8G8_SRGB";
        case static_cast<uint64_t>(23): return "VK_FORMAT_R8G8B8_UNORM";
        case static_cast<uint64_t>(24): return "VK_FORMAT_R8G8B8_SNORM";
        case static_cast<uint64_t>(25): return "VK_FORMAT_R8G8B8_USCALED";
        case static_cast<uint64_t>(26): return "VK_FORMAT_R8G8B8_SSCALED";
        case static_cast<uint64_t>(27): return "VK_FORMAT_R8G8B8_UINT";
        case static_cast<uint64_t>(28): return "VK_FORMAT_R8G8B8_SINT";
        case static_cast<uint64_t>(29): return "VK_FORMAT_R8G8B8_SRGB";
        case static_cast<uint64_t>(30): return "VK_FORMAT_B8G8R8_UNORM";
        case static_cast<uint64_t>(31): return "VK_FORMAT_B8G8R8_SNORM";
        case static_cast<uint64_t>(32): return "VK_FORMAT_B8G8R8_USCALED";
        case static_cast<uint64_t>(33): return "VK_FORMAT_B8G8R8_SSCALED";
        case static_cast<uint64_t>(34): return "VK_FORMAT_B8G8R8_UINT";
        case static_cast<uint64_t>(35): return "VK_FORMAT_B8G8R8_SINT";
        case static_cast<uint64_t>(36): return "VK_FORMAT_B8G8R8_SRGB";
        case static_cast<uint64_t>(37): return "VK_FORMAT_R8G8B8A8_UNORM";
        case static_cast<uint64_t>(38): return "VK_FORMAT_R8G8B8A8_SNORM";
        case static_cast<uint64_t>(39): return "VK_FORMAT_R8G8B8A8_USCALED";
        case static_cast<uint64_t>(40): return "VK_FORMAT_R8G8B8A8_SSCALED";
        case static_cast<uint64_t>(41): return "VK_FORMAT_R8G8B8A8_UINT";
        case static_cast<uint64_t>(42): return "VK_FORMAT_R8G8B8A8_SINT";
        case static_cast<uint64_t>(43): return "VK_FORMAT_R8G8B8A8_SRGB";
        case static_cast<uint64_t>(44): return "VK_FORMAT_B8G8R8A8_UNORM";
        case static_cast<uint64_t>(45): return "VK_FORMAT_B8G8R8A8_SNORM";
        case static_cast<uint64_t>(46): return "VK_FORMAT_B8G8R8A8_USCALED";
        case static_cast<uint64_t>(47): return "VK_FORMAT_B8G8R8A8_SSCALED";
        case static_cast<uint64_t>(48): return "VK_FORMAT_B8G8R8A8_UINT";
        case static_cast<uint64_t>(49): return "VK_FORMAT_B8G8R8A8_SINT";
        case static_cast<uint64_t>(50): return "VK_FORMAT_B8G8R8A8_SRGB";
        case static_cast<uint64_t>(51): return "VK_FORMAT_A8B8G8R8_UNORM_PACK32";
        case static_cast<uint64_t>(52): return "VK_FORMAT_A8B8G8R8_SNORM_PACK32";
        case static_cast<uint64_t>(53): return "VK_FORMAT_A8B8G8R8_USCALED_PACK32";
        case static_cast<uint64_t>(54): return "VK_FORMAT_A8B8G8R8_SSCALED_PACK32";
        case static_cast<uint64_t>(55): return "VK_FORMAT_A8B8G8R8_UINT_PACK32";
        case static_cast<uint64_t>(56): return "VK_FORMAT_A8B8G8R8_SINT_PACK32";
        case static_cast<uint64_t>(57): return "VK_FORMAT_A8B8G8R8_SRGB_PACK32";
        case static_cast<uint64_t>(58): return "VK_FORMAT_A2R10G10B10_UNORM_PACK32";
        case static_cast<uint64_t>(59): return "VK_FORMAT_A2R10G10B10_SNORM_PACK32";
        case static_cast<uint64_t>(60): return "VK_FORMAT_A2R10G10B10_USCALED_PACK32";
        case static_cast<uint64_t>(61): return "VK_FORMAT_A2R10G10B10_SSCALED_PACK32";
        case static_cast<uint64_t>(62): return "VK_FORMAT_A2R10G10B10_UINT_PACK32";
        case static_cast<uint64_t>(63): return "VK_FORMAT_A2R10G10B10_SINT_PACK32";
        case static_cast<uint64_t>(64): return "VK_FORMAT_A2B10G10R10_UNORM_PACK32";
        case static_cast<uint64_t>(65): return "VK_FORMAT_A2B10G10R10_SNORM_PACK32";
        case static_cast<uint64_t>(66): return "VK_FORMAT_A2B10G10R10_USCALED_PACK32";
        case static_cast<uint64_t>(67): return "VK_FORMAT_A2B10G10R10_SSCALED_PACK32";
        case static_cast<uint64_t>(68): return "VK_FORMAT_A2B10G10R10_UINT_PACK32";
        case static_cast<uint64_t>(69): return "VK_FORMAT_A2B10G10R10_SINT_PACK32";
        case static_cast<uint64_t>(70): return "VK_FORMAT_R16_UNORM";
        case static_cast<uint64_t>(71): return "VK_FORMAT_R16_SNORM";
        case static_cast<uint64_t>(72): return "VK_FORMAT_R16_USCALED";
        case static_cast<uint64_t>(73): return "VK_FORMAT_R16_SSCALED";
        case static_cast<uint64_t>(74): return "VK_FORMAT_R16_UINT";
        case static_cast<uint64_t>(75): return "VK_FORMAT_R16_SINT";
        case static_cast<uint64_t>(76): return "VK_FORMAT_R16_SFLOAT";
        case static_cast<uint64_t>(77): return "VK_FORMAT_R16G16_UNORM";
        case static_cast<uint64_t>(78): return "VK_FORMAT_R16G16_SNORM";
        case static_cast<uint64_t>(79): return "VK_FORMAT_R16G16_USCALED";
        case static_cast<uint64_t>(80): return "VK_FORMAT_R16G16_SSCALED";
        case static_cast<uint64_t>(81): return "VK_FORMAT_R16G16_UINT";
        case static_cast<uint64_t>(82): return "VK_FORMAT_R16G16_SINT";
        case static_cast<uint64_t>(83): return "VK_FORMAT_R16G16_SFLOAT";
        case static_cast<uint64_t>(84): return "VK_FORMAT_R16G16B16_UNORM";
        case static_cast<uint64_t>(85): return "VK_FORMAT_R16G16B16_SNORM";
        case static_cast<uint64_t>(86): return "VK_FORMAT_R16G16B16_USCALED";
        case static_cast<uint64_t>(87): return "VK_FORMAT_R16G16B16_SSCALED";
        case static_cast<uint64_t>(88): return "VK_FORMAT_R16G16B16_UINT";
        case static_cast<uint64_t>(89): return "VK_FORMAT_R16G16B16_SINT";
        case static_cast<uint64_t>(90): return "VK_FORMAT_R16G16B16_SFLOAT";
        case static_cast<uint64_t>(91): return "VK_FORMAT_R16G16B16A16_UNORM";
        case static_cast<uint64_t>(92): return "VK_FORMAT_R16G16B16A16_SNORM";
        case static_cast<uint64_t>(93): return "VK_FORMAT_R16G16B16A16_USCALED";
        case static_cast<uint64_t>(94): return "VK_FORMAT_R16G16B16A16_SSCALED";
        case static_cast<uint64_t>(95): return "VK_FORMAT_R16G16B16A16_UINT";
        case static_cast<uint64_t>(96): return "VK_FORMAT_R16G16B16A16_SINT";
        case static_cast<uint64_t>(97): return "VK_FORMAT_R16G16B16A16_SFLOAT";
        case static_cast<uint64_t>(98): return "VK_FORMAT_R32_UINT";
        case static_cast<uint64_t>(99): return "VK_FORMAT_R32_SINT";
        case static_cast<uint64_t>(100): return "VK_FORMAT_R32_SFLOAT";
        case static_cast<uint64_t>(101): return "VK_FORMAT_R32G32_UINT";
        case static_cast<uint64_t>(102): return "VK_FORMAT_R32G32_SINT";
        case static_cast<uint64_t>(103): return "VK_FORMAT_R32G32_SFLOAT";
        case static_cast<uint64_t>(104): return "VK_FORMAT_R32G32B32_UINT";
        case static_cast<uint64_t>(105): return "VK_FORMAT_R32G32B32_SINT";
        case static_cast<uint64_t>(106): return "VK_FORMAT_R32G32B32_SFLOAT";
        case static_cast<uint64_t>(107): return "VK_FORMAT_R32G32B32A32_UINT";
        case static_cast<uint64_t>(108): return "VK_FORMAT_R32G32B32A32_SINT";
        case static_cast<uint64_t>(109): return "VK_FORMAT_R32G32B32A32_SFLOAT";
        case static_cast<uint64_t>(110): return "VK_FORMAT_R64_UINT";
        case static_cast<uint64_t>(111): return "VK_FORMAT_R64_SINT";
        case static_cast<uint64_t>(112): return "VK_FORMAT_R64_SFLOAT";
        case static_cast<uint64_t>(113): return "VK_FORMAT_R64G64_UINT";
        case static_cast<uint64_t>(114): return "VK_FORMAT_R64G64_SINT";
        case static_cast<uint64_t>(115): return "VK_FORMAT_R64G64_SFLOAT";
        case static_cast<uint64_t>(116): return "VK_FORMAT_R64G64B64_UINT";
        case static_cast<uint64_t>(117): return "VK_FORMAT_R64G64B64_SINT";
        case static_cast<uint64_t>(118): return "VK_FORMAT_R64G64B64_SFLOAT";
        case static_cast<uint64_t>(119): return "VK_FORMAT_R64G64B64A64_UINT";
        case static_cast<uint64_t>(120): return "VK_FORMAT_R64G64B64A64_SINT";
        case static_cast<uint64_t>(121): return "VK_FORMAT_R64G64B64A64_SFLOAT";
        case static_cast<uint64_t>(122): return "VK_FORMAT_B10G11R11_UFLOAT_PACK32";
        case static_cast<uint64_t>(123): return "VK_FORMAT_E5B9G9R9_UFLOAT_PACK32";
        case static_cast<uint64_t>(124): return "VK_FORMAT_D16_UNORM";
        case static_cast<uint64_t>(125): return "VK_FORMAT_X8_D24_UNORM_PACK32";
        case static_cast<uint64_t>(126): return "VK_FORMAT_D32_SFLOAT";
        case static_cast<uint64_t>(127): return "VK_FORMAT_S8_UINT";
        case static_cast<uint64_t>(128): return "VK_FORMAT_D16_UNORM_S8_UINT";
        case static_cast<uint64_t>(129): return "VK_FORMAT_D24_UNORM_S8_UINT";
        case static_cast<uint64_t>(130): return "VK_FORMAT_D32_SFLOAT_S8_UINT";
        case static_cast<uint64_t>(131): return "VK_FORMAT_BC1_RGB_UNORM_BLOCK";
        case static_cast<uint64_t>(132): return "VK_FORMAT_BC1_RGB_SRGB_BLOCK";
        case static_cast<uint64_t>(133): return "VK_FORMAT_BC1_RGBA_UNORM_BLOCK";
        case static_cast<uint64_t>(134): return "VK_FORMAT_BC1_RGBA_SRGB_BLOCK";
        case static_cast<uint64_t>(135): return "VK_FORMAT_BC2_UNORM_BLOCK";
        case static_cast<uint64_t>(136): return "VK_FORMAT_BC2_SRGB_BLOCK";
        case static_cast<uint64_t>(137): return "VK_FORMAT_BC3_UNORM_BLOCK";
        case static_cast<uint64_t>(138): return "VK_FORMAT_BC3_SRGB_BLOCK";
        case static_cast<uint64_t>(139): return "VK_FORMAT_BC4_UNORM_BLOCK";
        case static_cast<uint64_t>(140): return "VK_FORMAT_BC4_SNORM_BLOCK";
        case static_cast<uint64_t>(141): return "VK_FORMAT_BC5_UNORM_BLOCK";
        case static_cast<uint64_t>(142): return "VK_FORMAT_BC5_SNORM_BLOCK";
        case static_cast<uint64_t>(143): return "VK_FORMAT_BC6H_UFLOAT_BLOCK";
        case static_cast<uint64_t>(144): return "VK_FORMAT_BC6H_SFLOAT_BLOCK";
        case static_cast<uint64_t>(145): return "VK_FORMAT_BC7_UNORM_BLOCK";
        case static_cast<uint64_t>(146): return "VK_FORMAT_BC7_SRGB_BLOCK";
        case static_cast<uint64_t>(147): return "VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK";
        case static_cast<uint64_t>(148): return "VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK";
        case static_cast<uint64_t>(149): return "VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK";
        case static_cast<uint64_t>(150): return "VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK";
        case static_cast<uint64_t>(151): return "VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK";
        case static_cast<uint64_t>(152): return "VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK";
        case static_cast<uint64_t>(153): return "VK_FORMAT_EAC_R11_UNORM_BLOCK";
        case static_cast<uint64_t>(154): return "VK_FORMAT_EAC_R11_SNORM_BLOCK";
        case static_cast<uint64_t>(155): return "VK_FORMAT_EAC_R11G11_UNORM_BLOCK";
        case static_cast<uint64_t>(156): return "VK_FORMAT_EAC_R11G11_SNORM_BLOCK";
        case static_cast<uint64_t>(157): return "VK_FORMAT_ASTC_4x4_UNORM_BLOCK";
        case static_cast<uint64_t>(158): return "VK_FORMAT_ASTC_4x4_SRGB_BLOCK";
        case static_cast<uint64_t>(159): return "VK_FORMAT_ASTC_5x4_UNORM_BLOCK";
        case static_cast<uint64_t>(160): return "VK_FORMAT_ASTC_5x4_SRGB_BLOCK";
        case static_cast<uint64_t>(161): return "VK_FORMAT_ASTC_5x5_UNORM_BLOCK";
        case static_cast<uint64_t>(162): return "VK_FORMAT_ASTC_5x5_SRGB_BLOCK";
        case static_cast<uint64_t>(163): return "VK_FORMAT_ASTC_6x5_UNORM_BLOCK";
        case static_cast<uint64_t>(164): return "VK_FORMAT_ASTC_6x5_SRGB_BLOCK";
        case static_cast<uint64_t>(165): return "VK_FORMAT_ASTC_6x6_UNORM_BLOCK";
        case static_cast<uint64_t>(166): return "VK_FORMAT_ASTC_6x6_SRGB_BLOCK";
        case static_cast<uint64_t>(167): return "VK_FORMAT_ASTC_8x5_UNORM_BLOCK";
        case static_cast<uint64_t>(168): return "VK_FORMAT_ASTC_8x5_SRGB_BLOCK";
        case static_cast<uint64_t>(169): return "VK_FORMAT_ASTC_8x6_UNORM_BLOCK";
        case static_cast<uint64_t>(170): return "VK_FORMAT_ASTC_8x6_SRGB_BLOCK";
        case static_cast<uint64_t>(171): return "VK_FORMAT_ASTC_8x8_UNORM_BLOCK";
        case static_cast<uint64_t>(172): return "VK_FORMAT_ASTC_8x8_SRGB_BLOCK";
        case static_cast<uint64_t>(173): return "VK_FORMAT_ASTC_10x5_UNORM_BLOCK";
        case static_cast<uint64_t>(174): return "VK_FORMAT_ASTC_10x5_SRGB_BLOCK";
        case static_cast<uint64_t>(175): return "VK_FORMAT_ASTC_10x6_UNORM_BLOCK";
        case static_cast<uint64_t>(176): return "VK_FORMAT_ASTC_10x6_SRGB_BLOCK";
        case static_cast<uint64_t>(177): return "VK_FORMAT_ASTC_10x8_UNORM_BLOCK";
        case static_cast<uint64_t>(178): return "VK_FORMAT_ASTC_10x8_SRGB_BLOCK";
        case static_cast<uint64_t>(179): return "VK_FORMAT_ASTC_10x10_UNORM_BLOCK";
        case static_cast<uint64_t>(180): return "VK_FORMAT_ASTC_10x10_SRGB_BLOCK";
        case static_cast<uint64_t>(181): return "VK_FORMAT_ASTC_12x10_UNORM_BLOCK";
        case static_cast<uint64_t>(182): return "VK_FORMAT_ASTC_12x10_SRGB_BLOCK";
        case static_cast<uint64_t>(183): return "VK_FORMAT_ASTC_12x12_UNORM_BLOCK";
        case static_cast<uint64_t>(184): return "VK_FORMAT_ASTC_12x12_SRGB_BLOCK";
        case static_cast<uint64_t>(2147483647): return "VK_FORMAT_MAX_ENUM";
        case static_cast<uint64_t>(1000156000): return "VK_FORMAT_G8B8G8R8_422_UNORM";
        case static_cast<uint64_t>(1000156001): return "VK_FORMAT_B8G8R8G8_422_UNORM";
        case static_cast<uint64_t>(1000156002): return "VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM";
        case static_cast<uint64_t>(1000156003): return "VK_FORMAT_G8_B8R8_2PLANE_420_UNORM";
        case static_cast<uint64_t>(1000156004): return "VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM";
        case static_cast<uint64_t>(1000156005): return "VK_FORMAT_G8_B8R8_2PLANE_422_UNORM";
        case static_cast<uint64_t>(1000156006): return "VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM";
        case static_cast<uint64_t>(1000156007): return "VK_FORMAT_R10X6_UNORM_PACK16";
        case static_cast<uint64_t>(1000156008): return "VK_FORMAT_R10X6G10X6_UNORM_2PACK16";
        case static_cast<uint64_t>(1000156009): return "VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16";
        case static_cast<uint64_t>(1000156010): return "VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16";
        case static_cast<uint64_t>(1000156011): return "VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16";
        case static_cast<uint64_t>(1000156012): return "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16";
        case static_cast<uint64_t>(1000156013): return "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16";
        case static_cast<uint64_t>(1000156014): return "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16";
        case static_cast<uint64_t>(1000156015): return "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16";
        case static_cast<uint64_t>(1000156016): return "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16";
        case static_cast<uint64_t>(1000156017): return "VK_FORMAT_R12X4_UNORM_PACK16";
        case static_cast<uint64_t>(1000156018): return "VK_FORMAT_R12X4G12X4_UNORM_2PACK16";
        case static_cast<uint64_t>(1000156019): return "VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16";
        case static_cast<uint64_t>(1000156020): return "VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16";
        case static_cast<uint64_t>(1000156021): return "VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16";
        case static_cast<uint64_t>(1000156022): return "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16";
        case static_cast<uint64_t>(1000156023): return "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16";
        case static_cast<uint64_t>(1000156024): return "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16";
        case static_cast<uint64_t>(1000156025): return "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16";
        case static_cast<uint64_t>(1000156026): return "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16";
        case static_cast<uint64_t>(1000156027): return "VK_FORMAT_G16B16G16R16_422_UNORM";
        case static_cast<uint64_t>(1000156028): return "VK_FORMAT_B16G16R16G16_422_UNORM";
        case static_cast<uint64_t>(1000156029): return "VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM";
        case static_cast<uint64_t>(1000156030): return "VK_FORMAT_G16_B16R16_2PLANE_420_UNORM";
        case static_cast<uint64_t>(1000156031): return "VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM";
        case static_cast<uint64_t>(1000156032): return "VK_FORMAT_G16_B16R16_2PLANE_422_UNORM";
        case static_cast<uint64_t>(1000156033): return "VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM";
        case static_cast<uint64_t>(1000330000): return "VK_FORMAT_G8_B8R8_2PLANE_444_UNORM";
        case static_cast<uint64_t>(1000330001): return "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16";
        case static_cast<uint64_t>(1000330002): return "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16";
        case static_cast<uint64_t>(1000330003): return "VK_FORMAT_G16_B16R16_2PLANE_444_UNORM";
        case static_cast<uint64_t>(1000340000): return "VK_FORMAT_A4R4G4B4_UNORM_PACK16";
        case static_cast<uint64_t>(1000340001): return "VK_FORMAT_A4B4G4R4_UNORM_PACK16";
        case static_cast<uint64_t>(1000066000): return "VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK";
        case static_cast<uint64_t>(1000066001): return "VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK";
        case static_cast<uint64_t>(1000066002): return "VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK";
        case static_cast<uint64_t>(1000066003): return "VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK";
        case static_cast<uint64_t>(1000066004): return "VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK";
        case static_cast<uint64_t>(1000066005): return "VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK";
        case static_cast<uint64_t>(1000066006): return "VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK";
        case static_cast<uint64_t>(1000066007): return "VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK";
        case static_cast<uint64_t>(1000066008): return "VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK";
        case static_cast<uint64_t>(1000066009): return "VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK";
        case static_cast<uint64_t>(1000066010): return "VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK";
        case static_cast<uint64_t>(1000066011): return "VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK";
        case static_cast<uint64_t>(1000066012): return "VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK";
        case static_cast<uint64_t>(1000066013): return "VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK";
        case static_cast<uint64_t>(1000054000): return "VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG";
        case static_cast<uint64_t>(1000054001): return "VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG";
        case static_cast<uint64_t>(1000054002): return "VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG";
        case static_cast<uint64_t>(1000054003): return "VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG";
        case static_cast<uint64_t>(1000054004): return "VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG";
        case static_cast<uint64_t>(1000054005): return "VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG";
        case static_cast<uint64_t>(1000054006): return "VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG";
        case static_cast<uint64_t>(1000054007): return "VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG";
        default: return "VkFormat_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkFormatFeatureFlagBits>(VkFormatFeatureFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT";
        case static_cast<uint64_t>(2): return "VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT";
        case static_cast<uint64_t>(4): return "VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT";
        case static_cast<uint64_t>(8): return "VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT";
        case static_cast<uint64_t>(16): return "VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT";
        case static_cast<uint64_t>(32): return "VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT";
        case static_cast<uint64_t>(64): return "VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT";
        case static_cast<uint64_t>(128): return "VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT";
        case static_cast<uint64_t>(256): return "VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT";
        case static_cast<uint64_t>(512): return "VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT";
        case static_cast<uint64_t>(1024): return "VK_FORMAT_FEATURE_BLIT_SRC_BIT";
        case static_cast<uint64_t>(2048): return "VK_FORMAT_FEATURE_BLIT_DST_BIT";
        case static_cast<uint64_t>(4096): return "VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_FORMAT_FEATURE_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(16384): return "VK_FORMAT_FEATURE_TRANSFER_SRC_BIT";
        case static_cast<uint64_t>(32768): return "VK_FORMAT_FEATURE_TRANSFER_DST_BIT";
        case static_cast<uint64_t>(131072): return "VK_FORMAT_FEATURE_MIDPOINT_CHROMA_SAMPLES_BIT";
        case static_cast<uint64_t>(262144): return "VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_LINEAR_FILTER_BIT";
        case static_cast<uint64_t>(524288): return "VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_SEPARATE_RECONSTRUCTION_FILTER_BIT";
        case static_cast<uint64_t>(1048576): return "VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_BIT";
        case static_cast<uint64_t>(2097152): return "VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_FORCEABLE_BIT";
        case static_cast<uint64_t>(4194304): return "VK_FORMAT_FEATURE_DISJOINT_BIT";
        case static_cast<uint64_t>(8388608): return "VK_FORMAT_FEATURE_COSITED_CHROMA_SAMPLES_BIT";
        case static_cast<uint64_t>(65536): return "VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_MINMAX_BIT";
        case static_cast<uint64_t>(33554432): return "VK_FORMAT_FEATURE_VIDEO_DECODE_OUTPUT_BIT_KHR";
        case static_cast<uint64_t>(67108864): return "VK_FORMAT_FEATURE_VIDEO_DECODE_DPB_BIT_KHR";
        case static_cast<uint64_t>(8192): return "VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_CUBIC_BIT_EXT";
        case static_cast<uint64_t>(16777216): return "VK_FORMAT_FEATURE_FRAGMENT_DENSITY_MAP_BIT_EXT";
        case static_cast<uint64_t>(1073741824): return "VK_FORMAT_FEATURE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR";
        case static_cast<uint64_t>(134217728): return "VK_FORMAT_FEATURE_VIDEO_ENCODE_INPUT_BIT_KHR";
        case static_cast<uint64_t>(268435456): return "VK_FORMAT_FEATURE_VIDEO_ENCODE_DPB_BIT_KHR";
        case static_cast<uint64_t>(536870912): return "VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR";
        default: return "VkFormatFeatureFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkFragmentShadingRateCombinerOpKHR>(VkFragmentShadingRateCombinerOpKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR";
        case static_cast<uint64_t>(1): return "VK_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_KHR";
        case static_cast<uint64_t>(2): return "VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MIN_KHR";
        case static_cast<uint64_t>(3): return "VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MAX_KHR";
        case static_cast<uint64_t>(4): return "VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MUL_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KHR_MAX_ENUM";
        default: return "VkFragmentShadingRateCombinerOpKHR_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkFragmentShadingRateNV>(VkFragmentShadingRateNV type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_FRAGMENT_SHADING_RATE_1_INVOCATION_PER_PIXEL_NV";
        case static_cast<uint64_t>(1): return "VK_FRAGMENT_SHADING_RATE_1_INVOCATION_PER_1X2_PIXELS_NV";
        case static_cast<uint64_t>(4): return "VK_FRAGMENT_SHADING_RATE_1_INVOCATION_PER_2X1_PIXELS_NV";
        case static_cast<uint64_t>(5): return "VK_FRAGMENT_SHADING_RATE_1_INVOCATION_PER_2X2_PIXELS_NV";
        case static_cast<uint64_t>(6): return "VK_FRAGMENT_SHADING_RATE_1_INVOCATION_PER_2X4_PIXELS_NV";
        case static_cast<uint64_t>(9): return "VK_FRAGMENT_SHADING_RATE_1_INVOCATION_PER_4X2_PIXELS_NV";
        case static_cast<uint64_t>(10): return "VK_FRAGMENT_SHADING_RATE_1_INVOCATION_PER_4X4_PIXELS_NV";
        case static_cast<uint64_t>(11): return "VK_FRAGMENT_SHADING_RATE_2_INVOCATIONS_PER_PIXEL_NV";
        case static_cast<uint64_t>(12): return "VK_FRAGMENT_SHADING_RATE_4_INVOCATIONS_PER_PIXEL_NV";
        case static_cast<uint64_t>(13): return "VK_FRAGMENT_SHADING_RATE_8_INVOCATIONS_PER_PIXEL_NV";
        case static_cast<uint64_t>(14): return "VK_FRAGMENT_SHADING_RATE_16_INVOCATIONS_PER_PIXEL_NV";
        case static_cast<uint64_t>(15): return "VK_FRAGMENT_SHADING_RATE_NO_INVOCATIONS_NV";
        case static_cast<uint64_t>(2147483647): return "VK_FRAGMENT_SHADING_RATE_NV_MAX_ENUM";
        default: return "VkFragmentShadingRateNV_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkFragmentShadingRateTypeNV>(VkFragmentShadingRateTypeNV type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_FRAGMENT_SHADING_RATE_TYPE_FRAGMENT_SIZE_NV";
        case static_cast<uint64_t>(1): return "VK_FRAGMENT_SHADING_RATE_TYPE_ENUMS_NV";
        case static_cast<uint64_t>(2147483647): return "VK_FRAGMENT_SHADING_RATE_TYPE_NV_MAX_ENUM";
        default: return "VkFragmentShadingRateTypeNV_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkFramebufferCreateFlagBits>(VkFramebufferCreateFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(2147483647): return "VK_FRAMEBUFFER_CREATE_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(1): return "VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT";
        default: return "VkFramebufferCreateFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkFrontFace>(VkFrontFace type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_FRONT_FACE_COUNTER_CLOCKWISE";
        case static_cast<uint64_t>(1): return "VK_FRONT_FACE_CLOCKWISE";
        case static_cast<uint64_t>(2147483647): return "VK_FRONT_FACE_MAX_ENUM";
        default: return "VkFrontFace_UNKNOWN";
      }
    }

#ifdef VK_USE_PLATFORM_WIN32_KHR
    template <> constexpr const char* enumString<VkFullScreenExclusiveEXT>(VkFullScreenExclusiveEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_FULL_SCREEN_EXCLUSIVE_DEFAULT_EXT";
        case static_cast<uint64_t>(1): return "VK_FULL_SCREEN_EXCLUSIVE_ALLOWED_EXT";
        case static_cast<uint64_t>(2): return "VK_FULL_SCREEN_EXCLUSIVE_DISALLOWED_EXT";
        case static_cast<uint64_t>(3): return "VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_FULL_SCREEN_EXCLUSIVE_EXT_MAX_ENUM";
        default: return "VkFullScreenExclusiveEXT_UNKNOWN";
      }
    }
#endif

    template <> constexpr const char* enumString<VkGeometryFlagBitsKHR>(VkGeometryFlagBitsKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_GEOMETRY_OPAQUE_BIT_KHR";
        case static_cast<uint64_t>(2): return "VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_GEOMETRY_FLAG_BITS_KHR_MAX_ENUM";
        default: return "VkGeometryFlagBitsKHR_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkGeometryInstanceFlagBitsKHR>(VkGeometryInstanceFlagBitsKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR";
        case static_cast<uint64_t>(2): return "VK_GEOMETRY_INSTANCE_TRIANGLE_FLIP_FACING_BIT_KHR";
        case static_cast<uint64_t>(4): return "VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR";
        case static_cast<uint64_t>(8): return "VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_GEOMETRY_INSTANCE_FLAG_BITS_KHR_MAX_ENUM";
        default: return "VkGeometryInstanceFlagBitsKHR_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkGeometryTypeKHR>(VkGeometryTypeKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_GEOMETRY_TYPE_TRIANGLES_KHR";
        case static_cast<uint64_t>(1): return "VK_GEOMETRY_TYPE_AABBS_KHR";
        case static_cast<uint64_t>(2): return "VK_GEOMETRY_TYPE_INSTANCES_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_GEOMETRY_TYPE_KHR_MAX_ENUM";
        default: return "VkGeometryTypeKHR_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkGraphicsPipelineLibraryFlagBitsEXT>(VkGraphicsPipelineLibraryFlagBitsEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_GRAPHICS_PIPELINE_LIBRARY_VERTEX_INPUT_INTERFACE_BIT_EXT";
        case static_cast<uint64_t>(2): return "VK_GRAPHICS_PIPELINE_LIBRARY_PRE_RASTERIZATION_SHADERS_BIT_EXT";
        case static_cast<uint64_t>(4): return "VK_GRAPHICS_PIPELINE_LIBRARY_FRAGMENT_SHADER_BIT_EXT";
        case static_cast<uint64_t>(8): return "VK_GRAPHICS_PIPELINE_LIBRARY_FRAGMENT_OUTPUT_INTERFACE_BIT_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_GRAPHICS_PIPELINE_LIBRARY_FLAG_BITS_EXT_MAX_ENUM";
        default: return "VkGraphicsPipelineLibraryFlagBitsEXT_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkImageAspectFlagBits>(VkImageAspectFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_IMAGE_ASPECT_COLOR_BIT";
        case static_cast<uint64_t>(2): return "VK_IMAGE_ASPECT_DEPTH_BIT";
        case static_cast<uint64_t>(4): return "VK_IMAGE_ASPECT_STENCIL_BIT";
        case static_cast<uint64_t>(8): return "VK_IMAGE_ASPECT_METADATA_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(16): return "VK_IMAGE_ASPECT_PLANE_0_BIT";
        case static_cast<uint64_t>(32): return "VK_IMAGE_ASPECT_PLANE_1_BIT";
        case static_cast<uint64_t>(64): return "VK_IMAGE_ASPECT_PLANE_2_BIT";
        case static_cast<uint64_t>(0): return "VK_IMAGE_ASPECT_NONE";
        case static_cast<uint64_t>(128): return "VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT";
        case static_cast<uint64_t>(256): return "VK_IMAGE_ASPECT_MEMORY_PLANE_1_BIT_EXT";
        case static_cast<uint64_t>(512): return "VK_IMAGE_ASPECT_MEMORY_PLANE_2_BIT_EXT";
        case static_cast<uint64_t>(1024): return "VK_IMAGE_ASPECT_MEMORY_PLANE_3_BIT_EXT";
        default: return "VkImageAspectFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkImageCompressionFixedRateFlagBitsEXT>(VkImageCompressionFixedRateFlagBitsEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_IMAGE_COMPRESSION_FIXED_RATE_NONE_EXT";
        case static_cast<uint64_t>(1): return "VK_IMAGE_COMPRESSION_FIXED_RATE_1BPC_BIT_EXT";
        case static_cast<uint64_t>(2): return "VK_IMAGE_COMPRESSION_FIXED_RATE_2BPC_BIT_EXT";
        case static_cast<uint64_t>(4): return "VK_IMAGE_COMPRESSION_FIXED_RATE_3BPC_BIT_EXT";
        case static_cast<uint64_t>(8): return "VK_IMAGE_COMPRESSION_FIXED_RATE_4BPC_BIT_EXT";
        case static_cast<uint64_t>(16): return "VK_IMAGE_COMPRESSION_FIXED_RATE_5BPC_BIT_EXT";
        case static_cast<uint64_t>(32): return "VK_IMAGE_COMPRESSION_FIXED_RATE_6BPC_BIT_EXT";
        case static_cast<uint64_t>(64): return "VK_IMAGE_COMPRESSION_FIXED_RATE_7BPC_BIT_EXT";
        case static_cast<uint64_t>(128): return "VK_IMAGE_COMPRESSION_FIXED_RATE_8BPC_BIT_EXT";
        case static_cast<uint64_t>(256): return "VK_IMAGE_COMPRESSION_FIXED_RATE_9BPC_BIT_EXT";
        case static_cast<uint64_t>(512): return "VK_IMAGE_COMPRESSION_FIXED_RATE_10BPC_BIT_EXT";
        case static_cast<uint64_t>(1024): return "VK_IMAGE_COMPRESSION_FIXED_RATE_11BPC_BIT_EXT";
        case static_cast<uint64_t>(2048): return "VK_IMAGE_COMPRESSION_FIXED_RATE_12BPC_BIT_EXT";
        case static_cast<uint64_t>(4096): return "VK_IMAGE_COMPRESSION_FIXED_RATE_13BPC_BIT_EXT";
        case static_cast<uint64_t>(8192): return "VK_IMAGE_COMPRESSION_FIXED_RATE_14BPC_BIT_EXT";
        case static_cast<uint64_t>(16384): return "VK_IMAGE_COMPRESSION_FIXED_RATE_15BPC_BIT_EXT";
        case static_cast<uint64_t>(32768): return "VK_IMAGE_COMPRESSION_FIXED_RATE_16BPC_BIT_EXT";
        case static_cast<uint64_t>(65536): return "VK_IMAGE_COMPRESSION_FIXED_RATE_17BPC_BIT_EXT";
        case static_cast<uint64_t>(131072): return "VK_IMAGE_COMPRESSION_FIXED_RATE_18BPC_BIT_EXT";
        case static_cast<uint64_t>(262144): return "VK_IMAGE_COMPRESSION_FIXED_RATE_19BPC_BIT_EXT";
        case static_cast<uint64_t>(524288): return "VK_IMAGE_COMPRESSION_FIXED_RATE_20BPC_BIT_EXT";
        case static_cast<uint64_t>(1048576): return "VK_IMAGE_COMPRESSION_FIXED_RATE_21BPC_BIT_EXT";
        case static_cast<uint64_t>(2097152): return "VK_IMAGE_COMPRESSION_FIXED_RATE_22BPC_BIT_EXT";
        case static_cast<uint64_t>(4194304): return "VK_IMAGE_COMPRESSION_FIXED_RATE_23BPC_BIT_EXT";
        case static_cast<uint64_t>(8388608): return "VK_IMAGE_COMPRESSION_FIXED_RATE_24BPC_BIT_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_IMAGE_COMPRESSION_FIXED_RATE_FLAG_BITS_EXT_MAX_ENUM";
        default: return "VkImageCompressionFixedRateFlagBitsEXT_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkImageCompressionFlagBitsEXT>(VkImageCompressionFlagBitsEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_IMAGE_COMPRESSION_DEFAULT_EXT";
        case static_cast<uint64_t>(1): return "VK_IMAGE_COMPRESSION_FIXED_RATE_DEFAULT_EXT";
        case static_cast<uint64_t>(2): return "VK_IMAGE_COMPRESSION_FIXED_RATE_EXPLICIT_EXT";
        case static_cast<uint64_t>(4): return "VK_IMAGE_COMPRESSION_DISABLED_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_IMAGE_COMPRESSION_FLAG_BITS_EXT_MAX_ENUM";
        default: return "VkImageCompressionFlagBitsEXT_UNKNOWN";
      }
    }

#ifdef VK_USE_PLATFORM_FUCHSIA
    template <> constexpr const char* enumString<VkImageConstraintsInfoFlagBitsFUCHSIA>(VkImageConstraintsInfoFlagBitsFUCHSIA type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_IMAGE_CONSTRAINTS_INFO_CPU_READ_RARELY_FUCHSIA";
        case static_cast<uint64_t>(2): return "VK_IMAGE_CONSTRAINTS_INFO_CPU_READ_OFTEN_FUCHSIA";
        case static_cast<uint64_t>(4): return "VK_IMAGE_CONSTRAINTS_INFO_CPU_WRITE_RARELY_FUCHSIA";
        case static_cast<uint64_t>(8): return "VK_IMAGE_CONSTRAINTS_INFO_CPU_WRITE_OFTEN_FUCHSIA";
        case static_cast<uint64_t>(16): return "VK_IMAGE_CONSTRAINTS_INFO_PROTECTED_OPTIONAL_FUCHSIA";
        case static_cast<uint64_t>(2147483647): return "VK_IMAGE_CONSTRAINTS_INFO_FLAG_BITS_FUCHSIA_MAX_ENUM";
        default: return "VkImageConstraintsInfoFlagBitsFUCHSIA_UNKNOWN";
      }
    }
#endif

    template <> constexpr const char* enumString<VkImageCreateFlagBits>(VkImageCreateFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_IMAGE_CREATE_SPARSE_BINDING_BIT";
        case static_cast<uint64_t>(2): return "VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT";
        case static_cast<uint64_t>(4): return "VK_IMAGE_CREATE_SPARSE_ALIASED_BIT";
        case static_cast<uint64_t>(8): return "VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT";
        case static_cast<uint64_t>(16): return "VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_IMAGE_CREATE_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(1024): return "VK_IMAGE_CREATE_ALIAS_BIT";
        case static_cast<uint64_t>(64): return "VK_IMAGE_CREATE_SPLIT_INSTANCE_BIND_REGIONS_BIT";
        case static_cast<uint64_t>(32): return "VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT";
        case static_cast<uint64_t>(128): return "VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT";
        case static_cast<uint64_t>(256): return "VK_IMAGE_CREATE_EXTENDED_USAGE_BIT";
        case static_cast<uint64_t>(2048): return "VK_IMAGE_CREATE_PROTECTED_BIT";
        case static_cast<uint64_t>(512): return "VK_IMAGE_CREATE_DISJOINT_BIT";
        case static_cast<uint64_t>(8192): return "VK_IMAGE_CREATE_CORNER_SAMPLED_BIT_NV";
        case static_cast<uint64_t>(4096): return "VK_IMAGE_CREATE_SAMPLE_LOCATIONS_COMPATIBLE_DEPTH_BIT_EXT";
        case static_cast<uint64_t>(16384): return "VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT";
        case static_cast<uint64_t>(262144): return "VK_IMAGE_CREATE_MULTISAMPLED_RENDER_TO_SINGLE_SAMPLED_BIT_EXT";
        case static_cast<uint64_t>(131072): return "VK_IMAGE_CREATE_2D_VIEW_COMPATIBLE_BIT_EXT";
        case static_cast<uint64_t>(32768): return "VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM";
        default: return "VkImageCreateFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkImageLayout>(VkImageLayout type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_IMAGE_LAYOUT_UNDEFINED";
        case static_cast<uint64_t>(1): return "VK_IMAGE_LAYOUT_GENERAL";
        case static_cast<uint64_t>(2): return "VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL";
        case static_cast<uint64_t>(3): return "VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL";
        case static_cast<uint64_t>(4): return "VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL";
        case static_cast<uint64_t>(5): return "VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL";
        case static_cast<uint64_t>(6): return "VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL";
        case static_cast<uint64_t>(7): return "VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL";
        case static_cast<uint64_t>(8): return "VK_IMAGE_LAYOUT_PREINITIALIZED";
        case static_cast<uint64_t>(2147483647): return "VK_IMAGE_LAYOUT_MAX_ENUM";
        case static_cast<uint64_t>(1000117000): return "VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL";
        case static_cast<uint64_t>(1000117001): return "VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL";
        case static_cast<uint64_t>(1000241000): return "VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL";
        case static_cast<uint64_t>(1000241001): return "VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL";
        case static_cast<uint64_t>(1000241002): return "VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL";
        case static_cast<uint64_t>(1000241003): return "VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL";
        case static_cast<uint64_t>(1000314000): return "VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL";
        case static_cast<uint64_t>(1000314001): return "VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL";
        case static_cast<uint64_t>(1000001002): return "VK_IMAGE_LAYOUT_PRESENT_SRC_KHR";
        case static_cast<uint64_t>(1000024000): return "VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR";
        case static_cast<uint64_t>(1000024001): return "VK_IMAGE_LAYOUT_VIDEO_DECODE_SRC_KHR";
        case static_cast<uint64_t>(1000024002): return "VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR";
        case static_cast<uint64_t>(1000111000): return "VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR";
        case static_cast<uint64_t>(1000218000): return "VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT";
        case static_cast<uint64_t>(1000164003): return "VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR";
        case static_cast<uint64_t>(1000299000): return "VK_IMAGE_LAYOUT_VIDEO_ENCODE_DST_KHR";
        case static_cast<uint64_t>(1000299001): return "VK_IMAGE_LAYOUT_VIDEO_ENCODE_SRC_KHR";
        case static_cast<uint64_t>(1000299002): return "VK_IMAGE_LAYOUT_VIDEO_ENCODE_DPB_KHR";
        default: return "VkImageLayout_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkImageTiling>(VkImageTiling type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_IMAGE_TILING_OPTIMAL";
        case static_cast<uint64_t>(1): return "VK_IMAGE_TILING_LINEAR";
        case static_cast<uint64_t>(2147483647): return "VK_IMAGE_TILING_MAX_ENUM";
        case static_cast<uint64_t>(1000158000): return "VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT";
        default: return "VkImageTiling_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkImageType>(VkImageType type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_IMAGE_TYPE_1D";
        case static_cast<uint64_t>(1): return "VK_IMAGE_TYPE_2D";
        case static_cast<uint64_t>(2): return "VK_IMAGE_TYPE_3D";
        case static_cast<uint64_t>(2147483647): return "VK_IMAGE_TYPE_MAX_ENUM";
        default: return "VkImageType_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkImageUsageFlagBits>(VkImageUsageFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_IMAGE_USAGE_TRANSFER_SRC_BIT";
        case static_cast<uint64_t>(2): return "VK_IMAGE_USAGE_TRANSFER_DST_BIT";
        case static_cast<uint64_t>(4): return "VK_IMAGE_USAGE_SAMPLED_BIT";
        case static_cast<uint64_t>(8): return "VK_IMAGE_USAGE_STORAGE_BIT";
        case static_cast<uint64_t>(16): return "VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT";
        case static_cast<uint64_t>(32): return "VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT";
        case static_cast<uint64_t>(64): return "VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT";
        case static_cast<uint64_t>(128): return "VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(1024): return "VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR";
        case static_cast<uint64_t>(2048): return "VK_IMAGE_USAGE_VIDEO_DECODE_SRC_BIT_KHR";
        case static_cast<uint64_t>(4096): return "VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR";
        case static_cast<uint64_t>(512): return "VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT";
        case static_cast<uint64_t>(256): return "VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR";
        case static_cast<uint64_t>(8192): return "VK_IMAGE_USAGE_VIDEO_ENCODE_DST_BIT_KHR";
        case static_cast<uint64_t>(16384): return "VK_IMAGE_USAGE_VIDEO_ENCODE_SRC_BIT_KHR";
        case static_cast<uint64_t>(32768): return "VK_IMAGE_USAGE_VIDEO_ENCODE_DPB_BIT_KHR";
        case static_cast<uint64_t>(262144): return "VK_IMAGE_USAGE_INVOCATION_MASK_BIT_HUAWEI";
        default: return "VkImageUsageFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkImageViewCreateFlagBits>(VkImageViewCreateFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(2147483647): return "VK_IMAGE_VIEW_CREATE_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(1): return "VK_IMAGE_VIEW_CREATE_FRAGMENT_DENSITY_MAP_DYNAMIC_BIT_EXT";
        case static_cast<uint64_t>(2): return "VK_IMAGE_VIEW_CREATE_FRAGMENT_DENSITY_MAP_DEFERRED_BIT_EXT";
        default: return "VkImageViewCreateFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkImageViewType>(VkImageViewType type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_IMAGE_VIEW_TYPE_1D";
        case static_cast<uint64_t>(1): return "VK_IMAGE_VIEW_TYPE_2D";
        case static_cast<uint64_t>(2): return "VK_IMAGE_VIEW_TYPE_3D";
        case static_cast<uint64_t>(3): return "VK_IMAGE_VIEW_TYPE_CUBE";
        case static_cast<uint64_t>(4): return "VK_IMAGE_VIEW_TYPE_1D_ARRAY";
        case static_cast<uint64_t>(5): return "VK_IMAGE_VIEW_TYPE_2D_ARRAY";
        case static_cast<uint64_t>(6): return "VK_IMAGE_VIEW_TYPE_CUBE_ARRAY";
        case static_cast<uint64_t>(2147483647): return "VK_IMAGE_VIEW_TYPE_MAX_ENUM";
        default: return "VkImageViewType_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkIndexType>(VkIndexType type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_INDEX_TYPE_UINT16";
        case static_cast<uint64_t>(1): return "VK_INDEX_TYPE_UINT32";
        case static_cast<uint64_t>(2147483647): return "VK_INDEX_TYPE_MAX_ENUM";
        case static_cast<uint64_t>(1000265000): return "VK_INDEX_TYPE_UINT8_EXT";
        case static_cast<uint64_t>(1000165000): return "VK_INDEX_TYPE_NONE_KHR";
        default: return "VkIndexType_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkIndirectCommandsLayoutUsageFlagBitsNV>(VkIndirectCommandsLayoutUsageFlagBitsNV type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_INDIRECT_COMMANDS_LAYOUT_USAGE_EXPLICIT_PREPROCESS_BIT_NV";
        case static_cast<uint64_t>(2): return "VK_INDIRECT_COMMANDS_LAYOUT_USAGE_INDEXED_SEQUENCES_BIT_NV";
        case static_cast<uint64_t>(4): return "VK_INDIRECT_COMMANDS_LAYOUT_USAGE_UNORDERED_SEQUENCES_BIT_NV";
        case static_cast<uint64_t>(2147483647): return "VK_INDIRECT_COMMANDS_LAYOUT_USAGE_FLAG_BITS_NV_MAX_ENUM";
        default: return "VkIndirectCommandsLayoutUsageFlagBitsNV_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkIndirectCommandsTokenTypeNV>(VkIndirectCommandsTokenTypeNV type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_INDIRECT_COMMANDS_TOKEN_TYPE_SHADER_GROUP_NV";
        case static_cast<uint64_t>(1): return "VK_INDIRECT_COMMANDS_TOKEN_TYPE_STATE_FLAGS_NV";
        case static_cast<uint64_t>(2): return "VK_INDIRECT_COMMANDS_TOKEN_TYPE_INDEX_BUFFER_NV";
        case static_cast<uint64_t>(3): return "VK_INDIRECT_COMMANDS_TOKEN_TYPE_VERTEX_BUFFER_NV";
        case static_cast<uint64_t>(4): return "VK_INDIRECT_COMMANDS_TOKEN_TYPE_PUSH_CONSTANT_NV";
        case static_cast<uint64_t>(5): return "VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_INDEXED_NV";
        case static_cast<uint64_t>(6): return "VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_NV";
        case static_cast<uint64_t>(7): return "VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_TASKS_NV";
        case static_cast<uint64_t>(2147483647): return "VK_INDIRECT_COMMANDS_TOKEN_TYPE_NV_MAX_ENUM";
        default: return "VkIndirectCommandsTokenTypeNV_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkIndirectStateFlagBitsNV>(VkIndirectStateFlagBitsNV type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_INDIRECT_STATE_FLAG_FRONTFACE_BIT_NV";
        case static_cast<uint64_t>(2147483647): return "VK_INDIRECT_STATE_FLAG_BITS_NV_MAX_ENUM";
        default: return "VkIndirectStateFlagBitsNV_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkInstanceCreateFlagBits>(VkInstanceCreateFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(2147483647): return "VK_INSTANCE_CREATE_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(1): return "VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR";
        default: return "VkInstanceCreateFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkInternalAllocationType>(VkInternalAllocationType type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_INTERNAL_ALLOCATION_TYPE_EXECUTABLE";
        case static_cast<uint64_t>(2147483647): return "VK_INTERNAL_ALLOCATION_TYPE_MAX_ENUM";
        default: return "VkInternalAllocationType_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkLineRasterizationModeEXT>(VkLineRasterizationModeEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_LINE_RASTERIZATION_MODE_DEFAULT_EXT";
        case static_cast<uint64_t>(1): return "VK_LINE_RASTERIZATION_MODE_RECTANGULAR_EXT";
        case static_cast<uint64_t>(2): return "VK_LINE_RASTERIZATION_MODE_BRESENHAM_EXT";
        case static_cast<uint64_t>(3): return "VK_LINE_RASTERIZATION_MODE_RECTANGULAR_SMOOTH_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_LINE_RASTERIZATION_MODE_EXT_MAX_ENUM";
        default: return "VkLineRasterizationModeEXT_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkLogicOp>(VkLogicOp type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_LOGIC_OP_CLEAR";
        case static_cast<uint64_t>(1): return "VK_LOGIC_OP_AND";
        case static_cast<uint64_t>(2): return "VK_LOGIC_OP_AND_REVERSE";
        case static_cast<uint64_t>(3): return "VK_LOGIC_OP_COPY";
        case static_cast<uint64_t>(4): return "VK_LOGIC_OP_AND_INVERTED";
        case static_cast<uint64_t>(5): return "VK_LOGIC_OP_NO_OP";
        case static_cast<uint64_t>(6): return "VK_LOGIC_OP_XOR";
        case static_cast<uint64_t>(7): return "VK_LOGIC_OP_OR";
        case static_cast<uint64_t>(8): return "VK_LOGIC_OP_NOR";
        case static_cast<uint64_t>(9): return "VK_LOGIC_OP_EQUIVALENT";
        case static_cast<uint64_t>(10): return "VK_LOGIC_OP_INVERT";
        case static_cast<uint64_t>(11): return "VK_LOGIC_OP_OR_REVERSE";
        case static_cast<uint64_t>(12): return "VK_LOGIC_OP_COPY_INVERTED";
        case static_cast<uint64_t>(13): return "VK_LOGIC_OP_OR_INVERTED";
        case static_cast<uint64_t>(14): return "VK_LOGIC_OP_NAND";
        case static_cast<uint64_t>(15): return "VK_LOGIC_OP_SET";
        case static_cast<uint64_t>(2147483647): return "VK_LOGIC_OP_MAX_ENUM";
        default: return "VkLogicOp_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkMemoryAllocateFlagBits>(VkMemoryAllocateFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_MEMORY_ALLOCATE_DEVICE_MASK_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_MEMORY_ALLOCATE_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(2): return "VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT";
        case static_cast<uint64_t>(4): return "VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_CAPTURE_REPLAY_BIT";
        default: return "VkMemoryAllocateFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkMemoryHeapFlagBits>(VkMemoryHeapFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_MEMORY_HEAP_DEVICE_LOCAL_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_MEMORY_HEAP_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(2): return "VK_MEMORY_HEAP_MULTI_INSTANCE_BIT";
        default: return "VkMemoryHeapFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkMemoryOverallocationBehaviorAMD>(VkMemoryOverallocationBehaviorAMD type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_MEMORY_OVERALLOCATION_BEHAVIOR_DEFAULT_AMD";
        case static_cast<uint64_t>(1): return "VK_MEMORY_OVERALLOCATION_BEHAVIOR_ALLOWED_AMD";
        case static_cast<uint64_t>(2): return "VK_MEMORY_OVERALLOCATION_BEHAVIOR_DISALLOWED_AMD";
        case static_cast<uint64_t>(2147483647): return "VK_MEMORY_OVERALLOCATION_BEHAVIOR_AMD_MAX_ENUM";
        default: return "VkMemoryOverallocationBehaviorAMD_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkMemoryPropertyFlagBits>(VkMemoryPropertyFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT";
        case static_cast<uint64_t>(2): return "VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT";
        case static_cast<uint64_t>(4): return "VK_MEMORY_PROPERTY_HOST_COHERENT_BIT";
        case static_cast<uint64_t>(8): return "VK_MEMORY_PROPERTY_HOST_CACHED_BIT";
        case static_cast<uint64_t>(16): return "VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_MEMORY_PROPERTY_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(32): return "VK_MEMORY_PROPERTY_PROTECTED_BIT";
        case static_cast<uint64_t>(64): return "VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD";
        case static_cast<uint64_t>(128): return "VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD";
        case static_cast<uint64_t>(256): return "VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV";
        default: return "VkMemoryPropertyFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkObjectType>(VkObjectType type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_OBJECT_TYPE_UNKNOWN";
        case static_cast<uint64_t>(1): return "VK_OBJECT_TYPE_INSTANCE";
        case static_cast<uint64_t>(2): return "VK_OBJECT_TYPE_PHYSICAL_DEVICE";
        case static_cast<uint64_t>(3): return "VK_OBJECT_TYPE_DEVICE";
        case static_cast<uint64_t>(4): return "VK_OBJECT_TYPE_QUEUE";
        case static_cast<uint64_t>(5): return "VK_OBJECT_TYPE_SEMAPHORE";
        case static_cast<uint64_t>(6): return "VK_OBJECT_TYPE_COMMAND_BUFFER";
        case static_cast<uint64_t>(7): return "VK_OBJECT_TYPE_FENCE";
        case static_cast<uint64_t>(8): return "VK_OBJECT_TYPE_DEVICE_MEMORY";
        case static_cast<uint64_t>(9): return "VK_OBJECT_TYPE_BUFFER";
        case static_cast<uint64_t>(10): return "VK_OBJECT_TYPE_IMAGE";
        case static_cast<uint64_t>(11): return "VK_OBJECT_TYPE_EVENT";
        case static_cast<uint64_t>(12): return "VK_OBJECT_TYPE_QUERY_POOL";
        case static_cast<uint64_t>(13): return "VK_OBJECT_TYPE_BUFFER_VIEW";
        case static_cast<uint64_t>(14): return "VK_OBJECT_TYPE_IMAGE_VIEW";
        case static_cast<uint64_t>(15): return "VK_OBJECT_TYPE_SHADER_MODULE";
        case static_cast<uint64_t>(16): return "VK_OBJECT_TYPE_PIPELINE_CACHE";
        case static_cast<uint64_t>(17): return "VK_OBJECT_TYPE_PIPELINE_LAYOUT";
        case static_cast<uint64_t>(18): return "VK_OBJECT_TYPE_RENDER_PASS";
        case static_cast<uint64_t>(19): return "VK_OBJECT_TYPE_PIPELINE";
        case static_cast<uint64_t>(20): return "VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT";
        case static_cast<uint64_t>(21): return "VK_OBJECT_TYPE_SAMPLER";
        case static_cast<uint64_t>(22): return "VK_OBJECT_TYPE_DESCRIPTOR_POOL";
        case static_cast<uint64_t>(23): return "VK_OBJECT_TYPE_DESCRIPTOR_SET";
        case static_cast<uint64_t>(24): return "VK_OBJECT_TYPE_FRAMEBUFFER";
        case static_cast<uint64_t>(25): return "VK_OBJECT_TYPE_COMMAND_POOL";
        case static_cast<uint64_t>(2147483647): return "VK_OBJECT_TYPE_MAX_ENUM";
        case static_cast<uint64_t>(1000156000): return "VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION";
        case static_cast<uint64_t>(1000085000): return "VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE";
        case static_cast<uint64_t>(1000295000): return "VK_OBJECT_TYPE_PRIVATE_DATA_SLOT";
        case static_cast<uint64_t>(1000000000): return "VK_OBJECT_TYPE_SURFACE_KHR";
        case static_cast<uint64_t>(1000001000): return "VK_OBJECT_TYPE_SWAPCHAIN_KHR";
        case static_cast<uint64_t>(1000002000): return "VK_OBJECT_TYPE_DISPLAY_KHR";
        case static_cast<uint64_t>(1000002001): return "VK_OBJECT_TYPE_DISPLAY_MODE_KHR";
        case static_cast<uint64_t>(1000011000): return "VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT";
        case static_cast<uint64_t>(1000023000): return "VK_OBJECT_TYPE_VIDEO_SESSION_KHR";
        case static_cast<uint64_t>(1000023001): return "VK_OBJECT_TYPE_VIDEO_SESSION_PARAMETERS_KHR";
        case static_cast<uint64_t>(1000128000): return "VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT";
        case static_cast<uint64_t>(1000160000): return "VK_OBJECT_TYPE_VALIDATION_CACHE_EXT";
        case static_cast<uint64_t>(1000165000): return "VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV";
        case static_cast<uint64_t>(1000210000): return "VK_OBJECT_TYPE_PERFORMANCE_CONFIGURATION_INTEL";
        case static_cast<uint64_t>(1000268000): return "VK_OBJECT_TYPE_DEFERRED_OPERATION_KHR";
        case static_cast<uint64_t>(1000277000): return "VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NV";
        case static_cast<uint64_t>(1000366000): return "VK_OBJECT_TYPE_BUFFER_COLLECTION_FUCHSIA";
        case static_cast<uint64_t>(1000150000): return "VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR";
        default: return "VkObjectType_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkPeerMemoryFeatureFlagBits>(VkPeerMemoryFeatureFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_PEER_MEMORY_FEATURE_COPY_SRC_BIT";
        case static_cast<uint64_t>(2): return "VK_PEER_MEMORY_FEATURE_COPY_DST_BIT";
        case static_cast<uint64_t>(4): return "VK_PEER_MEMORY_FEATURE_GENERIC_SRC_BIT";
        case static_cast<uint64_t>(8): return "VK_PEER_MEMORY_FEATURE_GENERIC_DST_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_PEER_MEMORY_FEATURE_FLAG_BITS_MAX_ENUM";
        default: return "VkPeerMemoryFeatureFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkPerformanceConfigurationTypeINTEL>(VkPerformanceConfigurationTypeINTEL type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_PERFORMANCE_CONFIGURATION_TYPE_COMMAND_QUEUE_METRICS_DISCOVERY_ACTIVATED_INTEL";
        case static_cast<uint64_t>(2147483647): return "VK_PERFORMANCE_CONFIGURATION_TYPE_INTEL_MAX_ENUM";
        default: return "VkPerformanceConfigurationTypeINTEL_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkPerformanceCounterDescriptionFlagBitsKHR>(VkPerformanceCounterDescriptionFlagBitsKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_PERFORMANCE_COUNTER_DESCRIPTION_PERFORMANCE_IMPACTING_BIT_KHR";
        case static_cast<uint64_t>(2): return "VK_PERFORMANCE_COUNTER_DESCRIPTION_CONCURRENTLY_IMPACTED_BIT_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_PERFORMANCE_COUNTER_DESCRIPTION_FLAG_BITS_KHR_MAX_ENUM";
        default: return "VkPerformanceCounterDescriptionFlagBitsKHR_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkPerformanceCounterScopeKHR>(VkPerformanceCounterScopeKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_PERFORMANCE_COUNTER_SCOPE_COMMAND_BUFFER_KHR";
        case static_cast<uint64_t>(1): return "VK_PERFORMANCE_COUNTER_SCOPE_RENDER_PASS_KHR";
        case static_cast<uint64_t>(2): return "VK_PERFORMANCE_COUNTER_SCOPE_COMMAND_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_PERFORMANCE_COUNTER_SCOPE_KHR_MAX_ENUM";
        default: return "VkPerformanceCounterScopeKHR_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkPerformanceCounterStorageKHR>(VkPerformanceCounterStorageKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_PERFORMANCE_COUNTER_STORAGE_INT32_KHR";
        case static_cast<uint64_t>(1): return "VK_PERFORMANCE_COUNTER_STORAGE_INT64_KHR";
        case static_cast<uint64_t>(2): return "VK_PERFORMANCE_COUNTER_STORAGE_UINT32_KHR";
        case static_cast<uint64_t>(3): return "VK_PERFORMANCE_COUNTER_STORAGE_UINT64_KHR";
        case static_cast<uint64_t>(4): return "VK_PERFORMANCE_COUNTER_STORAGE_FLOAT32_KHR";
        case static_cast<uint64_t>(5): return "VK_PERFORMANCE_COUNTER_STORAGE_FLOAT64_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_PERFORMANCE_COUNTER_STORAGE_KHR_MAX_ENUM";
        default: return "VkPerformanceCounterStorageKHR_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkPerformanceCounterUnitKHR>(VkPerformanceCounterUnitKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_PERFORMANCE_COUNTER_UNIT_GENERIC_KHR";
        case static_cast<uint64_t>(1): return "VK_PERFORMANCE_COUNTER_UNIT_PERCENTAGE_KHR";
        case static_cast<uint64_t>(2): return "VK_PERFORMANCE_COUNTER_UNIT_NANOSECONDS_KHR";
        case static_cast<uint64_t>(3): return "VK_PERFORMANCE_COUNTER_UNIT_BYTES_KHR";
        case static_cast<uint64_t>(4): return "VK_PERFORMANCE_COUNTER_UNIT_BYTES_PER_SECOND_KHR";
        case static_cast<uint64_t>(5): return "VK_PERFORMANCE_COUNTER_UNIT_KELVIN_KHR";
        case static_cast<uint64_t>(6): return "VK_PERFORMANCE_COUNTER_UNIT_WATTS_KHR";
        case static_cast<uint64_t>(7): return "VK_PERFORMANCE_COUNTER_UNIT_VOLTS_KHR";
        case static_cast<uint64_t>(8): return "VK_PERFORMANCE_COUNTER_UNIT_AMPS_KHR";
        case static_cast<uint64_t>(9): return "VK_PERFORMANCE_COUNTER_UNIT_HERTZ_KHR";
        case static_cast<uint64_t>(10): return "VK_PERFORMANCE_COUNTER_UNIT_CYCLES_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_PERFORMANCE_COUNTER_UNIT_KHR_MAX_ENUM";
        default: return "VkPerformanceCounterUnitKHR_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkPerformanceOverrideTypeINTEL>(VkPerformanceOverrideTypeINTEL type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_PERFORMANCE_OVERRIDE_TYPE_NULL_HARDWARE_INTEL";
        case static_cast<uint64_t>(1): return "VK_PERFORMANCE_OVERRIDE_TYPE_FLUSH_GPU_CACHES_INTEL";
        case static_cast<uint64_t>(2147483647): return "VK_PERFORMANCE_OVERRIDE_TYPE_INTEL_MAX_ENUM";
        default: return "VkPerformanceOverrideTypeINTEL_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkPerformanceParameterTypeINTEL>(VkPerformanceParameterTypeINTEL type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_PERFORMANCE_PARAMETER_TYPE_HW_COUNTERS_SUPPORTED_INTEL";
        case static_cast<uint64_t>(1): return "VK_PERFORMANCE_PARAMETER_TYPE_STREAM_MARKER_VALID_BITS_INTEL";
        case static_cast<uint64_t>(2147483647): return "VK_PERFORMANCE_PARAMETER_TYPE_INTEL_MAX_ENUM";
        default: return "VkPerformanceParameterTypeINTEL_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkPerformanceValueTypeINTEL>(VkPerformanceValueTypeINTEL type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_PERFORMANCE_VALUE_TYPE_UINT32_INTEL";
        case static_cast<uint64_t>(1): return "VK_PERFORMANCE_VALUE_TYPE_UINT64_INTEL";
        case static_cast<uint64_t>(2): return "VK_PERFORMANCE_VALUE_TYPE_FLOAT_INTEL";
        case static_cast<uint64_t>(3): return "VK_PERFORMANCE_VALUE_TYPE_BOOL_INTEL";
        case static_cast<uint64_t>(4): return "VK_PERFORMANCE_VALUE_TYPE_STRING_INTEL";
        case static_cast<uint64_t>(2147483647): return "VK_PERFORMANCE_VALUE_TYPE_INTEL_MAX_ENUM";
        default: return "VkPerformanceValueTypeINTEL_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkPhysicalDeviceType>(VkPhysicalDeviceType type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_PHYSICAL_DEVICE_TYPE_OTHER";
        case static_cast<uint64_t>(1): return "VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU";
        case static_cast<uint64_t>(2): return "VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU";
        case static_cast<uint64_t>(3): return "VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU";
        case static_cast<uint64_t>(4): return "VK_PHYSICAL_DEVICE_TYPE_CPU";
        case static_cast<uint64_t>(2147483647): return "VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM";
        default: return "VkPhysicalDeviceType_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkPipelineBindPoint>(VkPipelineBindPoint type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_PIPELINE_BIND_POINT_GRAPHICS";
        case static_cast<uint64_t>(1): return "VK_PIPELINE_BIND_POINT_COMPUTE";
        case static_cast<uint64_t>(2147483647): return "VK_PIPELINE_BIND_POINT_MAX_ENUM";
        case static_cast<uint64_t>(1000369003): return "VK_PIPELINE_BIND_POINT_SUBPASS_SHADING_HUAWEI";
        case static_cast<uint64_t>(1000165000): return "VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR";
        default: return "VkPipelineBindPoint_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkPipelineCacheCreateFlagBits>(VkPipelineCacheCreateFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(2147483647): return "VK_PIPELINE_CACHE_CREATE_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(1): return "VK_PIPELINE_CACHE_CREATE_EXTERNALLY_SYNCHRONIZED_BIT";
        default: return "VkPipelineCacheCreateFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkPipelineCacheHeaderVersion>(VkPipelineCacheHeaderVersion type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_PIPELINE_CACHE_HEADER_VERSION_ONE";
        case static_cast<uint64_t>(2147483647): return "VK_PIPELINE_CACHE_HEADER_VERSION_MAX_ENUM";
        default: return "VkPipelineCacheHeaderVersion_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkPipelineColorBlendStateCreateFlagBits>(VkPipelineColorBlendStateCreateFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(2147483647): return "VK_PIPELINE_COLOR_BLEND_STATE_CREATE_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(1): return "VK_PIPELINE_COLOR_BLEND_STATE_CREATE_RASTERIZATION_ORDER_ATTACHMENT_ACCESS_BIT_ARM";
        default: return "VkPipelineColorBlendStateCreateFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkPipelineCreateFlagBits>(VkPipelineCreateFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT";
        case static_cast<uint64_t>(2): return "VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT";
        case static_cast<uint64_t>(4): return "VK_PIPELINE_CREATE_DERIVATIVE_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_PIPELINE_CREATE_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(8): return "VK_PIPELINE_CREATE_VIEW_INDEX_FROM_DEVICE_INDEX_BIT";
        case static_cast<uint64_t>(16): return "VK_PIPELINE_CREATE_DISPATCH_BASE_BIT";
        case static_cast<uint64_t>(256): return "VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT";
        case static_cast<uint64_t>(512): return "VK_PIPELINE_CREATE_EARLY_RETURN_ON_FAILURE_BIT";
        case static_cast<uint64_t>(2097152): return "VK_PIPELINE_CREATE_RENDERING_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR";
        case static_cast<uint64_t>(4194304): return "VK_PIPELINE_CREATE_RENDERING_FRAGMENT_DENSITY_MAP_ATTACHMENT_BIT_EXT";
        case static_cast<uint64_t>(32): return "VK_PIPELINE_CREATE_DEFER_COMPILE_BIT_NV";
        case static_cast<uint64_t>(64): return "VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR";
        case static_cast<uint64_t>(128): return "VK_PIPELINE_CREATE_CAPTURE_INTERNAL_REPRESENTATIONS_BIT_KHR";
        case static_cast<uint64_t>(262144): return "VK_PIPELINE_CREATE_INDIRECT_BINDABLE_BIT_NV";
        case static_cast<uint64_t>(2048): return "VK_PIPELINE_CREATE_LIBRARY_BIT_KHR";
        case static_cast<uint64_t>(8388608): return "VK_PIPELINE_CREATE_RETAIN_LINK_TIME_OPTIMIZATION_INFO_BIT_EXT";
        case static_cast<uint64_t>(1024): return "VK_PIPELINE_CREATE_LINK_TIME_OPTIMIZATION_BIT_EXT";
        case static_cast<uint64_t>(1048576): return "VK_PIPELINE_CREATE_RAY_TRACING_ALLOW_MOTION_BIT_NV";
        case static_cast<uint64_t>(16384): return "VK_PIPELINE_CREATE_RAY_TRACING_NO_NULL_ANY_HIT_SHADERS_BIT_KHR";
        case static_cast<uint64_t>(32768): return "VK_PIPELINE_CREATE_RAY_TRACING_NO_NULL_CLOSEST_HIT_SHADERS_BIT_KHR";
        case static_cast<uint64_t>(65536): return "VK_PIPELINE_CREATE_RAY_TRACING_NO_NULL_MISS_SHADERS_BIT_KHR";
        case static_cast<uint64_t>(131072): return "VK_PIPELINE_CREATE_RAY_TRACING_NO_NULL_INTERSECTION_SHADERS_BIT_KHR";
        case static_cast<uint64_t>(4096): return "VK_PIPELINE_CREATE_RAY_TRACING_SKIP_TRIANGLES_BIT_KHR";
        case static_cast<uint64_t>(8192): return "VK_PIPELINE_CREATE_RAY_TRACING_SKIP_AABBS_BIT_KHR";
        case static_cast<uint64_t>(524288): return "VK_PIPELINE_CREATE_RAY_TRACING_SHADER_GROUP_HANDLE_CAPTURE_REPLAY_BIT_KHR";
        default: return "VkPipelineCreateFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkPipelineCreationFeedbackFlagBits>(VkPipelineCreationFeedbackFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_PIPELINE_CREATION_FEEDBACK_VALID_BIT";
        case static_cast<uint64_t>(2): return "VK_PIPELINE_CREATION_FEEDBACK_APPLICATION_PIPELINE_CACHE_HIT_BIT";
        case static_cast<uint64_t>(4): return "VK_PIPELINE_CREATION_FEEDBACK_BASE_PIPELINE_ACCELERATION_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_PIPELINE_CREATION_FEEDBACK_FLAG_BITS_MAX_ENUM";
        default: return "VkPipelineCreationFeedbackFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkPipelineDepthStencilStateCreateFlagBits>(VkPipelineDepthStencilStateCreateFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(2147483647): return "VK_PIPELINE_DEPTH_STENCIL_STATE_CREATE_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(1): return "VK_PIPELINE_DEPTH_STENCIL_STATE_CREATE_RASTERIZATION_ORDER_ATTACHMENT_DEPTH_ACCESS_BIT_ARM";
        case static_cast<uint64_t>(2): return "VK_PIPELINE_DEPTH_STENCIL_STATE_CREATE_RASTERIZATION_ORDER_ATTACHMENT_STENCIL_ACCESS_BIT_ARM";
        default: return "VkPipelineDepthStencilStateCreateFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkPipelineExecutableStatisticFormatKHR>(VkPipelineExecutableStatisticFormatKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_BOOL32_KHR";
        case static_cast<uint64_t>(1): return "VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_INT64_KHR";
        case static_cast<uint64_t>(2): return "VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR";
        case static_cast<uint64_t>(3): return "VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_FLOAT64_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_KHR_MAX_ENUM";
        default: return "VkPipelineExecutableStatisticFormatKHR_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkPipelineLayoutCreateFlagBits>(VkPipelineLayoutCreateFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(2147483647): return "VK_PIPELINE_LAYOUT_CREATE_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(2): return "VK_PIPELINE_LAYOUT_CREATE_INDEPENDENT_SETS_BIT_EXT";
        default: return "VkPipelineLayoutCreateFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkPipelineRobustnessBufferBehaviorEXT>(VkPipelineRobustnessBufferBehaviorEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DEVICE_DEFAULT_EXT";
        case static_cast<uint64_t>(1): return "VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DISABLED_EXT";
        case static_cast<uint64_t>(2): return "VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_ROBUST_BUFFER_ACCESS_EXT";
        case static_cast<uint64_t>(3): return "VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_ROBUST_BUFFER_ACCESS_2_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_EXT_MAX_ENUM";
        default: return "VkPipelineRobustnessBufferBehaviorEXT_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkPipelineRobustnessImageBehaviorEXT>(VkPipelineRobustnessImageBehaviorEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_DEVICE_DEFAULT_EXT";
        case static_cast<uint64_t>(1): return "VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_DISABLED_EXT";
        case static_cast<uint64_t>(2): return "VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_ROBUST_IMAGE_ACCESS_EXT";
        case static_cast<uint64_t>(3): return "VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_ROBUST_IMAGE_ACCESS_2_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_EXT_MAX_ENUM";
        default: return "VkPipelineRobustnessImageBehaviorEXT_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkPipelineShaderStageCreateFlagBits>(VkPipelineShaderStageCreateFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(2147483647): return "VK_PIPELINE_SHADER_STAGE_CREATE_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(1): return "VK_PIPELINE_SHADER_STAGE_CREATE_ALLOW_VARYING_SUBGROUP_SIZE_BIT";
        case static_cast<uint64_t>(2): return "VK_PIPELINE_SHADER_STAGE_CREATE_REQUIRE_FULL_SUBGROUPS_BIT";
        default: return "VkPipelineShaderStageCreateFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkPipelineStageFlagBits>(VkPipelineStageFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT";
        case static_cast<uint64_t>(2): return "VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT";
        case static_cast<uint64_t>(4): return "VK_PIPELINE_STAGE_VERTEX_INPUT_BIT";
        case static_cast<uint64_t>(8): return "VK_PIPELINE_STAGE_VERTEX_SHADER_BIT";
        case static_cast<uint64_t>(16): return "VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT";
        case static_cast<uint64_t>(32): return "VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT";
        case static_cast<uint64_t>(64): return "VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT";
        case static_cast<uint64_t>(128): return "VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT";
        case static_cast<uint64_t>(256): return "VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT";
        case static_cast<uint64_t>(512): return "VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT";
        case static_cast<uint64_t>(1024): return "VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT";
        case static_cast<uint64_t>(2048): return "VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT";
        case static_cast<uint64_t>(4096): return "VK_PIPELINE_STAGE_TRANSFER_BIT";
        case static_cast<uint64_t>(8192): return "VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT";
        case static_cast<uint64_t>(16384): return "VK_PIPELINE_STAGE_HOST_BIT";
        case static_cast<uint64_t>(32768): return "VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT";
        case static_cast<uint64_t>(65536): return "VK_PIPELINE_STAGE_ALL_COMMANDS_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(0): return "VK_PIPELINE_STAGE_NONE";
        case static_cast<uint64_t>(16777216): return "VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT";
        case static_cast<uint64_t>(262144): return "VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT";
        case static_cast<uint64_t>(524288): return "VK_PIPELINE_STAGE_TASK_SHADER_BIT_NV";
        case static_cast<uint64_t>(1048576): return "VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV";
        case static_cast<uint64_t>(8388608): return "VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT";
        case static_cast<uint64_t>(4194304): return "VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR";
        case static_cast<uint64_t>(131072): return "VK_PIPELINE_STAGE_COMMAND_PREPROCESS_BIT_NV";
        case static_cast<uint64_t>(33554432): return "VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR";
        case static_cast<uint64_t>(2097152): return "VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR";
        default: return "VkPipelineStageFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkPointClippingBehavior>(VkPointClippingBehavior type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_POINT_CLIPPING_BEHAVIOR_ALL_CLIP_PLANES";
        case static_cast<uint64_t>(1): return "VK_POINT_CLIPPING_BEHAVIOR_USER_CLIP_PLANES_ONLY";
        case static_cast<uint64_t>(2147483647): return "VK_POINT_CLIPPING_BEHAVIOR_MAX_ENUM";
        default: return "VkPointClippingBehavior_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkPolygonMode>(VkPolygonMode type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_POLYGON_MODE_FILL";
        case static_cast<uint64_t>(1): return "VK_POLYGON_MODE_LINE";
        case static_cast<uint64_t>(2): return "VK_POLYGON_MODE_POINT";
        case static_cast<uint64_t>(2147483647): return "VK_POLYGON_MODE_MAX_ENUM";
        case static_cast<uint64_t>(1000153000): return "VK_POLYGON_MODE_FILL_RECTANGLE_NV";
        default: return "VkPolygonMode_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkPresentModeKHR>(VkPresentModeKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_PRESENT_MODE_IMMEDIATE_KHR";
        case static_cast<uint64_t>(1): return "VK_PRESENT_MODE_MAILBOX_KHR";
        case static_cast<uint64_t>(2): return "VK_PRESENT_MODE_FIFO_KHR";
        case static_cast<uint64_t>(3): return "VK_PRESENT_MODE_FIFO_RELAXED_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_PRESENT_MODE_KHR_MAX_ENUM";
        case static_cast<uint64_t>(1000111000): return "VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR";
        case static_cast<uint64_t>(1000111001): return "VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR";
        default: return "VkPresentModeKHR_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkPrimitiveTopology>(VkPrimitiveTopology type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_PRIMITIVE_TOPOLOGY_POINT_LIST";
        case static_cast<uint64_t>(1): return "VK_PRIMITIVE_TOPOLOGY_LINE_LIST";
        case static_cast<uint64_t>(2): return "VK_PRIMITIVE_TOPOLOGY_LINE_STRIP";
        case static_cast<uint64_t>(3): return "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST";
        case static_cast<uint64_t>(4): return "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP";
        case static_cast<uint64_t>(5): return "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN";
        case static_cast<uint64_t>(6): return "VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY";
        case static_cast<uint64_t>(7): return "VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY";
        case static_cast<uint64_t>(8): return "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY";
        case static_cast<uint64_t>(9): return "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY";
        case static_cast<uint64_t>(10): return "VK_PRIMITIVE_TOPOLOGY_PATCH_LIST";
        case static_cast<uint64_t>(2147483647): return "VK_PRIMITIVE_TOPOLOGY_MAX_ENUM";
        default: return "VkPrimitiveTopology_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkProvokingVertexModeEXT>(VkProvokingVertexModeEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_PROVOKING_VERTEX_MODE_FIRST_VERTEX_EXT";
        case static_cast<uint64_t>(1): return "VK_PROVOKING_VERTEX_MODE_LAST_VERTEX_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_PROVOKING_VERTEX_MODE_EXT_MAX_ENUM";
        default: return "VkProvokingVertexModeEXT_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkQueryControlFlagBits>(VkQueryControlFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_QUERY_CONTROL_PRECISE_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_QUERY_CONTROL_FLAG_BITS_MAX_ENUM";
        default: return "VkQueryControlFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkQueryPipelineStatisticFlagBits>(VkQueryPipelineStatisticFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT";
        case static_cast<uint64_t>(2): return "VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT";
        case static_cast<uint64_t>(4): return "VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT";
        case static_cast<uint64_t>(8): return "VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT";
        case static_cast<uint64_t>(16): return "VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT";
        case static_cast<uint64_t>(32): return "VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT";
        case static_cast<uint64_t>(64): return "VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT";
        case static_cast<uint64_t>(128): return "VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT";
        case static_cast<uint64_t>(256): return "VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT";
        case static_cast<uint64_t>(512): return "VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT";
        case static_cast<uint64_t>(1024): return "VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_QUERY_PIPELINE_STATISTIC_FLAG_BITS_MAX_ENUM";
        default: return "VkQueryPipelineStatisticFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkQueryPoolSamplingModeINTEL>(VkQueryPoolSamplingModeINTEL type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_QUERY_POOL_SAMPLING_MODE_MANUAL_INTEL";
        case static_cast<uint64_t>(2147483647): return "VK_QUERY_POOL_SAMPLING_MODE_INTEL_MAX_ENUM";
        default: return "VkQueryPoolSamplingModeINTEL_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkQueryResultFlagBits>(VkQueryResultFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_QUERY_RESULT_64_BIT";
        case static_cast<uint64_t>(2): return "VK_QUERY_RESULT_WAIT_BIT";
        case static_cast<uint64_t>(4): return "VK_QUERY_RESULT_WITH_AVAILABILITY_BIT";
        case static_cast<uint64_t>(8): return "VK_QUERY_RESULT_PARTIAL_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_QUERY_RESULT_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(16): return "VK_QUERY_RESULT_WITH_STATUS_BIT_KHR";
        default: return "VkQueryResultFlagBits_UNKNOWN";
      }
    }

#ifdef VK_ENABLE_BETA_EXTENSIONS
    template <> constexpr const char* enumString<VkQueryResultStatusKHR>(VkQueryResultStatusKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(-1): return "VK_QUERY_RESULT_STATUS_ERROR_KHR";
        case static_cast<uint64_t>(0): return "VK_QUERY_RESULT_STATUS_NOT_READY_KHR";
        case static_cast<uint64_t>(1): return "VK_QUERY_RESULT_STATUS_COMPLETE_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_QUERY_RESULT_STATUS_KHR_MAX_ENUM";
        default: return "VkQueryResultStatusKHR_UNKNOWN";
      }
    }
#endif

    template <> constexpr const char* enumString<VkQueryType>(VkQueryType type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_QUERY_TYPE_OCCLUSION";
        case static_cast<uint64_t>(1): return "VK_QUERY_TYPE_PIPELINE_STATISTICS";
        case static_cast<uint64_t>(2): return "VK_QUERY_TYPE_TIMESTAMP";
        case static_cast<uint64_t>(2147483647): return "VK_QUERY_TYPE_MAX_ENUM";
        case static_cast<uint64_t>(1000023000): return "VK_QUERY_TYPE_RESULT_STATUS_ONLY_KHR";
        case static_cast<uint64_t>(1000028004): return "VK_QUERY_TYPE_TRANSFORM_FEEDBACK_STREAM_EXT";
        case static_cast<uint64_t>(1000116000): return "VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR";
        case static_cast<uint64_t>(1000165000): return "VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_NV";
        case static_cast<uint64_t>(1000210000): return "VK_QUERY_TYPE_PERFORMANCE_QUERY_INTEL";
        case static_cast<uint64_t>(1000299000): return "VK_QUERY_TYPE_VIDEO_ENCODE_BITSTREAM_BUFFER_RANGE_KHR";
        case static_cast<uint64_t>(1000382000): return "VK_QUERY_TYPE_PRIMITIVES_GENERATED_EXT";
        case static_cast<uint64_t>(1000386000): return "VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_BOTTOM_LEVEL_POINTERS_KHR";
        case static_cast<uint64_t>(1000386001): return "VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SIZE_KHR";
        case static_cast<uint64_t>(1000150000): return "VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR";
        case static_cast<uint64_t>(1000150001): return "VK_QUERY_TYPE_ACCELERATION_STRUCTURE_SERIALIZATION_SIZE_KHR";
        default: return "VkQueryType_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkQueueFlagBits>(VkQueueFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_QUEUE_GRAPHICS_BIT";
        case static_cast<uint64_t>(2): return "VK_QUEUE_COMPUTE_BIT";
        case static_cast<uint64_t>(4): return "VK_QUEUE_TRANSFER_BIT";
        case static_cast<uint64_t>(8): return "VK_QUEUE_SPARSE_BINDING_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_QUEUE_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(16): return "VK_QUEUE_PROTECTED_BIT";
        case static_cast<uint64_t>(32): return "VK_QUEUE_VIDEO_DECODE_BIT_KHR";
        case static_cast<uint64_t>(64): return "VK_QUEUE_VIDEO_ENCODE_BIT_KHR";
        default: return "VkQueueFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkQueueGlobalPriorityKHR>(VkQueueGlobalPriorityKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(128): return "VK_QUEUE_GLOBAL_PRIORITY_LOW_KHR";
        case static_cast<uint64_t>(256): return "VK_QUEUE_GLOBAL_PRIORITY_MEDIUM_KHR";
        case static_cast<uint64_t>(512): return "VK_QUEUE_GLOBAL_PRIORITY_HIGH_KHR";
        case static_cast<uint64_t>(1024): return "VK_QUEUE_GLOBAL_PRIORITY_REALTIME_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_QUEUE_GLOBAL_PRIORITY_KHR_MAX_ENUM";
        default: return "VkQueueGlobalPriorityKHR_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkRasterizationOrderAMD>(VkRasterizationOrderAMD type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_RASTERIZATION_ORDER_STRICT_AMD";
        case static_cast<uint64_t>(1): return "VK_RASTERIZATION_ORDER_RELAXED_AMD";
        case static_cast<uint64_t>(2147483647): return "VK_RASTERIZATION_ORDER_AMD_MAX_ENUM";
        default: return "VkRasterizationOrderAMD_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkRayTracingShaderGroupTypeKHR>(VkRayTracingShaderGroupTypeKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR";
        case static_cast<uint64_t>(1): return "VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR";
        case static_cast<uint64_t>(2): return "VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_RAY_TRACING_SHADER_GROUP_TYPE_KHR_MAX_ENUM";
        default: return "VkRayTracingShaderGroupTypeKHR_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkRenderPassCreateFlagBits>(VkRenderPassCreateFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(2147483647): return "VK_RENDER_PASS_CREATE_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(2): return "VK_RENDER_PASS_CREATE_TRANSFORM_BIT_QCOM";
        default: return "VkRenderPassCreateFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkRenderingFlagBits>(VkRenderingFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT";
        case static_cast<uint64_t>(2): return "VK_RENDERING_SUSPENDING_BIT";
        case static_cast<uint64_t>(4): return "VK_RENDERING_RESUMING_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_RENDERING_FLAG_BITS_MAX_ENUM";
        default: return "VkRenderingFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkResolveModeFlagBits>(VkResolveModeFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_RESOLVE_MODE_NONE";
        case static_cast<uint64_t>(1): return "VK_RESOLVE_MODE_SAMPLE_ZERO_BIT";
        case static_cast<uint64_t>(2): return "VK_RESOLVE_MODE_AVERAGE_BIT";
        case static_cast<uint64_t>(4): return "VK_RESOLVE_MODE_MIN_BIT";
        case static_cast<uint64_t>(8): return "VK_RESOLVE_MODE_MAX_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_RESOLVE_MODE_FLAG_BITS_MAX_ENUM";
        default: return "VkResolveModeFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkResult>(VkResult type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_SUCCESS";
        case static_cast<uint64_t>(1): return "VK_NOT_READY";
        case static_cast<uint64_t>(2): return "VK_TIMEOUT";
        case static_cast<uint64_t>(3): return "VK_EVENT_SET";
        case static_cast<uint64_t>(4): return "VK_EVENT_RESET";
        case static_cast<uint64_t>(5): return "VK_INCOMPLETE";
        case static_cast<uint64_t>(-1): return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case static_cast<uint64_t>(-2): return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case static_cast<uint64_t>(-3): return "VK_ERROR_INITIALIZATION_FAILED";
        case static_cast<uint64_t>(-4): return "VK_ERROR_DEVICE_LOST";
        case static_cast<uint64_t>(-5): return "VK_ERROR_MEMORY_MAP_FAILED";
        case static_cast<uint64_t>(-6): return "VK_ERROR_LAYER_NOT_PRESENT";
        case static_cast<uint64_t>(-7): return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case static_cast<uint64_t>(-8): return "VK_ERROR_FEATURE_NOT_PRESENT";
        case static_cast<uint64_t>(-9): return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case static_cast<uint64_t>(-10): return "VK_ERROR_TOO_MANY_OBJECTS";
        case static_cast<uint64_t>(-11): return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case static_cast<uint64_t>(-12): return "VK_ERROR_FRAGMENTED_POOL";
        case static_cast<uint64_t>(-13): return "VK_ERROR_UNKNOWN";
        case static_cast<uint64_t>(2147483647): return "VK_RESULT_MAX_ENUM";
        case static_cast<uint64_t>(-1000069000): return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case static_cast<uint64_t>(-1000072003): return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case static_cast<uint64_t>(-1000161000): return "VK_ERROR_FRAGMENTATION";
        case static_cast<uint64_t>(-1000257000): return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
        case static_cast<uint64_t>(1000297000): return "VK_PIPELINE_COMPILE_REQUIRED";
        case static_cast<uint64_t>(-1000000000): return "VK_ERROR_SURFACE_LOST_KHR";
        case static_cast<uint64_t>(-1000000001): return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case static_cast<uint64_t>(1000001003): return "VK_SUBOPTIMAL_KHR";
        case static_cast<uint64_t>(-1000001004): return "VK_ERROR_OUT_OF_DATE_KHR";
        case static_cast<uint64_t>(-1000003001): return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case static_cast<uint64_t>(-1000011001): return "VK_ERROR_VALIDATION_FAILED_EXT";
        case static_cast<uint64_t>(-1000012000): return "VK_ERROR_INVALID_SHADER_NV";
        case static_cast<uint64_t>(-1000023000): return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
        case static_cast<uint64_t>(-1000023001): return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
        case static_cast<uint64_t>(-1000023002): return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
        case static_cast<uint64_t>(-1000023003): return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
        case static_cast<uint64_t>(-1000023004): return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
        case static_cast<uint64_t>(-1000023005): return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
        case static_cast<uint64_t>(-1000158000): return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
        case static_cast<uint64_t>(-1000174001): return "VK_ERROR_NOT_PERMITTED_KHR";
        case static_cast<uint64_t>(-1000255000): return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
        case static_cast<uint64_t>(1000268000): return "VK_THREAD_IDLE_KHR";
        case static_cast<uint64_t>(1000268001): return "VK_THREAD_DONE_KHR";
        case static_cast<uint64_t>(1000268002): return "VK_OPERATION_DEFERRED_KHR";
        case static_cast<uint64_t>(1000268003): return "VK_OPERATION_NOT_DEFERRED_KHR";
        case static_cast<uint64_t>(-1000338000): return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
        default: return "VkResult_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkSampleCountFlagBits>(VkSampleCountFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_SAMPLE_COUNT_1_BIT";
        case static_cast<uint64_t>(2): return "VK_SAMPLE_COUNT_2_BIT";
        case static_cast<uint64_t>(4): return "VK_SAMPLE_COUNT_4_BIT";
        case static_cast<uint64_t>(8): return "VK_SAMPLE_COUNT_8_BIT";
        case static_cast<uint64_t>(16): return "VK_SAMPLE_COUNT_16_BIT";
        case static_cast<uint64_t>(32): return "VK_SAMPLE_COUNT_32_BIT";
        case static_cast<uint64_t>(64): return "VK_SAMPLE_COUNT_64_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM";
        default: return "VkSampleCountFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkSamplerAddressMode>(VkSamplerAddressMode type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_SAMPLER_ADDRESS_MODE_REPEAT";
        case static_cast<uint64_t>(1): return "VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT";
        case static_cast<uint64_t>(2): return "VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE";
        case static_cast<uint64_t>(3): return "VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER";
        case static_cast<uint64_t>(2147483647): return "VK_SAMPLER_ADDRESS_MODE_MAX_ENUM";
        case static_cast<uint64_t>(4): return "VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE";
        default: return "VkSamplerAddressMode_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkSamplerCreateFlagBits>(VkSamplerCreateFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(2147483647): return "VK_SAMPLER_CREATE_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(1): return "VK_SAMPLER_CREATE_SUBSAMPLED_BIT_EXT";
        case static_cast<uint64_t>(2): return "VK_SAMPLER_CREATE_SUBSAMPLED_COARSE_RECONSTRUCTION_BIT_EXT";
        case static_cast<uint64_t>(4): return "VK_SAMPLER_CREATE_NON_SEAMLESS_CUBE_MAP_BIT_EXT";
        default: return "VkSamplerCreateFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkSamplerMipmapMode>(VkSamplerMipmapMode type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_SAMPLER_MIPMAP_MODE_NEAREST";
        case static_cast<uint64_t>(1): return "VK_SAMPLER_MIPMAP_MODE_LINEAR";
        case static_cast<uint64_t>(2147483647): return "VK_SAMPLER_MIPMAP_MODE_MAX_ENUM";
        default: return "VkSamplerMipmapMode_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkSamplerReductionMode>(VkSamplerReductionMode type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE";
        case static_cast<uint64_t>(1): return "VK_SAMPLER_REDUCTION_MODE_MIN";
        case static_cast<uint64_t>(2): return "VK_SAMPLER_REDUCTION_MODE_MAX";
        case static_cast<uint64_t>(2147483647): return "VK_SAMPLER_REDUCTION_MODE_MAX_ENUM";
        default: return "VkSamplerReductionMode_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkSamplerYcbcrModelConversion>(VkSamplerYcbcrModelConversion type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_SAMPLER_YCBCR_MODEL_CONVERSION_RGB_IDENTITY";
        case static_cast<uint64_t>(1): return "VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_IDENTITY";
        case static_cast<uint64_t>(2): return "VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_709";
        case static_cast<uint64_t>(3): return "VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_601";
        case static_cast<uint64_t>(4): return "VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_2020";
        case static_cast<uint64_t>(2147483647): return "VK_SAMPLER_YCBCR_MODEL_CONVERSION_MAX_ENUM";
        default: return "VkSamplerYcbcrModelConversion_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkSamplerYcbcrRange>(VkSamplerYcbcrRange type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_SAMPLER_YCBCR_RANGE_ITU_FULL";
        case static_cast<uint64_t>(1): return "VK_SAMPLER_YCBCR_RANGE_ITU_NARROW";
        case static_cast<uint64_t>(2147483647): return "VK_SAMPLER_YCBCR_RANGE_MAX_ENUM";
        default: return "VkSamplerYcbcrRange_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkScopeNV>(VkScopeNV type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_SCOPE_DEVICE_NV";
        case static_cast<uint64_t>(2): return "VK_SCOPE_WORKGROUP_NV";
        case static_cast<uint64_t>(3): return "VK_SCOPE_SUBGROUP_NV";
        case static_cast<uint64_t>(5): return "VK_SCOPE_QUEUE_FAMILY_NV";
        case static_cast<uint64_t>(2147483647): return "VK_SCOPE_NV_MAX_ENUM";
        default: return "VkScopeNV_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkSemaphoreImportFlagBits>(VkSemaphoreImportFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_SEMAPHORE_IMPORT_TEMPORARY_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_SEMAPHORE_IMPORT_FLAG_BITS_MAX_ENUM";
        default: return "VkSemaphoreImportFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkSemaphoreType>(VkSemaphoreType type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_SEMAPHORE_TYPE_BINARY";
        case static_cast<uint64_t>(1): return "VK_SEMAPHORE_TYPE_TIMELINE";
        case static_cast<uint64_t>(2147483647): return "VK_SEMAPHORE_TYPE_MAX_ENUM";
        default: return "VkSemaphoreType_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkSemaphoreWaitFlagBits>(VkSemaphoreWaitFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_SEMAPHORE_WAIT_ANY_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_SEMAPHORE_WAIT_FLAG_BITS_MAX_ENUM";
        default: return "VkSemaphoreWaitFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkShaderFloatControlsIndependence>(VkShaderFloatControlsIndependence type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_32_BIT_ONLY";
        case static_cast<uint64_t>(1): return "VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_ALL";
        case static_cast<uint64_t>(2): return "VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_NONE";
        case static_cast<uint64_t>(2147483647): return "VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_MAX_ENUM";
        default: return "VkShaderFloatControlsIndependence_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkShaderGroupShaderKHR>(VkShaderGroupShaderKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_SHADER_GROUP_SHADER_GENERAL_KHR";
        case static_cast<uint64_t>(1): return "VK_SHADER_GROUP_SHADER_CLOSEST_HIT_KHR";
        case static_cast<uint64_t>(2): return "VK_SHADER_GROUP_SHADER_ANY_HIT_KHR";
        case static_cast<uint64_t>(3): return "VK_SHADER_GROUP_SHADER_INTERSECTION_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_SHADER_GROUP_SHADER_KHR_MAX_ENUM";
        default: return "VkShaderGroupShaderKHR_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkShaderInfoTypeAMD>(VkShaderInfoTypeAMD type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_SHADER_INFO_TYPE_STATISTICS_AMD";
        case static_cast<uint64_t>(1): return "VK_SHADER_INFO_TYPE_BINARY_AMD";
        case static_cast<uint64_t>(2): return "VK_SHADER_INFO_TYPE_DISASSEMBLY_AMD";
        case static_cast<uint64_t>(2147483647): return "VK_SHADER_INFO_TYPE_AMD_MAX_ENUM";
        default: return "VkShaderInfoTypeAMD_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkShaderStageFlagBits>(VkShaderStageFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_SHADER_STAGE_VERTEX_BIT";
        case static_cast<uint64_t>(2): return "VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT";
        case static_cast<uint64_t>(4): return "VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT";
        case static_cast<uint64_t>(8): return "VK_SHADER_STAGE_GEOMETRY_BIT";
        case static_cast<uint64_t>(16): return "VK_SHADER_STAGE_FRAGMENT_BIT";
        case static_cast<uint64_t>(32): return "VK_SHADER_STAGE_COMPUTE_BIT";
        case static_cast<uint64_t>(31): return "VK_SHADER_STAGE_ALL_GRAPHICS";
        case static_cast<uint64_t>(2147483647): return "VK_SHADER_STAGE_ALL";
        case static_cast<uint64_t>(64): return "VK_SHADER_STAGE_TASK_BIT_NV";
        case static_cast<uint64_t>(128): return "VK_SHADER_STAGE_MESH_BIT_NV";
        case static_cast<uint64_t>(16384): return "VK_SHADER_STAGE_SUBPASS_SHADING_BIT_HUAWEI";
        case static_cast<uint64_t>(256): return "VK_SHADER_STAGE_RAYGEN_BIT_KHR";
        case static_cast<uint64_t>(512): return "VK_SHADER_STAGE_ANY_HIT_BIT_KHR";
        case static_cast<uint64_t>(1024): return "VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR";
        case static_cast<uint64_t>(2048): return "VK_SHADER_STAGE_MISS_BIT_KHR";
        case static_cast<uint64_t>(4096): return "VK_SHADER_STAGE_INTERSECTION_BIT_KHR";
        case static_cast<uint64_t>(8192): return "VK_SHADER_STAGE_CALLABLE_BIT_KHR";
        default: return "VkShaderStageFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkShadingRatePaletteEntryNV>(VkShadingRatePaletteEntryNV type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_SHADING_RATE_PALETTE_ENTRY_NO_INVOCATIONS_NV";
        case static_cast<uint64_t>(1): return "VK_SHADING_RATE_PALETTE_ENTRY_16_INVOCATIONS_PER_PIXEL_NV";
        case static_cast<uint64_t>(2): return "VK_SHADING_RATE_PALETTE_ENTRY_8_INVOCATIONS_PER_PIXEL_NV";
        case static_cast<uint64_t>(3): return "VK_SHADING_RATE_PALETTE_ENTRY_4_INVOCATIONS_PER_PIXEL_NV";
        case static_cast<uint64_t>(4): return "VK_SHADING_RATE_PALETTE_ENTRY_2_INVOCATIONS_PER_PIXEL_NV";
        case static_cast<uint64_t>(5): return "VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_PIXEL_NV";
        case static_cast<uint64_t>(6): return "VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_2X1_PIXELS_NV";
        case static_cast<uint64_t>(7): return "VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_1X2_PIXELS_NV";
        case static_cast<uint64_t>(8): return "VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_2X2_PIXELS_NV";
        case static_cast<uint64_t>(9): return "VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_4X2_PIXELS_NV";
        case static_cast<uint64_t>(10): return "VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_2X4_PIXELS_NV";
        case static_cast<uint64_t>(11): return "VK_SHADING_RATE_PALETTE_ENTRY_1_INVOCATION_PER_4X4_PIXELS_NV";
        case static_cast<uint64_t>(2147483647): return "VK_SHADING_RATE_PALETTE_ENTRY_NV_MAX_ENUM";
        default: return "VkShadingRatePaletteEntryNV_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkSharingMode>(VkSharingMode type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_SHARING_MODE_EXCLUSIVE";
        case static_cast<uint64_t>(1): return "VK_SHARING_MODE_CONCURRENT";
        case static_cast<uint64_t>(2147483647): return "VK_SHARING_MODE_MAX_ENUM";
        default: return "VkSharingMode_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkSparseImageFormatFlagBits>(VkSparseImageFormatFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT";
        case static_cast<uint64_t>(2): return "VK_SPARSE_IMAGE_FORMAT_ALIGNED_MIP_SIZE_BIT";
        case static_cast<uint64_t>(4): return "VK_SPARSE_IMAGE_FORMAT_NONSTANDARD_BLOCK_SIZE_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_SPARSE_IMAGE_FORMAT_FLAG_BITS_MAX_ENUM";
        default: return "VkSparseImageFormatFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkSparseMemoryBindFlagBits>(VkSparseMemoryBindFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_SPARSE_MEMORY_BIND_METADATA_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_SPARSE_MEMORY_BIND_FLAG_BITS_MAX_ENUM";
        default: return "VkSparseMemoryBindFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkStencilFaceFlagBits>(VkStencilFaceFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_STENCIL_FACE_FRONT_BIT";
        case static_cast<uint64_t>(2): return "VK_STENCIL_FACE_BACK_BIT";
        case static_cast<uint64_t>(3): return "VK_STENCIL_FACE_FRONT_AND_BACK";
        case static_cast<uint64_t>(2147483647): return "VK_STENCIL_FACE_FLAG_BITS_MAX_ENUM";
        default: return "VkStencilFaceFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkStencilOp>(VkStencilOp type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_STENCIL_OP_KEEP";
        case static_cast<uint64_t>(1): return "VK_STENCIL_OP_ZERO";
        case static_cast<uint64_t>(2): return "VK_STENCIL_OP_REPLACE";
        case static_cast<uint64_t>(3): return "VK_STENCIL_OP_INCREMENT_AND_CLAMP";
        case static_cast<uint64_t>(4): return "VK_STENCIL_OP_DECREMENT_AND_CLAMP";
        case static_cast<uint64_t>(5): return "VK_STENCIL_OP_INVERT";
        case static_cast<uint64_t>(6): return "VK_STENCIL_OP_INCREMENT_AND_WRAP";
        case static_cast<uint64_t>(7): return "VK_STENCIL_OP_DECREMENT_AND_WRAP";
        case static_cast<uint64_t>(2147483647): return "VK_STENCIL_OP_MAX_ENUM";
        default: return "VkStencilOp_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkStructureType>(VkStructureType type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_STRUCTURE_TYPE_APPLICATION_INFO";
        case static_cast<uint64_t>(1): return "VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO";
        case static_cast<uint64_t>(2): return "VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO";
        case static_cast<uint64_t>(3): return "VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO";
        case static_cast<uint64_t>(4): return "VK_STRUCTURE_TYPE_SUBMIT_INFO";
        case static_cast<uint64_t>(5): return "VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO";
        case static_cast<uint64_t>(6): return "VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE";
        case static_cast<uint64_t>(7): return "VK_STRUCTURE_TYPE_BIND_SPARSE_INFO";
        case static_cast<uint64_t>(8): return "VK_STRUCTURE_TYPE_FENCE_CREATE_INFO";
        case static_cast<uint64_t>(9): return "VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO";
        case static_cast<uint64_t>(10): return "VK_STRUCTURE_TYPE_EVENT_CREATE_INFO";
        case static_cast<uint64_t>(11): return "VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO";
        case static_cast<uint64_t>(12): return "VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO";
        case static_cast<uint64_t>(13): return "VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO";
        case static_cast<uint64_t>(14): return "VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO";
        case static_cast<uint64_t>(15): return "VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO";
        case static_cast<uint64_t>(16): return "VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO";
        case static_cast<uint64_t>(17): return "VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO";
        case static_cast<uint64_t>(18): return "VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO";
        case static_cast<uint64_t>(19): return "VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO";
        case static_cast<uint64_t>(20): return "VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO";
        case static_cast<uint64_t>(21): return "VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO";
        case static_cast<uint64_t>(22): return "VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO";
        case static_cast<uint64_t>(23): return "VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO";
        case static_cast<uint64_t>(24): return "VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO";
        case static_cast<uint64_t>(25): return "VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO";
        case static_cast<uint64_t>(26): return "VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO";
        case static_cast<uint64_t>(27): return "VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO";
        case static_cast<uint64_t>(28): return "VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO";
        case static_cast<uint64_t>(29): return "VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO";
        case static_cast<uint64_t>(30): return "VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO";
        case static_cast<uint64_t>(31): return "VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO";
        case static_cast<uint64_t>(32): return "VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO";
        case static_cast<uint64_t>(33): return "VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO";
        case static_cast<uint64_t>(34): return "VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO";
        case static_cast<uint64_t>(35): return "VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET";
        case static_cast<uint64_t>(36): return "VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET";
        case static_cast<uint64_t>(37): return "VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO";
        case static_cast<uint64_t>(38): return "VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO";
        case static_cast<uint64_t>(39): return "VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO";
        case static_cast<uint64_t>(40): return "VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO";
        case static_cast<uint64_t>(41): return "VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO";
        case static_cast<uint64_t>(42): return "VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO";
        case static_cast<uint64_t>(43): return "VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO";
        case static_cast<uint64_t>(44): return "VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER";
        case static_cast<uint64_t>(45): return "VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER";
        case static_cast<uint64_t>(46): return "VK_STRUCTURE_TYPE_MEMORY_BARRIER";
        case static_cast<uint64_t>(47): return "VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO";
        case static_cast<uint64_t>(48): return "VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO";
        case static_cast<uint64_t>(2147483647): return "VK_STRUCTURE_TYPE_MAX_ENUM";
        case static_cast<uint64_t>(1000094000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES";
        case static_cast<uint64_t>(1000157000): return "VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO";
        case static_cast<uint64_t>(1000157001): return "VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO";
        case static_cast<uint64_t>(1000083000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES";
        case static_cast<uint64_t>(1000127000): return "VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS";
        case static_cast<uint64_t>(1000127001): return "VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO";
        case static_cast<uint64_t>(1000060000): return "VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO";
        case static_cast<uint64_t>(1000060003): return "VK_STRUCTURE_TYPE_DEVICE_GROUP_RENDER_PASS_BEGIN_INFO";
        case static_cast<uint64_t>(1000060004): return "VK_STRUCTURE_TYPE_DEVICE_GROUP_COMMAND_BUFFER_BEGIN_INFO";
        case static_cast<uint64_t>(1000060005): return "VK_STRUCTURE_TYPE_DEVICE_GROUP_SUBMIT_INFO";
        case static_cast<uint64_t>(1000060006): return "VK_STRUCTURE_TYPE_DEVICE_GROUP_BIND_SPARSE_INFO";
        case static_cast<uint64_t>(1000060013): return "VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_DEVICE_GROUP_INFO";
        case static_cast<uint64_t>(1000060014): return "VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_DEVICE_GROUP_INFO";
        case static_cast<uint64_t>(1000070000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GROUP_PROPERTIES";
        case static_cast<uint64_t>(1000070001): return "VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO";
        case static_cast<uint64_t>(1000146000): return "VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2";
        case static_cast<uint64_t>(1000146001): return "VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2";
        case static_cast<uint64_t>(1000146002): return "VK_STRUCTURE_TYPE_IMAGE_SPARSE_MEMORY_REQUIREMENTS_INFO_2";
        case static_cast<uint64_t>(1000146003): return "VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2";
        case static_cast<uint64_t>(1000146004): return "VK_STRUCTURE_TYPE_SPARSE_IMAGE_MEMORY_REQUIREMENTS_2";
        case static_cast<uint64_t>(1000059000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2";
        case static_cast<uint64_t>(1000059001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2";
        case static_cast<uint64_t>(1000059002): return "VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2";
        case static_cast<uint64_t>(1000059003): return "VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2";
        case static_cast<uint64_t>(1000059004): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2";
        case static_cast<uint64_t>(1000059005): return "VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2";
        case static_cast<uint64_t>(1000059006): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2";
        case static_cast<uint64_t>(1000059007): return "VK_STRUCTURE_TYPE_SPARSE_IMAGE_FORMAT_PROPERTIES_2";
        case static_cast<uint64_t>(1000059008): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SPARSE_IMAGE_FORMAT_INFO_2";
        case static_cast<uint64_t>(1000117000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_POINT_CLIPPING_PROPERTIES";
        case static_cast<uint64_t>(1000117001): return "VK_STRUCTURE_TYPE_RENDER_PASS_INPUT_ATTACHMENT_ASPECT_CREATE_INFO";
        case static_cast<uint64_t>(1000117002): return "VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO";
        case static_cast<uint64_t>(1000117003): return "VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_DOMAIN_ORIGIN_STATE_CREATE_INFO";
        case static_cast<uint64_t>(1000053000): return "VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO";
        case static_cast<uint64_t>(1000053001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES";
        case static_cast<uint64_t>(1000053002): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES";
        case static_cast<uint64_t>(1000120000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTERS_FEATURES";
        case static_cast<uint64_t>(1000145000): return "VK_STRUCTURE_TYPE_PROTECTED_SUBMIT_INFO";
        case static_cast<uint64_t>(1000145001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_FEATURES";
        case static_cast<uint64_t>(1000145002): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_PROPERTIES";
        case static_cast<uint64_t>(1000145003): return "VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2";
        case static_cast<uint64_t>(1000156000): return "VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO";
        case static_cast<uint64_t>(1000156001): return "VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO";
        case static_cast<uint64_t>(1000156002): return "VK_STRUCTURE_TYPE_BIND_IMAGE_PLANE_MEMORY_INFO";
        case static_cast<uint64_t>(1000156003): return "VK_STRUCTURE_TYPE_IMAGE_PLANE_MEMORY_REQUIREMENTS_INFO";
        case static_cast<uint64_t>(1000156004): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES";
        case static_cast<uint64_t>(1000156005): return "VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_IMAGE_FORMAT_PROPERTIES";
        case static_cast<uint64_t>(1000085000): return "VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO";
        case static_cast<uint64_t>(1000071000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO";
        case static_cast<uint64_t>(1000071001): return "VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES";
        case static_cast<uint64_t>(1000071002): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_BUFFER_INFO";
        case static_cast<uint64_t>(1000071003): return "VK_STRUCTURE_TYPE_EXTERNAL_BUFFER_PROPERTIES";
        case static_cast<uint64_t>(1000071004): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES";
        case static_cast<uint64_t>(1000072000): return "VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO";
        case static_cast<uint64_t>(1000072001): return "VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO";
        case static_cast<uint64_t>(1000072002): return "VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO";
        case static_cast<uint64_t>(1000112000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_FENCE_INFO";
        case static_cast<uint64_t>(1000112001): return "VK_STRUCTURE_TYPE_EXTERNAL_FENCE_PROPERTIES";
        case static_cast<uint64_t>(1000113000): return "VK_STRUCTURE_TYPE_EXPORT_FENCE_CREATE_INFO";
        case static_cast<uint64_t>(1000077000): return "VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO";
        case static_cast<uint64_t>(1000076000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_SEMAPHORE_INFO";
        case static_cast<uint64_t>(1000076001): return "VK_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_PROPERTIES";
        case static_cast<uint64_t>(1000168000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES";
        case static_cast<uint64_t>(1000168001): return "VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_SUPPORT";
        case static_cast<uint64_t>(1000063000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES";
        case static_cast<uint64_t>(49): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES";
        case static_cast<uint64_t>(50): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES";
        case static_cast<uint64_t>(51): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES";
        case static_cast<uint64_t>(52): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES";
        case static_cast<uint64_t>(1000147000): return "VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO";
        case static_cast<uint64_t>(1000109000): return "VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2";
        case static_cast<uint64_t>(1000109001): return "VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2";
        case static_cast<uint64_t>(1000109002): return "VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2";
        case static_cast<uint64_t>(1000109003): return "VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2";
        case static_cast<uint64_t>(1000109004): return "VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2";
        case static_cast<uint64_t>(1000109005): return "VK_STRUCTURE_TYPE_SUBPASS_BEGIN_INFO";
        case static_cast<uint64_t>(1000109006): return "VK_STRUCTURE_TYPE_SUBPASS_END_INFO";
        case static_cast<uint64_t>(1000177000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES";
        case static_cast<uint64_t>(1000196000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES";
        case static_cast<uint64_t>(1000180000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES";
        case static_cast<uint64_t>(1000082000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES";
        case static_cast<uint64_t>(1000197000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT_CONTROLS_PROPERTIES";
        case static_cast<uint64_t>(1000161000): return "VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO";
        case static_cast<uint64_t>(1000161001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES";
        case static_cast<uint64_t>(1000161002): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES";
        case static_cast<uint64_t>(1000161003): return "VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO";
        case static_cast<uint64_t>(1000161004): return "VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_LAYOUT_SUPPORT";
        case static_cast<uint64_t>(1000199000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_STENCIL_RESOLVE_PROPERTIES";
        case static_cast<uint64_t>(1000199001): return "VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE";
        case static_cast<uint64_t>(1000221000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES";
        case static_cast<uint64_t>(1000246000): return "VK_STRUCTURE_TYPE_IMAGE_STENCIL_USAGE_CREATE_INFO";
        case static_cast<uint64_t>(1000130000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_FILTER_MINMAX_PROPERTIES";
        case static_cast<uint64_t>(1000130001): return "VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO";
        case static_cast<uint64_t>(1000211000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_MEMORY_MODEL_FEATURES";
        case static_cast<uint64_t>(1000108000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGELESS_FRAMEBUFFER_FEATURES";
        case static_cast<uint64_t>(1000108001): return "VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO";
        case static_cast<uint64_t>(1000108002): return "VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO";
        case static_cast<uint64_t>(1000108003): return "VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO";
        case static_cast<uint64_t>(1000253000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFORM_BUFFER_STANDARD_LAYOUT_FEATURES";
        case static_cast<uint64_t>(1000175000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_EXTENDED_TYPES_FEATURES";
        case static_cast<uint64_t>(1000241000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURES";
        case static_cast<uint64_t>(1000241001): return "VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_STENCIL_LAYOUT";
        case static_cast<uint64_t>(1000241002): return "VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_STENCIL_LAYOUT";
        case static_cast<uint64_t>(1000261000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES";
        case static_cast<uint64_t>(1000207000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES";
        case static_cast<uint64_t>(1000207001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_PROPERTIES";
        case static_cast<uint64_t>(1000207002): return "VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO";
        case static_cast<uint64_t>(1000207003): return "VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO";
        case static_cast<uint64_t>(1000207004): return "VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO";
        case static_cast<uint64_t>(1000207005): return "VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO";
        case static_cast<uint64_t>(1000257000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES";
        case static_cast<uint64_t>(1000244001): return "VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO";
        case static_cast<uint64_t>(1000257002): return "VK_STRUCTURE_TYPE_BUFFER_OPAQUE_CAPTURE_ADDRESS_CREATE_INFO";
        case static_cast<uint64_t>(1000257003): return "VK_STRUCTURE_TYPE_MEMORY_OPAQUE_CAPTURE_ADDRESS_ALLOCATE_INFO";
        case static_cast<uint64_t>(1000257004): return "VK_STRUCTURE_TYPE_DEVICE_MEMORY_OPAQUE_CAPTURE_ADDRESS_INFO";
        case static_cast<uint64_t>(53): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES";
        case static_cast<uint64_t>(54): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES";
        case static_cast<uint64_t>(1000192000): return "VK_STRUCTURE_TYPE_PIPELINE_CREATION_FEEDBACK_CREATE_INFO";
        case static_cast<uint64_t>(1000215000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_TERMINATE_INVOCATION_FEATURES";
        case static_cast<uint64_t>(1000245000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TOOL_PROPERTIES";
        case static_cast<uint64_t>(1000276000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES";
        case static_cast<uint64_t>(1000295000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIVATE_DATA_FEATURES";
        case static_cast<uint64_t>(1000295001): return "VK_STRUCTURE_TYPE_DEVICE_PRIVATE_DATA_CREATE_INFO";
        case static_cast<uint64_t>(1000295002): return "VK_STRUCTURE_TYPE_PRIVATE_DATA_SLOT_CREATE_INFO";
        case static_cast<uint64_t>(1000297000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_CREATION_CACHE_CONTROL_FEATURES";
        case static_cast<uint64_t>(1000314000): return "VK_STRUCTURE_TYPE_MEMORY_BARRIER_2";
        case static_cast<uint64_t>(1000314001): return "VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2";
        case static_cast<uint64_t>(1000314002): return "VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2";
        case static_cast<uint64_t>(1000314003): return "VK_STRUCTURE_TYPE_DEPENDENCY_INFO";
        case static_cast<uint64_t>(1000314004): return "VK_STRUCTURE_TYPE_SUBMIT_INFO_2";
        case static_cast<uint64_t>(1000314005): return "VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO";
        case static_cast<uint64_t>(1000314006): return "VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO";
        case static_cast<uint64_t>(1000314007): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES";
        case static_cast<uint64_t>(1000325000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ZERO_INITIALIZE_WORKGROUP_MEMORY_FEATURES";
        case static_cast<uint64_t>(1000335000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_ROBUSTNESS_FEATURES";
        case static_cast<uint64_t>(1000337000): return "VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2";
        case static_cast<uint64_t>(1000337001): return "VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2";
        case static_cast<uint64_t>(1000337002): return "VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2";
        case static_cast<uint64_t>(1000337003): return "VK_STRUCTURE_TYPE_COPY_IMAGE_TO_BUFFER_INFO_2";
        case static_cast<uint64_t>(1000337004): return "VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2";
        case static_cast<uint64_t>(1000337005): return "VK_STRUCTURE_TYPE_RESOLVE_IMAGE_INFO_2";
        case static_cast<uint64_t>(1000337006): return "VK_STRUCTURE_TYPE_BUFFER_COPY_2";
        case static_cast<uint64_t>(1000337007): return "VK_STRUCTURE_TYPE_IMAGE_COPY_2";
        case static_cast<uint64_t>(1000337008): return "VK_STRUCTURE_TYPE_IMAGE_BLIT_2";
        case static_cast<uint64_t>(1000337009): return "VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2";
        case static_cast<uint64_t>(1000337010): return "VK_STRUCTURE_TYPE_IMAGE_RESOLVE_2";
        case static_cast<uint64_t>(1000225000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_PROPERTIES";
        case static_cast<uint64_t>(1000225001): return "VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO";
        case static_cast<uint64_t>(1000225002): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES";
        case static_cast<uint64_t>(1000138000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_FEATURES";
        case static_cast<uint64_t>(1000138001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_PROPERTIES";
        case static_cast<uint64_t>(1000138002): return "VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_INLINE_UNIFORM_BLOCK";
        case static_cast<uint64_t>(1000138003): return "VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_INLINE_UNIFORM_BLOCK_CREATE_INFO";
        case static_cast<uint64_t>(1000066000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXTURE_COMPRESSION_ASTC_HDR_FEATURES";
        case static_cast<uint64_t>(1000044000): return "VK_STRUCTURE_TYPE_RENDERING_INFO";
        case static_cast<uint64_t>(1000044001): return "VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO";
        case static_cast<uint64_t>(1000044002): return "VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO";
        case static_cast<uint64_t>(1000044003): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES";
        case static_cast<uint64_t>(1000044004): return "VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO";
        case static_cast<uint64_t>(1000280000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_DOT_PRODUCT_FEATURES";
        case static_cast<uint64_t>(1000280001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_DOT_PRODUCT_PROPERTIES";
        case static_cast<uint64_t>(1000281001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXEL_BUFFER_ALIGNMENT_PROPERTIES";
        case static_cast<uint64_t>(1000360000): return "VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_3";
        case static_cast<uint64_t>(1000413000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES";
        case static_cast<uint64_t>(1000413001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_PROPERTIES";
        case static_cast<uint64_t>(1000413002): return "VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS";
        case static_cast<uint64_t>(1000413003): return "VK_STRUCTURE_TYPE_DEVICE_IMAGE_MEMORY_REQUIREMENTS";
        case static_cast<uint64_t>(1000001000): return "VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR";
        case static_cast<uint64_t>(1000001001): return "VK_STRUCTURE_TYPE_PRESENT_INFO_KHR";
        case static_cast<uint64_t>(1000060007): return "VK_STRUCTURE_TYPE_DEVICE_GROUP_PRESENT_CAPABILITIES_KHR";
        case static_cast<uint64_t>(1000060008): return "VK_STRUCTURE_TYPE_IMAGE_SWAPCHAIN_CREATE_INFO_KHR";
        case static_cast<uint64_t>(1000060009): return "VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_SWAPCHAIN_INFO_KHR";
        case static_cast<uint64_t>(1000060010): return "VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR";
        case static_cast<uint64_t>(1000060011): return "VK_STRUCTURE_TYPE_DEVICE_GROUP_PRESENT_INFO_KHR";
        case static_cast<uint64_t>(1000060012): return "VK_STRUCTURE_TYPE_DEVICE_GROUP_SWAPCHAIN_CREATE_INFO_KHR";
        case static_cast<uint64_t>(1000002000): return "VK_STRUCTURE_TYPE_DISPLAY_MODE_CREATE_INFO_KHR";
        case static_cast<uint64_t>(1000002001): return "VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR";
        case static_cast<uint64_t>(1000003000): return "VK_STRUCTURE_TYPE_DISPLAY_PRESENT_INFO_KHR";
        case static_cast<uint64_t>(1000004000): return "VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR";
        case static_cast<uint64_t>(1000005000): return "VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR";
        case static_cast<uint64_t>(1000006000): return "VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR";
        case static_cast<uint64_t>(1000008000): return "VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR";
        case static_cast<uint64_t>(1000009000): return "VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR";
        case static_cast<uint64_t>(1000011000): return "VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000018000): return "VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_RASTERIZATION_ORDER_AMD";
        case static_cast<uint64_t>(1000022000): return "VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT";
        case static_cast<uint64_t>(1000022001): return "VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_TAG_INFO_EXT";
        case static_cast<uint64_t>(1000022002): return "VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT";
        case static_cast<uint64_t>(1000023000): return "VK_STRUCTURE_TYPE_VIDEO_PROFILE_KHR";
        case static_cast<uint64_t>(1000023001): return "VK_STRUCTURE_TYPE_VIDEO_CAPABILITIES_KHR";
        case static_cast<uint64_t>(1000023002): return "VK_STRUCTURE_TYPE_VIDEO_PICTURE_RESOURCE_KHR";
        case static_cast<uint64_t>(1000023003): return "VK_STRUCTURE_TYPE_VIDEO_GET_MEMORY_PROPERTIES_KHR";
        case static_cast<uint64_t>(1000023004): return "VK_STRUCTURE_TYPE_VIDEO_BIND_MEMORY_KHR";
        case static_cast<uint64_t>(1000023005): return "VK_STRUCTURE_TYPE_VIDEO_SESSION_CREATE_INFO_KHR";
        case static_cast<uint64_t>(1000023006): return "VK_STRUCTURE_TYPE_VIDEO_SESSION_PARAMETERS_CREATE_INFO_KHR";
        case static_cast<uint64_t>(1000023007): return "VK_STRUCTURE_TYPE_VIDEO_SESSION_PARAMETERS_UPDATE_INFO_KHR";
        case static_cast<uint64_t>(1000023008): return "VK_STRUCTURE_TYPE_VIDEO_BEGIN_CODING_INFO_KHR";
        case static_cast<uint64_t>(1000023009): return "VK_STRUCTURE_TYPE_VIDEO_END_CODING_INFO_KHR";
        case static_cast<uint64_t>(1000023010): return "VK_STRUCTURE_TYPE_VIDEO_CODING_CONTROL_INFO_KHR";
        case static_cast<uint64_t>(1000023011): return "VK_STRUCTURE_TYPE_VIDEO_REFERENCE_SLOT_KHR";
        case static_cast<uint64_t>(1000023012): return "VK_STRUCTURE_TYPE_VIDEO_QUEUE_FAMILY_PROPERTIES_2_KHR";
        case static_cast<uint64_t>(1000023013): return "VK_STRUCTURE_TYPE_VIDEO_PROFILES_KHR";
        case static_cast<uint64_t>(1000023014): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VIDEO_FORMAT_INFO_KHR";
        case static_cast<uint64_t>(1000023015): return "VK_STRUCTURE_TYPE_VIDEO_FORMAT_PROPERTIES_KHR";
        case static_cast<uint64_t>(1000023016): return "VK_STRUCTURE_TYPE_QUEUE_FAMILY_QUERY_RESULT_STATUS_PROPERTIES_2_KHR";
        case static_cast<uint64_t>(1000024000): return "VK_STRUCTURE_TYPE_VIDEO_DECODE_INFO_KHR";
        case static_cast<uint64_t>(1000024001): return "VK_STRUCTURE_TYPE_VIDEO_DECODE_CAPABILITIES_KHR";
        case static_cast<uint64_t>(1000026000): return "VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_IMAGE_CREATE_INFO_NV";
        case static_cast<uint64_t>(1000026001): return "VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_BUFFER_CREATE_INFO_NV";
        case static_cast<uint64_t>(1000026002): return "VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV";
        case static_cast<uint64_t>(1000028000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT";
        case static_cast<uint64_t>(1000028001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_PROPERTIES_EXT";
        case static_cast<uint64_t>(1000028002): return "VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_STREAM_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000038000): return "VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_CAPABILITIES_EXT";
        case static_cast<uint64_t>(1000038001): return "VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_SESSION_PARAMETERS_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000038002): return "VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_SESSION_PARAMETERS_ADD_INFO_EXT";
        case static_cast<uint64_t>(1000038003): return "VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_VCL_FRAME_INFO_EXT";
        case static_cast<uint64_t>(1000038004): return "VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_DPB_SLOT_INFO_EXT";
        case static_cast<uint64_t>(1000038005): return "VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_NALU_SLICE_EXT";
        case static_cast<uint64_t>(1000038006): return "VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_EMIT_PICTURE_PARAMETERS_EXT";
        case static_cast<uint64_t>(1000038007): return "VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_PROFILE_EXT";
        case static_cast<uint64_t>(1000038008): return "VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_RATE_CONTROL_INFO_EXT";
        case static_cast<uint64_t>(1000038009): return "VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_RATE_CONTROL_LAYER_INFO_EXT";
        case static_cast<uint64_t>(1000038010): return "VK_STRUCTURE_TYPE_VIDEO_ENCODE_H264_REFERENCE_LISTS_EXT";
        case static_cast<uint64_t>(1000039000): return "VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_CAPABILITIES_EXT";
        case static_cast<uint64_t>(1000039001): return "VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_SESSION_PARAMETERS_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000039002): return "VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_SESSION_PARAMETERS_ADD_INFO_EXT";
        case static_cast<uint64_t>(1000039003): return "VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_VCL_FRAME_INFO_EXT";
        case static_cast<uint64_t>(1000039004): return "VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_DPB_SLOT_INFO_EXT";
        case static_cast<uint64_t>(1000039005): return "VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_NALU_SLICE_SEGMENT_EXT";
        case static_cast<uint64_t>(1000039006): return "VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_EMIT_PICTURE_PARAMETERS_EXT";
        case static_cast<uint64_t>(1000039007): return "VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_PROFILE_EXT";
        case static_cast<uint64_t>(1000039008): return "VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_REFERENCE_LISTS_EXT";
        case static_cast<uint64_t>(1000039009): return "VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_RATE_CONTROL_INFO_EXT";
        case static_cast<uint64_t>(1000039010): return "VK_STRUCTURE_TYPE_VIDEO_ENCODE_H265_RATE_CONTROL_LAYER_INFO_EXT";
        case static_cast<uint64_t>(1000040000): return "VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_CAPABILITIES_EXT";
        case static_cast<uint64_t>(1000040001): return "VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PICTURE_INFO_EXT";
        case static_cast<uint64_t>(1000040002): return "VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_MVC_EXT";
        case static_cast<uint64_t>(1000040003): return "VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PROFILE_EXT";
        case static_cast<uint64_t>(1000040004): return "VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_SESSION_PARAMETERS_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000040005): return "VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_SESSION_PARAMETERS_ADD_INFO_EXT";
        case static_cast<uint64_t>(1000040006): return "VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_DPB_SLOT_INFO_EXT";
        case static_cast<uint64_t>(1000041000): return "VK_STRUCTURE_TYPE_TEXTURE_LOD_GATHER_FORMAT_PROPERTIES_AMD";
        case static_cast<uint64_t>(1000044006): return "VK_STRUCTURE_TYPE_RENDERING_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR";
        case static_cast<uint64_t>(1000044007): return "VK_STRUCTURE_TYPE_RENDERING_FRAGMENT_DENSITY_MAP_ATTACHMENT_INFO_EXT";
        case static_cast<uint64_t>(1000044008): return "VK_STRUCTURE_TYPE_ATTACHMENT_SAMPLE_COUNT_INFO_AMD";
        case static_cast<uint64_t>(1000044009): return "VK_STRUCTURE_TYPE_MULTIVIEW_PER_VIEW_ATTRIBUTES_INFO_NVX";
        case static_cast<uint64_t>(1000049000): return "VK_STRUCTURE_TYPE_STREAM_DESCRIPTOR_SURFACE_CREATE_INFO_GGP";
        case static_cast<uint64_t>(1000050000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CORNER_SAMPLED_IMAGE_FEATURES_NV";
        case static_cast<uint64_t>(1000056000): return "VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_NV";
        case static_cast<uint64_t>(1000056001): return "VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_NV";
        case static_cast<uint64_t>(1000057000): return "VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_NV";
        case static_cast<uint64_t>(1000057001): return "VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_NV";
        case static_cast<uint64_t>(1000058000): return "VK_STRUCTURE_TYPE_WIN32_KEYED_MUTEX_ACQUIRE_RELEASE_INFO_NV";
        case static_cast<uint64_t>(1000061000): return "VK_STRUCTURE_TYPE_VALIDATION_FLAGS_EXT";
        case static_cast<uint64_t>(1000062000): return "VK_STRUCTURE_TYPE_VI_SURFACE_CREATE_INFO_NN";
        case static_cast<uint64_t>(1000067000): return "VK_STRUCTURE_TYPE_IMAGE_VIEW_ASTC_DECODE_MODE_EXT";
        case static_cast<uint64_t>(1000067001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ASTC_DECODE_FEATURES_EXT";
        case static_cast<uint64_t>(1000068000): return "VK_STRUCTURE_TYPE_PIPELINE_ROBUSTNESS_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000068001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_ROBUSTNESS_FEATURES_EXT";
        case static_cast<uint64_t>(1000068002): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_ROBUSTNESS_PROPERTIES_EXT";
        case static_cast<uint64_t>(1000073000): return "VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR";
        case static_cast<uint64_t>(1000073001): return "VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_KHR";
        case static_cast<uint64_t>(1000073002): return "VK_STRUCTURE_TYPE_MEMORY_WIN32_HANDLE_PROPERTIES_KHR";
        case static_cast<uint64_t>(1000073003): return "VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR";
        case static_cast<uint64_t>(1000074000): return "VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR";
        case static_cast<uint64_t>(1000074001): return "VK_STRUCTURE_TYPE_MEMORY_FD_PROPERTIES_KHR";
        case static_cast<uint64_t>(1000074002): return "VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR";
        case static_cast<uint64_t>(1000075000): return "VK_STRUCTURE_TYPE_WIN32_KEYED_MUTEX_ACQUIRE_RELEASE_INFO_KHR";
        case static_cast<uint64_t>(1000078000): return "VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR";
        case static_cast<uint64_t>(1000078001): return "VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR";
        case static_cast<uint64_t>(1000078002): return "VK_STRUCTURE_TYPE_D3D12_FENCE_SUBMIT_INFO_KHR";
        case static_cast<uint64_t>(1000078003): return "VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR";
        case static_cast<uint64_t>(1000079000): return "VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_FD_INFO_KHR";
        case static_cast<uint64_t>(1000079001): return "VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR";
        case static_cast<uint64_t>(1000080000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR";
        case static_cast<uint64_t>(1000081000): return "VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_CONDITIONAL_RENDERING_INFO_EXT";
        case static_cast<uint64_t>(1000081001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONDITIONAL_RENDERING_FEATURES_EXT";
        case static_cast<uint64_t>(1000081002): return "VK_STRUCTURE_TYPE_CONDITIONAL_RENDERING_BEGIN_INFO_EXT";
        case static_cast<uint64_t>(1000084000): return "VK_STRUCTURE_TYPE_PRESENT_REGIONS_KHR";
        case static_cast<uint64_t>(1000087000): return "VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_W_SCALING_STATE_CREATE_INFO_NV";
        case static_cast<uint64_t>(1000090000): return "VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_EXT";
        case static_cast<uint64_t>(1000091000): return "VK_STRUCTURE_TYPE_DISPLAY_POWER_INFO_EXT";
        case static_cast<uint64_t>(1000091001): return "VK_STRUCTURE_TYPE_DEVICE_EVENT_INFO_EXT";
        case static_cast<uint64_t>(1000091002): return "VK_STRUCTURE_TYPE_DISPLAY_EVENT_INFO_EXT";
        case static_cast<uint64_t>(1000091003): return "VK_STRUCTURE_TYPE_SWAPCHAIN_COUNTER_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000092000): return "VK_STRUCTURE_TYPE_PRESENT_TIMES_INFO_GOOGLE";
        case static_cast<uint64_t>(1000098000): return "VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_SWIZZLE_STATE_CREATE_INFO_NV";
        case static_cast<uint64_t>(1000099000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DISCARD_RECTANGLE_PROPERTIES_EXT";
        case static_cast<uint64_t>(1000099001): return "VK_STRUCTURE_TYPE_PIPELINE_DISCARD_RECTANGLE_STATE_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000101000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT";
        case static_cast<uint64_t>(1000101001): return "VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000102000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT";
        case static_cast<uint64_t>(1000102001): return "VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000105000): return "VK_STRUCTURE_TYPE_HDR_METADATA_EXT";
        case static_cast<uint64_t>(1000111000): return "VK_STRUCTURE_TYPE_SHARED_PRESENT_SURFACE_CAPABILITIES_KHR";
        case static_cast<uint64_t>(1000114000): return "VK_STRUCTURE_TYPE_IMPORT_FENCE_WIN32_HANDLE_INFO_KHR";
        case static_cast<uint64_t>(1000114001): return "VK_STRUCTURE_TYPE_EXPORT_FENCE_WIN32_HANDLE_INFO_KHR";
        case static_cast<uint64_t>(1000114002): return "VK_STRUCTURE_TYPE_FENCE_GET_WIN32_HANDLE_INFO_KHR";
        case static_cast<uint64_t>(1000115000): return "VK_STRUCTURE_TYPE_IMPORT_FENCE_FD_INFO_KHR";
        case static_cast<uint64_t>(1000115001): return "VK_STRUCTURE_TYPE_FENCE_GET_FD_INFO_KHR";
        case static_cast<uint64_t>(1000116000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_FEATURES_KHR";
        case static_cast<uint64_t>(1000116001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_PROPERTIES_KHR";
        case static_cast<uint64_t>(1000116002): return "VK_STRUCTURE_TYPE_QUERY_POOL_PERFORMANCE_CREATE_INFO_KHR";
        case static_cast<uint64_t>(1000116003): return "VK_STRUCTURE_TYPE_PERFORMANCE_QUERY_SUBMIT_INFO_KHR";
        case static_cast<uint64_t>(1000116004): return "VK_STRUCTURE_TYPE_ACQUIRE_PROFILING_LOCK_INFO_KHR";
        case static_cast<uint64_t>(1000116005): return "VK_STRUCTURE_TYPE_PERFORMANCE_COUNTER_KHR";
        case static_cast<uint64_t>(1000116006): return "VK_STRUCTURE_TYPE_PERFORMANCE_COUNTER_DESCRIPTION_KHR";
        case static_cast<uint64_t>(1000119000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR";
        case static_cast<uint64_t>(1000119001): return "VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR";
        case static_cast<uint64_t>(1000119002): return "VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR";
        case static_cast<uint64_t>(1000121000): return "VK_STRUCTURE_TYPE_DISPLAY_PROPERTIES_2_KHR";
        case static_cast<uint64_t>(1000121001): return "VK_STRUCTURE_TYPE_DISPLAY_PLANE_PROPERTIES_2_KHR";
        case static_cast<uint64_t>(1000121002): return "VK_STRUCTURE_TYPE_DISPLAY_MODE_PROPERTIES_2_KHR";
        case static_cast<uint64_t>(1000121003): return "VK_STRUCTURE_TYPE_DISPLAY_PLANE_INFO_2_KHR";
        case static_cast<uint64_t>(1000121004): return "VK_STRUCTURE_TYPE_DISPLAY_PLANE_CAPABILITIES_2_KHR";
        case static_cast<uint64_t>(1000122000): return "VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_MVK";
        case static_cast<uint64_t>(1000123000): return "VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK";
        case static_cast<uint64_t>(1000128000): return "VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT";
        case static_cast<uint64_t>(1000128001): return "VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_TAG_INFO_EXT";
        case static_cast<uint64_t>(1000128002): return "VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT";
        case static_cast<uint64_t>(1000128003): return "VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT";
        case static_cast<uint64_t>(1000128004): return "VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000129000): return "VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_USAGE_ANDROID";
        case static_cast<uint64_t>(1000129001): return "VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_PROPERTIES_ANDROID";
        case static_cast<uint64_t>(1000129002): return "VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_FORMAT_PROPERTIES_ANDROID";
        case static_cast<uint64_t>(1000129003): return "VK_STRUCTURE_TYPE_IMPORT_ANDROID_HARDWARE_BUFFER_INFO_ANDROID";
        case static_cast<uint64_t>(1000129004): return "VK_STRUCTURE_TYPE_MEMORY_GET_ANDROID_HARDWARE_BUFFER_INFO_ANDROID";
        case static_cast<uint64_t>(1000129005): return "VK_STRUCTURE_TYPE_EXTERNAL_FORMAT_ANDROID";
        case static_cast<uint64_t>(1000129006): return "VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_FORMAT_PROPERTIES_2_ANDROID";
        case static_cast<uint64_t>(1000143000): return "VK_STRUCTURE_TYPE_SAMPLE_LOCATIONS_INFO_EXT";
        case static_cast<uint64_t>(1000143001): return "VK_STRUCTURE_TYPE_RENDER_PASS_SAMPLE_LOCATIONS_BEGIN_INFO_EXT";
        case static_cast<uint64_t>(1000143002): return "VK_STRUCTURE_TYPE_PIPELINE_SAMPLE_LOCATIONS_STATE_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000143003): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLE_LOCATIONS_PROPERTIES_EXT";
        case static_cast<uint64_t>(1000143004): return "VK_STRUCTURE_TYPE_MULTISAMPLE_PROPERTIES_EXT";
        case static_cast<uint64_t>(1000148000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BLEND_OPERATION_ADVANCED_FEATURES_EXT";
        case static_cast<uint64_t>(1000148001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BLEND_OPERATION_ADVANCED_PROPERTIES_EXT";
        case static_cast<uint64_t>(1000148002): return "VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_ADVANCED_STATE_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000149000): return "VK_STRUCTURE_TYPE_PIPELINE_COVERAGE_TO_COLOR_STATE_CREATE_INFO_NV";
        case static_cast<uint64_t>(1000152000): return "VK_STRUCTURE_TYPE_PIPELINE_COVERAGE_MODULATION_STATE_CREATE_INFO_NV";
        case static_cast<uint64_t>(1000154000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SM_BUILTINS_FEATURES_NV";
        case static_cast<uint64_t>(1000154001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SM_BUILTINS_PROPERTIES_NV";
        case static_cast<uint64_t>(1000158000): return "VK_STRUCTURE_TYPE_DRM_FORMAT_MODIFIER_PROPERTIES_LIST_EXT";
        case static_cast<uint64_t>(1000158002): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_DRM_FORMAT_MODIFIER_INFO_EXT";
        case static_cast<uint64_t>(1000158003): return "VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_LIST_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000158004): return "VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_EXPLICIT_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000158005): return "VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_PROPERTIES_EXT";
        case static_cast<uint64_t>(1000158006): return "VK_STRUCTURE_TYPE_DRM_FORMAT_MODIFIER_PROPERTIES_LIST_2_EXT";
        case static_cast<uint64_t>(1000160000): return "VK_STRUCTURE_TYPE_VALIDATION_CACHE_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000160001): return "VK_STRUCTURE_TYPE_SHADER_MODULE_VALIDATION_CACHE_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000163000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PORTABILITY_SUBSET_FEATURES_KHR";
        case static_cast<uint64_t>(1000163001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PORTABILITY_SUBSET_PROPERTIES_KHR";
        case static_cast<uint64_t>(1000164000): return "VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_SHADING_RATE_IMAGE_STATE_CREATE_INFO_NV";
        case static_cast<uint64_t>(1000164001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_FEATURES_NV";
        case static_cast<uint64_t>(1000164002): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_PROPERTIES_NV";
        case static_cast<uint64_t>(1000164005): return "VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_COARSE_SAMPLE_ORDER_STATE_CREATE_INFO_NV";
        case static_cast<uint64_t>(1000165000): return "VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV";
        case static_cast<uint64_t>(1000165001): return "VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV";
        case static_cast<uint64_t>(1000165003): return "VK_STRUCTURE_TYPE_GEOMETRY_NV";
        case static_cast<uint64_t>(1000165004): return "VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV";
        case static_cast<uint64_t>(1000165005): return "VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV";
        case static_cast<uint64_t>(1000165006): return "VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV";
        case static_cast<uint64_t>(1000165007): return "VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV";
        case static_cast<uint64_t>(1000165008): return "VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV";
        case static_cast<uint64_t>(1000165009): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV";
        case static_cast<uint64_t>(1000165011): return "VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV";
        case static_cast<uint64_t>(1000165012): return "VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV";
        case static_cast<uint64_t>(1000166000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_REPRESENTATIVE_FRAGMENT_TEST_FEATURES_NV";
        case static_cast<uint64_t>(1000166001): return "VK_STRUCTURE_TYPE_PIPELINE_REPRESENTATIVE_FRAGMENT_TEST_STATE_CREATE_INFO_NV";
        case static_cast<uint64_t>(1000170000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_VIEW_IMAGE_FORMAT_INFO_EXT";
        case static_cast<uint64_t>(1000170001): return "VK_STRUCTURE_TYPE_FILTER_CUBIC_IMAGE_VIEW_IMAGE_FORMAT_PROPERTIES_EXT";
        case static_cast<uint64_t>(1000178000): return "VK_STRUCTURE_TYPE_IMPORT_MEMORY_HOST_POINTER_INFO_EXT";
        case static_cast<uint64_t>(1000178001): return "VK_STRUCTURE_TYPE_MEMORY_HOST_POINTER_PROPERTIES_EXT";
        case static_cast<uint64_t>(1000178002): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_HOST_PROPERTIES_EXT";
        case static_cast<uint64_t>(1000181000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CLOCK_FEATURES_KHR";
        case static_cast<uint64_t>(1000183000): return "VK_STRUCTURE_TYPE_PIPELINE_COMPILER_CONTROL_CREATE_INFO_AMD";
        case static_cast<uint64_t>(1000184000): return "VK_STRUCTURE_TYPE_CALIBRATED_TIMESTAMP_INFO_EXT";
        case static_cast<uint64_t>(1000185000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_PROPERTIES_AMD";
        case static_cast<uint64_t>(1000187000): return "VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_CAPABILITIES_EXT";
        case static_cast<uint64_t>(1000187001): return "VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_SESSION_PARAMETERS_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000187002): return "VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_SESSION_PARAMETERS_ADD_INFO_EXT";
        case static_cast<uint64_t>(1000187003): return "VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_PROFILE_EXT";
        case static_cast<uint64_t>(1000187004): return "VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_PICTURE_INFO_EXT";
        case static_cast<uint64_t>(1000187005): return "VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_DPB_SLOT_INFO_EXT";
        case static_cast<uint64_t>(1000174000): return "VK_STRUCTURE_TYPE_DEVICE_QUEUE_GLOBAL_PRIORITY_CREATE_INFO_KHR";
        case static_cast<uint64_t>(1000388000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GLOBAL_PRIORITY_QUERY_FEATURES_KHR";
        case static_cast<uint64_t>(1000388001): return "VK_STRUCTURE_TYPE_QUEUE_FAMILY_GLOBAL_PRIORITY_PROPERTIES_KHR";
        case static_cast<uint64_t>(1000189000): return "VK_STRUCTURE_TYPE_DEVICE_MEMORY_OVERALLOCATION_CREATE_INFO_AMD";
        case static_cast<uint64_t>(1000190000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_PROPERTIES_EXT";
        case static_cast<uint64_t>(1000190001): return "VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_DIVISOR_STATE_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000190002): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES_EXT";
        case static_cast<uint64_t>(1000191000): return "VK_STRUCTURE_TYPE_PRESENT_FRAME_TOKEN_GGP";
        case static_cast<uint64_t>(1000201000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMPUTE_SHADER_DERIVATIVES_FEATURES_NV";
        case static_cast<uint64_t>(1000202000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV";
        case static_cast<uint64_t>(1000202001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_NV";
        case static_cast<uint64_t>(1000204000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_FOOTPRINT_FEATURES_NV";
        case static_cast<uint64_t>(1000205000): return "VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_EXCLUSIVE_SCISSOR_STATE_CREATE_INFO_NV";
        case static_cast<uint64_t>(1000205002): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXCLUSIVE_SCISSOR_FEATURES_NV";
        case static_cast<uint64_t>(1000206000): return "VK_STRUCTURE_TYPE_CHECKPOINT_DATA_NV";
        case static_cast<uint64_t>(1000206001): return "VK_STRUCTURE_TYPE_QUEUE_FAMILY_CHECKPOINT_PROPERTIES_NV";
        case static_cast<uint64_t>(1000209000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_FUNCTIONS_2_FEATURES_INTEL";
        case static_cast<uint64_t>(1000210000): return "VK_STRUCTURE_TYPE_QUERY_POOL_PERFORMANCE_QUERY_CREATE_INFO_INTEL";
        case static_cast<uint64_t>(1000210001): return "VK_STRUCTURE_TYPE_INITIALIZE_PERFORMANCE_API_INFO_INTEL";
        case static_cast<uint64_t>(1000210002): return "VK_STRUCTURE_TYPE_PERFORMANCE_MARKER_INFO_INTEL";
        case static_cast<uint64_t>(1000210003): return "VK_STRUCTURE_TYPE_PERFORMANCE_STREAM_MARKER_INFO_INTEL";
        case static_cast<uint64_t>(1000210004): return "VK_STRUCTURE_TYPE_PERFORMANCE_OVERRIDE_INFO_INTEL";
        case static_cast<uint64_t>(1000210005): return "VK_STRUCTURE_TYPE_PERFORMANCE_CONFIGURATION_ACQUIRE_INFO_INTEL";
        case static_cast<uint64_t>(1000212000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PCI_BUS_INFO_PROPERTIES_EXT";
        case static_cast<uint64_t>(1000213000): return "VK_STRUCTURE_TYPE_DISPLAY_NATIVE_HDR_SURFACE_CAPABILITIES_AMD";
        case static_cast<uint64_t>(1000213001): return "VK_STRUCTURE_TYPE_SWAPCHAIN_DISPLAY_NATIVE_HDR_CREATE_INFO_AMD";
        case static_cast<uint64_t>(1000214000): return "VK_STRUCTURE_TYPE_IMAGEPIPE_SURFACE_CREATE_INFO_FUCHSIA";
        case static_cast<uint64_t>(1000217000): return "VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000218000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_FEATURES_EXT";
        case static_cast<uint64_t>(1000218001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_PROPERTIES_EXT";
        case static_cast<uint64_t>(1000218002): return "VK_STRUCTURE_TYPE_RENDER_PASS_FRAGMENT_DENSITY_MAP_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000226000): return "VK_STRUCTURE_TYPE_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR";
        case static_cast<uint64_t>(1000226001): return "VK_STRUCTURE_TYPE_PIPELINE_FRAGMENT_SHADING_RATE_STATE_CREATE_INFO_KHR";
        case static_cast<uint64_t>(1000226002): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR";
        case static_cast<uint64_t>(1000226003): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR";
        case static_cast<uint64_t>(1000226004): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_KHR";
        case static_cast<uint64_t>(1000227000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_PROPERTIES_2_AMD";
        case static_cast<uint64_t>(1000229000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COHERENT_MEMORY_FEATURES_AMD";
        case static_cast<uint64_t>(1000234000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_ATOMIC_INT64_FEATURES_EXT";
        case static_cast<uint64_t>(1000237000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT";
        case static_cast<uint64_t>(1000238000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT";
        case static_cast<uint64_t>(1000238001): return "VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT";
        case static_cast<uint64_t>(1000239000): return "VK_STRUCTURE_TYPE_SURFACE_PROTECTED_CAPABILITIES_KHR";
        case static_cast<uint64_t>(1000240000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEDICATED_ALLOCATION_IMAGE_ALIASING_FEATURES_NV";
        case static_cast<uint64_t>(1000244000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_EXT";
        case static_cast<uint64_t>(1000244002): return "VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000247000): return "VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT";
        case static_cast<uint64_t>(1000248000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR";
        case static_cast<uint64_t>(1000249000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_NV";
        case static_cast<uint64_t>(1000249001): return "VK_STRUCTURE_TYPE_COOPERATIVE_MATRIX_PROPERTIES_NV";
        case static_cast<uint64_t>(1000249002): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_PROPERTIES_NV";
        case static_cast<uint64_t>(1000250000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COVERAGE_REDUCTION_MODE_FEATURES_NV";
        case static_cast<uint64_t>(1000250001): return "VK_STRUCTURE_TYPE_PIPELINE_COVERAGE_REDUCTION_STATE_CREATE_INFO_NV";
        case static_cast<uint64_t>(1000250002): return "VK_STRUCTURE_TYPE_FRAMEBUFFER_MIXED_SAMPLES_COMBINATION_NV";
        case static_cast<uint64_t>(1000251000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT";
        case static_cast<uint64_t>(1000252000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_YCBCR_IMAGE_ARRAYS_FEATURES_EXT";
        case static_cast<uint64_t>(1000254000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROVOKING_VERTEX_FEATURES_EXT";
        case static_cast<uint64_t>(1000254001): return "VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_PROVOKING_VERTEX_STATE_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000254002): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROVOKING_VERTEX_PROPERTIES_EXT";
        case static_cast<uint64_t>(1000255000): return "VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT";
        case static_cast<uint64_t>(1000255002): return "VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_FULL_SCREEN_EXCLUSIVE_EXT";
        case static_cast<uint64_t>(1000255001): return "VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_WIN32_INFO_EXT";
        case static_cast<uint64_t>(1000256000): return "VK_STRUCTURE_TYPE_HEADLESS_SURFACE_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000259000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT";
        case static_cast<uint64_t>(1000259001): return "VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_LINE_STATE_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000259002): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_PROPERTIES_EXT";
        case static_cast<uint64_t>(1000260000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT";
        case static_cast<uint64_t>(1000265000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES_EXT";
        case static_cast<uint64_t>(1000267000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT";
        case static_cast<uint64_t>(1000269000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR";
        case static_cast<uint64_t>(1000269001): return "VK_STRUCTURE_TYPE_PIPELINE_INFO_KHR";
        case static_cast<uint64_t>(1000269002): return "VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_PROPERTIES_KHR";
        case static_cast<uint64_t>(1000269003): return "VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INFO_KHR";
        case static_cast<uint64_t>(1000269004): return "VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_STATISTIC_KHR";
        case static_cast<uint64_t>(1000269005): return "VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INTERNAL_REPRESENTATION_KHR";
        case static_cast<uint64_t>(1000273000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_2_FEATURES_EXT";
        case static_cast<uint64_t>(1000277000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_PROPERTIES_NV";
        case static_cast<uint64_t>(1000277001): return "VK_STRUCTURE_TYPE_GRAPHICS_SHADER_GROUP_CREATE_INFO_NV";
        case static_cast<uint64_t>(1000277002): return "VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_SHADER_GROUPS_CREATE_INFO_NV";
        case static_cast<uint64_t>(1000277003): return "VK_STRUCTURE_TYPE_INDIRECT_COMMANDS_LAYOUT_TOKEN_NV";
        case static_cast<uint64_t>(1000277004): return "VK_STRUCTURE_TYPE_INDIRECT_COMMANDS_LAYOUT_CREATE_INFO_NV";
        case static_cast<uint64_t>(1000277005): return "VK_STRUCTURE_TYPE_GENERATED_COMMANDS_INFO_NV";
        case static_cast<uint64_t>(1000277006): return "VK_STRUCTURE_TYPE_GENERATED_COMMANDS_MEMORY_REQUIREMENTS_INFO_NV";
        case static_cast<uint64_t>(1000277007): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_FEATURES_NV";
        case static_cast<uint64_t>(1000278000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INHERITED_VIEWPORT_SCISSOR_FEATURES_NV";
        case static_cast<uint64_t>(1000278001): return "VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_VIEWPORT_SCISSOR_INFO_NV";
        case static_cast<uint64_t>(1000281000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXEL_BUFFER_ALIGNMENT_FEATURES_EXT";
        case static_cast<uint64_t>(1000282000): return "VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDER_PASS_TRANSFORM_INFO_QCOM";
        case static_cast<uint64_t>(1000282001): return "VK_STRUCTURE_TYPE_RENDER_PASS_TRANSFORM_BEGIN_INFO_QCOM";
        case static_cast<uint64_t>(1000284000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_MEMORY_REPORT_FEATURES_EXT";
        case static_cast<uint64_t>(1000284001): return "VK_STRUCTURE_TYPE_DEVICE_DEVICE_MEMORY_REPORT_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000284002): return "VK_STRUCTURE_TYPE_DEVICE_MEMORY_REPORT_CALLBACK_DATA_EXT";
        case static_cast<uint64_t>(1000286000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT";
        case static_cast<uint64_t>(1000286001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_PROPERTIES_EXT";
        case static_cast<uint64_t>(1000287000): return "VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000287001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_PROPERTIES_EXT";
        case static_cast<uint64_t>(1000287002): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT";
        case static_cast<uint64_t>(1000290000): return "VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR";
        case static_cast<uint64_t>(1000294000): return "VK_STRUCTURE_TYPE_PRESENT_ID_KHR";
        case static_cast<uint64_t>(1000294001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR";
        case static_cast<uint64_t>(1000299000): return "VK_STRUCTURE_TYPE_VIDEO_ENCODE_INFO_KHR";
        case static_cast<uint64_t>(1000299001): return "VK_STRUCTURE_TYPE_VIDEO_ENCODE_RATE_CONTROL_INFO_KHR";
        case static_cast<uint64_t>(1000299002): return "VK_STRUCTURE_TYPE_VIDEO_ENCODE_RATE_CONTROL_LAYER_INFO_KHR";
        case static_cast<uint64_t>(1000299003): return "VK_STRUCTURE_TYPE_VIDEO_ENCODE_CAPABILITIES_KHR";
        case static_cast<uint64_t>(1000300000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DIAGNOSTICS_CONFIG_FEATURES_NV";
        case static_cast<uint64_t>(1000300001): return "VK_STRUCTURE_TYPE_DEVICE_DIAGNOSTICS_CONFIG_CREATE_INFO_NV";
        case static_cast<uint64_t>(1000311000): return "VK_STRUCTURE_TYPE_EXPORT_METAL_OBJECT_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000311001): return "VK_STRUCTURE_TYPE_EXPORT_METAL_OBJECTS_INFO_EXT";
        case static_cast<uint64_t>(1000311002): return "VK_STRUCTURE_TYPE_EXPORT_METAL_DEVICE_INFO_EXT";
        case static_cast<uint64_t>(1000311003): return "VK_STRUCTURE_TYPE_EXPORT_METAL_COMMAND_QUEUE_INFO_EXT";
        case static_cast<uint64_t>(1000311004): return "VK_STRUCTURE_TYPE_EXPORT_METAL_BUFFER_INFO_EXT";
        case static_cast<uint64_t>(1000311005): return "VK_STRUCTURE_TYPE_IMPORT_METAL_BUFFER_INFO_EXT";
        case static_cast<uint64_t>(1000311006): return "VK_STRUCTURE_TYPE_EXPORT_METAL_TEXTURE_INFO_EXT";
        case static_cast<uint64_t>(1000311007): return "VK_STRUCTURE_TYPE_IMPORT_METAL_TEXTURE_INFO_EXT";
        case static_cast<uint64_t>(1000311008): return "VK_STRUCTURE_TYPE_EXPORT_METAL_IO_SURFACE_INFO_EXT";
        case static_cast<uint64_t>(1000311009): return "VK_STRUCTURE_TYPE_IMPORT_METAL_IO_SURFACE_INFO_EXT";
        case static_cast<uint64_t>(1000311010): return "VK_STRUCTURE_TYPE_EXPORT_METAL_SHARED_EVENT_INFO_EXT";
        case static_cast<uint64_t>(1000311011): return "VK_STRUCTURE_TYPE_IMPORT_METAL_SHARED_EVENT_INFO_EXT";
        case static_cast<uint64_t>(1000314008): return "VK_STRUCTURE_TYPE_QUEUE_FAMILY_CHECKPOINT_PROPERTIES_2_NV";
        case static_cast<uint64_t>(1000314009): return "VK_STRUCTURE_TYPE_CHECKPOINT_DATA_2_NV";
        case static_cast<uint64_t>(1000320000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GRAPHICS_PIPELINE_LIBRARY_FEATURES_EXT";
        case static_cast<uint64_t>(1000320001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GRAPHICS_PIPELINE_LIBRARY_PROPERTIES_EXT";
        case static_cast<uint64_t>(1000320002): return "VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000321000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_EARLY_AND_LATE_FRAGMENT_TESTS_FEATURES_AMD";
        case static_cast<uint64_t>(1000203000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR";
        case static_cast<uint64_t>(1000322000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_PROPERTIES_KHR";
        case static_cast<uint64_t>(1000323000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_UNIFORM_CONTROL_FLOW_FEATURES_KHR";
        case static_cast<uint64_t>(1000326000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_ENUMS_PROPERTIES_NV";
        case static_cast<uint64_t>(1000326001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_ENUMS_FEATURES_NV";
        case static_cast<uint64_t>(1000326002): return "VK_STRUCTURE_TYPE_PIPELINE_FRAGMENT_SHADING_RATE_ENUM_STATE_CREATE_INFO_NV";
        case static_cast<uint64_t>(1000327000): return "VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_MOTION_TRIANGLES_DATA_NV";
        case static_cast<uint64_t>(1000327001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_MOTION_BLUR_FEATURES_NV";
        case static_cast<uint64_t>(1000327002): return "VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MOTION_INFO_NV";
        case static_cast<uint64_t>(1000330000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_YCBCR_2_PLANE_444_FORMATS_FEATURES_EXT";
        case static_cast<uint64_t>(1000332000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_2_FEATURES_EXT";
        case static_cast<uint64_t>(1000332001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_2_PROPERTIES_EXT";
        case static_cast<uint64_t>(1000333000): return "VK_STRUCTURE_TYPE_COPY_COMMAND_TRANSFORM_INFO_QCOM";
        case static_cast<uint64_t>(1000336000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_WORKGROUP_MEMORY_EXPLICIT_LAYOUT_FEATURES_KHR";
        case static_cast<uint64_t>(1000338000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_COMPRESSION_CONTROL_FEATURES_EXT";
        case static_cast<uint64_t>(1000338001): return "VK_STRUCTURE_TYPE_IMAGE_COMPRESSION_CONTROL_EXT";
        case static_cast<uint64_t>(1000338002): return "VK_STRUCTURE_TYPE_SUBRESOURCE_LAYOUT_2_EXT";
        case static_cast<uint64_t>(1000338003): return "VK_STRUCTURE_TYPE_IMAGE_SUBRESOURCE_2_EXT";
        case static_cast<uint64_t>(1000338004): return "VK_STRUCTURE_TYPE_IMAGE_COMPRESSION_PROPERTIES_EXT";
        case static_cast<uint64_t>(1000340000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_4444_FORMATS_FEATURES_EXT";
        case static_cast<uint64_t>(1000342000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RASTERIZATION_ORDER_ATTACHMENT_ACCESS_FEATURES_ARM";
        case static_cast<uint64_t>(1000344000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RGBA10X6_FORMATS_FEATURES_EXT";
        case static_cast<uint64_t>(1000346000): return "VK_STRUCTURE_TYPE_DIRECTFB_SURFACE_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000351000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_VALVE";
        case static_cast<uint64_t>(1000351002): return "VK_STRUCTURE_TYPE_MUTABLE_DESCRIPTOR_TYPE_CREATE_INFO_VALVE";
        case static_cast<uint64_t>(1000352000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_INPUT_DYNAMIC_STATE_FEATURES_EXT";
        case static_cast<uint64_t>(1000352001): return "VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT";
        case static_cast<uint64_t>(1000352002): return "VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT";
        case static_cast<uint64_t>(1000353000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRM_PROPERTIES_EXT";
        case static_cast<uint64_t>(1000355000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_CONTROL_FEATURES_EXT";
        case static_cast<uint64_t>(1000355001): return "VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_DEPTH_CLIP_CONTROL_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000356000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIMITIVE_TOPOLOGY_LIST_RESTART_FEATURES_EXT";
        case static_cast<uint64_t>(1000364000): return "VK_STRUCTURE_TYPE_IMPORT_MEMORY_ZIRCON_HANDLE_INFO_FUCHSIA";
        case static_cast<uint64_t>(1000364001): return "VK_STRUCTURE_TYPE_MEMORY_ZIRCON_HANDLE_PROPERTIES_FUCHSIA";
        case static_cast<uint64_t>(1000364002): return "VK_STRUCTURE_TYPE_MEMORY_GET_ZIRCON_HANDLE_INFO_FUCHSIA";
        case static_cast<uint64_t>(1000365000): return "VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_ZIRCON_HANDLE_INFO_FUCHSIA";
        case static_cast<uint64_t>(1000365001): return "VK_STRUCTURE_TYPE_SEMAPHORE_GET_ZIRCON_HANDLE_INFO_FUCHSIA";
        case static_cast<uint64_t>(1000366000): return "VK_STRUCTURE_TYPE_BUFFER_COLLECTION_CREATE_INFO_FUCHSIA";
        case static_cast<uint64_t>(1000366001): return "VK_STRUCTURE_TYPE_IMPORT_MEMORY_BUFFER_COLLECTION_FUCHSIA";
        case static_cast<uint64_t>(1000366002): return "VK_STRUCTURE_TYPE_BUFFER_COLLECTION_IMAGE_CREATE_INFO_FUCHSIA";
        case static_cast<uint64_t>(1000366003): return "VK_STRUCTURE_TYPE_BUFFER_COLLECTION_PROPERTIES_FUCHSIA";
        case static_cast<uint64_t>(1000366004): return "VK_STRUCTURE_TYPE_BUFFER_CONSTRAINTS_INFO_FUCHSIA";
        case static_cast<uint64_t>(1000366005): return "VK_STRUCTURE_TYPE_BUFFER_COLLECTION_BUFFER_CREATE_INFO_FUCHSIA";
        case static_cast<uint64_t>(1000366006): return "VK_STRUCTURE_TYPE_IMAGE_CONSTRAINTS_INFO_FUCHSIA";
        case static_cast<uint64_t>(1000366007): return "VK_STRUCTURE_TYPE_IMAGE_FORMAT_CONSTRAINTS_INFO_FUCHSIA";
        case static_cast<uint64_t>(1000366008): return "VK_STRUCTURE_TYPE_SYSMEM_COLOR_SPACE_FUCHSIA";
        case static_cast<uint64_t>(1000366009): return "VK_STRUCTURE_TYPE_BUFFER_COLLECTION_CONSTRAINTS_INFO_FUCHSIA";
        case static_cast<uint64_t>(1000369000): return "VK_STRUCTURE_TYPE_SUBPASS_SHADING_PIPELINE_CREATE_INFO_HUAWEI";
        case static_cast<uint64_t>(1000369001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBPASS_SHADING_FEATURES_HUAWEI";
        case static_cast<uint64_t>(1000369002): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBPASS_SHADING_PROPERTIES_HUAWEI";
        case static_cast<uint64_t>(1000370000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INVOCATION_MASK_FEATURES_HUAWEI";
        case static_cast<uint64_t>(1000371000): return "VK_STRUCTURE_TYPE_MEMORY_GET_REMOTE_ADDRESS_INFO_NV";
        case static_cast<uint64_t>(1000371001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_RDMA_FEATURES_NV";
        case static_cast<uint64_t>(1000372000): return "VK_STRUCTURE_TYPE_PIPELINE_PROPERTIES_IDENTIFIER_EXT";
        case static_cast<uint64_t>(1000372001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_PROPERTIES_FEATURES_EXT";
        case static_cast<uint64_t>(1000376000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTISAMPLED_RENDER_TO_SINGLE_SAMPLED_FEATURES_EXT";
        case static_cast<uint64_t>(1000376001): return "VK_STRUCTURE_TYPE_SUBPASS_RESOLVE_PERFORMANCE_QUERY_EXT";
        case static_cast<uint64_t>(1000376002): return "VK_STRUCTURE_TYPE_MULTISAMPLED_RENDER_TO_SINGLE_SAMPLED_INFO_EXT";
        case static_cast<uint64_t>(1000377000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT";
        case static_cast<uint64_t>(1000378000): return "VK_STRUCTURE_TYPE_SCREEN_SURFACE_CREATE_INFO_QNX";
        case static_cast<uint64_t>(1000381000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COLOR_WRITE_ENABLE_FEATURES_EXT";
        case static_cast<uint64_t>(1000381001): return "VK_STRUCTURE_TYPE_PIPELINE_COLOR_WRITE_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000382000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIMITIVES_GENERATED_QUERY_FEATURES_EXT";
        case static_cast<uint64_t>(1000386000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_MAINTENANCE_1_FEATURES_KHR";
        case static_cast<uint64_t>(1000391000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_VIEW_MIN_LOD_FEATURES_EXT";
        case static_cast<uint64_t>(1000391001): return "VK_STRUCTURE_TYPE_IMAGE_VIEW_MIN_LOD_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000392000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTI_DRAW_FEATURES_EXT";
        case static_cast<uint64_t>(1000392001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTI_DRAW_PROPERTIES_EXT";
        case static_cast<uint64_t>(1000393000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_2D_VIEW_OF_3D_FEATURES_EXT";
        case static_cast<uint64_t>(1000411000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BORDER_COLOR_SWIZZLE_FEATURES_EXT";
        case static_cast<uint64_t>(1000411001): return "VK_STRUCTURE_TYPE_SAMPLER_BORDER_COLOR_COMPONENT_MAPPING_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000412000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PAGEABLE_DEVICE_LOCAL_MEMORY_FEATURES_EXT";
        case static_cast<uint64_t>(1000420000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_SET_HOST_MAPPING_FEATURES_VALVE";
        case static_cast<uint64_t>(1000420001): return "VK_STRUCTURE_TYPE_DESCRIPTOR_SET_BINDING_REFERENCE_VALVE";
        case static_cast<uint64_t>(1000420002): return "VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_HOST_MAPPING_INFO_VALVE";
        case static_cast<uint64_t>(1000422000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_NON_SEAMLESS_CUBE_MAP_FEATURES_EXT";
        case static_cast<uint64_t>(1000425000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_OFFSET_FEATURES_QCOM";
        case static_cast<uint64_t>(1000425001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_OFFSET_PROPERTIES_QCOM";
        case static_cast<uint64_t>(1000425002): return "VK_STRUCTURE_TYPE_SUBPASS_FRAGMENT_DENSITY_MAP_OFFSET_END_INFO_QCOM";
        case static_cast<uint64_t>(1000430000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINEAR_COLOR_ATTACHMENT_FEATURES_NV";
        case static_cast<uint64_t>(1000437000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_COMPRESSION_CONTROL_SWAPCHAIN_FEATURES_EXT";
        case static_cast<uint64_t>(1000458000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBPASS_MERGE_FEEDBACK_FEATURES_EXT";
        case static_cast<uint64_t>(1000458001): return "VK_STRUCTURE_TYPE_RENDER_PASS_CREATION_CONTROL_EXT";
        case static_cast<uint64_t>(1000458002): return "VK_STRUCTURE_TYPE_RENDER_PASS_CREATION_FEEDBACK_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000458003): return "VK_STRUCTURE_TYPE_RENDER_PASS_SUBPASS_FEEDBACK_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000462000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_MODULE_IDENTIFIER_FEATURES_EXT";
        case static_cast<uint64_t>(1000462001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_MODULE_IDENTIFIER_PROPERTIES_EXT";
        case static_cast<uint64_t>(1000462002): return "VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_MODULE_IDENTIFIER_CREATE_INFO_EXT";
        case static_cast<uint64_t>(1000462003): return "VK_STRUCTURE_TYPE_SHADER_MODULE_IDENTIFIER_EXT";
        case static_cast<uint64_t>(1000150007): return "VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR";
        case static_cast<uint64_t>(1000150000): return "VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR";
        case static_cast<uint64_t>(1000150002): return "VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR";
        case static_cast<uint64_t>(1000150003): return "VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR";
        case static_cast<uint64_t>(1000150004): return "VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR";
        case static_cast<uint64_t>(1000150005): return "VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR";
        case static_cast<uint64_t>(1000150006): return "VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR";
        case static_cast<uint64_t>(1000150009): return "VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_VERSION_INFO_KHR";
        case static_cast<uint64_t>(1000150010): return "VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR";
        case static_cast<uint64_t>(1000150011): return "VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_TO_MEMORY_INFO_KHR";
        case static_cast<uint64_t>(1000150012): return "VK_STRUCTURE_TYPE_COPY_MEMORY_TO_ACCELERATION_STRUCTURE_INFO_KHR";
        case static_cast<uint64_t>(1000150013): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR";
        case static_cast<uint64_t>(1000150014): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR";
        case static_cast<uint64_t>(1000150017): return "VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR";
        case static_cast<uint64_t>(1000150020): return "VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR";
        case static_cast<uint64_t>(1000347000): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR";
        case static_cast<uint64_t>(1000347001): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR";
        case static_cast<uint64_t>(1000150015): return "VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR";
        case static_cast<uint64_t>(1000150016): return "VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR";
        case static_cast<uint64_t>(1000150018): return "VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_INTERFACE_CREATE_INFO_KHR";
        case static_cast<uint64_t>(1000348013): return "VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR";
        default: return "VkStructureType_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkSubgroupFeatureFlagBits>(VkSubgroupFeatureFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_SUBGROUP_FEATURE_BASIC_BIT";
        case static_cast<uint64_t>(2): return "VK_SUBGROUP_FEATURE_VOTE_BIT";
        case static_cast<uint64_t>(4): return "VK_SUBGROUP_FEATURE_ARITHMETIC_BIT";
        case static_cast<uint64_t>(8): return "VK_SUBGROUP_FEATURE_BALLOT_BIT";
        case static_cast<uint64_t>(16): return "VK_SUBGROUP_FEATURE_SHUFFLE_BIT";
        case static_cast<uint64_t>(32): return "VK_SUBGROUP_FEATURE_SHUFFLE_RELATIVE_BIT";
        case static_cast<uint64_t>(64): return "VK_SUBGROUP_FEATURE_CLUSTERED_BIT";
        case static_cast<uint64_t>(128): return "VK_SUBGROUP_FEATURE_QUAD_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_SUBGROUP_FEATURE_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(256): return "VK_SUBGROUP_FEATURE_PARTITIONED_BIT_NV";
        default: return "VkSubgroupFeatureFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkSubmitFlagBits>(VkSubmitFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_SUBMIT_PROTECTED_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_SUBMIT_FLAG_BITS_MAX_ENUM";
        default: return "VkSubmitFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkSubpassContents>(VkSubpassContents type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_SUBPASS_CONTENTS_INLINE";
        case static_cast<uint64_t>(1): return "VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS";
        case static_cast<uint64_t>(2147483647): return "VK_SUBPASS_CONTENTS_MAX_ENUM";
        default: return "VkSubpassContents_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkSubpassDescriptionFlagBits>(VkSubpassDescriptionFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(2147483647): return "VK_SUBPASS_DESCRIPTION_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(4): return "VK_SUBPASS_DESCRIPTION_FRAGMENT_REGION_BIT_QCOM";
        case static_cast<uint64_t>(8): return "VK_SUBPASS_DESCRIPTION_SHADER_RESOLVE_BIT_QCOM";
        case static_cast<uint64_t>(16): return "VK_SUBPASS_DESCRIPTION_RASTERIZATION_ORDER_ATTACHMENT_COLOR_ACCESS_BIT_ARM";
        case static_cast<uint64_t>(32): return "VK_SUBPASS_DESCRIPTION_RASTERIZATION_ORDER_ATTACHMENT_DEPTH_ACCESS_BIT_ARM";
        case static_cast<uint64_t>(64): return "VK_SUBPASS_DESCRIPTION_RASTERIZATION_ORDER_ATTACHMENT_STENCIL_ACCESS_BIT_ARM";
        default: return "VkSubpassDescriptionFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkSubpassMergeStatusEXT>(VkSubpassMergeStatusEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_SUBPASS_MERGE_STATUS_MERGED_EXT";
        case static_cast<uint64_t>(1): return "VK_SUBPASS_MERGE_STATUS_DISALLOWED_EXT";
        case static_cast<uint64_t>(2): return "VK_SUBPASS_MERGE_STATUS_NOT_MERGED_SIDE_EFFECTS_EXT";
        case static_cast<uint64_t>(3): return "VK_SUBPASS_MERGE_STATUS_NOT_MERGED_SAMPLES_MISMATCH_EXT";
        case static_cast<uint64_t>(4): return "VK_SUBPASS_MERGE_STATUS_NOT_MERGED_VIEWS_MISMATCH_EXT";
        case static_cast<uint64_t>(5): return "VK_SUBPASS_MERGE_STATUS_NOT_MERGED_ALIASING_EXT";
        case static_cast<uint64_t>(6): return "VK_SUBPASS_MERGE_STATUS_NOT_MERGED_DEPENDENCIES_EXT";
        case static_cast<uint64_t>(7): return "VK_SUBPASS_MERGE_STATUS_NOT_MERGED_INCOMPATIBLE_INPUT_ATTACHMENT_EXT";
        case static_cast<uint64_t>(8): return "VK_SUBPASS_MERGE_STATUS_NOT_MERGED_TOO_MANY_ATTACHMENTS_EXT";
        case static_cast<uint64_t>(9): return "VK_SUBPASS_MERGE_STATUS_NOT_MERGED_INSUFFICIENT_STORAGE_EXT";
        case static_cast<uint64_t>(10): return "VK_SUBPASS_MERGE_STATUS_NOT_MERGED_DEPTH_STENCIL_COUNT_EXT";
        case static_cast<uint64_t>(11): return "VK_SUBPASS_MERGE_STATUS_NOT_MERGED_RESOLVE_ATTACHMENT_REUSE_EXT";
        case static_cast<uint64_t>(12): return "VK_SUBPASS_MERGE_STATUS_NOT_MERGED_SINGLE_SUBPASS_EXT";
        case static_cast<uint64_t>(13): return "VK_SUBPASS_MERGE_STATUS_NOT_MERGED_UNSPECIFIED_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_SUBPASS_MERGE_STATUS_EXT_MAX_ENUM";
        default: return "VkSubpassMergeStatusEXT_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkSurfaceCounterFlagBitsEXT>(VkSurfaceCounterFlagBitsEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_SURFACE_COUNTER_VBLANK_BIT_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_SURFACE_COUNTER_FLAG_BITS_EXT_MAX_ENUM";
        default: return "VkSurfaceCounterFlagBitsEXT_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkSurfaceTransformFlagBitsKHR>(VkSurfaceTransformFlagBitsKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR";
        case static_cast<uint64_t>(2): return "VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR";
        case static_cast<uint64_t>(4): return "VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR";
        case static_cast<uint64_t>(8): return "VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR";
        case static_cast<uint64_t>(16): return "VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR";
        case static_cast<uint64_t>(32): return "VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR";
        case static_cast<uint64_t>(64): return "VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR";
        case static_cast<uint64_t>(128): return "VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR";
        case static_cast<uint64_t>(256): return "VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_SURFACE_TRANSFORM_FLAG_BITS_KHR_MAX_ENUM";
        default: return "VkSurfaceTransformFlagBitsKHR_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkSwapchainCreateFlagBitsKHR>(VkSwapchainCreateFlagBitsKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(2147483647): return "VK_SWAPCHAIN_CREATE_FLAG_BITS_KHR_MAX_ENUM";
        case static_cast<uint64_t>(1): return "VK_SWAPCHAIN_CREATE_SPLIT_INSTANCE_BIND_REGIONS_BIT_KHR";
        case static_cast<uint64_t>(2): return "VK_SWAPCHAIN_CREATE_PROTECTED_BIT_KHR";
        case static_cast<uint64_t>(4): return "VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR";
        default: return "VkSwapchainCreateFlagBitsKHR_UNKNOWN";
      }
    }

#ifdef VKROOTS_PLATFORM_UNSUPPORTED
    template <> constexpr const char* enumString<VkSwapchainImageUsageFlagBitsANDROID>(VkSwapchainImageUsageFlagBitsANDROID type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_SWAPCHAIN_IMAGE_USAGE_SHARED_BIT_ANDROID";
        case static_cast<uint64_t>(2147483647): return "VK_SWAPCHAIN_IMAGE_USAGE_FLAG_BITS_ANDROID_MAX_ENUM";
        default: return "VkSwapchainImageUsageFlagBitsANDROID_UNKNOWN";
      }
    }
#endif

    template <> constexpr const char* enumString<VkSystemAllocationScope>(VkSystemAllocationScope type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_SYSTEM_ALLOCATION_SCOPE_COMMAND";
        case static_cast<uint64_t>(1): return "VK_SYSTEM_ALLOCATION_SCOPE_OBJECT";
        case static_cast<uint64_t>(2): return "VK_SYSTEM_ALLOCATION_SCOPE_CACHE";
        case static_cast<uint64_t>(3): return "VK_SYSTEM_ALLOCATION_SCOPE_DEVICE";
        case static_cast<uint64_t>(4): return "VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE";
        case static_cast<uint64_t>(2147483647): return "VK_SYSTEM_ALLOCATION_SCOPE_MAX_ENUM";
        default: return "VkSystemAllocationScope_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkTessellationDomainOrigin>(VkTessellationDomainOrigin type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_TESSELLATION_DOMAIN_ORIGIN_UPPER_LEFT";
        case static_cast<uint64_t>(1): return "VK_TESSELLATION_DOMAIN_ORIGIN_LOWER_LEFT";
        case static_cast<uint64_t>(2147483647): return "VK_TESSELLATION_DOMAIN_ORIGIN_MAX_ENUM";
        default: return "VkTessellationDomainOrigin_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkTimeDomainEXT>(VkTimeDomainEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_TIME_DOMAIN_DEVICE_EXT";
        case static_cast<uint64_t>(1): return "VK_TIME_DOMAIN_CLOCK_MONOTONIC_EXT";
        case static_cast<uint64_t>(2): return "VK_TIME_DOMAIN_CLOCK_MONOTONIC_RAW_EXT";
        case static_cast<uint64_t>(3): return "VK_TIME_DOMAIN_QUERY_PERFORMANCE_COUNTER_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_TIME_DOMAIN_EXT_MAX_ENUM";
        default: return "VkTimeDomainEXT_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkToolPurposeFlagBits>(VkToolPurposeFlagBits type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_TOOL_PURPOSE_VALIDATION_BIT";
        case static_cast<uint64_t>(2): return "VK_TOOL_PURPOSE_PROFILING_BIT";
        case static_cast<uint64_t>(4): return "VK_TOOL_PURPOSE_TRACING_BIT";
        case static_cast<uint64_t>(8): return "VK_TOOL_PURPOSE_ADDITIONAL_FEATURES_BIT";
        case static_cast<uint64_t>(16): return "VK_TOOL_PURPOSE_MODIFYING_FEATURES_BIT";
        case static_cast<uint64_t>(2147483647): return "VK_TOOL_PURPOSE_FLAG_BITS_MAX_ENUM";
        case static_cast<uint64_t>(32): return "VK_TOOL_PURPOSE_DEBUG_REPORTING_BIT_EXT";
        case static_cast<uint64_t>(64): return "VK_TOOL_PURPOSE_DEBUG_MARKERS_BIT_EXT";
        default: return "VkToolPurposeFlagBits_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkValidationCacheHeaderVersionEXT>(VkValidationCacheHeaderVersionEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_VALIDATION_CACHE_HEADER_VERSION_ONE_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_VALIDATION_CACHE_HEADER_VERSION_EXT_MAX_ENUM";
        default: return "VkValidationCacheHeaderVersionEXT_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkValidationCheckEXT>(VkValidationCheckEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_VALIDATION_CHECK_ALL_EXT";
        case static_cast<uint64_t>(1): return "VK_VALIDATION_CHECK_SHADERS_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_VALIDATION_CHECK_EXT_MAX_ENUM";
        default: return "VkValidationCheckEXT_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkValidationFeatureDisableEXT>(VkValidationFeatureDisableEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_VALIDATION_FEATURE_DISABLE_ALL_EXT";
        case static_cast<uint64_t>(1): return "VK_VALIDATION_FEATURE_DISABLE_SHADERS_EXT";
        case static_cast<uint64_t>(2): return "VK_VALIDATION_FEATURE_DISABLE_THREAD_SAFETY_EXT";
        case static_cast<uint64_t>(3): return "VK_VALIDATION_FEATURE_DISABLE_API_PARAMETERS_EXT";
        case static_cast<uint64_t>(4): return "VK_VALIDATION_FEATURE_DISABLE_OBJECT_LIFETIMES_EXT";
        case static_cast<uint64_t>(5): return "VK_VALIDATION_FEATURE_DISABLE_CORE_CHECKS_EXT";
        case static_cast<uint64_t>(6): return "VK_VALIDATION_FEATURE_DISABLE_UNIQUE_HANDLES_EXT";
        case static_cast<uint64_t>(7): return "VK_VALIDATION_FEATURE_DISABLE_SHADER_VALIDATION_CACHE_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_VALIDATION_FEATURE_DISABLE_EXT_MAX_ENUM";
        default: return "VkValidationFeatureDisableEXT_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkValidationFeatureEnableEXT>(VkValidationFeatureEnableEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT";
        case static_cast<uint64_t>(1): return "VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT";
        case static_cast<uint64_t>(2): return "VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT";
        case static_cast<uint64_t>(3): return "VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT";
        case static_cast<uint64_t>(4): return "VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_VALIDATION_FEATURE_ENABLE_EXT_MAX_ENUM";
        default: return "VkValidationFeatureEnableEXT_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkVendorId>(VkVendorId type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(65537): return "VK_VENDOR_ID_VIV";
        case static_cast<uint64_t>(65538): return "VK_VENDOR_ID_VSI";
        case static_cast<uint64_t>(65539): return "VK_VENDOR_ID_KAZAN";
        case static_cast<uint64_t>(65540): return "VK_VENDOR_ID_CODEPLAY";
        case static_cast<uint64_t>(65541): return "VK_VENDOR_ID_MESA";
        case static_cast<uint64_t>(65542): return "VK_VENDOR_ID_POCL";
        case static_cast<uint64_t>(2147483647): return "VK_VENDOR_ID_MAX_ENUM";
        default: return "VkVendorId_UNKNOWN";
      }
    }

    template <> constexpr const char* enumString<VkVertexInputRate>(VkVertexInputRate type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_VERTEX_INPUT_RATE_VERTEX";
        case static_cast<uint64_t>(1): return "VK_VERTEX_INPUT_RATE_INSTANCE";
        case static_cast<uint64_t>(2147483647): return "VK_VERTEX_INPUT_RATE_MAX_ENUM";
        default: return "VkVertexInputRate_UNKNOWN";
      }
    }

#ifdef VK_ENABLE_BETA_EXTENSIONS
    template <> constexpr const char* enumString<VkVideoCapabilityFlagBitsKHR>(VkVideoCapabilityFlagBitsKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_VIDEO_CAPABILITY_PROTECTED_CONTENT_BIT_KHR";
        case static_cast<uint64_t>(2): return "VK_VIDEO_CAPABILITY_SEPARATE_REFERENCE_IMAGES_BIT_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_VIDEO_CAPABILITY_FLAG_BITS_KHR_MAX_ENUM";
        default: return "VkVideoCapabilityFlagBitsKHR_UNKNOWN";
      }
    }
#endif

#ifdef VK_ENABLE_BETA_EXTENSIONS
    template <> constexpr const char* enumString<VkVideoChromaSubsamplingFlagBitsKHR>(VkVideoChromaSubsamplingFlagBitsKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_VIDEO_CHROMA_SUBSAMPLING_INVALID_BIT_KHR";
        case static_cast<uint64_t>(1): return "VK_VIDEO_CHROMA_SUBSAMPLING_MONOCHROME_BIT_KHR";
        case static_cast<uint64_t>(2): return "VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR";
        case static_cast<uint64_t>(4): return "VK_VIDEO_CHROMA_SUBSAMPLING_422_BIT_KHR";
        case static_cast<uint64_t>(8): return "VK_VIDEO_CHROMA_SUBSAMPLING_444_BIT_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_VIDEO_CHROMA_SUBSAMPLING_FLAG_BITS_KHR_MAX_ENUM";
        default: return "VkVideoChromaSubsamplingFlagBitsKHR_UNKNOWN";
      }
    }
#endif

#ifdef VK_ENABLE_BETA_EXTENSIONS
    template <> constexpr const char* enumString<VkVideoCodecOperationFlagBitsKHR>(VkVideoCodecOperationFlagBitsKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_VIDEO_CODEC_OPERATION_INVALID_BIT_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_VIDEO_CODEC_OPERATION_FLAG_BITS_KHR_MAX_ENUM";
        case static_cast<uint64_t>(65536): return "VK_VIDEO_CODEC_OPERATION_ENCODE_H264_BIT_EXT";
        case static_cast<uint64_t>(131072): return "VK_VIDEO_CODEC_OPERATION_ENCODE_H265_BIT_EXT";
        case static_cast<uint64_t>(1): return "VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_EXT";
        case static_cast<uint64_t>(2): return "VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_EXT";
        default: return "VkVideoCodecOperationFlagBitsKHR_UNKNOWN";
      }
    }
#endif

#ifdef VK_ENABLE_BETA_EXTENSIONS
    template <> constexpr const char* enumString<VkVideoCodingControlFlagBitsKHR>(VkVideoCodingControlFlagBitsKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_VIDEO_CODING_CONTROL_DEFAULT_KHR";
        case static_cast<uint64_t>(1): return "VK_VIDEO_CODING_CONTROL_RESET_BIT_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_VIDEO_CODING_CONTROL_FLAG_BITS_KHR_MAX_ENUM";
        default: return "VkVideoCodingControlFlagBitsKHR_UNKNOWN";
      }
    }
#endif

#ifdef VK_ENABLE_BETA_EXTENSIONS
    template <> constexpr const char* enumString<VkVideoCodingQualityPresetFlagBitsKHR>(VkVideoCodingQualityPresetFlagBitsKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_VIDEO_CODING_QUALITY_PRESET_NORMAL_BIT_KHR";
        case static_cast<uint64_t>(2): return "VK_VIDEO_CODING_QUALITY_PRESET_POWER_BIT_KHR";
        case static_cast<uint64_t>(4): return "VK_VIDEO_CODING_QUALITY_PRESET_QUALITY_BIT_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_VIDEO_CODING_QUALITY_PRESET_FLAG_BITS_KHR_MAX_ENUM";
        default: return "VkVideoCodingQualityPresetFlagBitsKHR_UNKNOWN";
      }
    }
#endif

#ifdef VK_ENABLE_BETA_EXTENSIONS
    template <> constexpr const char* enumString<VkVideoComponentBitDepthFlagBitsKHR>(VkVideoComponentBitDepthFlagBitsKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_VIDEO_COMPONENT_BIT_DEPTH_INVALID_KHR";
        case static_cast<uint64_t>(1): return "VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR";
        case static_cast<uint64_t>(4): return "VK_VIDEO_COMPONENT_BIT_DEPTH_10_BIT_KHR";
        case static_cast<uint64_t>(16): return "VK_VIDEO_COMPONENT_BIT_DEPTH_12_BIT_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_VIDEO_COMPONENT_BIT_DEPTH_FLAG_BITS_KHR_MAX_ENUM";
        default: return "VkVideoComponentBitDepthFlagBitsKHR_UNKNOWN";
      }
    }
#endif

#ifdef VK_ENABLE_BETA_EXTENSIONS
    template <> constexpr const char* enumString<VkVideoDecodeCapabilityFlagBitsKHR>(VkVideoDecodeCapabilityFlagBitsKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_VIDEO_DECODE_CAPABILITY_DEFAULT_KHR";
        case static_cast<uint64_t>(1): return "VK_VIDEO_DECODE_CAPABILITY_DPB_AND_OUTPUT_COINCIDE_BIT_KHR";
        case static_cast<uint64_t>(2): return "VK_VIDEO_DECODE_CAPABILITY_DPB_AND_OUTPUT_DISTINCT_BIT_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_VIDEO_DECODE_CAPABILITY_FLAG_BITS_KHR_MAX_ENUM";
        default: return "VkVideoDecodeCapabilityFlagBitsKHR_UNKNOWN";
      }
    }
#endif

#ifdef VK_ENABLE_BETA_EXTENSIONS
    template <> constexpr const char* enumString<VkVideoDecodeFlagBitsKHR>(VkVideoDecodeFlagBitsKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_VIDEO_DECODE_DEFAULT_KHR";
        case static_cast<uint64_t>(1): return "VK_VIDEO_DECODE_RESERVED_0_BIT_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_VIDEO_DECODE_FLAG_BITS_KHR_MAX_ENUM";
        default: return "VkVideoDecodeFlagBitsKHR_UNKNOWN";
      }
    }
#endif

#ifdef VK_ENABLE_BETA_EXTENSIONS
    template <> constexpr const char* enumString<VkVideoDecodeH264PictureLayoutFlagBitsEXT>(VkVideoDecodeH264PictureLayoutFlagBitsEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_VIDEO_DECODE_H264_PICTURE_LAYOUT_PROGRESSIVE_EXT";
        case static_cast<uint64_t>(1): return "VK_VIDEO_DECODE_H264_PICTURE_LAYOUT_INTERLACED_INTERLEAVED_LINES_BIT_EXT";
        case static_cast<uint64_t>(2): return "VK_VIDEO_DECODE_H264_PICTURE_LAYOUT_INTERLACED_SEPARATE_PLANES_BIT_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_VIDEO_DECODE_H2_64_PICTURE_LAYOUT_FLAG_BITS_EXT_MAX_ENUM";
        default: return "VkVideoDecodeH264PictureLayoutFlagBitsEXT_UNKNOWN";
      }
    }
#endif

#ifdef VK_ENABLE_BETA_EXTENSIONS
    template <> constexpr const char* enumString<VkVideoEncodeCapabilityFlagBitsKHR>(VkVideoEncodeCapabilityFlagBitsKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_VIDEO_ENCODE_CAPABILITY_DEFAULT_KHR";
        case static_cast<uint64_t>(1): return "VK_VIDEO_ENCODE_CAPABILITY_PRECEDING_EXTERNALLY_ENCODED_BYTES_BIT_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_VIDEO_ENCODE_CAPABILITY_FLAG_BITS_KHR_MAX_ENUM";
        default: return "VkVideoEncodeCapabilityFlagBitsKHR_UNKNOWN";
      }
    }
#endif

#ifdef VK_ENABLE_BETA_EXTENSIONS
    template <> constexpr const char* enumString<VkVideoEncodeFlagBitsKHR>(VkVideoEncodeFlagBitsKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_VIDEO_ENCODE_DEFAULT_KHR";
        case static_cast<uint64_t>(1): return "VK_VIDEO_ENCODE_RESERVED_0_BIT_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_VIDEO_ENCODE_FLAG_BITS_KHR_MAX_ENUM";
        default: return "VkVideoEncodeFlagBitsKHR_UNKNOWN";
      }
    }
#endif

#ifdef VK_ENABLE_BETA_EXTENSIONS
    template <> constexpr const char* enumString<VkVideoEncodeH264CapabilityFlagBitsEXT>(VkVideoEncodeH264CapabilityFlagBitsEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_VIDEO_ENCODE_H264_CAPABILITY_DIRECT_8X8_INFERENCE_ENABLED_BIT_EXT";
        case static_cast<uint64_t>(2): return "VK_VIDEO_ENCODE_H264_CAPABILITY_DIRECT_8X8_INFERENCE_DISABLED_BIT_EXT";
        case static_cast<uint64_t>(4): return "VK_VIDEO_ENCODE_H264_CAPABILITY_SEPARATE_COLOUR_PLANE_BIT_EXT";
        case static_cast<uint64_t>(8): return "VK_VIDEO_ENCODE_H264_CAPABILITY_QPPRIME_Y_ZERO_TRANSFORM_BYPASS_BIT_EXT";
        case static_cast<uint64_t>(16): return "VK_VIDEO_ENCODE_H264_CAPABILITY_SCALING_LISTS_BIT_EXT";
        case static_cast<uint64_t>(32): return "VK_VIDEO_ENCODE_H264_CAPABILITY_HRD_COMPLIANCE_BIT_EXT";
        case static_cast<uint64_t>(64): return "VK_VIDEO_ENCODE_H264_CAPABILITY_CHROMA_QP_OFFSET_BIT_EXT";
        case static_cast<uint64_t>(128): return "VK_VIDEO_ENCODE_H264_CAPABILITY_SECOND_CHROMA_QP_OFFSET_BIT_EXT";
        case static_cast<uint64_t>(256): return "VK_VIDEO_ENCODE_H264_CAPABILITY_PIC_INIT_QP_MINUS26_BIT_EXT";
        case static_cast<uint64_t>(512): return "VK_VIDEO_ENCODE_H264_CAPABILITY_WEIGHTED_PRED_BIT_EXT";
        case static_cast<uint64_t>(1024): return "VK_VIDEO_ENCODE_H264_CAPABILITY_WEIGHTED_BIPRED_EXPLICIT_BIT_EXT";
        case static_cast<uint64_t>(2048): return "VK_VIDEO_ENCODE_H264_CAPABILITY_WEIGHTED_BIPRED_IMPLICIT_BIT_EXT";
        case static_cast<uint64_t>(4096): return "VK_VIDEO_ENCODE_H264_CAPABILITY_WEIGHTED_PRED_NO_TABLE_BIT_EXT";
        case static_cast<uint64_t>(8192): return "VK_VIDEO_ENCODE_H264_CAPABILITY_TRANSFORM_8X8_BIT_EXT";
        case static_cast<uint64_t>(16384): return "VK_VIDEO_ENCODE_H264_CAPABILITY_CABAC_BIT_EXT";
        case static_cast<uint64_t>(32768): return "VK_VIDEO_ENCODE_H264_CAPABILITY_CAVLC_BIT_EXT";
        case static_cast<uint64_t>(65536): return "VK_VIDEO_ENCODE_H264_CAPABILITY_DEBLOCKING_FILTER_DISABLED_BIT_EXT";
        case static_cast<uint64_t>(131072): return "VK_VIDEO_ENCODE_H264_CAPABILITY_DEBLOCKING_FILTER_ENABLED_BIT_EXT";
        case static_cast<uint64_t>(262144): return "VK_VIDEO_ENCODE_H264_CAPABILITY_DEBLOCKING_FILTER_PARTIAL_BIT_EXT";
        case static_cast<uint64_t>(524288): return "VK_VIDEO_ENCODE_H264_CAPABILITY_DISABLE_DIRECT_SPATIAL_MV_PRED_BIT_EXT";
        case static_cast<uint64_t>(1048576): return "VK_VIDEO_ENCODE_H264_CAPABILITY_MULTIPLE_SLICE_PER_FRAME_BIT_EXT";
        case static_cast<uint64_t>(2097152): return "VK_VIDEO_ENCODE_H264_CAPABILITY_SLICE_MB_COUNT_BIT_EXT";
        case static_cast<uint64_t>(4194304): return "VK_VIDEO_ENCODE_H264_CAPABILITY_ROW_UNALIGNED_SLICE_BIT_EXT";
        case static_cast<uint64_t>(8388608): return "VK_VIDEO_ENCODE_H264_CAPABILITY_DIFFERENT_SLICE_TYPE_BIT_EXT";
        case static_cast<uint64_t>(16777216): return "VK_VIDEO_ENCODE_H264_CAPABILITY_B_FRAME_IN_L1_LIST_BIT_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_VIDEO_ENCODE_H2_64_CAPABILITY_FLAG_BITS_EXT_MAX_ENUM";
        default: return "VkVideoEncodeH264CapabilityFlagBitsEXT_UNKNOWN";
      }
    }
#endif

#ifdef VK_ENABLE_BETA_EXTENSIONS
    template <> constexpr const char* enumString<VkVideoEncodeH264InputModeFlagBitsEXT>(VkVideoEncodeH264InputModeFlagBitsEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_VIDEO_ENCODE_H264_INPUT_MODE_FRAME_BIT_EXT";
        case static_cast<uint64_t>(2): return "VK_VIDEO_ENCODE_H264_INPUT_MODE_SLICE_BIT_EXT";
        case static_cast<uint64_t>(4): return "VK_VIDEO_ENCODE_H264_INPUT_MODE_NON_VCL_BIT_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_VIDEO_ENCODE_H2_64_INPUT_MODE_FLAG_BITS_EXT_MAX_ENUM";
        default: return "VkVideoEncodeH264InputModeFlagBitsEXT_UNKNOWN";
      }
    }
#endif

#ifdef VK_ENABLE_BETA_EXTENSIONS
    template <> constexpr const char* enumString<VkVideoEncodeH264OutputModeFlagBitsEXT>(VkVideoEncodeH264OutputModeFlagBitsEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_VIDEO_ENCODE_H264_OUTPUT_MODE_FRAME_BIT_EXT";
        case static_cast<uint64_t>(2): return "VK_VIDEO_ENCODE_H264_OUTPUT_MODE_SLICE_BIT_EXT";
        case static_cast<uint64_t>(4): return "VK_VIDEO_ENCODE_H264_OUTPUT_MODE_NON_VCL_BIT_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_VIDEO_ENCODE_H2_64_OUTPUT_MODE_FLAG_BITS_EXT_MAX_ENUM";
        default: return "VkVideoEncodeH264OutputModeFlagBitsEXT_UNKNOWN";
      }
    }
#endif

#ifdef VK_ENABLE_BETA_EXTENSIONS
    template <> constexpr const char* enumString<VkVideoEncodeH264RateControlStructureFlagBitsEXT>(VkVideoEncodeH264RateControlStructureFlagBitsEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_VIDEO_ENCODE_H264_RATE_CONTROL_STRUCTURE_UNKNOWN_EXT";
        case static_cast<uint64_t>(1): return "VK_VIDEO_ENCODE_H264_RATE_CONTROL_STRUCTURE_FLAT_BIT_EXT";
        case static_cast<uint64_t>(2): return "VK_VIDEO_ENCODE_H264_RATE_CONTROL_STRUCTURE_DYADIC_BIT_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_VIDEO_ENCODE_H2_64_RATE_CONTROL_STRUCTURE_FLAG_BITS_EXT_MAX_ENUM";
        default: return "VkVideoEncodeH264RateControlStructureFlagBitsEXT_UNKNOWN";
      }
    }
#endif

#ifdef VK_ENABLE_BETA_EXTENSIONS
    template <> constexpr const char* enumString<VkVideoEncodeH265CapabilityFlagBitsEXT>(VkVideoEncodeH265CapabilityFlagBitsEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_VIDEO_ENCODE_H265_CAPABILITY_SEPARATE_COLOUR_PLANE_BIT_EXT";
        case static_cast<uint64_t>(2): return "VK_VIDEO_ENCODE_H265_CAPABILITY_SCALING_LISTS_BIT_EXT";
        case static_cast<uint64_t>(4): return "VK_VIDEO_ENCODE_H265_CAPABILITY_SAMPLE_ADAPTIVE_OFFSET_ENABLED_BIT_EXT";
        case static_cast<uint64_t>(8): return "VK_VIDEO_ENCODE_H265_CAPABILITY_PCM_ENABLE_BIT_EXT";
        case static_cast<uint64_t>(16): return "VK_VIDEO_ENCODE_H265_CAPABILITY_SPS_TEMPORAL_MVP_ENABLED_BIT_EXT";
        case static_cast<uint64_t>(32): return "VK_VIDEO_ENCODE_H265_CAPABILITY_HRD_COMPLIANCE_BIT_EXT";
        case static_cast<uint64_t>(64): return "VK_VIDEO_ENCODE_H265_CAPABILITY_INIT_QP_MINUS26_BIT_EXT";
        case static_cast<uint64_t>(128): return "VK_VIDEO_ENCODE_H265_CAPABILITY_LOG2_PARALLEL_MERGE_LEVEL_MINUS2_BIT_EXT";
        case static_cast<uint64_t>(256): return "VK_VIDEO_ENCODE_H265_CAPABILITY_SIGN_DATA_HIDING_ENABLED_BIT_EXT";
        case static_cast<uint64_t>(512): return "VK_VIDEO_ENCODE_H265_CAPABILITY_TRANSFORM_SKIP_ENABLED_BIT_EXT";
        case static_cast<uint64_t>(1024): return "VK_VIDEO_ENCODE_H265_CAPABILITY_TRANSFORM_SKIP_DISABLED_BIT_EXT";
        case static_cast<uint64_t>(2048): return "VK_VIDEO_ENCODE_H265_CAPABILITY_PPS_SLICE_CHROMA_QP_OFFSETS_PRESENT_BIT_EXT";
        case static_cast<uint64_t>(4096): return "VK_VIDEO_ENCODE_H265_CAPABILITY_WEIGHTED_PRED_BIT_EXT";
        case static_cast<uint64_t>(8192): return "VK_VIDEO_ENCODE_H265_CAPABILITY_WEIGHTED_BIPRED_BIT_EXT";
        case static_cast<uint64_t>(16384): return "VK_VIDEO_ENCODE_H265_CAPABILITY_WEIGHTED_PRED_NO_TABLE_BIT_EXT";
        case static_cast<uint64_t>(32768): return "VK_VIDEO_ENCODE_H265_CAPABILITY_TRANSQUANT_BYPASS_ENABLED_BIT_EXT";
        case static_cast<uint64_t>(65536): return "VK_VIDEO_ENCODE_H265_CAPABILITY_ENTROPY_CODING_SYNC_ENABLED_BIT_EXT";
        case static_cast<uint64_t>(131072): return "VK_VIDEO_ENCODE_H265_CAPABILITY_DEBLOCKING_FILTER_OVERRIDE_ENABLED_BIT_EXT";
        case static_cast<uint64_t>(262144): return "VK_VIDEO_ENCODE_H265_CAPABILITY_MULTIPLE_TILE_PER_FRAME_BIT_EXT";
        case static_cast<uint64_t>(524288): return "VK_VIDEO_ENCODE_H265_CAPABILITY_MULTIPLE_SLICE_PER_TILE_BIT_EXT";
        case static_cast<uint64_t>(1048576): return "VK_VIDEO_ENCODE_H265_CAPABILITY_MULTIPLE_TILE_PER_SLICE_BIT_EXT";
        case static_cast<uint64_t>(2097152): return "VK_VIDEO_ENCODE_H265_CAPABILITY_SLICE_SEGMENT_CTB_COUNT_BIT_EXT";
        case static_cast<uint64_t>(4194304): return "VK_VIDEO_ENCODE_H265_CAPABILITY_ROW_UNALIGNED_SLICE_SEGMENT_BIT_EXT";
        case static_cast<uint64_t>(8388608): return "VK_VIDEO_ENCODE_H265_CAPABILITY_DEPENDENT_SLICE_SEGMENT_BIT_EXT";
        case static_cast<uint64_t>(16777216): return "VK_VIDEO_ENCODE_H265_CAPABILITY_DIFFERENT_SLICE_TYPE_BIT_EXT";
        case static_cast<uint64_t>(33554432): return "VK_VIDEO_ENCODE_H265_CAPABILITY_B_FRAME_IN_L1_LIST_BIT_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_VIDEO_ENCODE_H2_65_CAPABILITY_FLAG_BITS_EXT_MAX_ENUM";
        default: return "VkVideoEncodeH265CapabilityFlagBitsEXT_UNKNOWN";
      }
    }
#endif

#ifdef VK_ENABLE_BETA_EXTENSIONS
    template <> constexpr const char* enumString<VkVideoEncodeH265CtbSizeFlagBitsEXT>(VkVideoEncodeH265CtbSizeFlagBitsEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_VIDEO_ENCODE_H265_CTB_SIZE_16_BIT_EXT";
        case static_cast<uint64_t>(2): return "VK_VIDEO_ENCODE_H265_CTB_SIZE_32_BIT_EXT";
        case static_cast<uint64_t>(4): return "VK_VIDEO_ENCODE_H265_CTB_SIZE_64_BIT_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_VIDEO_ENCODE_H2_65_CTB_SIZE_FLAG_BITS_EXT_MAX_ENUM";
        default: return "VkVideoEncodeH265CtbSizeFlagBitsEXT_UNKNOWN";
      }
    }
#endif

#ifdef VK_ENABLE_BETA_EXTENSIONS
    template <> constexpr const char* enumString<VkVideoEncodeH265InputModeFlagBitsEXT>(VkVideoEncodeH265InputModeFlagBitsEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_VIDEO_ENCODE_H265_INPUT_MODE_FRAME_BIT_EXT";
        case static_cast<uint64_t>(2): return "VK_VIDEO_ENCODE_H265_INPUT_MODE_SLICE_SEGMENT_BIT_EXT";
        case static_cast<uint64_t>(4): return "VK_VIDEO_ENCODE_H265_INPUT_MODE_NON_VCL_BIT_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_VIDEO_ENCODE_H2_65_INPUT_MODE_FLAG_BITS_EXT_MAX_ENUM";
        default: return "VkVideoEncodeH265InputModeFlagBitsEXT_UNKNOWN";
      }
    }
#endif

#ifdef VK_ENABLE_BETA_EXTENSIONS
    template <> constexpr const char* enumString<VkVideoEncodeH265OutputModeFlagBitsEXT>(VkVideoEncodeH265OutputModeFlagBitsEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_VIDEO_ENCODE_H265_OUTPUT_MODE_FRAME_BIT_EXT";
        case static_cast<uint64_t>(2): return "VK_VIDEO_ENCODE_H265_OUTPUT_MODE_SLICE_SEGMENT_BIT_EXT";
        case static_cast<uint64_t>(4): return "VK_VIDEO_ENCODE_H265_OUTPUT_MODE_NON_VCL_BIT_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_VIDEO_ENCODE_H2_65_OUTPUT_MODE_FLAG_BITS_EXT_MAX_ENUM";
        default: return "VkVideoEncodeH265OutputModeFlagBitsEXT_UNKNOWN";
      }
    }
#endif

#ifdef VK_ENABLE_BETA_EXTENSIONS
    template <> constexpr const char* enumString<VkVideoEncodeH265RateControlStructureFlagBitsEXT>(VkVideoEncodeH265RateControlStructureFlagBitsEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_VIDEO_ENCODE_H265_RATE_CONTROL_STRUCTURE_UNKNOWN_EXT";
        case static_cast<uint64_t>(1): return "VK_VIDEO_ENCODE_H265_RATE_CONTROL_STRUCTURE_FLAT_BIT_EXT";
        case static_cast<uint64_t>(2): return "VK_VIDEO_ENCODE_H265_RATE_CONTROL_STRUCTURE_DYADIC_BIT_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_VIDEO_ENCODE_H2_65_RATE_CONTROL_STRUCTURE_FLAG_BITS_EXT_MAX_ENUM";
        default: return "VkVideoEncodeH265RateControlStructureFlagBitsEXT_UNKNOWN";
      }
    }
#endif

#ifdef VK_ENABLE_BETA_EXTENSIONS
    template <> constexpr const char* enumString<VkVideoEncodeH265TransformBlockSizeFlagBitsEXT>(VkVideoEncodeH265TransformBlockSizeFlagBitsEXT type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(1): return "VK_VIDEO_ENCODE_H265_TRANSFORM_BLOCK_SIZE_4_BIT_EXT";
        case static_cast<uint64_t>(2): return "VK_VIDEO_ENCODE_H265_TRANSFORM_BLOCK_SIZE_8_BIT_EXT";
        case static_cast<uint64_t>(4): return "VK_VIDEO_ENCODE_H265_TRANSFORM_BLOCK_SIZE_16_BIT_EXT";
        case static_cast<uint64_t>(8): return "VK_VIDEO_ENCODE_H265_TRANSFORM_BLOCK_SIZE_32_BIT_EXT";
        case static_cast<uint64_t>(2147483647): return "VK_VIDEO_ENCODE_H2_65_TRANSFORM_BLOCK_SIZE_FLAG_BITS_EXT_MAX_ENUM";
        default: return "VkVideoEncodeH265TransformBlockSizeFlagBitsEXT_UNKNOWN";
      }
    }
#endif

#ifdef VK_ENABLE_BETA_EXTENSIONS
    template <> constexpr const char* enumString<VkVideoEncodeRateControlFlagBitsKHR>(VkVideoEncodeRateControlFlagBitsKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_VIDEO_ENCODE_RATE_CONTROL_DEFAULT_KHR";
        case static_cast<uint64_t>(1): return "VK_VIDEO_ENCODE_RATE_CONTROL_RESERVED_0_BIT_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_VIDEO_ENCODE_RATE_CONTROL_FLAG_BITS_KHR_MAX_ENUM";
        default: return "VkVideoEncodeRateControlFlagBitsKHR_UNKNOWN";
      }
    }
#endif

#ifdef VK_ENABLE_BETA_EXTENSIONS
    template <> constexpr const char* enumString<VkVideoEncodeRateControlModeFlagBitsKHR>(VkVideoEncodeRateControlModeFlagBitsKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_VIDEO_ENCODE_RATE_CONTROL_MODE_NONE_BIT_KHR";
        case static_cast<uint64_t>(1): return "VK_VIDEO_ENCODE_RATE_CONTROL_MODE_CBR_BIT_KHR";
        case static_cast<uint64_t>(2): return "VK_VIDEO_ENCODE_RATE_CONTROL_MODE_VBR_BIT_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_VIDEO_ENCODE_RATE_CONTROL_MODE_FLAG_BITS_KHR_MAX_ENUM";
        default: return "VkVideoEncodeRateControlModeFlagBitsKHR_UNKNOWN";
      }
    }
#endif

#ifdef VK_ENABLE_BETA_EXTENSIONS
    template <> constexpr const char* enumString<VkVideoSessionCreateFlagBitsKHR>(VkVideoSessionCreateFlagBitsKHR type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_VIDEO_SESSION_CREATE_DEFAULT_KHR";
        case static_cast<uint64_t>(1): return "VK_VIDEO_SESSION_CREATE_PROTECTED_CONTENT_BIT_KHR";
        case static_cast<uint64_t>(2147483647): return "VK_VIDEO_SESSION_CREATE_FLAG_BITS_KHR_MAX_ENUM";
        default: return "VkVideoSessionCreateFlagBitsKHR_UNKNOWN";
      }
    }
#endif

    template <> constexpr const char* enumString<VkViewportCoordinateSwizzleNV>(VkViewportCoordinateSwizzleNV type) {
      switch(static_cast<uint64_t>(type)) {
        case static_cast<uint64_t>(0): return "VK_VIEWPORT_COORDINATE_SWIZZLE_POSITIVE_X_NV";
        case static_cast<uint64_t>(1): return "VK_VIEWPORT_COORDINATE_SWIZZLE_NEGATIVE_X_NV";
        case static_cast<uint64_t>(2): return "VK_VIEWPORT_COORDINATE_SWIZZLE_POSITIVE_Y_NV";
        case static_cast<uint64_t>(3): return "VK_VIEWPORT_COORDINATE_SWIZZLE_NEGATIVE_Y_NV";
        case static_cast<uint64_t>(4): return "VK_VIEWPORT_COORDINATE_SWIZZLE_POSITIVE_Z_NV";
        case static_cast<uint64_t>(5): return "VK_VIEWPORT_COORDINATE_SWIZZLE_NEGATIVE_Z_NV";
        case static_cast<uint64_t>(6): return "VK_VIEWPORT_COORDINATE_SWIZZLE_POSITIVE_W_NV";
        case static_cast<uint64_t>(7): return "VK_VIEWPORT_COORDINATE_SWIZZLE_NEGATIVE_W_NV";
        case static_cast<uint64_t>(2147483647): return "VK_VIEWPORT_COORDINATE_SWIZZLE_NV_MAX_ENUM";
        default: return "VkViewportCoordinateSwizzleNV_UNKNOWN";
      }
    }
  }
}

namespace vkroots::tables {

  static inline void CreateDispatchTable(PFN_vkGetInstanceProcAddr nextInstanceProcAddr, PFN_GetPhysicalDeviceProcAddr nextPhysDevProcAddr, VkInstance instance) {
    auto instanceDispatch = InstanceDispatches.insert(instance, std::make_unique<VkInstanceDispatch>(nextInstanceProcAddr, instance));
    auto physicalDeviceDispatch = PhysicalDeviceInstanceDispatches.insert(instance, std::make_unique<VkPhysicalDeviceDispatch>(nextPhysDevProcAddr, instance, instanceDispatch));

    uint32_t physicalDeviceCount;
    VkResult res = instanceDispatch->EnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
    assert(res == VK_SUCCESS); // Not like we can do anything else with the result lol.
    if (res != VK_SUCCESS) return;
    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    res = instanceDispatch->EnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());
    assert(res == VK_SUCCESS); // Not like we can do anything else with the result lol.
    if (res != VK_SUCCESS) return;

    for (VkPhysicalDevice physicalDevice : physicalDevices)
      PhysicalDeviceDispatches.insert(physicalDevice, RawPointer(physicalDeviceDispatch));
  }

  static inline void CreateDispatchTable(const VkDeviceCreateInfo* pCreateInfo, PFN_vkGetDeviceProcAddr nextProcAddr, VkPhysicalDevice physicalDevice, VkDevice device) {
    auto physicalDeviceDispatch = vkroots::tables::LookupPhysicalDeviceDispatch(physicalDevice);
    auto deviceDispatch = DeviceDispatches.insert(device, std::make_unique<VkDeviceDispatch>(nextProcAddr, device, physicalDevice, physicalDeviceDispatch, pCreateInfo));

    for (uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; i++) {
      const auto &queueInfo = pCreateInfo->pQueueCreateInfos[i];
      for (uint32_t j = 0; j < queueInfo.queueCount; j++) {
        VkQueue queue;
        deviceDispatch->GetDeviceQueue(device, queueInfo.queueFamilyIndex, j, &queue);

        QueueDispatches.insert(queue, RawPointer(deviceDispatch));
      }
    }
  }

  static inline void DestroyDispatchTable(VkInstance instance) {
    const VkInstanceDispatch* instanceDispatch = InstanceDispatches.find(instance);
    assert(instanceDispatch);
    if (!instanceDispatch)
      return;

    uint32_t physicalDeviceCount;
    VkResult res = instanceDispatch->EnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
    assert(res == VK_SUCCESS); // Not like we can do anything else with the result lol.
    if (res == VK_SUCCESS) {
      std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
      res = instanceDispatch->EnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());
      assert(res == VK_SUCCESS); // Not like we can do anything else with the result lol.
      if (res == VK_SUCCESS) {
        for (VkPhysicalDevice physicalDevice : physicalDevices)
          PhysicalDeviceDispatches.remove(physicalDevice);
      }
    }

    PhysicalDeviceInstanceDispatches.remove(instance);
    InstanceDispatches.remove(instance);
  }

  static inline void DestroyDispatchTable(VkDevice device) {
    const VkDeviceDispatch* deviceDispatch = DeviceDispatches.find(device);
    assert(deviceDispatch);
    if (!deviceDispatch)
      return;

    for (const auto& queueInfo : deviceDispatch->DeviceQueueInfos) {
      for (uint32_t i = 0; i < queueInfo.queueCount; i++) {
        VkQueue queue;
        deviceDispatch->GetDeviceQueue(device, queueInfo.queueFamilyIndex, i, &queue);
        QueueDispatches.remove(queue);
      }
    }

    DeviceDispatches.remove(device);
  }

}


namespace vkroots::helpers {

  template <typename Func>
  inline void delimitStringView(std::string_view view, std::string_view delim, Func func) {
    size_t pos = 0;
    while ((pos = view.find(delim)) != std::string_view::npos) {
      std::string_view token = view.substr(0, pos);
      if (!func(token))
        return;
      view = view.substr(pos + 1);
    }
    func(view);
  }

  template <typename T, typename ArrType, typename Op>
  inline VkResult array(ArrType& arr, uint32_t *pCount, T* pOut, Op func) {
    const uint32_t count = uint32_t(arr.size());

    if (!pOut) {
      *pCount = count;
      return VK_SUCCESS;
    }

    const uint32_t outCount = std::min(*pCount, count);
    for (uint32_t i = 0; i < outCount; i++)
      func(pOut[i], arr[i]);

    *pCount = outCount;
    return count != outCount
      ? VK_INCOMPLETE
      : VK_SUCCESS;
  }

  template <typename T, typename ArrType>
  inline VkResult array(ArrType& arr, uint32_t *pCount, T* pOut) {
    return array(arr, pCount, pOut, [](T& x, const T& y) { x = y; });
  }

  template <typename Func, typename OutArray, typename... Args>
  uint32_t enumerate(Func function, OutArray& outArray, Args&&... arguments) {
    uint32_t count = 0;
    function(arguments..., &count, nullptr);

    outArray.resize(count);
    if (!count)
        return 0;

    function(std::forward<Args>(arguments)..., &count, outArray.data());
    return count;
  }

  template <typename Func, typename InArray, typename OutType, typename... Args>
  VkResult append(Func function, const InArray& inArray, uint32_t* pOutCount, OutType* pOut, Args&&... arguments) {
    uint32_t baseCount = 0;
    function(std::forward<Args>(arguments)..., &baseCount, nullptr);

    const uint32_t totalCount = baseCount + uint32_t(inArray.size());
    if (!pOut) {
      *pOutCount = totalCount;
      return VK_SUCCESS;
    }

    if (*pOutCount < totalCount) {
      function(std::forward<Args>(arguments)..., pOutCount, pOut);
      return VK_INCOMPLETE;
    }

    function(std::forward<Args>(arguments)..., &baseCount, pOut);
    for (size_t i = 0; i < inArray.size(); i++)
      pOut[baseCount + i] = inArray[i];
    return VK_SUCCESS;
  }

  template <typename SearchType, VkStructureType StructureTypeEnum, typename ChainBaseType>
  SearchType *chain(ChainBaseType* pNext) {
    for (VkBaseOutStructure* pBaseOut = reinterpret_cast<VkBaseOutStructure*>(pNext); pBaseOut; pBaseOut = pBaseOut->pNext) {
      if (pBaseOut->sType == StructureTypeEnum)
        return reinterpret_cast<SearchType*>(pBaseOut);
    }
    return nullptr;
  }

  template <typename Key, typename Data>
  class SynchronizedMapObject {
  public:
    using MapKey = Key;
    using MapData = Data;

    static SynchronizedMapObject get(const Key& key) {
      std::unique_lock lock{ s_mutex };
      auto iter = s_map.find(key);
      if (iter == s_map.end())
        return SynchronizedMapObject{ nullptr };
      return SynchronizedMapObject{ iter->second, std::move(lock) };
    }

    static SynchronizedMapObject create(const Key& key, Data data) {
      std::unique_lock lock{ s_mutex };
      auto val = s_map.insert(std::make_pair(key, std::move(data)));
      return SynchronizedMapObject{ val.first->second, std::move(lock) };
    }

    static bool remove(const Key& key) {
      std::unique_lock lock{ s_mutex };
      auto iter = s_map.find(key);
      if (iter == s_map.end())
        return false;
      s_map.erase(iter);
      return true;
    }

    Data* get() {
      return m_data;
    }

    const Data* get() const {
      return m_data;
    }

    Data* operator->() {
      return get();
    }

    const Data* operator->() const {
      return get();
    }

    bool has() const {
      return m_data != nullptr;
    }

    operator bool() const {
      return has();
    }

    void clear() {
      m_data = nullptr;
      m_lock = {};
    }

    SynchronizedMapObject(SynchronizedMapObject&& other)
      : m_data{ other.m_data }, m_lock{ std::move(other.m_lock) } {
    }

  private:
    SynchronizedMapObject(std::nullptr_t)
        : m_data{ nullptr }, m_lock{} {}

    SynchronizedMapObject(Data& data, std::unique_lock<std::mutex> lock) noexcept
        : m_data{ &data }, m_lock{ std::move(lock) } {}

    Data *m_data;
    std::unique_lock<std::mutex> m_lock;

    static std::mutex s_mutex;
    static std::unordered_map<Key, Data> s_map;
  };

#define VKROOTS_DEFINE_SYNCHRONIZED_MAP_TYPE(name, key) \
  using name = ::vkroots::helpers::SynchronizedMapObject<key, name##Data>;

#define VKROOTS_IMPLEMENT_SYNCHRONIZED_MAP_TYPE(x) \
  template <> std::mutex x::s_mutex = {}; \
  template <> std::unordered_map<x::MapKey, x::MapData> x::s_map = {};

}
namespace vkroots {

  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  VkResult NegotiateLoaderLayerInterfaceVersion(VkNegotiateLayerInterface* pVersionStruct) {
    if (pVersionStruct->loaderLayerInterfaceVersion < 2)
      return VK_ERROR_INITIALIZATION_FAILED;
    pVersionStruct->loaderLayerInterfaceVersion = 2;

    // Can't optimize away not having instance overrides from the layer, need to track device creation and instance dispatch and stuff.
    pVersionStruct->pfnGetInstanceProcAddr       = std::is_base_of<NoOverrides, InstanceOverrides>::value && std::is_base_of<NoOverrides, PhysicalDeviceOverrides>::value && std::is_base_of<NoOverrides, DeviceOverrides>::value
                                                     ? nullptr
                                                     : &GetInstanceProcAddr<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    pVersionStruct->pfnGetPhysicalDeviceProcAddr = std::is_base_of<NoOverrides, PhysicalDeviceOverrides>::value && std::is_base_of<NoOverrides, DeviceOverrides>::value
                                                     ? nullptr
                                                     : &GetPhysicalDeviceProcAddr<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;
    pVersionStruct->pfnGetDeviceProcAddr         = std::is_base_of<NoOverrides, DeviceOverrides>::value
                                                     ? nullptr
                                                     : &GetDeviceProcAddr<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>;

    return VK_SUCCESS;
  }

}

// Sadly, can't include this in VKROOTS_DEFINE_LAYER_INTERFACES
// as we need the stupid pragma comment for stdcall aliasing on Win32.
// So you can only have one layer interface per compilation unit.
#ifndef VKROOTS_NEGOTIATION_INTERFACE
#define VKROOTS_NEGOTIATION_INTERFACE vkNegotiateLoaderLayerInterfaceVersion
#endif

#ifdef _WIN32

// Define VK_LAYER_EXPORT to dllexport.
# undef VK_LAYER_EXPORT
# define VK_LAYER_EXPORT extern "C" __declspec(dllexport)

// Fix stdcall aliasing on 32-bit Windows.
# ifndef _WIN64
#  pragma comment(linker, "/EXPORT:" #VKROOTS_NEGOTIATION_INTERFACE "=_" #VKROOTS_NEGOTIATION_INTERFACE "@8")
# endif

#elif defined(__GNUC__)

# undef VK_LAYER_EXPORT
# define VK_LAYER_EXPORT extern "C" __attribute__((visibility("default")))

#endif

#define VKROOTS_DEFINE_LAYER_INTERFACES(InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides)                                   \
  VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL VKROOTS_NEGOTIATION_INTERFACE(VkNegotiateLayerInterface* pVersionStruct) {            \
    return vkroots::NegotiateLoaderLayerInterfaceVersion<InstanceOverrides, PhysicalDeviceOverrides, DeviceOverrides>(pVersionStruct); \
  }
