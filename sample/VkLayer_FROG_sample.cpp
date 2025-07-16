#include "vkroots.h"

#include <cstdio>

namespace MyLayer {

  class VkDeviceOverrides {
  public:
    static VkResult CreateImage(
      const vkroots::VkDeviceDispatch& dispatch,
            VkDevice                   device,
      const VkImageCreateInfo*         pCreateInfo,
      const VkAllocationCallbacks*     pAllocator,
            VkImage*                   pImage) {
      printf("The app has made an image, I bet it's going to be frogtastically beautiful!\n");
      return dispatch.CreateImage(device, pCreateInfo, pAllocator, pImage);
    }
  };

}

VKROOTS_DEFINE_LAYER_INTERFACES(vkroots::NoOverrides,
                                MyLayer::VkDeviceOverrides);
