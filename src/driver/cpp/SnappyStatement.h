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
 * SnappyStatement.h
 *
 * Defines wrapper class for the underlying JDBC Statement classes.
 */

#ifndef SNAPPYSTATEMENT_H_
#define SNAPPYSTATEMENT_H_

#include <Parameters.h>
#include <PreparedStatement.h>

#include "SnappyConnection.h"
#include "StringFunctions.h"
#include "SnappyDescriptor.h"
#include "BatchRowIterator.h"

namespace io {
namespace snappydata {

  enum class Cursor {
    FORWARD_ONLY, SCROLLABLE, UPDATABLE, UPDATABLE_SCROLLABLE
  };

  /**
   * Encapsulates a native Statement, ResultSet, bound parameters,
   * bound output values etc.
   *
   * Convention for interval types not available in native API:
   *
   * SQL_INTERVAL_MONTH: store total number of months as INTEGER with sign
   *
   * SQL_INTERVAL_YEAR: store total number of years as INTEGER with sign
   *
   * SQL_INTERVAL_YEAR_TO_MONTH: store total number of months as INTEGER
   *                             with sign
   *
   * SQL_INTERVAL_DAY: store total number of days as INTEGER with sign
   *
   * SQL_INTERVAL_HOUR: store total number of hours as INTEGER with sign
   *
   * SQL_INTERVAL_MINUTE: store total number of minutes as INTEGER with sign
   *
   * SQL_INTERVAL_SECOND: store total number of secs as INTEGER with sign
   *
   * SQL_INTERVAL_DAY_TO_HOUR: store total number of hours as BIGINT
   *                           with sign
   *
   * SQL_INTERVAL_DAY_TO_MINUTE: store total number of minutes as BIGINT
   *                             with sign
   *
   * SQL_INTERVAL_DAY_TO_SECOND: store total number of secs as BIGINT
   *                             with sign
   *
   * SQL_INTERVAL_HOUR_TO_MINUTE: store total number of minutes as BIGINT
   *                              with sign
   *
   * SQL_INTERVAL_HOUR_TO_SECOND: store total number of secs as BIGINT
   *                              with sign
   *
   * SQL_INTERVAL_MINUTE_TO_SECOND: store total number of secs as BIGINT
   *                                with sign
   */
  class SnappyStatement final : public SnappyHandleBase {
  private:
    /** the underlying connection */
    SnappyConnection& m_conn;

    /** the underlying native prepared statement */
    std::unique_ptr<PreparedStatement> m_pstmt;

    /** attributes for this statement */
    StatementAttributes m_stmtAttrs;

    struct Parameter final {
      // some fields uninitialized by design with invalid m_inputOutputType
      uint32_t m_paramNum;
      short m_inputOutputType { -1 };
      SQLType m_paramType;
      bool m_isDataAtExecParam { true };
      bool m_isBound { false };

      // original values
      SQLSMALLINT m_o_valueType;
      SQLSMALLINT m_o_paramType;
      SQLUINTEGER m_o_precision;
      SQLSMALLINT m_o_scale;
      SQLPOINTER m_o_value { nullptr };
      SQLLEN m_o_valueSize { 0 };
      SQLLEN* m_o_lenOrIndp { nullptr };

      ~Parameter() {
        m_o_value = nullptr;
      }

      inline void set(SQLUSMALLINT paramNum, SQLSMALLINT inputOutputType,
          SQLSMALLINT valueType, SQLSMALLINT paramType, SQLUINTEGER precision,
          SQLSMALLINT scale, SQLPOINTER value, SQLLEN valueSize,
          SQLLEN* lenOrIndp) {
        m_paramNum = paramNum;
        m_inputOutputType = inputOutputType;
        m_o_valueType = valueType;
        m_o_paramType = paramType;
        if (precision > 0) {
          m_o_precision = precision;
        } else {
          m_o_precision = DEFAULT_REAL_PRECISION;
        }
        m_o_scale = scale;
        // will be checked at actual bind time
        // lenOrIndp can be uninitialized at this point
        m_isDataAtExecParam = false;
        m_o_lenOrIndp = lenOrIndp;
        m_o_value = value;
        m_o_valueSize = valueSize;
        if (m_o_valueType == SQL_C_DEFAULT) {
          m_o_valueType = convertTypeToCType(m_o_paramType, paramNum);
        }
      }
    };

    struct OutputField final {
      SQLSMALLINT m_targetType { 0 };
      SQLPOINTER m_targetValue { nullptr };
      SQLLEN m_valueSize { 0 };
      SQLLEN *m_lenOrIndPtr { nullptr };

