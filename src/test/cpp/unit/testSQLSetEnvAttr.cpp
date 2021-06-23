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

/* ------------------------------------------------------------------------- */
#define TESTNAME "SQLSETENVATTR"
#define TABLE "SETENV"

#define MAX_NAME_LEN 256
#define STRING_LEN 10
#define CHAR_LEN 120

TEST(SQLSetEnvAttr, Test1) {
  DECLARE_SQLHANDLES

  retcode = ::SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
  EXPECT_NE(SQL_ERROR, retcode) << "SQLAllocHandle returned SQL_ERROR";
  EXPECT_NE(nullptr, henv)
      << "SQLAllocHandle failed to return valid env handle";

  retcode = ::SQLSetEnvAttr(SQL_NULL_HANDLE, 0, nullptr, 0);
  EXPECT_EQ(SQL_INVALID_HANDLE, retcode)
      << "Expected SQL_INVALID_HANDLE from SQLSetEnvAttr";

  retcode = ::SQLSetEnvAttr(henv, 0, nullptr, 0);
  EXPECT_EQ(SQL_ERROR, retcode) << "Expected SQL_ERROR from SQLSetEnvAttr";

  retcode = ::SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, nullptr, 0);
  EXPECT_EQ(SQL_ERROR, retcode) << "Expected SQL_ERROR from SQLSetEnvAttr";

  retcode = ::SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)123123,
      0);
  EXPECT_EQ(SQL_ERROR, retcode) << "Expected SQL_ERROR from SQLSetEnvAttr";

  retcode = ::SQLSetEnvAttr(henv, SQL_ATTR_OUTPUT_NTS, nullptr, 0);
  EXPECT_EQ(SQL_ERROR, retcode) << "Expected SQL_ERROR from SQLSetEnvAttr";

  retcode = ::SQLSetEnvAttr(henv, SQL_ATTR_OUTPUT_NTS, (SQLPOINTER)123123, 0);
#if defined(TEST_DIRECT) || defined(_WINDOWS)
  EXPECT_EQ(SQL_ERROR, retcode) << "Expected SQL_ERROR from SQLSetEnvAttr";
#else
  // bug in unixODBC
  EXPECT_EQ(SQL_SUCCESS, retcode) << "Expected SQL_SUCCESS from SQLSetEnvAttr";
#endif

  retcode = ::SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION,
      (SQLPOINTER)SQL_OV_ODBC3, 0);
  EXPECT_EQ(SQL_SUCCESS, retcode)
      << "Expected SQL_SUCCESS from SQLSetEnvAttr";

  retcode = ::SQLSetEnvAttr(henv, SQL_ATTR_CONNECTION_POOLING,
    (SQLPOINTER)SQL_CP_OFF, 0);
  DIAGRECCHECK(SQL_HANDLE_ENV, henv, 1, SQL_SUCCESS, retcode,
    "SQLSetEnvAttr");

  retcode = ::SQLSetEnvAttr(henv, SQL_ATTR_CP_MATCH,
    (SQLPOINTER)SQL_CP_STRICT_MATCH, 0);
  DIAGRECCHECK(SQL_HANDLE_ENV, henv, 1, SQL_SUCCESS, retcode,
    "SQLSetEnvAttr");

  retcode = ::SQLSetEnvAttr(henv, SQL_ATTR_OUTPUT_NTS, (SQLPOINTER)SQL_TRUE,
      0);
  EXPECT_EQ(SQL_SUCCESS, retcode) << "Expected SQL_SUCCESS from SQLSetEnvAttr";

  retcode = ::SQLFreeHandle(SQL_HANDLE_ENV, henv);
  EXPECT_NE(SQL_ERROR, retcode) << "SQLFreeHandle returned SQL_ERROR";
}

