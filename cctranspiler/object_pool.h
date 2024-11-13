// Copyright (C) The (still) SANE Authors/Vincent Hengel 2023
#pragma once

#include <queue>
#include <base/containers/vector.h>

#include <vector>
#include <queue>
// TODO: support for an invalid handle ref...
template <class T, typename TReal, typename THandle = u64>
class ObjectPool {
 public:
  using handle_type = THandle;
  using value_type = T;

  struct ObjectPair {
    T& obj;
    TReal handle;
  };

  explicit ObjectPool() : next_id_(0) {}

  template <typename... TArgs>
  ObjectPair Create(TArgs&&... args) {
    handle_type object_id;
    if (free_ids_.empty()) {
      object_id = next_id_++;
      storage_.emplace_back(base::forward<TArgs>(args)...);
    } else {
      object_id = free_ids_.front();
      free_ids_.pop();
      CHECK_BREAK;
      // storage_[object_id] = T(base::forward<TArgs>(args)...);
    }
    return ObjectPair{storage_[object_id], (TReal)object_id};
  }

  // owie
  template <typename TFunc>
  value_type* Find(TFunc&& func) {
    for (auto& obj : storage_) {
      if (func(obj )) {
        return &obj;
      }
    }
    return nullptr;
  }

  bool Remove(TReal handle) {
    const THandle h = (THandle)handle;
    if (h >= 0 && h < next_id_) {
      free_ids_.push(h);
      return true;
    }
    return false;
  }

  inline const T* Get(THandle h) const {
    return h >= 0 && h < next_id_ ? &storage_[h] : nullptr;
  }
  const T* Get(TReal h) const { return Get((THandle)h); }

 private:
  std::vector<T> storage_;
  std::queue<THandle> free_ids_;
  THandle next_id_;
};
