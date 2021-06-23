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
 * SnappyStatement.cpp
 *
 *  Contains implementation of statement creation and execution ODBC API.
 */

#include "SnappyEnvironment.h"
#include "SnappyStatement.h"

#include <ParametersBatch.h>
#include <limits>

using namespace io::snappydata;

const char* SnappyStatement::s_GUID_FORMAT =
    "%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x";

#define PARAM_VALUE(param, offset) ((const char*)param.m_o_value + offset)

namespace io {
namespace snappydata {
  extern void getIntValue(SQLULEN value, SQLPOINTER resultValueBuffer,
      SQLINTEGER* valueLen, bool isUnsigned);
}
}

void SnappyStatement::setResultSet(std::unique_ptr<ResultSet> &rs) {
  if (rs) {
    m_resultSet = std::move(rs);
    rs.release();
    m_cursor.initialize(*m_resultSet, true);
  } else {
    m_resultSet = nullptr;
    m_cursor.clear();
  }
}

void SnappyStatement::setResultSet(std::shared_ptr<ResultSet> &rs) {
  if (rs) {
    m_resultSet = rs;
    m_cursor.initialize(*m_resultSet, true);
  } else {
    m_resultSet = nullptr;
    m_cursor.clear();
  }
}

SQLRETURN SnappyStatement::bindParameter(Parameters& paramValues,
    Parameter& param, std::map<int32_t, OutputParameter>* outParams,
    bool appendPutData, SQLLEN valueOffset, SQLLEN lenOffset) {
  if (!appendPutData) {
    // check if data at exec param and return
    if (param.m_isDataAtExecParam || IS_DATA_AT_EXEC(param.m_o_lenOrIndp)) {
      param.m_isDataAtExecParam = true;
      param.m_o_lenOrIndp = nullptr;
      return SQL_NEED_DATA;
    }
    // do nothing if parameter has already been filled in by SQLPutData
    if (param.m_isBound) return SQL_SUCCESS;
  }

  // TODO: handle SQL_DEFAULT_PARAM in *m_o_lenOrIndp
  if (param.m_o_value
      && param.m_inputOutputType != SQL_PARAM_OUTPUT) {
    const SQLSMALLINT ctype = param.m_o_valueType;
    SQLLEN len;

    switch (ctype) {
      case SQL_C_CHAR: {
        if (param.m_o_lenOrIndp) {
          len = *(param.m_o_lenOrIndp + lenOffset);
        } else {
          len = SQL_NTS;
        }
        const char* value = PARAM_VALUE(param, valueOffset);
        if (len < 0) {
          if (appendPutData) {
            paramValues.appendString(param.m_paramNum, value);
          } else {
            paramValues.setString(param.m_paramNum, value);
          }
        } else {
          if (appendPutData) {
            paramValues.appendString(param.m_paramNum, value,
                static_cast<size_t>(len));
          } else {
            paramValues.setString(param.m_paramNum, value,
                static_cast<size_t>(len));
          }
        }
        param.m_paramType = SQLType::VARCHAR;
        break;
      }
      case SQL_C_WCHAR: {
        if (param.m_o_lenOrIndp) {
          len = *(param.m_o_lenOrIndp + lenOffset);
        } else {
          len = SQL_NTS;
        }
        if (appendPutData) {
          paramValues.appendString(param.m_paramNum, std::move(
              StringFunctions::toString(
                  (const SQLWCHAR*)PARAM_VALUE(param, valueOffset), len)));
        } else {
          paramValues.setString(param.m_paramNum, std::move(
              StringFunctions::toString(
                  (const SQLWCHAR*)PARAM_VALUE(param, valueOffset), len)));
        }
        param.m_paramType = SQLType::VARCHAR;
        break;
      }
      case SQL_C_SSHORT:
      case SQL_C_SHORT:
        paramValues.setShort(param.m_paramNum,
            *(const SQLSMALLINT*)PARAM_VALUE(param, valueOffset));
        param.m_paramType = SQLType::SMALLINT;
        if (param.m_o_valueSize <= 0) {
          param.m_o_valueSize = sizeof(SQLSMALLINT);
        }
        break;
      case SQL_C_USHORT:
        paramValues.setUnsignedShort(param.m_paramNum,
            *(const SQLUSMALLINT*)PARAM_VALUE(param, valueOffset));
        param.m_paramType = SQLType::SMALLINT;
        if (param.m_o_valueSize <= 0) {
          param.m_o_valueSize = sizeof(SQLUSMALLINT);
        }
        break;
      case SQL_C_SLONG:
        paramValues.setInt(param.m_paramNum,
            *(const SQLINTEGER*)PARAM_VALUE(param, valueOffset));
        param.m_paramType = SQLType::INTEGER;
        if (param.m_o_valueSize <= 0) {
          param.m_o_valueSize = sizeof(SQLINTEGER);
        }
        break;
      case SQL_C_ULONG:
        paramValues.setUnsignedInt(param.m_paramNum,
            *(const SQLUINTEGER*)PARAM_VALUE(param, valueOffset));
        param.m_paramType = SQLType::INTEGER;
        if (param.m_o_valueSize <= 0) {
          param.m_o_valueSize = sizeof(SQLUINTEGER);
        }
        break;
      case SQL_C_LONG:
        paramValues.setInt(param.m_paramNum,
            *(const SQLINTEGER*)PARAM_VALUE(param, valueOffset));
        param.m_paramType = SQLType::INTEGER;
        if (param.m_o_valueSize <= 0) {
          param.m_o_valueSize = sizeof(SQLINTEGER);
        }
        break;
      case SQL_C_FLOAT:
        paramValues.setFloat(param.m_paramNum,
            *(const SQLREAL*)PARAM_VALUE(param, valueOffset));
        param.m_paramType = SQLType::FLOAT;
        if (param.m_o_valueSize <= 0) {
          param.m_o_valueSize = sizeof(SQLREAL);
        }
        break;
      case SQL_C_DOUBLE:
        paramValues.setDouble(param.m_paramNum,
            *(const SQLDOUBLE*)PARAM_VALUE(param, valueOffset));
        param.m_paramType = SQLType::DOUBLE;
        if (param.m_o_valueSize <= 0) {
          param.m_o_valueSize = sizeof(SQLDOUBLE);
        }
        break;
      case SQL_C_BIT:
      case SQL_C_UTINYINT:
      case SQL_C_TINYINT:
        paramValues.setUnsignedByte(param.m_paramNum,
            *(const SQLCHAR*)PARAM_VALUE(param, valueOffset));
        param.m_paramType = SQLType::TINYINT;
        if (param.m_o_valueSize <= 0) {
          param.m_o_valueSize = sizeof(SQLCHAR);
        }
        break;
      case SQL_C_STINYINT:
        paramValues.setByte(param.m_paramNum,
            *(const SQLSCHAR*)PARAM_VALUE(param, valueOffset));
        param.m_paramType = SQLType::TINYINT;
        if (param.m_o_valueSize <= 0) {
          param.m_o_valueSize = sizeof(SQLSCHAR);
        }
        break;
      case SQL_C_SBIGINT:
        paramValues.setInt64(param.m_paramNum,
            *(const SQLBIGINT*)PARAM_VALUE(param, valueOffset));
        param.m_paramType = SQLType::BIGINT;
        if (param.m_o_valueSize <= 0) {
          param.m_o_valueSize = sizeof(SQLBIGINT);
        }
        break;
      case SQL_C_UBIGINT:
        paramValues.setUnsignedInt64(param.m_paramNum,
            *(const SQLUBIGINT*)PARAM_VALUE(param, valueOffset));
        param.m_paramType = SQLType::BIGINT;
        if (param.m_o_valueSize <= 0) {
          param.m_o_valueSize = sizeof(SQLUBIGINT);
        }
        break;
      case SQL_C_BINARY: {
        if (param.m_o_lenOrIndp) {
          len = *(param.m_o_lenOrIndp + lenOffset);
        } else {
          len = SQL_NTS;
        }
        const int8_t* bytes = (const int8_t*)PARAM_VALUE(param, valueOffset);
        if (len == SQL_NTS) {
          // assume NULL terminated data
          len = ::strlen((const char*)bytes);
        }
        if (appendPutData) {
          paramValues.appendBinary(param.m_paramNum, bytes, len);
        } else {
          paramValues.setBinary(param.m_paramNum, bytes, len);
        }
        param.m_paramType = SQLType::VARBINARY;
        break;
      }
      case SQL_C_DATE:
      case SQL_C_TYPE_DATE: {
        const SQL_DATE_STRUCT* date = (const SQL_DATE_STRUCT*)PARAM_VALUE(
            param, valueOffset);
        DateTime dt(date->year, date->month, date->day);
        paramValues.setDate(param.m_paramNum, dt);
        param.m_paramType = SQLType::DATE;
        if (param.m_o_valueSize <= 0) {
          param.m_o_valueSize = sizeof(SQL_DATE_STRUCT);
        }
        break;
      }
      case SQL_C_TIME:
      case SQL_C_TYPE_TIME: {
        const SQL_TIME_STRUCT* time = (const SQL_TIME_STRUCT*)PARAM_VALUE(
            param, valueOffset);
        DateTime tm(1970, 1, 1, time->hour, time->minute, time->second);
        paramValues.setTime(param.m_paramNum, tm);
        param.m_paramType = SQLType::TIME;
        if (param.m_o_valueSize <= 0) {
          param.m_o_valueSize = sizeof(SQL_TIME_STRUCT);
        }
        break;
      }
      case SQL_C_TIMESTAMP:
      case SQL_C_TYPE_TIMESTAMP: {
        const SQL_TIMESTAMP_STRUCT* timestamp =
            (const SQL_TIMESTAMP_STRUCT*)PARAM_VALUE(param, valueOffset);
        Timestamp ts(timestamp->year, timestamp->month, timestamp->day,
            timestamp->hour, timestamp->minute, timestamp->second,
            timestamp->fraction);
        paramValues.setTimestamp(param.m_paramNum, ts);
        param.m_paramType = SQLType::TIMESTAMP;
        if (param.m_o_valueSize <= 0) {
          param.m_o_valueSize = sizeof(SQL_TIMESTAMP_STRUCT);
        }
        break;
      }
      case SQL_C_NUMERIC: {
        const SQL_NUMERIC_STRUCT* numeric =
            (const SQL_NUMERIC_STRUCT*)PARAM_VALUE(param, valueOffset);
        const int precision = numeric->precision;
        if (precision == 0) {
          paramValues.setDecimal(param.m_paramNum, Decimal::ZERO);
        } else {
          const SQLCHAR* mag = numeric->val;
          // skip trailing zeros to get the length
          size_t maglen = SQL_MAX_NUMERIC_LEN;
          const SQLCHAR* magp = (mag + maglen - 1);
          while (*magp == 0) {
            maglen--;
            magp--;
          }
          paramValues.setDecimal(param.m_paramNum,
              numeric->sign == 1 ? 1 : -1, numeric->scale, (const int8_t*)mag,
              maglen, false);
        }
        param.m_paramType = SQLType::DECIMAL;
        if (param.m_o_valueSize <= 0) {
          param.m_o_valueSize = sizeof(SQL_NUMERIC_STRUCT);
        }
        break;
      }
      case SQL_C_INTERVAL_YEAR: {
        const SQL_INTERVAL_STRUCT* interval =
            (const SQL_INTERVAL_STRUCT*)PARAM_VALUE(param, valueOffset);
        const int years = interval->intval.year_month.year;
        paramValues.setInt(param.m_paramNum,
            (interval->interval_sign == SQL_FALSE) ? years : -years);
        param.m_paramType = SQLType::INTEGER;
        if (param.m_o_valueSize <= 0) {
          param.m_o_valueSize = sizeof(SQL_INTERVAL_STRUCT);
        }
        break;
      }
      case SQL_C_INTERVAL_MONTH: {
        const SQL_INTERVAL_STRUCT* interval =
            (const SQL_INTERVAL_STRUCT*)PARAM_VALUE(param, valueOffset);
        const int months = interval->intval.year_month.month;
        paramValues.setInt(param.m_paramNum,
            (interval->interval_sign == SQL_FALSE) ? months : -months);
        param.m_paramType = SQLType::INTEGER;
        if (param.m_o_valueSize <= 0) {
          param.m_o_valueSize = sizeof(SQL_INTERVAL_STRUCT);
        }
        break;
      }
      case SQL_C_INTERVAL_DAY: {
        const SQL_INTERVAL_STRUCT* interval =
            (const SQL_INTERVAL_STRUCT*)PARAM_VALUE(param, valueOffset);
        const int days = interval->intval.day_second.day;
        paramValues.setInt(param.m_paramNum,
            (interval->interval_sign == SQL_FALSE) ? days : -days);
        param.m_paramType = SQLType::INTEGER;
        if (param.m_o_valueSize <= 0) {
          param.m_o_valueSize = sizeof(SQL_INTERVAL_STRUCT);
        }
        break;
      }
      case SQL_C_INTERVAL_HOUR: {
        const SQL_INTERVAL_STRUCT* interval =
            (const SQL_INTERVAL_STRUCT*)PARAM_VALUE(param, valueOffset);
        const int hours = interval->intval.day_second.hour;
        paramValues.setInt(param.m_paramNum,
            (interval->interval_sign == SQL_FALSE) ? hours : -hours);
        param.m_paramType = SQLType::INTEGER;
        if (param.m_o_valueSize <= 0) {
          param.m_o_valueSize = sizeof(SQL_INTERVAL_STRUCT);
        }
        break;
      }
      case SQL_C_INTERVAL_MINUTE: {
        const SQL_INTERVAL_STRUCT* interval =
            (const SQL_INTERVAL_STRUCT*)PARAM_VALUE(param, valueOffset);
        const int minutes = interval->intval.day_second.minute;
        paramValues.setInt(param.m_paramNum,
            (interval->interval_sign == SQL_FALSE) ? minutes : -minutes);
        param.m_paramType = SQLType::INTEGER;
        if (param.m_o_valueSize <= 0) {
          param.m_o_valueSize = sizeof(SQL_INTERVAL_STRUCT);
        }
        break;
      }
      case SQL_C_INTERVAL_SECOND: {
        const SQL_INTERVAL_STRUCT* interval =
            (const SQL_INTERVAL_STRUCT*)PARAM_VALUE(param, valueOffset);
        const int seconds = interval->intval.day_second.second;
        paramValues.setInt(param.m_paramNum,
            (interval->interval_sign == SQL_FALSE) ? seconds : -seconds);
        param.m_paramType = SQLType::INTEGER;
        if (param.m_o_valueSize <= 0) {
          param.m_o_valueSize = sizeof(SQL_INTERVAL_STRUCT);
        }
        break;
      }
      case SQL_C_INTERVAL_YEAR_TO_MONTH: {
        const SQL_INTERVAL_STRUCT* interval =
            (const SQL_INTERVAL_STRUCT*)PARAM_VALUE(param, valueOffset);
        const int months = interval->intval.year_month.year * 12
            + interval->intval.year_month.month;
        paramValues.setInt(param.m_paramNum,
            (interval->interval_sign == SQL_FALSE) ? months : -months);
        param.m_paramType = SQLType::INTEGER;
        if (param.m_o_valueSize <= 0) {
          param.m_o_valueSize = sizeof(SQL_INTERVAL_STRUCT);
        }
        break;
      }
      case SQL_C_INTERVAL_DAY_TO_HOUR: {
        const SQL_INTERVAL_STRUCT* interval =
            (const SQL_INTERVAL_STRUCT*)PARAM_VALUE(param, valueOffset);
        const SQLBIGINT hours = ((SQLBIGINT)interval->intval.day_second.day)
            * 24 + interval->intval.day_second.hour;
        paramValues.setInt64(param.m_paramNum,
            (interval->interval_sign == SQL_FALSE) ? hours : -hours);
        param.m_paramType = SQLType::BIGINT;
        if (param.m_o_valueSize <= 0) {
          param.m_o_valueSize = sizeof(SQL_INTERVAL_STRUCT);
        }
        break;
      }
      case SQL_C_INTERVAL_DAY_TO_MINUTE: {
        const SQL_INTERVAL_STRUCT* interval =
            (const SQL_INTERVAL_STRUCT*)PARAM_VALUE(param, valueOffset);
        const SQLBIGINT hours = ((SQLBIGINT)interval->intval.day_second.day)
            * 24 + interval->intval.day_second.hour;
        const SQLBIGINT minutes = hours * 60
            + interval->intval.day_second.minute;
        paramValues.setInt64(param.m_paramNum,
            (interval->interval_sign == SQL_FALSE) ? minutes : -minutes);
        param.m_paramType = SQLType::BIGINT;
        if (param.m_o_valueSize <= 0) {
          param.m_o_valueSize = sizeof(SQL_INTERVAL_STRUCT);
        }
        break;
      }
      case SQL_C_INTERVAL_DAY_TO_SECOND: {
        const SQL_INTERVAL_STRUCT* interval =
            (const SQL_INTERVAL_STRUCT*)PARAM_VALUE(param, valueOffset);
        const SQLBIGINT hours = ((SQLBIGINT)interval->intval.day_second.day)
            * 24 + interval->intval.day_second.hour;
        const SQLBIGINT minutes = hours * 60
            + interval->intval.day_second.minute;
        const SQLBIGINT seconds = minutes * 60
            + interval->intval.day_second.second;
        paramValues.setInt64(param.m_paramNum,
            (interval->interval_sign == SQL_FALSE) ? seconds : -seconds);
        param.m_paramType = SQLType::BIGINT;
        if (param.m_o_valueSize <= 0) {
          param.m_o_valueSize = sizeof(SQL_INTERVAL_STRUCT);
        }
        break;
      }
      case SQL_C_INTERVAL_HOUR_TO_MINUTE: {
        const SQL_INTERVAL_STRUCT* interval =
            (const SQL_INTERVAL_STRUCT*)PARAM_VALUE(param, valueOffset);
        const SQLBIGINT minutes = ((SQLBIGINT)interval->intval.day_second.hour)
            * 60 + interval->intval.day_second.minute;
        paramValues.setInt64(param.m_paramNum,
            (interval->interval_sign == SQL_FALSE) ? minutes : -minutes);
        param.m_paramType = SQLType::BIGINT;
        if (param.m_o_valueSize <= 0) {
          param.m_o_valueSize = sizeof(SQL_INTERVAL_STRUCT);
        }
        break;
      }
      case SQL_C_INTERVAL_HOUR_TO_SECOND: {
        const SQL_INTERVAL_STRUCT* interval =
            (const SQL_INTERVAL_STRUCT*)PARAM_VALUE(param, valueOffset);
        const SQLBIGINT minutes = ((SQLBIGINT)interval->intval.day_second.hour)
            * 60 + interval->intval.day_second.minute;
        const SQLBIGINT seconds = minutes * 60
            + interval->intval.day_second.second;
        paramValues.setInt64(param.m_paramNum,
            (interval->interval_sign == SQL_FALSE) ? seconds : -seconds);
        param.m_paramType = SQLType::BIGINT;
        if (param.m_o_valueSize <= 0) {
          param.m_o_valueSize = sizeof(SQL_INTERVAL_STRUCT);
        }
        break;
      }
      case SQL_C_INTERVAL_MINUTE_TO_SECOND: {
        const SQL_INTERVAL_STRUCT* interval =
            (const SQL_INTERVAL_STRUCT*)PARAM_VALUE(param, valueOffset);
        const SQLBIGINT seconds =
            ((SQLBIGINT)interval->intval.day_second.minute) * 60
                + interval->intval.day_second.second;
        paramValues.setInt64(param.m_paramNum,
            (interval->interval_sign == SQL_FALSE) ? seconds : -seconds);
        param.m_paramType = SQLType::BIGINT;
        if (param.m_o_valueSize <= 0) {
          param.m_o_valueSize = sizeof(SQL_INTERVAL_STRUCT);
        }
        break;
      }
      case SQL_C_GUID: {
        const SQLGUID* guid = (const SQLGUID*)PARAM_VALUE(param, valueOffset);
        char guidChars[40];
        const int maxLen = sizeof(guidChars) - 1;
        // convert GUID to string representation
        const int guidLen = ::snprintf(guidChars, maxLen, s_GUID_FORMAT,
            guid->Data1, guid->Data2, guid->Data3, guid->Data4[0],
            guid->Data4[1], guid->Data4[2], guid->Data4[3], guid->Data4[4],
            guid->Data4[5], guid->Data4[6], guid->Data4[7]);
        paramValues.setString(param.m_paramNum, guidChars,
            static_cast<size_t>(guidLen));
        param.m_paramType = SQLType::VARCHAR;
        if (param.m_o_valueSize <= 0) {
          param.m_o_valueSize = sizeof(SQLGUID);
        }
        break;
      }
      default:
        setException(
            GET_SQLEXCEPTION2(SQLStateMessage::INVALID_CTYPE_MSG, ctype,
                (int)param.m_paramNum));
        return SQL_ERROR;
    }
  } else {
    paramValues.setNull(param.m_paramNum, true);
    param.m_paramType = convertTypeToSQLType(param.m_o_paramType,
        param.m_paramNum);
  }
  if ((param.m_inputOutputType == SQL_PARAM_OUTPUT
      || param.m_inputOutputType == SQL_PARAM_INPUT_OUTPUT)) {
    if (outParams) {
      OutputParameter &outParam = outParams->operator [](param.m_paramNum);
      outParam.setType(param.m_paramType);
      outParam.setScale(param.m_o_scale);
    } else {
      m_pstmt->registerOutParameter(param.m_paramNum, param.m_paramType,
          param.m_o_scale);
    }
  }
  return SQL_SUCCESS;
}

