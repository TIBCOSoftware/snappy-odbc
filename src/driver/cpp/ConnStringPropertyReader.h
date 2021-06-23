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
 * ConnStringPropertyReader.h
 */

#ifndef CONNSTRINGPROPERTYREADER_H_
#define CONNSTRINGPROPERTYREADER_H_

#include <common/SystemProperties.h>

#include <boost/algorithm/string.hpp>

#include "IniPropertyReader.h"
#include "OdbcIniKeys.h"
#include "ClientAttribute.h"

namespace io {
namespace snappydata {
namespace impl {

  template<typename CHAR_TYPE>
  class ConnStringPropertyReader : public PropertyReader<CHAR_TYPE> {
  private:
    const CHAR_TYPE* m_connStr;
    const CHAR_TYPE* m_connStrp;
    bool m_newConnStr;
    /** for DSN attribute in the connection string */
    IniPropertyReader<CHAR_TYPE> m_iniReader;
    /** property names for {@link #m_iniReader} */
    ArrayIterator<std::string> *m_iniAllPropNames;
    /** set to true when properties using DSN are being read currently */
    bool m_readingDSN;

  public:
    inline ConnStringPropertyReader() :
        m_connStr(nullptr), m_connStrp(nullptr), m_newConnStr(false),
        m_iniReader(), m_iniAllPropNames(nullptr), m_readingDSN(false) {
    }

    virtual ~ConnStringPropertyReader() {
      if (m_newConnStr && m_connStr) {
        delete[] m_connStr;
      }
    }

  public:
    virtual void init(const CHAR_TYPE* connStr, SQLLEN connStrLen,
        void* propData) {
      if (connStrLen == SQL_NTS || connStr[connStrLen] == 0) {
        m_connStr = connStr;
        m_newConnStr = false;
      } else {
        CHAR_TYPE* newConnStr = new CHAR_TYPE[connStrLen + 1];
        ::memcpy(newConnStr, connStr, sizeof(CHAR_TYPE) * connStrLen);
        newConnStr[connStrLen] = 0;
        m_connStr = newConnStr;
        m_newConnStr = true;
      }
      m_connStrp = m_connStr;
      m_iniAllPropNames = (ArrayIterator<std::string>*)propData;
    }

    virtual SQLRETURN read(std::string& outPropName,
        std::string& outPropValue, SnappyHandleBase* handle) {
      // if reading DSN then return values using IniPropertyReader
      if (m_readingDSN) {
        SQLRETURN ret = m_iniReader.read(outPropName, outPropValue,
            handle);
        // check if all DSN properties have been read
        if (ret != SQL_SUCCESS) {
          if (ret != SQL_SUCCESS_WITH_INFO) {
            m_readingDSN = false;
          }
          return ret;
        }
        // continue to remaining values in the connection string
        m_readingDSN = false;
      }
      // split the string assuming it to be of the form
      // <key1>=<value1>;<key2>=<value2>...
      if (m_connStrp
          && StringFunctions::strlen(m_connStrp) != 0) {
        const CHAR_TYPE* semicolonPos = StringFunctions::strchr(
            m_connStrp, ';');
        const CHAR_TYPE* equalPos = StringFunctions::strchr(m_connStrp,
            '=');
        size_t outPropLen;

        if (equalPos) {
          // trim spaces
          const CHAR_TYPE* connStr = StringFunctions::ltrim(m_connStrp);
          equalPos = StringFunctions::rtrim(equalPos);
          // overload of toString below will convert UTF16 to UTF8 if required
          outPropName = std::move(
              StringFunctions::toString(connStr, equalPos - connStr));
        } else {
          // should be handled by DriverManager
          std::string connStr = StringFunctions::toString(m_connStr, SQL_NTS);
          handle->setException(GET_SQLEXCEPTION(
              SQLState::INVALID_CONNECTION_PROPERTY_VALUE,
              SQLStateMessage::INVALID_CONNECTION_PROPERTY_VALUE_MSG
                  .format(connStr.c_str(), "")));
          return SQL_ERROR;
        }
        equalPos++;
        if (semicolonPos) {
          outPropLen = semicolonPos - equalPos;
          m_connStrp = semicolonPos + 1;
        } else {
          outPropLen = StringFunctions::strlen(equalPos);
          m_connStrp = nullptr;
        }
        // copy the string after '=' (converting into UTF-8 for wchar)
        outPropValue = std::move(
            StringFunctions::toString(equalPos, outPropLen));

        // check for DSN attribute
        if (boost::iequals(outPropName, OdbcIniKeys::DSN) &&
            outPropValue.size() > 0) {
          m_iniReader.init(equalPos, outPropLen, m_iniAllPropNames);
          // call self again with m_readingDSN as true
          m_readingDSN = true;
          return read(outPropName, outPropValue, handle);
        }

        // indicates that more results are available
        return SQL_SUCCESS_WITH_INFO;
      }
      // indicates that no more results are available
      return SQL_SUCCESS;
    }

    const std::string& getDSN() const noexcept {
      return m_iniReader.getDSN();
    }
  };

