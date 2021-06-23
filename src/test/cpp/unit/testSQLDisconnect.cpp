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

TEST(SQLDisconnect, Disconnect) {
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

  retcode = ::SQLDisconnect(SQL_NULL_HANDLE);
  EXPECT_EQ(SQL_INVALID_HANDLE, retcode)
      << "SQLDisconnect should return INVALID_HANDLE";

  retcode = ::SQLDisconnect(hdbc);
  EXPECT_EQ(SQL_ERROR, retcode) << "SQLDisconnect  should return sql error";

  retcode = SQLDriverConnect(hdbc, nullptr,
      (SQLCHAR*)SNAPPYCONNSTRINGSERVER.c_str(), SQL_NTS, nullptr, 0, nullptr,
      SQL_DRIVER_NOPROMPT);
  EXPECT_EQ(SQL_SUCCESS, retcode) << "SQLDriverConnect call failed";

  retcode = ::SQLDisconnect(hdbc);
  EXPECT_EQ(SQL_SUCCESS, retcode) << "SQLDisconnect  call failed";

  retcode = ::SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
  EXPECT_EQ(SQL_SUCCESS, retcode) << "SQLFreeHandle call failed";

  retcode = ::SQLFreeHandle(SQL_HANDLE_ENV, henv);
  EXPECT_EQ(SQL_SUCCESS, retcode) << "SQLFreeHandle call failed";
}