SQLRETURN SnappyStatement::newStatement(SnappyConnection* conn,
    SnappyStatement*& stmtRef) {
  try {
    if (conn) {
      stmtRef = new SnappyStatement(conn);
      return SQL_SUCCESS;
    } else {
      return SnappyHandleBase::errorNullHandle(SQL_HANDLE_DBC);
    }
  } catch (SQLException& sqle) {
    if (conn) {
      conn->setException(sqle);
    } else {
      SnappyHandleBase::setGlobalException(sqle);
    }
    return SQL_ERROR;
  } catch (std::exception& se) {
    if (conn) {
      conn->setException(__FILE__, __LINE__, se);
    } else {
      SnappyHandleBase::setGlobalException(__FILE__, __LINE__, se);
    }
    return SQL_ERROR;
  }
}

SQLRETURN SnappyStatement::freeStatement(SnappyStatement* stmt,
    SQLUSMALLINT option) {
  if (stmt) {
    SQLRETURN result = SQL_SUCCESS;
    try {
      switch (option) {
        case SQL_CLOSE:
          result = stmt->closeResultSet(true);
          stmt->m_result.reset();
          break;
        case SQL_UNBIND:
          stmt->m_outputFields.clear();
          break;
        case SQL_DROP:
          delete stmt;
          break;
        case SQL_RESET_PARAMS:
          result = stmt->resetParameters();
          break;
        default:
          stmt->setException(
              GET_SQLEXCEPTION2(SQLStateMessage::OPTION_TYPE_OUT_OF_RANGE,
                  option, "freeStatement"));
          result = SQL_ERROR;
          break;
      }
    } catch (SQLException& sqle) {
      SnappyHandleBase::setGlobalException(sqle);
      return SQL_ERROR;
    } catch (std::exception& se) {
      SnappyHandleBase::setGlobalException(__FILE__, __LINE__, se);
      return SQL_ERROR;
    }
    return result;
  } else {
    return SnappyHandleBase::errorNullHandle(SQL_HANDLE_STMT);
  }
}

SQLRETURN SnappyStatement::bindParameters(
    std::map<int32_t, OutputParameter>* outParams) {
  SQLRETURN retVal = SQL_SUCCESS;
  m_execParams.resize(m_params.size());
  for (auto &param : m_params) {
    retVal = bindParameter(m_execParams, param, outParams);
    if (retVal != SQL_SUCCESS) {
      break;
    }
  }
  return retVal;
}

static inline void updateStatus(SQLUSMALLINT& status, SQLRETURN& result,
    SQLRETURN ret) noexcept {
  // SQL_PARAM_* values are identical to SQL_ROW_* so this works for both
  if (ret != SQL_SUCCESS) {
    if (status != SQL_PARAM_ERROR) {
      if (ret == SQL_SUCCESS_WITH_INFO) {
        status = SQL_PARAM_SUCCESS_WITH_INFO;
      } else {
        status = SQL_PARAM_ERROR;
      }
    }
    if (result == SQL_SUCCESS || result == SQL_SUCCESS_WITH_INFO
        || ret == SQL_ERROR) {
      result = ret;
    }
  }
}

SQLRETURN SnappyStatement::bindArrayOfParameters(ParametersBatch& paramsBatch,
    SQLULEN setSize, SQLULEN* bindOffsetPtr, SQLULEN bindingOrientation,
    SQLUSMALLINT* statusArr, SQLULEN* processedPtr) {
  SQLRETURN result = SQL_SUCCESS;
  int numSetProcessed = 0;
  SQLLEN bindOffset = 0;

  if (bindOffsetPtr) {
    bindOffset = *bindOffsetPtr;
  }

  SQLLEN offset = bindOffset;
  const auto structSize = bindingOrientation;
  for (uint32_t i = 0; i < setSize; i++) {
    // SQL_PARAM_SUCCESS == SQL_ROW_SUCCESS == SQL_SUCCESS == 0
    SQLUSMALLINT status = SQL_SUCCESS;
    m_execParams.resize(paramsBatch.numParams());
    // SQL_PARAM_BIND_BY_COLUMN == SQL_BIND_BY_COLUMN
    if (structSize == SQL_BIND_BY_COLUMN) {
      /*
       * Column wise binding : When using column-wise binding,
       * an application binds one or two, or in some cases three,
       * arrays to each column for which data is to be returned.
       * The first array holds the data values, and the second array
       * holds length/indicator buffers.
       */
      for (Parameter& param : m_params) {
        // for the first call when i==0 then m_o_valueSize may not be set
        // which is fine but will be set in subsequent calls by bindParameter
        const SQLLEN valueOffset = (i * param.m_o_valueSize) + bindOffset;
        updateStatus(status, result, bindParameter(m_execParams, param, nullptr,
            false, valueOffset, offset));
      }
      offset++;
    } else {
      /*
       * ROW_WISE_BINDING : When using row-wise binding, an application
       * defines a structure containing one or two, or in some cases three,
       * elements for each column for which data is to be returned.
       * The first element holds the data value, and the second element
       * holds the length/indicator buffer
       */
      for (Parameter& param : m_params) {
        updateStatus(status, result, bindParameter(m_execParams, param, nullptr,
            false, offset, offset));
      }
      offset += structSize;
    }
    paramsBatch.moveParameters(m_execParams);
    m_execParams.clear();
    numSetProcessed++;
    if (statusArr) {
      statusArr[i] = status;
    }
  }
  if (processedPtr) {
    *processedPtr = numSetProcessed;
  }

  return result;
}

SQLRETURN SnappyStatement::fillOutput(const Row& outputRow,
    const uint32_t columnNum, SQLPOINTER value, const SQLLEN valueSize,
    SQLSMALLINT ctype, const SQLUINTEGER precision, SQLLEN* lenOrIndp) {
  SQLRETURN res = SQL_SUCCESS;

  if (ctype == SQL_C_DEFAULT) {
    ctype = convertSQLTypeToCType(outputRow.getType(columnNum));
  }
  switch (ctype) {
    case SQL_C_CHAR: {
      auto outStr = outputRow.getString(columnNum, precision);
      if (outStr) {
        if (StringFunctions::copyString((const SQLCHAR*)outStr->c_str(),
            outStr->length(), (SQLCHAR*)value, valueSize, lenOrIndp)) {
          res = SQL_SUCCESS_WITH_INFO;
        }
      } else {
        if (valueSize > 0) {
          *((SQLCHAR*)value) = '\0';
        }
        if (lenOrIndp) {
          *lenOrIndp = SQL_NULL_DATA;
        }
      }
      break;
    }
    case SQL_C_WCHAR: {
      auto outStr = outputRow.getString(columnNum, precision);
      if (outStr) {
        // as per MSDN docs the "valueSize" is in bytes hence length is half of it
        if (StringFunctions::copyString((const SQLCHAR*)outStr->c_str(),
            outStr->length(), (SQLWCHAR*)value, valueSize >> 1, lenOrIndp)) {
          res = SQL_SUCCESS_WITH_INFO;
        }
        if (lenOrIndp) {
          *lenOrIndp <<= 1; // expects number of bytes
        }
      } else {
        if (valueSize > 0) {
          *((SQLWCHAR*)value) = 0;
        }
        if (lenOrIndp) {
          *lenOrIndp = SQL_NULL_DATA;
        }
      }
      break;
    }
    case SQL_C_SSHORT:
    case SQL_C_SHORT: {
      const int16_t v = outputRow.getShort(columnNum);
      *(SQLSMALLINT*)value = v;
      if (lenOrIndp) {
        if (v == 0 && outputRow.isNull(columnNum)) {
          *lenOrIndp = SQL_NULL_DATA;
        } else {
          *lenOrIndp = sizeof(SQLSMALLINT);
        }
      }
      break;
    }
    case SQL_C_USHORT: {
      const uint16_t v = outputRow.getUnsignedShort(columnNum);
      *(SQLUSMALLINT*)value = v;
      if (lenOrIndp) {
        if (v == 0 && outputRow.isNull(columnNum)) {
          *lenOrIndp = SQL_NULL_DATA;
        } else {
          *lenOrIndp = sizeof(SQLUSMALLINT);
        }
      }
      break;
    }
    case SQL_C_SLONG: {
      const int32_t v = outputRow.getInt(columnNum);
      *(SQLINTEGER*)value = v;
      if (lenOrIndp) {
        if (v == 0 && outputRow.isNull(columnNum)) {
          *lenOrIndp = SQL_NULL_DATA;
        } else {
          *lenOrIndp = sizeof(SQLINTEGER);
        }
      }
      break;
    }
    case SQL_C_ULONG: {
      const uint32_t v = outputRow.getUnsignedInt(columnNum);
      *(SQLUINTEGER*)value = v;
      if (lenOrIndp) {
        if (v == 0 && outputRow.isNull(columnNum)) {
          *lenOrIndp = SQL_NULL_DATA;
        } else {
          *lenOrIndp = sizeof(SQLUINTEGER);
        }
      }
      break;
    }
    case SQL_C_LONG: {
      const int32_t v = outputRow.getInt(columnNum);
      *(SQLINTEGER*)value = v;
      if (lenOrIndp) {
        if (v == 0 && outputRow.isNull(columnNum)) {
          *lenOrIndp = SQL_NULL_DATA;
        } else {
          *lenOrIndp = sizeof(SQLLEN);
        }
      }
      break;
    }
    case SQL_C_FLOAT: {
      const float v = outputRow.getFloat(columnNum);
      *(SQLREAL*)value = v;
      if (lenOrIndp) {
        if (v == 0 && outputRow.isNull(columnNum)) {
          *lenOrIndp = SQL_NULL_DATA;
        } else {
          *lenOrIndp = sizeof(SQLREAL);
        }
      }
      break;
    }
    case SQL_C_DOUBLE: {
      const double v = outputRow.getDouble(columnNum);
      *(SQLDOUBLE*)value = v;
      if (lenOrIndp) {
        if (v == 0 && outputRow.isNull(columnNum)) {
          *lenOrIndp = SQL_NULL_DATA;
        } else {
          *lenOrIndp = sizeof(SQLDOUBLE);
        }
      }
      break;
    }
    case SQL_C_BIT: {
      const int8_t v = outputRow.getByte(columnNum);
      *(SQLCHAR*)value = v;
      if (lenOrIndp) {
        if (v == 0 && outputRow.isNull(columnNum)) {
          *lenOrIndp = SQL_NULL_DATA;
        } else {
          *lenOrIndp = sizeof(SQLCHAR);
        }
      }
      break;
    }
    case SQL_C_UTINYINT:
    case SQL_C_TINYINT: {
      const uint8_t v = outputRow.getUnsignedByte(columnNum);
      *(SQLCHAR*)value = v;
      if (lenOrIndp) {
        if (v == 0 && outputRow.isNull(columnNum)) {
          *lenOrIndp = SQL_NULL_DATA;
        } else {
          *lenOrIndp = sizeof(SQLCHAR);
        }
      }
      break;
    }
    case SQL_C_STINYINT: {
      const int8_t v = outputRow.getByte(columnNum);
      *(SQLSCHAR*)value = v;
      if (lenOrIndp) {
        if (v == 0 && outputRow.isNull(columnNum)) {
          *lenOrIndp = SQL_NULL_DATA;
        } else {
          *lenOrIndp = sizeof(SQLSCHAR);
        }
      }
      break;
    }
    case SQL_C_SBIGINT: {
      const int64_t v = outputRow.getInt64(columnNum);
      *(SQLBIGINT*)value = v;
      if (lenOrIndp) {
        if (v == 0 && outputRow.isNull(columnNum)) {
          *lenOrIndp = SQL_NULL_DATA;
        } else {
          *lenOrIndp = sizeof(SQLBIGINT);
        }
      }
      break;
    }
    case SQL_C_UBIGINT: {
      const uint64_t v = outputRow.getUnsignedInt64(columnNum);
      *(SQLUBIGINT*)value = v;
      if (lenOrIndp) {
        if (v == 0 && outputRow.isNull(columnNum)) {
          *lenOrIndp = SQL_NULL_DATA;
        } else {
          *lenOrIndp = sizeof(SQLUBIGINT);
        }
      }
      break;
    }
    case SQL_C_BINARY: {
      auto bytes = outputRow.getBinary(columnNum);
      if (bytes) {
        const size_t bytesLen = bytes->size();
        SQLLEN outLen = valueSize;
        if (outLen < 0 || (size_t)outLen >= bytesLen) {
          outLen = bytesLen;
        }
        ::memcpy(value, bytes->c_str(), outLen);
        if (lenOrIndp) {
          *lenOrIndp = outLen;
        }
      } else if (lenOrIndp) {
        *lenOrIndp = SQL_NULL_DATA;
      }
      break;
    }
    case SQL_C_DATE:
    case SQL_C_TYPE_DATE: {
      DateTime date = outputRow.getDate(columnNum);
      if (date.getEpochTime() != 0 || !outputRow.isNull(columnNum)) {
        SQL_DATE_STRUCT* outDate = (SQL_DATE_STRUCT*)value;
        struct tm t = date.toDateTime(false);
        outDate->year = t.tm_year + 1900;
        // MONTH in calender are zero numbered 0 = Jan
        outDate->month = t.tm_mon + 1;
        outDate->day = t.tm_mday;
        if (lenOrIndp) {
          *lenOrIndp = sizeof(SQL_DATE_STRUCT);
        }
      } else {
        ::memset(value, 0, sizeof(SQL_DATE_STRUCT));
        if (lenOrIndp) {
          *lenOrIndp = SQL_NULL_DATA;
        }
      }
      break;
    }
    case SQL_C_TIME:
    case SQL_C_TYPE_TIME: {
      DateTime time = outputRow.getTime(columnNum);
      if (time.getEpochTime() != 0 || !outputRow.isNull(columnNum)) {
        SQL_TIME_STRUCT* outTime = (SQL_TIME_STRUCT*)value;
        struct tm t = time.toDateTime(false);
        outTime->hour = t.tm_hour;
        outTime->minute = t.tm_min;
        outTime->second = t.tm_sec;
        if (lenOrIndp) {
          *lenOrIndp = sizeof(SQL_TIME_STRUCT);
        }
      } else {
        ::memset(value, 0, sizeof(SQL_TIME_STRUCT));
        if (lenOrIndp) {
          *lenOrIndp = SQL_NULL_DATA;
        }
      }
      break;
    }
    case SQL_C_TIMESTAMP:
    case SQL_C_TYPE_TIMESTAMP: {
      Timestamp ts = outputRow.getTimestamp(columnNum);
      if (ts.getEpochTime() != 0 || !outputRow.isNull(columnNum)) {
        SQL_TIMESTAMP_STRUCT* outTs = (SQL_TIMESTAMP_STRUCT*)value;
        struct tm t = ts.toDateTime(false);
        // tm_year is number of years from 1900, so adding 1900 for correct value
        outTs->year = t.tm_year + 1900;
        // MONTH in calender are zero numbered 0 = Jan
        outTs->month = t.tm_mon + 1;
        outTs->day = t.tm_mday;
        outTs->hour = t.tm_hour;
        outTs->minute = t.tm_min;
        outTs->second = t.tm_sec;
        outTs->fraction = ts.getNanos();
        if (lenOrIndp) {
          *lenOrIndp = sizeof(SQL_TIMESTAMP_STRUCT);
        }
      } else {
        ::memset(value, 0, sizeof(SQL_TIMESTAMP_STRUCT));
        if (lenOrIndp) {
          *lenOrIndp = SQL_NULL_DATA;
        }
      }
      break;
    }
    case SQL_C_NUMERIC: {
      auto tdec = outputRow.getTDecimal(columnNum, SQL_MAX_NUMERIC_LEN);
      if (tdec) {
        SQL_NUMERIC_STRUCT* numeric = (SQL_NUMERIC_STRUCT*)value;
        const auto magLen = tdec->magnitude.size();
        const auto scale = tdec->scale;
        if (magLen <= SQL_MAX_NUMERIC_LEN && scale <= SQL_MAX_NUMERIC_LEN) {
          numeric->precision = static_cast<SQLCHAR>(magLen);
          numeric->scale = static_cast<SQLSCHAR>(scale);
          numeric->sign = tdec->signum;
          ::memcpy(numeric->val, tdec->magnitude.c_str(), magLen);
        } else {
          // need to truncate fractional portion (result may still overflow)
          Decimal dec(*tdec);
          size_t wholeLen;
          if (dec.wholeDigits(numeric->val, SQL_MAX_NUMERIC_LEN, wholeLen)) {
            numeric->precision = static_cast<SQLCHAR>(SQL_MAX_NUMERIC_LEN);
            numeric->scale = 0;
            numeric->sign = dec.signum();
            setException(
                GET_SQLEXCEPTION2(SQLStateMessage::NUMERIC_TRUNCATED_MSG,
                    "output column", magLen - wholeLen));
            res = SQL_SUCCESS_WITH_INFO;
          } else {
            setException(GET_SQLEXCEPTION2(
                SQLStateMessage::LANG_OUTSIDE_RANGE_FOR_NUMERIC_MSG,
                SQL_MAX_NUMERIC_LEN, magLen));
            res = SQL_ERROR;
          }
        }
        if (lenOrIndp) {
          *lenOrIndp = magLen;
        }
      } else {
        ::memset(value, 0, sizeof(SQL_NUMERIC_STRUCT));
        if (lenOrIndp) {
          *lenOrIndp = SQL_NULL_DATA;
        }
      }
      break;
    }
    case SQL_C_INTERVAL_YEAR: {
      SQL_INTERVAL_STRUCT* interval = (SQL_INTERVAL_STRUCT*)value;
      interval->interval_type = SQL_IS_YEAR;
      const int32_t year = outputRow.getInt(columnNum);
      if (year >= 0) {
        interval->intval.year_month.year = year;
        interval->interval_sign = SQL_FALSE;
      } else {
        interval->intval.year_month.year = -year;
        interval->interval_sign = SQL_TRUE;
      }
      interval->intval.year_month.month = 0;
      if (lenOrIndp) {
        if (year == 0 && outputRow.isNull(columnNum)) {
          *lenOrIndp = SQL_NULL_DATA;
        } else {
          *lenOrIndp = sizeof(SQL_INTERVAL_STRUCT);
        }
      }
      break;
    }
    case SQL_C_INTERVAL_MONTH: {
      SQL_INTERVAL_STRUCT* interval = (SQL_INTERVAL_STRUCT*)value;
      interval->interval_type = SQL_IS_MONTH;
      const int32_t month = outputRow.getInt(columnNum);
      if (month >= 0) {
        interval->intval.year_month.month = month;
        interval->interval_sign = SQL_FALSE;
      } else {
        interval->intval.year_month.month = -month;
        interval->interval_sign = SQL_TRUE;
      }
      interval->intval.year_month.year = 0;
      if (lenOrIndp) {
        if (month == 0 && outputRow.isNull(columnNum)) {
          *lenOrIndp = SQL_NULL_DATA;
        } else {
          *lenOrIndp = sizeof(SQL_INTERVAL_STRUCT);
        }
      }
      break;
    }
    case SQL_C_INTERVAL_DAY: {
      SQL_INTERVAL_STRUCT* interval = (SQL_INTERVAL_STRUCT*)value;
      ::memset(&interval->intval.day_second, 0, sizeof(SQL_DAY_SECOND_STRUCT));
      interval->interval_type = SQL_IS_DAY;
      const int32_t day = outputRow.getInt(columnNum);
      if (day >= 0) {
        interval->intval.day_second.day = day;
        interval->interval_sign = SQL_FALSE;
      } else {
        interval->intval.day_second.day = -day;
        interval->interval_sign = SQL_TRUE;
      }
      if (lenOrIndp) {
        if (day == 0 && outputRow.isNull(columnNum)) {
          *lenOrIndp = SQL_NULL_DATA;
        } else {
          *lenOrIndp = sizeof(SQL_INTERVAL_STRUCT);
        }
      }
      break;
    }
    case SQL_C_INTERVAL_HOUR: {
      SQL_INTERVAL_STRUCT* interval = (SQL_INTERVAL_STRUCT*)value;
      ::memset(&interval->intval.day_second, 0, sizeof(SQL_DAY_SECOND_STRUCT));
      interval->interval_type = SQL_IS_HOUR;
      const int32_t hour = outputRow.getInt(columnNum);
      if (hour >= 0) {
        interval->intval.day_second.hour = hour;
        interval->interval_sign = SQL_FALSE;
      } else {
        interval->intval.day_second.hour = -hour;
        interval->interval_sign = SQL_TRUE;
      }
      if (lenOrIndp) {
        if (hour == 0 && outputRow.isNull(columnNum)) {
          *lenOrIndp = SQL_NULL_DATA;
        } else {
          *lenOrIndp = sizeof(SQL_INTERVAL_STRUCT);
        }
      }
      break;
    }
    case SQL_C_INTERVAL_MINUTE: {
      SQL_INTERVAL_STRUCT* interval = (SQL_INTERVAL_STRUCT*)value;
      ::memset(&interval->intval.day_second, 0, sizeof(SQL_DAY_SECOND_STRUCT));
      interval->interval_type = SQL_IS_MINUTE;
      const int32_t minute = outputRow.getInt(columnNum);
      if (minute >= 0) {
        interval->intval.day_second.minute = minute;
        interval->interval_sign = SQL_FALSE;
      } else {
        interval->intval.day_second.minute = -minute;
        interval->interval_sign = SQL_TRUE;
      }
      if (lenOrIndp) {
        if (minute == 0 && outputRow.isNull(columnNum)) {
          *lenOrIndp = SQL_NULL_DATA;
        } else {
          *lenOrIndp = sizeof(SQL_INTERVAL_STRUCT);
        }
      }
      break;
    }
    case SQL_C_INTERVAL_SECOND: {
      SQL_INTERVAL_STRUCT* interval = (SQL_INTERVAL_STRUCT*)value;
      ::memset(&interval->intval.day_second, 0, sizeof(SQL_DAY_SECOND_STRUCT));
      interval->interval_type = SQL_IS_SECOND;
      const int32_t second = outputRow.getInt(columnNum);
      if (second >= 0) {
        interval->intval.day_second.second = second;
        interval->interval_sign = SQL_FALSE;
      } else {
        interval->intval.day_second.second = -second;
        interval->interval_sign = SQL_TRUE;
      }
      if (lenOrIndp) {
        if (second == 0 && outputRow.isNull(columnNum)) {
          *lenOrIndp = SQL_NULL_DATA;
        } else {
          *lenOrIndp = sizeof(SQL_INTERVAL_STRUCT);
        }
      }
      break;
    }
    case SQL_C_INTERVAL_YEAR_TO_MONTH: {
      SQL_INTERVAL_STRUCT* interval = (SQL_INTERVAL_STRUCT*)value;
      interval->interval_type = SQL_IS_YEAR_TO_MONTH;
      const int32_t month = outputRow.getInt(columnNum);
      SQLUINTEGER months;
      if (month >= 0) {
        months = month;
        interval->interval_sign = SQL_FALSE;
      } else {
        months = -month;
        interval->interval_sign = SQL_TRUE;
      }
      interval->intval.year_month.year = months / 12;
      interval->intval.year_month.month = months % 12;
      if (lenOrIndp) {
        if (month == 0 && outputRow.isNull(columnNum)) {
          *lenOrIndp = SQL_NULL_DATA;
        } else {
          *lenOrIndp = sizeof(SQL_INTERVAL_STRUCT);
        }
      }
      break;
    }
    case SQL_C_INTERVAL_DAY_TO_HOUR: {
      SQL_INTERVAL_STRUCT* interval = (SQL_INTERVAL_STRUCT*)value;
      ::memset(&interval->intval.day_second, 0, sizeof(SQL_DAY_SECOND_STRUCT));
      interval->interval_type = SQL_IS_DAY_TO_HOUR;
      const int32_t hour = outputRow.getInt(columnNum);
      uint32_t hours;
      if (hour >= 0) {
        hours = static_cast<uint32_t>(hour);
        interval->interval_sign = SQL_FALSE;
      } else {
        hours = static_cast<uint32_t>(-hour);
        interval->interval_sign = SQL_TRUE;
      }
      interval->intval.day_second.day = hours / 24;
      interval->intval.day_second.hour = hours % 24;
      if (lenOrIndp) {
        if (hour == 0 && outputRow.isNull(columnNum)) {
          *lenOrIndp = SQL_NULL_DATA;
        } else {
          *lenOrIndp = sizeof(SQL_INTERVAL_STRUCT);
        }
      }
      break;
    }
    case SQL_C_INTERVAL_DAY_TO_MINUTE: {
      SQL_INTERVAL_STRUCT* interval = (SQL_INTERVAL_STRUCT*)value;
      ::memset(&interval->intval.day_second, 0, sizeof(SQL_DAY_SECOND_STRUCT));
      interval->interval_type = SQL_IS_DAY_TO_MINUTE;
      const int64_t minute = outputRow.getInt64(columnNum);
      uint64_t minutes;
      if (minute >= 0) {
        minutes = static_cast<uint64_t>(minute);
        interval->interval_sign = SQL_FALSE;
      } else {
        minutes = static_cast<uint64_t>(-minute);
        interval->interval_sign = SQL_TRUE;
      }
      const auto hours = minutes / 60;
      interval->intval.day_second.day = static_cast<SQLUINTEGER>(hours / 24);
      interval->intval.day_second.hour = static_cast<SQLUINTEGER>(hours % 24);
      interval->intval.day_second.minute =
        static_cast<SQLUINTEGER>(minutes % 60);
      if (lenOrIndp) {
        if (minute == 0 && outputRow.isNull(columnNum)) {
          *lenOrIndp = SQL_NULL_DATA;
        } else {
          *lenOrIndp = sizeof(SQL_INTERVAL_STRUCT);
        }
      }
      break;
    }
    case SQL_C_INTERVAL_DAY_TO_SECOND: {
      SQL_INTERVAL_STRUCT* interval = (SQL_INTERVAL_STRUCT*)value;
      ::memset(&interval->intval.day_second, 0, sizeof(SQL_DAY_SECOND_STRUCT));
      interval->interval_type = SQL_IS_DAY_TO_SECOND;
      const int64_t second = outputRow.getInt64(columnNum);
      uint64_t seconds;
      if (second >= 0) {
        seconds = static_cast<uint64_t>(second);
        interval->interval_sign = SQL_FALSE;
      } else {
        seconds = static_cast<uint64_t>(-second);
        interval->interval_sign = SQL_TRUE;
      }
      const auto minutes = seconds / 60;
      const auto hours = minutes / 60;
      interval->intval.day_second.day = static_cast<SQLUINTEGER>(hours / 24);
      interval->intval.day_second.hour = static_cast<SQLUINTEGER>(hours % 24);
      interval->intval.day_second.minute =
          static_cast<SQLUINTEGER>(minutes % 60);
      interval->intval.day_second.second =
        static_cast<SQLUINTEGER>(seconds % 60);
      if (lenOrIndp) {
        if (second == 0 && outputRow.isNull(columnNum)) {
          *lenOrIndp = SQL_NULL_DATA;
        } else {
          *lenOrIndp = sizeof(SQL_INTERVAL_STRUCT);
        }
      }
      break;
    }
    case SQL_C_INTERVAL_HOUR_TO_MINUTE: {
      SQL_INTERVAL_STRUCT* interval = (SQL_INTERVAL_STRUCT*)value;
      ::memset(&interval->intval.day_second, 0, sizeof(SQL_DAY_SECOND_STRUCT));
      interval->interval_type = SQL_IS_HOUR_TO_MINUTE;
      const int64_t minute = outputRow.getInt64(columnNum);
      uint64_t minutes;
      if (minute >= 0) {
        minutes = static_cast<uint64_t>(minute);
        interval->interval_sign = SQL_FALSE;
      } else {
        minutes = static_cast<uint64_t>(-minute);
        interval->interval_sign = SQL_TRUE;
      }
      interval->intval.day_second.hour =
          static_cast<SQLUINTEGER>(minutes / 60);
      interval->intval.day_second.minute =
          static_cast<SQLUINTEGER>(minutes % 60);
      if (lenOrIndp) {
        if (minute == 0 && outputRow.isNull(columnNum)) {
          *lenOrIndp = SQL_NULL_DATA;
        } else {
          *lenOrIndp = sizeof(SQL_INTERVAL_STRUCT);
        }
      }
      break;
    }
    case SQL_C_INTERVAL_HOUR_TO_SECOND: {
      SQL_INTERVAL_STRUCT* interval = (SQL_INTERVAL_STRUCT*)value;
      ::memset(&interval->intval.day_second, 0, sizeof(SQL_DAY_SECOND_STRUCT));
      interval->interval_type = SQL_IS_HOUR_TO_SECOND;
      const int64_t second = outputRow.getInt64(columnNum);
      uint64_t seconds;
      if (second >= 0) {
        seconds = static_cast<uint64_t>(second);
        interval->interval_sign = SQL_FALSE;
      } else {
        seconds = static_cast<uint64_t>(-second);
        interval->interval_sign = SQL_TRUE;
      }
      const auto minutes = seconds / 60;
      interval->intval.day_second.hour =
          static_cast<SQLUINTEGER>(minutes / 60);
      interval->intval.day_second.minute =
          static_cast<SQLUINTEGER>(minutes % 60);
      interval->intval.day_second.second =
          static_cast<SQLUINTEGER>(seconds % 60);
      if (lenOrIndp) {
        if (second == 0 && outputRow.isNull(columnNum)) {
          *lenOrIndp = SQL_NULL_DATA;
        } else {
          *lenOrIndp = sizeof(SQL_INTERVAL_STRUCT);
        }
      }
      break;
    }
    case SQL_C_INTERVAL_MINUTE_TO_SECOND: {
      SQL_INTERVAL_STRUCT* interval = (SQL_INTERVAL_STRUCT*)value;
      ::memset(&interval->intval.day_second, 0, sizeof(SQL_DAY_SECOND_STRUCT));
      interval->interval_type = SQL_IS_MINUTE_TO_SECOND;
      const int64_t second = outputRow.getInt64(columnNum);
      uint64_t seconds;
      if (second >= 0) {
        seconds = static_cast<uint64_t>(second);
        interval->interval_sign = SQL_FALSE;
      } else {
        seconds = static_cast<uint64_t>(-second);
        interval->interval_sign = SQL_TRUE;
      }
      interval->intval.day_second.minute =
          static_cast<SQLUINTEGER>(seconds / 60);
      interval->intval.day_second.second =
          static_cast<SQLUINTEGER>(seconds % 60);
      if (lenOrIndp) {
        if (second == 0 && outputRow.isNull(columnNum)) {
          *lenOrIndp = SQL_NULL_DATA;
        } else {
          *lenOrIndp = sizeof(SQL_INTERVAL_STRUCT);
        }
      }
      break;
    }
    case SQL_C_GUID: {
      auto str = outputRow.getString(columnNum, precision);
      if (str) {
        SQLGUID* guid = (SQLGUID*)value;
        // convert from string representation to GUID
        unsigned int c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11;
#ifdef _WINDOWS
        ::sscanf_s(str->c_str(), s_GUID_FORMAT, &c1, &c2, &c3, &c4, &c5, &c6,
          &c7, &c8, &c9, &c10, &c11);
#else
        ::sscanf(str->c_str(), s_GUID_FORMAT, &c1, &c2, &c3, &c4, &c5, &c6,
            &c7, &c8, &c9, &c10, &c11);
#endif
        guid->Data1 = c1;
        guid->Data2 = c2;
        guid->Data3 = c3;
        guid->Data4[0] = c4;
        guid->Data4[1] = c5;
        guid->Data4[2] = c6;
        guid->Data4[3] = c7;
        guid->Data4[4] = c8;
        guid->Data4[5] = c9;
        guid->Data4[6] = c10;
        guid->Data4[7] = c11;

        if (lenOrIndp) {
          *lenOrIndp = sizeof(SQLGUID);
        }
      } else {
        ::memset(value, 0, sizeof(SQLGUID));
        if (lenOrIndp) {
          *lenOrIndp = SQL_NULL_DATA;
        }
      }
      break;
    }
    default:
      setException(
          GET_SQLEXCEPTION2(SQLStateMessage::INVALID_CTYPE_MSG, ctype,
              columnNum + 1));
      res = SQL_ERROR;
      break;
  }

  if (res == SQL_SUCCESS_WITH_INFO
      && (ctype == SQL_CHAR || ctype == SQL_WCHAR)) {
    setException(
        GET_SQLEXCEPTION2(SQLStateMessage::STRING_TRUNCATED_MSG,
            "output column", valueSize));
  }
  return res;
}

