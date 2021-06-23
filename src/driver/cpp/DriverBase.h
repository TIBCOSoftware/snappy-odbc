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
 * DriverBase.h
 */

#ifndef DRIVERBASE_H_
#define DRIVERBASE_H_

#include "OdbcBase.h"

#include <string>
#include <vector>

extern "C" {
#include <stdio.h>
}

#include <Types.h>
#include <SQLException.h>
#include <Utils.h>
#include <ClientBase.h>

#include <atomic>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include "StringFunctions.h"

using namespace io::snappydata::client;

/////////////////////////////////////////////////////////////////////
///     global defines
/////////////////////////////////////////////////////////////////////

/** Driver version, name etc */
#define ODBC_PRODUCT_NAME        "SnappyData"
#define ODBC_OLD_PRODUCT_NAME    "TIBCO ComputeDB"
#define SNAPPY_DRIVER_SECTION    "SnappyData"
#define SNAPPY_DBMS_NAME         "SnappyData"
#define SNAPPY_DATABASE_NAME     "snappydata"
#ifndef ODBC_DRIVER_VERSION
#define ODBC_DRIVER_VERSION      "01.03.0000"
#endif
#ifndef SNAPPY_DBMS_VERSION
#define SNAPPY_DBMS_VERSION      "01.06.0600"
#endif
#ifdef _WINDOWS
#define ODBC_DRIVER_NAME         "snappyodbc.dll"
#elif defined(_MACOSX)
#define ODBC_DRIVER_NAME         "libsnappyodbc.dylib"
#else
#define ODBC_DRIVER_NAME         "libsnappyodbc.so"
#endif
#define DEFAULT_DSN_NAME         "Default"

#define SQL_PRODUCT_NAME         50001

#define SNAPPY_GLOBAL_ERROR io::snappydata::SnappyHandleBase::lastGlobalError()

#ifdef _WINDOWS
#define FUNCTION_LOG(tag, ...) \
  io::snappydata::FunctionTracer::trace(tag, __FILE__, __LINE__, __FUNCSIG__, __VA_ARGS__)
#define FUNCTION_ENTER(...) FUNCTION_LOG("ENTER", ##__VA_ARGS__)
#define FUNCTION_RETURN(r, ...) \
  if (io::snappydata::client::LogWriter::debugEnabled()) { \
    if (SQL_SUCCEEDED(r)) { \
      io::snappydata::FunctionTracer::traceExit("EXIT", __FILE__, __LINE__, __FUNCSIG__, \
          r, __VA_ARGS__); \
    } else if (SNAPPY_GLOBAL_ERROR) { \
      FUNCTION_LOG("EXIT", "ERROR", SNAPPY_GLOBAL_ERROR->toString(), "Result", r); \
    } else { \
      FUNCTION_LOG("EXIT", "Result", r); \
    } \
  } \
  return r
#define FUNCTION_RETURN_HANDLE(h, r, ...) \
  if (io::snappydata::client::LogWriter::debugEnabled()) { \
    if (SQL_SUCCEEDED(r)) { \
      io::snappydata::FunctionTracer::traceExit("EXIT", __FILE__, __LINE__, __FUNCSIG__, \
          r, __VA_ARGS__); \
    } else if (h && ((SnappyHandleBase*)h)->lastError()) { \
      FUNCTION_LOG("EXIT", "ERROR", ((SnappyHandleBase*)h)->lastError()->toString(), "Result", r); \
    } else if (SNAPPY_GLOBAL_ERROR) { \
      FUNCTION_LOG("EXIT", "ERROR", SNAPPY_GLOBAL_ERROR->toString(), "Result", r); \
    } else { \
      FUNCTION_LOG("EXIT", "Result", r); \
    } \
  } \
  return r