      inline void set(SQLSMALLINT targetType, SQLPOINTER targetValue,
          SQLLEN valueSize, SQLLEN* lenOrIndPtr) {
        m_targetType = targetType;
        m_targetValue = targetValue;
        m_valueSize = valueSize;
        m_lenOrIndPtr = lenOrIndPtr;
      }
    };

    /** the parameters bound to this statement */
    std::vector<Parameter> m_params;

    /** the underlying parameters converted from m_params used for execution */
    client::Parameters m_execParams;

    /** the output fields bound to this statement */
    std::vector<OutputField> m_outputFields;

    /**
     * The result of a statement execution. This is returned in case normal
     * execute has been invoked on underlying native API and not
     * executeQuery or executeUpdate. It encapsulates ResultSet, if any,
     * procedure output parameters, if any, update count, generated keys etc
     */
    std::unique_ptr<Result> m_result;

    /**
     * The ResultSet, if any, obtained from the last execution of this
     * statement. If this is nullptr then it indicates that the statement
     * execution returned an update count.
     */
    std::shared_ptr<ResultSet> m_resultSet;

    /**
     * The updatable cursor from the ResultSet, if any, obtained from the
     * last execution of this statement.
     */
    ResultSet::iterator m_cursor;

    /**
     * The updatable bulk fetch cursor from the ResultSet, if any, obtained from the
     * last execution of this statement. Initialized if SQL_ATTR_ROW_ARRAY_SIZE
     * attribute has been set.
     */
    BatchRowIterator m_bulkCursor;

    /** the type of cursor created for current ResultSet */
    Cursor m_cursorType;

    /**
     * ODBC implicit descriptor handles required for ODBC driver manger
     */
    std::unique_ptr<SnappyDescriptor> m_apdDesc;
    std::unique_ptr<SnappyDescriptor> m_ipdDesc;
    std::unique_ptr<SnappyDescriptor> m_ardDesc;
    std::unique_ptr<SnappyDescriptor> m_irdDesc;

    /**
     * if true then use case insensitive arguments to meta-data queries
     * else the values are case sensitive
     */
    bool m_argsAsIdentifiers;

    /** the current bookmark offset */
    SQLINTEGER m_bookmark;

    /** the maximum number of rows to return from the result set */
    SQLLEN m_maxRows;

    /** the maximum number of rows to return from the result set */
    SQLULEN m_KeySetSize;

    /** value that sets the binding orientation to be used */
    SQLULEN m_bindingOrientation;

    /** value that sets the param binding orientation to be used */
    SQLULEN m_paramBindingOrientation;

    /** param set size for param array binding */
    SQLULEN m_paramSetSize;

    /** Array in which to return status information for each row of parameter values.*/
    SQLUSMALLINT* m_paramStatusArr;

    /** array of SQLUSMALLINT values used to ignore a parameter during execution of an SQL statement.*/
    SQLUSMALLINT* m_paramOperationPtr;

    /**  array of SQLUSMALLINT values used to ignore a row during a bulk operation using SQLSetPos*/
    SQLUSMALLINT* m_rowOperationPtr;

    /** number of sets of parameters processed */
    SQLULEN* m_paramsProcessedPtr;

    /** value that points to an offset added to pointers to change binding of dynamic parameters*/
    SQLULEN* m_paramBindOffsetPtr;

    /** address of the row status array filled by Snappyetch and SnappyetchScroll*/
    SQLUSMALLINT* m_rowStatusPtr;

    /**
     * value that points to a buffer in which to return the number of rows
     * fetched after a call to Snappyetch or SnappyetchScroll
     */
    SQLULEN* m_fetchedRowsPtr;

    /** value that points to an offset added to pointers to change binding of column data*/
    SQLULEN* m_bindOffsetPtr;

    /** current executing parameter */
    size_t m_currentParameterIndex;

    /** C-style printf GUID format string */
    static const char* s_GUID_FORMAT;

    /** needs to access m_resultSet and some others for SnappyDiagRecField */
    friend class SnappyEnvironment;

    inline SnappyStatement(SnappyConnection* conn) :
        m_conn(*conn), m_params(), m_execParams(), m_outputFields(),
        m_cursor(), m_bulkCursor(m_cursor, 1),
        m_apdDesc(new SnappyDescriptor(SQL_ATTR_APP_PARAM_DESC)),
        m_ipdDesc(new SnappyDescriptor(SQL_ATTR_IMP_PARAM_DESC)),
        m_ardDesc(new SnappyDescriptor(SQL_ATTR_APP_ROW_DESC)),
        m_irdDesc(new SnappyDescriptor(SQL_ATTR_IMP_ROW_DESC)) {
      initWithDefaultValues();
    }

    ~SnappyStatement() {
      close();
    }

