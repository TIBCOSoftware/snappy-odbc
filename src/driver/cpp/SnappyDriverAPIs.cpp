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
 * SnappyDriverAPIs.cpp
 */

#include "OdbcBase.h"
#include "SnappyConnection.h"
#include "SnappyStatement.h"
#include "ConnStringPropertyReader.h"
#include "IniPropertyReader.h"
#include "SnappyEnvironment.h"

using namespace io::snappydata;
using namespace io::snappydata::impl;

/*
 * List of functions supported by the SnappyData ODBC driver used
 * in SQLGetFunctions API
 */
SQLUSMALLINT snappySupportedfunctions[] = { SQL_API_SQLALLOCHANDLE,
                                            SQL_API_SQLALLOCSTMT,
                                            SQL_API_SQLBINDCOL,
                                            // TODO: no Spark job cancellation support
                                            // SQL_API_SQLCANCEL,
                                            SQL_API_SQLCLOSECURSOR,
                                            SQL_API_SQLCOLATTRIBUTE,
                                            SQL_API_SQLCOLUMNS,
                                            SQL_API_SQLCONNECT,
                                            SQL_API_SQLDISCONNECT,
                                            SQL_API_SQLENDTRAN,
                                            SQL_API_SQLEXECDIRECT,
                                            SQL_API_SQLEXECUTE,
                                            SQL_API_SQLFETCH,
                                            SQL_API_SQLFETCHSCROLL,
                                            SQL_API_SQLFREEHANDLE,
                                            SQL_API_SQLFREESTMT,
                                            SQL_API_SQLGETCONNECTATTR,
                                            SQL_API_SQLGETCURSORNAME,
                                            SQL_API_SQLGETDATA,
                                            SQL_API_SQLGETDIAGFIELD,
                                            SQL_API_SQLGETDIAGREC,
                                            SQL_API_SQLGETENVATTR,
                                            SQL_API_SQLGETFUNCTIONS,
                                            SQL_API_SQLGETINFO,
                                            SQL_API_SQLGETSTMTATTR,
                                            SQL_API_SQLGETTYPEINFO,
                                            SQL_API_SQLNUMRESULTCOLS,
                                            SQL_API_SQLPARAMDATA,
                                            SQL_API_SQLPREPARE,
                                            SQL_API_SQLPUTDATA,
                                            SQL_API_SQLROWCOUNT,
                                            SQL_API_SQLSETCONNECTATTR,
                                            SQL_API_SQLSETCURSORNAME,
                                            SQL_API_SQLSETENVATTR,
                                            SQL_API_SQLSETSTMTATTR,
                                            SQL_API_SQLSPECIALCOLUMNS,
                                            SQL_API_SQLSTATISTICS,
                                            SQL_API_SQLTABLES,
                                            SQL_API_SQLBULKOPERATIONS,
                                            SQL_API_SQLBINDPARAMETER,
                                            SQL_API_SQLCOLUMNPRIVILEGES,
                                            SQL_API_SQLDRIVERCONNECT,
                                            SQL_API_SQLFOREIGNKEYS,
                                            SQL_API_SQLMORERESULTS,
                                            SQL_API_SQLNATIVESQL,
                                            SQL_API_SQLNUMPARAMS,
                                            SQL_API_SQLPRIMARYKEYS,
                                            SQL_API_SQLPROCEDURECOLUMNS,
                                            SQL_API_SQLPROCEDURES,
                                            SQL_API_SQLSETPOS,
                                            SQL_API_SQLTABLEPRIVILEGES,
                                            SQL_API_SQLDESCRIBECOL,
                                            SQL_API_SQLDESCRIBEPARAM };

///////////////////////////////////////////////////////////////////////////////
////               Environment APIS
///////////////////////////////////////////////////////////////////////////////

SQLRETURN SQL_API SQLAllocHandle(SQLSMALLINT handleType,
    SQLHANDLE inputHandle, SQLHANDLE* outputHandle) {
  FUNCTION_ENTER("HandleType", handleType, "InputHandle", inputHandle);
  SQLRETURN result = SnappyEnvironment::globalInitialize();
  if (result != SQL_SUCCESS) {
    FUNCTION_RETURN(result);
  }
  switch (handleType) {
    case SQL_HANDLE_ENV:
      if (!outputHandle) {
        SnappyHandleBase::errorNullHandle(SQL_HANDLE_ENV);
        FUNCTION_RETURN(SQL_ERROR);
      } else if (!inputHandle) {
        SnappyEnvironment* env;
        result = SnappyEnvironment::newEnvironment(env);
        *outputHandle = env;
        FUNCTION_RETURN(result, "ENV", outputHandle);
      } else {
        *outputHandle = SQL_NULL_HENV;
        result = SnappyHandleBase::errorNullHandle(SQL_HANDLE_ENV);
        FUNCTION_RETURN(result);
      }
    case SQL_HANDLE_DBC: {
      SnappyEnvironment* env = SQL_NULL_HENV;
      if (!outputHandle) {
        result = SnappyHandleBase::errorNullHandle(SQL_HANDLE_ENV);
        FUNCTION_RETURN(result);
      } else if (inputHandle) {
        env = (SnappyEnvironment*)inputHandle;
        SnappyConnection* conn;
        result = SnappyConnection::newConnection(env, conn);
        *outputHandle = conn;
        FUNCTION_RETURN(result, "DBC", outputHandle);
      } else {
        *outputHandle = SQL_NULL_HDBC;
        result = SnappyHandleBase::errorNullHandle(SQL_HANDLE_DBC, env);
        FUNCTION_RETURN(result);
      }
    }
    case SQL_HANDLE_STMT: {
      SnappyConnection* conn = SQL_NULL_HDBC;
      if (!outputHandle) {
        result = SnappyHandleBase::errorNullHandle(SQL_HANDLE_ENV);
        FUNCTION_RETURN(result);
      } else if (inputHandle) {
        conn = (SnappyConnection*)inputHandle;
        if (conn && conn->isActive()) {
          SnappyStatement* stmt;
          result = SnappyStatement::newStatement(conn, stmt);
          *outputHandle = stmt;
          FUNCTION_RETURN(result, "STMT", outputHandle);
        } else {
          *outputHandle = SQL_NULL_HSTMT;
          SnappyHandleBase::setGlobalException(GET_SQLEXCEPTION2(
              SQLStateMessage::NO_CURRENT_CONNECTION_MSG1));
          FUNCTION_RETURN(SQL_ERROR);
        }
      } else {
        *outputHandle = SQL_NULL_HSTMT;
        result = SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT, conn);
        FUNCTION_RETURN(result);
      }
    }
    case SQL_HANDLE_DESC:
      if (!outputHandle) {
        result = SnappyHandleBase::errorNullHandle(SQL_HANDLE_ENV);
        FUNCTION_RETURN(result);
      } else if (inputHandle) {
        // TODO: not implemented, fall-through to default is deliberate
        // return impl::snappyAllocDesc(inputHandle, outputHandle);
      } else {
        *outputHandle = SQL_NULL_HDESC;
        result = SnappyHandleBase::errorNullHandle(SQL_HANDLE_DESC);
        FUNCTION_RETURN(result);
      }

    default:
      if (outputHandle) *outputHandle = nullptr;
      SnappyHandleBase::setGlobalException(GET_SQLEXCEPTION2(
          SQLStateMessage::INVALID_HANDLE_TYPE_MSG, handleType));
      FUNCTION_RETURN(SQL_ERROR);
  }
}

SQLRETURN SQL_API SQLFreeHandle(SQLSMALLINT handleType, SQLHANDLE handle) {
  FUNCTION_ENTER("HandleType", handleType, "InputHandle", handle);
  SQLRETURN result;
  switch (handleType) {
    case SQL_HANDLE_ENV:
      if (!handle) {
        result = SnappyHandleBase::errorNullHandle(handleType);
      } else {
        result = SnappyEnvironment::freeEnvironment(
            (SnappyEnvironment*)handle);
      }
      FUNCTION_RETURN(result);

    case SQL_HANDLE_DBC:
      if (!handle) {
        result = SnappyHandleBase::errorNullHandle(handleType);
      } else {
        result = SnappyConnection::freeConnection(
            (SnappyConnection*)handle);
      }
      FUNCTION_RETURN(result);

    case SQL_HANDLE_STMT:
      if (!handle) {
        result = SnappyHandleBase::errorNullHandle(handleType);
      } else {
        result = SnappyStatement::freeStatement(
            (SnappyStatement*)handle, SQL_DROP);
      }
      FUNCTION_RETURN(result);

    case SQL_HANDLE_DESC:
      // return impl::snappyFreeDesc(handle);

    default:
      SnappyHandleBase::setGlobalException(
          GET_SQLEXCEPTION2(SQLStateMessage::INVALID_HANDLE_TYPE_MSG,
              handleType));
      FUNCTION_RETURN(SQL_ERROR);
  }
}

