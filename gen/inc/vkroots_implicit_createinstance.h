
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

