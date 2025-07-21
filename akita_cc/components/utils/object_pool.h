// Copyright (C) The (still) SANE Authors/Vincent Hengel 2023
#pragma once

#include <base/optional.h>
#include <base/containers/vector.h>

#include <queue>
#include <vector>

template <typename T, typename IndexType = u64, typename GenerationType = u64>
struct ObjectPool final {
  // safe, non-owning reference to an object in the pool.
  struct Handle {
    IndexType index = {};  // TODO: max<IndexType>() is not constexpr, so we use -1u.
    GenerationType generation = {-1};

    [[nodiscard]] bool IsValid() const noexcept {
      return index != static_cast<IndexType>(-1);
    }
    bool operator==(const Handle& other) const = default;
  };

  // For compatibility with std::unordered_map
  struct HandleHash {
    std::size_t operator()(const Handle& h) const noexcept {
      // A simple hash combination function.
      const std::size_t h1 = std::hash<IndexType>{}(h.index);
      const std::size_t h2 = std::hash<GenerationType>{}(h.generation);
      return h1 ^ (h2 << 1);
    }
  };

  static constexpr Handle kInvalidHandle();

  ObjectPool() = default;
  ObjectPool(const ObjectPool&) = delete;
  ObjectPool& operator=(const ObjectPool&) = delete;
  ObjectPool(ObjectPool&&) = delete;
  ObjectPool& operator=(ObjectPool&&) = delete;

  template <typename... Args>
  [[nodiscard]] Handle Create(Args&&... args) {
    IndexType index;
    if (free_indices_.empty()) {
      // No free slots, so create a new one at the end.
      index = static_cast<IndexType>(slots_.size());
      slots_.emplace_back();
    } else {
      // Reuse a slot from the free list.
      index = free_indices_.front();
      free_indices_.pop();
    }

    Slot& slot = slots_[index.value];
    // Emplace a new object, which calls T's constructor.
    slot.object.emplace(base::forward<Args>(args)...);
    // The slot's generation is already set. If it was a new slot, it's 0.
    // If it was a reused slot, its generation was incremented on destruction.
    return Handle{index, slot.generation};
  }

  bool Destroy(Handle handle) {
    if (!IsValid(handle)) {
      return false;
    }
    Slot& slot = slots_[handle.index];
    slot.object.reset();
    slot.generation++;
    free_indices_.push(handle.index);
    return true;
  }

  [[nodiscard]] T* Get(Handle handle) {
    if (!IsValid(handle)) {
      return nullptr;
    }
    return const_cast<T*>(static_cast<const ObjectPool&>(*this).Get(handle));
  }
  [[nodiscard]] const T* Get(Handle handle) const {
    if (!IsValid(handle)) {
      return nullptr;
    }
    return &(*slots_[handle.index.value].object);
  }

  [[nodiscard]] const T* GetByArrayIndex(mem_size idx) const {
    if (idx >= slots_.size()) {
      return nullptr;
    }
    const Slot& slot = slots_[idx];
    if (!slot.object.has_value()) {
      return nullptr;
    }
    return &(*slot.object);
  }

  // Checks if a handle currently points to a live object.
  [[nodiscard]] bool IsValid(Handle handle) const {
    return handle.IsValid() && handle.index < slots_.size() &&
           slots_[handle.index].generation == handle.generation &&
           slots_[handle.index].object.has_value();
  }

  [[nodiscard]] mem_size active_object_count() const noexcept {
    return slots_.size() - free_indices_.size();
  }
  [[nodiscard]] mem_size capacity() const noexcept { return slots_.size(); }

 private:
  // Each slot contains the object (optional) and its generation.
  struct Slot {
    std::optional<T> object; // TBD: base
    GenerationType generation = 0;
  };

  std::vector<Slot> slots_; // tbd: base
  std::queue<IndexType> free_indices_; // tbd: base
};