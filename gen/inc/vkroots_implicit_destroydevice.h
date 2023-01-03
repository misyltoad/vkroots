
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void implicit_wrap_DestroyDevice(
          VkDevice               device,
    const VkAllocationCallbacks* pAllocator) {
    const VkDeviceDispatch* dispatch = tables::LookupDeviceDispatch(device);
    // Implemented in the Dispatch class, goes to DestroyDeviceWrapper.
    // Make sure we call ours here.
    dispatch->DestroyDevice(device, pAllocator);
  }

