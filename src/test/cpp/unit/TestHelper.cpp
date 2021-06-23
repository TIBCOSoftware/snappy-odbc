/*
 * Copyright (c) 2016 SnappyData, Inc. All rights reserved.
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

#include "TestHelper.h"

#include <string>
#include <fstream>

static std::string getConnString(const char* serverHostVar,
    bool useDSN = true) {
  if (useDSN) {
    return "DSN=snappydsn";
  }

  const char* serverHost = ::getenv(serverHostVar);
  if (!serverHost || serverHost[0] == '\0') {
    serverHost = "localhost";
  }
  std::string connStr;
  if (useDSN) {
    connStr.append("Driver=SnappyData;");
  }
  connStr.append("server=").append(serverHost);
  connStr.append(
      ";port=1527;auto-reconnect=false;credential-manager=false"
      ";load-balance=true;password=snappyodbc;ssl=false;user=snappyodbc");

  std::cout << "USING CONNECTION STRING: " << connStr << "\n\n" << std::endl;
  return connStr;
}

std::string SNAPPYCONNSTRING(std::move(getConnString("ODBC_SERVERHOST")));

std::string SNAPPYCONNSTRINGNODRIVER(std::move(getConnString("SERVERHOST", false)));
std::string SNAPPYCONNSTRINGSERVER(std::move(getConnString("SERVERHOST")));

char* getStringForSQLCODE(SQLRETURN inputCode, char* outputStr) {
  memset(outputStr, 0, 1024);
  switch (inputCode) {
    case SQL_SUCCESS:
      strcpy(outputStr, "SQL_SUCCESS");
      break;
    case SQL_SUCCESS_WITH_INFO:
      strcpy(outputStr, "SQL_SUCCESS_WITH_INFO");
      break;
    case SQL_ERROR:
      strcpy(outputStr, "SQL_ERROR");
      break;
    case SQL_INVALID_HANDLE:
      strcpy(outputStr, "SQL_INVALID_HANDLE");
      break;
    case SQL_NO_DATA:
      strcpy(outputStr, "SQL_NO_DATA");
      break;
    case SQL_STILL_EXECUTING:
      strcpy(outputStr, "SQL_STILL_EXECUTING");
      break;
    case SQL_NEED_DATA:
      strcpy(outputStr, "SQL_NEED_DATA");
      break;
  }
  return outputStr;
}

/* --------------------------------------------------------------------------
 | DiagRecCheck:
 |     This function will do a simple comparison of return codes and issue
 |     erros on failure.
 |
 | Returns:
 |
 --------------------------------------------------------------------------
 */
bool DiagRec_Check(SQLSMALLINT HandleType, SQLHANDLE Handle,
    SQLSMALLINT RecNumber, SQLRETURN expected, SQLRETURN actual,
    LPSTR szFuncName, int lineNumber) {
  SQLRETURN api_rc;
  SQLCHAR sqlstate[10];
  SQLINTEGER esq_sql_code;
  SQLCHAR error_txt[1024];
  SQLSMALLINT len_error_txt = 1024; //ERROR_TEXT_LEN
  SQLSMALLINT used_error_txt;
  char outtxt[50]; //MAX_NAME_LEN
  char expectedStr[1024];
  char actualStr[1024];

  int opt = OUTPUT;
  size_t i;
  bool retVal;
  char buffer[1280];

  i = strlen(szFuncName);
  if (szFuncName[i - 1] == OUTPUTCH) opt = NO_OUTPUT;

  if (opt != NO_OUTPUT) {
    memset(buffer, 0, 1280);
    sprintf(buffer, "%s -> retcode: %d at line %d\n", szFuncName, actual,
        lineNumber);
    LOGSTR(buffer);
  }

  if (opt != NO_OUTPUT) {
    memset(buffer, 0, 1280);
    sprintf(buffer, "%s -> Expected output : '%s' Actual output: '%s'\n",
        szFuncName, getStringForSQLCODE(expected, expectedStr),
        getStringForSQLCODE(actual, actualStr));
    LOGSTR(buffer);
  }

  if (expected != actual) {
    retVal = false;
  } else
    retVal = true;

  if (actual != SQL_SUCCESS) {
    api_rc = SQLGetDiagRec(HandleType, Handle, RecNumber, sqlstate,
        &esq_sql_code, error_txt, len_error_txt, &used_error_txt);
    if (opt != NO_OUTPUT) {
      if (api_rc == SQL_NO_DATA_FOUND) {
        LOGSTR("SQLGetDiagRec -> (SQL_NO_DATA_FOUND)\n");
      } else {
        if (actual < 0)
          strcpy(outtxt, "Error");
        else
          strcpy(outtxt, "Warning");

        memset(buffer, 0, 1280);
        sprintf(buffer, "SQLGetDiagRec (%s) -> SQLSTATE: %s SQLCODE: %d",
            outtxt, sqlstate, esq_sql_code);
        LOGSTR(buffer);
        memset(buffer, 0, 1280);
        sprintf(buffer, "SQLGetDiagRec (%s) -> Error message: %s\n", outtxt,
            error_txt);
        LOGSTR(buffer);
      }
    }
  }
  return retVal;
}

