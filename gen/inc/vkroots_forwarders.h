namespace vkroots {

  // Consistency!
  using PFN_vkGetPhysicalDeviceProcAddr = PFN_GetPhysicalDeviceProcAddr;

  class VkInstanceDispatch;
  class VkPhysicalDeviceDispatch;
  class VkDeviceDispatch;
  class VkQueueDispatch;
  class VkCommandBufferDispatch;
  class VkExternalComputeQueueNVDispatch;

  class NoOverrides { static constexpr bool IsNoOverrides = true; };

  template <typename Type>
  constexpr VkStructureType ResolveSType();

  template <> constexpr VkStructureType ResolveSType<VkLayerInstanceCreateInfo>() { return VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO; }
  template <> constexpr VkStructureType ResolveSType<const VkLayerInstanceCreateInfo>() { return VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO; }
  template <> constexpr VkStructureType ResolveSType<VkLayerDeviceCreateInfo>() { return VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO; }
  template <> constexpr VkStructureType ResolveSType<const VkLayerDeviceCreateInfo>() { return VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO; }

  template <typename T>
  static constexpr bool TypeIsSinglePointer() {
    // If we aren't a pointer at all, return false
    // eg. int
    if (!std::is_pointer<T>::value)
      return false;

    // If we are still a pointer after removing pointer, return false.
    // eg. void**
    if (std::is_pointer<typename std::remove_pointer<T>::type>::value)
      return false;

    // Must be a single * ptr.
    return true;
  }

  template <typename Type, typename AnyStruct>
  const Type* FindInChain(const AnyStruct* obj) {
    static_assert(TypeIsSinglePointer<decltype(obj)>());

    for (const VkBaseInStructure* header = reinterpret_cast<const VkBaseInStructure*>(obj); header; header = header->pNext) {
      if (header->sType == ResolveSType<Type>())
        return reinterpret_cast<const Type*>(header);
    }
    return nullptr;
  }

  template <typename Type, typename AnyStruct>
  Type* FindInChainMutable(AnyStruct* obj) {
    static_assert(TypeIsSinglePointer<decltype(obj)>());

    for (VkBaseOutStructure* header = reinterpret_cast<VkBaseOutStructure*>(obj); header; header = header->pNext) {
      if (header->sType == ResolveSType<Type>())
        return reinterpret_cast<Type*>(header);
    }
    return nullptr;
  }

  template <typename Type, typename AnyStruct>
  std::tuple<Type *, VkBaseOutStructure *> RemoveFromChain(AnyStruct *obj) {
    static_assert(TypeIsSinglePointer<decltype(obj)>());

    for (VkBaseOutStructure* header = reinterpret_cast<VkBaseOutStructure*>(obj); header; header = header->pNext) {
      VkBaseOutStructure *pNextInChain = header->pNext;
      if (pNextInChain && pNextInChain->sType == ResolveSType<Type>()) {
        header->pNext = pNextInChain->pNext;
        return std::make_tuple(reinterpret_cast<Type*>(pNextInChain), header);
      }
    }
    return std::make_tuple(nullptr, nullptr);
  }

  template <typename Type, typename AnyStruct>
  Type *AddToChain(AnyStruct *pParent, Type *pType) {
    static_assert(TypeIsSinglePointer<decltype(pParent)>());
    static_assert(TypeIsSinglePointer<decltype(pType)>());

    void **ppParentNext = reinterpret_cast<void **>(&pParent->pNext);
    void **ppTypeNext   = reinterpret_cast<void **>(&pType->pNext);

    *ppTypeNext = std::exchange(*ppParentNext, reinterpret_cast<void *>(pType));
    return pType;
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
    inline VkDispatchTableMap<VkInstance,               VkInstanceDispatch,               std::unique_ptr<const VkInstanceDispatch>>       InstanceDispatches;
    inline VkDispatchTableMap<VkPhysicalDevice,         VkPhysicalDeviceDispatch,         std::unique_ptr<const VkPhysicalDeviceDispatch>> PhysicalDeviceDispatches;
    inline VkDispatchTableMap<VkDevice,                 VkDeviceDispatch,                 std::unique_ptr<const VkDeviceDispatch>>         DeviceDispatches;
    inline VkDispatchTableMap<VkQueue,                  VkQueueDispatch,                  std::unique_ptr<const VkQueueDispatch>>          QueueDispatches;
    inline VkDispatchTableMap<VkCommandBuffer,          VkCommandBufferDispatch,          std::unique_ptr<const VkCommandBufferDispatch>>  CommandBufferDispatches;
    inline VkDispatchTableMap<VkExternalComputeQueueNV, VkExternalComputeQueueNVDispatch, std::unique_ptr<const VkExternalComputeQueueNVDispatch>> ExternalComputeQueueDispatches;

    static inline const VkInstanceDispatch*               LookupDispatch        (VkInstance instance)             { return InstanceDispatches.find(instance); }
    static inline const VkPhysicalDeviceDispatch*         LookupDispatch        (VkPhysicalDevice physicalDevice) { return PhysicalDeviceDispatches.find(physicalDevice); }
    static inline const VkDeviceDispatch*                 LookupDispatch        (VkDevice device)                 { return DeviceDispatches.find(device); }
    static inline const VkQueueDispatch*                  LookupDispatch        (VkQueue device)                  { return QueueDispatches.find(device); }
    static inline const VkCommandBufferDispatch*          LookupDispatch        (VkCommandBuffer cmdBuffer)       { return CommandBufferDispatches.find(cmdBuffer); }
    static inline const VkExternalComputeQueueNVDispatch* LookupDispatch        (VkExternalComputeQueueNV externalComputeQueueNV) { return ExternalComputeQueueDispatches.find(externalComputeQueueNV); }

    static inline void CreateDispatchTable(PFN_vkGetInstanceProcAddr nextInstanceProcAddr, PFN_GetPhysicalDeviceProcAddr nextPhysDevProcAddr, VkInstance instance);
    static inline void CreateDispatchTable(const VkDeviceCreateInfo* pCreateInfo, PFN_vkGetDeviceProcAddr nextProcAddr, VkPhysicalDevice physicalDevice, VkDevice device);
    static inline void DestroyDispatchTable(VkInstance instance);
    static inline void DestroyDispatchTable(VkDevice device);

    static inline void AssignDispatchTable(VkPhysicalDevice physDev, const VkInstanceDispatch *pDispatch) { PhysicalDeviceDispatches.insert(physDev, std::make_unique<VkPhysicalDeviceDispatch>(physDev, pDispatch)); }
    static inline void AssignDispatchTable(VkCommandBuffer cmdBuffer, const VkDeviceDispatch *pDispatch) { CommandBufferDispatches.insert(cmdBuffer, std::make_unique<VkCommandBufferDispatch>(cmdBuffer, pDispatch)); }
    static inline void AssignDispatchTable(VkQueue queue, const VkDeviceDispatch *pDispatch) { QueueDispatches.insert(queue, std::make_unique<VkQueueDispatch>(queue, pDispatch)); }
    static inline void AssignDispatchTable(VkExternalComputeQueueNV queue, const VkDeviceDispatch *pDispatch) { ExternalComputeQueueDispatches.insert(queue, std::make_unique<VkExternalComputeQueueNVDispatch>(queue, pDispatch)); }
    static inline void UnassignDispatchTable(VkPhysicalDevice physDev) { PhysicalDeviceDispatches.remove(physDev); }
    static inline void UnassignDispatchTable(VkCommandBuffer cmdBuffer) { CommandBufferDispatches.remove(cmdBuffer); }
    static inline void UnassignDispatchTable(VkQueue queue) { QueueDispatches.remove(queue); }
    static inline void UnassignDispatchTable(VkExternalComputeQueueNV queue) { ExternalComputeQueueDispatches.remove(queue); }
  }

  struct VkInstanceProcAddrFuncs {
    PFN_vkGetInstanceProcAddr NextGetInstanceProcAddr;
    PFN_vkGetPhysicalDeviceProcAddr NextGetPhysicalDeviceProcAddr;
  };

  static inline VkResult GetProcAddrs(const VkInstanceCreateInfo* pInfo, VkInstanceProcAddrFuncs *pOutFuncs) {
    const void* pNext = (const void*) pInfo;
    const VkLayerInstanceCreateInfo* layerInfo;
    while ((layerInfo = FindInChain<const VkLayerInstanceCreateInfo>(pNext)) && layerInfo->function != VK_LAYER_LINK_INFO)
      pNext = layerInfo->pNext;
    assert(layerInfo);
    if (!layerInfo)
      return VK_ERROR_INITIALIZATION_FAILED;
    *pOutFuncs = VkInstanceProcAddrFuncs{ layerInfo->u.pLayerInfo->pfnNextGetInstanceProcAddr, layerInfo->u.pLayerInfo->pfnNextGetPhysicalDeviceProcAddr };
    // Josh:
    // It really sucks that we have to advance this ourselves given the const situation here... 
    VkLayerInstanceCreateInfo* layerInfoMutable = const_cast<VkLayerInstanceCreateInfo *>(layerInfo);
    layerInfoMutable->u.pLayerInfo = layerInfoMutable->u.pLayerInfo->pNext;
    return VK_SUCCESS;
  }

  static inline VkResult GetProcAddrs(const VkDeviceCreateInfo* pInfo, PFN_vkGetDeviceProcAddr *pOutAddr) {
    const void* pNext = (const void*) pInfo;
    const VkLayerDeviceCreateInfo* layerInfo;
    while ((layerInfo = FindInChain<const VkLayerDeviceCreateInfo>(pNext)) && layerInfo->function != VK_LAYER_LINK_INFO)
      pNext = layerInfo->pNext;
    assert(layerInfo);
    if (!layerInfo)
      return VK_ERROR_INITIALIZATION_FAILED;
    *pOutAddr = layerInfo->u.pLayerInfo->pfnNextGetDeviceProcAddr;
    // Josh:
    // It really sucks that we have to advance this ourselves given the const situation here... 
    VkLayerDeviceCreateInfo* layerInfoMutable = const_cast<VkLayerDeviceCreateInfo *>(layerInfo);
    layerInfoMutable->u.pLayerInfo = layerInfoMutable->u.pLayerInfo->pNext;
    return VK_SUCCESS;
  }

}
