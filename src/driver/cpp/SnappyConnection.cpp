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
 * SnappyConnection.cpp
 *
 *  Contains implementation of setup and connection ODBC API.
 */

#include <ClientProperty.h>

#include "SnappyEnvironment.h"
#include "SnappyConnection.h"
#include "StringFunctions.h"
#include "ArrayIterator.h"
#include "Library.h"
#include "ConnStringPropertyReader.h"
#include "OdbcIniKeys.h"

using namespace io::snappydata;

namespace io {
namespace snappydata {
  void getIntValue(SQLULEN value, SQLPOINTER resultValueBuffer,
    SQLINTEGER* valueLen, bool isUnsigned) {
    if (isUnsigned) {
      if (resultValueBuffer) *(SQLULEN*)resultValueBuffer = value;
      if (valueLen) *valueLen = sizeof(SQLULEN);
    } else {
      if (resultValueBuffer) {
        *(SQLLEN*)resultValueBuffer = (SQLLEN)value;
      }
      if (valueLen) *valueLen = sizeof(SQLLEN);
    }
  }
}
}

SQLRETURN SnappyConnection::newConnection(SnappyEnvironment *env,
    SnappyConnection*& connRef) {
  if (env) {
    try {
      connRef = new SnappyConnection(env);
      return SQL_SUCCESS;
    } catch (SQLException& sqle) {
      SnappyHandleBase::setGlobalException(sqle);
      return SQL_ERROR;
    } catch (std::exception& se) {
      SnappyHandleBase::setGlobalException(__FILE__, __LINE__, se);
      return SQL_ERROR;
    }
  } else {
    return SnappyHandleBase::errorNullHandle(SQL_HANDLE_ENV);
  }
}

SQLRETURN SnappyConnection::freeConnection(SnappyConnection* conn) {
  if (conn) {
    if (!conn->isActive()) {
      SQLSMALLINT result = conn->m_env->removeActiveConnection(conn);
      if (result == SQL_SUCCESS || result == SQL_NO_DATA) {
        clearLastGlobalError();
        delete conn;
        return !lastGlobalError() ? SQL_SUCCESS : SQL_ERROR;
      } else {
        return result;
      }
    } else {
      conn->setException(GET_SQLEXCEPTION2(
          SQLStateMessage::FUNCTION_SEQUENCE_ERROR_MSG,
          "SQLFreeHandle(SQL_HANDLE_DBC) invoked before SQLDisconnect"));
      return SQL_ERROR;
    }
  } else {
    return SnappyHandleBase::errorNullHandle(SQL_HANDLE_DBC);
  }
}

SnappyConnection::SnappyConnection(SnappyEnvironment* env):
    m_conn(), m_env(env), m_attributes(), m_argsAsIdentifiers(false),
    m_hwnd(nullptr), m_translateOption(0), m_translationLibrary(nullptr),
    m_dataSourceToDriver(nullptr), m_driverToDataSource(nullptr) {
  env->addNewActiveConnection(this);
}

SnappyConnection::~SnappyConnection() {
  // delete any string attributes
  for (AttributeMap::const_iterator iter = m_attributes.begin();
      iter != m_attributes.end(); ++iter) {
    switch (iter->first) {
      case SQL_ATTR_CURRENT_CATALOG:
      case SQL_ATTR_TRANSLATE_LIB:
        if (iter->second.m_ascii) {
          delete[] (SQLCHAR*)iter->second.m_val.m_refv;
        } else {
          delete[] (SQLWCHAR*)iter->second.m_val.m_refv;
        }
        break;
    }
  }
  if (m_translationLibrary) {
    delete m_translationLibrary;
    m_translationLibrary = nullptr;
  }
  // destructor should never throw an exception
  try {
    m_conn.close();
  } catch (SQLException& sqle) {
    SnappyHandleBase::setGlobalException(sqle);
  } catch (std::exception& se) {
    SnappyHandleBase::setGlobalException(__FILE__, __LINE__, se);
  } catch (...) {
    std::runtime_error err("Unknown exception in connection close");
    SnappyHandleBase::setGlobalException(__FILE__, __LINE__, err);
  }
}

template<typename CHAR_TYPE, typename CHAR_TYPE2>
SQLRETURN SnappyConnection::getStringValue(const CHAR_TYPE *str,
    const SQLLEN len, CHAR_TYPE2 *resultValue, SQLLEN bufferLength,
    SQLINTEGER *stringLengthPtr, const char *op) {
  SQLRETURN result = SQL_SUCCESS;
  if (resultValue) {
    SQLLEN stringLen;
    if (StringFunctions::copyString(str, len, resultValue, bufferLength,
        &stringLen)) {
      result = SQL_SUCCESS_WITH_INFO;
      setException(GET_SQLEXCEPTION2(
          SQLStateMessage::STRING_TRUNCATED_MSG, op, bufferLength - 1));
    }
    if (stringLengthPtr) {
      *stringLengthPtr = StringFunctions::restrictLength<SQLINTEGER, SQLLEN>(
          stringLen);
    }
  } else if (stringLengthPtr) {
    *stringLengthPtr = StringFunctions::restrictLength<SQLINTEGER, size_t>(
        StringFunctions::strlen(str));
  }
  return result;
}

template<typename CHAR_TYPE>
SQLRETURN SnappyConnection::connectT(const std::string& server, const int port,
    const Properties& connProps, CHAR_TYPE* outConnStr,
    const SQLINTEGER outConnStrLen, SQLSMALLINT* connStrLen) {
  clearLastError();
  SQLRETURN result = SQL_SUCCESS, result2;
  if (!m_conn.isOpen()) {
    m_conn.open(server, port, connProps);

    if (outConnStr) {
      std::string connStr;
      connStr.append("server=").append(server).append(";port=").append(
          std::to_string(port));
      for (Properties::const_iterator iter = connProps.begin();
          iter != connProps.end(); ++iter) {
        connStr.append(";").append(iter->first).append("=").append(
            iter->second);
      }
      SQLINTEGER connLen = 0;
      result = getStringValue((const SQLCHAR*)connStr.data(),
          StringFunctions::restrictLength<SQLLEN, size_t>(connStr.size()),
          outConnStr, outConnStrLen, &connLen, "connection string");
      if (connStrLen) {
        *connStrLen = StringFunctions::restrictLength<SQLSMALLINT, SQLLEN>(
            connLen);
      }
    }
    // now set all the attributes on the connection
    for (AttributeMap::const_iterator iter = m_attributes.begin();
        iter != m_attributes.end(); ++iter) {
      const SQLINTEGER attrKey = iter->first;
      if (attrKey != SQL_ATTR_LOGIN_TIMEOUT) {
        result2 = setConnectionAttribute(attrKey, iter->second);
        if (result2 != SQL_SUCCESS) {
          if (result2 == SQL_ERROR) {
            return result2;
          } else {
            result = result2;
          }
        }
      }
    }
    return result;
  } else {
    setException(GET_SQLEXCEPTION2(SQLStateMessage::CONNECTION_IN_USE_MSG));
    return SQL_ERROR;
  }
}

SQLRETURN SnappyConnection::connect(const std::string& server, const int port,
    const Properties& connProps, const std::string& dsn, SQLCHAR* outConnStr,
    const SQLINTEGER outConnStrLen, SQLSMALLINT* connStrLen) {
  SQLRETURN result;
  try {
    result = connectT(server, port, connProps, outConnStr, outConnStrLen,
        connStrLen);
  } catch (SQLException& sqle) {
    setException(sqle);
    result = SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    result = SQL_ERROR;
  }
  if (result != SQL_ERROR) {
    m_dsn = dsn;
    return result;
  } else {
    m_dsn.clear();
    return SQL_ERROR;
  }
}

SQLRETURN SnappyConnection::connect(const std::string& server, const int port,
    const Properties& connProps, const std::string& dsn, SQLWCHAR* outConnStr,
    const SQLINTEGER outConnStrLen, SQLSMALLINT* connStrLen) {
  SQLRETURN result;
  try {
    result = connectT(server, port, connProps, outConnStr, outConnStrLen,
        connStrLen);
  } catch (SQLException& sqle) {
    setException(sqle);
    result = SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    result = SQL_ERROR;
  }
  if (result != SQL_ERROR) {
    m_dsn = dsn;
    return result;
  } else {
    m_dsn.clear();
    return SQL_ERROR;
  }
}

SQLRETURN SnappyConnection::disconnect() {
  clearLastError();
  if (m_conn.isOpen()) {
    try {
      m_conn.close();
      return SQL_SUCCESS;
    } catch (SQLException& sqle) {
      setException(sqle);
      return SQL_ERROR;
    } catch (std::exception& se) {
      setException(__FILE__, __LINE__, se);
      return SQL_ERROR;
    }
  } else {
    setException(
        GET_SQLEXCEPTION2(SQLStateMessage::NO_CURRENT_CONNECTION_MSG1));
    return SQL_ERROR;
  }
}

