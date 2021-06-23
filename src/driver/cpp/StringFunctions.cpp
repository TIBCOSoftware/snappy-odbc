/*
 * Copyright (c) 2010-2015 Pivotal Software, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License. You
 * may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License. See accompanying
 * LICENSE file.
 */
/*
 * Changes for SnappyData data platform.
 *
 * Portions Copyright (c) 2018 SnappyData, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License. You
 * may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License. See accompanying
 * LICENSE file.
 */

/**
 * StringFunctions.cpp
 *
 * Various utility functions for string manipulation.
 */

#include "StringFunctions.h"
#include "Utils.h"

#include <algorithm>

using namespace io::snappydata;

namespace _snappy_impl {

static bool copyString(const void *chars, const size_t len, void *outStr,
    SQLLEN outMaxLen, SQLLEN *totalLen, int charSize) {
  SQLLEN maxLen = StringFunctions::restrictLength<SQLLEN, size_t>(len);
  if (totalLen) *totalLen = maxLen;
  if (--outMaxLen >= maxLen) {
    size_t size = maxLen * charSize;
    ::memcpy(outStr, chars, size);
    ::memset(((char*)outStr) + size, 0, charSize); // null termination
    return false;
  } else if (outMaxLen >= 0) {
    size_t size = outMaxLen * charSize;
    ::memcpy(outStr, chars, size);
    ::memset(((char*)outStr) + size, 0, charSize); // null termination
    return true;
  } else {
    return true;
  }
}

static std::string toString(const SQLWCHAR *chars, const SQLLEN len) {
  std::string result;
  if (len != SQL_NTS) {
    // reserve some length to reduce reallocations (for best case ASCII)
    result.reserve(len);
  }
  client::Utils::convertUTF16ToUTF8(chars, len, [&](char c) {
    result.push_back(c);
  });
  return result;
}

}

bool StringFunctions::copyString(const SQLCHAR *chars, SQLLEN len,
    SQLCHAR *outStr, SQLLEN outMaxLen, SQLLEN *totalLen) {
  if (chars) {
    if (len == SQL_NTS) len = restrictLength<SQLLEN, size_t>(strlen(chars));
    return _snappy_impl::copyString(chars, len, outStr, outMaxLen, totalLen, 1);
  } else {
    if (totalLen) *totalLen = SQL_NULL_DATA;
    return false;
  }
}

bool StringFunctions::copyString(const SQLWCHAR *chars, SQLLEN len,
    SQLWCHAR *outStr, SQLLEN outMaxLen, SQLLEN *totalLen) {
  if (chars) {
    if (len == SQL_NTS) len = restrictLength<SQLLEN, size_t>(strlen(chars));
    return _snappy_impl::copyString(chars, len, outStr, outMaxLen, totalLen, 2);
  } else {
    if (totalLen) *totalLen = SQL_NULL_DATA;
    return false;
  }
}

bool StringFunctions::copyString(const SQLCHAR *chars, SQLLEN len,
    SQLWCHAR *outStr, SQLLEN outMaxLen, SQLLEN *totalLen) {
  // It is safer and faster in most cases to do the full conversion in one pass
  // that will avoid any checks for outMaxLen and totalLen for each character.
  // The truncation case should be an uncommon one.
  if (chars) {
    std::u16string result;
    if (len != SQL_NTS) {
      // reserve some length to reduce reallocations (for best case ASCII)
      result.reserve(len);
    }
    client::Utils::convertUTF8ToUTF16((const char*)chars, len, [&](int c) {
      result.push_back(static_cast<char16_t>(c));
    });
    return _snappy_impl::copyString(result.c_str(), result.length(),
        outStr, outMaxLen, totalLen, 2);
  } else {
    if (totalLen) *totalLen = SQL_NULL_DATA;
    return false;
  }
}

bool StringFunctions::copyString(const SQLWCHAR *chars, SQLLEN len,
    SQLCHAR *outStr, SQLLEN outMaxLen, SQLLEN *totalLen) {
  // It is safer and faster in most cases to do the full conversion in one pass
  // that will avoid any checks for outMaxLen and totalLen for each character.
  // The truncation case should be an uncommon one.
  if (chars) {
    // compiler should optimize to avoid any copy or move, so no rvalue below
    std::string result = _snappy_impl::toString(chars, len);
    return _snappy_impl::copyString(result.c_str(), result.length(),
        outStr, outMaxLen, totalLen, 1);
  } else {
    if (totalLen) *totalLen = SQL_NULL_DATA;
    return false;
  }
}

std::string StringFunctions::toString(const SQLWCHAR *chars, const SQLLEN len) {
  return chars ? _snappy_impl::toString(chars, len) : std::string();
}

void StringFunctions::split(const SQLCHAR* str, SQLLEN len, char delimiter,
    std::vector<std::string>& result) {
  if (str) {
    const SQLCHAR* splitStart = str;
    size_t splitSize = 0;
    SQLCHAR c = *str;
    while (len != 0 && c != 0) {
      if (c != delimiter) {
        splitSize++;
      } else {
        result.emplace_back((const char*)splitStart, splitSize);
        splitStart += (splitSize + 1);
        splitSize = 0;
      }
      c = *(++str);
      if (len > 0) len--;
    }
    if (splitSize > 0) {
      result.emplace_back((const char*)splitStart, splitSize);
    } else {
      result.emplace_back();
    }
  }
}

void StringFunctions::split(const SQLWCHAR* str, SQLLEN len, wchar_t delimiter,
    std::vector<std::string>& result) {
  if (str) {
    std::string buffer;
    SQLWCHAR c = *str;
    while (len != 0 && c != 0) {
      if (c != delimiter) {
        // no need for conversions since table type is always an ASCII string
        buffer.push_back((char)c);
      } else {
        result.push_back(std::move(buffer));
        buffer.resize(0);
      }
      c = *(++str);
      if (len > 0) len--;
    }
    result.push_back(std::move(buffer));
  }
}