  template<typename CHAR_TYPE>
  static SQLRETURN readProperties(PropertyReader<CHAR_TYPE>* reader,
      const CHAR_TYPE* inputRef, SQLINTEGER inputLen, void* propData,
      const CHAR_TYPE* userName, SQLINTEGER userNameLen,
      const CHAR_TYPE* password, SQLINTEGER passwordLen,
      std::string& outServer, int& outPort, Properties& connProps,
      SnappyHandleBase* handle) {
    std::string propName, connPropName;
    std::string propValue;
    std::string user, passwd;
    int flags;
    SQLRETURN result = SQL_SUCCESS;
    SQLRETURN ret;

    outPort = 0;
    // initialize user name and password attributes from passed ones
    if (userName) {
      user = std::move(StringFunctions::toString(userName, userNameLen));
      if (user.empty()) {
        return SnappyHandleBase::errorInvalidBufferLength(0,
            "UserName length", handle);
      }
    }
    if (password) {
      passwd = std::move(StringFunctions::toString(password, passwordLen));
    }
    // initialize the property reader
    reader->init(inputRef, inputLen, propData);
    // read the property names in a loop
    while ((ret = reader->read(propName, propValue, handle))
        == SQL_SUCCESS_WITH_INFO) {
      // lookup the mapping for this property name
      if (!OdbcIniKeys::getConnPropertyName(propName, connPropName, flags)) {
        // need to resort to case-insensitive matching against all
        // known names also support property names containing "-"
        boost::replace_all(propName,  "-", "");
        const OdbcIniKeys::KeyMap& keyMap = OdbcIniKeys::getKeyMap();
        bool foundProp = false;
        for (const auto& p : keyMap) {
          if (boost::iequals(propName, p.first)) {
            connPropName = p.second.getPropertyName();
            flags = p.second.getFlags();
            foundProp = true;
            break;
          }
        }
        if (!foundProp) {
          handle->setException(GET_SQLEXCEPTION2(
              SQLStateMessage::INVALID_CONNECTION_PROPERTY_MSG,
              propName.c_str()));
          result = SQL_SUCCESS_WITH_INFO;
          continue;
        }
      }
      // first check the "Driver" attribute
      if ((flags & ConnectionProperty::F_IS_DRIVER) != 0) {
        const auto propSize = propValue.size();
        if (propValue[0] == '{' && propValue[propSize - 1] == '}') {
          propValue = propValue.substr(1, propSize - 2);
        }
        if (propValue.find(ODBC_DRIVER_NAME) == std::string::npos &&
            !boost::istarts_with(propValue, ODBC_PRODUCT_NAME) &&
            !boost::istarts_with(propValue, ODBC_OLD_PRODUCT_NAME)) {
          handle->setException(
              GET_SQLEXCEPTION2(SQLStateMessage::INVALID_DRIVER_NAME_MSG,
                  propValue.c_str(), ODBC_PRODUCT_NAME));
          return SQL_ERROR;
        }
      // then check the "Server" attribute
      } else if ((flags & ConnectionProperty::F_IS_SERVER) != 0) {
        outServer = std::move(propValue);
      // then the "Port" attribute
      } else if ((flags & ConnectionProperty::F_IS_PORT) != 0) {
        char* endp = nullptr;
        outPort = ::strtol(propValue.c_str(), &endp, 10);
        if (!endp || *endp != 0) {
          handle->setException(GET_SQLEXCEPTION2(
              SQLStateMessage::INVALID_CONNECTION_PROPERTY_VALUE_MSG,
              propValue.c_str(), propName.c_str()));
          return SQL_ERROR;
        }
      // then the "User"/"UserName"/"UID" and "Password"/"PWD" attributes if
      // nothing has been passed
      } else if ((flags & ConnectionProperty::F_IS_USER) != 0) {
        if (user.empty()) {
          if (propValue.empty()) {
            return SnappyHandleBase::errorInvalidBufferLength(0,
                "UserName length", handle);
          }
          user = std::move(propValue);
        }
      } else if ((flags & ConnectionProperty::F_IS_PASSWD) != 0) {
        if (passwd.empty()) {
          passwd = std::move(propValue);
        }
      // next the remaining connection properties
      } else if ((flags & ConnectionProperty::F_IS_SYSTEM_PROP) == 0) {
        connProps[connPropName] = std::move(propValue);
      // lastly the system properties shared across all envs etc
      } else {
        SystemProperties::setProperty(connPropName, propValue);
      }
    } // end while

    if (ret != SQL_SUCCESS) {
      return ret;
    }
    if (outPort <= 0) {
      outPort = SnappyDefaults::DEFAULT_SERVER_PORT;
    }
    if (user.size() > 0) {
      connProps[ClientAttribute::USERNAME] = user;
    }
    if (passwd.size() > 0) {
      connProps[ClientAttribute::PASSWORD] = passwd;
    }
    return result;
  }

} /* namespace impl */
} /* namespace snappydata */
} /* namespace io */

#endif /* CONNSTRINGPROPERTYREADER_H_ */
