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
 * SnappyConnection.h
 *
 *  Defines wrapper class for the underlying native Connection.
 */

#ifndef SNAPPYCONNECTION_H_
#define SNAPPYCONNECTION_H_

#include <Connection.h>

#include "SnappyEnvironment.h"
#include "SnappyDefaults.h"
#include "Library.h"

namespace io {
namespace snappydata {

  class SnappyEnvironment;
  class SnappyStatement;

  /**
   * Encapsulates a native {@link Connection} and adds ODBC
   * specific methods and state.
   */
  class SnappyConnection final : public SnappyHandleBase {
  private:
    /** the underlying native connection */
    Connection m_conn;

    /** the current SnappyEnvironment */
    SnappyEnvironment* const m_env;

    static const int TRANSACTION_UNKNOWN = -1;

    /** the DSN for this connection, if any */
    std::string m_dsn;

    /**
     * set of attributes set for this connection using
     * {@link #setAttribute}
     */
    AttributeMap m_attributes;

    /**
     * if true then use case insensitive arguments to meta-data queries
     * else the values are case sensitive
     */
    bool m_argsAsIdentifiers;

    /**
     * Handle of the parent window used to display any dialog boxes.
     * If this is null then no dialogs will be displayed.
     */
    SQLPOINTER m_hwnd;

    /**
     * Translation option argument for the transaction function invocations.
     */
    UDWORD m_translateOption;

    /**
     * The translation library associated with this connection, if any.
     */
    native::Library* m_translationLibrary;

    /**
     * Translation function for charset from DataSource to Driver loaded
     * from the translation library, if any.
     */
    DataSourceToDriver m_dataSourceToDriver;

    /**
     * Translation function for charset from Driver to DataSource loaded
     * from the translation library, if any.
     */
    DriverToDataSource m_driverToDataSource;

    /** SnappyEnvironment needs to access the private destructor */
    friend class SnappyEnvironment;

    /** let SnappyStatement access the private fields */
    friend class SnappyStatement;

    /**
     * Constructor for a SnappyConnection given handle to SnappyEnvironment.
     */
    SnappyConnection(SnappyEnvironment* env);

    ~SnappyConnection();

    /**
     * Establish a new connection to the SnappyData system. The network server
     * location is specified in the passed "server" and "port" arguments,
     * while the other connection properties (including user/password,
     *   if any) is passed in the "connProps" property bag.
     *
     * @throws SQLException on error, so caller should handle
     */
    template<typename CHAR_TYPE>
    SQLRETURN connectT(const std::string& server, const int port,
        const Properties& connProps, CHAR_TYPE* outConnStr,
        const SQLINTEGER outConnStrLen, SQLSMALLINT* connStrLen);

    /**
     * Set an ODBC connection attribute on native connection.
     *
     * @throws SQLException on error, so caller should handle
     */
    SQLRETURN setConnectionAttribute(SQLINTEGER attribute,
        const AttributeValue& attrValue);

    /**
     * Fill in a string value from given string.
     */
    template<typename CHAR_TYPE, typename CHAR_TYPE2>
    SQLRETURN getStringValue(const CHAR_TYPE *str, const SQLLEN len,
        CHAR_TYPE2 *resultValue, SQLLEN bufferLength,
        SQLINTEGER *stringLengthPtr, const char* op);

    /**
     * Get the value of an ODBC connection attribute set previously
     * using {@link #setAttribute}.
     *
     * @throws SQLException on error, so caller should handle
     */
    template<typename CHAR_TYPE>
    SQLRETURN getAttributeT(SQLINTEGER attribute, SQLPOINTER value,
        SQLINTEGER bufferLength, SQLINTEGER* stringLengthPtr);

    /**
     * Returns more information about the connection as in ODBC SQLGetInfo.
     *
     * @throws SQLException on error, so caller should handle
     */
    template<typename CHAR_TYPE>
    SQLRETURN getInfoT(SQLUSMALLINT infoType, SQLPOINTER infoValue,
        SQLSMALLINT bufferLength, SQLSMALLINT* stringLength);