    void initWithDefaultValues() {
      m_cursorType = Cursor::FORWARD_ONLY;
      m_stmtAttrs.setResultSetHoldability(
          m_conn.m_conn.getResultSetHoldability());
      m_bookmark = -1;
      m_bindingOrientation = SQL_BIND_BY_COLUMN;
      m_rowStatusPtr = nullptr;
      m_paramBindingOrientation = SQL_PARAM_BIND_BY_COLUMN;
      m_paramSetSize = 1;
      m_currentParameterIndex = 0;
      m_argsAsIdentifiers = false;
      m_KeySetSize = 0;
      m_paramStatusArr = nullptr;
      m_paramOperationPtr = nullptr;
      m_rowOperationPtr = nullptr;
      m_paramsProcessedPtr = nullptr;
      m_paramBindOffsetPtr = nullptr;
      m_fetchedRowsPtr = nullptr;
      m_bindOffsetPtr = nullptr;
    }

    /**
     * Sets the ResultSet in this statement and transfers ownership. Note that
     * incoming ResultSet is nulled after this call and should never be used.
     */
    void setResultSet(std::unique_ptr<ResultSet>& rs);

    /**
     * Sets the ResultSet in this statement.
     */
    void setResultSet(std::shared_ptr<ResultSet>& rs);

    /**
     * Checks the unsupported conversion from ctype to sqltype
     * http://msdn.microsoft.com/en-us/library/windows/desktop/ms716298(v=vs.85).aspx
     */
    bool checkSupportedCtypeToSQLTypeConversion(SQLSMALLINT cType,
        SQLSMALLINT sqlType);

    /** bind all the parameters to the underlying prepared statement */
    SQLRETURN bindParameters(std::map<int32_t, OutputParameter>* outParams);

    /** bind an array of parameters to the underlying prepared statement */
    SQLRETURN bindArrayOfParameters(ParametersBatch& paramsBatch,
        SQLULEN setSize, SQLULEN* bindOffsetPtr,
        SQLULEN bindingOrientation, SQLUSMALLINT* statusArr,
        SQLULEN* processedPtr);

    /** bind a given parameter to the underlying prepared statement */
    SQLRETURN bindParameter(Parameters& paramValues, Parameter& param,
        std::map<int32_t, OutputParameter>* outParams,
        bool appendPutData = false, SQLLEN valueOffset = 0,
        SQLLEN lenOffset = 0);

    /** fill output values from given Row */
    SQLRETURN fillOutput(const Row& outputRow, const uint32_t outputColumn,
        SQLPOINTER value, const SQLLEN valueSize, SQLSMALLINT ctype,
        const SQLUINTEGER precision, SQLLEN* lenOrIndp);

    /** fill output and input parameters in this prepared statement */
    SQLRETURN fillOutParameters(const Result& result);

    SQLRETURN fillOutputFields();

    SQLRETURN fillOutputFieldsWithArrays();

    void setRowStatus();

    /** Prepare the statement with current parameters. */
    SQLRETURN prepare(const std::string& sqlText);

    /*
     * Execute given query string with any parameters already bound.
     */
    SQLRETURN execute(const std::string& sqlText);

    /*
     * Executes statement for array of parameters.
     */
    SQLRETURN executeWithArrayOfParams(const std::string& sqlText);

    template<typename HANDLE_TYPE>
    SQLRETURN handleWarnings(const HANDLE_TYPE* handle) {
      if (handle && !handle->hasWarnings()) {
        return SQL_SUCCESS;
      } else {
        std::unique_ptr<SQLWarning> warnings = m_result->getWarnings();
        if (!warnings) {
          return SQL_SUCCESS;
        }
        setSQLWarning(*warnings);
        return SQL_SUCCESS_WITH_INFO;
      }
    }

    inline SQLSMALLINT getLengthTruncatedToSmallInt(size_t len) const noexcept {
      return StringFunctions::restrictLength<SQLSMALLINT, size_t>(len);
    }

    template<typename CHAR_TYPE>
    SQLRETURN getAttributeT(SQLINTEGER attribute, SQLPOINTER valueBuffer,
        SQLINTEGER bufferLen, SQLINTEGER* valueLen);

    template<typename CHAR_TYPE>
    SQLRETURN setAttributeT(SQLINTEGER attribute, SQLPOINTER valueBuffer,
        SQLINTEGER valueLen);

    /**
     * Get the ColumnDescriptor from ResultSet or PreparedStatement of the
     * given column number.
     *
     * @throws SQLException caller must handle
     */
    const ColumnDescriptor getColumnDescriptor(SQLUSMALLINT columnNumber,
        uint32_t* columnCount = nullptr) const;