SQLRETURN SQL_API SQLGetDiagRec(SQLSMALLINT handleType,
    SQLHANDLE handle, SQLSMALLINT recNumber, SQLCHAR* sqlState,
    SQLINTEGER* nativeError, SQLCHAR* messageText, SQLSMALLINT bufferLength,
    SQLSMALLINT* textLength) {
  FUNCTION_ENTER("HandleType", handleType, "InputHandle", handle,
      "RecNumber", recNumber, "BufferLength", bufferLength);
  SQLRETURN result = SnappyEnvironment::handleError(handleType, handle,
      recNumber, sqlState, nativeError, messageText, bufferLength, textLength);
  FUNCTION_RETURN(result, "SQLState", sqlState, "ErrorCode", nativeError,
      "MessageText", messageText, "TextLength", textLength);
}

SQLRETURN SQL_API SQLGetDiagRecW(SQLSMALLINT handleType,
    SQLHANDLE handle, SQLSMALLINT recNumber, SQLWCHAR* sqlState,
    SQLINTEGER* nativeError, SQLWCHAR* messageText, SQLSMALLINT bufferLength,
    SQLSMALLINT* textLength) {
  FUNCTION_ENTER("HandleType", handleType, "InputHandle", handle,
      "RecNumber", recNumber, "BufferLength", bufferLength);
  SQLRETURN result = SnappyEnvironment::handleError(handleType, handle,
      recNumber, sqlState, nativeError, messageText, bufferLength, textLength);
  FUNCTION_RETURN(result, "SQLState", sqlState, "ErrorCode", nativeError,
      "MessageText", messageText, "TextLength", textLength);
}

SQLRETURN SQL_API SQLGetDiagField(SQLSMALLINT handleType,
    SQLHANDLE handle, SQLSMALLINT recNumber, SQLSMALLINT diagId,
    SQLPOINTER diagInfo, SQLSMALLINT bufferLength, SQLSMALLINT* stringLength) {
  FUNCTION_ENTER("HandleType", handleType, "InputHandle", handle,
      "RecNumber", recNumber, "DiagID", diagId, "DiagInfo", diagInfo,
      "BufferLength", bufferLength);
  if (handle) {
    SQLRETURN result = SnappyEnvironment::handleDiagField(handleType, handle,
        recNumber, diagId, diagInfo, bufferLength, stringLength);
    FUNCTION_RETURN_HANDLE(handle, result);
  }
  return SQL_INVALID_HANDLE;
}

SQLRETURN SQL_API SQLGetDiagFieldW(SQLSMALLINT handleType,
    SQLHANDLE handle, SQLSMALLINT recNumber, SQLSMALLINT diagId,
    SQLPOINTER diagInfo, SQLSMALLINT bufferLength, SQLSMALLINT* stringLength) {
  FUNCTION_ENTER("HandleType", handleType, "InputHandle", handle,
      "RecNumber", recNumber, "DiagID", diagId, "DiagInfo", diagInfo,
      "BufferLength", bufferLength);
  if (handle) {
    SQLRETURN result = SnappyEnvironment::handleDiagFieldW(handleType, handle,
        recNumber, diagId, diagInfo, bufferLength, stringLength);
    FUNCTION_RETURN_HANDLE(handle, result);
  }
  return SQL_INVALID_HANDLE;
}

