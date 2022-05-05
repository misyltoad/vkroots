
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult implicit_wrap_CreateDevice(
            VkPhysicalDevice       physicalDevice,
      const VkDeviceCreateInfo*    pCreateInfo,
      const VkAllocationCallbacks* pAllocator,
            VkDevice*              pDevice) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(physicalDevice);
    VkResult ret = dispatch->CreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
    if (ret == VK_SUCCESS) {
      PFN_vkGetDeviceProcAddr deviceProcAddr = GetProcAddrs(pCreateInfo);
      tables::CreateDispatchTable(pCreateInfo, deviceProcAddr, physicalDevice, *pDevice);
    }
    return ret;
  }

