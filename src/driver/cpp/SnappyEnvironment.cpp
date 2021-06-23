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
 * SnappyEnvironment.cpp
 *
 * Implementation of general allocation functions and execution environment.
 */

#include "SnappyEnvironment.h"
#include "StringFunctions.h"
#include "SnappyConnection.h"
#include "SnappyStatement.h"
#include "OdbcIniKeys.h"
#include "SnappyDefaults.h"
#include "Library.h"

#include <vector>

using namespace io::snappydata;
using namespace io::snappydata::native;

std::mutex SnappyEnvironment::g_sync;
bool SnappyEnvironment::g_initialized = false;
std::vector<SnappyEnvironment*> SnappyEnvironment::g_envHandles;

namespace _snappy_impl {
  static const std::vector<std::string> s_odbc30StatePrefixes = {
    "01S", "07S", "08S", "21S", "25S", "42S", "HY", "IM"
  };
}

template<typename CHAR_TYPE>
SQLRETURN SnappyEnvironment::handleErrorT(SQLSMALLINT handleType,
    SQLHANDLE handle, SQLSMALLINT recNumber, CHAR_TYPE* sqlState,
    SQLINTEGER* nativeError, CHAR_TYPE* messageText, SQLSMALLINT bufferLength,
    SQLSMALLINT* textLength, SQLSMALLINT textLenFactor) {

  SQLException* sqlEx = nullptr;
  const std::string* message;
  SQLRETURN result = getExceptionRecord(handleType, handle, recNumber,
      std::max(SQL_MAX_MESSAGE_LENGTH, bufferLength - 1), textLength, sqlEx,
      message);
  if (!SQL_SUCCEEDED(result)) {
    return result;
  }

  if (sqlState) {
    const std::string& state = sqlEx->getSQLState();
    StringFunctions::copyString((SQLCHAR*)state.c_str(), state.length(),
        sqlState, 6, nullptr);
  }
  if (nativeError) {
    *nativeError = sqlEx->getSeverity();
  }
  if (messageText || textLength) {
    if (bufferLength < 0) {
      return SQL_ERROR;
    }
    if (recNumber == 1 && LogWriter::debugEnabled()) {
      LogWriter::warn() << "Exception in operation" << LogWriter::NEWLINE
          << *message << LogWriter::NEWLINE;
    }
    if (!messageText) bufferLength = 0;
    SQLLEN len = 0;
    // copy the message as much as there is space in buffer
    if (StringFunctions::copyString((SQLCHAR*)message->data(),
        message->length(), messageText, bufferLength / textLenFactor, &len)) {
      result = SQL_SUCCESS_WITH_INFO;
    }
    if (textLength) {
      *textLength = StringFunctions::restrictLength<SQLSMALLINT, int64_t>(
          static_cast<int64_t>(len) * static_cast<int64_t>(textLenFactor));
    }
  }
  return result;
}

SQLRETURN SnappyEnvironment::getExceptionRecord(SQLSMALLINT handleType,
    SQLHANDLE handle, SQLSMALLINT recNumber, SQLUINTEGER maxRecordSize,
    SQLSMALLINT* textLength, SQLException*& err, const std::string*& message) {
  // return the last error, if any, else the last warning
  if (recNumber <= 0) {
    return SQL_ERROR;
  }
  // try the global error first since it will override everything else
  SnappyHandleBase* handleBase = nullptr;
  err = SnappyHandleBase::lastGlobalError();
  std::mutex* sync = &g_sync;
  if (!err) {
    if (!handle) {
      return SQL_INVALID_HANDLE;
    }
    switch (handleType) {
      case SQL_HANDLE_ENV:
        handleBase = (SnappyEnvironment*)handle;
        break;
      case SQL_HANDLE_DBC:
        handleBase = (SnappyConnection*)handle;
        break;
      case SQL_HANDLE_STMT:
        handleBase = (SnappyStatement*)handle;
        break;
      case SQL_HANDLE_DESC:
        handleBase = (SnappyDescriptor*)handle;
        break;
      default:
        return SQL_INVALID_HANDLE;
    }
    err = handleBase->lastError();
    sync = &handleBase->m_connLock;
  }
  if (err) {
    {
      // always overwrite global exception if there is some lock error
      LockGuard<std::mutex> lock(*sync, false);

      if (lock.lockFailed()) return SQL_ERROR;
      err->fillRecords("[" ODBC_PRODUCT_NAME "] ", maxRecordSize,
          LogWriter::fineEnabled());
    }
    const auto& records = err->getRecords();
    if (static_cast<size_t>(recNumber) <= records.size()) {
      err = records[recNumber - 1].first;
      message = &records[recNumber - 1].second;
    } else {
      err = nullptr;
      message = nullptr;
    }
  }
  if (err) {
    return SQL_SUCCESS;
  } else {
    if (textLength) *textLength = 0;
    return SQL_NO_DATA;
  }
}

