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

  // RwLock impl by doitsujin
  class RwLock {
    static constexpr uint32_t ReadBit  = 1u;
    static constexpr uint32_t WriteBit = 1u << 31u;
  public:

    RwLock() = default;

    RwLock(const RwLock&) = delete;

    RwLock& operator = (const RwLock&) = delete;

    void lock() {
      auto value = m_lock.load(std::memory_order_relaxed);

      while (value || !m_lock.compare_exchange_strong(value, WriteBit, std::memory_order_acquire, std::memory_order_relaxed)) {
        m_lock.wait(value, std::memory_order_acquire);
        value = m_lock.load(std::memory_order_relaxed);
      }
    }

    bool try_lock() {
      auto value = m_lock.load(std::memory_order_relaxed);

      if (value)
        return false;

      return m_lock.compare_exchange_strong(value, WriteBit, std::memory_order_acquire, std::memory_order_relaxed);
    }

    void unlock() {
      m_lock.store(0u, std::memory_order_release);
      m_lock.notify_all();
    }

    void lock_shared() {
      auto value = m_lock.load(std::memory_order_relaxed);

      do {
        while (value & WriteBit) {
          m_lock.wait(value, std::memory_order_acquire);
          value = m_lock.load(std::memory_order_relaxed);
        }
      } while (!m_lock.compare_exchange_strong(value, value + ReadBit, std::memory_order_acquire, std::memory_order_relaxed));
    }

    bool try_lock_shared() {
      auto value = m_lock.load(std::memory_order_relaxed);

      if (value & WriteBit)
        return false;

      return m_lock.compare_exchange_strong(value, value + ReadBit, std::memory_order_acquire, std::memory_order_relaxed);
    }

    void unlock_shared() {
      m_lock.fetch_sub(ReadBit, std::memory_order_release);
      m_lock.notify_one();
    }

  private:

    std::atomic<uint32_t> m_lock = { 0u };

  };

  template <typename K, typename V>
  class ObjectMap {
  public:

    V *find(const K& key) const {
      std::shared_lock lock(m_lock);

      auto entry = m_map.find(key);

      if (entry == m_map.end())
        return nullptr;

      return entry->second.get();
    }


    template<typename... Args>
    V* create(const K& key, Args&&... args) {
      std::unique_lock lock(m_lock);

      auto result = m_map.emplace(std::piecewise_construct,
        std::tuple(key),
        std::tuple());

      if (!result.second)
        return nullptr;

      result.first->second = std::make_unique<V>(std::forward<Args>(args)...);
      return result.first->second.get();
    }

    void erase(const K& key) {
      std::unique_lock lock(m_lock);
      m_map.erase(key);
    }
  private:
    std::unordered_map<K, std::unique_ptr<V>> m_map;
    mutable RwLock m_lock;
  };

  namespace tables {

    // All our dispatchables...
    inline ObjectMap<VkInstance,               const VkInstanceDispatch>               InstanceDispatches;
    inline ObjectMap<VkPhysicalDevice,         const VkPhysicalDeviceDispatch>         PhysicalDeviceDispatches;
    inline ObjectMap<VkDevice,                 const VkDeviceDispatch>                 DeviceDispatches;
    inline ObjectMap<VkQueue,                  const VkQueueDispatch>                  QueueDispatches;
    inline ObjectMap<VkCommandBuffer,          const VkCommandBufferDispatch>          CommandBufferDispatches;
    inline ObjectMap<VkExternalComputeQueueNV, const VkExternalComputeQueueNVDispatch> ExternalComputeQueueDispatches;

    static inline void CreateDispatchTable(PFN_vkGetInstanceProcAddr nextInstanceProcAddr, PFN_GetPhysicalDeviceProcAddr nextPhysDevProcAddr, VkInstance instance);
    static inline void CreateDispatchTable(const VkDeviceCreateInfo* pCreateInfo, PFN_vkGetDeviceProcAddr nextProcAddr, VkPhysicalDevice physicalDevice, VkDevice device);
    static inline void DestroyDispatchTable(VkInstance instance);
    static inline void DestroyDispatchTable(VkDevice device);

    static inline const VkPhysicalDeviceDispatch *AssignDispatchTable(VkPhysicalDevice physDev, const VkInstanceDispatch *pDispatch) { return PhysicalDeviceDispatches.create(physDev, physDev, pDispatch); }
    static inline const VkCommandBufferDispatch *AssignDispatchTable(VkCommandBuffer cmdBuffer, const VkDeviceDispatch *pDispatch) { return CommandBufferDispatches.create(cmdBuffer, cmdBuffer, pDispatch); }
    static inline const VkQueueDispatch *AssignDispatchTable(VkQueue queue, const VkDeviceDispatch *pDispatch) { return QueueDispatches.create(queue, queue, pDispatch); }
    static inline const VkExternalComputeQueueNVDispatch *AssignDispatchTable(VkExternalComputeQueueNV queue, const VkDeviceDispatch *pDispatch) { return ExternalComputeQueueDispatches.create(queue, queue, pDispatch); }
    static inline void UnassignDispatchTable(VkPhysicalDevice physDev) { PhysicalDeviceDispatches.erase(physDev); }
    static inline void UnassignDispatchTable(VkCommandBuffer cmdBuffer) { CommandBufferDispatches.erase(cmdBuffer); }
    static inline void UnassignDispatchTable(VkQueue queue) { QueueDispatches.erase(queue); }
    static inline void UnassignDispatchTable(VkExternalComputeQueueNV queue) { ExternalComputeQueueDispatches.erase(queue); }
  }

  static inline const VkInstanceDispatch*               LookupDispatch        (VkInstance instance)                             { return tables::InstanceDispatches.find(instance); }
  static inline const VkPhysicalDeviceDispatch*         LookupDispatch        (VkPhysicalDevice physicalDevice)                 { return tables::PhysicalDeviceDispatches.find(physicalDevice); }
  static inline const VkDeviceDispatch*                 LookupDispatch        (VkDevice device)                                 { return tables::DeviceDispatches.find(device); }
  static inline const VkQueueDispatch*                  LookupDispatch        (VkQueue device)                                  { return tables::QueueDispatches.find(device); }
  static inline const VkCommandBufferDispatch*          LookupDispatch        (VkCommandBuffer cmdBuffer)                       { return tables::CommandBufferDispatches.find(cmdBuffer); }
  static inline const VkExternalComputeQueueNVDispatch* LookupDispatch        (VkExternalComputeQueueNV externalComputeQueueNV) { return tables::ExternalComputeQueueDispatches.find(externalComputeQueueNV); }

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
