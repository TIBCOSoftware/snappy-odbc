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

TEST(SQLConnect, Connect) {
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

  retcode = ::SQLConnect(SQL_NULL_HANDLE, nullptr, 0, nullptr, 0, nullptr, 0);
  EXPECT_EQ(SQL_INVALID_HANDLE, retcode)
      << "SQLConnect should return invalid handle";

  retcode = ::SQLConnect(hdbc, nullptr, 0, nullptr, 0, nullptr, 0);
  EXPECT_EQ(SQL_ERROR, retcode) << "SQLConnect should return SQL_ERROR";

  const char* dsn = "snappydsn";

  retcode = ::SQLConnect(hdbc, (SQLCHAR*)dsn, 0, nullptr, 0, nullptr, 0);
  EXPECT_EQ(SQL_ERROR, retcode) << "SQLConnect should return SQL_ERROR";

  retcode = ::SQLConnect(hdbc, (SQLCHAR*)dsn, SQL_NTS, nullptr, 0, nullptr, 0);
  DIAGRECCHECK(SQL_HANDLE_DBC, hdbc, 1, SQL_SUCCESS, retcode, "SQLConnect");

  retcode = ::SQLDisconnect(hdbc);
  EXPECT_EQ(SQL_SUCCESS, retcode) << "SQLDisconnect  call failed";

  retcode = ::SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
  EXPECT_EQ(SQL_SUCCESS, retcode) << "SQLFreeHandle call failed";

  retcode = ::SQLFreeHandle(SQL_HANDLE_ENV, henv);
  EXPECT_EQ(SQL_SUCCESS, retcode) << "SQLFreeHandle call failed";
}
