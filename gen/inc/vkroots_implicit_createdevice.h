
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

