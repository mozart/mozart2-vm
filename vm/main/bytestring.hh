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

#ifndef __BYTESTRING_H
#define __BYTESTRING_H

#include <string>
#include "mozartcore.hh"

#ifndef MOZART_GENERATOR

namespace mozart {

////////////////
// ByteString //
////////////////

#include "ByteString-implem.hh"

// Core methods ----------------------------------------------------------------

ByteString::ByteString(VM vm, GR gr, Self from)
  : _bytes(vm, from->_bytes) {}

bool ByteString::equals(VM vm, Self right) {
  return _bytes == right->_bytes;
}

UnstableNode ByteString::getValueAt(Self self, VM vm, nativeint feature) {
  return mozart::build(vm, _bytes[feature]);
}

namespace internal {
  inline
  int compareByteStrings(LString<unsigned char> left,
                         LString<unsigned char> right) {
    int cmpRes = left.string != right.string;
    if (cmpRes) {
      size_t minLength = std::min(left.bytesCount(), right.bytesCount());
      cmpRes = memcmp(left.string, right.string, minLength);
    }
    if (!cmpRes) {
      return left.length < right.length ? -1 :
             left.length > right.length ? 1 : 0;
    } else {
      return cmpRes;
    }
  }
}

int ByteString::compare(Self self, VM vm, RichNode right) {
  return internal::compareByteStrings(_bytes,
                                      *StringLike(right).byteStringGet(vm));
}

// StringLike ------------------------------------------------------------------

LString<unsigned char>* ByteString::byteStringGet(Self self, VM vm) {
  return &_bytes;
}

LString<nchar>* ByteString::stringGet(Self self, VM vm) {
  raiseTypeError(vm, MOZART_STR("String"), self);
}

nativeint ByteString::stringCharAt(Self self, VM vm, RichNode offsetNode) {
  auto offset = getArgument<nativeint>(vm, offsetNode, MOZART_STR("integer"));

  if (offset < 0 || offset >= _bytes.length)
    raiseIndexOutOfBounds(vm, offsetNode, self);

  return _bytes[offset];
}

UnstableNode ByteString::stringAppend(Self self, VM vm, RichNode right) {
  auto rightBytes = StringLike(right).byteStringGet(vm);
  LString<unsigned char> resultBytes = concatLString(vm, _bytes, *rightBytes);
  if (resultBytes.isError())
    raiseUnicodeError(vm, resultBytes.error, self, right);
  return ByteString::build(vm, resultBytes);
}

UnstableNode ByteString::stringSlice(Self self, VM vm,
                                     RichNode from, RichNode to) {
  auto fromOffset = getArgument<nativeint>(vm, from, MOZART_STR("integer"));
  auto toOffset = getArgument<nativeint>(vm, to, MOZART_STR("integer"));

  if (fromOffset < 0 || fromOffset > toOffset || toOffset > _bytes.length)
    raiseIndexOutOfBounds(vm, fromOffset, toOffset);

  return ByteString::build(vm, _bytes.slice(fromOffset, toOffset));
}

void ByteString::stringSearch(
  Self self, VM vm, RichNode from, RichNode needleNode,
  UnstableNode& begin, UnstableNode& end) {
/*
  using namespace patternmatching;

  auto fromOffset = getArgument<nativeint>(vm, from, MOZART_STR("integer"));

  if (fromOffset < 0 || fromOffset > _bytes.length)
    raiseIndexOutOfBounds(vm, fromOffset);

  LString<unsigned char> haystack = _bytes.slice(fromOffset);

  nativeint character = 0;

  if (matches(vm, needleNode, capture(character))) {

    if (character < 0 || character >= 0x100)
      raiseTypeError(vm, MOZART_STR("Integer between 0 and 255"), needleNode);

    auto haystackUnsafe = const_cast<unsigned char*>(haystack.string);
    const void* searchRes = memchr(haystackUnsafe, character,
                                   haystack.bytesCount());
    if (searchRes == nullptr) {
      begin = Boolean::build(vm, false);
      end = Boolean::build(vm, false);
    } else {
      nativeint foundOffset =
        static_cast<const unsigned char*>(searchRes) - _bytes.string;
      begin = SmallInt::build(vm, foundOffset);
      end = SmallInt::build(vm, foundOffset + 1);
    }

  } else {

    auto needle = StringLike(needleNode).byteStringGet(vm);
    auto foundIter = std::search(haystack.begin(), haystack.end(),
                                 needle->begin(), needle->end());
    if (foundIter == haystack.end()) {
      begin = Boolean::build(vm, false);
      end = Boolean::build(vm, false);
    } else {
      nativeint foundOffset = foundIter - _bytes.string;
      begin = SmallInt::build(vm, foundOffset);
      end = SmallInt::build(vm, foundOffset + needle->length);
    }

  }
  */
}

bool ByteString::stringHasPrefix(Self self, VM vm, RichNode prefixNode) {
  auto prefix = StringLike(prefixNode).stringGet(vm);
  if (_bytes.length < prefix->length)
    return false;
  else
    return memcmp(_bytes.string, prefix->string, prefix->bytesCount()) == 0;
}

bool ByteString::stringHasSuffix(Self self, VM vm, RichNode suffixNode) {
  auto suffix = StringLike(suffixNode).stringGet(vm);
  if (_bytes.length < suffix->length)
    return false;
  else
    return memcmp(_bytes.end() - suffix->length, suffix->string,
                  suffix->bytesCount()) == 0;
}

// VirtualString ---------------------------------------------------------------

void ByteString::toString(Self self, VM vm,
                          std::basic_ostream<nchar>& sink) {
  sink << decodeLatin1(_bytes, EncodingVariant::none);
}

nativeint ByteString::vsLength(Self self, VM vm) {
  return _bytes.length;
}

// Encode & decode -------------------------------------------------------------

UnstableNode ByteString::decode(Self self, VM vm, ByteStringEncoding encoding,
                                EncodingVariant variant) {
  DecoderFun decoder;
  switch (encoding) {
    case ByteStringEncoding::latin1: decoder = &decodeLatin1; break;
    case ByteStringEncoding::utf8:   decoder = &decodeUTF8;   break;
    case ByteStringEncoding::utf16:  decoder = &decodeUTF16;  break;
    case ByteStringEncoding::utf32:  decoder = &decodeUTF32;  break;
    default:
      assert(false);
      decoder = nullptr;
  }

  auto res = newLString(vm, decoder(_bytes, variant));
  if (res.isError())
    raiseUnicodeError(vm, res.error);
  return String::build(vm, res);
}

UnstableNode encodeToBytestring(VM vm, const BaseLString<nchar>& input,
                                ByteStringEncoding encoding,
                                EncodingVariant variant) {
  EncoderFun encoder;
  switch (encoding) {
    case ByteStringEncoding::latin1: encoder = &encodeLatin1; break;
    case ByteStringEncoding::utf8:   encoder = &encodeUTF8;   break;
    case ByteStringEncoding::utf16:  encoder = &encodeUTF16;  break;
    case ByteStringEncoding::utf32:  encoder = &encodeUTF32;  break;
    default:
      assert(false);
      encoder = nullptr;
  }

  auto res = newLString(vm, encoder(input, variant));
  if (res.isError())
    raiseUnicodeError(vm, res.error);
  return ByteString::build(vm, res);
}

// Miscellaneous ---------------------------------------------------------------

void ByteString::printReprToStream(Self self, VM vm, std::ostream& out,
                                   int depth) {
  out << "<ByteString \"";
  out.write(reinterpret_cast<const char*>(_bytes.string), _bytes.length);
  // TODO: Escape characters.
  out << "\">";
}

}

#endif // MOZART_GENERATOR

#endif // __BYTESTRING_H
