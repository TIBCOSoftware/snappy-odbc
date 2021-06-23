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
 * StringFunctions.h
 *
 * Various utility functions for string manipulation.
 */

#ifndef STRINGFUNCTIONS_H_
#define STRINGFUNCTIONS_H_

#include "OdbcBase.h"

extern "C" {
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <wctype.h>

#include <assert.h>
}

#include <algorithm>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>

namespace io {
namespace snappydata {

/**
 * Static class containing various string manipulation functions.
 */
class StringFunctions final {
private:
  StringFunctions() = delete; // no instance

public:
  /**
   * Reduce the given length parameter to the maximum allowed for 'TLEN' type.
   * The type of input value 'TIN' is assumed to be wider or same as 'TLEN'.
   */
  template <typename TLEN, typename TIN>
  static TLEN restrictLength(TIN len) noexcept {
    return std::is_same<TLEN, TIN>::value ? static_cast<TLEN>(len) : (len >= 0
        ? static_cast<TLEN>(std::min(static_cast<TIN>(
            std::numeric_limits<TLEN>::max()), len))
        : static_cast<TLEN>(std::max(static_cast<TIN>(
            std::numeric_limits<TLEN>::min()), len)));
  }

  /**
   * copy given string upto given max size of the output buffer;
   * return true if the output string was truncated; also returns the total
   * length of encoded string if the provided "totalLen" param is non-null
   */
  static bool copyString(const SQLCHAR *chars, SQLLEN len, SQLCHAR *outStr,
      SQLLEN outMaxLen, SQLLEN *totalLen);

  /**
   * copy given wide-string upto given max size of the output buffer;
   * return true if the output string was truncated; also returns the total
   * length of encoded string if the provided "totalLen" param is non-null
   */
  static bool copyString(const SQLWCHAR *chars, SQLLEN len, SQLWCHAR *outStr,
      SQLLEN outMaxLen, SQLLEN *totalLen);

  /**
   * convert given UTF8 encoded string to wide-characted string upto
   * given max size of the output buffer; return the size of output string
   * excluding the terminating null
   */
  static bool copyString(const SQLCHAR *chars, SQLLEN len,
      SQLWCHAR *outStr, SQLLEN outMaxLen, SQLLEN *totalLen);

  /**
   * convert given wide-characted string to UTF8 encoded string upto
   * given max size of the output buffer; return true if the output string
   * was truncated; also returns the total length of encoded string if the
   * provided "totalLen" param is non-null
   */
  static bool copyString(const SQLWCHAR *chars, SQLLEN len,
      SQLCHAR *outStr, SQLLEN outMaxLen, SQLLEN *totalLen);

  /**
   * return a string for given C string (interprets SQL_NTS)
   */
  inline static std::string toString(const SQLCHAR *chars, const SQLLEN len) {
    char *cs = (char*)chars;
    return cs ? (len == SQL_NTS ? std::string(cs) : std::string(cs, len))
        : std::string();
  }

  /**
   * return UTF8 encoded string for a given wide-charactered string
   */
  static std::string toString(const SQLWCHAR *chars, const SQLLEN len);

  template<typename CHAR_TYPE>
  static size_t strlen(const CHAR_TYPE* str) {
    const CHAR_TYPE* strp = str;
    while (*strp) {
      strp++;
    }
    return (strp - str);
  }

  inline static size_t strlen(const SQLCHAR* str) {
    return ::strlen((const char*)str);
  }

  inline static size_t strlen(const char* str) {
    return ::strlen(str);
  }

  template<typename CHAR_TYPE, typename CHAR_TYPE2>
  static int strcmp(const CHAR_TYPE* str1, int str1Len,
      const CHAR_TYPE2* str2, int str2Len) {
    CHAR_TYPE ch1;
    CHAR_TYPE2 ch2;
    while (str1Len-- > 0) {
      if (str2Len-- <= 0) {
        return 1;
      } else if ((ch1 = *str1) != (ch2 = *str2)) {
        return (ch1 - ch2);
      }
      str1++, str2++;
    }
    return (str2Len == 0) ? 0 : -1;
  }

  template<typename CHAR_TYPE, typename CHAR_TYPE2>
  static int strncmp(const CHAR_TYPE* str1, const CHAR_TYPE2* str2,
      int strLen) {
    CHAR_TYPE ch1;
    CHAR_TYPE2 ch2;
    while (strLen-- > 0) {
      if ((ch2 = *str2) == 0) {
        return 1;
      } else if ((ch1 = *str1) != ch2) {
        return (ch1 - ch2);
      }
      str1++, str2++;
    }
    return (strLen == 0) ? 0 : -1;
  }

  template<typename CHAR_TYPE>
  static const CHAR_TYPE* strchr(const CHAR_TYPE* s, const char c) {
    CHAR_TYPE ch;
    while ((ch = *s)) {
      if (ch == c) {
        return s;
      }
      s++;
    }
    // check for 0 itself being searched
    if (ch == c) {
      return s;
    }
    return nullptr;
  }

  /**
   * Return the starting pointer after skipping leading blanks and tabs.
   */
  template<typename CHAR_TYPE>
  static const CHAR_TYPE* ltrim(const CHAR_TYPE* startp) {
    CHAR_TYPE ch;
    if (sizeof(CHAR_TYPE) == 1) {
      while ((ch = *startp) == ' ' || ch == '\t') {
        startp++;
      }
    } else {
      while ((ch = *startp) == L' ' || ch == L'\t') {
        startp++;
      }
    }
    return startp;
  }

  /**
   * Return the ending pointer after skipping trailing blanks and tabs.
   */
  template<typename CHAR_TYPE>
  static const CHAR_TYPE* rtrim(const CHAR_TYPE* endp) {
    CHAR_TYPE ch;
    if (sizeof(CHAR_TYPE) == 1) {
      while ((ch = *endp) == ' ' || ch == '\t') {
        endp--;
      }
    } else {
      while ((ch = *endp) == L' ' || ch == L'\t') {
        endp--;
      }
    }
    return endp;
  }

  static void split(const SQLCHAR* str, SQLLEN len, char delimiter,
      std::vector<std::string>& result);
  static void split(const SQLWCHAR* str, SQLLEN len, wchar_t delimiter,
      std::vector<std::string>& result);
};

} /* namespace snappydata */
} /* namespace io */

#endif /* STRINGFUNCTIONS_H_ */
