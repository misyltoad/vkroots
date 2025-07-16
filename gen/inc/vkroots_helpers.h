
namespace vkroots::helpers {

  template <typename Func>
  inline void delimitStringView(std::string_view view, std::string_view delim, Func func) {
    size_t pos = 0;
    while ((pos = view.find(delim)) != std::string_view::npos) {
      std::string_view token = view.substr(0, pos);
      if (!func(token))
        return;
      view = view.substr(pos + 1);
    }
    func(view);
  }

  template <typename T, typename ArrType, typename Op>
  inline VkResult array(ArrType& arr, uint32_t *pCount, T* pOut, Op func) {
    const uint32_t count = uint32_t(arr.size());

    if (!pOut) {
      *pCount = count;
      return VK_SUCCESS;
    }

    const uint32_t outCount = std::min(*pCount, count);
    for (uint32_t i = 0; i < outCount; i++)
      func(pOut[i], arr[i]);

    *pCount = outCount;
    return count != outCount
      ? VK_INCOMPLETE
      : VK_SUCCESS;
  }

  template <typename T, typename ArrType>
  inline VkResult array(ArrType& arr, uint32_t *pCount, T* pOut) {
    return array(arr, pCount, pOut, [](T& x, const T& y) { x = y; });
  }

  template <typename Func, typename OutArray, typename... Args>
  uint32_t enumerate(Func function, OutArray& outArray, Args&&... arguments) {
    uint32_t count = 0;
    function(arguments..., &count, nullptr);

    outArray.resize(count);
    if (!count)
        return 0;

    function(std::forward<Args>(arguments)..., &count, outArray.data());
    return count;
  }

  template <typename Func, typename InArray, typename OutType, typename... Args>
  VkResult append(Func function, const InArray& inArray, uint32_t* pOutCount, OutType* pOut, Args&&... arguments) {
    uint32_t baseCount = 0;
    function(std::forward<Args>(arguments)..., &baseCount, nullptr);

    const uint32_t totalCount = baseCount + uint32_t(inArray.size());
    if (!pOut) {
      *pOutCount = totalCount;
      return VK_SUCCESS;
    }

    if (*pOutCount < totalCount) {
      function(std::forward<Args>(arguments)..., pOutCount, pOut);
      return VK_INCOMPLETE;
    }

    function(std::forward<Args>(arguments)..., &baseCount, pOut);
    for (size_t i = 0; i < inArray.size(); i++)
      pOut[baseCount + i] = inArray[i];
    return VK_SUCCESS;
  }

  template <typename SearchType, VkStructureType StructureTypeEnum, typename ChainBaseType>
  SearchType *chain(ChainBaseType* pNext) {
    for (VkBaseOutStructure* pBaseOut = reinterpret_cast<VkBaseOutStructure*>(pNext); pBaseOut; pBaseOut = pBaseOut->pNext) {
      if (pBaseOut->sType == StructureTypeEnum)
        return reinterpret_cast<SearchType*>(pBaseOut);
    }
    return nullptr;
  }

}

namespace vkroots {
  template <typename Type, typename UserData = uint64_t>
  class ChainPatcher {
  public:
    template <typename AnyStruct>
    ChainPatcher(const AnyStruct *obj, std::function<bool(UserData&, Type *)> func) {
      const Type *type = vkroots::FindInChain<Type>(obj);
      if (type) {
        func(m_ctx, const_cast<Type *>(type));
      } else {
        if (func(m_ctx, &m_value)) {
          AnyStruct *mutObj = const_cast<AnyStruct*>(obj);
          m_value.sType = ResolveSType<Type>();
          m_value.pNext = const_cast<void*>(std::exchange(mutObj->pNext, reinterpret_cast<const void*>(&m_value)));
        }
      }
    }

    template <typename AnyStruct>
    ChainPatcher(const AnyStruct *obj, std::function<bool(Type *)> func)
      : ChainPatcher(obj, [&](UserData& ctx, Type *obj) { return func(obj); }) {
    }

  private:
    Type m_value{};
    UserData m_ctx;
  };
}