SQLRETURN SnappyStatement::fillOutParameters(const Result& result) {
  const auto& outputParams = result.getOutputParameters();
  if (outputParams.empty()) {
    return SQL_SUCCESS;
  }
  SQLRETURN retVal = SQL_SUCCESS;
  // transfer to a row in order to ease processing in fillOutput
  Row outParams(outputParams.size());
  uint32_t outParamIndex = 0;
  for (const auto& param : m_params) {
    const short inoutType = param.m_inputOutputType;
    if (inoutType == SQL_PARAM_OUTPUT || inoutType == SQL_PARAM_INPUT_OUTPUT) {
      auto result = outputParams.find(static_cast<int32_t>(param.m_paramNum));
      if (result != outputParams.end()) {
        // std::move is safe here since result.getOutputParameters is called
        // exactly once from here and never used after this
        outParams.addColumn(std::move(result->second));
        retVal = fillOutput(outParams, ++outParamIndex, param.m_o_value,
            param.m_o_valueSize, param.m_o_valueType, param.m_o_precision,
            param.m_o_lenOrIndp);
      }
      if (retVal != SQL_SUCCESS) {
        break;
      }
    }
  }
  return retVal;
}

SQLRETURN SnappyStatement::fillOutputFields() {
  SQLRETURN result = SQL_SUCCESS, result2 = SQL_SUCCESS;

  const Row* currentRow = m_cursor.get();
  if (currentRow) {
    // now bind the output fields
    uint32_t columnNum = 1;
    for (const auto &outputField : m_outputFields) {
      auto targetValue = outputField.m_targetValue;
      if (targetValue) {
        result2 = fillOutput(*currentRow, columnNum, targetValue,
            outputField.m_valueSize, outputField.m_targetType,
            DEFAULT_REAL_PRECISION, outputField.m_lenOrIndPtr);
      }
      if (result2 != SQL_SUCCESS) result = result2;
      ++columnNum;
    }
    return result;
  } else {
    setException(
        GET_SQLEXCEPTION2(SQLStateMessage::INVALID_CURSOR_STATE_MSG2));
    return SQL_ERROR;
  }
}

#include <iostream>

SQLRETURN SnappyStatement::fillOutputFieldsWithArrays() {
  Row* currentRow;
  SQLRETURN result = SQL_SUCCESS;
  SQLULEN bindOffset = 0;
  int rowsFetched = 0;
  // int rowsIgnored = 0;
  if (m_bindOffsetPtr) {
    bindOffset = *m_bindOffsetPtr;
  }

  do {
    currentRow = m_cursor.get();
    int32_t position = m_bulkCursor.position();
    if (m_bindingOrientation == SQL_BIND_BY_COLUMN) {
      uint32_t columnNum = 0;
      for (const auto& outputField : m_outputFields) {
        ++columnNum;
        SQLPOINTER targetValue = outputField.m_targetValue;
        if (!targetValue) continue;
        const SQLLEN valueSize = outputField.m_valueSize;
        targetValue = (((char*)targetValue)
            + (position * valueSize) + bindOffset);
        SQLLEN* lenOrIndPtr = (SQLLEN*)((char*)(outputField.m_lenOrIndPtr
            + position) + bindOffset);

        result = fillOutput(*currentRow, columnNum, targetValue, valueSize,
            outputField.m_targetType, DEFAULT_REAL_PRECISION, lenOrIndPtr);

        if (result == SQL_ERROR) {
          break;
        }
      }
    } else {/*ROW_WISE_BINDING*/
      const auto structSize = m_bindingOrientation;
      uint32_t columnNum = 0;
      for (const auto& outputField : m_outputFields) {
        ++columnNum;
        SQLPOINTER targetValue = outputField.m_targetValue;
        if (!targetValue) continue;
        targetValue = (((char*)targetValue)
            + (position * structSize) + bindOffset);
        SQLLEN* lenOrIndPtr = (SQLLEN*)(((char*)outputField.m_lenOrIndPtr)
            + ((position * structSize) + bindOffset));
        result = fillOutput(*currentRow, columnNum, targetValue,
            outputField.m_valueSize, outputField.m_targetType,
            DEFAULT_REAL_PRECISION, lenOrIndPtr);
        if (result == SQL_ERROR) {
          break;
        }
      }
    }
    if (result == SQL_ERROR) {
      break;
    }
    rowsFetched++;
    if (m_rowStatusPtr) {
      m_rowStatusPtr[position] = SQL_ROW_SUCCESS;
    }
  } while (m_bulkCursor.next());
  if (m_fetchedRowsPtr) {
    *m_fetchedRowsPtr = rowsFetched;
  }
  return result == SQL_SUCCESS && rowsFetched == 0 ? SQL_NO_DATA : result;
}

void SnappyStatement::setRowStatus() {
  if (m_resultSet) {
    if (m_fetchedRowsPtr) {
      // this case should always be for single row fetch case
      *m_fetchedRowsPtr = 1;
    }
    if (m_rowStatusPtr) {
      if (m_cursor.rowDeleted()) {
        *m_rowStatusPtr = SQL_ROW_DELETED;
      } else if (m_cursor.rowUpdated()) {
        *m_rowStatusPtr = SQL_ROW_UPDATED;
      } else if (m_cursor.rowInserted()) {
        *m_rowStatusPtr = SQL_ROW_ADDED;
      } else {
        *m_rowStatusPtr = SQL_ROW_SUCCESS;
      }
    }
  }
}

SQLType SnappyStatement::convertTypeToSQLType(
    const SQLSMALLINT odbcType, const uint32_t paramNum) {
  switch (odbcType) {
    case SQL_CHAR:
      return SQLType::CHAR;
    case SQL_VARCHAR:
      return SQLType::VARCHAR;
    case SQL_LONGVARCHAR:
      return SQLType::LONGVARCHAR;
      // GemXD support wide-character types as non-wide UTF-8 encoded strings
    case SQL_WCHAR:
      return SQLType::CHAR;
    case SQL_WVARCHAR:
      return SQLType::VARCHAR;
    case SQL_WLONGVARCHAR:
      return SQLType::LONGVARCHAR;
    case SQL_DECIMAL:
    case SQL_NUMERIC:
      return SQLType::DECIMAL;
    case SQL_SMALLINT:
      return SQLType::SMALLINT;
    case SQL_INTEGER:
      return SQLType::INTEGER;
    case SQL_REAL:
      return SQLType::FLOAT;
    case SQL_DOUBLE:
    case SQL_FLOAT:
      return SQLType::DOUBLE;
    case SQL_BIT:
      return SQLType::BOOLEAN;
    case SQL_TINYINT:
      return SQLType::TINYINT;
    case SQL_BIGINT:
      return SQLType::BIGINT;
    case SQL_BINARY:
      return SQLType::BINARY;
    case SQL_VARBINARY:
      return SQLType::VARBINARY;
    case SQL_LONGVARBINARY:
      return SQLType::LONGVARBINARY;
    case SQL_TYPE_DATE:
    case SQL_DATE:
      return SQLType::DATE;
    case SQL_TYPE_TIME:
    case SQL_TIME:
      return SQLType::TIME;
    case SQL_TYPE_TIMESTAMP:
    case SQL_TIMESTAMP:
      return SQLType::TIMESTAMP;
      // TODO: there are no interval data types in JDBC or SnappyData 
      // below mapped according to closest precision data type but will it work?
    case SQL_INTERVAL_MONTH:
      return SQLType::INTEGER;
    case SQL_INTERVAL_YEAR:
      return SQLType::INTEGER;
    case SQL_INTERVAL_YEAR_TO_MONTH:
      return SQLType::DATE;
    case SQL_INTERVAL_DAY:
      return SQLType::INTEGER;
    case SQL_INTERVAL_HOUR:
      return SQLType::INTEGER;
    case SQL_INTERVAL_MINUTE:
      return SQLType::INTEGER;
    case SQL_INTERVAL_SECOND:
      return SQLType::INTEGER;
    case SQL_INTERVAL_DAY_TO_HOUR:
      return SQLType::TIMESTAMP;
    case SQL_INTERVAL_DAY_TO_MINUTE:
      return SQLType::TIMESTAMP;
    case SQL_INTERVAL_DAY_TO_SECOND:
      return SQLType::TIMESTAMP;
    case SQL_INTERVAL_HOUR_TO_MINUTE:
      return SQLType::TIME;
    case SQL_INTERVAL_HOUR_TO_SECOND:
      return SQLType::TIME;
    case SQL_INTERVAL_MINUTE_TO_SECOND:
      return SQLType::TIME;
    case SQL_GUID:
      return SQLType::VARCHAR;
    default:
      throw GET_SQLEXCEPTION2(SQLStateMessage::INVALID_PARAMETER_TYPE_MSG,
          odbcType, paramNum);
  }
}