template<typename CHAR_TYPE>
SQLRETURN SnappyEnvironment::handleDiagFieldT(SQLSMALLINT handleType,
    SQLHANDLE handle, SQLSMALLINT recNumber, SQLSMALLINT diagId,
    SQLPOINTER diagInfo, SQLSMALLINT bufferLength, SQLSMALLINT* stringLength) {
  SQLRETURN result = SQL_SUCCESS;
  bool hasIntRes = false;
  SQLLEN intRes = 0;
  SQLSMALLINT expectedIntSize = 0;
  std::string stringRes;
  SQLException* sqlEx = nullptr;
  const std::string* message;
  switch (diagId) {
    case SQL_DIAG_CURSOR_ROW_COUNT: {
      if (handleType != SQL_HANDLE_STMT
          || !((SnappyStatement*)handle)->m_resultSet) {
        return SQL_ERROR;
      }
      hasIntRes = true;
      intRes = 0;
      // this is SQLLEN as per API but apps frequently use SQLINTEGER
      expectedIntSize = SQL_IS_INTEGER;
      // TODO: SW: need to revisit
      /*
      SnappyStatement* stmt = (SnappyStatement*)handle;
      if (stmt->m_resultSet.get()) {
        const int32_t currentRow = stmt->m_resultSet->getRow();
        if (stmt->m_resultSet->last()) {
          intRes = stmt->m_resultSet->getRow();
          if (currentRow == 0) {
            stmt->m_resultSet->beforeFirst();
          } else {
            stmt->m_resultSet->absolute(currentRow);
          }
        }
      }
      */
      break;
    }
    case SQL_DIAG_NUMBER: {
      hasIntRes = true;
      intRes = 0;
      expectedIntSize = SQL_IS_INTEGER;
      result = getExceptionRecord(handleType, handle, 1, SQL_MAX_MESSAGE_LENGTH,
          stringLength, sqlEx, message);
      if (SQL_SUCCEEDED(result)) {
        intRes = StringFunctions::restrictLength<SQLLEN, size_t>(
            sqlEx->getRecords().size());
      } else {
        return result;
      }
      break;
    }
    case SQL_DIAG_ROW_COUNT: {
      if (handleType != SQL_HANDLE_STMT) {
        return SQL_ERROR;
      }
      hasIntRes = true;
      intRes = 0;
      // this is SQLLEN as per API but apps frequently use SQLINTEGER
      expectedIntSize = SQL_IS_INTEGER;
      SnappyStatement* stmt = (SnappyStatement*)handle;
      if (!SQL_SUCCEEDED(result = stmt->getUpdateCount(&intRes, false))) {
        return result;
      } else if (intRes < 0) {
        return SQL_ERROR;
      }
      break;
    }
    case SQL_DIAG_CLASS_ORIGIN: {
      result = getExceptionRecord(handleType, handle, recNumber,
          SQL_MAX_MESSAGE_LENGTH, stringLength, sqlEx, message);
      if (SQL_SUCCEEDED(result)) {
        if (sqlEx->getSQLState().find("IM") == 0) {
          stringRes = "ODBC 3.0";
        } else {
          stringRes = "ISO 9075";
        }
      } else {
        return result;
      }
      break;
    }
    case SQL_DIAG_SUBCLASS_ORIGIN: {
      result = getExceptionRecord(handleType, handle, recNumber,
          SQL_MAX_MESSAGE_LENGTH, stringLength, sqlEx, message);
      if (SQL_SUCCEEDED(result)) {
        const std::string& state = sqlEx->getSQLState();
        for (auto& prefix : _snappy_impl::s_odbc30StatePrefixes) {
          if (state.find(prefix) == 0) {
            stringRes = "ODBC 3.0";
            break;
          }
        }
        if (stringRes.empty()) {
          stringRes = "ISO 9075";
        }
      } else {
        return result;
      }
      break;
    }
    case SQL_DIAG_COLUMN_NUMBER: {
      if (handleType != SQL_HANDLE_STMT) {
        return SQL_ERROR;
      }
      hasIntRes = true;
      intRes = SQL_COLUMN_NUMBER_UNKNOWN;
      expectedIntSize = SQL_IS_INTEGER;
      break;
    }
    case SQL_DIAG_ROW_NUMBER: {
      if (handleType != SQL_HANDLE_STMT) {
        return SQL_ERROR;
      }
      hasIntRes = true;
      intRes = SQL_ROW_NUMBER_UNKNOWN;
      // this is SQLLEN as per API but apps frequently use SQLINTEGER
      expectedIntSize = SQL_IS_INTEGER;
      break;
    }
    case SQL_DIAG_CONNECTION_NAME: {
      switch (handleType) {
        case SQL_HANDLE_DBC:
          stringRes = ((SnappyConnection*)handle)->m_conn.toString();
          break;
        case SQL_HANDLE_STMT:
          stringRes = ((SnappyStatement*)handle)->m_conn.m_conn.toString();
          break;
        default:
          break;
      }
      break;
    }
    case SQL_DIAG_SERVER_NAME: {
      switch (handleType) {
        case SQL_HANDLE_DBC:
          stringRes = ((SnappyConnection*)handle)->getDSN();
          break;
        case SQL_HANDLE_STMT:
          stringRes = ((SnappyStatement*)handle)->m_conn.getDSN();
          break;
        default:
          break;
      }
      break;
    }
    case SQL_DIAG_SQLSTATE: {
      result = getExceptionRecord(handleType, handle, recNumber,
          SQL_MAX_MESSAGE_LENGTH, stringLength, sqlEx, message);
      if (SQL_SUCCEEDED(result)) {
        stringRes = sqlEx->getSQLState();
      } else {
        return result;
      }
      break;
    }
    case SQL_DIAG_NATIVE: {
      hasIntRes = true;
      expectedIntSize = SQL_IS_INTEGER;
      result = getExceptionRecord(handleType, handle, recNumber,
          SQL_MAX_MESSAGE_LENGTH, stringLength, sqlEx, message);
      if (SQL_SUCCEEDED(result)) {
        intRes = sqlEx->getSeverity();
      } else {
        return result;
      }
      break;
    }
    case SQL_DIAG_MESSAGE_TEXT:
      return handleErrorT<CHAR_TYPE>(handleType, handle, recNumber, nullptr,
          nullptr, (CHAR_TYPE*)diagInfo, bufferLength, stringLength,
          sizeof(CHAR_TYPE));
    default:
      // TODO: implement SQL_DIAG_RETURNCODE, SQL_DIAG_DYNAMIC_FUNCTION*
      return SQL_ERROR;
  }
  if (hasIntRes) {
    if (!diagInfo) return SQL_ERROR;
    if (bufferLength < 0) expectedIntSize = bufferLength;
    if (bufferLength > 0
        && bufferLength <= static_cast<SQLSMALLINT>(sizeof(SQLLEN))) {
      if (bufferLength == sizeof(SQLLEN)) {
        *((SQLLEN*)diagInfo) = intRes;
      } else if (bufferLength == sizeof(SQLINTEGER)) {
        *((SQLINTEGER*)diagInfo) = StringFunctions::restrictLength<SQLINTEGER,
            SQLLEN>(intRes);
      } else if (bufferLength == sizeof(SQLSMALLINT)) {
        *((SQLSMALLINT*)diagInfo) = StringFunctions::restrictLength<SQLSMALLINT,
            SQLLEN>(intRes);
      } else {
        return SQL_ERROR;
      }
    } else
      switch (expectedIntSize) {
        case SQL_IS_INTEGER:
          *((SQLINTEGER*)diagInfo) = StringFunctions::restrictLength<SQLINTEGER,
              SQLLEN>(intRes);
          break;
        case SQL_IS_UINTEGER:
          *((SQLUINTEGER*)diagInfo) = StringFunctions::restrictLength<
              SQLUINTEGER, SQLLEN>(intRes);
          break;
        case SQL_IS_SMALLINT:
          *((SQLSMALLINT*)diagInfo) = StringFunctions::restrictLength<
              SQLSMALLINT, SQLLEN>(intRes);
          break;
        case SQL_IS_USMALLINT:
          *((SQLUSMALLINT*)diagInfo) = StringFunctions::restrictLength<
              SQLUSMALLINT, SQLLEN>(intRes);
          break;
        default:
          return SQL_ERROR;
      }
  } else {
    SQLLEN len = 0;
    if (StringFunctions::copyString((SQLCHAR*)stringRes.data(),
        stringRes.length(), (CHAR_TYPE*)diagInfo,
        bufferLength / sizeof(CHAR_TYPE), &len)) {
      result = SQL_SUCCESS_WITH_INFO;
    } else {
      result = SQL_SUCCESS;
    }
    if (stringLength) {
      *stringLength = StringFunctions::restrictLength<SQLSMALLINT, int64_t>(
          static_cast<int64_t>(len) * sizeof(CHAR_TYPE));
    }
  }
  return result;
}

