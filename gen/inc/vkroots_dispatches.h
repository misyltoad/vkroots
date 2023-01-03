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

