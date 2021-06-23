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
 * SnappyDefaults.cpp
 */

#include "SnappyDefaults.h"

extern "C" {
#include <string.h>
}

static const SQLWCHAR* newSQLWCHAR(const char* str) {
  SQLWCHAR* wname = new SQLWCHAR[::strlen(str) + 1];
  SQLWCHAR* pwname = wname;
  while ((*pwname++ = *str++) != 0)
    ;

  return wname;
}

const SQLCHAR SnappyDefaults::DEFAULT_DSN[] = { DEFAULT_DSN_NAME };
// below one is a minor "leak" of global value
const SQLWCHAR* SnappyDefaults::DEFAULT_DSNW = newSQLWCHAR(DEFAULT_DSN_NAME);
const char* SnappyDefaults::ODBC_INI = "ODBC.INI";
const char* SnappyDefaults::ODBCINST_INI = "ODBCINST.INI";

// using the Snappy/Derby default client port
const int SnappyDefaults::DEFAULT_SERVER_PORT = 1527;