SQLRETURN SnappyEnvironment::globalInitialize() {
  LockGuard<std::mutex> lock(g_sync, false);

  if (lock.lockFailed()) {
    return SQL_ERROR;
  }
  if (g_initialized) {
    return SQL_SUCCESS;
  } else {
    LogWriter::setGlobalLoggingFlag("ODBC");
    Connection::initializeService();
    if (OdbcIniKeys::init() == SQL_ERROR) {
      return SQL_ERROR;
    }
    g_initialized = true;
  }
  return SQL_SUCCESS;
}

SQLRETURN SnappyEnvironment::newEnvironment(SnappyEnvironment*& envRef) {
  SQLRETURN result = globalInitialize();
  if (result != SQL_SUCCESS) {
    return result;
  }

  LockGuard<std::mutex> lock(g_sync, false);
  if (lock.lockFailed()) {
    return SQL_ERROR;
  }
  SnappyEnvironment* env = new SnappyEnvironment(false);
  envRef = env;
  g_envHandles.push_back(env);
  return SQL_SUCCESS;
}

SQLRETURN SnappyEnvironment::freeEnvironment(SnappyEnvironment* env) {
  if (env) {
    size_t numConnections = 0;
    {
      LockGuard<std::mutex> lock(env->m_connLock, false);
      if (lock.lockFailed()) {
        return SQL_ERROR;
      }

      numConnections = env->m_connections.size();
    }

    // if there is any active connection then return error
    if (numConnections > 0) {
      SnappyHandleBase::setGlobalException(
          GET_SQLEXCEPTION2(SQLStateMessage::FUNCTION_SEQUENCE_ERROR_MSG,
              "active connections when freeing HENV handle"));
      return SQL_ERROR;
    }

    {
      LockGuard<std::mutex> lock(g_sync, false);
      if (lock.lockFailed()) {
        return SQL_ERROR;
      }

      //remove this env handle
      for (std::vector<SnappyEnvironment*>::iterator iter =
          g_envHandles.begin(); iter != g_envHandles.end(); ++iter) {
        if (env == (*iter)) {
          g_envHandles.erase(iter);
          break;
        }
      }
    }

    delete env;
    return SQL_SUCCESS;
  } else {
    return SnappyHandleBase::errorNullHandle(SQL_HANDLE_ENV);
  }
}