SQLRETURN SnappyConnection::setConnectionAttribute(SQLINTEGER attribute,
    const AttributeValue& attrValue) {
  clearLastError();
  switch (attribute) {
    case SQL_ATTR_ACCESS_MODE:
      switch (attrValue.m_val.m_intv) {
        case SQL_MODE_READ_ONLY:
          m_conn.setTransactionAttribute(
              TransactionAttribute::READ_ONLY_CONNECTION, true);
          break;
        case SQL_MODE_READ_WRITE:
          m_conn.setTransactionAttribute(
              TransactionAttribute::READ_ONLY_CONNECTION, false);
          break;
        default:
          // should be handled by DriverManager
          setException(GET_SQLEXCEPTION2(
              SQLStateMessage::INVALID_ATTRIBUTE_VALUE_MSG,
              attrValue.m_val.m_intv, "SQL_ATTR_ACCESS_MODE"));
          return SQL_ERROR;
      }
      break;
    case SQL_ATTR_AUTOCOMMIT:
      switch (attrValue.m_val.m_intv) {
        case SQL_AUTOCOMMIT_OFF:
          m_conn.setTransactionAttribute(TransactionAttribute::AUTOCOMMIT,
              false);
          break;
        case SQL_AUTOCOMMIT_ON:
          m_conn.setTransactionAttribute(TransactionAttribute::AUTOCOMMIT,
              true);
          break;
        default:
          // should be handled by DriverManager
          setException(GET_SQLEXCEPTION2(
              SQLStateMessage::INVALID_ATTRIBUTE_VALUE_MSG,
              attrValue.m_val.m_intv, "SQL_ATTR_AUTOCOMMIT"));
          return SQL_ERROR;
      }
      break;
    case SQL_ATTR_CONNECTION_TIMEOUT:
      // setReceiveTimeout is in milliseconds
      m_conn.setReceiveTimeout(attrValue.m_val.m_intv * 1000);
      break;
    case SQL_ATTR_LOGIN_TIMEOUT:
      // setConnectTimeout is in milliseconds
      m_conn.setConnectTimeout(attrValue.m_val.m_intv * 1000);
      break;
    case SQL_ATTR_CURRENT_CATALOG:
      // catalogs not relevant in SnappyData; silently ignore
      m_conn.checkAndGetService();
      break;
    case SQL_ATTR_METADATA_ID:
      switch (attrValue.m_val.m_intv) {
        case SQL_TRUE:
          m_argsAsIdentifiers = true;
          break;
        case SQL_FALSE:
          m_argsAsIdentifiers = false;
          break;
        default:
          // should be handled by DriverManager
          setException(GET_SQLEXCEPTION2(
              SQLStateMessage::INVALID_ATTRIBUTE_VALUE_MSG,
              attrValue.m_val.m_intv, "SQL_ATTR_METADATA_ID"));
          return SQL_ERROR;
      }
      break;
    case SQL_ATTR_PACKET_SIZE:
      m_conn.setSendBufferSize(attrValue.m_val.m_intv);
      break;
    case SQL_ATTR_QUIET_MODE:
      m_hwnd = attrValue.m_val.m_refv;
      break;
    case SQL_ATTR_TRANSLATE_LIB: {
      if (m_translationLibrary) {
        delete m_translationLibrary;
        m_translationLibrary = nullptr;
      }
      // for unicode version convert to UTF8
      if (attrValue.m_ascii) {
        m_translationLibrary = new native::Library(
            (const char*)attrValue.m_val.m_refv);
      } else {
        const SQLWCHAR* wchars = (const SQLWCHAR*)attrValue.m_val.m_refv;
        std::string chars = StringFunctions::toString(wchars, SQL_NTS);
        m_translationLibrary = new native::Library(chars.c_str());
      }
      m_driverToDataSource =
          (DriverToDataSource)m_translationLibrary->getFunction(
              "SQLDriverToDataSource");
      m_dataSourceToDriver =
          (DataSourceToDriver)m_translationLibrary->getFunction(
              "SQLDataSourceToDriver");
      break;
    }
    case SQL_ATTR_TRANSLATE_OPTION:
      m_translateOption = attrValue.m_val.m_intv;
      break;
    case SQL_ATTR_TXN_ISOLATION:
      switch (attrValue.m_val.m_intv) {
        case SQL_TXN_READ_COMMITTED:
          m_conn.beginTransaction(IsolationLevel::READ_COMMITTED);
          break;
        case SQL_TXN_REPEATABLE_READ:
          m_conn.beginTransaction(IsolationLevel::REPEATABLE_READ);
          break;
        case 0:
          m_conn.beginTransaction(IsolationLevel::NONE);
          break;
        case SQL_TXN_READ_UNCOMMITTED:
          m_conn.beginTransaction(IsolationLevel::READ_UNCOMMITTED);
          break;
        case SQL_TXN_SERIALIZABLE:
          m_conn.beginTransaction(IsolationLevel::SERIALIZABLE);
          break;
        default:
          setException(GET_SQLEXCEPTION2(
              SQLStateMessage::INVALID_ATTRIBUTE_VALUE_MSG,
              attrValue.m_val.m_intv, "SQL_ATTR_TXN_ISOLATION"));
          return SQL_ERROR;
      }
      break;
    case SQL_ATTR_ANSI_APP:
      // both ANSI and Unicode APIs are supported in identical manner
      return SQL_ERROR;
    default:
      // should be handled by DriverManager/ODBC API
      setException(GET_SQLEXCEPTION2(
          SQLStateMessage::UNKNOWN_ATTRIBUTE_MSG, attribute));
      return SQL_ERROR;
  }
  return SQL_SUCCESS;
}

SQLRETURN SnappyConnection::setAttribute(SQLINTEGER attribute, SQLPOINTER value,
    SQLINTEGER stringLength, bool isAscii) {
  clearLastError();
  AttributeValue attrValue(isAscii);
  switch (attribute) {
    case SQL_ATTR_ACCESS_MODE:
    case SQL_ATTR_AUTOCOMMIT:
    case SQL_ATTR_CONNECTION_TIMEOUT:
    case SQL_ATTR_METADATA_ID:
    case SQL_ATTR_PACKET_SIZE:
    case SQL_ATTR_TRANSLATE_OPTION:
    case SQL_ATTR_TXN_ISOLATION:
      // integer values
      attrValue.m_val.m_intv = (SQLUINTEGER)(SQLBIGINT)value;
      m_attributes[attribute] = attrValue;
      break;
    case SQL_ATTR_LOGIN_TIMEOUT:
      if (m_conn.isOpen()) {
        setException(GET_SQLEXCEPTION2(
            SQLStateMessage::ATTRIBUTE_CANNOT_BE_SET_NOW_MSG,
            "SQL_ATTR_LOGIN_TIMEOUT ", "Connection is open."));
        return SQL_ERROR;
      } else {
        attrValue.m_val.m_intv = (SQLUINTEGER)(SQLBIGINT)value;
        m_attributes[attribute] = attrValue;
      }
      break;
    case SQL_ATTR_QUIET_MODE:
      attrValue.m_val.m_refv = value;
      m_attributes[SQL_ATTR_QUIET_MODE] = attrValue;
      break;
    case SQL_ATTR_CURRENT_CATALOG:
    case SQL_ATTR_TRANSLATE_LIB: {
      // string values
      if (isAscii) {
        if (stringLength < 0) {
          stringLength = static_cast<SQLINTEGER>(::strlen((const char*)value));
        }
        SQLCHAR* strv = new SQLCHAR[static_cast<size_t>(stringLength) + 1];
        ::memcpy(strv, value, static_cast<size_t>(stringLength) + 1);
        attrValue.m_val.m_refv = strv;
      } else {
        if (stringLength < 0) {
          stringLength = static_cast<SQLINTEGER>(StringFunctions::strlen(
              (const SQLWCHAR*)value));
        }
        SQLWCHAR* strv = new SQLWCHAR[static_cast<size_t>(stringLength) + 1];
        ::memcpy(strv, value, (static_cast<size_t>(stringLength) + 1) *
            sizeof(SQLWCHAR));
        attrValue.m_val.m_refv = strv;
      }
      // free up any old value
      AttributeMap::const_iterator oldValue = m_attributes.find(attribute);
      if (oldValue != m_attributes.end()) {
        if (oldValue->second.m_ascii) {
          delete[] (SQLCHAR*)oldValue->second.m_val.m_refv;
        } else {
          delete[] (SQLWCHAR*)oldValue->second.m_val.m_refv;
        }
      }
      m_attributes[attribute] = attrValue;
      break;
    }
    case SQL_ATTR_ODBC_CURSORS: {
      // only DRIVER setting allowed here
      auto intv = (SQLULEN)value;
      if (intv != SQL_CUR_USE_DRIVER) {
        setException(
            GET_SQLEXCEPTION2(SQLStateMessage::INVALID_ATTRIBUTE_VALUE_MSG,
                intv, "setAttribute(SQL_ATTR_ODBC_CURSORS)"));
        return SQL_ERROR;
      }
      break;
    }
    case SQL_ATTR_ASYNC_ENABLE:
      // TODO: async execution by just invoking send_* & recv_ in background?
      setException(GET_SQLEXCEPTION2(
          SQLStateMessage::FEATURE_NOT_IMPLEMENTED_MSG1,
          "setAttribute(SQL_ATTR_ASYNC_ENABLE)"));
      return SQL_ERROR;
    case SQL_ATTR_AUTO_IPD:
      // TODO: descriptors not yet implemented
      setException(GET_SQLEXCEPTION2(
          SQLStateMessage::FEATURE_NOT_IMPLEMENTED_MSG1,
          "setAttribute(SQL_ATTR_AUTO_IPD)"));
      return SQL_ERROR;
    case SQL_ATTR_ENLIST_IN_DTC:
      // TODO: XA transactions support not yet added to ODBC driver
      // see IDtcToXaHelperSinglePipe, ITransactionResourceAsync etc.
      setException(GET_SQLEXCEPTION2(
          SQLStateMessage::FEATURE_NOT_IMPLEMENTED_MSG1,
          "setAttribute(SQL_ATTR_ENLIST_IN_DTC)"));
      return SQL_ERROR;

    case SQL_ATTR_CONNECTION_DEAD:
    case SQL_ATTR_TRACE:
    case SQL_ATTR_TRACEFILE:
    default:
      // should be handled by DriverManager
      setException(GET_SQLEXCEPTION2(
          SQLStateMessage::UNKNOWN_ATTRIBUTE_MSG, attribute));
      return SQL_ERROR;
  }
  if (m_conn.isOpen()) {
    if (attribute != SQL_ATTR_LOGIN_TIMEOUT) {
      try {
        return setConnectionAttribute(attribute, attrValue);
      } catch (SQLException& sqle) {
        setException(sqle);
        return SQL_ERROR;
      } catch (std::exception& se) {
        setException(__FILE__, __LINE__, se);
        return SQL_ERROR;
      }
    }
  }

  return SQL_SUCCESS;
}