TEST(SQLSetEnvAttr, Test2) {
  DECLARE_SQLHANDLES

  /* ------------------------------------------------------------------------- */
  CHAR buf[MAX_NAME_LEN + 1];

  SQLINTEGER pConnectPool;
  SQLINTEGER pCpMatch;
  DWORD pOdbcVer;
  DWORD pOdbcVerSet;
  DWORD pOutputNts;

  SQLINTEGER BufferLength = 0;
  SQLINTEGER StringLengthPtr = 0;
  SQLINTEGER StringLength = 0;
  /* ---------------------------------------------------------------------har- */

  /* --- Connect ----------------------------------------------------- */
  // This test will assume that the ODBC handles passed in
  //              are nullptr.  One could have this function do a connection
  //              and pass the handles to other test functions.
  retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
  DIAGRECCHECK(SQL_HANDLE_ENV, henv, 1, SQL_SUCCESS, retcode,
      "SQLAllocHandle (HENV)");

  retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC2,
      0);
  DIAGRECCHECK(SQL_HANDLE_ENV, henv, 1, SQL_SUCCESS, retcode,
      "SQLSetEnvAttr (SQL_ATTR_ODBC_VERSION)");

  /* --- SetEnvAttr ----------------------------------------------- */
  /* *** SQL_ATTR_CONNECTION_POOLING --------------- *** */
  pConnectPool = SQL_CP_OFF;
  retcode = SQLSetEnvAttr(henv, SQL_ATTR_CONNECTION_POOLING,
      (SQLPOINTER)(uintptr_t)pConnectPool, StringLength);
  DIAGRECCHECK(SQL_HANDLE_ENV, henv, 1, SQL_SUCCESS, retcode,
      "SQLSetEnvAttr (SQL_ATTR_CONNECTION_POOLING)");

  /* *** SQL_ATTR_CP_MATCH ------------------------- *** */
  pCpMatch = SQL_CP_STRICT_MATCH;
  retcode = SQLSetEnvAttr(henv, SQL_ATTR_CP_MATCH,
      (SQLPOINTER)(uintptr_t)pCpMatch, StringLength);
  DIAGRECCHECK(SQL_HANDLE_ENV, henv, 1, SQL_SUCCESS, retcode,
      "SQLSetEnvAttr (SQL_ATTR_CP_MATCH)");

  /* *** SQL_ATTR_ODBC_VERSION --------------------- *** */
  pOdbcVerSet = SQL_OV_ODBC3;
  retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION,
      (SQLPOINTER)(uintptr_t)pOdbcVerSet, StringLength);
  DIAGRECCHECK(SQL_HANDLE_ENV, henv, 1, SQL_SUCCESS, retcode,
      "SQLSetEnvAttr (SQL_ATTR_ODBC_VERSION)");

  /* *** SQL_ATTR_OUTPUT_NTS ----------------------- *** */
  pOutputNts = SQL_TRUE;
  retcode = SQLSetEnvAttr(henv, SQL_ATTR_OUTPUT_NTS,
      (SQLPOINTER)(uintptr_t)pOutputNts, StringLength);
  DIAGRECCHECK(SQL_HANDLE_ENV, henv, 1, SQL_SUCCESS, retcode,
      "SQLSetEnvAttr (SQL_ATTR_OUTPUT_NTS)");

  /* --- GetEnvAttr -------------------------------------------------- */
  /* *** SQL_ATTR_CONNECTION_POOLING --------------- *** */
  retcode = SQLGetEnvAttr(henv, SQL_ATTR_CONNECTION_POOLING, &pConnectPool,
      BufferLength, &StringLengthPtr);
  DIAGRECCHECK(SQL_HANDLE_ENV, henv, 1, SQL_SUCCESS, retcode,
      "SQLGetEnvAttr (SQL_ATTR_CONNECTION_POOLING)");

  /* *** SQL_ATTR_CP_MATCH ------------------------- *** */
  retcode = SQLGetEnvAttr(henv, SQL_ATTR_CP_MATCH, &pCpMatch, BufferLength,
      &StringLengthPtr);
  DIAGRECCHECK(SQL_HANDLE_ENV, henv, 1, SQL_SUCCESS, retcode,
      "SQLGetEnvAttr (SQL_ATTR_CP_MATCH)");

  /* *** SQL_ATTR_ODBC_VERSION --------------------- *** */
  retcode = SQLGetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, &pOdbcVer,
      BufferLength, &StringLengthPtr);
  DIAGRECCHECK(SQL_HANDLE_ENV, henv, 1, SQL_SUCCESS, retcode,
      "SQLGetEnvAttr (SQL_ATTR_ODBC_VERSION)");

  /* *** SQL_ATTR_OUTPUT_NTS ----------------------- *** */
  retcode = SQLGetEnvAttr(henv, SQL_ATTR_OUTPUT_NTS, &pOutputNts,
      BufferLength, &StringLengthPtr);
  DIAGRECCHECK(SQL_HANDLE_ENV, henv, 1, SQL_SUCCESS, retcode,
      "SQLGetEnvAttr (SQL_ATTR_OUTPUT_NTS)");

  /* --- Ouput EntAttributes -------------------------------------- */
  /* *** SQL_ATTR_CONNECTION_POOLING --------------- *** */
  switch (pConnectPool) {
    case (SQL_CP_OFF):
      strcpy(buf, "SQL_CP_OFF");
      break;
    case (SQL_CP_ONE_PER_DRIVER):
      strcpy(buf, "SQL_ONE_PER_DRIVER");
      break;
    case (SQL_CP_ONE_PER_HENV):
      strcpy(buf, "SQL_ONE_PER_HENV");
      break;
    default:
      ADD_FAILURE() << "Unknown value for SQL_ATTR_CONNECTION_POOLING: "
          << pConnectPool;
      break;
  }
  printf("\t SQL_ATTR_CONNECTION_POOLING  : '%s' \r\n", buf);

  /* *** SQL_ATTR_CP_MATCH ------------------------ *** */
  switch (pCpMatch) {
    case (SQL_CP_STRICT_MATCH):
      strcpy(buf, "SQL_CP_STRICT_MATCH");
      break;
    case (SQL_CP_RELAXED_MATCH):
      strcpy(buf, "SQL_CP_RELAXED_MATCH");
      break;
    default:
      ADD_FAILURE() << "Unknown value for SQL_ATTR_CP_MATCH: " << pCpMatch;
      break;
  }
  printf("\t SQL_ATTR_CP_MATCH      : '%s' \r\n", buf);

  /* *** SQL_ATTR_ODBC_VERSION -------------------- *** */
  switch (pOdbcVer) {
    case (SQL_OV_ODBC3):
      strcpy(buf, "SQL_OV_ODBC3");
      break;
    case (SQL_OV_ODBC2):
      strcpy(buf, "SQL_OV_ODBC2");
      break;
    default:
      ADD_FAILURE() << "Unknown value for SQL_ATTR_ODBC_VERSION: " << pOdbcVer;
      break;
  }
  printf("\t SQL_ATTR_ODBC_VERSION    : '%s' \r\n", buf);

  /* *** SQL_ATTR_OUTPUT_NTS ---------------------- *** */
  switch (pOutputNts) {
    case (SQL_TRUE):
      strcpy(buf, "SQL_TRUE");
      break;
    case (SQL_FALSE):
      strcpy(buf, "SQL_FALSE");
      break;
    default:
      ADD_FAILURE() << "Unknown value for SQL_ATTR_OUTPUT_NTS: " << pOutputNts;
      break;
  }
  printf("\t SQL_ATTR_OUTPUT_NTS    : '%s' \r\n", buf);

  /* ----------------------------------------------------------------- */
  retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
  DIAGRECCHECK(SQL_HANDLE_DBC, hdbc, 1, SQL_SUCCESS, retcode,
      "SQLAllocHandle (HDBC)");

  retcode = SQLDriverConnect(hdbc, nullptr, (SQLCHAR*)SNAPPYCONNSTRING.c_str(),
      SQL_NTS, nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);
  DIAGRECCHECK(SQL_HANDLE_DBC, hdbc, 1, SQL_SUCCESS, retcode,
      "SQLDriverConnect");

  retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLAllocHandle (HSTMT)");

  /* --- Disconnect -------------------------------------------------- */
  //free sql handles (stmt, dbc, env)
  FREE_SQLHANDLES
}
