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

// This is a sample test added for debugging purpose
#include "TestHelper.h"

TEST(SQLAllocHandle, BasicAlloc) {
  DECLARE_SQLHANDLES

  retcode = ::SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE,
      SQL_NULL_HANDLE);
  EXPECT_EQ(SQL_ERROR, retcode) << "SQLAllocHandle should return SQL_ERROR";

  retcode = ::SQLAllocHandle(100, SQL_NULL_HANDLE, SQL_NULL_HANDLE);
  EXPECT_TRUE(retcode == SQL_ERROR || retcode == SQL_INVALID_HANDLE)
      << "SQLAllocHandle should return error for invalid handle type";

  retcode = ::SQLAllocHandle(100, SQL_NULL_HANDLE, &henv);
  EXPECT_TRUE(retcode == SQL_ERROR || retcode == SQL_INVALID_HANDLE)
      << "SQLAllocHandle should return error for invalid handle type";
  EXPECT_EQ(SQL_NULL_HANDLE, henv)
      << "SQLAllocHandle should return nullptr handle";

  retcode = ::SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
  EXPECT_EQ(SQL_SUCCESS, retcode) << "SQLAllocHandle call failed";
  EXPECT_NE(nullptr, henv)
      << "SQLAllocHandle failed to return valid env handle";

  SQLHENV henv1 = nullptr;
  retcode = ::SQLAllocHandle(SQL_HANDLE_ENV, henv, &henv1);
  EXPECT_EQ(SQL_INVALID_HANDLE, retcode)
      << "SQLAllocHandle should return SQL_INVALID_HANDLE";
  EXPECT_EQ(SQL_NULL_HANDLE, henv1)
      << "SQLAllocHandle should set HENV handle to nullptr handle";

  retcode = ::SQLAllocHandle(SQL_HANDLE_DBC, SQL_NULL_HANDLE,
      SQL_NULL_HANDLE);
  EXPECT_EQ(SQL_INVALID_HANDLE, retcode)
      << "SQLAllocHandle should return SQL_INVALID_HANDLE";

  retcode = ::SQLAllocHandle(SQL_HANDLE_DBC, SQL_NULL_HANDLE, &hdbc);
  EXPECT_EQ(SQL_INVALID_HANDLE, retcode)
      << "SQLAllocHandle should return SQL_INVALID_HANDLE";

  retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
  DIAGRECCHECK(SQL_HANDLE_ENV, henv, 1, SQL_SUCCESS, retcode,
      "SQLSetEnvAttr (HENV)");

  retcode = ::SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
  EXPECT_NE(SQL_ERROR, retcode) << "SQLAllocHandle returned SQL_ERROR";
  EXPECT_NE(nullptr, hdbc)
      << "SQLAllocHandle failed to return valid DBC handle";

  retcode = ::SQLAllocHandle(SQL_HANDLE_STMT, SQL_NULL_HANDLE,
      SQL_NULL_HANDLE);
  EXPECT_EQ(SQL_INVALID_HANDLE, retcode)
      << "SQLAllocHandle should return SQL_INVALID_HANDLE";

  retcode = ::SQLAllocHandle(SQL_HANDLE_STMT, SQL_NULL_HANDLE, &hstmt);
  EXPECT_EQ(SQL_INVALID_HANDLE, retcode)
      << "SQLAllocHandle should return SQL_INVALID_HANDLE";

  /*retcode = ::SQLAllocHandle(SQL_HANDLE_STMT, henv, &hstmt);
   EXPECT_EQ(SQL_SUCCESS, retcode) << "SQLAllocHandle returned SQL_ERROR";
   EXPECT_NE(nullptr, hstmt)
       << "SQLAllocHandle failed to return valid env handle";*/

  retcode = ::SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
  EXPECT_EQ(SQL_ERROR, retcode)
      << "SQLAllocHandle should return SQL_ERROR as connection is not created";
  retcode = ::SQLAllocHandle(SQL_HANDLE_DESC, SQL_NULL_HANDLE, &hdesc);
  EXPECT_EQ(SQL_INVALID_HANDLE, retcode)
      << "SQLAllocHandle should return SQL_INVALID_HANDLE for null handle";
  EXPECT_EQ(nullptr, hdesc) << "SQLAllocHandle should return nullptr handle";

  retcode = ::SQLAllocHandle(SQL_HANDLE_DESC, hstmt, &hdesc);
  EXPECT_EQ(SQL_INVALID_HANDLE, retcode)
      << "SQLAllocHandle should return SQL_INVALID_HANDLE for null handle";
  EXPECT_EQ(nullptr, hdesc) << "SQLAllocHandle should return nullptr handle";

  retcode = SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
  DIAGRECCHECK(SQL_HANDLE_DBC, hdbc, 1, SQL_SUCCESS, retcode,
      "SQLFreeHandle (HDBC)");
  retcode = SQLFreeHandle(SQL_HANDLE_ENV, henv);
  DIAGRECCHECK(SQL_HANDLE_ENV, henv, 1, SQL_SUCCESS, retcode,
      "SQLFreeHandle (HENV)");
}