    template<typename CHAR_TYPE>
    SQLRETURN getResultColumnDescriptorT(SQLUSMALLINT columnNumber,
        CHAR_TYPE* columnName, SQLSMALLINT bufferLength,
        SQLSMALLINT* nameLength, SQLSMALLINT* dataType, SQLULEN* columnSize,
        SQLSMALLINT* decimalDigits, SQLSMALLINT* nullable);

    SQLRETURN getColumnAttributeT(SQLUSMALLINT columnNumber,
        SQLUSMALLINT fieldId, SQLPOINTER charAttribute,
        SQLSMALLINT bufferLength, SQLSMALLINT* stringLength,
        SQLLEN* numericAttribute, const int sizeOfChar);

    template<typename CHAR_TYPE>
    SQLRETURN getCursorNameT(CHAR_TYPE* cursorName,
        SQLSMALLINT bufferLength, SQLSMALLINT* nameLength);

    template<typename CHAR_TYPE>
    SQLRETURN setCursorNameT(CHAR_TYPE* cursorName, SQLSMALLINT nameLength);

    template<typename CHAR_TYPE>
    SQLRETURN getTablesT(CHAR_TYPE* schemaName, SQLSMALLINT nameLength1,
        CHAR_TYPE* tableName, SQLSMALLINT nameLength2,
        CHAR_TYPE* tableTypes, SQLSMALLINT nameLength3);

    template<typename CHAR_TYPE>
    SQLRETURN getTablePrivilegesT(CHAR_TYPE* schemaName,
        SQLSMALLINT nameLength1, CHAR_TYPE* tableName,
        SQLSMALLINT nameLength2);

    template<typename CHAR_TYPE>
    SQLRETURN getColumnsT(CHAR_TYPE* schemaName, SQLSMALLINT nameLength1,
        CHAR_TYPE* tableName, SQLSMALLINT nameLength2,
        CHAR_TYPE* columnName, SQLSMALLINT nameLength3);

    template<typename CHAR_TYPE>
    SQLRETURN getSpecialColumnsT(SQLUSMALLINT idType, CHAR_TYPE *schemaName,
        SQLSMALLINT nameLength1, CHAR_TYPE *tableName,
        SQLSMALLINT nameLength2, SQLUSMALLINT scope, SQLUSMALLINT nullable);

    template<typename CHAR_TYPE>
    SQLRETURN getColumnPrivilegesT(CHAR_TYPE* schemaName,
        SQLSMALLINT nameLength1, CHAR_TYPE* tableName,
        SQLSMALLINT nameLength2, CHAR_TYPE* columnName,
        SQLSMALLINT nameLength3);

    template<typename CHAR_TYPE>
    SQLRETURN getIndexInfoT(CHAR_TYPE* schemaName, SQLSMALLINT nameLength1,
        CHAR_TYPE* tableName, SQLSMALLINT nameLength2, bool unique,
        bool approximate);

    template<typename CHAR_TYPE>
    SQLRETURN getPrimaryKeysT(CHAR_TYPE* schemaName,
        SQLSMALLINT nameLength1, CHAR_TYPE* tableName,
        SQLSMALLINT nameLength2);

    template<typename CHAR_TYPE>
    SQLRETURN getImportedKeysT(CHAR_TYPE* schemaName,
        SQLSMALLINT nameLength1, CHAR_TYPE* tableName,
        SQLSMALLINT nameLength2);

    template<typename CHAR_TYPE>
    SQLRETURN getExportedKeysT(CHAR_TYPE* schemaName,
        SQLSMALLINT nameLength1, CHAR_TYPE* tableName,
        SQLSMALLINT nameLength2);

    template<typename CHAR_TYPE>
    SQLRETURN getCrossReferenceT(CHAR_TYPE* parentSchemaName,
        SQLSMALLINT nameLength1, CHAR_TYPE* parentTableName,
        SQLSMALLINT nameLength2, CHAR_TYPE* foreignSchemaName,
        SQLSMALLINT nameLength3, CHAR_TYPE* foreignTableName,
        SQLSMALLINT nameLength4);

    template<typename CHAR_TYPE>
    SQLRETURN getProceduresT(CHAR_TYPE* schemaPattern,
        SQLSMALLINT nameLength1, CHAR_TYPE* procedureNamePattern,
        SQLSMALLINT nameLength2);

    template<typename CHAR_TYPE>
    SQLRETURN getProcedureColumnsT(CHAR_TYPE* schemaPattern,
        SQLSMALLINT nameLength1, CHAR_TYPE* procedureNamePattern,
        SQLSMALLINT nameLength2, CHAR_TYPE* columnNamePattern,
        SQLSMALLINT nameLength3);

