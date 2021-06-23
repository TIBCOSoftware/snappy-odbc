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
 * IniPropertyReader.h
 */

#ifndef INIPROPERTYREADER_H_
#define INIPROPERTYREADER_H_

#include "PropertyReader.h"
#include "ArrayIterator.h"
#include "SnappyDefaults.h"
#include "StringFunctions.h"

extern "C" {
#include <odbcinst.h>
}

namespace io {
namespace snappydata {
namespace impl {

  template<typename CHAR_TYPE>
  class IniPropertyReader final : public PropertyReader<CHAR_TYPE> {
  private:
    std::string m_dsn;
    ArrayIterator<std::string>* m_propData;

  public:
    inline IniPropertyReader() : m_dsn(), m_propData(nullptr) {
    }

    inline ~IniPropertyReader() {
    }

    void init(const CHAR_TYPE* dsn, SQLLEN dsnLen, void* propData) {
      m_dsn = std::move(StringFunctions::toString(dsn, dsnLen));
      m_propData = (ArrayIterator<std::string>*)propData;
    }

    SQLRETURN read(std::string& outPropName, std::string& outPropValue,
        SnappyHandleBase* handle) {
      // void* is taken to be the iterator on all property names
      // (or those present in the ini file)
      ArrayIterator<std::string> &iter = *m_propData;
      const char* dsnStr = m_dsn.c_str();
      if (iter.hasCurrent()) {
        char outProp[8192];
        int len;
        for (;;) {
          outPropName = *iter;
          len = ::SQLGetPrivateProfileString(dsnStr, outPropName.c_str(), "",
              outProp, 8192, SnappyDefaults::ODBC_INI);
          if (len > 0) {
            // got a property declared in ini file
            outPropValue.assign(outProp, len);
            ++iter;
            // indicates that more results are available
            return SQL_SUCCESS_WITH_INFO;
          }
          if (!++iter) {
            // indicates that no more results are available
            return SQL_SUCCESS;
          }
        }
      }
      // indicates that no more results are available
      return SQL_SUCCESS;
    }

    const std::string& getDSN() const noexcept {
      return m_dsn;
    }
  };

} /* namespace impl */
} /* namespace snappydata */
} /* namespace io */

#endif /* INIPROPERTYREADER_H_ */
