
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static VkResult implicit_wrap_CreateInstance(
    const VkInstanceCreateInfo*  pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
          VkInstance*            pInstance) {
    VkInstanceProcAddrFuncs instanceProcAddrFuncs = GetProcAddrs(pCreateInfo);
    PFN_vkCreateInstance createInstanceProc = (PFN_vkCreateInstance) instanceProcAddrFuncs.NextGetInstanceProcAddr(nullptr, "vkCreateInstance");
    VkResult ret = createInstanceProc(pCreateInfo, pAllocator, pInstance);
    if (ret == VK_SUCCESS)
      tables::CreateDispatchTable(instanceProcAddrFuncs.NextGetInstanceProcAddr, instanceProcAddrFuncs.NextGetPhysicalDeviceProcAddr, *pInstance);
    return ret;
  }