void SnappyEnvironment::addNewActiveConnection(SnappyConnection* conn) {
  LockGuard<std::mutex> lock(m_connLock);

  m_connections.push_back(conn);
}

size_t SnappyEnvironment::getActiveConnectionsCount() {
  LockGuard<std::mutex> lock(m_connLock);

  return m_connections.size();
}

SQLRETURN SnappyEnvironment::forEachActiveConnection(
    std::function<SQLRETURN(SnappyConnection*)> connOperation) {
  SQLRETURN result = SQL_SUCCESS, res;
  SnappyConnection* conn;

  LockGuard<std::mutex> lock(m_connLock, false, this);
  if (lock.lockFailed()) {
    return SQL_ERROR;
  }

  for (std::vector<SnappyConnection*>::iterator iter = m_connections.begin();
      iter != m_connections.end(); ++iter) {
    conn = (*iter);
    if (conn->isActive()) {
      res = connOperation(conn);
      switch (res) {
        case SQL_SUCCESS:
          // no change to existing result
          break;
        case SQL_ERROR:
          result = SQL_ERROR;
          break;
        case SQL_SUCCESS_WITH_INFO:
          // change in existing result only if it is SQL_SUCCESS
          if (result == SQL_SUCCESS) {
            result = res;
          }
          break;
        default:
          if (result != SQL_ERROR) {
            result = res;
          }
          break;
      }
    }
  }
  return result;
}