//*---------------------------------------------------------------------------------
//| Get_pfSqlType:
//|
//|
//| Returns:
//*---------------------------------------------------------------------------------
RETCODE Get_pfSqlType(SQLSMALLINT pfSqlType, CHAR *buffer) {
  switch (pfSqlType) {
    case SQL_BIGINT:
      strcpy(buffer, "SQL_BIGINT");
      break;
    case SQL_BINARY:
      strcpy(buffer, "SQL_BINARY");
      break;
    case SQL_BIT:
      strcpy(buffer, "SQL_BIT");
      break;
    case SQL_CHAR:
      strcpy(buffer, "SQL_CHAR");
      break;
    case SQL_DATE:
      strcpy(buffer, "SQL_DATE");
      break;
    case SQL_DECIMAL:
      strcpy(buffer, "SQL_DECIMAL");
      break;
    case SQL_DOUBLE:
      strcpy(buffer, "SQL_DOUBLE");
      break;
    case SQL_FLOAT:
      strcpy(buffer, "SQL_FLOAT");
      break;
    case SQL_INTEGER:
      strcpy(buffer, "SQL_INTEGER");
      break;
    case SQL_LONGVARBINARY:
      strcpy(buffer, "SQL_LONGVARBINARY");
      break;
    case SQL_LONGVARCHAR:
      strcpy(buffer, "SQL_LONGVARCHAR");
      break;
    case SQL_NUMERIC:
      strcpy(buffer, "SQL_NUMERIC");
      break;
    case SQL_REAL:
      strcpy(buffer, "SQL_REAL");
      break;
    case SQL_SMALLINT:
      strcpy(buffer, "SQL_SMALLINT");
      break;
    case SQL_TIME:
      strcpy(buffer, "SQL_TIME");
      break;
    case SQL_TIMESTAMP:
      strcpy(buffer, "SQL_TIMESTAMP");
      break;
    case SQL_TINYINT:
      strcpy(buffer, "SQL_TINYINT");
      break;
    case SQL_VARBINARY:
      strcpy(buffer, "SQL_VARBINARY");
      break;
    case SQL_VARCHAR:
      strcpy(buffer, "SQL_VARCHAR");
      break;
  }
  return (0);
}
//*---------------------------------------------------------------------------------
//| Get_pfNullable:
//|
//|
//| Returns:
//*---------------------------------------------------------------------------------
RETCODE Get_pfNullable(SQLSMALLINT pfNullable, CHAR *buffer) {
  switch (pfNullable) {
    case SQL_NO_NULLS:
      strcpy(buffer, "SQL_NO_NULLS");
      break;
    case SQL_NULLABLE:
      strcpy(buffer, "SQL_NULLABLE");
      break;
    case SQL_NULLABLE_UNKNOWN:
      strcpy(buffer, "SQL_NULL_UNKNOWN");
      break;
  }
  return (0);
}
//*---------------------------------------------------------------------------------
//| Get_BoolValue:
//|
//|
//| Returns:
//*---------------------------------------------------------------------------------
RETCODE Get_BoolValue(SQLSMALLINT pfDesc, CHAR *buffer) {
  switch (pfDesc) {
    case TRUE:
      strcpy(buffer, "TRUE");
      break;
    case FALSE:
      strcpy(buffer, "FALSE");
      break;
    default:
      strcpy(buffer, "<NULL>");
      break;
  }
  return (0);
}
//*---------------------------------------------------------------------------------
//| Get_Searchable:
//|
//|
//| Returns:
//*---------------------------------------------------------------------------------
RETCODE Get_Searchable(SQLSMALLINT pfDesc, CHAR *buffer) {
  switch (pfDesc) {
    case SQL_UNSEARCHABLE:
      strcpy(buffer, "SQL_UNSEARCHABLE");
      break;
    case SQL_LIKE_ONLY:
      strcpy(buffer, "SQL_LIKE_ONLY");
      break;
    case SQL_ALL_EXCEPT_LIKE:
      strcpy(buffer, "SQL_ALL_EXCEPT_LIKE");
      break;
    case SQL_SEARCHABLE:
      strcpy(buffer, "SQL_SEARCHABLE");
      break;
    default:
      strcpy(buffer, "<NULL>");
      break;
  }
  return (0);
}
//*---------------------------------------------------------------------------------
//| Get_Updatable:
//|
//|
//| Returns:
//*---------------------------------------------------------------------------------
RETCODE Get_Updatable(SQLSMALLINT pfDesc, CHAR *buffer) {
  switch (pfDesc) {
    case SQL_ATTR_READONLY:
      strcpy(buffer, "SQL_ATTR_READONLY");
      break;
    case SQL_ATTR_WRITE:
      strcpy(buffer, "SQL_ATTR_WRITE");
      break;
    case SQL_ATTR_READWRITE_UNKNOWN:
      strcpy(buffer, "SQL_ATTR_READWRITE_UNKNOWN");
      break;
    default:
      strcpy(buffer, "<NULL>");
      break;
  }
  return (0);
}