    /**
     * Converts the given SQL statement into the system's native SQL grammar.
     */
    template<typename CHAR_TYPE>
    SQLRETURN nativeSQLT(CHAR_TYPE* inStatementText, SQLINTEGER textLength1,
        CHAR_TYPE* outStatementText, SQLINTEGER bufferLength,
        SQLINTEGER* textLength2Ptr);

  public:
    static SQLRETURN newConnection(SnappyEnvironment *env,
        SnappyConnection*& connRef);

    static SQLRETURN freeConnection(SnappyConnection* conn);

    /**
     * Get the SnappyEnvironment for this connection.
     */
    inline SnappyEnvironment* getEnvironment() {
      return m_env;
    }

    /**
     * Establish a new connection to the SnappyData system. The network server
     * location is specified in the passed "server" and "port" arguments,
     * while the other connection properties (including user/password,
     *   if any) is passed in the "connProps" property bag.
     */
    SQLRETURN connect(const std::string& server, const int port,
        const Properties& connProps, const std::string& dsn,
        SQLCHAR* outConnStr, const SQLINTEGER outConnStrLen,
        SQLSMALLINT* connStrLen);

    /**
     * Establish a new connection to the SnappyData system. The network server
     * location is specified in the passed "server" and "port" arguments,
     * while the other connection properties (including user/password,
     *   if any) is passed in the "connProps" property bag.
     *
     * This is the wide-character string version for returning the
     * output connect string.
     */
    SQLRETURN connect(const std::string& server, const int port,
        const Properties& connProps, const std::string& dsn,
        SQLWCHAR* outConnStr, const SQLINTEGER outConnStrLen,
        SQLSMALLINT* connStrLen);

    /**
     * Disconnect this connection.
     */
    SQLRETURN disconnect();

    /**
     * Return true if this connection is currently active.
     */
    inline bool isActive() {
      return m_conn.isOpen();
    }

    const std::string& getDSN() const noexcept {
      return m_dsn;
    }

    /**
     * Set an ODBC connection attribute.
     */
    SQLRETURN setAttribute(SQLINTEGER attribute, SQLPOINTER value,
        SQLINTEGER stringLength, bool isAscii);

    /**
     * Get the value of an ODBC connection attribute set previously
     * using {@link #setAttribute}.
     */
    SQLRETURN getAttribute(SQLINTEGER attribute, SQLPOINTER value,
        SQLINTEGER bufferLength, SQLINTEGER* stringLengthPtr);

    /**
     * Get the value of an ODBC connection attribute set previously
     * using {@link #setAttribute}.
     *
     * This is the wide-string version.
     */
    SQLRETURN getAttributeW(SQLINTEGER attribute, SQLPOINTER value,
        SQLINTEGER bufferLength, SQLINTEGER* stringLengthPtr);

    /**
     * Returns more information about the connection as in ODBC SQLGetInfo.
     */
    SQLRETURN getInfo(SQLUSMALLINT infoType, SQLPOINTER infoValue,
        SQLSMALLINT bufferLength, SQLSMALLINT* stringLength);

    /**
     * Returns more information about the connection as in ODBC SQLGetInfo.
     */
    SQLRETURN getInfoW(SQLUSMALLINT infoType, SQLPOINTER infoValue,
        SQLSMALLINT bufferLength, SQLSMALLINT* stringLength);

    /**
     * Converts the given SQL statement into the system's native SQL grammar.
     */
    SQLRETURN nativeSQL(SQLCHAR* inStatementText, SQLINTEGER textLength1,
        SQLCHAR* outStatementText, SQLINTEGER bufferLength,
        SQLINTEGER* textLength2Ptr);

    /**
     * Converts the given SQL statement into the system's native SQL grammar.
     * This is the wide-char version.
     */
    SQLRETURN nativeSQLW(SQLWCHAR* inStatementText, SQLINTEGER textLength1,
        SQLWCHAR* outStatementText, SQLINTEGER bufferLength,
        SQLINTEGER* textLength2Ptr);

    /**
     * Commit the current active transaction.
     */
    SQLRETURN commit();

    /**
     * Rollback the current active transaction.
     */
    SQLRETURN rollback();

    /**
     * Cancel the current statement executing on the connection, if any.
     */
    SQLRETURN cancelCurrentStatement();
  };

} /* namespace snappydata */
} /* namespace io */

#endif /* SNAPPYCONNECTION_H_ */