static inline SQLINTEGER translateTransactionIsolation(
    const IsolationLevel isolationLevel) noexcept {
  switch (isolationLevel) {
    case IsolationLevel::READ_COMMITTED:
      return SQL_TXN_READ_COMMITTED;
    case IsolationLevel::REPEATABLE_READ:
      return SQL_TXN_REPEATABLE_READ;
    case IsolationLevel::NONE:
      return 0;
    case IsolationLevel::READ_UNCOMMITTED:
      return SQL_TXN_READ_UNCOMMITTED;
    case IsolationLevel::SERIALIZABLE:
      return SQL_TXN_SERIALIZABLE;
    default:
      return -1;
  }
}

template<typename CHAR_TYPE>
SQLRETURN SnappyConnection::getAttributeT(SQLINTEGER attribute,
    SQLPOINTER resultValue, SQLINTEGER bufferLength,
    SQLINTEGER* stringLengthPtr) {
  clearLastError();
  if (m_conn.isOpen()) {
    SQLUINTEGER intResult = SQL_NTS;
    switch (attribute) {
      case SQL_ATTR_CONNECTION_DEAD:
        intResult = m_conn.isOpen() ? SQL_FALSE : SQL_TRUE;
        break;
      case SQL_ATTR_ACCESS_MODE:
        intResult = m_conn.getTransactionAttribute(
            TransactionAttribute::READ_ONLY_CONNECTION)
            ? SQL_MODE_READ_ONLY : SQL_MODE_READ_WRITE;
        break;
      case SQL_ATTR_AUTOCOMMIT:
        intResult = m_conn.getTransactionAttribute(
            TransactionAttribute::AUTOCOMMIT)
            || thrift::snappydataConstants::DEFAULT_AUTOCOMMIT
            ? SQL_AUTOCOMMIT_ON : SQL_AUTOCOMMIT_OFF;
        break;
      case SQL_ATTR_CONNECTION_TIMEOUT:
        intResult = m_conn.getReceiveTimeout();
        break;
      case SQL_ATTR_METADATA_ID:
        intResult = m_argsAsIdentifiers ? SQL_TRUE : SQL_FALSE;
        break;
      case SQL_ATTR_PACKET_SIZE:
        intResult = m_conn.getSendBufferSize();
        break;
      case SQL_ATTR_TRANSLATE_OPTION:
        intResult = m_translateOption;
        break;
      case SQL_ATTR_LOGIN_TIMEOUT:
        intResult = m_conn.getConnectTimeout();
        break;
      case SQL_ATTR_TXN_ISOLATION: {
        const auto isolation = m_conn.getCurrentIsolationLevel();
        const SQLINTEGER result = translateTransactionIsolation(isolation);
        if (result >= 0) {
          intResult = static_cast<SQLUINTEGER>(result);
        } else {
          // unexpected value from driver?
          setException(
              GET_SQLEXCEPTION2(SQLStateMessage::INVALID_ATTRIBUTE_VALUE_MSG,
                  static_cast<int>(isolation), "SQL_ATTR_TXN_ISOLATION"));
          return SQL_ERROR;
        }
        break;
      }
      case SQL_ATTR_CURRENT_CATALOG:
        // catalogs not valid for SnappyData
        return SQL_NO_DATA;
      case SQL_ATTR_TRANSLATE_LIB: {
        // translation library name is in the attributes map
        AttributeMap::const_iterator attrSearch = m_attributes.find(
            SQL_ATTR_TRANSLATE_LIB);
        if (attrSearch != m_attributes.end()) {
          if (attrSearch->second.m_ascii) {
            const SQLCHAR* str =
                (const SQLCHAR*)attrSearch->second.m_val.m_refv;
            return getStringValue(str, SQL_NTS, (CHAR_TYPE*)resultValue,
                bufferLength, stringLengthPtr, "attribute");
          } else {
            const SQLWCHAR* str =
                (const SQLWCHAR*)attrSearch->second.m_val.m_refv;
            return getStringValue(str, SQL_NTS, (CHAR_TYPE*)resultValue,
                bufferLength, stringLengthPtr, "attribute");
          }
        } else {
          return SQL_NO_DATA;
        }
      }
      case SQL_ATTR_QUIET_MODE:
        if (m_hwnd) {
          if (resultValue) {
            *((SQLPOINTER*)resultValue) = m_hwnd;
            if (stringLengthPtr) {
              *stringLengthPtr = sizeof(SQLPOINTER);
            }
          }
          return SQL_SUCCESS;
        } else {
          return SQL_NO_DATA;
        }
      case SQL_ATTR_ASYNC_ENABLE:
        // always off for now
        getIntValue(SQL_ASYNC_ENABLE_OFF, resultValue, stringLengthPtr, true);
        return SQL_SUCCESS;
      case SQL_ATTR_ODBC_CURSORS:
        // driver manager should handle this but if it somehow reaches here
        // then this will always be DRIVER level
        getIntValue(SQL_CUR_USE_DRIVER, resultValue, stringLengthPtr, true);
        return SQL_SUCCESS;
      // Below attributes are handled by the driver manager
      case SQL_ATTR_TRACE:
      case SQL_ATTR_TRACEFILE:
      default:
        // should be handled by DriverManager/ODBC API
        setException(
            GET_SQLEXCEPTION2(SQLStateMessage::UNKNOWN_ATTRIBUTE_MSG,
                attribute));
        return SQL_ERROR;
    }
    // if control reaches here then it is an integer result
    if (resultValue) {
      *((SQLUINTEGER*)resultValue) = intResult;
      if (stringLengthPtr) *stringLengthPtr = sizeof(SQLUINTEGER);
    }

    return SQL_SUCCESS;
  } else {
    AttributeMap::const_iterator attrSearch = m_attributes.find(attribute);

    switch (attribute) {
      case SQL_ATTR_METADATA_ID:
        *((SQLUINTEGER*)resultValue) =
            m_argsAsIdentifiers ? SQL_TRUE : SQL_FALSE;
        if (stringLengthPtr) *stringLengthPtr = sizeof(SQLUINTEGER);
        return SQL_SUCCESS;
      case SQL_ATTR_ACCESS_MODE:
      case SQL_ATTR_AUTOCOMMIT:
      case SQL_ATTR_CONNECTION_TIMEOUT:
      case SQL_ATTR_LOGIN_TIMEOUT:
      case SQL_ATTR_PACKET_SIZE:
      case SQL_ATTR_TXN_ISOLATION: {
        // integer value
        SQLINTEGER result = -1;
        if (attrSearch != m_attributes.end()) {
          result = attrSearch->second.m_val.m_intv;
        } else if (attribute == SQL_ATTR_ACCESS_MODE) {
          result = SQL_MODE_READ_WRITE;
        } else if (attribute == SQL_ATTR_AUTOCOMMIT) {
          result = thrift::snappydataConstants::DEFAULT_AUTOCOMMIT
              ? SQL_AUTOCOMMIT_ON : SQL_AUTOCOMMIT_OFF;
        } else if (attribute == SQL_ATTR_LOGIN_TIMEOUT) {
          result = 0;
        }
        if (result >= 0) {
          if (resultValue) {
            *((SQLUINTEGER*)resultValue) = static_cast<SQLUINTEGER>(result);
            if (stringLengthPtr) {
              *stringLengthPtr = sizeof(SQLUINTEGER);
            }
          }
          return SQL_SUCCESS;
        } else {
          setException(
            GET_SQLEXCEPTION2(SQLStateMessage::NO_CURRENT_CONNECTION_MSG1));
          return SQL_ERROR;
        }
      }

      case SQL_ATTR_CURRENT_CATALOG:
      case SQL_ATTR_TRANSLATE_LIB:
      case SQL_ATTR_TRANSLATE_OPTION:
      // TODO: async execution?
      case SQL_ATTR_ASYNC_ENABLE:
        setException(
            GET_SQLEXCEPTION2(SQLStateMessage::NO_CURRENT_CONNECTION_MSG1));
        return SQL_ERROR;

      case SQL_ATTR_QUIET_MODE:
        if (attrSearch != m_attributes.end()) {
          if (resultValue) {
            *(SQLPOINTER*)resultValue = attrSearch->second.m_val.m_refv;
          }
          if (stringLengthPtr) *stringLengthPtr = sizeof(SQLPOINTER);
          return SQL_SUCCESS;
        } else {
          setException(
            GET_SQLEXCEPTION2(SQLStateMessage::NO_CURRENT_CONNECTION_MSG1));
          return SQL_ERROR;
        }

      case SQL_ATTR_AUTO_IPD:
        // TODO: descriptors not yet implemented
        setException(GET_SQLEXCEPTION2(
            SQLStateMessage::FEATURE_NOT_IMPLEMENTED_MSG1,
            "getAttribute(SQL_ATTR_AUTO_IPD)"));
        return SQL_ERROR;
      case SQL_ATTR_ENLIST_IN_DTC:
        // TODO: XA transactions support not yet added to ODBC driver
        // see IDtcToXaHelperSinglePipe, ITransactionResourceAsync etc.
        setException(GET_SQLEXCEPTION2(
            SQLStateMessage::FEATURE_NOT_IMPLEMENTED_MSG1,
            "getAttribute(SQL_ATTR_ENLIST_IN_DTC)"));
        return SQL_ERROR;
      case SQL_ATTR_ODBC_CURSORS:
        // driver manager should handle this but if it somehow reaches here
        // then this will always be DRIVER level
        if (resultValue) {
          *((SQLULEN*)resultValue) = static_cast<SQLULEN>(SQL_CUR_USE_DRIVER);
          if (stringLengthPtr) *stringLengthPtr = sizeof(SQLULEN);
        }
        return SQL_SUCCESS;

      // these attributes are handled by the Driver manager
      case SQL_ATTR_TRACE:
      case SQL_ATTR_TRACEFILE:
      default:
        // should be handled by DriverManager
        setException(GET_SQLEXCEPTION2(
            SQLStateMessage::UNKNOWN_ATTRIBUTE_MSG, attribute));
        return SQL_ERROR;
    }
  }
}

