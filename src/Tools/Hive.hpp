#pragma once

#include <concepts>
#include <cstddef>
#include <memory>

template<typename T, std::size_t Size> class Hive {
  struct Colony {
    T elements[Size];
    std::size_t skipField[Size];
    Colony* next;
  };

  class iterator {
    using iterator_category = std::random_access_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = T;
    using pointer           = value_type*;
    using reference         = value_type&;

    iterator(const T* ptr) : element(ptr) {
      /* Compute elementIndex and colony. */
    }

    Colony* colony;
    std::size_t elementIndex;
    pointer element;

    reference operator*() const { return *element; }
    pointer operator->() { return element; }

    iterator operator++() { return *this; }
    iterator operator++(int) { return *this; }
    iterator operator--() { return *this; }
    iterator operator--(int) { return *this; }
    iterator operator+() { return *this; }
    iterator operator-() { return *this; }

    friend bool operator==(const iterator& a, const iterator& b) { return a.element == b.element; }
    friend bool operator!=(const iterator& a, const iterator& b) { return a.element != b.element; }

    iterator begin() { return iterator(firstColony, 0, &firstColony->elements[0]); }
    iterator end() { return iterator(lastColony, 0, &lastColony->elements[Size - 1]); }
  };

  Colony* firstColony{nullptr};
  Colony* lastColony{nullptr};
  iterator insertPosition{};

  template<typename... Args> requires std::constructible_from<T, Args...> iterator insert(Args&&... args) {
    std::construct_at(insertPosition.element, args...);
    return insertPosition++;
  }

  iterator insert(T element) {
    *insertPosition.element = std::move(element);
    return insertPosition++;
  }
};