    template<typename CHAR_TYPE>
    SQLRETURN getTypeInfoT(SQLSMALLINT dataType);

    static SQLSMALLINT convertTypeToCType(const SQLSMALLINT odbcType,
        const uint32_t paramNum);

    static SQLType convertTypeToSQLType(const SQLSMALLINT odbcType,
        const uint32_t paramNum);

    static SQLSMALLINT convertSQLTypeToCType(const SQLType sqlType);

      static SQLSMALLINT convertSQLTypeToType(const SQLType sqlType,
          const bool isASCII = true);

    static SQLSMALLINT convertNullability(const ColumnNullability nullability);

  public:
    static SQLRETURN newStatement(SnappyConnection* conn,
        SnappyStatement*& stmtRef);

    /**
     * Free the cursor or parameters of the statement, or this statement
     * itself depending on the given option.
     */
    static SQLRETURN freeStatement(SnappyStatement* stmt, SQLUSMALLINT opt);

    /**
     * Add a new parameter with given attributes to the parameter list
     * to be bound before execution time.
     */
    SQLRETURN addParameter(SQLUSMALLINT paramNum,
        SQLSMALLINT inputOutputType, SQLSMALLINT valueType,
        SQLSMALLINT paramType, SQLULEN precision, SQLSMALLINT scale,
        SQLPOINTER paramValue, SQLLEN valueSize, SQLLEN* lenOrIndPtr);

    /** Prepare the statement with current parameters. */
    inline SQLRETURN prepare(SQLCHAR *stmtText, SQLINTEGER textLength) {
      std::string stmt = std::move(
          StringFunctions::toString(stmtText, textLength));
      return prepare(stmt);
    }

    /** Prepare the statement with current parameters. */
    inline SQLRETURN prepare(SQLWCHAR *stmtText, SQLINTEGER textLength) {
      std::string stmt = std::move(
          StringFunctions::toString(stmtText, textLength));
      return prepare(stmt);
    }

    /**
     * Return false if {@link #prepare} has been invoked for this statement
     * else true.
     */
    inline bool isUnprepared() const {
      return !m_pstmt;
    }

    /**
     * Return true if {@link #prepare} has been invoked for this statement
     * else false.
     */
    inline bool isPrepared() const {
      return m_pstmt != nullptr;
    }

    /*
     * Execute given query string with any parameters already bound.
     */
    inline SQLRETURN execute(SQLCHAR* stmtText, SQLINTEGER textLength) {
      std::string stmt = std::move(
          StringFunctions::toString(stmtText, textLength));
      return execute(stmt);
    }

    /*
     * Execute given query string with any parameters already bound.
     */
    inline SQLRETURN execute(SQLWCHAR* stmtText, SQLINTEGER textLength) {
    std::string stmt = std::move(
        StringFunctions::toString(stmtText, textLength));
    return execute(stmt);
    }

    /**
     * Execute already prepared query with any parameters already bound.
     */
    SQLRETURN execute();

    /**Execute batched inserts.*/
    SQLRETURN bulkOperations(SQLUSMALLINT operation);

    /**
     * Bind an output column to be fetched by next() calls.
     */
    SQLRETURN bindOutputField(SQLUSMALLINT columnNum,
        SQLSMALLINT targetType, SQLPOINTER targetValue, SQLLEN valueSize,
        SQLLEN* lenOrIndPtr);

    /**
     * Move to the next row of the ResultSet and fetch the values in the
     * bound columns.
     */
    SQLRETURN next();

    /**
     * Fetches the row in the ResultSet with fetchOrientation and fetchOffset.
     */
    SQLRETURN fetchScroll(SQLSMALLINT fetchOrientation,
        SQLLEN fetchOffset);

    /**sets the cursor position in a rowset and allows an application
     * to refresh data in the rowset or to update or delete data in the result set.*/
    SQLRETURN setPos(SQLSETPOSIROW rowNumber, SQLUSMALLINT operation,
        SQLUSMALLINT lockType);

    /**
     * Retrieve result set data without binding column values.
     */
    SQLRETURN getData(SQLUSMALLINT columnNum, SQLSMALLINT targetType,
        SQLPOINTER targetValue, SQLLEN valueSize, SQLLEN* lenOrIndPtr);

    /**
     * If the previously executed statement was a DML statement, then
     * return the number of rows affected, else return -1.
     *
     * If the "updateError" parameter is true (default), then previous error
     * is cleared and new one set if no execution was done previously.
     */
    SQLRETURN getUpdateCount(SQLLEN *count, bool updateError = true);

    /**
     * Get a given statement attribute (ODBC SQLGetStmtAttr).
     */
    SQLRETURN getAttribute(SQLINTEGER attribute, SQLPOINTER valueBuffer,
        SQLINTEGER bufferLen, SQLINTEGER* valueLen);