template<typename CHAR_TYPE>
SQLRETURN SnappyConnection::getInfoT(SQLUSMALLINT infoType, SQLPOINTER infoValue,
    SQLSMALLINT bufferLength, SQLSMALLINT* stringLength) {
  clearLastError();
  SQLRETURN ret = SQL_SUCCESS;
  const char* resStr = nullptr;
  const char* resInfo = nullptr;
  std::string resultString;
  SQLLEN resStrLen = 0;
  switch (infoType) {
    case SQL_DRIVER_NAME:
      resStr = ODBC_DRIVER_NAME;
      resStrLen = SQL_NTS;
      resInfo = "SQL_DRIVER_NAME";
      break;
    case SQL_PRODUCT_NAME:
      resStr = ODBC_PRODUCT_NAME;
      resStrLen = SQL_NTS;
      resInfo = "SQL_PRODUCT_NAME";
      break;
    case SQL_DRIVER_VER:
      resStr = ODBC_DRIVER_VERSION;
      resStrLen = SQL_NTS;
      resInfo = "SQL_DRIVER_VER";
      break;
    case SQL_DRIVER_ODBC_VER:
      resStr = ODBCVER_DRIVER_STRING;
      resStrLen = SQL_NTS;
      resInfo = "SQL_DRIVER_ODBC_VER";
      break;
    case SQL_ODBC_VER:
      resStr = ODBCVER_STRING;
      resStrLen = SQL_NTS;
      resInfo = "SQL_ODBC_VER";
      break;
    case SQL_DATABASE_NAME:
      resStr = SNAPPY_DATABASE_NAME;
      resStrLen = SQL_NTS;
      resInfo = "SQL_DATABASE_NAME";
      break;
    case SQL_DBMS_NAME:
      resStr = SNAPPY_DBMS_NAME;
      resStrLen = SQL_NTS;
      resInfo = "SQL_DBMS_NAME";
      break;
    case SQL_DBMS_VER:
      resStr = SNAPPY_DBMS_VERSION;
      resStrLen = SQL_NTS;
      resInfo = "SQL_DBMS_VER";
      break;
    case SQL_ODBC_INTERFACE_CONFORMANCE:
      *(SQLUINTEGER*)infoValue = SQL_OIC_LEVEL1;
      break;
    case SQL_ODBC_API_CONFORMANCE:
      // ODBC 2.0 has the size as SQLSMALLINT
      *(SQLSMALLINT*)infoValue = SQL_OAC_LEVEL1;
      break;
    case SQL_SQL_CONFORMANCE: {
      SQLUINTEGER result = 0;
      const DatabaseMetaData* dbmd = m_conn.getServiceMetaData();
      if (dbmd->isFeatureSupported(
          DatabaseFeature::SQL_GRAMMAR_ANSI92_ENTRY)) {
        result |= SQL_SC_SQL92_ENTRY;
      }
      if (dbmd->isFeatureSupported(
          DatabaseFeature::SQL_GRAMMAR_ANSI92_INTERMEDIATE)) {
        result |= SQL_SC_SQL92_INTERMEDIATE;
      }
      if (dbmd->isFeatureSupported(
          DatabaseFeature::SQL_GRAMMAR_ANSI92_FULL)) {
        result |= SQL_SC_SQL92_FULL;
      }
      *(SQLUINTEGER*)infoValue = result;
      break;
    }
    case SQL_ODBC_SQL_CONFORMANCE: {
      SQLSMALLINT result = 0;
      const DatabaseMetaData* dbmd = m_conn.getServiceMetaData();
      if (dbmd->isFeatureSupported(DatabaseFeature::SQL_GRAMMAR_MINIMUM)) {
        result |= SQL_OSC_MINIMUM;
      }
      if (dbmd->isFeatureSupported(DatabaseFeature::SQL_GRAMMAR_CORE)) {
        result |= SQL_OSC_CORE;
      }
      if (dbmd->isFeatureSupported(DatabaseFeature::SQL_GRAMMAR_EXTENDED)) {
        result |= SQL_OSC_EXTENDED;
      }
      *(SQLSMALLINT*)infoValue = result;
      break;
    }
    case SQL_STANDARD_CLI_CONFORMANCE:
      *(SQLUINTEGER*)infoValue =
          SQL_SCC_XOPEN_CLI_VERSION1 | SQL_SCC_ISO92_CLI;
      break;
    case SQL_XOPEN_CLI_YEAR:
      resStr = XOPEN_CLI_YEAR;
      resStrLen = SQL_NTS;
      resInfo = "SQL_XOPEN_CLI_YEAR";
      break;
    case SQL_SERVER_NAME:
      resStr = m_conn.getCurrentHostAddress().hostName.c_str();
      resStrLen = SQL_NTS;
      resInfo = "SQL_SERVER_NAME";
      break;
    case SQL_USER_NAME:
      resStr = m_conn.getConnectionArgs().userName.c_str();
      resStrLen = SQL_NTS;
      resInfo = "SQL_USER_NAME";
      break;
    case SQL_MAX_IDENTIFIER_LEN:
      // limit on identifier length is 128 (as per Apache Derby)
      *(SQLUSMALLINT*)infoValue = 128;
      break;
    case SQL_MAX_DRIVER_CONNECTIONS:
      // no specific limit
      *(SQLUSMALLINT*)infoValue = 0;
      break;
    case SQL_MAX_CONCURRENT_ACTIVITIES:
      // no specific limit
      *(SQLUSMALLINT*)infoValue = 0;
      break;
    case SQL_ACTIVE_ENVIRONMENTS:
      // no specific limit
      *(SQLUSMALLINT*)infoValue = 0;
      break;
    case SQL_DATA_SOURCE_NAME:
      resStr = m_dsn.data();
      resStrLen = StringFunctions::restrictLength<SQLLEN, size_t>(
          m_dsn.length());
      resInfo = "SQL_DATA_SOURCE_NAME";
      break;
    case SQL_SCROLL_OPTIONS: {
      SQLUINTEGER flags = 0;
      const DatabaseMetaData *dbmd = m_conn.getServiceMetaData();
      if (dbmd->isFeatureSupported(DatabaseFeature::RESULTSET_FORWARD_ONLY)) {
        flags |= SQL_SO_FORWARD_ONLY;
      }
      if (dbmd->isFeatureSupported(
          DatabaseFeature::RESULTSET_SCROLL_INSENSITIVE)) {
        flags |= SQL_SO_STATIC;
      }
      if (dbmd->isFeatureSupported(
          DatabaseFeature::RESULTSET_SCROLL_SENSITIVE)) {
        flags |= SQL_SO_DYNAMIC;
      }
      *(SQLUINTEGER*)infoValue = flags;
      break;
    }
    case SQL_SCROLL_CONCURRENCY: {
      // this only works in DatabaseMetaData when given the type of cursor
      // so use the "lowest" FORWARD_ONLY
      const ResultSetType dynamicCursorType = ResultSetType::FORWARD_ONLY;
      SQLINTEGER flags = 0;
      const DatabaseMetaData *dbmd = m_conn.getServiceMetaData();
      if (dbmd->supportsResultSetReadOnly(dynamicCursorType)) {
        flags |= SQL_SCCO_READ_ONLY;
      }
      if (dbmd->supportsResultSetUpdatable(dynamicCursorType)) {
        flags |= SQL_SCCO_LOCK;
      }
      *(SQLINTEGER*)infoValue = flags;
      break;
    }
    case SQL_STATIC_SENSITIVITY: {
      // this only works in DatabaseMetaData when given the type of cursor
      // so use the "lowest" FORWARD_ONLY
      const ResultSetType dynamicCursorType = ResultSetType::FORWARD_ONLY;
      SQLINTEGER flags = 0;
      const DatabaseMetaData *dbmd = m_conn.getServiceMetaData();
      if (dbmd->ownInsertsVisible(dynamicCursorType)) {
        flags |= SQL_SS_ADDITIONS;
      }
      if (dbmd->ownDeletesVisible(dynamicCursorType)) {
        flags |= SQL_SS_DELETIONS;
      }
      if (dbmd->ownUpdatesVisible(dynamicCursorType)) {
        flags |= SQL_SS_UPDATES;
      }
      *(SQLINTEGER*)infoValue = flags;
      break;
    }
    case SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES1: {
      // no lock/unlock of current row
      SQLUINTEGER flags = SQL_CA1_LOCK_NO_CHANGE;
      const DatabaseMetaData* dbmd = m_conn.getServiceMetaData();
      if (dbmd->isFeatureSupported(DatabaseFeature::POSITIONED_UPDATE)) {
        flags |= SQL_CA1_POSITIONED_UPDATE;
      }
      if (dbmd->isFeatureSupported(DatabaseFeature::POSITIONED_DELETE)) {
        flags |= SQL_CA1_POSITIONED_DELETE;
      }
      if (dbmd->isFeatureSupported(DatabaseFeature::SELECT_FOR_UPDATE)) {
        flags |= SQL_CA1_SELECT_FOR_UPDATE;
      }
      if (dbmd->isFeatureSupported(
          DatabaseFeature::SQL_GRAMMAR_ANSI92_ENTRY)) {
        flags |= SQL_CA1_NEXT;
      }
      *(SQLUINTEGER*)infoValue = flags;
      break;
    }
    case SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES2: {
      // by default try to guarantee single row changes for positioned ops
      SQLUINTEGER flags = SQL_CA2_SIMULATE_TRY_UNIQUE;
      const DatabaseMetaData* dbmd = m_conn.getServiceMetaData();
      const ResultSetType dynamicCursorType = ResultSetType::FORWARD_ONLY;
      if (dbmd->supportsResultSetReadOnly(dynamicCursorType)) {
        flags |= SQL_CA2_READ_ONLY_CONCURRENCY;
      }
      if (dbmd->othersInsertsVisible(dynamicCursorType)) {
        flags |= SQL_CA2_SENSITIVITY_ADDITIONS;
      }
      if (dbmd->othersDeletesVisible(dynamicCursorType)) {
        flags |= SQL_CA2_SENSITIVITY_DELETIONS;
      }
      if (dbmd->othersUpdatesVisible(dynamicCursorType)) {
        flags |= SQL_CA2_SENSITIVITY_UPDATES;
      }
      *(SQLUINTEGER*)infoValue = flags;
      break;
    }
    case SQL_DYNAMIC_CURSOR_ATTRIBUTES1: {
      // no lock/unlock of current row
      SQLUINTEGER flags = SQL_CA1_LOCK_NO_CHANGE;
      // below are implemented in the driver
      flags |= SQL_CA1_POS_POSITION | SQL_CA1_POS_UPDATE | SQL_CA1_POS_DELETE
          | SQL_CA1_POS_REFRESH | SQL_CA1_BULK_ADD;
      const DatabaseMetaData* dbmd = m_conn.getServiceMetaData();
      if (dbmd->isFeatureSupported(DatabaseFeature::POSITIONED_UPDATE)) {
        flags |= SQL_CA1_POSITIONED_UPDATE;
      }
      if (dbmd->isFeatureSupported(DatabaseFeature::POSITIONED_DELETE)) {
        flags |= SQL_CA1_POSITIONED_DELETE;
      }
      if (dbmd->isFeatureSupported(DatabaseFeature::SELECT_FOR_UPDATE)) {
        flags |= SQL_CA1_SELECT_FOR_UPDATE;
      }
      if (dbmd->isFeatureSupported(
          DatabaseFeature::SQL_GRAMMAR_ANSI92_ENTRY)) {
        flags |= SQL_CA1_NEXT | SQL_CA1_ABSOLUTE | SQL_CA1_RELATIVE;
      }
      *(SQLUINTEGER*)infoValue = flags;
      break;
    }
    case SQL_DYNAMIC_CURSOR_ATTRIBUTES2: {
      // by default try to guarantee single row changes for positioned ops
      SQLUINTEGER flags = SQL_CA2_SIMULATE_TRY_UNIQUE;
      const DatabaseMetaData* dbmd = m_conn.getServiceMetaData();
      const ResultSetType dynamicCursorType = ResultSetType::FORWARD_ONLY;
      if (dbmd->supportsResultSetReadOnly(dynamicCursorType)) {
        flags |= SQL_CA2_READ_ONLY_CONCURRENCY;
      }
      if (dbmd->othersInsertsVisible(dynamicCursorType)) {
        flags |= SQL_CA2_SENSITIVITY_ADDITIONS;
      }
      if (dbmd->othersDeletesVisible(dynamicCursorType)) {
        flags |= SQL_CA2_SENSITIVITY_DELETIONS;
      }
      if (dbmd->othersUpdatesVisible(dynamicCursorType)) {
        flags |= SQL_CA2_SENSITIVITY_UPDATES;
      }
      *(SQLUINTEGER*)infoValue = flags;
      break;
    }
    case SQL_STATIC_CURSOR_ATTRIBUTES1: {
      // no lock/unlock of current row
      SQLUINTEGER flags = SQL_CA1_LOCK_NO_CHANGE;
      // below are implemented in the driver
      flags |= SQL_CA1_POS_POSITION | SQL_CA1_POS_UPDATE | SQL_CA1_POS_DELETE
          | SQL_CA1_POS_REFRESH | SQL_CA1_BULK_ADD;
      const DatabaseMetaData* dbmd = m_conn.getServiceMetaData();
      if (dbmd->isFeatureSupported(DatabaseFeature::POSITIONED_UPDATE)) {
        flags |= SQL_CA1_POSITIONED_UPDATE;
      }
      if (dbmd->isFeatureSupported(DatabaseFeature::POSITIONED_DELETE)) {
        flags |= SQL_CA1_POSITIONED_DELETE;
      }
      if (dbmd->isFeatureSupported(DatabaseFeature::SELECT_FOR_UPDATE)) {
        flags |= SQL_CA1_SELECT_FOR_UPDATE;
      }
      if (dbmd->isFeatureSupported(
          DatabaseFeature::SQL_GRAMMAR_ANSI92_ENTRY)) {
        flags |= SQL_CA1_NEXT | SQL_CA1_ABSOLUTE | SQL_CA1_RELATIVE;
      }
      *(SQLUINTEGER*)infoValue = flags;
      break;
    }
    case SQL_STATIC_CURSOR_ATTRIBUTES2: {
      // by default guarantee single row changes
      SQLUINTEGER flags = SQL_CA2_SIMULATE_TRY_UNIQUE;
      const DatabaseMetaData* dbmd = m_conn.getServiceMetaData();
      const ResultSetType staticCursorType = ResultSetType::INSENSITIVE;
      if (dbmd->supportsResultSetReadOnly(staticCursorType)) {
        flags |= SQL_CA2_READ_ONLY_CONCURRENCY;
      }
      if (dbmd->othersInsertsVisible(staticCursorType)) {
        flags |= SQL_CA2_SENSITIVITY_ADDITIONS;
      }
      if (dbmd->othersDeletesVisible(staticCursorType)) {
        flags |= SQL_CA2_SENSITIVITY_DELETIONS;
      }
      if (dbmd->othersUpdatesVisible(staticCursorType)) {
        flags |= SQL_CA2_SENSITIVITY_UPDATES;
      }
      *(SQLUINTEGER*)infoValue = flags;
      break;
    }
    case SQL_BATCH_SUPPORT: {
      const DatabaseMetaData* dbmd = m_conn.getServiceMetaData();
      SQLUINTEGER flags = SQL_BS_SELECT_PROC | SQL_BS_ROW_COUNT_PROC;
      if (dbmd->isFeatureSupported(DatabaseFeature::BATCH_UPDATES)) {
        flags |= SQL_BS_ROW_COUNT_EXPLICIT;
      }
      *(SQLUINTEGER*)infoValue = flags;
      break;
    }
    case SQL_BATCH_ROW_COUNT:
      *(SQLUINTEGER*)infoValue = SQL_BRC_EXPLICIT;
      break;
    case SQL_PARAM_ARRAY_ROW_COUNTS:
      *(SQLUINTEGER*)infoValue = SQL_PARC_BATCH;
      break;
    case SQL_CURSOR_COMMIT_BEHAVIOR: {
      const DatabaseMetaData* dbmd = m_conn.getServiceMetaData();
      *(SQLUSMALLINT*)infoValue =
          dbmd->isFeatureSupported(
              DatabaseFeature::OPEN_CURSORS_ACROSS_COMMIT)
              ? SQL_CB_PRESERVE : (dbmd->isFeatureSupported(
                  DatabaseFeature::OPEN_STATEMENTS_ACROSS_COMMIT)
                  ? SQL_CB_CLOSE : SQL_CB_DELETE);
      break;
    }
    case SQL_CURSOR_ROLLBACK_BEHAVIOR: {
      const DatabaseMetaData* dbmd = m_conn.getServiceMetaData();
      *(SQLUSMALLINT*)infoValue =
          dbmd->isFeatureSupported(
              DatabaseFeature::OPEN_CURSORS_ACROSS_ROLLBACK)
              ? SQL_CB_PRESERVE : (dbmd->isFeatureSupported(
                  DatabaseFeature::OPEN_STATEMENTS_ACROSS_ROLLBACK)
                  ? SQL_CB_CLOSE : SQL_CB_DELETE);
      break;
    }
    case SQL_CURSOR_SENSITIVITY: {
      SQLUINTEGER result = SQL_UNSPECIFIED;
      const DatabaseMetaData *dbmd = m_conn.getServiceMetaData();
      if (dbmd->supportsResultSetType(ResultSetType::INSENSITIVE)) {
        result |= SQL_INSENSITIVE;
      }
      if (dbmd->supportsResultSetType(ResultSetType::SENSITIVE)) {
        result |= SQL_SENSITIVE;
      }
      *(SQLUINTEGER*)infoValue = result;
      break;
    }
    case SQL_EXPRESSIONS_IN_ORDERBY: {
      const DatabaseMetaData* dbmd = m_conn.getServiceMetaData();
      resStr = dbmd->isFeatureSupported(
          DatabaseFeature::ORDER_BY_EXPRESSIONS) ? "Y" : "N";
      resStrLen = 1;
      resInfo = "SQL_EXPRESSIONS_IN_ORDERBY";
      break;
    }
    case SQL_OJ_CAPABILITIES: {
      SQLUINTEGER result = 0;
      const DatabaseMetaData* dbmd = m_conn.getServiceMetaData();
      if (dbmd->isFeatureSupported(DatabaseFeature::OUTER_JOINS)) {
        result |= (SQL_OJ_LEFT | SQL_OJ_RIGHT);
      }
      if (dbmd->isFeatureSupported(DatabaseFeature::OUTER_JOINS_FULL)) {
        result |= SQL_OJ_FULL;
      }
      *(SQLUINTEGER*)infoValue = result;
      break;
    }
    case SQL_GETDATA_EXTENSIONS:
      // TODO: support SQL_GD_OUTPUT_PARAMS to allow for output parameters
      // in SQLGetData (and thus chunked results for blobs/clobs)
      *(SQLUINTEGER*)infoValue = SQL_GD_ANY_COLUMN | SQL_GD_ANY_ORDER
          | SQL_GD_BLOCK | SQL_GD_BOUND;
      break;
    case SQL_TXN_CAPABLE: {
      // TODO: Implement transactions for column tables.
      *(SQLUSMALLINT*)infoValue = SQL_TC_DML;
      break;
    }
    case SQL_CATALOG_USAGE: {
      // Catalogs are not supported (only schema and table)
      *(SQLUINTEGER*)infoValue = 0;
      break;
    }
    case SQL_CATALOG_TERM: {
      // Catalogs are not supported (only schema and table)
      resStr = "";
      resStrLen = 0;
      resInfo = "SQL_CATALOG_TERM";
      break;
    }
    case SQL_SCHEMA_TERM: { // also SQL_OWNER_TERM
      resStr = "schema";
      resStrLen = SQL_NTS;
      resInfo = "SQL_SCHEMA_TERM";
      break;
    }
    case SQL_SCHEMA_USAGE: { // also SQL_OWNER_USAGE
      // All possible values:
      // SQL_OU_DML_STATEMENTS SQL_OU_INDEX_DEFINITION SQL_OU_PRIVILEGE_DEFINITION
      // SQL_OU_PROCEDURE_INVOCATION SQL_OU_TABLE_DEFINITION
      *(SQLUINTEGER*)infoValue = SQL_SU_DML_STATEMENTS |
        SQL_SU_TABLE_DEFINITION | SQL_SU_INDEX_DEFINITION |
        SQL_SU_PRIVILEGE_DEFINITION;
      break;
    }
    case SQL_TABLE_TERM: {
      // TODO: Check whether database is correct. 
      resStr = "table";
      resStrLen = SQL_NTS;
      resInfo = "SQL_TABLE_TERM";
      break;
    }
    case SQL_COLUMN_ALIAS: {
      resStr = "Y";
      resStrLen = 1;
      resInfo = "SQL_COLUMN_ALIAS";
      break;
    }
    case SQL_IDENTIFIER_QUOTE_CHAR: {
      resStr = "`";
      resStrLen = 1;
      resInfo = "SQL_IDENTIFIER_QUOTE_CHAR";
      break;
    }
    case SQL_CATALOG_NAME: {
      // TODO: Returning "N" as SQL-92 Full level conformant driver returns "Y"
      resStr = "N";
      resStrLen = 1;
      resInfo = "SQL_CATALOG_NAME";
      break;
    }
    case SQL_CATALOG_NAME_SEPARATOR: {
      resStr = "";
      resStrLen = 0;
      resInfo = "SQL_CATALOG_NAME_SEPARATOR";
      break;
    }
    case SQL_SPECIAL_CHARACTERS: {
      resStr = "";
      resStrLen = 0;
      resInfo = "SQL_SPECIAL_CHARACTERS";
      break;
    }
    case SQL_QUOTED_IDENTIFIER_CASE: {
      // Possible Values :
      // SQL_IC_MIXED         when connected to a server running a case-insensitive sort order.
      // SQL_IC_SENSITIVE     when connected to a server running a case-sensitive sort order.
      *(SQLUSMALLINT*)infoValue = SQL_IC_SENSITIVE;
      break;
    }
    case SQL_AGGREGATE_FUNCTIONS: {
      *(SQLUINTEGER*)infoValue = SQL_AF_ALL | SQL_AF_AVG | SQL_AF_COUNT |
        SQL_AF_DISTINCT | SQL_AF_MAX | SQL_AF_MIN | SQL_AF_SUM;
      break;
    }
    case SQL_NUMERIC_FUNCTIONS: {
      // Removing below ones as did not find in spark sql numeric function list
      // SQL_FN_NUM_COT, SQL_FN_NUM_MOD (pmod is present), SQL_FN_NUM_TRUNCATE
      *(SQLUINTEGER*)infoValue = SQL_FN_NUM_ABS | SQL_FN_NUM_ACOS |
          SQL_FN_NUM_ASIN | SQL_FN_NUM_ATAN | SQL_FN_NUM_ATAN2 |
          SQL_FN_NUM_CEILING | SQL_FN_NUM_COS | SQL_FN_NUM_DEGREES |
          SQL_FN_NUM_EXP | SQL_FN_NUM_FLOOR | SQL_FN_NUM_LOG |
          SQL_FN_NUM_LOG10 | SQL_FN_NUM_PI | SQL_FN_NUM_POWER |
          SQL_FN_NUM_RADIANS | SQL_FN_NUM_RAND | SQL_FN_NUM_ROUND |
          SQL_FN_NUM_SIGN | SQL_FN_NUM_SIN | SQL_FN_NUM_SQRT | SQL_FN_NUM_TAN;
      break;
    }
    case SQL_STRING_FUNCTIONS: {
      // Removing below ones as did not find in spark sql numeric function list
      // SQL_FN_STR_BIT_LENGTH, SQL_FN_STR_CHAR, SQL_FN_STR_CHAR_LENGTH,
      // SQL_FN_STR_CHARACTER_LENGTH, SQL_FN_STR_DIFFERENCE, SQL_FN_STR_INSERT,
      // SQL_FN_STR_LEFT, SQL_FN_STR_OCTET_LENGTH, SQL_FN_STR_POSITION,
      // SQL_FN_STR_REPLACE, SQL_FN_STR_RIGHT

      *(SQLUINTEGER*)infoValue = SQL_FN_STR_ASCII | SQL_FN_STR_CONCAT |
          SQL_FN_STR_LENGTH | SQL_FN_STR_LCASE | SQL_FN_STR_LOCATE |
          SQL_FN_STR_LTRIM | SQL_FN_STR_REPEAT | SQL_FN_STR_RTRIM |
          SQL_FN_STR_SOUNDEX | SQL_FN_STR_SPACE | SQL_FN_STR_SUBSTRING |
          SQL_FN_STR_UCASE;
      break;
    }
    case SQL_TIMEDATE_FUNCTIONS: {
      // Removing below ones as did not find in spark sql numeric function list
      // SQL_FN_TD_CURDATE, SQL_FN_TD_CURRENT_TIME, SQL_FN_TD_CURTIME,
      // SQL_FN_TD_DAYNAME, SQL_FN_TD_DAYOFWEEK, SQL_FN_TD_EXTRACT,
      // SQL_FN_TD_MONTHNAME, SQL_FN_TD_TIMESTAMPADD, SQL_FN_TD_TIMESTAMPDIFF,
      // SQL_FN_TD_WEEK

      *(SQLUINTEGER*)infoValue =  SQL_FN_TD_CURRENT_DATE |
          SQL_FN_TD_CURRENT_TIMESTAMP | SQL_FN_TD_DAYOFMONTH |
          SQL_FN_TD_DAYOFYEAR | SQL_FN_TD_HOUR | SQL_FN_TD_MINUTE |
          SQL_FN_TD_MONTH | SQL_FN_TD_NOW | SQL_FN_TD_QUARTER |
          SQL_FN_TD_SECOND | SQL_FN_TD_YEAR;
      break;
    }
    case SQL_TIMEDATE_ADD_INTERVALS: {
      // All possible values:
      // SQL_FN_TSI_DAY, SQL_FN_TSI_FRAC_SECOND, SQL_FN_TSI_HOUR,
      // SQL_FN_TSI_MINUTE, SQL_FN_TSI_MONTH, SQL_FN_TSI_QUARTER,
      // SQL_FN_TSI_SECOND, SQL_FN_TSI_WEEK, SQL_FN_TSI_YEAR

      // TIMESTAMPADD is not supported
      *(SQLUINTEGER*)infoValue = 0;
      break;
    }
    case SQL_TIMEDATE_DIFF_INTERVALS: {
      // All possible values:
      // SQL_FN_TSI_DAY, SQL_FN_TSI_FRAC_SECOND, SQL_FN_TSI_HOUR,
      // SQL_FN_TSI_MINUTE, SQL_FN_TSI_MONTH, SQL_FN_TSI_QUARTER,
      // SQL_FN_TSI_SECOND, SQL_FN_TSI_WEEK, SQL_FN_TSI_YEAR

      // TIMESTAMPDIFF is not supported
      *(SQLUINTEGER*)infoValue = 0;
      break;
    }
    case SQL_DATETIME_LITERALS: {
      // These are the datetime literals listed in the SQL-92 specification
      // and are separate from the datetime literal escape
      // clauses defined by ODBC.
      // SnappyData does not support these.
      *(SQLUINTEGER*)infoValue = 0;
      break;
    }
    case SQL_SYSTEM_FUNCTIONS: {
      // Removing the ones below:
      // SQL_FN_SYS_DBNAME, SQL_FN_SYS_USERNAME

      *(SQLUINTEGER*)infoValue = SQL_FN_SYS_IFNULL;
      break;
    }
    case SQL_CONVERT_FUNCTIONS: {
      // Removing the ones below:
      // SQL_FN_CVT_CONVERT

      *(SQLUINTEGER*)infoValue = SQL_FN_CVT_CAST;
      break;
    }
    case SQL_CONVERT_BIGINT:
    case SQL_CONVERT_BINARY:
    case SQL_CONVERT_BIT:
    case SQL_CONVERT_CHAR:
    case SQL_CONVERT_DATE:
    case SQL_CONVERT_DECIMAL:
    case SQL_CONVERT_DOUBLE:
    case SQL_CONVERT_FLOAT:
#ifndef IODBC
    case SQL_CONVERT_GUID:
#endif
    case SQL_CONVERT_INTEGER:
    case SQL_CONVERT_INTERVAL_DAY_TIME:
    case SQL_CONVERT_INTERVAL_YEAR_MONTH:
    case SQL_CONVERT_LONGVARBINARY:
    case SQL_CONVERT_LONGVARCHAR:
    case SQL_CONVERT_NUMERIC:
    case SQL_CONVERT_REAL:
    case SQL_CONVERT_SMALLINT:
    case SQL_CONVERT_TIME:
    case SQL_CONVERT_TIMESTAMP:
    case SQL_CONVERT_TINYINT:
    case SQL_CONVERT_VARBINARY:
    case SQL_CONVERT_VARCHAR:
    case SQL_CONVERT_WCHAR:
    case SQL_CONVERT_WLONGVARCHAR:
    case SQL_CONVERT_WVARCHAR:
      *(SQLUINTEGER*)infoValue = 0;
      break;

    case SQL_SQL92_VALUE_EXPRESSIONS: {
      *(SQLUINTEGER*)infoValue = SQL_SVE_CASE | SQL_SVE_CAST |
          SQL_SVE_COALESCE | SQL_SVE_NULLIF;
      break;
    }
    case SQL_SQL92_NUMERIC_VALUE_FUNCTIONS: {
      // All possible values:
      // SQL_SNVF_BIT_LENGTH, SQL_SNVF_CHAR_LENGTH, SQL_SNVF_CHARACTER_LENGTH,
      // SQL_SNVF_EXTRACT, SQL_SNVF_OCTET_LENGTH, SQL_SNVF_POSITION

      *(SQLUINTEGER*)infoValue = 0;
      break;
    }
    case SQL_SQL92_STRING_FUNCTIONS: {
      // Removed the ones below:
      // SQL_SSF_CONVERT, SQL_SSF_TRIM_BOTH, SQL_SSF_TRIM_LEADING, SQL_SSF_TRIM_TRAILING

      *(SQLUINTEGER*)infoValue = SQL_SSF_LOWER | SQL_SSF_UPPER |
          SQL_SSF_SUBSTRING | SQL_SSF_TRANSLATE;
      break;
    }
    case SQL_SQL92_DATETIME_FUNCTIONS: {
      // Removed the ones below:
      // SQL_SDF_CURRENT_TIME

      *(SQLUINTEGER*)infoValue = SQL_SDF_CURRENT_DATE | SQL_SDF_CURRENT_TIMESTAMP;
      break;
    }
    case SQL_SQL92_RELATIONAL_JOIN_OPERATORS: {
      // Removed the ones below:
      // SQL_SRJO_CORRESPONDING_CLAUSE, SQL_SRJO_UNION_JOIN

      *(SQLUINTEGER*)infoValue = SQL_SRJO_CROSS_JOIN | SQL_SRJO_EXCEPT_JOIN |
          SQL_SRJO_FULL_OUTER_JOIN | SQL_SRJO_INNER_JOIN | SQL_SRJO_INTERSECT_JOIN |
          SQL_SRJO_LEFT_OUTER_JOIN | SQL_SRJO_NATURAL_JOIN | SQL_SRJO_RIGHT_OUTER_JOIN;
      break;
    }
    case SQL_SQL92_PREDICATES: {
      // Removed the ones below:
      // SQL_SP_MATCH_FULL, SQL_SP_MATCH_PARTIAL, SQL_SP_MATCH_UNIQUE_FULL,
      // SQL_SP_MATCH_UNIQUE_PARTIAL, SQL_SP_OVERLAPS,
      // SQL_SP_QUANTIFIED_COMPARISON, SQL_SP_UNIQUE

      *(SQLUINTEGER*)infoValue = SQL_SP_BETWEEN | SQL_SP_COMPARISON |
          SQL_SP_EXISTS | SQL_SP_ISNOTNULL | SQL_SP_ISNULL | SQL_SP_LIKE |
          SQL_SP_IN;
      break;
    }
    case SQL_SQL92_GRANT: {
      // Removed the ones below:
      // SQL_SG_INSERT_COLUMN, SQL_SG_USAGE_ON_DOMAIN,
      // SQL_SG_USAGE_ON_CHARACTER_SET, SQL_SG_USAGE_ON_COLLATION,
      // SQL_SG_USAGE_ON_TRANSLATION

      *(SQLUINTEGER*)infoValue = SQL_SG_DELETE_TABLE | SQL_SG_INSERT_TABLE |
          SQL_SG_REFERENCES_TABLE | SQL_SG_REFERENCES_COLUMN |
          SQL_SG_SELECT_TABLE | SQL_SG_UPDATE_COLUMN |
          SQL_SG_UPDATE_TABLE | SQL_SG_WITH_GRANT_OPTION;
      break;
    }
    case SQL_SQL92_REVOKE: {
      // Removed the ones below:
      // SQL_SR_CASCADE, SQL_SR_GRANT_OPTION_FOR, SQL_SR_INSERT_COLUMN,
      // SQL_SR_RESTRICT, SQL_SR_USAGE_ON_DOMAIN, SQL_SR_USAGE_ON_CHARACTER_SET,
      // SQL_SR_USAGE_ON_COLLATION, SQL_SR_USAGE_ON_TRANSLATION

      *(SQLUINTEGER*)infoValue = SQL_SR_DELETE_TABLE | SQL_SR_INSERT_TABLE |
          SQL_SR_REFERENCES_TABLE | SQL_SR_REFERENCES_COLUMN |
          SQL_SR_SELECT_TABLE | SQL_SR_UPDATE_COLUMN | SQL_SR_UPDATE_TABLE;
      break;
    }
    case SQL_SQL92_ROW_VALUE_CONSTRUCTOR: {
      // Removed the ones below:
      // SQL_SRVC_DEFAULT (supported by GemFireXD store but not by Spark)

      *(SQLUINTEGER*)infoValue = SQL_SRVC_NULL | SQL_SRVC_VALUE_EXPRESSION |
          SQL_SRVC_ROW_SUBQUERY;
      break;
    }
    case SQL_KEYWORDS: {
      const DatabaseMetaData* dbmd = m_conn.getServiceMetaData();
      auto keywords = dbmd->getSQLKeyWords();
      for (auto iter = keywords.cbegin(); iter != keywords.cend(); ++iter) {
        if (iter != keywords.cbegin()) resultString.append(",");
        resultString.append(*iter);
      }
      resStr = resultString.data();
      resInfo = "SQL_KEYWORDS";
      resStrLen = StringFunctions::restrictLength<SQLLEN, size_t>(
          resultString.length());
      break;
    }
    case SQL_FILE_USAGE:
      *(SQLUSMALLINT*)infoValue = SQL_FILE_NOT_SUPPORTED;
      break;
    case SQL_ROW_UPDATES:
      resStr = "N";
      resInfo = "SQL_ROW_UPDATES";
      resStrLen = 1;
      break;
    case SQL_SEARCH_PATTERN_ESCAPE:
      resStr = "\\";
      resInfo = "SQL_SEARCH_PATTERN_ESCAPE";
      resStrLen = 1;
      break;
    case SQL_PROCEDURES:
      resStr = "N"; // procedures are not supported in Spark
      resInfo = "SQL_PROCEDURES";
      resStrLen = 1;
      break;
    case SQL_ACCESSIBLE_PROCEDURES:
      resStr = "N"; // procedures are not supported in Spark
      resInfo = "SQL_ACCESSIBLE_PROCEDURES";
      resStrLen = 1;
      break;
    case SQL_PROCEDURE_TERM:
      resStr = ""; // procedures are not supported in Spark
      resInfo = "SQL_PROCEDURE_TERM";
      resStrLen = 0;
      break;
    case SQL_MAX_PROCEDURE_NAME_LEN:
      *(SQLUSMALLINT*)infoValue = 0; // procedures are not supported in Spark
      break;
    case SQL_ACCESSIBLE_TABLES: {
      const DatabaseMetaData *dbmd = m_conn.getServiceMetaData();
      resStr = dbmd->isFeatureSupported(DatabaseFeature::ALL_TABLES_SELECTABLE)
          ? "Y" : "N";
      resInfo = "SQL_ACCESSIBLE_TABLES";
      resStrLen = 1;
      break;
    }
    case SQL_CONCAT_NULL_BEHAVIOR: {
      const DatabaseMetaData *dbmd = m_conn.getServiceMetaData();
      *(SQLUSMALLINT*)infoValue = dbmd->isFeatureSupported(DatabaseFeature::
          NULL_CONCAT_NON_NULL_IS_NULL) ? SQL_CB_NULL : SQL_CB_NON_NULL;
      break;
    }
    case SQL_DATA_SOURCE_READ_ONLY: {
      const DatabaseMetaData *dbmd = m_conn.getServiceMetaData();
      resStr = dbmd->isReadOnly() ? "Y" : "N";
      resInfo = "SQL_DATA_SOURCE_READ_ONLY";
      resStrLen = 1;
      break;
    }
    case SQL_DEFAULT_TXN_ISOLATION: {
      const DatabaseMetaData *dbmd = m_conn.getServiceMetaData();
      *(SQLUINTEGER*)infoValue = translateTransactionIsolation(
          dbmd->defaultTransactionIsolation());
      break;
    }
    case SQL_TXN_ISOLATION_OPTION: {
      SQLUINTEGER levels = 0;
      const DatabaseMetaData *dbmd = m_conn.getServiceMetaData();
      if (dbmd->supportsTransactionIsolationLevel(
          IsolationLevel::READ_UNCOMMITTED)) {
        levels |= SQL_TXN_READ_UNCOMMITTED;
      }
      if (dbmd->supportsTransactionIsolationLevel(
          IsolationLevel::READ_COMMITTED)) {
        levels |= SQL_TXN_READ_COMMITTED;
      }
      if (dbmd->supportsTransactionIsolationLevel(
          IsolationLevel::REPEATABLE_READ)) {
        levels |= SQL_TXN_REPEATABLE_READ;
      }
      if (dbmd->supportsTransactionIsolationLevel(
          IsolationLevel::SERIALIZABLE)) {
        levels |= SQL_TXN_SERIALIZABLE;
      }
      *(SQLUINTEGER*)infoValue = levels;
      break;
    }
    case SQL_MULT_RESULT_SETS: {
      const DatabaseMetaData *dbmd = m_conn.getServiceMetaData();
      resStr = dbmd->isFeatureSupported(
          DatabaseFeature::MULTIPLE_RESULTSETS) ? "Y" : "N";
      resInfo = "SQL_MULT_RESULT_SETS";
      resStrLen = 1;
      break;
    }
    case SQL_MULTIPLE_ACTIVE_TXN: {
      const DatabaseMetaData *dbmd = m_conn.getServiceMetaData();
      resStr = dbmd->isFeatureSupported(
          DatabaseFeature::MULTIPLE_TRANSACTIONS) ? "Y" : "N";
      resInfo = "SQL_MULTIPLE_ACTIVE_TXN";
      resStrLen = 1;
      break;
    }
    case SQL_NEED_LONG_DATA_LEN:
      resStr = "N";
      resInfo = "SQL_NEED_LONG_DATA_LEN";
      resStrLen = 1;
      break;
    case SQL_NULL_COLLATION: {
      SQLUSMALLINT nullsSorting = 0;
      const DatabaseMetaData *dbmd = m_conn.getServiceMetaData();
      if (dbmd->isFeatureSupported(DatabaseFeature::NULLS_SORTED_END)) {
        nullsSorting = SQL_NC_END;
      } else if (dbmd->isFeatureSupported(DatabaseFeature::NULLS_SORTED_HIGH)) {
        nullsSorting = SQL_NC_HIGH;
      } else if (dbmd->isFeatureSupported(DatabaseFeature::NULLS_SORTED_LOW)) {
        nullsSorting = SQL_NC_LOW;
      } else if (dbmd->isFeatureSupported(
          DatabaseFeature::NULLS_SORTED_START)) {
        nullsSorting = SQL_NC_START;
      }
      *(SQLUSMALLINT*)infoValue = nullsSorting;
      break;
    }
    case SQL_COLLATION_SEQ:
      resStr = "utf8"; // always UTF-8
      resInfo = "SQL_COLLATION_SEQ";
      resStrLen = SQL_NTS;
      break;
    case SQL_CREATE_SCHEMA:
      *(SQLUINTEGER*)infoValue = SQL_CS_CREATE_SCHEMA | SQL_CS_AUTHORIZATION;
      break;
    case SQL_DROP_SCHEMA:
      *(SQLUINTEGER*)infoValue = SQL_DS_DROP_SCHEMA | SQL_DS_CASCADE |
          SQL_DS_RESTRICT;
      break;
    case SQL_CREATE_TABLE:
      *(SQLUINTEGER*)infoValue = SQL_CT_CREATE_TABLE | SQL_CT_TABLE_CONSTRAINT |
          SQL_CT_CONSTRAINT_NAME_DEFINITION | SQL_CT_COLUMN_CONSTRAINT |
          SQL_CT_COLUMN_DEFAULT | SQL_CT_COMMIT_DELETE;
      break;
    case SQL_DROP_TABLE:
      *(SQLUINTEGER*)infoValue = SQL_DT_DROP_TABLE;
      break;
    case SQL_CREATE_VIEW:
      *(SQLUINTEGER*)infoValue = SQL_CV_CREATE_VIEW;
      break;
    case SQL_DROP_VIEW:
      *(SQLUINTEGER*)infoValue = SQL_DV_DROP_VIEW;
      break;
    case SQL_DDL_INDEX:
      *(SQLUINTEGER*)infoValue = SQL_DI_CREATE_INDEX | SQL_DI_DROP_INDEX;
      break;
    case SQL_DESCRIBE_PARAMETER:
      resStr = "N";
      resInfo = "SQL_DESCRIBE_PARAMETER";
      resStrLen = 1;
      break;
    case SQL_INDEX_KEYWORDS:
      *(SQLUINTEGER*)infoValue = SQL_IK_ASC | SQL_IK_DESC;
      break;
    case SQL_INSERT_STATEMENT:
      *(SQLUINTEGER*)infoValue = SQL_IS_INSERT_LITERALS |
          SQL_IS_INSERT_SEARCHED | SQL_IS_SELECT_INTO;
      break;
    case SQL_PARAM_ARRAY_SELECTS:
      *(SQLUINTEGER*)infoValue = SQL_PAS_NO_SELECT;
      break;
    case SQL_ASYNC_MODE:
      *(SQLUINTEGER*)infoValue = SQL_AM_NONE; // unsupported
      break;
    case SQL_ALTER_DOMAIN:
    case SQL_BOOKMARK_PERSISTENCE:
    case SQL_CREATE_ASSERTION:
    case SQL_CREATE_CHARACTER_SET:
    case SQL_CREATE_COLLATION:
    case SQL_CREATE_DOMAIN:
    case SQL_CREATE_TRANSLATION:
    case SQL_DROP_ASSERTION:
    case SQL_DROP_CHARACTER_SET:
    case SQL_DROP_COLLATION:
    case SQL_DROP_DOMAIN:
    case SQL_DROP_TRANSLATION:
    case SQL_INFO_SCHEMA_VIEWS:
    case SQL_MAX_ASYNC_CONCURRENT_STATEMENTS:
      *(SQLUINTEGER*)infoValue = 0; // unsupported
      break;

    default:
      // TODO: SW: implement remaining info types
      std::ostringstream sstr;
      sstr << "getInfo(" << infoType << ")";
      setException(GET_SQLEXCEPTION2(
          SQLStateMessage::FEATURE_NOT_IMPLEMENTED_MSG1, sstr.str().c_str()));
      return SQL_ERROR;
  }
  if (resStr) {
    SQLINTEGER totalLen = 0;
    ret = getStringValue((const SQLCHAR*)resStr, resStrLen,
        (CHAR_TYPE*)infoValue, bufferLength, &totalLen, resInfo);
    if (stringLength) {
      *stringLength = StringFunctions::restrictLength<SQLSMALLINT, int64_t>(
          static_cast<int64_t>(totalLen) * sizeof(CHAR_TYPE));
    }
  }
  return ret;
}

