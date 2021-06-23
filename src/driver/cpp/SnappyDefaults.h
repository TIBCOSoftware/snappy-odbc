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
 * SnappyDefines.h
 */

#ifndef SNAPPYDEFAULTS_H_
#define SNAPPYDEFAULTS_H_

#include "DriverBase.h"

using namespace io::snappydata;

namespace io {
namespace snappydata {

  class SnappyDefaults {
  public:
    static const SQLCHAR DEFAULT_DSN[];
    static const SQLWCHAR* DEFAULT_DSNW;
    static const char* ODBC_INI;
    static const char* ODBCINST_INI;

    static const int DEFAULT_SERVER_PORT;
  };

  /** typedef for SQLWCHAR strings like std::string */
  typedef std::basic_string<SQLWCHAR> sqlwstring;

  /** typedef for C string property map */
  typedef std::map<std::string, std::string> Properties;

  /**
   * Union for the different types of attributes in SQLSet*Attr methods.
   */
  struct AttributeValue {
    bool m_ascii;
    union {
      SQLULEN m_lenv;
      SQLUINTEGER m_intv;
      SQLPOINTER m_refv;
    } m_val;

    inline AttributeValue() : m_ascii(true), m_val { 0 } {
    }

    inline AttributeValue(bool isAscii) : m_ascii(isAscii), m_val { 0 } {
    }
  };

  /***
   * Typedef for the map holding currently defined attribute name/values.
   */
  typedef std::unordered_map<SQLINTEGER, AttributeValue> AttributeMap;

  /**
   * Function definition for SQLDriverToDataSource.
   */
  typedef BOOL (*DriverToDataSource)(UDWORD fOption, SWORD fSqlType,
      PTR rgbValueIn, SDWORD cbValueIn, PTR rgbValueOut,
      SDWORD cbValueOutMax, SDWORD * pcbValueOut, UCHAR * szErrorMsg,
      SWORD cbErrorMsgMax, SWORD * pcbErrorMsg);

  /**
   * Function definition for SQLDataSourceToDriver.
   */
  typedef BOOL (*DataSourceToDriver)(UDWORD fOption, SWORD fSqlType,
      PTR rgbValueIn, SDWORD cbValueIn, PTR rgbValueOut,
      SDWORD cbValueOutMax, SDWORD * pcbValueOut, UCHAR * szErrorMsg,
      SWORD cbErrorMsgMax, SWORD * pcbErrorMsg);

  /* Check if a paramater is a data-at-exec paramter.*/
  #define IS_DATA_AT_EXEC(X)((X) && \
                            (*(X) == SQL_DATA_AT_EXEC || \
                            *(X) <= SQL_LEN_DATA_AT_EXEC_OFFSET))

} /* namespace snappydata */
} /* namespace io */

#endif /* SNAPPYDEFAULTS_H_ */
