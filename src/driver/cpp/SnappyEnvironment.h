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
 * SnappyEnvironment.h
 *
 * Holds common execution environment and utilities.
 */

#ifndef SNAPPYENVIRONMENT_H_
#define SNAPPYENVIRONMENT_H_

#include <functional>
#include <mutex>

#include <Connection.h>

#include "DriverBase.h"

namespace io {
namespace snappydata {

  class SnappyConnection;

  /**
   * Encapsulates the current execution context including the connections,
   * the application ODBC version etc.
   */
  class SnappyEnvironment final : public SnappyHandleBase {
  private:
    /** global lock for static initializations */
    static std::mutex g_sync;

    /**
     * Flag to indicate if global initialization is done.
     * Not strictly required since ClientService.initialize will already
     * handle it in a lock so this is a weak check and not under a lock.
     */
    static bool g_initialized;

    /** the list of all environment handles allocated for this app*/
    static std::vector<SnappyEnvironment*> g_envHandles;

    // TODO: implement the shared/non-shared environments
    const bool m_isShared;
    /** the list of all connections registered in this environment */
    std::vector<SnappyConnection*> m_connections;
    /**
     * if set to true then the driver will do mappings as required for an
     * ODBC 2.x application; see
     * http://msdn.microsoft.com/en-us/library/windows/desktop/ms714001.aspx
     */
    bool m_appIsVersion2x;

    static SQLRETURN getExceptionRecord(SQLSMALLINT handleType,
        SQLHANDLE handle, SQLSMALLINT recNumber, SQLUINTEGER maxRecordSize,
        SQLSMALLINT* textLength, SQLException*& err,
        const std::string*& message);

    // couple of template error handling methods used by SQLCHAR/SQLWCHAR
    // variants of ODBC error retrieval methods

    template<typename CHAR_TYPE>
    static SQLRETURN handleErrorT(SQLSMALLINT handleType, SQLHANDLE handle,
        SQLSMALLINT recNumber, CHAR_TYPE* sqlState, SQLINTEGER* nativeError,
        CHAR_TYPE* messageText, SQLSMALLINT bufferLength,
        SQLSMALLINT* textLength, SQLSMALLINT textLenFactor);

    template<typename CHAR_TYPE>
    static SQLRETURN handleDiagFieldT(SQLSMALLINT handleType, SQLHANDLE handle,
        SQLSMALLINT recNumber, SQLSMALLINT diagId, SQLPOINTER diagInfo,
        SQLSMALLINT bufferLength, SQLSMALLINT* stringLength);

    /** register a new connection in this environment */
    void addNewActiveConnection(SnappyConnection* conn);

    /** gets the active connections for this ENV */
    size_t getActiveConnectionsCount();

    /**
     * Remove an existing connection from this environment;
     *
     * @return SQL_SUCCESS if connection was found and removed,
     * SQL_NO_DATA if not found and SQL_ERROR in case of some error
     */
    SQLRETURN removeActiveConnection(SnappyConnection* conn);

    friend class SnappyConnection;

  public:

    /**
     * Constructor for SnappyEnvironment to created shared or
     * non-shared environments.
     */
    inline SnappyEnvironment(const bool shared) :
        m_isShared(shared), m_connections(), m_appIsVersion2x(false) {
    }

    /**
     * Returns true if this is a shared environment.
     */
    inline bool isShared() {
      return m_isShared;
    }

    /**
     * Returns true if the application is an ODBC 2.x one.
     *
     * TODO: implement the behaviour differences as noted in the link
     * of the docs in {@link #m_appIsVersion2x}.
     */
    inline bool isApplicationVersion2() {
      return m_appIsVersion2x;
    }

    /**
     * Execute a given function for each active connection in this
     * environment.
     */
    SQLRETURN forEachActiveConnection(
        std::function<SQLRETURN(SnappyConnection*)> connOperation);

    /**
     * Set an ODBC environment attribute.
     */
    SQLRETURN setAttribute(SQLINTEGER attribute, SQLPOINTER value,
        SQLINTEGER stringLength);

    /**
     * Get the value of an ODBC environment attribute set previously
     * using {@link #setAttribute}.
     */
    SQLRETURN getAttribute(SQLINTEGER attribute, SQLPOINTER value,
        SQLINTEGER bufferLength, SQLINTEGER* stringLengthPtr);

    /**
     * Allocate a new SnappyEnvironment.
     */
    static SQLRETURN globalInitialize();

    /**
     * Allocate a new SnappyEnvironment.
     */
    static SQLRETURN newEnvironment(SnappyEnvironment*& envRef);

    /**
     * Free the given SnappyEnvironment.
     */
    static SQLRETURN freeEnvironment(SnappyEnvironment* env);

    static SQLRETURN handleError(SQLSMALLINT handleType, SQLHANDLE handle,
        SQLSMALLINT recNumber, SQLCHAR* sqlState, SQLINTEGER* nativeError,
        SQLCHAR* messageText, SQLSMALLINT bufferLength,
        SQLSMALLINT* textLength);

    static SQLRETURN handleError(SQLSMALLINT handleType, SQLHANDLE handle,
        SQLSMALLINT recNumber, SQLWCHAR* sqlState, SQLINTEGER* nativeError,
        SQLWCHAR* messageText, SQLSMALLINT bufferLength,
        SQLSMALLINT* textLength);

    static SQLRETURN handleDiagField(SQLSMALLINT handleType,
        SQLHANDLE handle, SQLSMALLINT recNumber, SQLSMALLINT diagId,
        SQLPOINTER diagInfo, SQLSMALLINT bufferLength,
        SQLSMALLINT* stringLength);

    static SQLRETURN handleDiagFieldW(SQLSMALLINT handleType,
        SQLHANDLE handle, SQLSMALLINT recNumber, SQLSMALLINT diagId,
        SQLPOINTER diagInfo, SQLSMALLINT bufferLength,
        SQLSMALLINT* stringLength);
  };

} /* namespace snappydata */
} /* namespace io */

#endif /* SNAPPYENVIRONMENT_H_ */