    /**
     * Get a given statement attribute (ODBC SQLGetStmtAttrW).
     */
    SQLRETURN getAttributeW(SQLINTEGER attribute, SQLPOINTER valueBuffer,
        SQLINTEGER bufferLen, SQLINTEGER* valueLen);

    /**
     * Set a given statement attribute (ODBC SQLSetStmtAttr).
     */
    SQLRETURN setAttribute(SQLINTEGER attribute, SQLPOINTER valueBuffer,
        SQLINTEGER valueLen);

    /**
     * Set a given statement attribute (ODBC SQLSetStmtAttrW).
     */
    SQLRETURN setAttributeW(SQLINTEGER attribute, SQLPOINTER valueBuffer,
        SQLINTEGER valueLen);

    /**
     * Return the column description from current result set.
     */
    SQLRETURN getResultColumnDescriptor(SQLUSMALLINT columnNumber,
        SQLCHAR* columnName, SQLSMALLINT bufferLength,
        SQLSMALLINT* nameLength, SQLSMALLINT* dataType, SQLULEN* columnSize,
        SQLSMALLINT* decimalDigits, SQLSMALLINT* nullable);

    /**
     * Return the column description from current result set.
     */
    SQLRETURN getResultColumnDescriptor(SQLUSMALLINT columnNumber,
        SQLWCHAR* columnName, SQLSMALLINT bufferLength,
        SQLSMALLINT* nameLength, SQLSMALLINT* dataType, SQLULEN* columnSize,
        SQLSMALLINT* decimalDigits, SQLSMALLINT* nullable);

    /**
     * Return descriptor information about a column in current result set.
     */
    SQLRETURN getColumnAttribute(SQLUSMALLINT columnNumber,
        SQLUSMALLINT fieldId, SQLPOINTER charAttribute,
        SQLSMALLINT bufferLength, SQLSMALLINT* stringLength,
        SQLLEN* numericAttribute);

    /**
     * Return descriptor information about a column in current result set.
     */
    SQLRETURN getColumnAttributeW(SQLUSMALLINT columnNumber,
        SQLUSMALLINT fieldId, SQLPOINTER charAttribute,
        SQLSMALLINT bufferLength, SQLSMALLINT* stringLength,
        SQLLEN* numericAttribute);

    /**
     * Returns the description about parameters associated with
     * a prepared SQL statement
     */
    SQLRETURN getParamMetadata(SQLUSMALLINT paramNumber,
        SQLSMALLINT* patamDataTypePtr, SQLULEN* paramSizePtr,
        SQLSMALLINT* decimalDigitsPtr, SQLSMALLINT* nullablePtr);

    /**
     * Returns the number of columns in the current result set.
     */
    SQLRETURN getNumResultColumns(SQLSMALLINT* columnCount);

    /**
     * Returns the number of parameters in the current statement.
     */
    SQLRETURN getNumParameters(SQLSMALLINT* parameterCount);

    /**
     * Return the current cursor name for the open result set.
     */
    SQLRETURN getCursorName(SQLCHAR* cursorName, SQLSMALLINT bufferLength,
        SQLSMALLINT* nameLength);

    /**
     * Return the current cursor name for the open result set.
     */
    SQLRETURN getCursorName(SQLWCHAR* cursorName, SQLSMALLINT bufferLength,
        SQLSMALLINT* nameLength);

    /**
     * Set the current cursor name for the open result set.
     */
    SQLRETURN setCursorName(SQLCHAR* cursorName, SQLSMALLINT nameLength);

    /**
     * Set the current cursor name for the open result set.
     */
    SQLRETURN setCursorName(SQLWCHAR* cursorName, SQLSMALLINT nameLength);

    /**
     * Returns the list of table, or schema names, and table types
     * as a result set in this statement.
     */
    SQLRETURN getTables(SQLCHAR* schemaName, SQLSMALLINT nameLength1,
        SQLCHAR* tableName, SQLSMALLINT nameLength2, SQLCHAR* tableTypes,
        SQLSMALLINT nameLength3);

    /**
     * Returns the list of table, or schema names, and table types
     * as a result set in this statement.
     */
    SQLRETURN getTables(SQLWCHAR* schemaName, SQLSMALLINT nameLength1,
        SQLWCHAR* tableName, SQLSMALLINT nameLength2, SQLWCHAR* tableTypes,
        SQLSMALLINT nameLength3);

    /**
     * Returns a description of the access rights for each table available
     * in a schema as a result set in this statement.
     */
    SQLRETURN getTablePrivileges(SQLCHAR* schemaName,
        SQLSMALLINT nameLength1, SQLCHAR* tableName,
        SQLSMALLINT nameLength2);