#else
#define FUNCTION_LOG(tag, ...) \
  io::snappydata::FunctionTracer::trace(tag, __FILE__, __LINE__, __PRETTY_FUNCTION__, ##__VA_ARGS__)
#define FUNCTION_ENTER(...) FUNCTION_LOG("ENTER", ##__VA_ARGS__)
#define FUNCTION_RETURN(r, ...) \
  if (io::snappydata::client::LogWriter::debugEnabled()) { \
    if (SQL_SUCCEEDED(r)) { \
      io::snappydata::FunctionTracer::traceExit("EXIT", __FILE__, __LINE__, __PRETTY_FUNCTION__, \
          r, ##__VA_ARGS__); \
    } else if (SNAPPY_GLOBAL_ERROR) { \
      FUNCTION_LOG("EXIT", "ERROR", SNAPPY_GLOBAL_ERROR->toString(), "Result", r); \
    } else { \
      FUNCTION_LOG("EXIT", "Result", r); \
    } \
  } \
  return r
#define FUNCTION_RETURN_HANDLE(h, r, ...) \
  if (io::snappydata::client::LogWriter::debugEnabled()) { \
    if (SQL_SUCCEEDED(r)) { \
      io::snappydata::FunctionTracer::traceExit("EXIT", __FILE__, __LINE__, __PRETTY_FUNCTION__, \
          r, ##__VA_ARGS__); \
    } else if (h && ((SnappyHandleBase*)h)->lastError()) { \
      FUNCTION_LOG("EXIT", "ERROR", ((SnappyHandleBase*)h)->lastError()->toString(), "Result", r); \
    } else if (SNAPPY_GLOBAL_ERROR) { \
      FUNCTION_LOG("EXIT", "ERROR", SNAPPY_GLOBAL_ERROR->toString(), "Result", r); \
    } else { \
      FUNCTION_LOG("EXIT", "Result", r); \
    } \
  } \
  return r
#endif

namespace io {
namespace snappydata {

  class SnappyEnvironment;

  class FunctionTracer {
  private:
    static void writeArgs(std::ostream& out) {
    }

    static void writeArgs(std::ostream& out, const char* name,
        const SQLCHAR*& value) {
      if (value) {
        out << '\t' << name << " = " << value << LogWriter::NEWLINE;
      } else {
        out << '\t' << name << " POINTER IS NULL" << LogWriter::NEWLINE;
      }
    }

    static void writeArgs(std::ostream& out, const char* name,
        const SQLWCHAR*& value) {
      if (value) {
      out << '\t' << name << " = " << StringFunctions::toString(value, SQL_NTS)
          << LogWriter::NEWLINE;
      } else {
        out << '\t' << name << " POINTER IS NULL" << LogWriter::NEWLINE;
      }
    }

    template<typename V>
    static void writeArgs(std::ostream& out, const char* name,
        const V& value) {
      out << '\t' << name << " = " << value << LogWriter::NEWLINE;
    }

    template<typename V, typename... T>
    static void writeArgs(std::ostream& out, const char* name,
        const V& value, const T... rest) {
      writeArgs(out, name, value);
      writeArgs(out, rest...);
    }

    static void writeOutArgs(std::ostream& out) {
    }

    static void writeOutArgs(std::ostream& out, const char* name,
        const SQLCHAR* value) {
      writeArgs(out, name, value);
    }

    static void writeOutArgs(std::ostream& out, const char* name,
        const SQLWCHAR* value) {
      writeArgs(out, name, value);
    }

    template<typename V>
    static void writeOutArgs(std::ostream& out, const char* name,
        const V* value) {
      if (value) {
        out << '\t' << name << " = " << *value << LogWriter::NEWLINE;
      } else {
        out << '\t' << name << " POINTER IS NULL" << LogWriter::NEWLINE;
      }
    }

    template<typename V, typename... T>
    static void writeOutArgs(std::ostream& out, const char* name,
        const V* value, const T... rest) {
      writeOutArgs(out, name, value);
      writeOutArgs(out, rest...);
    }

  public:
    template<typename... T>
    static void trace(const char* tag, const char* file, int line,
        const char* funcName, const T... args) {
      if (LogWriter::debugEnabled()) {
        std::ostream& out = LogWriter::debug();
        if (file) {
          out << LogWriter::NEWLINE << tag << ' ' << funcName << " at (" << file
              << ':' << line << "):" << LogWriter::NEWLINE;
        } else {
          out << LogWriter::NEWLINE;
        }
        writeArgs(out, args...);
      }
    }

    template<typename... T>
    static void traceExit(const char* tag, const char* file, int line,
        const char* funcName, const SQLRETURN result, const T... args) {
      if (LogWriter::debugEnabled()) {
        std::ostream& out = LogWriter::debug();
        out << LogWriter::NEWLINE << tag << ' ' << funcName << " at (" << file
            << ':' << line << "):" << LogWriter::NEWLINE;
        writeOutArgs(out, args...);
        out << "\tResult = " << result << LogWriter::NEWLINE;
      }
    }
  };

  class SnappyHandleBase {
  private:
    static std::atomic<SQLException*> s_globalError;

    std::unique_ptr<SQLException> m_lastError;

    static SQLException* getUnknownException(const char* file, int line,
        std::exception& ex);

    static void setGlobalException(SQLException* ex);

  protected:
    /** the lock to protect concurrent actions on the handle */
    std::mutex m_connLock;

    inline SnappyHandleBase() : m_lastError() {
    }

    virtual ~SnappyHandleBase() {
    }

    friend class SnappyEnvironment;

  public:
    static void setGlobalException(SQLException& ex);
    static void setGlobalException(SQLException&& ex);

    static void setGlobalException(const char* file, int line,
        std::exception& ex);

    static SQLException* lastGlobalError() noexcept;
    static void clearLastGlobalError() noexcept;

    void setException(SQLException& ex);
    void setException(SQLException&& ex);
    void setException(const char* file, int line, std::exception& ex);

    void setSQLWarning(SQLWarning& warning);

    SQLException* lastError() const noexcept;
    void clearLastError() noexcept;

    /** Common utility to handle a nullptr passed in handle. */
    static SQLRETURN errorNullHandle(SQLSMALLINT handleType);
    /** Common utility to handle a nullptr passed in handle. */
    static SQLRETURN errorNullHandle(SQLSMALLINT handleType,
        SnappyHandleBase* handle);

    /** Return error condition for an API function not implemented. */
    static SQLRETURN errorNotImplemented(const char* function);
    /** Return error condition for an API function not implemented. */
    static SQLRETURN errorNotImplemented(const char* function,
        SnappyHandleBase* handle);

    /** Return error condition for an invalid input string/buffer length */
    static SQLRETURN errorInvalidBufferLength(int length, const char* name,
        SnappyHandleBase* handle);
  };

  template<typename MT>
  class LockGuard final {
  public:
    explicit LockGuard(MT& m, bool throwException = true,
        SnappyHandleBase* handle = nullptr) : m_sync(m), m_lockFailed(false) {
      try {
        m_sync.lock();
      } catch (const std::exception& ex) {
        std::string err("Failed to acquire connection lock: ");
        err.append(ex.what());
        m_lockFailed = true;
        if (throwException) {
          throw GET_SQLEXCEPTION(client::SQLState::UNKNOWN_EXCEPTION, err);
        } else {
          if (!handle) {
            SnappyHandleBase::setGlobalException(
                GET_SQLEXCEPTION(client::SQLState::UNKNOWN_EXCEPTION, err));
          } else {
            handle->setException(
                GET_SQLEXCEPTION(client::SQLState::UNKNOWN_EXCEPTION, err));
          }
        }
      }
    }

    bool lockFailed() const {
      return m_lockFailed;
    }

    ~LockGuard() {
      if (!m_lockFailed) m_sync.unlock();
    }

    LockGuard(const LockGuard&) = delete;
    LockGuard& operator=(const LockGuard&) = delete;

  private:
    MT& m_sync;
    bool m_lockFailed;
  };

} /* namespace snappydata */
} /* namespace io */

#endif /* DRIVERBASE_H_ */
