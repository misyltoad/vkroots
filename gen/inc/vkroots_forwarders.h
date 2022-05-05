namespace vkroots {

  // Consistency!
  using PFN_vkGetPhysicalDeviceProcAddr = PFN_GetPhysicalDeviceProcAddr;

  class VkInstanceDispatch;
  class VkPhysicalDeviceDispatch;
  class VkDeviceDispatch;

  class NoOverrides { static constexpr bool IsNoOverrides = true; };

  struct VkStructHeader {
    VkStructureType sType;
    void*           pNext;
  };

  template <VkStructureType SType, typename Type, typename AnyStruct>
  Type* FindInChain(AnyStruct* obj) {
    for (const VkStructHeader* header = reinterpret_cast<const VkStructHeader*>(obj); header; header = reinterpret_cast<const VkStructHeader*>(header->pNext)) {
      if (header->sType == SType)
        return reinterpret_cast<Type*>(header);
    }
    return nullptr;
  }

  namespace tables {

    template <typename T>
    class RawPointer {
    public:
      RawPointer(T* value) : m_value(value) {}
            T* get()       { return m_value; }
      const T* get() const { return m_value; }
    private:
      T* m_value;
    };

    template <typename Object, typename DispatchType, typename DispatchPtr>
    class VkDispatchTableMap {
    public:
      const DispatchType* insert(Object obj, DispatchPtr ptr) {
        assert(obj);
        auto lock = std::unique_lock(m_mutex);
        const DispatchType* val = ptr.get();
        m_map.insert(std::make_pair(obj, std::move(ptr)));
        return val;
      }
      void remove(Object obj) {
        assert(obj);
        auto lock = std::unique_lock(m_mutex);
        m_map.erase(obj);
      }
      const DispatchType* find(Object obj) const {
        if (!obj) return nullptr;
        auto lock = std::unique_lock(m_mutex);
        auto iter = m_map.find(obj);
        if (iter == m_map.end())
          return nullptr;
        return iter->second.get();
      }
    private:
      std::unordered_map<Object, DispatchPtr> m_map;
      mutable std::mutex m_mutex;
    };

    // All our dispatchables...
    inline VkDispatchTableMap<VkInstance,       VkInstanceDispatch,       std::unique_ptr<const VkInstanceDispatch>>       InstanceDispatches;
    inline VkDispatchTableMap<VkInstance,       VkPhysicalDeviceDispatch, std::unique_ptr<const VkPhysicalDeviceDispatch>> PhysicalDeviceInstanceDispatches;
    inline VkDispatchTableMap<VkPhysicalDevice, VkPhysicalDeviceDispatch, RawPointer     <const VkPhysicalDeviceDispatch>> PhysicalDeviceDispatches;
    inline VkDispatchTableMap<VkDevice,         VkDeviceDispatch,         std::unique_ptr<const VkDeviceDispatch>>         DeviceDispatches;
    inline VkDispatchTableMap<VkQueue,          VkDeviceDispatch,         RawPointer     <const VkDeviceDispatch>>         QueueDispatches;
    inline VkDispatchTableMap<VkCommandBuffer,  VkDeviceDispatch,         RawPointer     <const VkDeviceDispatch>>         CommandBufferDispatches;

    static inline const VkInstanceDispatch*       LookupInstanceDispatch      (VkInstance instance)             { return InstanceDispatches.find(instance); }
    static inline const VkPhysicalDeviceDispatch* LookupPhysicalDeviceDispatch(VkInstance instance)             { return PhysicalDeviceInstanceDispatches.find(instance); }
    static inline const VkPhysicalDeviceDispatch* LookupPhysicalDeviceDispatch(VkPhysicalDevice physicalDevice) { return PhysicalDeviceDispatches.find(physicalDevice); }
    static inline const VkDeviceDispatch*         LookupDeviceDispatch        (VkDevice device)                 { return DeviceDispatches.find(device); }
    static inline const VkDeviceDispatch*         LookupDeviceDispatch        (VkQueue device)                  { return QueueDispatches.find(device); }
    static inline const VkDeviceDispatch*         LookupDeviceDispatch        (VkCommandBuffer cmdBuffer)       { return CommandBufferDispatches.find(cmdBuffer); }

    static inline void CreateDispatchTable(PFN_vkGetInstanceProcAddr nextInstanceProcAddr, PFN_GetPhysicalDeviceProcAddr nextPhysDevProcAddr, VkInstance instance);
    static inline void CreateDispatchTable(const VkDeviceCreateInfo* pCreateInfo, PFN_vkGetDeviceProcAddr nextProcAddr, VkPhysicalDevice physicalDevice, VkDevice device);
  }

  struct VkInstanceProcAddrFuncs {
    PFN_vkGetInstanceProcAddr NextGetInstanceProcAddr;
    PFN_vkGetPhysicalDeviceProcAddr NextGetPhysicalDeviceProcAddr;
  };

  static inline VkInstanceProcAddrFuncs GetProcAddrs(const VkInstanceCreateInfo* pInfo) {
    const void* pNext = (const void*) pInfo;
    const VkLayerInstanceCreateInfo* layerInfo;
    while ((layerInfo = FindInChain<VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO, const VkLayerInstanceCreateInfo>(pNext)) && layerInfo->function != VK_LAYER_LINK_INFO)
      pNext = layerInfo->pNext;
    assert(layerInfo);
    return { layerInfo->u.pLayerInfo->pfnNextGetInstanceProcAddr, layerInfo->u.pLayerInfo->pfnNextGetPhysicalDeviceProcAddr };
  }

  static inline PFN_vkGetDeviceProcAddr GetProcAddrs(const VkDeviceCreateInfo* pInfo) {
    const void* pNext = (const void*) pInfo;
    const VkLayerDeviceCreateInfo* layerInfo;
    while ((layerInfo = FindInChain<VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO, const VkLayerDeviceCreateInfo>(pNext)) && layerInfo->function != VK_LAYER_LINK_INFO)
      pNext = layerInfo->pNext;
    assert(layerInfo);
    return layerInfo->u.pLayerInfo->pfnNextGetDeviceProcAddr;
  }

}