    /**
     * Returns a description of the access rights for each table available
     * in a schema as a result set in this statement.
     */
    SQLRETURN getTablePrivileges(SQLWCHAR* schemaName,
        SQLSMALLINT nameLength1, SQLWCHAR* tableName,
        SQLSMALLINT nameLength2);

    /**
     * Get the columns in given tables as a result set in this statement.
     */
    SQLRETURN getColumns(SQLCHAR* schemaName, SQLSMALLINT nameLength1,
        SQLCHAR* tableName, SQLSMALLINT nameLength2, SQLCHAR* columnName,
        SQLSMALLINT nameLength3);

    /**
     * Get the columns in given tables as a result set in this statement.
     */
    SQLRETURN getColumns(SQLWCHAR* schemaName, SQLSMALLINT nameLength1,
        SQLWCHAR* tableName, SQLSMALLINT nameLength2, SQLWCHAR* columnName,
        SQLSMALLINT nameLength3);

    /**
     * Get the following information for columns in given tables as a result set
     * in this statement.
     * 1. The optimal set of columns that uniquely identifies a row in the table.
     * 2. Columns that are automatically updated when any value in the row is
     *    updated by a transaction.
     */
    SQLRETURN getSpecialColumns(SQLUSMALLINT idType, SQLCHAR *schemaName,
        SQLSMALLINT nameLength1, SQLCHAR *tableName,
        SQLSMALLINT nameLength2, SQLUSMALLINT scope, SQLUSMALLINT nullable);

    /**
     * Get the following information for columns in given tables as a result set
     * in this statement.
     * 1. The optimal set of columns that uniquely identifies a row in the table.
     * 2. Columns that are automatically updated when any value in the row is
     *    updated by a transaction.
     */
    SQLRETURN getSpecialColumns(SQLUSMALLINT identifierType,
        SQLWCHAR* schemaName, SQLSMALLINT nameLength1, SQLWCHAR* tableName,
        SQLSMALLINT nameLength2, SQLUSMALLINT scope, SQLUSMALLINT nullable);

    /**
     * Retrieves a description of the access rights for a table's columns
     * as a result set in this statement.
     */
    SQLRETURN getColumnPrivileges(SQLCHAR* schemaName,
        SQLSMALLINT nameLength1, SQLCHAR* tableName,
        SQLSMALLINT nameLength2, SQLCHAR* columnName,
        SQLSMALLINT nameLength3);

    /**
     * Retrieves a description of the access rights for a table's columns
     * as a result set in this statement.
     */
    SQLRETURN getColumnPrivileges(SQLWCHAR* schemaName,
        SQLSMALLINT nameLength1, SQLWCHAR* tableName,
        SQLSMALLINT nameLength2, SQLWCHAR* columnName,
        SQLSMALLINT nameLength3);

    /**
     * Get information about the indexes in given tables as a result set
     * in this statement.
     */
    SQLRETURN getIndexInfo(SQLCHAR* schemaName, SQLSMALLINT nameLength1,
        SQLCHAR* tableName, SQLSMALLINT nameLength2, bool unique,
        bool approximate);

    /**
     * Get information about the indexes in given tables as a result set
     * in this statement.
     */
    SQLRETURN getIndexInfo(SQLWCHAR* schemaName, SQLSMALLINT nameLength1,
        SQLWCHAR* tableName, SQLSMALLINT nameLength2, bool unique,
        bool approximate);

    /**
     * Get information about the the given table's primary key columns
     * as a result set in this statement.
     */
    SQLRETURN getPrimaryKeys(SQLCHAR* schemaName, SQLSMALLINT nameLength1,
        SQLCHAR* tableName, SQLSMALLINT nameLength2);

    /**
     * Get information about the the given table's primary key columns
     * as a result set in this statement.
     */
    SQLRETURN getPrimaryKeys(SQLWCHAR* schemaName, SQLSMALLINT nameLength1,
        SQLWCHAR* tableName, SQLSMALLINT nameLength2);

    /**
     * Get information about the primary/unique key columns that are
     * referenced by the given table's foreign key columns (the
     * primary/unique keys imported by a table) as a result set in
     * this statement.
     */
    SQLRETURN getImportedKeys(SQLCHAR* schemaName, SQLSMALLINT nameLength1,
        SQLCHAR* tableName, SQLSMALLINT nameLength2);

    /**
     * Get information about the primary/unique key columns that are
     * referenced by the given table's foreign key columns (the
     * primary/unique keys imported by a table) as a result set in
     * this statement.
     */
    SQLRETURN getImportedKeys(SQLWCHAR* schemaName, SQLSMALLINT nameLength1,
        SQLWCHAR* tableName, SQLSMALLINT nameLength2);

