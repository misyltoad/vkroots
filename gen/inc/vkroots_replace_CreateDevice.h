
  VkResult CreateDevice(
            VkPhysicalDevice       physicalDevice,
      const VkDeviceCreateInfo*    pCreateInfo,
      const VkAllocationCallbacks* pAllocator,
            VkDevice*              pDevice) const {
    PFN_vkGetDeviceProcAddr deviceProcAddr;
    VkResult procAddrRes = GetProcAddrs(pCreateInfo, &deviceProcAddr);
    if (procAddrRes != VK_SUCCESS)
      return procAddrRes;
    VkResult ret = m_CreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
    if (ret == VK_SUCCESS)
      tables::CreateDispatchTable(pCreateInfo, deviceProcAddr, physicalDevice, *pDevice);
    return ret;
  }

