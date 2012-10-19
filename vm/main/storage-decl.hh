// Copyright © 2011, Université catholique de Louvain
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// *  Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// *  Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef __STORAGE_DECL_H
#define __STORAGE_DECL_H

#include "core-forward-decl.hh"

#include "type-decl.hh"
#include "memword.hh"
#include "arrays.hh"

#include <type_traits>

// Hacky helper (see below) - officially UB
// namespace std {
//   template<typename> struct has_trivial_destructor;
//   template<typename> struct is_trivially_destructible;
// }

namespace mozart {

/* First a hackish helper to support compilers and std libs which still use
 * has_trivial_destructor instead of is_trivially_destructible. */

// template<typename T>
// class have_cxx11_trait_helper {
// private:
//   template<typename T2, bool = std::is_trivially_destructible<T2>::value>
//   static std::true_type test(int);

//   template<typename T2, bool = std::has_trivial_destructor<T2>::value>
//   static std::false_type test(...);

// public:
//   typedef decltype(test<T>(0)) type;
// };

// template<typename T>
// struct have_cxx11_trait : have_cxx11_trait_helper<T>::type {
// };

// template<typename T>
// using is_trivially_destructible =
//   typename std::conditional<have_cxx11_trait<T>::value,
//                             std::is_trivially_destructible<T>,
//                             std::has_trivial_destructor<T>>::type;

// Now our stuff

template<typename T>
struct ImplAndCleanupListNode {
  template<class... Args>
  inline
  ImplAndCleanupListNode(VM vm, Args&&... args);

  T impl;
  VMCleanupListNode cleanupListNode;
};

template<typename T>
struct DerefPotentialImplAndCleanup {
  static T& deref(T& impl) {
    return impl;
  }
};

template<typename T>
struct DerefPotentialImplAndCleanup<ImplAndCleanupListNode<T>> {
  static T& deref(ImplAndCleanupListNode<T>& implAndCleanupListNode) {
    return implAndCleanupListNode.impl;
  }
};

template<class I, class E>
class ImplWithArray {
  I* p;
public:
  ImplWithArray(I* p) : p(p) {}

  I* operator->() {
    return p;
  }

  E& operator[](size_t i) {
    return getRawArray()[i];
  }

  StaticArray<E> getArray(size_t size) {
    return StaticArray<E>(getRawArray(), size);
  }
private:
  template <typename T, typename U>
  friend class AccessorHelper;

  E* getRawArray() {
    return static_cast<E*>(static_cast<void*>(
      static_cast<char*>(static_cast<void*>(p)) + sizeof(I)));
  }
};

// Marker class that specifies to use the default storage (pointer to value)
template<class T>
class DefaultStorage {
};

// Meta-function from Type to its storage
template<class T>
class Storage {
public:
  typedef DefaultStorage<T> Type;
};

template<class T, class U>
class AccessorHelper {
public:
  // StoredAs types must be trivially destructible
  static_assert(std::is_trivially_destructible<U>::value,
                "The type U in StoredAs<U> must be trivially destructible.");

  template<class... Args>
  static void init(Type& type, MemWord& value, VM vm, Args&&... args) {
    type = T::type();
    value.alloc<U>(vm);
    T::create(value.get<U>(), vm, std::forward<Args>(args)...);
  }

  static T get(MemWord value) {
    return T(value.get<U>());
  }
};

template<class T>
class AccessorHelper<T, DefaultStorage<T>> {
private:
  typedef typename std::conditional<
    std::is_trivially_destructible<T>::value,
    T, ImplAndCleanupListNode<T>>::type ActualT;
public:
  template<class... Args>
  static void init(Type& type, MemWord& value, VM vm, Args&&... args) {
    type = T::type();
    ActualT* val = new (vm) ActualT(vm, std::forward<Args>(args)...);
    value.init<ActualT*>(vm, val);
  }

  static T& get(MemWord value) {
    return DerefPotentialImplAndCleanup<ActualT>::deref(
      *(value.get<ActualT*>()));
  }
};

template<class T, class E>
class AccessorHelper<T, ImplWithArray<T, E>> {
public:
  // Limitation of the current implementation
  static_assert(
    std::is_trivially_destructible<T>::value && std::is_trivially_destructible<E>::value,
    "The types T and E in T: StoredWithArrayOfAs<U> must be trivially destructible.");

  template<class... Args>
  static void init(Type& type, MemWord& value, VM vm,
                   size_t elemCount, Args&&... args) {
    // Allocate memory
    void* memory = operator new (sizeof(T) + elemCount*sizeof(E), vm);
    ImplWithArray<T, E> implWithArray(static_cast<T*>(memory));

    // Initialize the array
    E* array = implWithArray.getRawArray();
    new (array) E[elemCount];

    // Initialize the impl
    T* impl = implWithArray.operator->();
    new (impl) T(vm, elemCount, implWithArray.getArray(elemCount),
                 std::forward<Args>(args)...);

    // Fill in output parameters
    type = T::type();
    value.init<T*>(vm, impl);
  }

  static T& get(MemWord value) {
    return *(value.get<T*>());
  }
};

template <typename T>
using Accessor = AccessorHelper<T, typename Storage<T>::Type>;

}

#endif // __STORAGE_DECL_H
