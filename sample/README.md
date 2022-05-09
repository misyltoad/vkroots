# VkLayer_FROG_sample

This is an example layer using vkroots.

## How should I name my layer?

Your layer .json should be called `VkLayer_[prefix]_[name].json`.

Your layer name in the .json should similarly conform to the following spec:
`VK_LAYER_[prefix]_[name]`.

## What should vkNegotiateLoaderLayerInterfaceVersion be set to for my layer?

The mapping for the `vkNegotiateLoaderLayerInterfaceVersion` function should either be `vkNegotiateLoaderLayerInterfaceVersion` or whatever you defined `VKROOTS_NEGOTIATION_INTERFACE` to be before including `vkroots.h`.
Defining your own `VKROOTS_NEGOTIATION_INTERFACE` is useful when you want to expose multiple layers from one .so

## Notes about prefixes

From the [Vulkan spec](https://www.khronos.org/registry/vulkan/specs/1.3/styleguide.html#extensions-naming-conventions):

```
    Layers are named with the syntax VK_LAYER_<author>_<name> or VK_LAYER_<fqdn>_<name>.

    Extension and layer names also contain an author ID, indicated by above, identifying the author of the extension/layer. This ID is a short, capitalized string identifying an author, such as a Khronos member developing Vulkan implementations for their devices, or a non-Khronos developer creating Vulkan layers. Author IDs must be registered with Khronos.

    Layer authors that prefer not to register an author ID can instead use a fully-qualified domain name (FQDN) in reverse-order as an author ID, replacing . (period) with _ (underscore) characters. The restriction that layer names must be valid C identifiers means that some FQDNs cannot be used as part of layer names.

    [...]

    The FQDN should be a domain name owned by the author.
```
