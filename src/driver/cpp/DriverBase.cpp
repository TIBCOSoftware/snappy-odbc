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
 * DriverBase.cpp
 */

#include "DriverBase.h"

#include <typeinfo>

using namespace io::snappydata;

// define the last global exception (during initialization etc)
std::atomic<SQLException*> SnappyHandleBase::s_globalError;

SQLException* SnappyHandleBase::getUnknownException(const char* file,
    int line, std::exception& ex) {
  // check for SQLException itself
  SQLException* sqle = dynamic_cast<SQLException*>(&ex);
  if (sqle) {
    return sqle->clone(true);
  }
  // check for out of memory separately
  if (dynamic_cast<const std::bad_alloc*>(&ex)) {
    return new SQLException(file, line, SQLState::OUT_OF_MEMORY,
        SQLStateMessage::OUT_OF_MEMORY_MSG.format(ex.what()));
  }

  std::string reason;
  Utils::demangleTypeName(typeid(ex).name(), reason);
  reason.append(": ").append(ex.what());
  return new SQLException(file, line, SQLState::UNKNOWN_EXCEPTION, reason);
}

void SnappyHandleBase::setGlobalException(SQLException* ex) {
  delete s_globalError.exchange(ex, std::memory_order_acquire);
}

void SnappyHandleBase::setGlobalException(SQLException& ex) {
  setGlobalException(ex.clone(true));
}

void SnappyHandleBase::setGlobalException(SQLException&& ex) {
  setGlobalException(ex.clone(true));
}

void SnappyHandleBase::setGlobalException(const char* file, int line,
    std::exception& ex) {
  setGlobalException(getUnknownException(file, line, ex));
}

SQLException* SnappyHandleBase::lastGlobalError() noexcept {
  return s_globalError.load(std::memory_order_acquire);
}

void SnappyHandleBase::clearLastGlobalError() noexcept {
  delete s_globalError.exchange(nullptr, std::memory_order_acquire);
}

void SnappyHandleBase::setException(SQLException& ex) {
  m_lastError.reset(ex.clone(true));
}

void SnappyHandleBase::setException(SQLException&& ex) {
  m_lastError.reset(ex.clone(true));
}

void SnappyHandleBase::setException(const char* file, int line,
    std::exception& ex) {
  m_lastError.reset(getUnknownException(file, line, ex));
}

void SnappyHandleBase::setSQLWarning(SQLWarning& warning) {
  m_lastError.reset(warning.clone(true));
}

SQLException* SnappyHandleBase::lastError() const noexcept {
  return m_lastError.get();
}

void SnappyHandleBase::clearLastError() noexcept {
  if (m_lastError) {
    m_lastError = nullptr;
  }
  clearLastGlobalError();
}

SQLRETURN SnappyHandleBase::errorNullHandle(SQLSMALLINT handleType) {
  SnappyHandleBase::setGlobalException(
      GET_SQLEXCEPTION2(SQLStateMessage::NULL_HANDLE_MSG, handleType));
  return SQL_INVALID_HANDLE;
}

SQLRETURN SnappyHandleBase::errorNullHandle(SQLSMALLINT handleType,
    SnappyHandleBase* handle) {
  if (handle) {
    handle->setException(
        GET_SQLEXCEPTION2(SQLStateMessage::NULL_HANDLE_MSG, handleType));
    return SQL_INVALID_HANDLE;
  } else {
    return errorNullHandle(handleType);
  }
}

SQLRETURN SnappyHandleBase::errorNotImplemented(const char* function) {
  SnappyHandleBase::setGlobalException(
      GET_SQLEXCEPTION2(SQLStateMessage::NOT_IMPLEMENTED_MSG, function));
  return SQL_ERROR;
}

SQLRETURN SnappyHandleBase::errorNotImplemented(const char* function,
    SnappyHandleBase* handle) {
  if (handle) {
    handle->setException(
        GET_SQLEXCEPTION2(SQLStateMessage::NOT_IMPLEMENTED_MSG, function));
    return SQL_ERROR;
  } else {
    return errorNotImplemented(function);
  }
}

SQLRETURN SnappyHandleBase::errorInvalidBufferLength(int length,
    const char* name, SnappyHandleBase* handle) {
  handle->setException(
      GET_SQLEXCEPTION2(SQLStateMessage::INVALID_BUFFER_LENGTH_MSG, length,
          name));
  return SQL_ERROR;
}