SQLSMALLINT SnappyStatement::convertTypeToCType(const SQLSMALLINT odbcType,
    const uint32_t paramNum) {
  switch (odbcType) {
    case SQL_CHAR:
      return SQL_C_CHAR;
    case SQL_VARCHAR:
      return SQL_C_CHAR;
    case SQL_LONGVARCHAR:
      return SQL_C_CHAR;
    case SQL_WCHAR:
      return SQL_C_WCHAR;
    case SQL_WVARCHAR:
      return SQL_C_WCHAR;
    case SQL_WLONGVARCHAR:
      return SQL_C_WCHAR;
    case SQL_DECIMAL:
      return SQL_C_NUMERIC;
    case SQL_NUMERIC:
      return SQL_C_NUMERIC;
    case SQL_SMALLINT:
      return SQL_C_SSHORT;
    case SQL_INTEGER:
      return SQL_C_SLONG;
    case SQL_REAL:
      return SQL_C_FLOAT;
    case SQL_FLOAT:
    case SQL_DOUBLE:
      return SQL_C_DOUBLE;
    case SQL_BIT:
      return SQL_C_BIT;
    case SQL_TINYINT:
      return SQL_C_TINYINT;
    case SQL_BIGINT:
      return SQL_C_SBIGINT;
    case SQL_BINARY:
      return SQL_C_BINARY;
    case SQL_VARBINARY:
      return SQL_C_BINARY;
    case SQL_LONGVARBINARY:
      return SQL_C_BINARY;
    case SQL_DATE:
    case SQL_TYPE_DATE:
      return SQL_C_TYPE_DATE;
    case SQL_TIME:
    case SQL_TYPE_TIME:
      return SQL_C_TYPE_TIME;
    case SQL_TIMESTAMP:
    case SQL_TYPE_TIMESTAMP:
      return SQL_C_TYPE_TIMESTAMP;
    case SQL_INTERVAL_MONTH:
      return SQL_C_INTERVAL_MONTH;
    case SQL_INTERVAL_YEAR:
      return SQL_C_INTERVAL_YEAR;
    case SQL_INTERVAL_YEAR_TO_MONTH:
      return SQL_C_INTERVAL_YEAR_TO_MONTH;
    case SQL_INTERVAL_DAY:
      return SQL_C_INTERVAL_DAY;
    case SQL_INTERVAL_HOUR:
      return SQL_C_INTERVAL_HOUR;
    case SQL_INTERVAL_MINUTE:
      return SQL_C_INTERVAL_MINUTE;
    case SQL_INTERVAL_SECOND:
      return SQL_C_INTERVAL_SECOND;
    case SQL_INTERVAL_DAY_TO_HOUR:
      return SQL_C_INTERVAL_DAY_TO_HOUR;
    case SQL_INTERVAL_DAY_TO_MINUTE:
      return SQL_C_INTERVAL_DAY_TO_MINUTE;
    case SQL_INTERVAL_DAY_TO_SECOND:
      return SQL_C_INTERVAL_DAY_TO_SECOND;
    case SQL_INTERVAL_HOUR_TO_MINUTE:
      return SQL_C_INTERVAL_HOUR_TO_MINUTE;
    case SQL_INTERVAL_HOUR_TO_SECOND:
      return SQL_C_INTERVAL_HOUR_TO_SECOND;
    case SQL_INTERVAL_MINUTE_TO_SECOND:
      return SQL_C_INTERVAL_MINUTE_TO_SECOND;
    case SQL_GUID:
      return SQL_C_GUID;
    default:
      throw GET_SQLEXCEPTION2(SQLStateMessage::INVALID_PARAMETER_TYPE_MSG,
          odbcType, paramNum);
  }
}

SQLSMALLINT SnappyStatement::convertSQLTypeToCType(const SQLType sqlType) {
  switch (sqlType) {
    case SQLType::BIGINT:
      return SQL_C_SBIGINT;
    case SQLType::BINARY:
    case SQLType::BLOB:
    case SQLType::LONGVARBINARY:
    case SQLType::VARBINARY:
      return SQL_C_BINARY;
    case SQLType::BOOLEAN:
      return SQL_C_BIT;
    case SQLType::CHAR:
    case SQLType::CLOB:
    case SQLType::LONGVARCHAR:
    case SQLType::VARCHAR:
      return SQL_C_CHAR;
    case SQLType::DATE:
      return SQL_C_TYPE_DATE;
    case SQLType::DECIMAL:
      return SQL_C_NUMERIC;
    case SQLType::DOUBLE:
      return SQL_C_DOUBLE;
    case SQLType::FLOAT:
      return SQL_C_FLOAT;
    case SQLType::INTEGER:
      return SQL_C_LONG;
    case SQLType::SMALLINT:
      return SQL_C_SHORT;
    case SQLType::TIME:
      return SQL_C_TYPE_TIME;
    case SQLType::TIMESTAMP:
      return SQL_C_TYPE_TIMESTAMP;
    case SQLType::TINYINT:
      return SQL_C_TINYINT;
    default:
      return SQL_C_CHAR;
  }
}

SQLSMALLINT SnappyStatement::convertSQLTypeToType(const SQLType sqlType,
    const bool isASCII) {
  switch (sqlType) {
    case SQLType::BIGINT:
      return SQL_BIGINT;
    case SQLType::BINARY:
      return SQL_BINARY;
    case SQLType::BLOB:
    case SQLType::LONGVARBINARY:
      return SQL_LONGVARBINARY;
    case SQLType::VARBINARY:
      return SQL_VARBINARY;
    case SQLType::BOOLEAN:
      return SQL_BIT;
    case SQLType::CHAR:
      return isASCII ? SQL_CHAR : SQL_WCHAR;
    case SQLType::CLOB:
    case SQLType::LONGVARCHAR:
      return isASCII ? SQL_LONGVARCHAR : SQL_WLONGVARCHAR;
    case SQLType::VARCHAR:
      return isASCII ? SQL_VARCHAR : SQL_WVARCHAR;
    case SQLType::DATE:
      return SQL_TYPE_DATE;
    case SQLType::DECIMAL:
      return SQL_DECIMAL;
    case SQLType::DOUBLE:
      return SQL_DOUBLE;
    case SQLType::FLOAT:
      return SQL_REAL;
    case SQLType::INTEGER:
      return SQL_INTEGER;
    case SQLType::SMALLINT:
      return SQL_SMALLINT;
    case SQLType::TIME:
      return SQL_TYPE_TIME;
    case SQLType::TIMESTAMP:
      return SQL_TYPE_TIMESTAMP;
    case SQLType::TINYINT:
      return SQL_TINYINT;
    default:
      return SQL_UNKNOWN_TYPE;
  }
}

SQLSMALLINT SnappyStatement::convertNullability(
    const ColumnNullability nullability) {
  switch (nullability) {
    case ColumnNullability::NONULLS:
      return SQL_NO_NULLS;
    case ColumnNullability::NULLABLE:
      return SQL_NULLABLE;
    default:
      return SQL_NULLABLE_UNKNOWN;
  }
}

// Check for the conversions supported
bool SnappyStatement::checkSupportedCtypeToSQLTypeConversion(SQLSMALLINT cType,
    SQLSMALLINT sqlType) {
  bool result = true;
  switch (cType) {
    case SQL_C_SSHORT:
    case SQL_C_SHORT:
    case SQL_C_USHORT:
    case SQL_C_SLONG:
    case SQL_C_ULONG:
    case SQL_C_LONG:
    case SQL_C_BIT:
    case SQL_C_UTINYINT:
    case SQL_C_TINYINT:
    case SQL_C_STINYINT:
    case SQL_C_SBIGINT:
    case SQL_C_UBIGINT:
    case SQL_C_NUMERIC:
      switch (sqlType) {
        case SQL_BINARY:
        case SQL_VARBINARY:
        case SQL_LONGVARBINARY:
        case SQL_DATE:
        case SQL_TIME:
        case SQL_TIMESTAMP:
        case SQL_TYPE_DATE:
        case SQL_TYPE_TIME:
        case SQL_TYPE_TIMESTAMP:
        case SQL_GUID:
          result = false;
          break;
        default:
          break;
      }
      break;
    case SQL_C_FLOAT:
    case SQL_C_DOUBLE:
      if (sqlType == SQL_BINARY || sqlType == SQL_VARBINARY
          || sqlType == SQL_LONGVARBINARY || sqlType == SQL_TYPE_DATE
          || sqlType == SQL_TYPE_TIME || sqlType == SQL_TYPE_TIMESTAMP
          || sqlType == SQL_GUID) {
        result = false;
      }
      break;
    case SQL_C_DATE:
    case SQL_C_TYPE_DATE: {
      if (!(sqlType == SQL_CHAR || sqlType == SQL_VARCHAR
          || sqlType == SQL_LONGVARCHAR || sqlType == SQL_WCHAR
          || sqlType == SQL_WVARCHAR || sqlType == SQL_WLONGVARCHAR
          || sqlType == SQL_TYPE_DATE || sqlType == SQL_TYPE_TIMESTAMP)) {
        result = false;
      }
      break;
    }
    case SQL_C_TIME:
    case SQL_C_TYPE_TIME: {
      if (!(sqlType == SQL_CHAR || sqlType == SQL_VARCHAR
          || sqlType == SQL_LONGVARCHAR || sqlType == SQL_WCHAR
          || sqlType == SQL_WVARCHAR || sqlType == SQL_WLONGVARCHAR
          || sqlType == SQL_TYPE_TIME)) {
        result = false;
      }
      break;
    }
    case SQL_C_TIMESTAMP:
    case SQL_C_TYPE_TIMESTAMP: {
      if (!(sqlType == SQL_CHAR || sqlType == SQL_VARCHAR
          || sqlType == SQL_LONGVARCHAR || sqlType == SQL_WCHAR
          || sqlType == SQL_WVARCHAR || sqlType == SQL_WLONGVARCHAR
          || sqlType == SQL_TYPE_DATE || sqlType == SQL_TYPE_TIME
          || sqlType == SQL_TYPE_TIMESTAMP)) {
        result = false;
      }
      break;
    }
    case SQL_C_GUID: {
      if (sqlType != SQL_GUID) {
        result = false;
      }
      break;
    }
  }
  return result;
}

SQLRETURN SnappyStatement::addParameter(SQLUSMALLINT paramNum,
    SQLSMALLINT inputOutputType, SQLSMALLINT valueType, SQLSMALLINT paramType,
    SQLULEN precision, SQLSMALLINT scale, SQLPOINTER paramValue,
    SQLLEN valueSize, SQLLEN* lenOrIndPtr) {
  const size_t sz = m_params.size();
  if (sz == 0) {
    m_params.reserve(std::max<SQLUSMALLINT>(4, paramNum));
  }
  if (sz < paramNum) {
    m_params.resize(paramNum);
  }
  if (paramValue && lenOrIndPtr && *lenOrIndPtr == SQL_NULL_DATA) {
    paramValue = nullptr;
  }
  m_params[paramNum - 1].set(paramNum, inputOutputType, valueType, paramType,
      StringFunctions::restrictLength<SQLUINTEGER, SQLULEN>(precision), scale,
      paramValue, valueSize, lenOrIndPtr);
  if (checkSupportedCtypeToSQLTypeConversion(valueType, paramType)) {
    return SQL_SUCCESS;
  } else {
    setException(GET_SQLEXCEPTION2(
        SQLStateMessage::TYPE_ATTRIBUTE_VIOLATION_MSG,
        valueType, paramType));
    return SQL_ERROR;
  }
}

