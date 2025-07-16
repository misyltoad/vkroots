// The vkroots.h header is licensed under Apache-2.0 OR MIT
// as it was generated from the Vulkan Registry, which is licensed
// under the same license.

#pragma once

#include <vulkan/vk_layer.h>
#include <vulkan/vulkan.h>

#include <cstring>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <type_traits>
#include <memory>
#include <cassert>
#include <vector>
#include <utility>
#include <optional>
#include <string_view>
#include <array>
#include <functional>
#include <shared_mutex>
#include <atomic>
#include <ranges>
#include <format>
#include <iostream>
#include <algorithm>

#define VKROOTS_VERSION_MAJOR 0
#define VKROOTS_VERSION_MINOR 1
#define VKROOTS_VERSION_PATCH 0

#define VKROOTS_VERSION VK_MAKE_API_VERSION(0, VKROOTS_VERSION_MAJOR, VKROOTS_VERSION_MINOR, VKROOTS_VERSION_PATCH)

namespace vkroots {

  class GenericUserData {
  public:
    GenericUserData() {}
    ~GenericUserData() {
      destroy();
    }

    template <typename T, typename... Args>
    void emplace(Args&&... args) {
      destroy();

      m_data = new T{ std::forward<Args>(args)... };
      m_type = []() -> std::type_info const&{ return typeid(T); };
      m_destroy = [](void* data) -> void { delete static_cast<T*>(data); };
    }

    template <typename T>
    void set(T* ptr) {
      m_data = ptr;
      m_type = []() -> std::type_info const&{ return typeid(T*); };
      m_destroy = [](void* data) -> void {}; // Do nothing for implicit ptrs.
    }

    bool has() {
      return m_data != nullptr;
    }

    const std::type_info &type() {
      if (!has())
        return typeid(nullptr);
      return m_type();
    }

    void destroy() {
      if (!m_data)
        return;
      m_destroy(m_data);
      m_data = nullptr;
    }

    template <typename T>
    T& cast() {
      assert(type() == typeid(T));
      return *static_cast<T*>(m_data);
    }

    template <typename T>
    operator T& () {
      return cast<T>();
    }

    operator bool() {
      return has();
    }

  private:
    void* m_data = nullptr;
    std::type_info const &(*m_type)() = nullptr;
    void (*m_destroy)(void *data) = nullptr;
  };

  template <typename T>
  T& userdata_cast(GenericUserData &userdata) {
    return userdata.cast<T>();
  }

}