SQLRETURN SQL_API SQLSetEnvAttr(SQLHENV envHandle,
    SQLINTEGER attribute, SQLPOINTER value, SQLINTEGER stringLength) {
  FUNCTION_ENTER("InputHandle", envHandle, "Attribute", attribute,
      "Value", value, "StringLength", stringLength);
  if (envHandle) {
    SQLRETURN result = ((SnappyEnvironment*)envHandle)->setAttribute(
        attribute, value, stringLength);
    FUNCTION_RETURN_HANDLE(envHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_ENV);
}

SQLRETURN SQL_API SQLGetEnvAttr(SQLHENV envHandle,
    SQLINTEGER attribute, SQLPOINTER resultValue, SQLINTEGER bufferLength,
    SQLINTEGER* stringLengthPtr) {
  FUNCTION_ENTER("InputHandle", envHandle, "Attribute", attribute,
      "BufferLength", bufferLength);
  if (envHandle) {
    SQLRETURN result = ((SnappyEnvironment*)envHandle)->getAttribute(
        attribute, resultValue, bufferLength, stringLengthPtr);
    FUNCTION_RETURN_HANDLE(envHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_ENV);
}

///////////////////////////////////////////////////////////////////////////////
////               Connection APIS
///////////////////////////////////////////////////////////////////////////////

SQLRETURN SQL_API SQLGetFunctions(SQLHDBC hdbc,
    SQLUSMALLINT fFunction, SQLUSMALLINT *pfExists) {
  FUNCTION_ENTER("InputHandle", hdbc, "Function", fFunction,
      "ResultPtr", pfExists);

  if (!hdbc || !pfExists) {
    return SnappyHandleBase::errorNullHandle(SQL_HANDLE_ENV);
  }

  SQLRETURN result = SQL_SUCCESS;
  SQLUSMALLINT supported_func_size;
  int index;

  supported_func_size = sizeof(snappySupportedfunctions)
      / sizeof(snappySupportedfunctions[0]);

  if (fFunction == SQL_API_ODBC3_ALL_FUNCTIONS) {
    // Clear and set bits in the 4000 bit vector
    ::memset(pfExists, 0,
        sizeof(SQLUSMALLINT) * SQL_API_ODBC3_ALL_FUNCTIONS_SIZE);
    for (index = 0; index < supported_func_size; ++index) {
      SQLUSMALLINT id = snappySupportedfunctions[index];
      pfExists[id >> 4] |= (1 << (id & 0x000F));
    }
  } else if (fFunction == SQL_API_ALL_FUNCTIONS) {
    // Clear and set elements in the SQLUSMALLINT 100 element array
    ::memset(pfExists, 0, sizeof(SQLUSMALLINT) * 100);
    for (index = 0; index < supported_func_size; ++index) {
      if (snappySupportedfunctions[index] < 100) {
        pfExists[snappySupportedfunctions[index]] = SQL_TRUE;
      }
    }
  } else {
    *pfExists = SQL_FALSE;
    for (index = 0; index < supported_func_size; ++index) {
      if (snappySupportedfunctions[index] == fFunction) {
        *pfExists = SQL_TRUE;
        break;
      }
    }
  }
  FUNCTION_RETURN_HANDLE(hdbc, result);
}

SQLRETURN SQL_API SQLConnect(SQLHDBC connHandle, SQLCHAR* dsn,
    SQLSMALLINT dsnLen, SQLCHAR* userName, SQLSMALLINT userNameLen,
    SQLCHAR* passwd, SQLSMALLINT passwdLen) {
  FUNCTION_ENTER("InputHandle", connHandle,
      "DSN", StringFunctions::toString(dsn, dsnLen),
      "UserName", StringFunctions::toString(userName, userNameLen),
      "Password", StringFunctions::toString(passwd, passwdLen));
  SnappyConnection* conn = (SnappyConnection*)connHandle;
  if (conn) {
    std::string server;
    int port;
    // the Properties object to be passed to the underlying Connection
    Properties connProps;

    // lookup the data source name and get all the properties'
    if (!dsn || *dsn == 0 || dsnLen == 0) {
      dsn = (SQLCHAR*)SnappyDefaults::DEFAULT_DSN;
      dsnLen = SQL_NTS;
    }
    SQLRETURN result;
    std::string mdsn;
    try {
      ArrayIterator<std::string> allPropNames(OdbcIniKeys::ALL_PROPERTIES,
          OdbcIniKeys::NUM_ALL_PROPERTIES);
      IniPropertyReader<SQLCHAR> propReader;
      result = readProperties<SQLCHAR>(&propReader, dsn, dsnLen,
          &allPropNames, userName, userNameLen, passwd, passwdLen, server,
          port, connProps, conn);
      mdsn = propReader.getDSN();
    } catch (std::exception& se) {
      conn->setException(__FILE__, __LINE__, se);
      FUNCTION_RETURN_HANDLE(conn, SQL_ERROR);
    }
    if (result != SQL_ERROR) {
      result = conn->connect(server, port, connProps, mdsn, (SQLCHAR*)nullptr,
          -1, nullptr);
      FunctionTracer::trace("PROPS", nullptr, -1, "",
          "Server", server, "Port", port, "DSN", mdsn);
      FUNCTION_RETURN_HANDLE(conn, result);
    }
    FUNCTION_RETURN_HANDLE(conn, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_DBC);
}

SQLRETURN SQL_API SQLConnectW(SQLHDBC connHandle, SQLWCHAR* dsn,
    SQLSMALLINT dsnLen, SQLWCHAR* userName, SQLSMALLINT userNameLen,
    SQLWCHAR* passwd, SQLSMALLINT passwdLen) {
  FUNCTION_ENTER("InputHandle", connHandle,
      "DSN", StringFunctions::toString(dsn, dsnLen),
      "UserName", StringFunctions::toString(userName, userNameLen),
      "Password", StringFunctions::toString(passwd, passwdLen));
  SnappyConnection* conn = (SnappyConnection*)connHandle;
  if (conn) {
    std::string server;
    int port;
    // the Properties object to be passed to the underlying Connection
    Properties connProps;

    // lookup the data source name and get all the properties'
    if (!dsn || *dsn == 0 || dsnLen == 0) {
      dsn = (SQLWCHAR*)SnappyDefaults::DEFAULT_DSNW;
      dsnLen = SQL_NTS;
    }
    SQLRETURN result;
    std::string mdsn;
    try {
      ArrayIterator<std::string> allPropNames(OdbcIniKeys::ALL_PROPERTIES,
          OdbcIniKeys::NUM_ALL_PROPERTIES);
      IniPropertyReader<SQLWCHAR> propReader;
      result = readProperties(&propReader, dsn, dsnLen, &allPropNames,
          userName, userNameLen, passwd, passwdLen, server, port, connProps,
          conn);
      mdsn = propReader.getDSN();
    } catch (std::exception& se) {
      conn->setException(__FILE__, __LINE__, se);
      FUNCTION_RETURN_HANDLE(conn, SQL_ERROR);
    }
    if (result != SQL_ERROR) {
      result = conn->connect(server, port, connProps, mdsn, (SQLCHAR*)nullptr,
          -1, nullptr);
      FunctionTracer::trace("PROPS", nullptr, -1, "",
          "Server", server, "Port", port, "DSN", mdsn);
      FUNCTION_RETURN_HANDLE(conn, result);
    }
    FUNCTION_RETURN_HANDLE(conn, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_DBC);
}

SQLRETURN SQL_API SQLDriverConnect(SQLHDBC connHandle,
    SQLHWND windowHandle, SQLCHAR* connStr, SQLSMALLINT connStrLen,
    SQLCHAR* outConnStr, SQLSMALLINT outConnStrSize, SQLSMALLINT* outConnStrLen,
    SQLUSMALLINT completionFlag) {
  FUNCTION_ENTER("InputHandle", connHandle, "WindowHandle", windowHandle,
      "ConnStr", StringFunctions::toString(connStr, connStrLen),
      "CompletionFlag", completionFlag);
  SnappyConnection* conn = (SnappyConnection*)connHandle;
  SQLRETURN result;
  if (conn) {
    if (completionFlag != SQL_DRIVER_PROMPT) {
      std::string server;
      int port;
      // the Properties object to be passed to the underlying Connection
      Properties connProps;
      if (connStrLen <= 0 && connStrLen != SQL_NTS) {
        result = SnappyHandleBase::errorInvalidBufferLength(connStrLen,
            "Connection string length", conn);
        FUNCTION_RETURN_HANDLE(conn, result);
      }
      // split the given connection string to obtain the server, port and other
      // attributes
      std::string mdsn;
      try {
        ArrayIterator<std::string> allPropNames(OdbcIniKeys::ALL_PROPERTIES,
            OdbcIniKeys::NUM_ALL_PROPERTIES);
        ConnStringPropertyReader<SQLCHAR> connStrReader;
        result = readProperties<SQLCHAR>(&connStrReader, connStr, connStrLen,
            &allPropNames, nullptr, -1, nullptr, -1, server, port, connProps, conn);
        mdsn = connStrReader.getDSN();
      } catch (std::exception& se) {
        conn->setException(__FILE__, __LINE__, se);
        FUNCTION_RETURN_HANDLE(conn, SQL_ERROR);
      }
      if (result != SQL_ERROR) {
        result = conn->connect(server, port, connProps, mdsn, outConnStr,
            outConnStrSize, outConnStrLen);
        FunctionTracer::trace("PROPS", nullptr, -1, "",
            "Server", server, "Port", port, "DSN", mdsn);
        FUNCTION_RETURN_HANDLE(conn, result, "OutConnStr", outConnStr,
            "OutConnStrLen", outConnStrLen);
      }
    } else {
      result = SnappyHandleBase::errorNotImplemented(
          "SQLDriverConnect with prompt");
    }
    FUNCTION_RETURN_HANDLE(conn, result);
  } else {
    return SnappyHandleBase::errorNullHandle(SQL_HANDLE_DBC);
  }
}

SQLRETURN SQL_API SQLDriverConnectW(SQLHDBC connHandle,
    SQLHWND windowHandle, SQLWCHAR* connStr, SQLSMALLINT connStrLen,
    SQLWCHAR* outConnStr, SQLSMALLINT outConnStrSize,
    SQLSMALLINT* outConnStrLen, SQLUSMALLINT completionFlag) {
  FUNCTION_ENTER("InputHandle", connHandle, "WindowHandle", windowHandle,
      "ConnStr", StringFunctions::toString(connStr, connStrLen),
      "CompletionFlag", completionFlag);
  SnappyConnection* conn = (SnappyConnection*)connHandle;
  SQLRETURN result;
  if (conn) {
    if (completionFlag != SQL_DRIVER_PROMPT) {
      std::string server;
      int port;
      // the Properties object to be passed to the underlying Connection
      Properties connProps;
      if (connStrLen <= 0 && connStrLen != SQL_NTS) {
        result = SnappyHandleBase::errorInvalidBufferLength(connStrLen,
            "Connection string length", conn);
        FUNCTION_RETURN_HANDLE(conn, result);
      }
      // split the given connection string to obtain the server, port and other
      // attributes
      std::string mdsn;
      try {
        ArrayIterator<std::string> allPropNames(OdbcIniKeys::ALL_PROPERTIES,
            OdbcIniKeys::NUM_ALL_PROPERTIES);
        ConnStringPropertyReader<SQLWCHAR> connStrReader;
        result = readProperties(&connStrReader, connStr, connStrLen,
            &allPropNames, (const SQLWCHAR*)nullptr, -1,
            (const SQLWCHAR*)nullptr, -1, server, port, connProps, conn);
        mdsn = connStrReader.getDSN();
      } catch (std::exception& se) {
        conn->setException(__FILE__, __LINE__, se);
        FUNCTION_RETURN_HANDLE(conn, SQL_ERROR);
      }
      if (result != SQL_ERROR) {
        result = conn->connect(server, port, connProps, mdsn, outConnStr,
            outConnStrSize, outConnStrLen);
        FunctionTracer::trace("PROPS", nullptr, -1, "",
            "Server", server, "Port", port, "DSN", mdsn);
        FUNCTION_RETURN_HANDLE(conn, result, "OutConnStr", outConnStr,
            "OutConnStrLen", outConnStrLen);
      }
    } else {
      result = SnappyHandleBase::errorNotImplemented(
          "SQLDriverConnectW with prompt");
    }
    FUNCTION_RETURN_HANDLE(conn, result);
  } else {
    return SnappyHandleBase::errorNullHandle(SQL_HANDLE_DBC);
  }
}

SQLRETURN SQL_API SQLDisconnect(SQLHDBC connHandle) {
  FUNCTION_ENTER("InputHandle", connHandle);
  if (connHandle) {
    SnappyConnection* conn = (SnappyConnection*)connHandle;
    SQLRETURN result = conn->disconnect();
    FUNCTION_RETURN_HANDLE(conn, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_DBC);
}

SQLRETURN SQL_API SQLEndTran(SQLSMALLINT handleType, SQLHANDLE handle,
    SQLSMALLINT completionType) {
  FUNCTION_ENTER("HandleType", handleType, "InputHandle", handle,
      "CompletionType", completionType);
  if (handle) {
    SQLRETURN result;
    if (handleType == SQL_HANDLE_DBC) {
      SnappyConnection* conn = (SnappyConnection*)handle;
      if (!conn->getEnvironment()->isShared()) {
        switch (completionType) {
          case SQL_COMMIT:
            result = conn->commit();
            break;
          case SQL_ROLLBACK:
            result = conn->rollback();
            break;
          default:
            ((SnappyConnection*)handle)->setException(GET_SQLEXCEPTION2(
                SQLStateMessage::INVALID_TRANSACTION_OPERATION_MSG1,
                "completionType"));
            result = SQL_ERROR;
            break;
        }
      } else {
        ((SnappyConnection*)handle)->setException(GET_SQLEXCEPTION2(
            SQLStateMessage::INVALID_TRANSACTION_OPERATION_MSG2));
        result = SQL_ERROR;
      }
    } else if (handleType == SQL_HANDLE_ENV) {
      SnappyEnvironment* env = (SnappyEnvironment*)handle;
      if (!env->isShared()) {
        switch (completionType) {
          case SQL_COMMIT:
            result = env->forEachActiveConnection([](SnappyConnection *conn) {
              return conn->commit();
            });
            break;
          case SQL_ROLLBACK:
            result = env->forEachActiveConnection([](SnappyConnection *conn) {
              return conn->rollback();
            });
            break;
          default:
            ((SnappyConnection*)handle)->setException(
                GET_SQLEXCEPTION2(
                    SQLStateMessage::INVALID_TRANSACTION_OPERATION_MSG1,
                    "completionType"));
            result = SQL_ERROR;
        }
      } else {
        ((SnappyConnection*)handle)->setException(GET_SQLEXCEPTION2(
            SQLStateMessage::INVALID_TRANSACTION_OPERATION_MSG2));
        result = SQL_ERROR;
      }
    } else {
      ((SnappyHandleBase*)handle)->setException(GET_SQLEXCEPTION2(
          SQLStateMessage::INVALID_HANDLE_TYPE_MSG, handleType));
      result = SQL_ERROR;
    }
    FUNCTION_RETURN_HANDLE(handle, result);
  } else {
    return SnappyHandleBase::errorNullHandle(handleType);
  }
}

SQLRETURN SQL_API SQLSetConnectAttr(SQLHDBC connHandle,
    SQLINTEGER attribute, SQLPOINTER value, SQLINTEGER stringLength) {
  FUNCTION_ENTER("InputHandle", connHandle, "Attribute", attribute,
      "ValuePtr", value, "StringLength", stringLength);
  if (connHandle) {
    SnappyConnection* conn = (SnappyConnection*)connHandle;
    SQLRETURN result = conn->setAttribute(attribute, value, stringLength, true);
    FUNCTION_RETURN_HANDLE(conn, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_DBC);
}

SQLRETURN SQL_API SQLSetConnectAttrW(SQLHDBC connHandle,
    SQLINTEGER attribute, SQLPOINTER value, SQLINTEGER stringLength) {
  FUNCTION_ENTER("InputHandle", connHandle, "Attribute", attribute,
      "ValuePtr", value, "StringLength", stringLength);
  if (connHandle) {
    SnappyConnection* conn = (SnappyConnection*)connHandle;
    SQLRETURN result = conn->setAttribute(attribute, value, stringLength, false);
    FUNCTION_RETURN_HANDLE(conn, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_DBC);
}

SQLRETURN SQL_API SQLGetConnectAttr(SQLHDBC connHandle,
    SQLINTEGER attribute, SQLPOINTER resultValue, SQLINTEGER bufferLength,
    SQLINTEGER* stringLengthPtr) {
  FUNCTION_ENTER("InputHandle", connHandle, "Attribute", attribute,
      "BufferLength", bufferLength);
  if (connHandle) {
    SnappyConnection* conn = (SnappyConnection*)connHandle;
    SQLRETURN result = conn->getAttribute(attribute, resultValue, bufferLength,
        stringLengthPtr);
    FUNCTION_RETURN_HANDLE(conn, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_DBC);
}

SQLRETURN SQL_API SQLGetConnectAttrW(SQLHDBC connHandle,
    SQLINTEGER attribute, SQLPOINTER resultValue, SQLINTEGER bufferLength,
    SQLINTEGER* stringLengthPtr) {
  FUNCTION_ENTER("InputHandle", connHandle, "Attribute", attribute,
      "BufferLength", bufferLength);
  if (connHandle) {
    SnappyConnection* conn = (SnappyConnection*)connHandle;
    SQLRETURN result = conn->getAttributeW(attribute, resultValue, bufferLength,
        stringLengthPtr);
    FUNCTION_RETURN_HANDLE(conn, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_DBC);
}

SQLRETURN SQL_API SQLGetInfo(SQLHDBC connHandle,
    SQLUSMALLINT infoType, SQLPOINTER infoValue, SQLSMALLINT bufferLength,
    SQLSMALLINT* stringLength) {
  FUNCTION_ENTER("InputHandle", connHandle, "InfoType", infoType,
      "ValuePtr", infoValue, "BufferLength", bufferLength);
  if (connHandle) {
    SnappyConnection* conn = (SnappyConnection*)connHandle;
    SQLRETURN result = conn->getInfo(infoType, infoValue, bufferLength,
        stringLength);
    FUNCTION_RETURN_HANDLE(conn, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_DBC);
}

SQLRETURN SQL_API SQLGetInfoW(SQLHDBC connHandle,
    SQLUSMALLINT infoType, SQLPOINTER infoValue, SQLSMALLINT bufferLength,
    SQLSMALLINT* stringLength) {
  FUNCTION_ENTER("InputHandle", connHandle, "InfoType", infoType,
      "ValuePtr", infoValue, "BufferLength", bufferLength);
  if (connHandle) {
    SnappyConnection* conn = (SnappyConnection*)connHandle;
    SQLRETURN result = conn->getInfoW(infoType, infoValue, bufferLength,
        stringLength);
    FUNCTION_RETURN_HANDLE(conn, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_DBC);
}

SQLRETURN SQL_API SQLNativeSql(SQLHDBC connHandle,
    SQLCHAR* inStatementText, SQLINTEGER textLength1, SQLCHAR* outStatementText,
    SQLINTEGER bufferLength, SQLINTEGER* textLength2Ptr) {
  FUNCTION_ENTER("InputHandle", connHandle,
      "Statement", StringFunctions::toString(inStatementText, textLength1),
      "BufferLength", bufferLength);
  if (connHandle) {
    SnappyConnection* conn = (SnappyConnection*)connHandle;
    SQLRETURN result = conn->nativeSQL(inStatementText, textLength1,
        outStatementText, bufferLength, textLength2Ptr);
    FUNCTION_RETURN_HANDLE(conn, result, "OutStatement", outStatementText,
        "OutTextLength", textLength2Ptr);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_DBC);
}

SQLRETURN SQL_API SQLNativeSqlW(SQLHDBC connHandle,
    SQLWCHAR* inStatementText, SQLINTEGER textLength1,
    SQLWCHAR* outStatementText, SQLINTEGER bufferLength,
    SQLINTEGER* textLength2Ptr) {
  FUNCTION_ENTER("InputHandle", connHandle,
      "Statement", StringFunctions::toString(inStatementText, textLength1),
      "BufferLength", bufferLength);
  if (connHandle) {
    SnappyConnection* conn = (SnappyConnection*)connHandle;
    SQLRETURN result = conn->nativeSQLW(inStatementText, textLength1,
        outStatementText, bufferLength, textLength2Ptr);
    FUNCTION_RETURN_HANDLE(conn, result, "OutStatement", outStatementText,
        "OutTextLength", textLength2Ptr);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_DBC);
}

///////////////////////////////////////////////////////////////////////////////
////               Statement APIs
///////////////////////////////////////////////////////////////////////////////

SQLRETURN SQL_API SQLBindParameter(SQLHSTMT stmtHandle,
    SQLUSMALLINT paramNum, SQLSMALLINT inputOutputType, SQLSMALLINT valueType,
    SQLSMALLINT paramType, SQLULEN precision, SQLSMALLINT scale,
    SQLPOINTER paramValue, SQLLEN valueSize, SQLLEN* lenOrIndPtr) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "ParamNum", paramNum,
      "InputOutputType", inputOutputType, "ValueType", valueType,
      "ParamType", paramType, "Precision", precision, "Scale", scale,
      "ParamValuePtr", paramValue, "ValueSize", valueSize);
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->addParameter(paramNum,
        inputOutputType, valueType, paramType, precision, scale, paramValue,
        valueSize, lenOrIndPtr);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLPrepare(SQLHSTMT stmtHandle, SQLCHAR *stmtText,
    SQLINTEGER textLength) {
  FUNCTION_ENTER("InputHandle", stmtHandle,
      "Statement", StringFunctions::toString(stmtText, textLength));
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->prepare(stmtText,
        textLength);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLPrepareW(SQLHSTMT stmtHandle, SQLWCHAR *stmtText,
    SQLINTEGER textLength) {
  FUNCTION_ENTER("InputHandle", stmtHandle,
      "Statement", StringFunctions::toString(stmtText, textLength));
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->prepare(stmtText,
        textLength);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLExecDirect(SQLHSTMT stmtHandle,
    SQLCHAR* stmtText, SQLINTEGER textLength) {
  FUNCTION_ENTER("InputHandle", stmtHandle,
      "Statement", StringFunctions::toString(stmtText, textLength));
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->execute(stmtText,
        textLength);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLExecDirectW(SQLHSTMT stmtHandle,
    SQLWCHAR* stmtText, SQLINTEGER textLength) {
  FUNCTION_ENTER("InputHandle", stmtHandle,
      "Statement", StringFunctions::toString(stmtText, textLength));
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->execute(stmtText,
        textLength);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLExecute(SQLHSTMT stmtHandle) {
  FUNCTION_ENTER("InputHandle", stmtHandle);
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->execute();
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLBindCol(SQLHSTMT stmtHandle,
    SQLUSMALLINT columnNum, SQLSMALLINT targetType, SQLPOINTER targetValue,
    SQLLEN valueSize, SQLLEN *lenOrIndPtr) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "ColumnNum", columnNum,
      "TargetType", targetType, "ValueSize", valueSize);
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->bindOutputField(
        columnNum, targetType, targetValue, valueSize, lenOrIndPtr);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLRowCount(SQLHSTMT stmtHandle, SQLLEN* count) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "ResultCountPtr", count);
  if (stmtHandle) {
    if (count) {
      SQLRETURN result = ((SnappyStatement*)stmtHandle)->getUpdateCount(count);
      FUNCTION_RETURN_HANDLE(stmtHandle, result, "Count", count);
    }
    return SnappyHandleBase::errorNullHandle(SQL_PARAM_ERROR);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLFetch(SQLHSTMT stmtHandle) {
  FUNCTION_ENTER("InputHandle", stmtHandle);
  if (stmtHandle) {
    // TODO: what do docs say about multiple rowsets?
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->next();
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLFetchScroll(SQLHSTMT stmtHandle,
    SQLSMALLINT fetchOrientation, SQLLEN fetchOffset) {
  FUNCTION_ENTER("InputHandle", stmtHandle,
      "FetchOrientation", fetchOrientation, "FetchOffset", fetchOffset);
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->fetchScroll(
        fetchOrientation, fetchOffset);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLSetPos(SQLHSTMT stmtHandle,
    SQLSETPOSIROW rowNumber, SQLUSMALLINT operation, SQLUSMALLINT lockType) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "RowNumber", rowNumber,
      "Operation", operation, "LockType", lockType);
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->setPos(
        rowNumber, operation, lockType);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLGetData(SQLHSTMT stmtHandle,
    SQLUSMALLINT columnNum, SQLSMALLINT targetType, SQLPOINTER targetValue,
    SQLLEN valueSize, SQLLEN *lenOrIndPtr) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "ColumnNum", columnNum,
      "TargetType", targetType, "TargetValue", targetValue,
      "ValueSize", valueSize);
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->getData(
        columnNum, targetType, targetValue, valueSize, lenOrIndPtr);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLCloseCursor(SQLHSTMT stmtHandle) {
  FUNCTION_ENTER("InputHandle", stmtHandle);
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->closeResultSet(false);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLCancel(SQLHSTMT stmtHandle) {
  FUNCTION_ENTER("InputHandle", stmtHandle);
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->cancel();
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

// ODBC 3.8
extern "C" {
  SQLRETURN SQL_API SQLCancelHandle(SQLSMALLINT handleType, SQLHANDLE handle);
}

SQLRETURN SQL_API SQLCancelHandle(SQLSMALLINT handleType,
    SQLHANDLE handle) {
  FUNCTION_ENTER("HandleType", handleType, "InputHandle", handle);
  if (handle) {
    SQLRETURN result;
    if (handleType == SQL_HANDLE_STMT) {
      result = ((SnappyStatement*)handle)->cancel();
    } else if (handleType == SQL_HANDLE_DBC) {
      result = ((SnappyConnection*)handle)->cancelCurrentStatement();
    } else {
      ((SnappyHandleBase*)handle)->setException(GET_SQLEXCEPTION2(
          SQLStateMessage::INVALID_HANDLE_TYPE_MSG, handleType));
      result = SQL_ERROR;
    }
    FUNCTION_RETURN_HANDLE(handle, result);
  }
  return SnappyHandleBase::errorNullHandle(handleType);
}

SQLRETURN SQL_API SQLFreeStmt(SQLHSTMT stmtHandle,
    SQLUSMALLINT option) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "Option", option);
  if (stmtHandle) {
    SQLRETURN result = SnappyStatement::freeStatement(
        (SnappyStatement*)stmtHandle, option);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLGetStmtAttr(SQLHSTMT stmtHandle,
    SQLINTEGER attribute, SQLPOINTER valueBuffer, SQLINTEGER bufferLength,
    SQLINTEGER* valueLen) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "Attribute", attribute,
      "ValueBuffer", valueBuffer, "BufferLength", bufferLength);
  if (stmtHandle) {
    SQLRETURN result =  ((SnappyStatement*)stmtHandle)->getAttribute(
        attribute, valueBuffer, bufferLength, valueLen);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLGetStmtAttrW(SQLHSTMT stmtHandle,
    SQLINTEGER attribute, SQLPOINTER valueBuffer, SQLINTEGER bufferLen,
    SQLINTEGER* valueLen) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "Attribute", attribute,
      "ValueBuffer", valueBuffer, "BufferLength", bufferLen);
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->getAttributeW(
        attribute, valueBuffer, bufferLen, valueLen);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLSetStmtAttr(SQLHSTMT stmtHandle,
    SQLINTEGER attribute, SQLPOINTER valueBuffer, SQLINTEGER bufferLength) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "Attribute", attribute,
      "ValueBuffer", valueBuffer, "BufferLength", bufferLength);
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->setAttribute(
        attribute, valueBuffer, bufferLength);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLSetStmtAttrW(SQLHSTMT stmtHandle,
    SQLINTEGER attribute, SQLPOINTER valueBuffer, SQLINTEGER bufferLength) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "Attribute", attribute,
      "ValueBuffer", valueBuffer, "BufferLength", bufferLength);
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->setAttributeW(
        attribute, valueBuffer, bufferLength);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLDescribeCol(SQLHSTMT stmtHandle,
    SQLUSMALLINT columnNumber, SQLCHAR* columnName, SQLSMALLINT bufferLength,
    SQLSMALLINT* nameLength, SQLSMALLINT* dataType, SQLULEN* columnSize,
    SQLSMALLINT* decimalDigits, SQLSMALLINT* nullable) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "ColumnNumber", columnNumber);
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->getResultColumnDescriptor(
        columnNumber, columnName, bufferLength, nameLength, dataType,
        columnSize, decimalDigits, nullable);
    FUNCTION_RETURN_HANDLE(stmtHandle, result, "ColumnName", columnName,
        "NameLength", nameLength, "DataType", dataType,
        "ColumnSize", columnSize, "DecimalDigits", decimalDigits,
        "Nullable", nullable);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLDescribeColW(SQLHSTMT stmtHandle,
    SQLUSMALLINT columnNumber, SQLWCHAR* columnName, SQLSMALLINT bufferLength,
    SQLSMALLINT* nameLength, SQLSMALLINT* dataType, SQLULEN* columnSize,
    SQLSMALLINT* decimalDigits, SQLSMALLINT* nullable) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "ColumnNumber", columnNumber);
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->getResultColumnDescriptor(
        columnNumber, columnName, bufferLength, nameLength, dataType,
        columnSize, decimalDigits, nullable);
    FUNCTION_RETURN_HANDLE(stmtHandle, result, "ColumnName", columnName,
        "NameLength", nameLength, "DataType", dataType,
        "ColumnSize", columnSize, "DecimalDigits", decimalDigits,
        "Nullable", nullable);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLDescribeParam(SQLHSTMT stmtHandle,
    SQLUSMALLINT paramNumber, SQLSMALLINT* paramDataTypePtr,
    SQLULEN* paramSizePtr, SQLSMALLINT* decimalDigitsPtr,
    SQLSMALLINT* nullablePtr) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "ParamNumber", paramNumber);
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->getParamMetadata(
        paramNumber, paramDataTypePtr, paramSizePtr, decimalDigitsPtr,
        nullablePtr);
    FUNCTION_RETURN_HANDLE(stmtHandle, result, "DataType", paramDataTypePtr,
        "ParamSize", paramSizePtr, "DecimalDigits", decimalDigitsPtr,
        "Nullable", nullablePtr);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

#if defined(_WINDOWS) && !defined(_WIN64)
SQLRETURN SQL_API SQLColAttribute(SQLHSTMT stmtHandle,
    SQLUSMALLINT columnNumber, SQLUSMALLINT fieldId, SQLPOINTER charAttribute,
    SQLSMALLINT bufferLength, SQLSMALLINT* stringLength,
    SQLPOINTER numericAttribute) {
#else
SQLRETURN SQL_API SQLColAttribute(SQLHSTMT stmtHandle,
    SQLUSMALLINT columnNumber, SQLUSMALLINT fieldId, SQLPOINTER charAttribute,
    SQLSMALLINT bufferLength, SQLSMALLINT* stringLength,
    SQLLEN* numericAttribute) {
#endif
  FUNCTION_ENTER("InputHandle", stmtHandle, "ColumnNumber", columnNumber,
      "FieldId", fieldId, "CharAttributePtr", charAttribute,
      "BufferLength", bufferLength);
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->getColumnAttribute(
        columnNumber, fieldId, charAttribute, bufferLength, stringLength,
        (SQLLEN*)numericAttribute);
    FUNCTION_RETURN_HANDLE(stmtHandle, result,
        "NumericAttribute", (SQLLEN*)numericAttribute);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

#if defined(_WINDOWS) && !defined(_WIN64)
SQLRETURN SQL_API SQLColAttributeW(SQLHSTMT stmtHandle,
    SQLUSMALLINT columnNumber, SQLUSMALLINT fieldId, SQLPOINTER charAttribute,
    SQLSMALLINT bufferLength, SQLSMALLINT* stringLength,
    SQLPOINTER numericAttribute) {
#else
SQLRETURN SQL_API SQLColAttributeW(SQLHSTMT stmtHandle,
    SQLUSMALLINT columnNumber, SQLUSMALLINT fieldId, SQLPOINTER charAttribute,
    SQLSMALLINT bufferLength, SQLSMALLINT* stringLength,
    SQLLEN* numericAttribute) {
#endif
  FUNCTION_ENTER("InputHandle", stmtHandle, "ColumnNumber", columnNumber,
      "FieldId", fieldId, "CharAttributePtr", charAttribute,
      "BufferLength", bufferLength);
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->getColumnAttributeW(
        columnNumber, fieldId, charAttribute, bufferLength, stringLength,
        (SQLLEN*)numericAttribute);
    FUNCTION_RETURN_HANDLE(stmtHandle, result,
        "NumericAttribute", (SQLLEN*)numericAttribute);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLGetCursorName(SQLHSTMT stmtHandle,
    SQLCHAR* cursorName, SQLSMALLINT bufferLength, SQLSMALLINT* nameLength) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "BufferLength", bufferLength);
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->getCursorName(
        cursorName, bufferLength, nameLength);
    FUNCTION_RETURN_HANDLE(stmtHandle, result, "CursorName", cursorName,
        "NameLength", nameLength);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLGetCursorNameW(SQLHSTMT stmtHandle,
    SQLWCHAR* cursorName, SQLSMALLINT bufferLength, SQLSMALLINT* nameLength) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "BufferLength", bufferLength);
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->getCursorName(
        cursorName, bufferLength, nameLength);
    FUNCTION_RETURN_HANDLE(stmtHandle, result, "CursorName", cursorName,
        "NameLength", nameLength);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLSetCursorName(SQLHSTMT stmtHandle,
    SQLCHAR* cursorName, SQLSMALLINT nameLength) {
  FUNCTION_ENTER("InputHandle", stmtHandle,
      "CursorName", StringFunctions::toString(cursorName, nameLength));
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->setCursorName(
        cursorName, nameLength);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLSetCursorNameW(SQLHSTMT stmtHandle,
    SQLWCHAR* cursorName, SQLSMALLINT nameLength) {
  FUNCTION_ENTER("InputHandle", stmtHandle,
      "CursorName", StringFunctions::toString(cursorName, nameLength));
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->setCursorName(
        cursorName, nameLength);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLNumResultCols(SQLHSTMT stmtHandle,
    SQLSMALLINT* columnCount) {
  FUNCTION_ENTER("InputHandle", stmtHandle);
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->getNumResultColumns(
        columnCount);
    FUNCTION_RETURN_HANDLE(stmtHandle, result, "ColumnCount", columnCount);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLNumParams(SQLHSTMT stmtHandle,
    SQLSMALLINT* paramCount) {
  FUNCTION_ENTER("InputHandle", stmtHandle);
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->getNumParameters(
        paramCount);
    FUNCTION_RETURN_HANDLE(stmtHandle, result, "ParameterCount", paramCount);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLTables(SQLHSTMT stmtHandle, SQLCHAR* catalogName,
    SQLSMALLINT nameLength1, SQLCHAR* schemaName, SQLSMALLINT nameLength2,
    SQLCHAR* tableName, SQLSMALLINT nameLength3, SQLCHAR* tableTypes,
    SQLSMALLINT nameLength4) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "Catalog",
      StringFunctions::toString(catalogName, nameLength1), "Schema",
      StringFunctions::toString(schemaName, nameLength2), "Table",
      StringFunctions::toString(tableName, nameLength3), "TableTypes",
      StringFunctions::toString(tableTypes, nameLength4));
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->getTables(schemaName,
        nameLength2, tableName, nameLength3, tableTypes, nameLength4);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLTablesW(SQLHSTMT stmtHandle, SQLWCHAR* catalogName,
    SQLSMALLINT nameLength1, SQLWCHAR* schemaName, SQLSMALLINT nameLength2,
    SQLWCHAR* tableName, SQLSMALLINT nameLength3, SQLWCHAR* tableTypes,
    SQLSMALLINT nameLength4) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "Catalog",
      StringFunctions::toString(catalogName, nameLength1), "Schema",
      StringFunctions::toString(schemaName, nameLength2), "Table",
      StringFunctions::toString(tableName, nameLength3), "TableTypes",
      StringFunctions::toString(tableTypes, nameLength4));
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->getTables(schemaName,
        nameLength2, tableName, nameLength3, tableTypes, nameLength4);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLTablePrivileges(SQLHSTMT stmtHandle,
    SQLCHAR* catalogName, SQLSMALLINT nameLength1, SQLCHAR* schemaName,
    SQLSMALLINT nameLength2, SQLCHAR* tableName, SQLSMALLINT nameLength3) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "Catalog",
      StringFunctions::toString(catalogName, nameLength1), "Schema",
      StringFunctions::toString(schemaName, nameLength2), "Table",
      StringFunctions::toString(tableName, nameLength3));
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->getTablePrivileges(
        schemaName, nameLength2, tableName, nameLength3);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLTablePrivilegesW(SQLHSTMT stmtHandle,
    SQLWCHAR* catalogName, SQLSMALLINT nameLength1, SQLWCHAR* schemaName,
    SQLSMALLINT nameLength2, SQLWCHAR* tableName, SQLSMALLINT nameLength3) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "Catalog",
      StringFunctions::toString(catalogName, nameLength1), "Schema",
      StringFunctions::toString(schemaName, nameLength2), "Table",
      StringFunctions::toString(tableName, nameLength3));
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->getTablePrivileges(
        schemaName, nameLength2, tableName, nameLength3);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLColumns(SQLHSTMT stmtHandle,
    SQLCHAR* catalogName, SQLSMALLINT nameLength1, SQLCHAR* schemaName,
    SQLSMALLINT nameLength2, SQLCHAR* tableName, SQLSMALLINT nameLength3,
    SQLCHAR* columnName, SQLSMALLINT nameLength4) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "Catalog",
      StringFunctions::toString(catalogName, nameLength1), "Schema",
      StringFunctions::toString(schemaName, nameLength2), "Table",
      StringFunctions::toString(tableName, nameLength3), "Column",
      StringFunctions::toString(columnName, nameLength4));
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->getColumns(schemaName,
        nameLength2, tableName, nameLength3, columnName, nameLength4);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLColumnsW(SQLHSTMT stmtHandle,
    SQLWCHAR* catalogName, SQLSMALLINT nameLength1, SQLWCHAR* schemaName,
    SQLSMALLINT nameLength2, SQLWCHAR* tableName, SQLSMALLINT nameLength3,
    SQLWCHAR* columnName, SQLSMALLINT nameLength4) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "Catalog",
      StringFunctions::toString(catalogName, nameLength1), "Schema",
      StringFunctions::toString(schemaName, nameLength2), "Table",
      StringFunctions::toString(tableName, nameLength3), "Column",
      StringFunctions::toString(columnName, nameLength4));
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->getColumns(schemaName,
        nameLength2, tableName, nameLength3, columnName, nameLength4);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLColumnPrivileges(SQLHSTMT stmtHandle, SQLCHAR* catalogName,
    SQLSMALLINT nameLength1, SQLCHAR* schemaName, SQLSMALLINT nameLength2,
    SQLCHAR* tableName, SQLSMALLINT nameLength3, SQLCHAR* columnName,
    SQLSMALLINT nameLength4) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "Catalog",
      StringFunctions::toString(catalogName, nameLength1), "Schema",
      StringFunctions::toString(schemaName, nameLength2), "Table",
      StringFunctions::toString(tableName, nameLength3), "Column",
      StringFunctions::toString(columnName, nameLength4));
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->getColumnPrivileges(
        schemaName, nameLength2, tableName, nameLength3, columnName,
        nameLength4);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLColumnPrivilegesW(SQLHSTMT stmtHandle,
    SQLWCHAR* catalogName, SQLSMALLINT nameLength1, SQLWCHAR* schemaName,
    SQLSMALLINT nameLength2, SQLWCHAR* tableName, SQLSMALLINT nameLength3,
    SQLWCHAR* columnName, SQLSMALLINT nameLength4) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "Catalog",
      StringFunctions::toString(catalogName, nameLength1), "Schema",
      StringFunctions::toString(schemaName, nameLength2), "Table",
      StringFunctions::toString(tableName, nameLength3), "Column",
      StringFunctions::toString(columnName, nameLength4));
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->getColumnPrivileges(
        schemaName, nameLength2, tableName, nameLength3, columnName,
        nameLength4);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLStatistics(SQLHSTMT stmtHandle,
    SQLCHAR* catalogName, SQLSMALLINT nameLength1, SQLCHAR* schemaName,
    SQLSMALLINT nameLength2, SQLCHAR* tableName, SQLSMALLINT nameLength3,
    SQLUSMALLINT unique, SQLUSMALLINT reserved) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "Catalog",
      StringFunctions::toString(catalogName, nameLength1), "Schema",
      StringFunctions::toString(schemaName, nameLength2), "Table",
      StringFunctions::toString(tableName, nameLength3), "Unique", unique,
      "Reserved", reserved);
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->getIndexInfo(schemaName,
        nameLength2, tableName, nameLength3, unique == SQL_INDEX_UNIQUE,
        reserved == SQL_QUICK);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLStatisticsW(SQLHSTMT stmtHandle,
    SQLWCHAR* catalogName, SQLSMALLINT nameLength1, SQLWCHAR* schemaName,
    SQLSMALLINT nameLength2, SQLWCHAR* tableName, SQLSMALLINT nameLength3,
    SQLUSMALLINT unique, SQLUSMALLINT reserved) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "Catalog",
      StringFunctions::toString(catalogName, nameLength1), "Schema",
      StringFunctions::toString(schemaName, nameLength2), "Table",
      StringFunctions::toString(tableName, nameLength3), "Unique", unique,
      "Reserved", reserved);
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->getIndexInfo(schemaName,
        nameLength2, tableName, nameLength3, unique == SQL_INDEX_UNIQUE,
        reserved == SQL_QUICK);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLPrimaryKeys(SQLHSTMT stmtHandle,
    SQLCHAR* catalogName, SQLSMALLINT nameLength1, SQLCHAR* schemaName,
    SQLSMALLINT nameLength2, SQLCHAR* tableName, SQLSMALLINT nameLength3) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "Catalog",
      StringFunctions::toString(catalogName, nameLength1), "Schema",
      StringFunctions::toString(schemaName, nameLength2), "Table",
      StringFunctions::toString(tableName, nameLength3));
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->getPrimaryKeys(
        schemaName, nameLength2, tableName, nameLength3);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLPrimaryKeysW(SQLHSTMT stmtHandle,
    SQLWCHAR* catalogName, SQLSMALLINT nameLength1, SQLWCHAR* schemaName,
    SQLSMALLINT nameLength2, SQLWCHAR* tableName, SQLSMALLINT nameLength3) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "Catalog",
      StringFunctions::toString(catalogName, nameLength1), "Schema",
      StringFunctions::toString(schemaName, nameLength2), "Table",
      StringFunctions::toString(tableName, nameLength3));
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->getPrimaryKeys(
        schemaName, nameLength2, tableName, nameLength3);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLForeignKeys(SQLHSTMT stmtHandle,
    SQLCHAR* pkCatalogName, SQLSMALLINT nameLength1, SQLCHAR* pkSchemaName,
    SQLSMALLINT nameLength2, SQLCHAR* pkTableName, SQLSMALLINT nameLength3,
    SQLCHAR* fkCatalogName, SQLSMALLINT nameLength4, SQLCHAR* fkSchemaName,
    SQLSMALLINT nameLength5, SQLCHAR* fkTableName, SQLSMALLINT nameLength6) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "PrimaryCatalog",
      StringFunctions::toString(pkCatalogName, nameLength1), "PrimarySchema",
      StringFunctions::toString(pkSchemaName, nameLength2), "PrimaryTable",
      StringFunctions::toString(pkTableName, nameLength3), "ForeignCatalog",
      StringFunctions::toString(fkCatalogName, nameLength1), "ForeignSchema",
      StringFunctions::toString(fkSchemaName, nameLength2), "ForeignTable",
      StringFunctions::toString(fkTableName, nameLength3));
  if (stmtHandle) {
    SQLRETURN result;
    SnappyStatement* stmt = (SnappyStatement*)stmtHandle;
    if (pkTableName) {
      if (fkTableName) {
        result = stmt->getCrossReference(pkSchemaName, nameLength2, pkTableName,
            nameLength3, fkSchemaName, nameLength5, fkTableName, nameLength6);
        FUNCTION_RETURN_HANDLE(stmtHandle, result);
      } else {
        result = stmt->getExportedKeys(pkSchemaName, nameLength2, pkTableName,
            nameLength3);
      }
    } else {
      // both nullptr should never happen since DriverManager is supposed to
      // handle it
      result = stmt->getImportedKeys(fkSchemaName, nameLength2, fkTableName,
          nameLength3);
    }
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLForeignKeysW(SQLHSTMT stmtHandle,
    SQLWCHAR* pkCatalogName, SQLSMALLINT nameLength1, SQLWCHAR* pkSchemaName,
    SQLSMALLINT nameLength2, SQLWCHAR* pkTableName, SQLSMALLINT nameLength3,
    SQLWCHAR* fkCatalogName, SQLSMALLINT nameLength4, SQLWCHAR* fkSchemaName,
    SQLSMALLINT nameLength5, SQLWCHAR* fkTableName, SQLSMALLINT nameLength6) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "PrimaryCatalog",
      StringFunctions::toString(pkCatalogName, nameLength1), "PrimarySchema",
      StringFunctions::toString(pkSchemaName, nameLength2), "PrimaryTable",
      StringFunctions::toString(pkTableName, nameLength3), "ForeignCatalog",
      StringFunctions::toString(fkCatalogName, nameLength1), "ForeignSchema",
      StringFunctions::toString(fkSchemaName, nameLength2), "ForeignTable",
      StringFunctions::toString(fkTableName, nameLength3));
  if (stmtHandle) {
    SQLRETURN result;
    SnappyStatement* stmt = (SnappyStatement*)stmtHandle;
    if (pkTableName) {
      if (fkTableName) {
        result = stmt->getCrossReference(pkSchemaName, nameLength2, pkTableName,
            nameLength3, fkSchemaName, nameLength5, fkTableName, nameLength6);
      } else {
        result = stmt->getExportedKeys(pkSchemaName, nameLength2, pkTableName,
            nameLength3);
      }
    } else {
      // both nullptr should never happen since DriverManager is supposed to
      // handle it
      result = stmt->getImportedKeys(fkSchemaName, nameLength2, fkTableName,
          nameLength3);
    }
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLProcedures(SQLHSTMT stmtHandle,
    SQLCHAR* catalogName, SQLSMALLINT nameLength1, SQLCHAR* schemaPattern,
    SQLSMALLINT nameLength2, SQLCHAR* procedureNamePattern,
    SQLSMALLINT nameLength3) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "Catalog",
      StringFunctions::toString(catalogName, nameLength1), "SchemaPattern",
      StringFunctions::toString(schemaPattern, nameLength2), "ProcedurePattern",
      StringFunctions::toString(procedureNamePattern, nameLength3));
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->getProcedures(
        schemaPattern, nameLength2, procedureNamePattern, nameLength3);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLProceduresW(SQLHSTMT stmtHandle,
    SQLWCHAR* catalogName, SQLSMALLINT nameLength1, SQLWCHAR* schemaPattern,
    SQLSMALLINT nameLength2, SQLWCHAR* procedureNamePattern,
    SQLSMALLINT nameLength3) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "Catalog",
      StringFunctions::toString(catalogName, nameLength1), "SchemaPattern",
      StringFunctions::toString(schemaPattern, nameLength2), "ProcedurePattern",
      StringFunctions::toString(procedureNamePattern, nameLength3));
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->getProcedures(
        schemaPattern, nameLength2, procedureNamePattern, nameLength3);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLProcedureColumns(SQLHSTMT stmtHandle,
    SQLCHAR* catalogName, SQLSMALLINT nameLength1, SQLCHAR* schemaPattern,
    SQLSMALLINT nameLength2, SQLCHAR* procedureNamePattern,
    SQLSMALLINT nameLength3, SQLCHAR* columnNamePattern,
    SQLSMALLINT nameLength4) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "Catalog",
      StringFunctions::toString(catalogName, nameLength1), "SchemaPattern",
      StringFunctions::toString(schemaPattern, nameLength2), "ProcedurePattern",
      StringFunctions::toString(procedureNamePattern, nameLength3),
      "ColumnPattern",
      StringFunctions::toString(columnNamePattern, nameLength4));
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->getProcedureColumns(
        schemaPattern, nameLength2, procedureNamePattern, nameLength3,
        columnNamePattern, nameLength4);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLProcedureColumnsW(SQLHSTMT stmtHandle,
    SQLWCHAR* catalogName, SQLSMALLINT nameLength1, SQLWCHAR* schemaPattern,
    SQLSMALLINT nameLength2, SQLWCHAR* procedureNamePattern,
    SQLSMALLINT nameLength3, SQLWCHAR* columnNamePattern,
    SQLSMALLINT nameLength4) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "Catalog",
      StringFunctions::toString(catalogName, nameLength1), "SchemaPattern",
      StringFunctions::toString(schemaPattern, nameLength2), "ProcedurePattern",
      StringFunctions::toString(procedureNamePattern, nameLength3),
      "ColumnPattern",
      StringFunctions::toString(columnNamePattern, nameLength4));
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->getProcedureColumns(
        schemaPattern, nameLength2, procedureNamePattern, nameLength3,
        columnNamePattern, nameLength4);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLGetTypeInfo(SQLHSTMT stmtHandle,
    SQLSMALLINT dataType) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "DataType", dataType);
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->getTypeInfo(dataType);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLGetTypeInfoW(SQLHSTMT stmtHandle,
    SQLSMALLINT dataType) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "DataType", dataType);
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->getTypeInfoW(dataType);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLBulkOperations(SQLHSTMT stmtHandle,
    SQLSMALLINT operation) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "Operation", operation);
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->bulkOperations(
        operation);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLPutData(SQLHSTMT stmtHandle, SQLPOINTER dataPtr,
    SQLLEN strLen) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "DataPtr", dataPtr,
      "StringLength", strLen);
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->putData(dataPtr, strLen);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLMoreResults(SQLHSTMT stmtHandle) {
  FUNCTION_ENTER("InputHandle", stmtHandle);
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->getMoreResults();
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLParamData(SQLHSTMT stmtHandle,
    SQLPOINTER* valuePtr) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "ValuePtr", valuePtr);
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->getParamData(valuePtr);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLSpecialColumns(SQLHSTMT stmtHandle,
    SQLUSMALLINT identifierType, SQLCHAR* catalogName, SQLSMALLINT nameLength1,
    SQLCHAR* schemaName, SQLSMALLINT nameLength2, SQLCHAR* tableName,
    SQLSMALLINT nameLength3, SQLUSMALLINT scope, SQLUSMALLINT nullable) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "IdentifierType", identifierType,
      "Catalog", StringFunctions::toString(catalogName, nameLength1), "Schema",
      StringFunctions::toString(schemaName, nameLength2), "Table",
      StringFunctions::toString(tableName, nameLength3), "Scope", scope,
      "Nullable", nullable);
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->getSpecialColumns(
        identifierType, schemaName, nameLength2, tableName, nameLength3, scope,
        nullable);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}

SQLRETURN SQL_API SQLSpecialColumnsW(SQLHSTMT stmtHandle,
    SQLUSMALLINT identifierType, SQLWCHAR* catalogName, SQLSMALLINT nameLength1,
    SQLWCHAR* schemaName, SQLSMALLINT nameLength2, SQLWCHAR* tableName,
    SQLSMALLINT nameLength3, SQLUSMALLINT scope, SQLUSMALLINT nullable) {
  FUNCTION_ENTER("InputHandle", stmtHandle, "IdentifierType", identifierType,
      "Catalog", StringFunctions::toString(catalogName, nameLength1), "Schema",
      StringFunctions::toString(schemaName, nameLength2), "Table",
      StringFunctions::toString(tableName, nameLength3), "Scope", scope,
      "Nullable", nullable);
  if (stmtHandle) {
    SQLRETURN result = ((SnappyStatement*)stmtHandle)->getSpecialColumns(
        identifierType, schemaName, nameLength2, tableName, nameLength3,
        scope, nullable);
    FUNCTION_RETURN_HANDLE(stmtHandle, result);
  }
  return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
}
