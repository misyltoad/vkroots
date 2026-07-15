namespace vkroots::tables {

  static inline void CreateDispatchTable(PFN_vkGetInstanceProcAddr nextInstanceProcAddr, PFN_GetPhysicalDeviceProcAddr nextPhysDevProcAddr, VkInstance instance) {
    auto instanceDispatch = InstanceDispatches.create(instance, nextInstanceProcAddr, instance, nextPhysDevProcAddr);

    uint32_t physicalDeviceCount;
    VkResult res = instanceDispatch->EnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
    assert(res == VK_SUCCESS); // Not like we can do anything else with the result lol.
    if (res != VK_SUCCESS) return;
    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    res = instanceDispatch->EnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());
    assert(res == VK_SUCCESS); // Not like we can do anything else with the result lol.
    if (res != VK_SUCCESS) return;

    instanceDispatch->PhysicalDevices.resize(physicalDevices.size());
    instanceDispatch->PhysicalDeviceDispatches.resize(physicalDevices.size());
    for (VkPhysicalDevice physicalDevice : physicalDevices) {
      const vkroots::VkPhysicalDeviceDispatch *pPhysicalDeviceDispatch = tables::AssignDispatchTable(physicalDevice, instanceDispatch);
      instanceDispatch->PhysicalDevices.push_back(physicalDevice);
      instanceDispatch->PhysicalDeviceDispatches.push_back(pPhysicalDeviceDispatch);
    }
  }

  // Queues created with non-zero VkDeviceQueueCreateFlags (eg. PROTECTED_BIT or
  // INTERNALLY_SYNCHRONIZED_BIT_KHR) cannot be retrieved with vkGetDeviceQueue,
  // which returns VK_NULL_HANDLE for them. Use vkGetDeviceQueue2 with matching
  // flags for those, and keep the original call for flag-less queues so
  // Vulkan 1.0 devices remain unaffected.
  static inline VkQueue GetDeviceQueueByCreateInfo(const VkDeviceDispatch *deviceDispatch, VkDevice device, const VkDeviceQueueCreateInfo& queueInfo, uint32_t queueIndex) {
    VkQueue queue = VK_NULL_HANDLE;
    if (queueInfo.flags) {
      VkDeviceQueueInfo2 queueInfo2 = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
        .pNext = nullptr,
        .flags = queueInfo.flags,
        .queueFamilyIndex = queueInfo.queueFamilyIndex,
        .queueIndex = queueIndex,
      };
      deviceDispatch->GetDeviceQueue2(device, &queueInfo2, &queue);
    } else {
      deviceDispatch->GetDeviceQueue(device, queueInfo.queueFamilyIndex, queueIndex, &queue);
    }
    return queue;
  }

  static inline void CreateDispatchTable(const VkDeviceCreateInfo* pCreateInfo, PFN_vkGetDeviceProcAddr nextProcAddr, VkPhysicalDevice physicalDevice, VkDevice device) {
    auto physicalDeviceDispatch = vkroots::LookupDispatch(physicalDevice);
    auto deviceDispatch = DeviceDispatches.create(device, nextProcAddr, device, physicalDevice, physicalDeviceDispatch, pCreateInfo);

    for (uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; i++) {
      const auto &queueInfo = pCreateInfo->pQueueCreateInfos[i];
      for (uint32_t j = 0; j < queueInfo.queueCount; j++) {
        VkQueue queue = GetDeviceQueueByCreateInfo(deviceDispatch, device, queueInfo, j);

        tables::AssignDispatchTable(queue, deviceDispatch);
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
          tables::UnassignDispatchTable(physicalDevice);
      }
    }

    InstanceDispatches.erase(instance);
  }

  static inline void DestroyDispatchTable(VkDevice device) {
    const VkDeviceDispatch* deviceDispatch = DeviceDispatches.find(device);
    assert(deviceDispatch);
    if (!deviceDispatch)
      return;

    for (const auto& queueInfo : deviceDispatch->DeviceQueueInfos) {
      for (uint32_t i = 0; i < queueInfo.queueCount; i++) {
        VkQueue queue = GetDeviceQueueByCreateInfo(deviceDispatch, device, queueInfo, i);
        tables::UnassignDispatchTable(queue);
      }
    }

    DeviceDispatches.erase(device);
  }

}