SQLRETURN SnappyEnvironment::removeActiveConnection(SnappyConnection* conn) {
  LockGuard<std::mutex> lock(m_connLock, false, this);
  if (lock.lockFailed()) {
    return SQL_ERROR;
  }

  for (std::vector<SnappyConnection*>::iterator iter = m_connections.begin();
      iter != m_connections.end(); ++iter) {
    if (conn == (*iter)) {
      m_connections.erase(iter);
      return SQL_SUCCESS;
    }
  }
  return SQL_NO_DATA;
}

SQLRETURN SnappyEnvironment::setAttribute(SQLINTEGER attribute, SQLPOINTER value,
    SQLINTEGER stringLength) {
  clearLastError();
  const SQLINTEGER intValue = (SQLINTEGER)(SQLBIGINT)value;
  switch (attribute) {
    case SQL_ATTR_ODBC_VERSION: {
      switch (intValue) {
        case SQL_OV_ODBC3:
          m_appIsVersion2x = false;
          break;
        case SQL_OV_ODBC2:
          m_appIsVersion2x = true;
          break;
        default:
          std::ostringstream sstr;
          sstr << "setAttribute(SQL_ATTR_ODBC_VERSION=" << intValue << ")";
          setException(
              GET_SQLEXCEPTION2(SQLStateMessage::FEATURE_NOT_IMPLEMENTED_MSG1,
                  sstr.str().c_str()));
          return SQL_ERROR;
      }
      break;
    }
    case SQL_ATTR_OUTPUT_NTS:
      if (intValue != SQL_TRUE) {
        setException(
            GET_SQLEXCEPTION2(SQLStateMessage::FEATURE_NOT_IMPLEMENTED_MSG1,
                "setAttribute(SQL_ATTR_OUTPUT_NTS!=SQL_TRUE)"));
        return SQL_ERROR;
      }
      break;
    case SQL_ATTR_CONNECTION_POOLING:
      // only CP_OFF supported for now
      if (intValue != SQL_CP_OFF) {
        setException(
            GET_SQLEXCEPTION2(SQLStateMessage::FEATURE_NOT_IMPLEMENTED_MSG1,
                "setAttribute(SQL_ATTR_CONNECTION_POOLING)"));
        return SQL_ERROR;
      }
      break;
    case SQL_ATTR_CP_MATCH:
      // only STRICT_MATCH supported
      if (intValue != SQL_CP_STRICT_MATCH) {
        setException(
            GET_SQLEXCEPTION2(SQLStateMessage::FEATURE_NOT_IMPLEMENTED_MSG1,
                "setAttribute(SQL_ATTR_CP_MATCH)"));
        return SQL_ERROR;
      }
      break;
    default:
      // should be handled by DriverManager
      setException(
          GET_SQLEXCEPTION2(SQLStateMessage::UNKNOWN_ATTRIBUTE_MSG, attribute));
      return SQL_ERROR;
  }
  return SQL_SUCCESS;
}

