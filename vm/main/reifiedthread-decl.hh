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

#ifndef __REIFIEDTHREAD_DECL_H
#define __REIFIEDTHREAD_DECL_H

#include "mozartcore-decl.hh"

namespace mozart {

///////////////////
// ReifiedThread //
///////////////////

class ReifiedThread;

#ifndef MOZART_GENERATOR
#include "ReifiedThread-implem-decl.hh"
#endif

template <>
class Implementation<ReifiedThread>:
  StoredAs<Runnable*>, Copyable, WithValueBehavior {
public:
  typedef SelfType<ReifiedThread>::Self Self;
public:
  static atom_t getTypeAtom(VM vm) {
    return vm->getAtom(MOZART_STR("thread"));
  }

  Implementation(Runnable* runnable): _runnable(runnable) {}

  static void build(Runnable*& self, VM vm, Runnable* runnable) {
    self = runnable;
  }

  inline
  static void build(Runnable*& self, VM vm, GR gr, Self from);

public:
  inline
  bool equals(VM vm, Self right);

public:
  Runnable* value() {
    return _runnable;
  }

public:
  // Wakeable interface

  inline
  OpResult wakeUp(VM vm);

  inline
  bool shouldWakeUpUnderSpace(VM vm, Space* space);

public:
  // ThreadLike interface

  OpResult isThread(Self self, VM vm, bool& result) {
    result = true;
    return OpResult::proceed();
  }

  inline
  OpResult getThreadPriority(VM vm, ThreadPriority& result);

  inline
  OpResult setThreadPriority(VM vm, ThreadPriority priority);

private:
  Runnable* _runnable;
};

#ifndef MOZART_GENERATOR
#include "ReifiedThread-implem-decl-after.hh"
#endif

}

#endif // __REIFIEDTHREAD_DECL_H
