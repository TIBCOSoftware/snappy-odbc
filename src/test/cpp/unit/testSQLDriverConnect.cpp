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

TEST(SQLDriverConnect, Connect) {
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

  retcode = ::SQLDriverConnect(nullptr, nullptr, nullptr, 0, nullptr, 0,
      nullptr, 0);
  EXPECT_EQ(SQL_INVALID_HANDLE, retcode)
      << "SQLDriverConnect should return invalid handle";

  retcode = ::SQLDriverConnect(nullptr, nullptr, nullptr, 0, nullptr, 0,
      nullptr, SQL_DRIVER_PROMPT);
  EXPECT_EQ(SQL_INVALID_HANDLE, retcode)
      << "SQLDriverConnect should return invalid handle";

  retcode = ::SQLDriverConnect(hdbc, nullptr, (SQLCHAR*)"InvalidConnectionString",
      (SQLSMALLINT)strlen("InvalidConnectionString"), nullptr, 0, nullptr,
      SQL_DRIVER_NOPROMPT);
  EXPECT_EQ(SQL_ERROR, retcode) << "SQLDriverConnect should return sql error";
  retcode = ::SQLDriverConnect(hdbc, nullptr, (SQLCHAR*)"InvalidKey=Value",
      (SQLSMALLINT)strlen("InvalidKey=Value"), nullptr, 0, nullptr,
      SQL_DRIVER_NOPROMPT);
  EXPECT_EQ(SQL_ERROR, retcode) << "SQLDriverConnect should return sql error";
  retcode = ::SQLDriverConnect(hdbc, nullptr, (SQLCHAR*)"DSN=Invalid",
      (SQLSMALLINT)strlen("DSN=Invalid"), nullptr, 0, nullptr,
      SQL_DRIVER_NOPROMPT);
  EXPECT_EQ(SQL_ERROR, retcode) << "SQLDriverConnect should return sql error";

  retcode = ::SQLDriverConnect(hdbc, nullptr, (SQLCHAR*)"DSN=",
      (SQLSMALLINT)strlen("DSN="), nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);
  EXPECT_EQ(SQL_ERROR, retcode) << "SQLDriverConnect should return sql error";

  retcode = ::SQLDriverConnect(hdbc, nullptr, (SQLCHAR*)"DSN=DEFAULT",
      (SQLSMALLINT)strlen("DSN=DEFAULT"), nullptr, 0, nullptr,
      SQL_DRIVER_NOPROMPT);
  EXPECT_EQ(SQL_ERROR, retcode) << "SQLDriverConnect should return sql error";

  SQLCHAR outConnStr[255];
  SQLSMALLINT outConnStrLen = 0;
  retcode = ::SQLDriverConnect(hdbc, nullptr,
      (SQLCHAR*)SNAPPYCONNSTRINGSERVER.c_str(), SQL_NTS, outConnStr, 255,
      &outConnStrLen, SQL_DRIVER_NOPROMPT);
  DIAGRECCHECK(SQL_HANDLE_DBC, hdbc, 1, SQL_SUCCESS, retcode,
      "SQLDriverConnect");
  LOGF("\nout conn str is %s and length is %d", outConnStr, outConnStrLen);
  EXPECT_EQ(outConnStrLen, (SQLSMALLINT)strlen((char*)outConnStr))
      << "OutConnStrLen and Actual length should match";
  EXPECT_EQ(SNAPPYCONNSTRINGNODRIVER, std::string((char*)outConnStr))
      << "Returned output connection string sould match";

  // SQLDisconnect is required before free as per ODBC spec.
  retcode = ::SQLDisconnect(hdbc);
  EXPECT_EQ(SQL_SUCCESS, retcode) << "SQLDisconnect call failed";

  retcode = ::SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
  EXPECT_EQ(SQL_SUCCESS, retcode) << "SQLFreeHandle call failed";

  retcode = ::SQLFreeHandle(SQL_HANDLE_ENV, henv);
  EXPECT_EQ(SQL_SUCCESS, retcode) << "SQLFreeHandle call failed";
}