//*---------------------------------------------------------------------------------
//| lst_ColumnNames:
//|
//|
//| Returns:
//*---------------------------------------------------------------------------------
RETCODE lst_ColumnNames(SQLHENV henv, SQLHDBC hdbc, SQLHSTMT hstmt,
    int outcol) {
  RETCODE rc = SQL_ERROR;
  /* ------------------------------------------------------------------------- */
  SQLSMALLINT icol;
  SQLCHAR szColName[50];
  SQLSMALLINT cbColNameMax;
  SQLSMALLINT pcbColName;
  SQLSMALLINT pfSqlType;
  SQLULEN pcbColDef;
  SQLSMALLINT pibScale;
  SQLSMALLINT pfNullable;
  /* ------------------------------------------------------------------------- */
  printf("\tColumns->|");

  icol = 1;
  cbColNameMax = 18;
  while (icol <= outcol) {
    rc = SQLDescribeCol(hstmt, icol, szColName, cbColNameMax, &pcbColName,
        &pfSqlType, &pcbColDef, &pibScale, &pfNullable);
    printf("%s|", szColName);
    icol++;
  }
  printf("\r\n");

  return rc;
}

RETCODE installOrReplaceJar(SQLHENV henv, SQLHDBC hdbc, SQLHSTMT hstmt,
    const char* jarFile, const char* jarInstallName) {

  if (!jarFile || !jarInstallName) {
    return SQL_ERROR;
  }

  std::ifstream jar;
  int length;
  jar.open(jarFile, std::ios::binary);
  jar.seekg(0, std::ios::end);
  length = (int)jar.tellg();
  jar.seekg(0, std::ios::beg);
  char* buffer = new char[length];
  jar.read(buffer, length);
  jar.close();

  RETCODE ret = SQL_SUCCESS;
  SQLLEN len = SQL_NTS;
  ::SQLPrepare(hstmt, (SQLCHAR*)"CALL SQLJ.INSTALL_JAR_BYTES(?, ?)", SQL_NTS);
  ::SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_BINARY, 0, 0,
      buffer, length, nullptr);
  ::SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 0, 0,
      (SQLPOINTER)jarInstallName, 0, &len);
  if (::SQLExecute(hstmt) == SQL_ERROR) {
    // try replace
    ::SQLPrepare(hstmt, (SQLCHAR*)"CALL SQLJ.REPLACE_JAR_BYTES(?, ?)",
        SQL_NTS);
    ::SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_BINARY, 0, 0,
        buffer, length, nullptr);
    ::SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 0, 0,
        (SQLPOINTER)jarInstallName, 0, &len);
    ret = ::SQLExecute(hstmt);
  }

  ::SQLFreeStmt(hstmt, SQL_RESET_PARAMS);

  delete[] buffer;
  return ret;
}
