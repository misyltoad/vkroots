![vkroots logo](assets/vkroots.png)

# What is vkroots?

vkroots is a framework for writing Vulkan layers that takes **all** the complexity/hastle away from you! It's so simple!

## Example

vkroots is incredibly easy to integrate into your project, it's a single `#include`, and even supports
defining multiple layers in a single shared library with `VKROOTS_NEGOTIATION_INTERFACE`.

All you do is just implement the functions you want, and they are automagically hooked using C++20 concepts magic! Just watch:

```cpp
#include "vkroots.h"

#include <cstdio>

namespace MyLayer {

  class VkDeviceOverrides {
  public:
    static VkResult CreateImage(
      const vkroots::VkDeviceDispatch* pDispatch,
            VkDevice                   device,
      const VkImageCreateInfo*         pCreateInfo,
      const VkAllocationCallbacks*     pAllocator,
            VkImage*                   pImage) {
      printf("The app has made an image, I bet it's going to be frogtastically beautiful!\n");
      return pDispatch->CreateImage(device, pCreateInfo, pAllocator, pImage);
    }
  };

}

VKROOTS_DEFINE_LAYER_INTERFACES(vkroots::NoOverrides,
                                vkroots::NoOverrides,
                                MyLayer::VkDeviceOverrides);
```

## How do I pull this into my project?

You can either add this repo as a git submodule, copy the header from this repo directly, or generate it yourself with `gen/make_vkroots`.

The vkroots header can be generated from any Vulkan Registry XML (even for unreleased/non-standard extensions).
This was used, for example, in the sample [VK_FOOL_printed_surface](https://github.com/Joshua-Ashton/VkLayer_FOOL_printed_surface) implementation using CUPS.

## Dependencies

There are no dependencies other a C++20-capable compiler.

If you wish to generate the `vkroots.h` header from a Vulkan Registry XML, you will need Python 3.

## Contributing

If you find any issues with the project or have any feature requests, please feel free to make an issue or a pull request.

## Projects using vkroots

 - [VK_FOOL_printed_surface](https://github.com/Joshua-Ashton/VkLayer_FOOL_printed_surface)

## License

The python generator scripts: `gen/make_vkroots`, `gen/vulkan_helpers.py`, are
licensed under LGPL v2.1 as they take a bunch of code from WineVulkan, see the header of
make_vkroots and vulkan_helpers.py for more verbose license info.

Everything else, `vkroots.h` header, other intermediate headers (`inc`), and samples are licensed under Apache-2.0 OR MIT to be compatible
with the Vulkan Registry XML.

See the top of associated files for more information.

# Happy Layering! 🐸✨