    /**
     * Get information about the foreign key columns that reference the
     * given table's primary key columns (the foreign keys exported by a
     * table) as a result set in this statement.
     */
    SQLRETURN getExportedKeys(SQLCHAR* schemaName, SQLSMALLINT nameLength1,
        SQLCHAR* tableName, SQLSMALLINT nameLength2);

    /**
     * Get information about the foreign key columns that reference the
     * given table's primary key columns (the foreign keys exported by a
     * table) as a result set in this statement.
     */
    SQLRETURN getExportedKeys(SQLWCHAR* schemaName, SQLSMALLINT nameLength1,
        SQLWCHAR* tableName, SQLSMALLINT nameLength2);

    /**
     * Get information about the foreign key columns in the given foreign
     * key table that reference the primary key or the columns representing
     * a unique constraint of the parent table (could be the same or a
     * different table) as a result set in this statement.
     */
    SQLRETURN getCrossReference(SQLCHAR* parentSchemaName,
        SQLSMALLINT nameLength1, SQLCHAR* parentTableName,
        SQLSMALLINT nameLength2, SQLCHAR* foreignSchemaName,
        SQLSMALLINT nameLength3, SQLCHAR* foreignTableName,
        SQLSMALLINT nameLength4);

    /**
     * Get information about the foreign key columns in the given foreign
     * key table that reference the primary key or the columns representing
     * a unique constraint of the parent table (could be the same or a
     * different table) as a result set in this statement.
     */
    SQLRETURN getCrossReference(SQLWCHAR* parentSchemaName,
        SQLSMALLINT nameLength1, SQLWCHAR* parentTableName,
        SQLSMALLINT nameLength2, SQLWCHAR* foreignSchemaName,
        SQLSMALLINT nameLength3, SQLWCHAR* foreignTableName,
        SQLSMALLINT nameLength4);

    /**
     * Get information about the stored procedures available in the given
     * schemas as a result set in this statement.
     */
    SQLRETURN getProcedures(SQLCHAR* schemaPattern, SQLSMALLINT nameLength1,
        SQLCHAR* procedureNamePattern, SQLSMALLINT nameLength2);

    /**
     * Get information about the stored procedures available in the given
     * schemas as a result set in this statement.
     */
    SQLRETURN getProcedures(SQLWCHAR* schemaPattern,
        SQLSMALLINT nameLength1, SQLWCHAR* procedureNamePattern,
        SQLSMALLINT nameLength2);

    /**
     * Get information about the given schemas' stored procedure
     * parameter and result columns as a result set in this statement.
     */
    SQLRETURN getProcedureColumns(SQLCHAR* schemaPattern,
        SQLSMALLINT nameLength1, SQLCHAR* procedureNamePattern,
        SQLSMALLINT nameLength2, SQLCHAR* columnNamePattern,
        SQLSMALLINT nameLength3);

    /**
     * Get information about the given schemas' stored procedure
     * parameter and result columns as a result set in this statement.
     */
    SQLRETURN getProcedureColumns(SQLWCHAR* schemaPattern,
        SQLSMALLINT nameLength1, SQLWCHAR* procedureNamePattern,
        SQLSMALLINT nameLength2, SQLWCHAR* columnNamePattern,
        SQLSMALLINT nameLength3);

    /**
     * Determines whether more results are available on a statement containing
     * SELECT, UPDATE, INSERT, or DELETE statements and, if so, initializes
     *  processing for those results.
     */
    SQLRETURN getMoreResults();

    SQLRETURN getParamData(SQLPOINTER* valuePtr);

    SQLRETURN putData(SQLPOINTER dataPtr, SQLLEN strLen);

    /**
     * Get information about the data types supported by the system.
     */
    SQLRETURN getTypeInfo(SQLSMALLINT dataType);

    /**
     * Get information about the data types supported by the system.
     */
    SQLRETURN getTypeInfoW(SQLSMALLINT dataType);

    /**
     * Close any current open ResultSet clearing any remaining results.
     *
     * When the "ifPresent" argument is true, then don't fail
     * if there is no open cursor.
     */
    SQLRETURN closeResultSet(bool ifPresent);

    /**
     * Reset all the parameters to remove all parameters added so far.
     */
    SQLRETURN resetParameters();

    /**
     * Cancel the currently executing statement.
     */
    SQLRETURN cancel();

    /**
     * Close this statement.
     */
    SQLRETURN close() noexcept;
  };

} /* namespace snappydata */
} /* namespace io */

#endif /* SNAPPYSTATEMENT_H_ */
