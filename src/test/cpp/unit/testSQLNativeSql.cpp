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

#include "TestHelper.h"

//*-------------------------------------------------------------------------

#define TESTNAME "SQLNativeSql"

#define MAX_NAME_LEN 256
#define ERROR_TEXT_LEN 511

#define STR_LEN 128+1
#define REM_LEN 254+1

#define NULL_VALUE "<NULL>"

//*-------------------------------------------------------------------------

TEST(SQLNativeSql, Test1) {
  DECLARE_SQLHANDLES

  retcode = ::SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
  EXPECT_EQ(SQL_SUCCESS, retcode) << "SQLAllocHandle call failed";
  EXPECT_NE(nullptr, henv)
      << "SQLAllocHandle failed to return valid env handle";

  retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
  DIAGRECCHECK(SQL_HANDLE_ENV, henv, 1, SQL_SUCCESS, retcode,
      "SQLSetEnvAttr (HENV)");

  retcode = ::SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
  EXPECT_EQ(SQL_SUCCESS, retcode) << "SQLAllocHandle call failed";
  EXPECT_NE(nullptr, hdbc)
      << "SQLAllocHandle failed to return valid DBC handle";

  retcode = ::SQLNativeSql(SQL_NULL_HANDLE, nullptr, 0, nullptr, 0, nullptr);
  EXPECT_EQ(SQL_INVALID_HANDLE, retcode)
      << "SQLNativeSql should return Invalid Handle";

  retcode = ::SQLNativeSql(SQL_NULL_HANDLE, (SQLCHAR*)"abc", 0, nullptr, 0,
      nullptr);
  EXPECT_EQ(SQL_INVALID_HANDLE, retcode)
      << "SQLNativeSql should return Invalid Handle";

  retcode = ::SQLNativeSql(hdbc, nullptr, 0, nullptr, 0, nullptr);
  EXPECT_EQ(SQL_ERROR, retcode) << "SQLNativeSql should return sql error";

  retcode = ::SQLNativeSql(hdbc, (SQLCHAR*)"abc", 0, nullptr, 0, nullptr);
  EXPECT_EQ(SQL_ERROR, retcode) << "SQLNativeSql should return sql error";

  retcode = SQLDriverConnect(hdbc, nullptr, (SQLCHAR*)SNAPPYCONNSTRING.c_str(),
      SQL_NTS, nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);
  EXPECT_EQ(SQL_SUCCESS, retcode) << "SQLDriverConnect call failed";

  SQLCHAR buffer[1024];
  const char* sql = "SELECT { fn CONVERT (empid, SQL_SMALLINT) } FROM employee";
  retcode = ::SQLNativeSql(hdbc, (SQLCHAR*)sql, (SQLINTEGER)strlen(sql),
      buffer, 1024, nullptr);
  EXPECT_EQ(SQL_SUCCESS, retcode) << "SQLNativeSql call failed";

  //this call is working now not sure how ODBC driver handles this
  sql = "INVALIDSQL";
  retcode = ::SQLNativeSql(hdbc, (SQLCHAR*)sql, (SQLINTEGER)strlen(sql),
      buffer, 1024, nullptr);
  EXPECT_EQ(SQL_SUCCESS, retcode) << "SQLNativeSql call failed";

  retcode = ::SQLDisconnect(hdbc);
  EXPECT_EQ(SQL_SUCCESS, retcode) << "SQLDisconnect  call failed";

  retcode = ::SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
  EXPECT_EQ(SQL_SUCCESS, retcode) << "SQLFreeHandle call failed";

  retcode = ::SQLFreeHandle(SQL_HANDLE_ENV, henv);
  EXPECT_EQ(SQL_SUCCESS, retcode) << "SQLFreeHandle call failed";
}

TEST(SQLNativeSql, Test2) {
  DECLARE_SQLHANDLES

  /* ------------------------------------------------------------------------- */
  /* Declare storage locations for result set data */
  CHAR szSqlStrIn[STR_LEN], szSqlStr[STR_LEN];

  SQLINTEGER pcbSqlStr;

  SQLINTEGER cbSqlStrIn, cbSqlStrMax;

  //CHAR sqlstate[10];
  //DWORD esq_sql_code;
  //CHAR error_txt[ERROR_TEXT_LEN + 1];
  //SQLSMALLINT len_error_txt = ERROR_TEXT_LEN;
  //SQLSMALLINT used_error_txt;
  //CHAR buffer[1024];
  /* ---------------------------------------------------------------------har- */

  //init sql handles (stmt, dbc, env)
  INIT_SQLHANDLES

  /* ------------------------------------------------------------------------- */
  /* ***************************************************************** */
  /* *** I. SQLNativSql ********************************************** */
  /* ***************************************************************** */
  LOGF("I.) SQLNativeSql -> '");

  strcpy(szSqlStrIn, "CREATE TABLE TEST (KEY INTEGER, NAME CHAR(30))");
  cbSqlStrMax = STR_LEN;
  cbSqlStrIn = SQL_NTS;
  LOGF("IN => SqlStrIn   : %s' \t cbSqlStrIn : %d  \t cbSqlStrMax: %d ",
      szSqlStrIn, cbSqlStrIn, cbSqlStrMax);

  retcode = SQLNativeSql(hdbc, (SQLCHAR*)szSqlStrIn, cbSqlStrIn,
      (SQLCHAR*)szSqlStr, cbSqlStrMax, &pcbSqlStr);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLNativeSql");

  LOGF("OUT => SqlStr : '%s'\t pcbSqlStr : %d '", szSqlStr, pcbSqlStr);

  /* ***************************************************************** */
  /* *** II. SQLNativSql ********************************************* */
  /* ***************************************************************** */
  LOGF("II.) SQLNativeSql -> (Data trucated)'");

  strcpy(szSqlStrIn, "CREATE TABLE TEST (KEY INTEGER, NAME CHAR(30))");
  cbSqlStrMax = STR_LEN;
  cbSqlStrIn = SQL_NTS;
  LOGF("IN => SqlStrIn   : %s' \t cbSqlStrIn : %d  \t cbSqlStrMax: %d ",
      szSqlStrIn, cbSqlStrIn, cbSqlStrMax);

  retcode = SQLNativeSql(hdbc, (SQLCHAR*)szSqlStrIn, cbSqlStrIn,
      (SQLCHAR*)szSqlStr, cbSqlStrMax, &pcbSqlStr);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLNativeSql");

  LOGF("OUT => SqlStr : '%s'\t pcbSqlStr : %d '", szSqlStr, pcbSqlStr);

  /* ***************************************************************** */
  /* *** III. SQLNativSql ******************************************** */
  /* ***************************************************************** */
  LOGF("III.) SQLNativeSql -> (Error)'");

  strcpy(szSqlStrIn, "CREATE TABLE TEST (KEY INTEGER, NAME CHAR(30))");
  cbSqlStrMax = STR_LEN;
  cbSqlStrIn = SQL_NTS;
  LOGF("IN => SqlStrIn   : %s' \t cbSqlStrIn : %d  \t cbSqlStrMax: %d ",
      szSqlStrIn, cbSqlStrIn, cbSqlStrMax);

  retcode = SQLNativeSql(hdbc, (SQLCHAR*)szSqlStrIn, cbSqlStrIn,
      (SQLCHAR*)szSqlStr, cbSqlStrMax, &pcbSqlStr);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLNativeSql");

  //free sql handles (stmt, dbc, env)
  FREE_SQLHANDLES
}