SQLRETURN SnappyEnvironment::getAttribute(SQLINTEGER attribute,
    SQLPOINTER resultValue, SQLINTEGER bufferLength,
    SQLINTEGER* stringLengthPtr) {
  clearLastError();
  SQLINTEGER* result = (SQLINTEGER*)resultValue;
  switch (attribute) {
    case SQL_ATTR_ODBC_VERSION:
      if (result) {
        *result = !m_appIsVersion2x ? SQL_OV_ODBC3 : SQL_OV_ODBC2;
        //*result = SQL_OV_ODBC3;
      }
      break;
    case SQL_ATTR_OUTPUT_NTS:
      if (result) {
        *result = SQL_TRUE;
      }
      break;
    case SQL_ATTR_CONNECTION_POOLING:
      // TODO: implement connection pooling (also fix setAttribute once done)
      if (result) {
        *result = SQL_CP_OFF;
      }
      break;
    case SQL_ATTR_CP_MATCH:
      if (result) {
        *result = SQL_CP_STRICT_MATCH;
      }
      break;
    default:
      // should be handled by DriverManager
      std::ostringstream sstr;
      sstr << "getAttribute(" << attribute << ")";
      setException(
          GET_SQLEXCEPTION2(SQLStateMessage::FEATURE_NOT_IMPLEMENTED_MSG1,
              sstr.str().c_str()));
      return SQL_ERROR;
  }
  if (stringLengthPtr) {
    *stringLengthPtr = sizeof(SQLINTEGER);
  }
  return SQL_SUCCESS;
}

SQLRETURN SnappyEnvironment::handleError(SQLSMALLINT handleType,
    SQLHANDLE handle, SQLSMALLINT recNumber, SQLCHAR* sqlState,
    SQLINTEGER* nativeError, SQLCHAR* messageText, SQLSMALLINT bufferLength,
    SQLSMALLINT* textLength) {
  return handleErrorT(handleType, handle, recNumber, sqlState, nativeError,
      messageText, bufferLength, textLength, 1);
}

SQLRETURN SnappyEnvironment::handleError(SQLSMALLINT handleType,
    SQLHANDLE handle, SQLSMALLINT recNumber, SQLWCHAR* sqlState,
    SQLINTEGER* nativeError, SQLWCHAR* messageText, SQLSMALLINT bufferLength,
    SQLSMALLINT* textLength) {
  return handleErrorT(handleType, handle, recNumber, sqlState, nativeError,
      messageText, bufferLength, textLength, 1);
}

SQLRETURN SnappyEnvironment::handleDiagField(SQLSMALLINT handleType,
    SQLHANDLE handle, SQLSMALLINT recNumber, SQLSMALLINT diagId,
    SQLPOINTER diagInfo, SQLSMALLINT bufferLength,
    SQLSMALLINT* stringLength) {
  return handleDiagFieldT<SQLCHAR>(handleType, handle, recNumber, diagId,
      diagInfo, bufferLength, stringLength);
}

SQLRETURN SnappyEnvironment::handleDiagFieldW(SQLSMALLINT handleType,
    SQLHANDLE handle, SQLSMALLINT recNumber, SQLSMALLINT diagId,
    SQLPOINTER diagInfo, SQLSMALLINT bufferLength,
    SQLSMALLINT* stringLength) {
  return handleDiagFieldT<SQLWCHAR>(handleType, handle, recNumber, diagId,
      diagInfo, bufferLength, stringLength);
}