//test for testing multiple env handles
TEST(SQLAllocHandle, MultipleEnv) {
  DECLARE_SQLHANDLES

  SQLHDBC hdbc1;
  SQLHENV henv1;
  SQLHSTMT hstmt1;

  retcode = SQLAllocHandle(SQL_HANDLE_ENV, nullptr, &henv);
  DIAGRECCHECK(SQL_HANDLE_ENV, henv, 1, SQL_SUCCESS, retcode,
      "SQLAllocHandle (HENV)");

  retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3,
      0);
  DIAGRECCHECK(SQL_HANDLE_ENV, henv, 1, SQL_SUCCESS, retcode,
      "SQLSetEnvAttr (HENV)")

  retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
  DIAGRECCHECK(SQL_HANDLE_DBC, hdbc, 1, SQL_SUCCESS, retcode,
      "SQLAllocHandle (HDBC)");

  retcode = SQLDriverConnect(hdbc, nullptr, (SQLCHAR*)SNAPPYCONNSTRING.c_str(),
      SQL_NTS, nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);
  DIAGRECCHECK(SQL_HANDLE_DBC, hdbc, 1, SQL_SUCCESS, retcode,
      "SQLDriverConnect");

  retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLAllocHandle (HSTMT)")

  retcode = SQLAllocHandle(SQL_HANDLE_ENV, nullptr, &henv1);
  DIAGRECCHECK(SQL_HANDLE_ENV, henv1, 1, SQL_SUCCESS, retcode,
      "SQLAllocHandle (HENV)");

  retcode = SQLSetEnvAttr(henv1, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3,
      0);
  DIAGRECCHECK(SQL_HANDLE_ENV, henv1, 1, SQL_SUCCESS, retcode,
      "SQLSetEnvAttr (HENV)")

  retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv1, &hdbc1);
  DIAGRECCHECK(SQL_HANDLE_DBC, hdbc, 1, SQL_SUCCESS, retcode,
      "SQLAllocHandle (HDBC)");

  retcode = SQLDriverConnect(hdbc1, nullptr, (SQLCHAR*)SNAPPYCONNSTRING.c_str(),
      SQL_NTS, nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);
  DIAGRECCHECK(SQL_HANDLE_DBC, hdbc1, 1, SQL_SUCCESS, retcode,
      "SQLDriverConnect");

  retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc1, &hstmt1);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt1, 1, SQL_SUCCESS, retcode,
      "SQLAllocHandle (HSTMT)");

  retcode = SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLFreeHandle (HSTMT)");

  retcode = SQLDisconnect(hdbc);
  DIAGRECCHECK(SQL_HANDLE_DBC, hdbc, 1, SQL_SUCCESS, retcode, "SQLDisconnect");

  retcode = SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
  DIAGRECCHECK(SQL_HANDLE_DBC, hdbc, 1, SQL_SUCCESS, retcode,
      "SQLFreeHandle (HDBC)");

  retcode = SQLFreeHandle(SQL_HANDLE_ENV, henv);
  DIAGRECCHECK(SQL_HANDLE_ENV, henv, 1, SQL_SUCCESS, retcode,
      "SQLFreeHandle (HENV)");

  retcode = SQLFreeHandle(SQL_HANDLE_STMT, hstmt1);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt1, 1, SQL_SUCCESS, retcode,
      "SQLFreeHandle (HSTMT)");

  retcode = SQLDisconnect(hdbc1);
  DIAGRECCHECK(SQL_HANDLE_DBC, hdbc1, 1, SQL_SUCCESS, retcode,
      "SQLDisconnect");

  retcode = SQLFreeHandle(SQL_HANDLE_DBC, hdbc1);
  DIAGRECCHECK(SQL_HANDLE_DBC, hdbc1, 1, SQL_SUCCESS, retcode,
      "SQLFreeHandle (HDBC)");

  retcode = SQLFreeHandle(SQL_HANDLE_ENV, henv1);
  DIAGRECCHECK(SQL_HANDLE_ENV, henv1, 1, SQL_SUCCESS, retcode,
      "SQLFreeHandle (HENV)");
}
