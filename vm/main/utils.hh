// Copyright © 2012, Université catholique de Louvain
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

#ifndef __UTILS_H
#define __UTILS_H

#include "mozart.hh"

#include <type_traits>

#ifndef MOZART_GENERATOR

namespace mozart {

// getArgument -----------------------------------------------------------------

template <class T>
atom_t PrimitiveTypeToExpectedAtom<T>::result(VM vm) {
  typedef typename patternmatching::PrimitiveTypeToOzType<T>::result OzType;
  return OzType::type()->getTypeAtom(vm);
}

template <class T>
T getArgument(VM vm, RichNode argValue, const nchar* expectedType) {
  using namespace patternmatching;

  T result;

  if (!matches(vm, argValue, capture(result)))
    raiseTypeError(vm, expectedType, argValue);

  return result;
}

template <class T>
T getArgument(VM vm, RichNode argValue) {
  using namespace patternmatching;

  T result;

  if (!matches(vm, argValue, capture(result))) {
    atom_t expected = PrimitiveTypeToExpectedAtom<T>::result(vm);
    raiseTypeError(vm, expected, argValue);
  }

  return result;
}

/* Important note to anyone thinking that getPointerArgument should return a
 * shared_ptr.
 *
 * It is *intentional* that we break the "safety" of shared_ptr by returning
 * the raw pointer here.
 *
 * The use case for getPointerArgument is, in a VM-called function, to extract
 * the pointer from a ForeignPointer.
 *
 * A first important observation is that code further in the method can throw a
 * Mozart exception (waiting, raise, etc.) for any kind of reason. Now,
 * Mozart exceptions are implemented with the setjmp/longjmp, which break
 * through all C++ try-catches in the stack, including the destructor of the
 * potential shared_ptr.
 * This would cause a memory leak!
 *
 * The other important observation is that the method will end before any
 * garbage collection can be performed. And only garbage collection can delete
 * the shared_ptr inside the ForeignPointer. Hence, its is already guaranteed
 * that the reference count will not reach 0 before the calling method ends and
 * stops using the returning pointer.
 *
 * Conclusion: we don't need to return a shared_ptr, and doing so would
 * actually cause *memory leaks* in some (non-rare) cases.
 */

template <class T>
T* getPointerArgument(VM vm, RichNode argValue, const nchar* expectedType) {
  return getArgument<std::shared_ptr<T>>(vm, argValue, expectedType).get();
}

template <class T>
T* getPointerArgument(VM vm, RichNode argValue) {
  return getArgument<std::shared_ptr<T>>(vm, argValue).get();
}

// requireFeature --------------------------------------------------------------

void requireFeature(VM vm, RichNode feature) {
  if (!feature.isFeature())
    PotentialFeature(feature).makeFeature(vm);
}

// OzListBuilder ---------------------------------------------------------------

OzListBuilder::OzListBuilder(VM vm) {
  _head.init(vm);
  _tail = &_head;
}

template <class T>
void OzListBuilder::push_front(VM vm, T&& value) {
  _head = buildCons(vm, std::forward<T>(value), std::move(_head));
  if (_tail == &_head) {
    _tail = RichNode(_head).as<Cons>().getTail();
  }
}

template <class T>
void OzListBuilder::push_back(VM vm, T&& value) {
  auto cons = buildCons(vm, std::forward<T>(value), unit);
  auto newTail = RichNode(cons).as<Cons>().getTail();
  _tail.fill(vm, std::move(cons));
  _tail = newTail;
}

UnstableNode OzListBuilder::get(VM vm) {
  _tail.fill(vm, buildNil(vm));
  return std::move(_head);
}

// ozListForEach ---------------------------------------------------------------

template <class F, class G>
void ozListForEach(VM vm, RichNode cons, const F& onHead, const G& onTail) {
  using namespace patternmatching;

  typename std::remove_reference<
    typename function_traits<F>::template arg<0>::type>::type head;

  UnstableNode tail;

  while (true) {
    if (matchesCons(vm, cons, capture(head), capture(tail))) {
      onHead(head);
      cons = tail;
    } else if (matches(vm, cons, vm->coreatoms.nil)) {
      return;
    } else {
      return onTail(cons);
    }
  }
}

template <class F>
void ozListForEach(VM vm, RichNode cons, const F& onHead,
                   const nchar* expectedType) {
  return ozListForEach(vm, cons, onHead, [=](RichNode node) {
    return raiseTypeError(vm, expectedType, node);
  });
}

size_t ozListLength(VM vm, RichNode list) {
  size_t result = 0;

  UnstableNode nextList;

  while (true) {
    using namespace patternmatching;

    UnstableNode tail;

    if (matchesCons(vm, list, wildcard(), capture(tail))) {
      result++;
      nextList = std::move(tail);
      list = nextList;
    } else if (matches(vm, list, vm->coreatoms.nil)) {
      return result;
    } else {
      raiseTypeError(vm, MOZART_STR("list"), list);
    }
  }
}

namespace internal {
  template <class C>
  struct VSToStringHelper {
    inline
    static std::basic_string<C> call(VM vm, RichNode vs);
  };

  template <>
  struct VSToStringHelper<nchar> {
    static std::basic_string<nchar> call(VM vm, RichNode vs) {
      std::basic_string<nchar> x;
      return x;
      // std::basic_stringstream<nchar> buffer;
      // VirtualString(vs).toString(vm, buffer);
      // return buffer.str();
    }
  };

  template <class C>
  std::basic_string<C> VSToStringHelper<C>::call(VM vm, RichNode vs) {
    auto nresult = VSToStringHelper<nchar>::call(vm, vs);
    auto str = toUTF<C>(makeLString(nresult.c_str(), nresult.size()));
    return std::basic_string<C>(str.string, str.length);
  }
}

template <class C>
std::basic_string<C> vsToString(VM vm, RichNode vs) {
  return internal::VSToStringHelper<C>::call(vm, vs);
}

}

#endif

#endif // __UTILS_H