SQLRETURN SnappyStatement::prepare(const std::string& sqlText) {
  clearLastError();
  try {
    // need to prepare the statement and bind the parameters

    // clear any old parameters
    m_params.clear();
    m_execParams.clear();
    m_pstmt = m_conn.m_conn.prepareStatement(sqlText, EMPTY_OUTPUT_PARAMS,
        m_stmtAttrs);

    return handleWarnings(m_pstmt.get());
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
}

static inline SQLSMALLINT getRowStatus(const int8_t type) noexcept {
  switch (type) {
    case thrift::snappydataConstants::STATEMENT_TYPE_INSERT:
      return SQL_ROW_ADDED;
    case thrift::snappydataConstants::STATEMENT_TYPE_UPDATE:
      return SQL_ROW_UPDATED;
    case thrift::snappydataConstants::STATEMENT_TYPE_DELETE:
      return SQL_ROW_DELETED;
    default:
      return SQL_ROW_SUCCESS;
  }
}

SQLRETURN SnappyStatement::executeWithArrayOfParams(
    const std::string& sqlText) {
  try {
    if (!isPrepared()) {
      // clear any old parameters
      m_params.clear();
      m_execParams.clear();
      m_pstmt = m_conn.m_conn.prepareStatement(sqlText, EMPTY_OUTPUT_PARAMS,
          m_stmtAttrs);
    }

    ParametersBatch paramsBatch(*m_pstmt);
    paramsBatch.reserve(m_paramSetSize);
    SQLRETURN result = bindArrayOfParameters(paramsBatch, m_paramSetSize,
        m_paramBindOffsetPtr, m_paramBindingOrientation, m_paramStatusArr,
        m_paramsProcessedPtr);
    const auto updateCounts(std::move(m_pstmt->executeBatch(paramsBatch)));
    if (m_rowStatusPtr) {
      const SQLSMALLINT rowStatus = getRowStatus(
          m_pstmt->getStatementType());
      for (uint32_t i = 0; i < m_paramSetSize; i++) {
        m_rowStatusPtr[i] = updateCounts.at(i) > 0 ? rowStatus : SQL_ROW_NOROW;
      }
    }
    return result;
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
}

SQLRETURN SnappyStatement::execute(const std::string& sqlText) {
  clearLastError();
  SQLRETURN result = SQL_SUCCESS;
  if (!m_resultSet) {
    try {
      m_result.reset();
      if (m_paramSetSize > 1) {
        return executeWithArrayOfParams(sqlText);
      }

      const size_t numParams = m_params.size();
      if (numParams > 0) {
        std::map<int32_t, OutputParameter> outParams;
        result = bindParameters(&outParams);
        if (result != SQL_SUCCESS && result != SQL_SUCCESS_WITH_INFO) {
          return result;
        }
        // need to prepare too, so use prepareAndExecute
        m_result = m_conn.m_conn.prepareAndExecute(sqlText, m_execParams,
            outParams, m_stmtAttrs);
        m_pstmt = m_result->getPreparedStatement();
        m_execParams.clear();
      } else {
        m_result = m_conn.m_conn.execute(sqlText, EMPTY_OUTPUT_PARAMS,
            m_stmtAttrs);
        m_pstmt.reset();
      }

      auto rs = m_result->getResultSet();
      setResultSet(rs);
      if (numParams > 0) {
        fillOutParameters(*m_result);
      }

      return handleWarnings(m_result.get());
    } catch (SQLException& sqle) {
      setException(sqle);
      return SQL_ERROR;
    } catch (std::exception& se) {
      setException(__FILE__, __LINE__, se);
      return SQL_ERROR;
    }
  } else {
    // old cursor still open
    setException(GET_SQLEXCEPTION2(SQLStateMessage::INVALID_CURSOR_STATE_MSG1));
    return SQL_ERROR;
  }
}

SQLRETURN SnappyStatement::execute() {
  clearLastError();
  SQLRETURN result = SQL_SUCCESS;
  if (!m_resultSet) {
    try {
      m_result.reset();
      if (!isPrepared()) {
        // should be handled by DriverManager
        setException(
            GET_SQLEXCEPTION2(SQLStateMessage::STATEMENT_NOT_PREPARED_MSG));
        return SQL_ERROR;
      }
      if (m_paramSetSize > 1) {
        // isPrepared() is true here so no use of passing sqlText
        return executeWithArrayOfParams("");
      }

      result = bindParameters(nullptr);
      if (result != SQL_SUCCESS && result != SQL_SUCCESS_WITH_INFO) {
        return result;
      }
      m_result = m_pstmt->execute(m_execParams);
      m_execParams.clear();
      auto rs = m_result->getResultSet();
      setResultSet(rs);
      fillOutParameters(*m_result);

      return handleWarnings(m_result.get());
    } catch (SQLException& sqle) {
      setException(sqle);
      return SQL_ERROR;
    } catch (std::exception& se) {
      setException(__FILE__, __LINE__, se);
      return SQL_ERROR;
    }
  } else {
    // old cursor still open
    setException(GET_SQLEXCEPTION2(SQLStateMessage::INVALID_CURSOR_STATE_MSG1));
    return SQL_ERROR;
  }
}

SQLRETURN SnappyStatement::bulkOperations(SQLUSMALLINT operation) {
  if (operation == SQL_ADD) {
    try {
      std::string batchQueryString("INSERT INTO ");
      if (!m_resultSet) {
        if (!m_pstmt) {
          setException(GET_SQLEXCEPTION2(
              SQLStateMessage::STATEMENT_NOT_PREPARED_MSG));
          return SQL_ERROR;
        } else {
          batchQueryString.append(m_pstmt->getColumnDescriptor(1).getTable());
        }
      } else {
        batchQueryString.append(m_resultSet->getColumnDescriptor(1).getTable());
      }
      batchQueryString.append(" VALUES (");
      // TODO: if bind m_targetValue is null then need to bind by column names
      // skipping such fields? (check SQLBulkOperations documentation)
      SQLUSMALLINT columnNum = 1;
      for (const auto &outputField : m_outputFields) {
        if (columnNum == 1) {
          batchQueryString.push_back('?');
        } else {
          batchQueryString.append(",?");
        }
        SQLType sqlType = (!m_resultSet
            ? m_pstmt->getColumnDescriptor(columnNum).getSQLType()
            : m_resultSet->getColumnDescriptor(columnNum).getSQLType());
        addParameter(columnNum, SQL_PARAM_INPUT, outputField.m_targetType,
            convertSQLTypeToType(sqlType), 0, 0, outputField.m_targetValue,
            outputField.m_valueSize, nullptr /* null-terminated strings */);
        ++columnNum;
      }
      batchQueryString.push_back(')');
      // need to prepare the statement and bind the parameters
      auto batchStmt = m_conn.m_conn.prepareStatement(
          batchQueryString, EMPTY_OUTPUT_PARAMS, m_stmtAttrs);
      ParametersBatch paramsBatch(*batchStmt);
      SQLULEN paramSetSize = (SQLULEN)m_bulkCursor.batchSize();
      paramsBatch.reserve(paramSetSize);
      SQLRETURN result = bindArrayOfParameters(paramsBatch, paramSetSize,
          m_bindOffsetPtr, m_bindingOrientation, m_rowStatusPtr, nullptr);
      const auto updateCounts(std::move(batchStmt->executeBatch(paramsBatch)));
      if (m_rowStatusPtr) {
        for (uint32_t i = 0; i < paramSetSize; i++) {
          m_rowStatusPtr[i] =
              updateCounts.at(i) > 0 ? SQL_ROW_ADDED : SQL_ROW_NOROW;
        }
      }
      return result;
    } catch (SQLException& sqle) {
      setException(sqle);
      return SQL_ERROR;
    } catch (std::exception& se) {
      setException(__FILE__, __LINE__, se);
      return SQL_ERROR;
    }
  } else {
    // not supported operation
    std::ostringstream ostr;
    ostr << "BulkOperations for operation=" << operation;
    setException(GET_SQLEXCEPTION2(
        SQLStateMessage::FEATURE_NOT_IMPLEMENTED_MSG1, ostr.str().c_str()));
    return SQL_ERROR;
  }
}

SQLRETURN SnappyStatement::bindOutputField(SQLUSMALLINT columnNum,
    SQLSMALLINT targetType, SQLPOINTER targetValue, SQLLEN valueSize,
    SQLLEN *lenOrIndPtr) {
  clearLastError();
  uint32_t numColumns;
  if (m_resultSet) {
    numColumns = m_resultSet->getColumnCount();
  /* (doesn't work for routed queries in current snappy master)
  } else if (isPrepared()) {
    numColumns = m_pstmt->getColumnCount();
  */
  } else {
    numColumns = std::numeric_limits<uint32_t>::max();
  }
  if (columnNum > 0 && columnNum <= numColumns) {
    if (m_outputFields.size() < columnNum) {
      m_outputFields.resize(columnNum);
    }
    if (targetValue && lenOrIndPtr && *lenOrIndPtr == SQL_NULL_DATA) {
      targetValue = nullptr;
    }
    m_outputFields[columnNum - 1].set(targetType, targetValue, valueSize,
        lenOrIndPtr);
    return SQL_SUCCESS;
  } else {
    setException(
        GET_SQLEXCEPTION2(SQLStateMessage::COLUMN_NOT_FOUND_MSG1, columnNum,
            static_cast<int>(numColumns)));
    return SQL_ERROR;
  }
}

SQLRETURN SnappyStatement::setPos(SQLSETPOSIROW rowNum,
    SQLUSMALLINT operation, SQLUSMALLINT lockType) {
  clearLastError();
  // TODO: need to verify other lock types
  if (lockType != SQL_LOCK_NO_CHANGE || rowNum < 0 ||
      rowNum >= std::numeric_limits<int32_t>::max()) {
    // not supported lock type
    if (lockType != SQL_LOCK_NO_CHANGE) {
      std::ostringstream ostr;
      ostr << "setPos with lock type = " << lockType;
      setException(GET_SQLEXCEPTION2(
          SQLStateMessage::NOT_IMPLEMENTED_MSG, ostr.str().c_str()));
    } else {
      setException(GET_SQLEXCEPTION2(
          SQLStateMessage::INVALID_ATTRIBUTE_VALUE_MSG, rowNum, "ROW NUMBER"));
    }
    return SQL_ERROR;
  }
  const int32_t rowNumber = static_cast<int32_t>(rowNum);
  try {
    if (m_resultSet) {
      SQLRETURN result = SQL_SUCCESS, result2;
      switch (operation) {
        case SQL_POSITION:
          // position the cursor to the row number
          m_cursor = m_resultSet->begin(rowNumber - 1);
          setRowStatus();
          break;
        case SQL_REFRESH:
          m_cursor.clearInsertRow();
          if (rowNumber > 0) {
            // position the cursor to the row number and refresh the row
            m_cursor = m_resultSet->begin(rowNumber - 1);
            setRowStatus();
          } else {
            for (m_cursor = m_resultSet->begin();
                m_cursor != m_resultSet->end(); ++m_cursor) {
              setRowStatus();
            }
          }
          break;
        case SQL_UPDATE:
          if (rowNumber > 0) {
            // position the cursor to the row number and update the row
            m_cursor = m_resultSet->begin(rowNumber - 1);
            m_cursor.updateRow();
            setRowStatus();
          } else {
            for (m_cursor = m_resultSet->begin();
                m_cursor != m_resultSet->end(); ++m_cursor) {
              m_cursor.updateRow();
              setRowStatus();
            }
          }
          break;
        case SQL_DELETE:
          if (rowNumber > 0) {
            // position the cursor to the row number and refresh the row
            m_cursor = m_resultSet->begin(rowNumber - 1);
            m_cursor.deleteRow();
            setRowStatus();
          } else {
            for (m_cursor = m_resultSet->begin();
                m_cursor != m_resultSet->end(); ++m_cursor) {
              m_cursor.deleteRow();
              setRowStatus();
            }
          }
          break;
        case SQL_ADD:
          bulkOperations(SQL_ADD);
          break;
        default:
          // unknown operation
          setException(GET_SQLEXCEPTION2(
              SQLStateMessage::INVALID_ATTRIBUTE_VALUE_MSG, operation,
              "operation type in setPos"));
          return SQL_ERROR;
      }
      if (m_bulkCursor.batchSize() <= 1) {
        result = fillOutputFields();
      } else {
        m_bulkCursor.initNextBatch();
        result = fillOutputFieldsWithArrays();
      }
      result2 = handleWarnings(m_resultSet.get());
      return result == SQL_SUCCESS ? result2 : result;
    } else {
      // no open cursor
      setException(GET_SQLEXCEPTION2(
          SQLStateMessage::INVALID_CURSOR_STATE_MSG2));
      return SQL_ERROR;
    }
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
  return SQL_NO_DATA;
}

SQLRETURN SnappyStatement::fetchScroll(SQLSMALLINT fetchOrientation,
    SQLLEN offset) {
  clearLastError();

  int32_t fetchOffset = StringFunctions::restrictLength<int32_t, SQLLEN>(
      offset);
  try {
    if (m_resultSet) {
      SQLRETURN result = SQL_SUCCESS, result2;
      bool bRetVal = false;
      switch (fetchOrientation) {
        case SQL_FETCH_NEXT:
          bRetVal = m_cursor.next();
          break;
        case SQL_FETCH_PRIOR:
          bRetVal = m_cursor.previous();
          break;
        case SQL_FETCH_RELATIVE:
          m_cursor += fetchOffset;
          bRetVal = m_cursor.isOnRow();
          break;
        case SQL_FETCH_ABSOLUTE: {
          const bool beforeFirst = fetchOffset == 0;
          if (fetchOffset > 0) {
            fetchOffset--;
          }
          m_cursor = m_resultSet->begin(fetchOffset);
          if (beforeFirst) {
            bRetVal = m_cursor.previous();
          } else {
            bRetVal = m_cursor.isOnRow();
          }
          break;
        }
        case SQL_FETCH_FIRST:
          m_cursor = m_resultSet->begin();
          bRetVal = m_cursor.isOnRow();
          break;
        case SQL_FETCH_LAST:
          m_cursor = m_resultSet->begin(-1);
          bRetVal = m_cursor.isOnRow();
          break;
        case SQL_FETCH_BOOKMARK:
        default:
          // not supported fetch orientation
          setException(
              GET_SQLEXCEPTION2(SQLStateMessage::FEATURE_NOT_IMPLEMENTED_MSG1,
                  "SQL_FETCH_BOOKMARK in fetchScroll"));
          return SQL_ERROR;
      }
      if (bRetVal) {
        if (m_bulkCursor.batchSize() <= 1) {
          setRowStatus();
          // now bind the output fields
          result = fillOutputFields();
        } else {
          // TODO: SW: honour fetch direction
          m_bulkCursor.initNextBatch();
          result = fillOutputFieldsWithArrays();
        }
      } else {
        result = SQL_NO_DATA;
      }
      result2 = handleWarnings(m_resultSet.get());
      return result == SQL_SUCCESS ? result2 : result;
    } else {
      // no open cursor
      setException(
          GET_SQLEXCEPTION2(SQLStateMessage::INVALID_CURSOR_STATE_MSG2));
      return SQL_ERROR;
    }
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
  return SQL_NO_DATA;
}

SQLRETURN SnappyStatement::next() {
  clearLastError();
  try {
    if (!m_resultSet) {
      // no open cursor
      setException(
        GET_SQLEXCEPTION2(SQLStateMessage::INVALID_CURSOR_STATE_MSG2));
      return SQL_ERROR;
    } else if (m_cursor.next()) {
      SQLRETURN result = SQL_SUCCESS, r;
      if (m_bulkCursor.batchSize() <= 1) {
        setRowStatus();
        // now bind the output fields
        result = fillOutputFields();
      } else {
        m_bulkCursor.initNextBatch();
        result = fillOutputFieldsWithArrays();
      }
      r = handleWarnings(m_resultSet.get());
      return result == SQL_SUCCESS ? r : result;
    }
  } catch (SQLException& sqle) {
    if (sqle.getReason() != SQLStateMessage::NO_CURRENT_ROW_MSG.format()) {
      setException(sqle);
      return SQL_ERROR;
    }
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
  return SQL_NO_DATA;
}

SQLRETURN SnappyStatement::getUpdateCount(SQLLEN *count, bool updateError) {
  if (updateError) clearLastError();
  // TODO: also handle for UPDATE/DELETE in SQLSetPos
  if (m_result) {
    if (count) *count = m_result->getUpdateCount();
    return SQL_SUCCESS;
  } else {
    if (count) *count = -1;
    if (updateError) {
      setException(
          GET_SQLEXCEPTION2(SQLStateMessage::FUNCTION_SEQUENCE_ERROR_MSG,
              "no result or statement not executed"));
    }
    return SQL_ERROR;
  }
}

SQLRETURN SnappyStatement::getData(SQLUSMALLINT columnNum,
    SQLSMALLINT targetType, SQLPOINTER targetValue, SQLLEN valueSize,
    SQLLEN *lenOrIndPtr) {
  clearLastError();
  try {
    if (m_cursor.isOnRow()) {
      if (targetValue) {
        const Row* currentRow = m_cursor.get();
        const SQLRETURN result = fillOutput(*currentRow, columnNum,
            targetValue, valueSize, targetType,
            DEFAULT_REAL_PRECISION, lenOrIndPtr);
        const SQLRETURN ret = handleWarnings(m_resultSet.get());
        return result == SQL_SUCCESS ? ret : result;
      }
    } else {
      // no open cursor
      setException(
          GET_SQLEXCEPTION2(SQLStateMessage::INVALID_CURSOR_STATE_MSG2));
      return SQL_ERROR;
    }
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
  return SQL_NO_DATA;
}

template<typename CHAR_TYPE>
SQLRETURN SnappyStatement::getAttributeT(SQLINTEGER attribute,
    SQLPOINTER valueBuffer, SQLINTEGER bufferLen, SQLINTEGER* valueLen) {
  clearLastError();
  SQLRETURN ret = SQL_SUCCESS;
  SQLULEN result;
  try {
    switch (attribute) {
      case SQL_ATTR_APP_ROW_DESC:
        if (valueBuffer) *(SQLPOINTER*)valueBuffer = m_ardDesc.get();
        if (valueLen) *valueLen = sizeof(SnappyDescriptor);
        break;

      case SQL_ATTR_IMP_ROW_DESC:
        if (valueBuffer) *(SQLPOINTER*)valueBuffer = m_irdDesc.get();
        if (valueLen) *valueLen = sizeof(SnappyDescriptor);
        break;

      case SQL_ATTR_APP_PARAM_DESC:
        if (valueBuffer) *(SQLPOINTER*)valueBuffer = m_apdDesc.get();
        if (valueLen) *valueLen = sizeof(SnappyDescriptor);
        break;

      case SQL_ATTR_IMP_PARAM_DESC:
        if (valueBuffer) *(SQLPOINTER*)valueBuffer = m_ipdDesc.get();
        if (valueLen) *valueLen = sizeof(SnappyDescriptor);
        break;

      case SQL_ATTR_CONCURRENCY:
        result = m_stmtAttrs.isUpdatable() ? SQL_CONCUR_LOCK
            : SQL_CONCUR_READ_ONLY;
        getIntValue(result, valueBuffer, valueLen, true);
        break;

      case SQL_ATTR_CURSOR_SCROLLABLE:
        switch (m_stmtAttrs.getResultSetType()) {
          case ResultSetType::FORWARD_ONLY:
            result = SQL_NONSCROLLABLE;
            break;
          case ResultSetType::INSENSITIVE:
          case ResultSetType::SENSITIVE:
            result = SQL_SCROLLABLE;
            break;
          default:
            result = SQL_NONSCROLLABLE;
            break;
        }
        getIntValue(result, valueBuffer, valueLen, true);
        break;

      case SQL_ATTR_CURSOR_SENSITIVITY:
        switch (m_stmtAttrs.getResultSetType()) {
          case ResultSetType::FORWARD_ONLY:
          case ResultSetType::SENSITIVE:
            result = SQL_SENSITIVE;
            break;
          case ResultSetType::INSENSITIVE:
            result = SQL_INSENSITIVE;
            break;
          default:
            result = SQL_UNSPECIFIED;
        }
        getIntValue(result, valueBuffer, valueLen, true);
        break;

      case SQL_ATTR_CURSOR_TYPE:
        switch (m_stmtAttrs.getResultSetType()) {
          case ResultSetType::FORWARD_ONLY:
            result = SQL_CURSOR_FORWARD_ONLY;
            break;
          case ResultSetType::INSENSITIVE:
            result = SQL_CURSOR_STATIC;
            break;
          case ResultSetType::SENSITIVE:
            result = SQL_CURSOR_DYNAMIC;
            break;
          default:
            result = SQL_CURSOR_FORWARD_ONLY;
            break;
        }
        getIntValue(result, valueBuffer, valueLen, true);
        break;

      case SQL_ATTR_ENABLE_AUTO_IPD:
        getIntValue(SQL_FALSE, valueBuffer, valueLen, true);
        break;

      case SQL_ATTR_FETCH_BOOKMARK_PTR:
        getIntValue(m_bookmark, valueBuffer, valueLen, false);
        break;

      case SQL_ATTR_MAX_LENGTH:
        getIntValue(m_stmtAttrs.getMaxFieldSize(), valueBuffer,
            valueLen, true);
        break;

      case SQL_ATTR_MAX_ROWS:
        getIntValue(m_stmtAttrs.getMaxRows(), valueBuffer, valueLen, true);
        break;

      case SQL_ATTR_ROW_ARRAY_SIZE:
        getIntValue(m_bulkCursor.batchSize(), valueBuffer, valueLen, true);
        break;

      case SQL_ATTR_ROW_BIND_TYPE:
        getIntValue(m_bindingOrientation, valueBuffer, valueLen, true);
        break;

      case SQL_ATTR_ROW_STATUS_PTR:
        if (valueBuffer) *(SQLPOINTER*)valueBuffer = m_rowStatusPtr;
        if (valueLen) *valueLen = sizeof(m_rowStatusPtr);
        break;

      case SQL_ATTR_ROWS_FETCHED_PTR:
        if (valueBuffer) *(SQLPOINTER*)valueBuffer = m_fetchedRowsPtr;
        if (valueLen) *valueLen = sizeof(m_fetchedRowsPtr);
        break;

      case SQL_ATTR_ROW_BIND_OFFSET_PTR:
        if (valueBuffer) *(SQLPOINTER*)valueBuffer = m_bindOffsetPtr;
        if (valueLen) *valueLen = sizeof(m_bindOffsetPtr);
        break;

      case SQL_ATTR_METADATA_ID:
        getIntValue(m_argsAsIdentifiers ? SQL_TRUE : SQL_FALSE, valueBuffer,
            valueLen, true);
        break;

      case SQL_ATTR_NOSCAN:
        getIntValue(m_stmtAttrs.hasEscapeProcessing() ? SQL_NOSCAN_OFF
            : SQL_NOSCAN_ON, valueBuffer, valueLen, true);
        break;

      case SQL_ATTR_PARAM_BIND_TYPE:
        getIntValue(m_paramBindingOrientation, valueBuffer, valueLen, true);
        break;

      case SQL_ATTR_PARAMSET_SIZE:
        getIntValue(m_paramSetSize, valueBuffer, valueLen, true);
        break;

      case SQL_ATTR_PARAM_STATUS_PTR:
        if (valueBuffer) *(SQLPOINTER*)valueBuffer = m_paramStatusArr;
        if (valueLen) *valueLen = sizeof(m_paramStatusArr);
        break;

      case SQL_ATTR_PARAMS_PROCESSED_PTR:
        if (valueBuffer) {
          *(SQLPOINTER*)valueBuffer = m_paramsProcessedPtr;
        }
        if (valueLen) *valueLen = sizeof(m_paramsProcessedPtr);
        break;

      case SQL_ATTR_QUERY_TIMEOUT:
        getIntValue(m_stmtAttrs.getTimeout(), valueBuffer, valueLen, true);
        break;

      case SQL_ATTR_KEYSET_SIZE:
        getIntValue(m_KeySetSize, valueBuffer, valueLen, true);
        break;

      case SQL_ATTR_PARAM_BIND_OFFSET_PTR:
        if (valueBuffer) {
          *(SQLPOINTER*)valueBuffer = m_paramBindOffsetPtr;
        }
        if (valueLen) *valueLen = sizeof(m_paramBindOffsetPtr);
        break;

      case SQL_ATTR_PARAM_OPERATION_PTR:
        if (valueBuffer) {
          *(SQLPOINTER*)valueBuffer = m_paramOperationPtr;
        }
        if (valueLen) *valueLen = sizeof(m_paramOperationPtr);
        break;

      case SQL_ATTR_ROW_OPERATION_PTR:
        if (valueBuffer) *(SQLPOINTER*)valueBuffer = m_rowOperationPtr;
        if (valueLen) *valueLen = sizeof(m_rowOperationPtr);
        break;

      // TODO: Need to implement below attribs
      case SQL_ATTR_ASYNC_ENABLE:
        getIntValue(SQL_ASYNC_ENABLE_OFF, valueBuffer, valueLen, true);
        break;

      case SQL_ATTR_RETRIEVE_DATA:
        getIntValue(SQL_RD_ON, valueBuffer, valueLen, true);
        break;

      default:
        std::ostringstream ostr;
        ostr << "getAttribute for " << attribute;
        setException(
            GET_SQLEXCEPTION2(SQLStateMessage::FEATURE_NOT_IMPLEMENTED_MSG1,
                ostr.str().c_str()));
        ret = SQL_ERROR;
        break;
    }
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
  return ret;
}

template<typename CHAR_TYPE>
SQLRETURN SnappyStatement::setAttributeT(SQLINTEGER attribute,
    SQLPOINTER valueBuffer, SQLINTEGER valueLen) {
  SQLRETURN ret = SQL_SUCCESS;
  clearLastError();
  try {
    switch (attribute) {
      case SQL_ATTR_CONCURRENCY:
        if (isUnprepared()) {
          const SQLULEN intValue = (SQLULEN)valueBuffer;
          switch (intValue) {
            case SQL_CONCUR_READ_ONLY:
              m_stmtAttrs.setUpdatable(false);
              break;
            case SQL_CONCUR_LOCK:
              // mapping CONCUR_LOCK to JDBC CONCUR_UPDATABLE
              m_stmtAttrs.setUpdatable(true);
              break;
            case SQL_CONCUR_ROWVER:
            case SQL_CONCUR_VALUES:
              // mapping these to JDBC CONCUR_UPDATABLE with info
              m_stmtAttrs.setUpdatable(true);
              setException(
                  GET_SQLEXCEPTION2(SQLStateMessage::OPTION_VALUE_CHANGED_MSG,
                      "SQL_ATTR_CONCURRENCY", intValue, SQL_CONCUR_LOCK));
              ret = SQL_SUCCESS_WITH_INFO;
              break;
            default:
              setException(GET_SQLEXCEPTION2(
                  SQLStateMessage::INVALID_ATTRIBUTE_VALUE_MSG,
                  intValue, "SQL_ATTR_CONCURRENCY"));
              ret = SQL_ERROR;
              break;
          }
        } else {
          setException(
              GET_SQLEXCEPTION2(
                  SQLStateMessage::OPTION_CANNOT_BE_SET_FOR_STATEMENT_MSG,
                  "SQL_ATTR_CONCURRENCY"));
          ret = SQL_ERROR;
        }
        break;

      case SQL_ATTR_CURSOR_SCROLLABLE:
        if (isUnprepared()) {
          const SQLULEN intValue = (SQLULEN)valueBuffer;
          switch (intValue) {
            case SQL_NONSCROLLABLE:
              m_stmtAttrs.setResultSetType(ResultSetType::FORWARD_ONLY);
              break;
            case SQL_SCROLLABLE:
              if (m_stmtAttrs.getResultSetType()
                  == ResultSetType::FORWARD_ONLY) {
                m_stmtAttrs.setResultSetType(ResultSetType::INSENSITIVE);
              }
              break;
            default:
              setException(GET_SQLEXCEPTION2(
                  SQLStateMessage::INVALID_ATTRIBUTE_VALUE_MSG,
                  intValue, "SQL_ATTR_CURSOR_SCROLLABLE"));
              ret = SQL_ERROR;
              break;
          }
        } else {
          setException(
              GET_SQLEXCEPTION2(
                  SQLStateMessage::OPTION_CANNOT_BE_SET_FOR_STATEMENT_MSG,
                  "SQL_ATTR_CURSOR_SCROLLABLE"));
          ret = SQL_ERROR;
        }
        break;

      case SQL_ATTR_CURSOR_SENSITIVITY:
        if (isUnprepared()) {
          const SQLULEN intValue = (SQLULEN)valueBuffer;
          switch (intValue) {
            case SQL_UNSPECIFIED:
              // nothing to be done
              break;
            case SQL_INSENSITIVE:
              m_stmtAttrs.setResultSetType(ResultSetType::INSENSITIVE);
              break;
            case SQL_SENSITIVE:
              // forward-only cursors are already sensitive to changes
              if (m_stmtAttrs.getResultSetType()
                  != ResultSetType::FORWARD_ONLY) {
                m_stmtAttrs.setResultSetType(ResultSetType::SENSITIVE);
              }
              break;
            default:
              setException(GET_SQLEXCEPTION2(
                  SQLStateMessage::INVALID_ATTRIBUTE_VALUE_MSG,
                  intValue, "SQL_ATTR_CURSOR_SENSITIVITY"));
              ret = SQL_ERROR;
              break;
          }
        } else {
          setException(
              GET_SQLEXCEPTION2(
                  SQLStateMessage::OPTION_CANNOT_BE_SET_FOR_STATEMENT_MSG,
                  "SQL_ATTR_CURSOR_SENSITIVITY"));
          ret = SQL_ERROR;
        }
        break;

      case SQL_ATTR_CURSOR_TYPE:
        if (isUnprepared()) {
          const SQLULEN intValue = (SQLULEN)valueBuffer;
          switch (intValue) {
            case SQL_CURSOR_FORWARD_ONLY:
              m_stmtAttrs.setResultSetType(ResultSetType::FORWARD_ONLY);
              break;
            case SQL_CURSOR_STATIC:
              m_stmtAttrs.setResultSetType(ResultSetType::INSENSITIVE);
              break;
            case SQL_CURSOR_DYNAMIC:
              m_stmtAttrs.setResultSetType(ResultSetType::SENSITIVE);
              break;
            case SQL_CURSOR_KEYSET_DRIVEN:
              m_stmtAttrs.setResultSetType(ResultSetType::INSENSITIVE);
              setException(
                  GET_SQLEXCEPTION2(SQLStateMessage::OPTION_VALUE_CHANGED_MSG,
                      "SQL_ATTR_CURSOR_TYPE", intValue, SQL_CURSOR_STATIC));
              ret = SQL_SUCCESS_WITH_INFO;
              break;
            default:
              setException(GET_SQLEXCEPTION2(
                  SQLStateMessage::INVALID_ATTRIBUTE_VALUE_MSG,
                  intValue, "SQL_ATTR_CURSOR_TYPE"));
              ret = SQL_ERROR;
              break;
          }
        } else {
          setException(
              GET_SQLEXCEPTION2(
                  SQLStateMessage::OPTION_CANNOT_BE_SET_FOR_STATEMENT_MSG,
                  "SQL_ATTR_CURSOR_TYPE"));
          ret = SQL_ERROR;
        }
        break;

      case SQL_ATTR_ENABLE_AUTO_IPD: {
        // ResultSetMetadata is always available in SnappyData after prepare
        const SQLULEN intValue = (SQLULEN)valueBuffer;
        switch (intValue) {
          case SQL_TRUE:
          case SQL_FALSE:
            // nothing to be done
            break;
          default:
            setException(GET_SQLEXCEPTION2(
                SQLStateMessage::INVALID_ATTRIBUTE_VALUE_MSG,
                intValue, "SQL_ATTR_ENABLE_AUTO_IPD"));
            ret = SQL_ERROR;
            break;
        }
        break;
      }

      case SQL_ATTR_FETCH_BOOKMARK_PTR: {
        // our bookmark is just an offset of current row
        SQLLEN* bookmark = (SQLLEN*)valueBuffer;
        if (bookmark) {
          m_bookmark = (SQLINTEGER)*bookmark;
        } else {
          m_bookmark = -1;
        }
        break;
      }
      case SQL_ATTR_MAX_LENGTH:
        if (isPrepared()) {
          const auto intValue = (uint32_t)(SQLULEN)valueBuffer;
          if (!m_resultSet) {
            m_stmtAttrs.setMaxFieldSize(intValue);
          } else {
            setException(
                GET_SQLEXCEPTION2(SQLStateMessage::OPTION_VALUE_CHANGED_MSG,
                    "SQL_ATTR_MAX_LENGTH", intValue, -1));
            ret = SQL_SUCCESS_WITH_INFO;
          }
        } else {
          m_stmtAttrs.setMaxFieldSize((uint32_t)(SQLULEN)valueBuffer);
        }
        break;

      case SQL_ATTR_MAX_ROWS:
        if (isPrepared()) {
          const auto intValue = (uint32_t)(SQLULEN)valueBuffer;
          if (!m_resultSet) {
            m_stmtAttrs.setMaxRows(intValue);
          } else {
            setException(
                GET_SQLEXCEPTION2(SQLStateMessage::OPTION_VALUE_CHANGED_MSG,
                    "SQL_ATTR_MAX_ROWS", intValue, -1));
            ret = SQL_SUCCESS_WITH_INFO;
          }
        } else {
          m_stmtAttrs.setMaxRows((uint32_t)(SQLULEN)valueBuffer);
        }
        break;

      case SQL_ATTR_METADATA_ID:
        m_argsAsIdentifiers = ((SQLULEN)valueBuffer != SQL_FALSE);
        break;

      case SQL_ATTR_ROW_ARRAY_SIZE:
        m_bulkCursor.setBatchSize((uint32_t)(SQLULEN)valueBuffer);
        break;

      case SQL_ATTR_ROW_BIND_TYPE:
        m_bindingOrientation = (SQLULEN)valueBuffer;
        break;

      case SQL_ATTR_ROW_STATUS_PTR:
        m_rowStatusPtr = (SQLUSMALLINT*)valueBuffer;
        break;

      case SQL_ATTR_ROWS_FETCHED_PTR:
        m_fetchedRowsPtr = (SQLULEN*)valueBuffer;
        break;

      case SQL_ATTR_ROW_BIND_OFFSET_PTR:
        m_bindOffsetPtr = (SQLULEN*)valueBuffer;
        break;

      case SQL_ATTR_NOSCAN:
        m_stmtAttrs.setEscapeProcessing(
            (SQLULEN)valueBuffer == SQL_NOSCAN_OFF);
        break;

      case SQL_ATTR_PARAM_BIND_TYPE:
        m_paramBindingOrientation = (SQLULEN)valueBuffer;
        break;

      case SQL_ATTR_PARAMSET_SIZE:
        m_paramSetSize = (SQLULEN)valueBuffer;
        break;

      case SQL_ATTR_PARAM_STATUS_PTR:
        m_paramStatusArr = (SQLUSMALLINT*)valueBuffer;
        break;

      case SQL_ATTR_PARAMS_PROCESSED_PTR:
        m_paramsProcessedPtr = (SQLULEN*)valueBuffer;
        break;

      case SQL_ATTR_QUERY_TIMEOUT:
        m_stmtAttrs.setTimeout((uint32_t)(SQLULEN)valueBuffer);
        break;

      case SQL_ATTR_KEYSET_SIZE:
        m_KeySetSize = (SQLULEN)valueBuffer;
        break;

      case SQL_ATTR_PARAM_BIND_OFFSET_PTR:
        m_paramBindOffsetPtr = (SQLULEN*)valueBuffer;
        break;

      case SQL_ATTR_PARAM_OPERATION_PTR:
        m_paramOperationPtr = (SQLUSMALLINT*)valueBuffer;
        break;

      case SQL_ATTR_ROW_OPERATION_PTR:
        m_rowOperationPtr = (SQLUSMALLINT*)valueBuffer;
        break;

      // TODO: implement below
      case SQL_ATTR_APP_PARAM_DESC:
        setException(
            GET_SQLEXCEPTION2(SQLStateMessage::FEATURE_NOT_IMPLEMENTED_MSG1,
                "SQL_ATTR_APP_PARAM_DESC"));
        ret = SQL_ERROR;
        break;
      case SQL_ATTR_APP_ROW_DESC:
        setException(
            GET_SQLEXCEPTION2(SQLStateMessage::FEATURE_NOT_IMPLEMENTED_MSG1,
                "SQL_ATTR_APP_ROW_DESC"));
        ret = SQL_ERROR;
        break;
      case SQL_ATTR_ASYNC_ENABLE:
        setException(
            GET_SQLEXCEPTION2(SQLStateMessage::FEATURE_NOT_IMPLEMENTED_MSG1,
                "SQL_ATTR_ASYNC_ENABLE"));
        ret = SQL_ERROR;
        break;
      case SQL_ATTR_RETRIEVE_DATA:
        setException(
            GET_SQLEXCEPTION2(SQLStateMessage::FEATURE_NOT_IMPLEMENTED_MSG1,
                "SQL_ATTR_RETRIEVE_DATA"));
        ret = SQL_ERROR;
        break;

      default:
        setException(GET_SQLEXCEPTION2(
            SQLStateMessage::INVALID_ATTRIBUTE_VALUE_MSG,
            attribute, "SQLSetStmtAttr"));
        ret = SQL_ERROR;
        break;
    }
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
  return ret;
}

const ColumnDescriptor SnappyStatement::getColumnDescriptor(
    SQLUSMALLINT columnNumber, uint32_t* columnCount) const {
  if (m_resultSet) {
    if (columnCount) {
      *columnCount = m_resultSet->getColumnCount();
    }
    return m_resultSet->getColumnDescriptor(columnNumber);
  } else if (isPrepared()) {
    if (columnCount) {
      *columnCount = m_pstmt->getColumnCount();
    }
    return m_pstmt->getColumnDescriptor(columnNumber);
  } else {
    // no open cursor
    throw GET_SQLEXCEPTION2(SQLStateMessage::INVALID_CURSOR_STATE_MSG2);
  }
}

template<typename CHAR_TYPE>
SQLRETURN SnappyStatement::getResultColumnDescriptorT(
    SQLUSMALLINT columnNumber, CHAR_TYPE* columnName,
    SQLSMALLINT bufferLength, SQLSMALLINT* nameLength, SQLSMALLINT* dataType,
    SQLULEN* columnSize, SQLSMALLINT* decimalDigits, SQLSMALLINT* nullable) {
  clearLastError();
  // TODO: handle SQL_ATTR_USE_BOOKMARKS below for columnNumber == 0
  try {
    SQLRETURN result = SQL_SUCCESS;
    const ColumnDescriptor descriptor = getColumnDescriptor(columnNumber);

    if (columnName) {
      if (bufferLength >= 0) {
        const std::string& colName = descriptor.getName();
        const size_t sz = colName.size();
        SQLLEN nameLen;
        if (sz > 0) {
          ::memset(columnName, 0, bufferLength);
          if (StringFunctions::copyString((const SQLCHAR*)colName.c_str(), sz,
              columnName, bufferLength, &nameLen)) {
            result = SQL_SUCCESS_WITH_INFO;
            setException(
                GET_SQLEXCEPTION2(SQLStateMessage::STRING_TRUNCATED_MSG,
                    "Column Name", bufferLength - 1));
          }
          if (nameLength) {
            *nameLength = getLengthTruncatedToSmallInt(nameLen);
          }
        } else {
          *columnName = 0;
          if (nameLength) {
            *nameLength = 0;
          }
        }
      } else {
        result = SnappyHandleBase::errorInvalidBufferLength(bufferLength,
            "Column Name", this);
      }
    }
    if (dataType) {
      *dataType = convertSQLTypeToType(descriptor.getSQLType(),
          sizeof(CHAR_TYPE) == 1);
    }
    if (columnSize) {
      *columnSize = descriptor.getDisplaySize();
    }
    if (decimalDigits) {
      *decimalDigits = descriptor.getPrecision();
    }
    if (nullable) {
      *nullable = convertNullability(descriptor.getNullability());
    }
    return result;
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
}

SQLRETURN SnappyStatement::getColumnAttributeT(SQLUSMALLINT columnNumber,
    SQLUSMALLINT fieldId, SQLPOINTER charAttribute, SQLSMALLINT bufferLength,
    SQLSMALLINT* stringLength, SQLLEN* numericAttribute,
    const int sizeOfChar) {
  clearLastError();
  // TODO: handle SQL_ATTR_USE_BOOKMARKS below for columnNumber == 0
  try {
    SQLRETURN result = SQL_SUCCESS;
    uint32_t columnCount;
    const ColumnDescriptor descriptor = getColumnDescriptor(columnNumber,
        &columnCount);

    const char* descType = nullptr;
    std::string stringAttribute;
    const char* cstringAttribute = nullptr;
    switch (fieldId) {
      case SQL_DESC_AUTO_UNIQUE_VALUE:
        if (numericAttribute) {
          *numericAttribute =
              descriptor.isAutoIncrement() ? SQL_TRUE : SQL_FALSE;
        }
        break;
      case SQL_DESC_BASE_COLUMN_NAME:
        if (charAttribute) {
          descType = "Column Name";
          stringAttribute = descriptor.getName();
        }
        break;
      case SQL_DESC_NAME:
      case SQL_COLUMN_NAME:
        if (charAttribute) {
          descType = "Column Name or Alias";
          stringAttribute = descriptor.getName();
        }
        break;
      case SQL_DESC_BASE_TABLE_NAME:
        if (charAttribute) {
          descType = "Table Name";
          stringAttribute = descriptor.getTable();
        }
        break;
      case SQL_DESC_CASE_SENSITIVE:
        if (numericAttribute) {
          *numericAttribute =
              descriptor.isCaseSensitive() ? SQL_TRUE : SQL_FALSE;
        }
        break;
      case SQL_DESC_CATALOG_NAME:
        if (charAttribute) {
          descType = "Catalog Name";
          // always an empty name in SnappyData
        }
        break;
      case SQL_DESC_CONCISE_TYPE:
        if (numericAttribute) {
          *numericAttribute = convertSQLTypeToType(descriptor.getSQLType(),
              sizeOfChar == 1);
        }
        break;
      case SQL_DESC_COUNT:
      case SQL_COLUMN_COUNT:
        if (numericAttribute) {
          *numericAttribute = columnCount;
        }
        break;
      case SQL_DESC_DISPLAY_SIZE:
        if (numericAttribute) {
          *numericAttribute = descriptor.getDisplaySize();
        }
        break;
      case SQL_DESC_FIXED_PREC_SCALE:
        if (numericAttribute) {
          switch (descriptor.getSQLType()) {
            case SQLType::DECIMAL:
              *numericAttribute =
                  descriptor.getScale() > 0 ? SQL_TRUE : SQL_FALSE;
              break;
            default:
              *numericAttribute = SQL_FALSE;
              break;
          }
        }
        break;
      case SQL_DESC_LABEL:
        if (charAttribute) {
          descType = "Column Label";
          stringAttribute = descriptor.getLabel();
        }
        break;
      case SQL_DESC_LENGTH:
      case SQL_DESC_PRECISION:
      case SQL_COLUMN_LENGTH:
      case SQL_COLUMN_PRECISION:
        if (numericAttribute) {
          *numericAttribute = descriptor.getPrecision();
        }
        break;
      case SQL_DESC_LITERAL_PREFIX:
        descType = "Literal Prefix";
        // deliberate fall through
      case SQL_DESC_LITERAL_SUFFIX:
        if (charAttribute) {
          if (!descType) {
            descType = "Literal Suffix";
          }
          switch (descriptor.getSQLType()) {
            case SQLType::CHAR:
            case SQLType::VARCHAR:
            case SQLType::LONGVARCHAR:
            case SQLType::CLOB:
              cstringAttribute = "'";
              break;
            case SQLType::BINARY:
            case SQLType::VARBINARY:
            case SQLType::LONGVARBINARY:
              cstringAttribute =
                  (fieldId == SQL_DESC_LITERAL_PREFIX) ? "X'" : "'";
              break;
            case SQLType::DATE:
              cstringAttribute =
                  (fieldId == SQL_DESC_LITERAL_PREFIX) ? "DATE'" : "'";
              break;
            case SQLType::TIME:
              cstringAttribute =
                  (fieldId == SQL_DESC_LITERAL_PREFIX) ? "TIME'" : "'";
              break;
            case SQLType::TIMESTAMP:
              cstringAttribute =
                  (fieldId == SQL_DESC_LITERAL_PREFIX) ? "TIMESTAMP'" : "'";
              break;
            case SQLType::SQLXML:
              cstringAttribute =
                  (fieldId == SQL_DESC_LITERAL_PREFIX) ? "XMLPARSE (DOCUMENT '" :
                      "' PRESERVE WHITESPACE)";
              break;
            default:
              cstringAttribute = "";
              break;
          }
        }
        break;
      case SQL_DESC_LOCAL_TYPE_NAME:
        if (charAttribute) {
          descType = "Column Local Type Name";
          stringAttribute = descriptor.getTypeName();
        }
        break;
      case SQL_DESC_TYPE_NAME:
        if (charAttribute) {
          descType = "Column Type Name";
          stringAttribute = descriptor.getTypeName();
        }
        break;
      case SQL_DESC_NULLABLE:
      case SQL_COLUMN_NULLABLE:
        if (numericAttribute) {
          *numericAttribute = convertNullability(descriptor.getNullability());
        }
        break;
      case SQL_DESC_NUM_PREC_RADIX:
        if (numericAttribute) {
          switch (descriptor.getSQLType()) {
            case SQLType::BIGINT:
            case SQLType::BOOLEAN:
            case SQLType::DATE:
            case SQLType::DECIMAL:
            case SQLType::INTEGER:
            case SQLType::SMALLINT:
            case SQLType::TIME:
            case SQLType::TIMESTAMP:
            case SQLType::TINYINT:
              *numericAttribute = 10;
              break;
            case SQLType::DOUBLE:
            case SQLType::FLOAT:
              *numericAttribute = 2;
              break;
            default:
              *numericAttribute = 0;
              break;
          }
        }
        break;
      case SQL_DESC_OCTET_LENGTH:
        if (numericAttribute) {
          SQLLEN precision = descriptor.getPrecision();
          switch (descriptor.getSQLType()) {
            case SQLType::CHAR:
            case SQLType::VARCHAR:
            case SQLType::LONGVARCHAR:
            case SQLType::CLOB:
            case SQLType::SQLXML:
              if (precision <= (0x7fffffff >> 1)) {
                precision <<= 1;
              }
              break;
            default:
              break;
          }
          *numericAttribute = precision;
        }
        break;
      case SQL_DESC_SCALE:
      case SQL_COLUMN_SCALE:
        if (numericAttribute) {
          *numericAttribute = descriptor.getScale();
        }
        break;
      case SQL_DESC_SCHEMA_NAME:
        if (charAttribute) {
          descType = "Schema Name";
          stringAttribute = descriptor.getSchema();
        }
        break;
      case SQL_DESC_TABLE_NAME:
        if (charAttribute) {
          descType = "Table Name";
          stringAttribute = descriptor.getTable();
        }
        break;
      case SQL_DESC_TYPE: {
        if (numericAttribute) {
          const auto sqlType = descriptor.getSQLType();
          switch (sqlType) {
            case SQLType::DATE:
            case SQLType::TIME:
            case SQLType::TIMESTAMP:
              *numericAttribute = SQL_DATETIME;
              break;
            default:
              *numericAttribute = convertSQLTypeToType(sqlType,
                  sizeOfChar == 1);
              break;
          }
        }
        break;
      }
      case SQL_DESC_SEARCHABLE:
        if (numericAttribute) {
          switch (descriptor.getSQLType()) {
            case SQLType::CHAR:
            case SQLType::VARCHAR:
            case SQLType::LONGVARCHAR:
              *numericAttribute = SQL_PRED_SEARCHABLE;
              break;
            case SQLType::CLOB:
            case SQLType::BLOB:
            case SQLType::SQLXML:
              *numericAttribute = SQL_PRED_CHAR;
              break;
            case SQLType::JAVA_OBJECT:
              *numericAttribute = SQL_PRED_NONE;
              break;
            default:
              *numericAttribute = SQL_PRED_BASIC;
              break;
          }
        }
        break;
      case SQL_DESC_UNNAMED:
        if (numericAttribute) {
          const auto columnName = descriptor.getName();
          *numericAttribute = columnName.size() > 0 ? SQL_NAMED
              : SQL_UNNAMED;
        }
        break;
      case SQL_DESC_UNSIGNED:
        if (numericAttribute) {
          *numericAttribute = descriptor.isSigned() ? SQL_FALSE : SQL_TRUE;
        }
        break;
      case SQL_DESC_UPDATABLE:
        if (numericAttribute) {
          switch (descriptor.getUpdatable()) {
            case ColumnUpdatable::READ_ONLY:
              *numericAttribute = SQL_ATTR_READONLY;
              break;
            case ColumnUpdatable::UPDATABLE:
              *numericAttribute = SQL_DESC_UPDATABLE;
              break;
            case ColumnUpdatable::DEFINITELY_UPDATABLE:
              *numericAttribute = SQL_ATTR_WRITE;
              break;
            default:
              *numericAttribute = SQL_ATTR_READWRITE_UNKNOWN;
              break;
          }
        }
        break;
      default:
        setException(
            GET_SQLEXCEPTION2(
                SQLStateMessage::INVALID_DESCRIPTOR_FIELD_ID_MSG, fieldId));
        return SQL_ERROR;
    }
    const size_t ssize = stringAttribute.size();
    if (ssize > 0 || cstringAttribute) {
      bool truncated;
      SQLLEN totalLen = 0;
      SQLLEN* totalLenp = stringLength ? &totalLen : nullptr;
      if (ssize > 0) {
        if (sizeOfChar == 1) {
          truncated = StringFunctions::copyString(
              (const SQLCHAR*)stringAttribute.c_str(), ssize,
              (SQLCHAR*)charAttribute, bufferLength, totalLenp);
        } else {
          truncated = StringFunctions::copyString(
              (const SQLCHAR*)stringAttribute.c_str(), ssize,
              (SQLWCHAR*)charAttribute, bufferLength, totalLenp);
        }
      } else {
        if (sizeOfChar == 1) {
          truncated = StringFunctions::copyString(
              (const SQLCHAR*)cstringAttribute, SQL_NTS,
              (SQLCHAR*)charAttribute, bufferLength, totalLenp);
        } else {
          truncated = StringFunctions::copyString(
              (const SQLCHAR*)cstringAttribute, SQL_NTS,
              (SQLWCHAR*)charAttribute, bufferLength, totalLenp);
        }
      }

      if (stringLength) {
        *stringLength = getLengthTruncatedToSmallInt(totalLen * sizeOfChar);
      }
      if (truncated) {
        result = SQL_SUCCESS_WITH_INFO;
        setException(
            GET_SQLEXCEPTION2(SQLStateMessage::STRING_TRUNCATED_MSG, descType,
                bufferLength - 1));
      }
    }
    return result;
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
}

template<typename CHAR_TYPE>
SQLRETURN SnappyStatement::getCursorNameT(CHAR_TYPE* cursorName,
    SQLSMALLINT bufferLength, SQLSMALLINT* nameLength) {
  clearLastError();
  SQLRETURN result = SQL_SUCCESS;
  try {
    if (cursorName) {
      if (bufferLength >= 0) {
        std::string rcursorName =
            m_resultSet ? m_resultSet->getCursorName() : "";
        const auto rsize = rcursorName.size();
        if (rsize > 0) {
          SQLLEN nameLen;
          if (StringFunctions::copyString((const SQLCHAR*)rcursorName.c_str(),
              rsize, cursorName, bufferLength, &nameLen)) {
            result = SQL_SUCCESS_WITH_INFO;
            setException(
                GET_SQLEXCEPTION2(SQLStateMessage::STRING_TRUNCATED_MSG,
                    "Cursor Name", bufferLength - 1));
          }
          if (nameLength) {
            *nameLength = getLengthTruncatedToSmallInt(nameLen);
          }
        } else {
          *cursorName = 0;
          if (nameLength) {
            *nameLength = 0;
          }
        }
      } else {
        result = SnappyHandleBase::errorInvalidBufferLength(bufferLength,
            "Cursor Name", this);
      }
    }
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
  return result;
}

template<typename CHAR_TYPE>
SQLRETURN SnappyStatement::setCursorNameT(CHAR_TYPE* cursorName,
    SQLSMALLINT nameLength) {
  clearLastError();
  SQLRETURN result = SQL_SUCCESS;
  try {
    if (cursorName) {
      if (nameLength >= 0) {
        m_stmtAttrs.setCursorName(
            StringFunctions::toString(cursorName, nameLength));
      } else {
        result = SnappyHandleBase::errorInvalidBufferLength(nameLength,
            "Cursor Name", this);
      }
    } else {
      setException(GET_SQLEXCEPTION2(SQLStateMessage::NULL_CURSOR_MSG));
      return SQL_ERROR;
    }
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
  return result;
}

template<typename CHAR_TYPE>
SQLRETURN SnappyStatement::getTablesT(CHAR_TYPE* schemaName,
    SQLSMALLINT nameLength1, CHAR_TYPE* tableName, SQLSMALLINT nameLength2,
    CHAR_TYPE* tableTypes, SQLSMALLINT nameLength3) {
  clearLastError();
  if (m_resultSet) {
    // old cursor still open
    setException(
        GET_SQLEXCEPTION2(SQLStateMessage::INVALID_CURSOR_STATE_MSG1));
    return SQL_ERROR;
  }
  try {
    DatabaseMetaDataArgs args;

    m_params.clear();
    m_execParams.clear();

    if (schemaName) {
      args.setSchema(StringFunctions::toString(schemaName, nameLength1));
    }
    if (tableName) {
      args.setTable(StringFunctions::toString(tableName, nameLength2));
    }
    if (tableTypes) {
      std::vector<std::string> stableTypes;
      const int allTypesLen = sizeof(SQL_ALL_TABLE_TYPES) - 1;
      // check for "%" pattern for all types that corresponds to nullptr
      // in JDBC
      if ((nameLength3 == SQL_NTS || nameLength3 == allTypesLen)
          && StringFunctions::strncmp(SQL_ALL_TABLE_TYPES, tableTypes,
              allTypesLen) > 0) {
        // split the table types on "," then remove surrounding single quotes
        StringFunctions::split(tableTypes, nameLength3, ',', stableTypes);
        for (std::string& tableType : stableTypes) {
          size_t len;
          if (tableType[0] == '\'' &&
              tableType[(len = tableType.size()) - 1] == '\'') {
            tableType = tableType.substr(1, len - 1);
          }
        }
      }
      args.setTableTypes(stableTypes);
    }
    auto rs = m_conn.m_conn.getSchemaMetaData(
        DatabaseMetaDataCall::TABLES, args);
    setResultSet(rs);
    m_pstmt.reset();
    return handleWarnings(m_resultSet.get());
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
}

template<typename CHAR_TYPE>
SQLRETURN SnappyStatement::getTablePrivilegesT(CHAR_TYPE* schemaName,
    SQLSMALLINT nameLength1, CHAR_TYPE* tableName, SQLSMALLINT nameLength2) {
  clearLastError();
  if (m_resultSet) {
    // old cursor still open
    setException(
        GET_SQLEXCEPTION2(SQLStateMessage::INVALID_CURSOR_STATE_MSG1));
    return SQL_ERROR;
  }
  try {
    DatabaseMetaDataArgs args;

    m_params.clear();
    m_execParams.clear();

    if (schemaName) {
      args.setSchema(StringFunctions::toString(schemaName, nameLength1));
    }
    if (tableName) {
      args.setTable(StringFunctions::toString(tableName, nameLength2));
    }
    auto rs = m_conn.m_conn.getSchemaMetaData(
        DatabaseMetaDataCall::TABLEPRIVILEGES, args);
    setResultSet(rs);
    m_pstmt.reset();
    return handleWarnings(m_resultSet.get());
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
}

template<typename CHAR_TYPE>
SQLRETURN SnappyStatement::getColumnsT(CHAR_TYPE* schemaName,
    SQLSMALLINT nameLength1, CHAR_TYPE* tableName, SQLSMALLINT nameLength2,
    CHAR_TYPE* columnName, SQLSMALLINT nameLength3) {
  clearLastError();
  if (m_resultSet) {
    // old cursor still open
    setException(
        GET_SQLEXCEPTION2(SQLStateMessage::INVALID_CURSOR_STATE_MSG1));
    return SQL_ERROR;
  }
  try {
    DatabaseMetaDataArgs args;

    m_params.clear();
    m_execParams.clear();

    if (schemaName) {
      args.setSchema(StringFunctions::toString(schemaName, nameLength1));
    }
    if (tableName) {
      args.setTable(StringFunctions::toString(tableName, nameLength2));
    }
    if (columnName) {
      args.setColumnName(StringFunctions::toString(columnName, nameLength3));
    }
    auto rs = m_conn.m_conn.getSchemaMetaData(
        DatabaseMetaDataCall::COLUMNS, args);
    setResultSet(rs);
    m_pstmt.reset();
    return handleWarnings(m_resultSet.get());
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
}

template<typename CHAR_TYPE>
SQLRETURN SnappyStatement::getSpecialColumnsT(SQLUSMALLINT idType,
    CHAR_TYPE *schemaName, SQLSMALLINT nameLength1, CHAR_TYPE *tableName,
    SQLSMALLINT nameLength2, SQLUSMALLINT scope, SQLUSMALLINT nullable) {
  clearLastError();
  if (m_resultSet) {
    // old cursor still open
    setException(
        GET_SQLEXCEPTION2(SQLStateMessage::INVALID_CURSOR_STATE_MSG1));
    return SQL_ERROR;
  }
  try {
    DatabaseMetaDataArgs args;

    m_params.clear();
    m_execParams.clear();

    if (schemaName) {
      args.setSchema(StringFunctions::toString(schemaName, nameLength1));
    }
    if (tableName) {
      args.setTable(StringFunctions::toString(tableName, nameLength2));
    }
    if (idType == SQL_BEST_ROWID) {
      auto rs = m_conn.m_conn.getBestRowIdentifier(args, scope,
          nullable != SQL_NO_NULLS);
      setResultSet(rs);
    } else if (idType == SQL_ROWVER) {
      auto rs = m_conn.m_conn.getSchemaMetaData(
          DatabaseMetaDataCall::VERSIONCOLUMNS, args);
      setResultSet(rs);
    }
    m_pstmt.reset();
    return handleWarnings(m_resultSet.get());
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
}

template<typename CHAR_TYPE>
SQLRETURN SnappyStatement::getColumnPrivilegesT(CHAR_TYPE* schemaName,
    SQLSMALLINT nameLength1, CHAR_TYPE* tableName, SQLSMALLINT nameLength2,
    CHAR_TYPE* columnName, SQLSMALLINT nameLength3) {
  clearLastError();
  if (m_resultSet) {
    // old cursor still open
    setException(
        GET_SQLEXCEPTION2(SQLStateMessage::INVALID_CURSOR_STATE_MSG1));
    return SQL_ERROR;
  }
  try {
    DatabaseMetaDataArgs args;

    m_params.clear();
    m_execParams.clear();

    if (schemaName) {
      args.setSchema(StringFunctions::toString(schemaName, nameLength1));
    }
    if (tableName) {
      args.setTable(StringFunctions::toString(tableName, nameLength2));
    }
    if (columnName) {
      args.setColumnName(StringFunctions::toString(columnName, nameLength3));
    }
    auto rs = m_conn.m_conn.getSchemaMetaData(
        DatabaseMetaDataCall::COLUMNPRIVILEGES, args);
    setResultSet(rs);
    m_pstmt.reset();
    return handleWarnings(m_resultSet.get());
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
}

template<typename CHAR_TYPE>
SQLRETURN SnappyStatement::getIndexInfoT(CHAR_TYPE* schemaName,
    SQLSMALLINT nameLength1, CHAR_TYPE* tableName, SQLSMALLINT nameLength2,
    bool unique, bool approximate) {
  clearLastError();
  if (m_resultSet) {
    // old cursor still open
    setException(
        GET_SQLEXCEPTION2(SQLStateMessage::INVALID_CURSOR_STATE_MSG1));
    return SQL_ERROR;
  }
  try {
    DatabaseMetaDataArgs args;

    m_params.clear();
    m_execParams.clear();

    if (schemaName) {
      args.setSchema(StringFunctions::toString(schemaName, nameLength1));
    }
    if (tableName) {
      args.setTable(StringFunctions::toString(tableName, nameLength2));
    }
    auto rs = m_conn.m_conn.getIndexInfo(args, unique, approximate);
    setResultSet(rs);
    m_pstmt.reset();
    return handleWarnings(m_resultSet.get());
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
}

template<typename CHAR_TYPE>
SQLRETURN SnappyStatement::getPrimaryKeysT(CHAR_TYPE* schemaName,
    SQLSMALLINT nameLength1, CHAR_TYPE* tableName, SQLSMALLINT nameLength2) {
  clearLastError();
  if (m_resultSet) {
    // old cursor still open
    setException(
        GET_SQLEXCEPTION2(SQLStateMessage::INVALID_CURSOR_STATE_MSG1));
    return SQL_ERROR;
  }
  try {
    DatabaseMetaDataArgs args;

    m_params.clear();
    m_execParams.clear();

    if (schemaName) {
      args.setSchema(StringFunctions::toString(schemaName, nameLength1));
    }
    if (tableName) {
      args.setTable(StringFunctions::toString(tableName, nameLength2));
    }
    auto rs = m_conn.m_conn.getSchemaMetaData(
        DatabaseMetaDataCall::PRIMARYKEYS, args);
    setResultSet(rs);
    m_pstmt.reset();
    return handleWarnings(m_resultSet.get());
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
}

template<typename CHAR_TYPE>
SQLRETURN SnappyStatement::getImportedKeysT(CHAR_TYPE* schemaName,
    SQLSMALLINT nameLength1, CHAR_TYPE* tableName, SQLSMALLINT nameLength2) {
  clearLastError();
  if (m_resultSet) {
    // old cursor still open
    setException(
        GET_SQLEXCEPTION2(SQLStateMessage::INVALID_CURSOR_STATE_MSG1));
    return SQL_ERROR;
  }
  try {
    DatabaseMetaDataArgs args;

    m_params.clear();
    m_execParams.clear();

    if (schemaName) {
      args.setSchema(StringFunctions::toString(schemaName, nameLength1));
    }
    if (tableName) {
      args.setTable(StringFunctions::toString(tableName, nameLength2));
    }
    auto rs = m_conn.m_conn.getSchemaMetaData(
        DatabaseMetaDataCall::IMPORTEDKEYS, args);
    setResultSet(rs);
    m_pstmt.reset();
    return handleWarnings(m_resultSet.get());
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
}

template<typename CHAR_TYPE>
SQLRETURN SnappyStatement::getExportedKeysT(CHAR_TYPE* schemaName,
    SQLSMALLINT nameLength1, CHAR_TYPE* tableName, SQLSMALLINT nameLength2) {
  clearLastError();
  if (m_resultSet) {
    // old cursor still open
    setException(
        GET_SQLEXCEPTION2(SQLStateMessage::INVALID_CURSOR_STATE_MSG1));
    return SQL_ERROR;
  }
  try {
    DatabaseMetaDataArgs args;

    m_params.clear();
    m_execParams.clear();

    if (schemaName) {
      args.setSchema(StringFunctions::toString(schemaName, nameLength1));
    }
    if (tableName) {
      args.setTable(StringFunctions::toString(tableName, nameLength2));
    }
    auto rs = m_conn.m_conn.getSchemaMetaData(
        DatabaseMetaDataCall::EXPORTEDKEYS, args);
    setResultSet(rs);
    m_pstmt.reset();
    return handleWarnings(m_resultSet.get());
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
}

template<typename CHAR_TYPE>
SQLRETURN SnappyStatement::getCrossReferenceT(CHAR_TYPE* parentSchemaName,
    SQLSMALLINT nameLength1, CHAR_TYPE* parentTableName,
    SQLSMALLINT nameLength2, CHAR_TYPE* foreignSchemaName,
    SQLSMALLINT nameLength3, CHAR_TYPE* foreignTableName,
    SQLSMALLINT nameLength4) {
  clearLastError();
  if (m_resultSet) {
    // old cursor still open
    setException(
        GET_SQLEXCEPTION2(SQLStateMessage::INVALID_CURSOR_STATE_MSG1));
    return SQL_ERROR;
  }
  try {
    DatabaseMetaDataArgs args;
    std::string sparentSchemaName, sparentTableName, sforeignSchemaName,
        sforeignTableName;

    m_params.clear();
    m_execParams.clear();

    if (parentSchemaName) {
      args.setSchema(StringFunctions::toString(parentSchemaName, nameLength1));
    }
    if (parentTableName) {
      args.setTable(StringFunctions::toString(parentTableName, nameLength2));
    }
    if (foreignSchemaName) {
      args.setForeignSchema(
          StringFunctions::toString(foreignSchemaName, nameLength3));
    }
    if (foreignTableName) {
      args.setForeignTable(
          StringFunctions::toString(foreignTableName, nameLength4));
    }
    auto rs = m_conn.m_conn.getSchemaMetaData(
        DatabaseMetaDataCall::CROSSREFERENCE, args);
    setResultSet(rs);
    m_pstmt.reset();
    return handleWarnings(m_resultSet.get());
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
}

template<typename CHAR_TYPE>
SQLRETURN SnappyStatement::getProceduresT(CHAR_TYPE* schemaPattern,
    SQLSMALLINT nameLength1, CHAR_TYPE* procedureNamePattern,
    SQLSMALLINT nameLength2) {
  clearLastError();
  if (m_resultSet) {
    // old cursor still open
    setException(
        GET_SQLEXCEPTION2(SQLStateMessage::INVALID_CURSOR_STATE_MSG1));
    return SQL_ERROR;
  }
  try {
    DatabaseMetaDataArgs args;

    m_params.clear();
    m_execParams.clear();

    if (schemaPattern) {
      args.setSchema(StringFunctions::toString(schemaPattern, nameLength1));
    }
    if (procedureNamePattern) {
      args.setProcedureName(
          StringFunctions::toString(procedureNamePattern, nameLength2));
    }
    auto rs = m_conn.m_conn.getSchemaMetaData(
        DatabaseMetaDataCall::PROCEDURES, args);
    setResultSet(rs);
    m_pstmt.reset();
    return handleWarnings(m_resultSet.get());
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
}

template<typename CHAR_TYPE>
SQLRETURN SnappyStatement::getProcedureColumnsT(CHAR_TYPE* schemaPattern,
    SQLSMALLINT nameLength1, CHAR_TYPE* procedureNamePattern,
    SQLSMALLINT nameLength2, CHAR_TYPE* columnNamePattern,
    SQLSMALLINT nameLength3) {
  clearLastError();
  if (m_resultSet) {
    // old cursor still open
    setException(
        GET_SQLEXCEPTION2(SQLStateMessage::INVALID_CURSOR_STATE_MSG1));
    return SQL_ERROR;
  }
  try {
    DatabaseMetaDataArgs args;

    m_params.clear();
    m_execParams.clear();

    if (schemaPattern) {
      args.setSchema(StringFunctions::toString(schemaPattern, nameLength1));
    }
    if (procedureNamePattern) {
      args.setProcedureName(
          StringFunctions::toString(procedureNamePattern, nameLength2));
    }
    if (columnNamePattern) {
      args.setColumnName(
          StringFunctions::toString(columnNamePattern, nameLength3));
    }
    auto rs = m_conn.m_conn.getSchemaMetaData(
        DatabaseMetaDataCall::PROCEDURECOLUMNS, args);
    setResultSet(rs);
    m_pstmt.reset();
    return handleWarnings(m_resultSet.get());
  } catch (SQLException &sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
}

template<typename CHAR_TYPE>
SQLRETURN SnappyStatement::getTypeInfoT(SQLSMALLINT dataType) {
  clearLastError();
  if (m_resultSet) {
    // old cursor still open
    setException(
        GET_SQLEXCEPTION2(SQLStateMessage::INVALID_CURSOR_STATE_MSG1));
    return SQL_ERROR;
  }
  try {
    DatabaseMetaDataArgs args;
    m_params.clear();
    m_execParams.clear();

    if (dataType == SQL_ALL_TYPES) {
      auto rs = m_conn.m_conn.getSchemaMetaData(
          DatabaseMetaDataCall::TYPEINFO, args);
      setResultSet(rs);
    } else {
      auto rs = m_conn.m_conn.getSchemaMetaData(
          DatabaseMetaDataCall::TYPEINFO,
          args.setType(convertTypeToSQLType(dataType, 1)));
      setResultSet(rs);
    }
    m_pstmt.reset();
    return handleWarnings(m_resultSet.get());
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
}

SQLRETURN SnappyStatement::getAttribute(SQLINTEGER attribute,
    SQLPOINTER valueBuffer, SQLINTEGER bufferLen, SQLINTEGER* valueLen) {
  return getAttributeT<SQLCHAR>(attribute, valueBuffer, bufferLen, valueLen);
}

SQLRETURN SnappyStatement::getAttributeW(SQLINTEGER attribute,
    SQLPOINTER valueBuffer, SQLINTEGER bufferLen, SQLINTEGER* valueLen) {
  return getAttributeT<SQLWCHAR>(attribute, valueBuffer, bufferLen, valueLen);
}

SQLRETURN SnappyStatement::setAttribute(SQLINTEGER attribute,
    SQLPOINTER valueBuffer, SQLINTEGER valueLen) {
  return setAttributeT<SQLCHAR>(attribute, valueBuffer, valueLen);
}

SQLRETURN SnappyStatement::setAttributeW(SQLINTEGER attribute,
    SQLPOINTER valueBuffer, SQLINTEGER valueLen) {
  return setAttributeT<SQLWCHAR>(attribute, valueBuffer, valueLen);
}

SQLRETURN SnappyStatement::getResultColumnDescriptor(
    SQLUSMALLINT columnNumber, SQLCHAR* columnName, SQLSMALLINT bufferLength,
    SQLSMALLINT* nameLength, SQLSMALLINT* dataType, SQLULEN* columnSize,
    SQLSMALLINT* decimalDigits, SQLSMALLINT* nullable) {
  return getResultColumnDescriptorT(columnNumber, columnName, bufferLength,
      nameLength, dataType, columnSize, decimalDigits, nullable);
}

SQLRETURN SnappyStatement::getResultColumnDescriptor(
    SQLUSMALLINT columnNumber, SQLWCHAR* columnName, SQLSMALLINT bufferLength,
    SQLSMALLINT* nameLength, SQLSMALLINT* dataType, SQLULEN* columnSize,
    SQLSMALLINT* decimalDigits, SQLSMALLINT* nullable) {
  return getResultColumnDescriptorT(columnNumber, columnName, bufferLength,
      nameLength, dataType, columnSize, decimalDigits, nullable);
}

SQLRETURN SnappyStatement::getColumnAttribute(SQLUSMALLINT columnNumber,
    SQLUSMALLINT fieldId, SQLPOINTER charAttribute, SQLSMALLINT bufferLength,
    SQLSMALLINT* stringLength, SQLLEN* numericAttribute) {
  return getColumnAttributeT(columnNumber, fieldId, charAttribute,
      bufferLength, stringLength, numericAttribute, sizeof(SQLCHAR));
}

SQLRETURN SnappyStatement::getColumnAttributeW(SQLUSMALLINT columnNumber,
    SQLUSMALLINT fieldId, SQLPOINTER charAttribute, SQLSMALLINT bufferLength,
    SQLSMALLINT* stringLength, SQLLEN* numericAttribute) {
  return getColumnAttributeT(columnNumber, fieldId, charAttribute,
      bufferLength, stringLength, numericAttribute, sizeof(SQLWCHAR));
}

SQLRETURN SnappyStatement::getParamMetadata(SQLUSMALLINT paramNumber,
    SQLSMALLINT* paramDataTypePtr, SQLULEN* paramSizePtr,
    SQLSMALLINT* decimalDigitsPtr, SQLSMALLINT* nullablePtr) {
  clearLastError();
  try {
    if (isPrepared()) {
      auto pmd = m_pstmt->getParameterDescriptor(paramNumber);
      *paramDataTypePtr = convertSQLTypeToType(pmd.getSQLType());
      *paramSizePtr = pmd.getPrecision();
      *decimalDigitsPtr = pmd.getScale();
      *nullablePtr = convertNullability(pmd.getNullability());
      return SQL_SUCCESS;
    } else {
      // statement not prepared
      setException(
          GET_SQLEXCEPTION2(SQLStateMessage::FUNCTION_SEQUENCE_ERROR_MSG,
              "statement not prepared for parameter descriptor"));
      return SQL_ERROR;
    }
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
}

SQLRETURN SnappyStatement::getNumResultColumns(SQLSMALLINT* columnCount) {
  clearLastError();
  try {
    if (m_resultSet) {
      if (columnCount) {
        *columnCount = m_resultSet->getColumnCount();
      }
      return SQL_SUCCESS;
    } else if (isPrepared()) {
      if (columnCount) {
        *columnCount = m_pstmt->getColumnCount();
      }
      return SQL_SUCCESS;
    } else if (m_result) {
      // statement does not return a ResultSet
      if (columnCount) {
        *columnCount = 0;
      }
      return SQL_SUCCESS;
    } else {
      // no execution done
      setException(
          GET_SQLEXCEPTION2(SQLStateMessage::INVALID_CURSOR_STATE_MSG2));
      return SQL_ERROR;
    }
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
}

SQLRETURN SnappyStatement::getNumParameters(SQLSMALLINT* parameterCount) {
  clearLastError();
  try {
    if (isPrepared()) {
      if (parameterCount) {
        *parameterCount = m_pstmt->getParameterCount();
      }
      return SQL_SUCCESS;
    } else {
      setException(
          GET_SQLEXCEPTION2(SQLStateMessage::STATEMENT_NOT_PREPARED_MSG));
      return SQL_ERROR;
    }
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
}

SQLRETURN SnappyStatement::getCursorName(SQLCHAR* cursorName,
    SQLSMALLINT bufferLength, SQLSMALLINT* nameLength) {
  return getCursorNameT(cursorName, bufferLength, nameLength);
}

SQLRETURN SnappyStatement::getCursorName(SQLWCHAR* cursorName,
    SQLSMALLINT bufferLength, SQLSMALLINT* nameLength) {
  return getCursorNameT(cursorName, bufferLength, nameLength);
}

SQLRETURN SnappyStatement::setCursorName(SQLCHAR* cursorName,
    SQLSMALLINT nameLength) {
  return setCursorNameT(cursorName, nameLength);
}

SQLRETURN SnappyStatement::setCursorName(SQLWCHAR* cursorName,
    SQLSMALLINT nameLength) {
  return setCursorNameT(cursorName, nameLength);
}

SQLRETURN SnappyStatement::getTables(SQLCHAR* schemaName,
    SQLSMALLINT nameLength1, SQLCHAR* tableName, SQLSMALLINT nameLength2,
    SQLCHAR* tableTypes, SQLSMALLINT nameLength3) {
  return getTablesT(schemaName, nameLength1, tableName, nameLength2,
      tableTypes, nameLength3);
}

SQLRETURN SnappyStatement::getTables(SQLWCHAR* schemaName,
    SQLSMALLINT nameLength1, SQLWCHAR* tableName, SQLSMALLINT nameLength2,
    SQLWCHAR* tableTypes, SQLSMALLINT nameLength3) {
  return getTablesT(schemaName, nameLength1, tableName, nameLength2,
      tableTypes, nameLength3);
}

SQLRETURN SnappyStatement::getTablePrivileges(SQLCHAR* schemaName,
    SQLSMALLINT nameLength1, SQLCHAR* tableName, SQLSMALLINT nameLength2) {
  return getTablePrivilegesT(schemaName, nameLength1, tableName, nameLength2);
}

SQLRETURN SnappyStatement::getTablePrivileges(SQLWCHAR* schemaName,
    SQLSMALLINT nameLength1, SQLWCHAR* tableName, SQLSMALLINT nameLength2) {
  return getTablePrivilegesT(schemaName, nameLength1, tableName, nameLength2);
}

SQLRETURN SnappyStatement::getColumns(SQLCHAR* schemaName,
    SQLSMALLINT nameLength1, SQLCHAR* tableName, SQLSMALLINT nameLength2,
    SQLCHAR* columnName, SQLSMALLINT nameLength3) {
  return getColumnsT(schemaName, nameLength1, tableName, nameLength2,
      columnName, nameLength3);
}

SQLRETURN SnappyStatement::getColumns(SQLWCHAR* schemaName,
    SQLSMALLINT nameLength1, SQLWCHAR* tableName, SQLSMALLINT nameLength2,
    SQLWCHAR* columnName, SQLSMALLINT nameLength3) {
  return getColumnsT(schemaName, nameLength1, tableName, nameLength2,
      columnName, nameLength3);
}

SQLRETURN SnappyStatement::getSpecialColumns(SQLUSMALLINT identifierType,
    SQLCHAR *schemaName, SQLSMALLINT nameLength1, SQLCHAR *tableName,
    SQLSMALLINT nameLength2, SQLUSMALLINT scope, SQLUSMALLINT nullable) {
  return getSpecialColumnsT(identifierType, schemaName, nameLength1,
      tableName, nameLength2, scope, nullable);
}

SQLRETURN SnappyStatement::getSpecialColumns(SQLUSMALLINT identifierType,
    SQLWCHAR* schemaName,
    SQLSMALLINT nameLength1, SQLWCHAR* tableName, SQLSMALLINT nameLength2,
    SQLUSMALLINT scope, SQLUSMALLINT nullable) {
  return getSpecialColumnsT(identifierType,
      schemaName, nameLength1, tableName, nameLength2, scope, nullable);
}

SQLRETURN SnappyStatement::getColumnPrivileges(SQLCHAR* schemaName,
    SQLSMALLINT nameLength1, SQLCHAR* tableName, SQLSMALLINT nameLength2,
    SQLCHAR* columnName, SQLSMALLINT nameLength3) {
  return getColumnPrivilegesT(schemaName, nameLength1, tableName, nameLength2,
      columnName, nameLength3);
}

SQLRETURN SnappyStatement::getColumnPrivileges(SQLWCHAR* schemaName,
    SQLSMALLINT nameLength1, SQLWCHAR* tableName, SQLSMALLINT nameLength2,
    SQLWCHAR* columnName, SQLSMALLINT nameLength3) {
  return getColumnPrivilegesT(schemaName, nameLength1, tableName, nameLength2,
      columnName, nameLength3);
}

SQLRETURN SnappyStatement::getIndexInfo(SQLCHAR* schemaName,
    SQLSMALLINT nameLength1, SQLCHAR* tableName, SQLSMALLINT nameLength2,
    bool unique, bool approximate) {
  return getIndexInfoT(schemaName, nameLength1, tableName, nameLength2,
      unique, approximate);
}

SQLRETURN SnappyStatement::getIndexInfo(SQLWCHAR* schemaName,
    SQLSMALLINT nameLength1, SQLWCHAR* tableName, SQLSMALLINT nameLength2,
    bool unique, bool approximate) {
  return getIndexInfoT(schemaName, nameLength1, tableName, nameLength2,
      unique, approximate);
}

SQLRETURN SnappyStatement::getPrimaryKeys(SQLCHAR* schemaName,
    SQLSMALLINT nameLength1, SQLCHAR* tableName, SQLSMALLINT nameLength2) {
  return getPrimaryKeysT(schemaName, nameLength1, tableName, nameLength2);
}

SQLRETURN SnappyStatement::getPrimaryKeys(SQLWCHAR* schemaName,
    SQLSMALLINT nameLength1, SQLWCHAR* tableName, SQLSMALLINT nameLength2) {
  return getPrimaryKeysT(schemaName, nameLength1, tableName, nameLength2);
}

SQLRETURN SnappyStatement::getImportedKeys(SQLCHAR* schemaName,
    SQLSMALLINT nameLength1, SQLCHAR* tableName, SQLSMALLINT nameLength2) {
  return getImportedKeysT(schemaName, nameLength1, tableName, nameLength2);
}

SQLRETURN SnappyStatement::getImportedKeys(SQLWCHAR* schemaName,
    SQLSMALLINT nameLength1, SQLWCHAR* tableName, SQLSMALLINT nameLength2) {
  return getImportedKeysT(schemaName, nameLength1, tableName, nameLength2);
}

SQLRETURN SnappyStatement::getExportedKeys(SQLCHAR* schemaName,
    SQLSMALLINT nameLength1, SQLCHAR* tableName, SQLSMALLINT nameLength2) {
  return getExportedKeysT(schemaName, nameLength1, tableName, nameLength2);
}

SQLRETURN SnappyStatement::getExportedKeys(SQLWCHAR* schemaName,
    SQLSMALLINT nameLength1, SQLWCHAR* tableName, SQLSMALLINT nameLength2) {
  return getExportedKeysT(schemaName, nameLength1, tableName, nameLength2);
}

SQLRETURN SnappyStatement::getCrossReference(SQLCHAR* parentSchemaName,
    SQLSMALLINT nameLength1, SQLCHAR* parentTableName,
    SQLSMALLINT nameLength2, SQLCHAR* foreignSchemaName,
    SQLSMALLINT nameLength3, SQLCHAR* foreignTableName,
    SQLSMALLINT nameLength4) {
  return getCrossReferenceT(parentSchemaName, nameLength1, parentTableName,
      nameLength2, foreignSchemaName, nameLength3, foreignTableName,
      nameLength4);
}

SQLRETURN SnappyStatement::getCrossReference(SQLWCHAR* parentSchemaName,
    SQLSMALLINT nameLength1, SQLWCHAR* parentTableName,
    SQLSMALLINT nameLength2, SQLWCHAR* foreignSchemaName,
    SQLSMALLINT nameLength3, SQLWCHAR* foreignTableName,
    SQLSMALLINT nameLength4) {
  return getCrossReferenceT(parentSchemaName, nameLength1, parentTableName,
      nameLength2, foreignSchemaName, nameLength3, foreignTableName,
      nameLength4);
}

SQLRETURN SnappyStatement::getProcedures(SQLCHAR* schemaPattern,
    SQLSMALLINT nameLength1, SQLCHAR* procedureNamePattern,
    SQLSMALLINT nameLength2) {
  return getProceduresT(schemaPattern, nameLength1, procedureNamePattern,
      nameLength2);
}

SQLRETURN SnappyStatement::getProcedures(SQLWCHAR* schemaPattern,
    SQLSMALLINT nameLength1, SQLWCHAR* procedureNamePattern,
    SQLSMALLINT nameLength2) {
  return getProceduresT(schemaPattern, nameLength1, procedureNamePattern,
      nameLength2);
}

SQLRETURN SnappyStatement::getProcedureColumns(SQLCHAR* schemaPattern,
    SQLSMALLINT nameLength1, SQLCHAR* procedureNamePattern,
    SQLSMALLINT nameLength2, SQLCHAR* columnNamePattern,
    SQLSMALLINT nameLength3) {
  return getProcedureColumnsT(schemaPattern, nameLength1,
      procedureNamePattern, nameLength2, columnNamePattern, nameLength3);
}

SQLRETURN SnappyStatement::getProcedureColumns(SQLWCHAR* schemaPattern,
    SQLSMALLINT nameLength1, SQLWCHAR* procedureNamePattern,
    SQLSMALLINT nameLength2, SQLWCHAR* columnNamePattern,
    SQLSMALLINT nameLength3) {
  return getProcedureColumnsT(schemaPattern, nameLength1,
      procedureNamePattern, nameLength2, columnNamePattern, nameLength3);
}

SQLRETURN SnappyStatement::getTypeInfo(SQLSMALLINT dataType) {
  return getTypeInfoT<SQLCHAR>(dataType);
}

SQLRETURN SnappyStatement::getTypeInfoW(SQLSMALLINT dataType) {
  return getTypeInfoT<SQLWCHAR>(dataType);
}

SQLRETURN SnappyStatement::getMoreResults() {
  clearLastError();

  try {
    if (isPrepared()) {
      auto rs = m_pstmt->getNextResults();
      setResultSet(rs);
      if (m_resultSet && m_resultSet->isOpen()) {
        return handleWarnings(m_resultSet.get());
      } else {
        return SQL_NO_DATA;
      }
    } else if (m_resultSet) {
      auto rs = m_resultSet->getNextResults();
      setResultSet(rs);
      if (m_resultSet && m_resultSet->isOpen()) {
        return handleWarnings(m_resultSet.get());
      } else {
        return SQL_NO_DATA;
      }
    } else {
      return SQL_NO_DATA;
    }
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
}

// TODO: do we need to check the statement type
SQLRETURN SnappyStatement::getParamData(SQLPOINTER* valuePtr) {
  clearLastError();
  const size_t totalParamCount = m_params.size();
  try {
    for (size_t i = m_currentParameterIndex; i < totalParamCount; i++) {
      Parameter& param = m_params[i];
      if (param.m_isDataAtExecParam || IS_DATA_AT_EXEC(param.m_o_lenOrIndp)) {
        valuePtr = &param.m_o_value;
        m_currentParameterIndex = i;
        param.m_o_value = nullptr;
        param.m_o_valueSize = 0;
        param.m_o_lenOrIndp = nullptr;
        param.m_isBound = false;
        param.m_isDataAtExecParam = false;
        return SQL_NEED_DATA;
      }
    }
    // No more data at exec parameters so execute the statement
    return execute();
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
}

SQLRETURN SnappyStatement::putData(SQLPOINTER dataPtr, SQLLEN dataLength) {
  clearLastError();
  if (m_currentParameterIndex >= m_params.size()) {
    setException(
        GET_SQLEXCEPTION2(SQLStateMessage::FUNCTION_SEQUENCE_ERROR_MSG,
            "invalid current parameter position (SQLBindParameter not done?)"));
    return SQL_ERROR;
  }
  try {
    Parameter &currentParam = m_params[m_currentParameterIndex];
    currentParam.m_isDataAtExecParam = false;
    if (dataPtr && dataLength == SQL_NULL_DATA) {
      dataPtr = nullptr;
    }
    currentParam.m_o_value = dataPtr;
    // temporarily point m_o_lenOrIndp to dataLength and use RAII to revert
    struct SwitchLenOrIndp {
      SQLLEN *lenOrIndp;
      Parameter &param;
      ~SwitchLenOrIndp() {
        param.m_o_lenOrIndp = lenOrIndp;
      }
    };
    SwitchLenOrIndp s = { currentParam.m_o_lenOrIndp, currentParam };
    currentParam.m_o_lenOrIndp = &dataLength;
    SQLRETURN res = bindParameter(m_execParams, currentParam, nullptr, true);
    if (res == SQL_SUCCESS) {
      currentParam.m_isBound = true;
    }
    return res;
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
}

SQLRETURN SnappyStatement::closeResultSet(bool ifPresent) {
  clearLastError();
  try {
    if (m_resultSet) {
      m_resultSet->close(false);
      m_resultSet = nullptr;
      m_cursor.clear();
    } else if (!ifPresent) {
      // no open cursor
      setException(
          GET_SQLEXCEPTION2(SQLStateMessage::INVALID_CURSOR_STATE_MSG2));
      return SQL_ERROR;
    }
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
  return SQL_SUCCESS;
}

SQLRETURN SnappyStatement::resetParameters() {
  clearLastError();
  m_params.clear();
  m_execParams.clear();
  return SQL_SUCCESS;
}

SQLRETURN SnappyStatement::cancel() {
  clearLastError();
  try {
    if (isPrepared()) {
      if (m_pstmt->cancel()) return SQL_SUCCESS;
    } else if (m_resultSet) {
      if (m_resultSet->cancelStatement()) return SQL_SUCCESS;
    } else {
      return m_conn.cancelCurrentStatement();
    }
    setException(
        GET_SQLEXCEPTION2(SQLStateMessage::FUNCTION_SEQUENCE_ERROR_MSG,
            "SQLCancel on an invalid statement (unprepared or not executed)"));
    return SQL_ERROR;
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  }
}

SQLRETURN SnappyStatement::close() noexcept {
  clearLastError();
  try {
    if (m_resultSet) {
      m_resultSet->close(!isPrepared());
      m_resultSet = nullptr;
    }
    if (isPrepared()) {
      m_pstmt->close();
      m_pstmt.reset();
    }
    m_result.reset();
    m_params.clear();
    m_execParams.clear();
    m_outputFields.clear();
  } catch (SQLException& sqle) {
    setException(sqle);
    return SQL_ERROR;
  } catch (std::exception& se) {
    setException(__FILE__, __LINE__, se);
    return SQL_ERROR;
  } catch (...) {
    std::runtime_error err("Unknown exception in SnappyStatement close");
    setException(__FILE__, __LINE__, err);
    return SQL_ERROR;
  }
  return SQL_SUCCESS;
}