SQLRETURN SnappyConnection::getAttribute(SQLINTEGER attribute,
    SQLPOINTER resultValue, SQLINTEGER bufferLength,
    SQLINTEGER* stringLengthPtr) {
  try {
    return getAttributeT<SQLCHAR>(attribute, resultValue, bufferLength,
        stringLengthPtr);
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
}

SQLRETURN SnappyConnection::getAttributeW(SQLINTEGER attribute,
    SQLPOINTER resultValue, SQLINTEGER bufferLength,
    SQLINTEGER* stringLengthPtr) {
  try {
    return getAttributeT<SQLWCHAR>(attribute, resultValue, bufferLength,
        stringLengthPtr);
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
}

SQLRETURN SnappyConnection::getInfo(SQLUSMALLINT infoType, SQLPOINTER infoValue,
    SQLSMALLINT bufferLength, SQLSMALLINT* stringLength) {
  try {
    return getInfoT<SQLCHAR>(infoType, infoValue, bufferLength, stringLength);
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
}

SQLRETURN SnappyConnection::getInfoW(SQLUSMALLINT infoType, SQLPOINTER infoValue,
    SQLSMALLINT bufferLength, SQLSMALLINT* stringLength) {
  try {
    return getInfoT<SQLWCHAR>(infoType, infoValue, bufferLength, stringLength);
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
}

template<typename CHAR_TYPE>
SQLRETURN SnappyConnection::nativeSQLT(CHAR_TYPE* inStatementText,
    SQLINTEGER textLength1, CHAR_TYPE* outStatementText,
    SQLINTEGER bufferLength, SQLINTEGER* textLength2Ptr) {
  clearLastError();
  if (!outStatementText
      || (bufferLength <= 0 && bufferLength != SQL_NTS)) {
    return SnappyHandleBase::errorInvalidBufferLength(bufferLength,
        "outStatementText string length", this);
  }
  std::string stmt = StringFunctions::toString(inStatementText, textLength1);
  std::string nativeSQL = m_conn.getNativeSQL(stmt);
  return getStringValue((const SQLCHAR*)nativeSQL.data(),
      static_cast<SQLINTEGER>(nativeSQL.size()), outStatementText,
      bufferLength, textLength2Ptr, "nativeSQL");
}

SQLRETURN SnappyConnection::nativeSQL(SQLCHAR* inStatementText,
    SQLINTEGER textLength1, SQLCHAR* outStatementText, SQLINTEGER bufferLength,
    SQLINTEGER* textLength2Ptr) {
  return nativeSQLT<SQLCHAR>(inStatementText, textLength1, outStatementText,
      bufferLength, textLength2Ptr);
}

SQLRETURN SnappyConnection::nativeSQLW(SQLWCHAR* inStatementText,
    SQLINTEGER textLength1, SQLWCHAR* outStatementText, SQLINTEGER bufferLength,
    SQLINTEGER* textLength2Ptr) {
  return nativeSQLT<SQLWCHAR>(inStatementText, textLength1, outStatementText,
      bufferLength, textLength2Ptr);
}

SQLRETURN SnappyConnection::commit() {
  clearLastError();
  try {
    m_conn.commitTransaction(true);
    return SQL_SUCCESS;
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
}

SQLRETURN SnappyConnection::rollback() {
  clearLastError();
  try {
    m_conn.rollbackTransaction(true);
    return SQL_SUCCESS;
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
}

SQLRETURN SnappyConnection::cancelCurrentStatement() {
  clearLastError();
  try {
    m_conn.cancelCurrentStatement();
    return SQL_SUCCESS;
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
}
