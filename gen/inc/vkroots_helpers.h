namespace vkroots {

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

  static bool contains(const std::vector<const char *> vec, std::string_view lookupValue) {
    return std::ranges::any_of(vec, std::bind_front(std::equal_to{}, lookupValue));
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

  // For dispatch functions, you might need eg:
  //   [&] (auto... args) { dispatch.GetPhysicalDeviceQueueFamilyProperties(args...); },
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

  // For dispatch functions, you might need eg:
  //   [&] (auto... args) { dispatch.GetPhysicalDeviceQueueFamilyProperties(args...); },
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

  namespace log
  {
      enum LogLevel
      {
          Fatal,
          Error,
          Warning,
          Info,
          Debug,

          Count
      };

      constexpr std::string_view ToString(LogLevel level)
      {
          switch (level)
          {
              case Fatal:   return "fatal";
              case Error:   return "error";
              case Warning: return "warning";
              default:
              case Info:    return "info";
              case Debug:   return "debug";
          }
      }

      constexpr LogLevel FromString(std::string_view scope)
      {
          if (scope == "fatal")
              return Fatal;
          else if (scope == "error")
              return Error;
          else if (scope == "warning")
              return Warning;
          else if (scope == "debug")
              return Debug;
          else
              return Info;
      }

      constexpr std::string_view ToPrint(LogLevel level)
      {
          switch (level)
          {
              case Fatal:		return "\e[38;2;0;0;0;48;2;255;0;0m" "Fatal " "\e[0m";
              case Error:		return "\e[0;31m" "Error " "\e[0m";
              case Warning:	return " \e[0;33m" "Warn " "\e[0m";
              default:
              case Info:		return " \e[0;34m" "Info " "\e[0m";
              case Debug:		return "\e[0;35m" "Debug " "\e[0m";
          }
      }

      template <typename... Args>
      void print_log(std::string_view file, int line, LogLevel level, std::string_view prefix, std::format_string<Args...> fmt, Args&&... args)
      {
          std::string msg = std::format(std::move(fmt), std::forward<Args>(args)...);
          //std::print(stderr, "{} {}: {}\n", ToPrint(level), prefix, msg);
          int spaceCount = std::max(13 - int(prefix.length()), 0);
          std::string out = std::format("{}| {}{:>{}}| {} \e[0;90m({}:{})\e[0m", ToPrint(level), prefix, ' ', spaceCount, msg, file, line);
          std::cout << out << std::endl;
      }

      class LogScope
      {
      public:
          LogScope(std::string_view name, LogLevel maxLevel = log::Info)
              : LogScope(name, name, maxLevel)
          {
          }

          LogScope(std::string_view name, std::string_view prefix, LogLevel maxLevel = log::Info)
              : m_name{ name }
              , m_prefix{ prefix }
              , m_maxLevel{ maxLevel }
          {
          }

          ~LogScope()
          {
          }

          bool Enabled(LogLevel level) const
          {
              return level <= m_maxLevel;
          }

          void SetLevel(LogLevel level)
          {
              m_maxLevel = level;
          }

          template <typename... Args> 
          void log(std::string_view file, int line, LogLevel level, std::format_string<Args...> fmt, Args&&... args)
          {
              if (!Enabled(level))
                  return;

              print_log(file, line, level, m_prefix, std::move(fmt), std::forward<Args>(args)...);
          }

      private:
          std::string_view m_name;
          std::string_view m_prefix;

          LogLevel m_maxLevel = vkroots::log::Info;
      };

      namespace util
      {
          template <typename T, size_t S>
          constexpr size_t GetFileNameOffset(const T (& str)[S], size_t i = S - 1)
          {
              return (str[i] == '/' || str[i] == '\\') ? i + 1 : (i > 0 ? GetFileNameOffset(str, i - 1) : 0);
          }

          template <typename T>
          constexpr size_t GetFileNameOffset(T (& str)[1])
          {
              return 0;
          }
      }

      #define vkr_log_generic(scope, level, ...) \
          (log_ ## scope).log((&__FILE__[::vkroots::log::util::GetFileNameOffset(__FILE__)]), (__LINE__), (level), __VA_ARGS__)
      #define vkr_log_debug(scope, ...) vkr_log_generic(scope, ::vkroots::log::Debug, __VA_ARGS__)
      #define vkr_log_info(scope, ...) vkr_log_generic(scope, ::vkroots::log::Info, __VA_ARGS__)
      #define vkr_log_warn(scope, ...) vkr_log_generic(scope, ::vkroots::log::Warning, __VA_ARGS__)
      #define vkr_log_err(scope, ...) vkr_log_generic(scope, ::vkroots::log::Error, __VA_ARGS__)
      #define vkr_log_fatal(scope, ...) vkr_log_generic(scope, ::vkroots::log::Fatal, __VA_ARGS__)

  }


}