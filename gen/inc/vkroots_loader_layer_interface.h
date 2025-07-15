namespace vkroots {

  template <typename InstanceOverrides, typename DeviceOverrides>
  VkResult NegotiateLoaderLayerInterfaceVersion(VkNegotiateLayerInterface* pVersionStruct) {
    if (pVersionStruct->loaderLayerInterfaceVersion < 2)
      return VK_ERROR_INITIALIZATION_FAILED;
    pVersionStruct->loaderLayerInterfaceVersion = 2;

    // Can't optimize away not having instance overrides from the layer, need to track device creation and instance dispatch and stuff.
    pVersionStruct->pfnGetInstanceProcAddr       = std::is_base_of<NoOverrides, InstanceOverrides>::value && std::is_base_of<NoOverrides, DeviceOverrides>::value
                                                     ? nullptr
                                                     : &GetInstanceProcAddr<InstanceOverrides, DeviceOverrides>;
    pVersionStruct->pfnGetPhysicalDeviceProcAddr = std::is_base_of<NoOverrides, InstanceOverrides>::value && std::is_base_of<NoOverrides, DeviceOverrides>::value
                                                     ? nullptr
                                                     : &GetPhysicalDeviceProcAddr<InstanceOverrides, DeviceOverrides>;
    pVersionStruct->pfnGetDeviceProcAddr         = std::is_base_of<NoOverrides, DeviceOverrides>::value
                                                     ? nullptr
                                                     : &GetDeviceProcAddr<InstanceOverrides, DeviceOverrides>;

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

#define VKROOTS_DEFINE_LAYER_INTERFACES(InstanceOverrides, DeviceOverrides)                                   \
  VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL VKROOTS_NEGOTIATION_INTERFACE(VkNegotiateLayerInterface* pVersionStruct) {            \
    return vkroots::NegotiateLoaderLayerInterfaceVersion<InstanceOverrides, DeviceOverrides>(pVersionStruct); \
  }
