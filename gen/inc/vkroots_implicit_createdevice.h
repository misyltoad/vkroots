namespace vkroots {
  
  static VkResult implicit_wrap_CreateDevice(
            VkPhysicalDevice       physicalDevice,
      const VkDeviceCreateInfo*    pCreateInfo,
      const VkAllocationCallbacks* pAllocator,
            VkDevice*              pDevice) {
    const VkPhysicalDeviceDispatch* dispatch = tables::LookupPhysicalDeviceDispatch(physicalDevice);
    PFN_vkGetDeviceProcAddr deviceProcAddr;
    VkResult procAddrRes = GetProcAddrs(pCreateInfo, &deviceProcAddr);
    if (procAddrRes != VK_SUCCESS)
      return procAddrRes;
    VkResult ret = dispatch->pInstanceDispatch->_RealCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
    if (ret == VK_SUCCESS)
      tables::CreateDispatchTable(pCreateInfo, deviceProcAddr, physicalDevice, *pDevice);
    return ret;
  }

}
