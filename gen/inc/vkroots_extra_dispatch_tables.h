namespace vkroots {

  class VkPhysicalDeviceDispatch
  {
  public:
    VkPhysicalDeviceDispatch( const VkInstanceDispatch *pInstanceDispatch )
      : pInstanceDispatch{ pInstanceDispatch }
    {
    }

    const VkInstanceDispatch *pInstanceDispatch = nullptr;
  };

}
