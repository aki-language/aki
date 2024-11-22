#pragma once

#include <list>
#include <stack>

#include "base/compiler.h"

template <typename T>
class FIFOStack {
 private:
  std::list<T> list;

 public:
  void enqueue(const T& value) { list.push_back(value); }

  T dequeue() {
    if (list.empty()) {
      // throw std::out_of_range("Queue is empty");
    }

    T frontValue = list.front();
    list.pop_front();

    return frontValue;
  }

  T peek() {
    if (list.empty()) {
      CHECK_BREAK;
      // throw std::out_of_range("Queue is empty");
    }

    return list.front();
  }

  T peek_back(size_t index) {
    if (index >= size()) {
      CHECK_BREAK;
      // throw std::out_of_range("Index out of range");
    }

    auto iter = list.rbegin();
    std::advance(iter, index);

    return *iter;
  }

  T front() {
    if (list.empty()) {
      // throw std::out_of_range("Queue is empty");
      DEBUG_TRAP;
    }

    return list.front();
  }

  bool empty() const { return list.empty(); }

  size_t size() const { return list.size(); }
};
