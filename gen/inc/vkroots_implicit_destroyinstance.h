
  template <typename InstanceOverrides, typename PhysicalDeviceOverrides, typename DeviceOverrides>
  static void implicit_wrap_DestroyInstance(
          VkInstance             instance,
    const VkAllocationCallbacks* pAllocator) {
    const VkInstanceDispatch* dispatch = tables::LookupInstanceDispatch(instance);
    // Implemented in the Dispatch class, goes to DestroyInstanceWrapper.
    // Make sure we call ours here.
    dispatch->DestroyInstance(instance, pAllocator);
  }

