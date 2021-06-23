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

#define FUNCTIONS 100

#define CHECK_UNSUPPORTEDAPI(API) retcode = ::SQLGetFunctions(hdbc, API, &isSupported); \
  EXPECT_NE(SQL_ERROR, retcode) << "SQLGetFunctions returned SQL_ERROR"; \
  EXPECT_NE(SQL_TRUE, isSupported) << "API "#API" should not be supported!"

#define CHECK_SUPPORTEDAPI(API)  retcode = ::SQLGetFunctions(hdbc, API, &isSupported); \
  EXPECT_NE(SQL_ERROR, retcode) << "SQLGetFunctions returned SQL_ERROR"; \
  EXPECT_NE(SQL_FALSE, isSupported) << "API "#API" is not supported"

#define CHECK_ODBC2_SUPPORTED_API(API)   EXPECT_EQ(SQL_TRUE, fExists[API]) \
                                            << "API "#API" is not supported"
#define CHECK_ODBC2_UNSUPPORTED_API(API) EXPECT_NE(SQL_TRUE, fExists[API]) \
                                            << "API "#API" is supported"
#define CHECK_ODBC3_SUPPORTED_API(API)   EXPECT_EQ(SQL_TRUE, SQL_FUNC_EXISTS(fExists,API)) \
                                            << "API"#API"is not supported"
#define CHECK_ODBC3_UNSUPPORTED_API(API) EXPECT_NE(SQL_TRUE, SQL_FUNC_EXISTS(fExists,API)) \
                                            << "API"#API"is supported"

//*-------------------------------------------------------------------------

#define TESTNAME "SQLGetFunctions"

#define TABLE ""
#define SQLSTMT1 "SELECT * FROM DUAL"
#define MAX_NAME_LEN 256

#define STR_LEN 128+1
#define REM_LEN 254+1

#define NULL_VALUE "<NULL>"

//*-------------------------------------------------------------------------

void CheckEachAPI(SQLHDBC hdbc) {
  SQLRETURN retcode = SQL_SUCCESS;
  SQLUSMALLINT isSupported = SQL_FALSE;

  CHECK_SUPPORTEDAPI(SQL_API_SQLALLOCHANDLE);
  CHECK_SUPPORTEDAPI(SQL_API_SQLALLOCSTMT);
  CHECK_SUPPORTEDAPI(SQL_API_SQLBINDCOL);
  CHECK_SUPPORTEDAPI(SQL_API_SQLBINDPARAMETER);
  CHECK_SUPPORTEDAPI(SQL_API_SQLCLOSECURSOR);
  CHECK_SUPPORTEDAPI(SQL_API_SQLCOLATTRIBUTE);
  CHECK_SUPPORTEDAPI(SQL_API_SQLCOLUMNS);
  CHECK_SUPPORTEDAPI(SQL_API_SQLCONNECT);
  CHECK_SUPPORTEDAPI(SQL_API_SQLDISCONNECT);
  CHECK_SUPPORTEDAPI(SQL_API_SQLENDTRAN);
  CHECK_SUPPORTEDAPI(SQL_API_SQLEXECDIRECT);
  CHECK_SUPPORTEDAPI(SQL_API_SQLEXECUTE);
  CHECK_SUPPORTEDAPI(SQL_API_SQLFETCH);
  CHECK_SUPPORTEDAPI(SQL_API_SQLFETCHSCROLL);
  CHECK_SUPPORTEDAPI(SQL_API_SQLFREEHANDLE);
  CHECK_SUPPORTEDAPI(SQL_API_SQLFREESTMT);
  CHECK_SUPPORTEDAPI(SQL_API_SQLGETCONNECTATTR);
  CHECK_SUPPORTEDAPI(SQL_API_SQLGETCURSORNAME);
  CHECK_SUPPORTEDAPI(SQL_API_SQLGETDATA);
  CHECK_SUPPORTEDAPI(SQL_API_SQLGETDIAGFIELD);
  CHECK_SUPPORTEDAPI(SQL_API_SQLGETDIAGREC);
  CHECK_SUPPORTEDAPI(SQL_API_SQLGETENVATTR);
  CHECK_SUPPORTEDAPI(SQL_API_SQLGETFUNCTIONS);
  CHECK_SUPPORTEDAPI(SQL_API_SQLGETINFO);
  CHECK_SUPPORTEDAPI(SQL_API_SQLGETSTMTATTR);
  CHECK_SUPPORTEDAPI(SQL_API_SQLGETTYPEINFO);
  CHECK_SUPPORTEDAPI(SQL_API_SQLNUMRESULTCOLS);
  CHECK_SUPPORTEDAPI(SQL_API_SQLPARAMDATA);
  CHECK_SUPPORTEDAPI(SQL_API_SQLPREPARE);
  CHECK_SUPPORTEDAPI(SQL_API_SQLPUTDATA);
  CHECK_SUPPORTEDAPI(SQL_API_SQLROWCOUNT);
  CHECK_SUPPORTEDAPI(SQL_API_SQLSETCONNECTATTR);
  CHECK_SUPPORTEDAPI(SQL_API_SQLSETCURSORNAME);
  CHECK_SUPPORTEDAPI(SQL_API_SQLSETENVATTR);
  CHECK_SUPPORTEDAPI(SQL_API_SQLSETSTMTATTR);
  CHECK_SUPPORTEDAPI(SQL_API_SQLSPECIALCOLUMNS);
  CHECK_SUPPORTEDAPI(SQL_API_SQLSTATISTICS);
  CHECK_SUPPORTEDAPI(SQL_API_SQLTABLES);
  CHECK_SUPPORTEDAPI(SQL_API_SQLBULKOPERATIONS);
  CHECK_SUPPORTEDAPI(SQL_API_SQLCOLUMNPRIVILEGES);
  CHECK_SUPPORTEDAPI(SQL_API_SQLDRIVERCONNECT);
  CHECK_SUPPORTEDAPI(SQL_API_SQLFOREIGNKEYS);
  CHECK_SUPPORTEDAPI(SQL_API_SQLMORERESULTS);
  CHECK_SUPPORTEDAPI(SQL_API_SQLNATIVESQL);
  CHECK_SUPPORTEDAPI(SQL_API_SQLNUMPARAMS);
  CHECK_SUPPORTEDAPI(SQL_API_SQLPRIMARYKEYS);
  CHECK_SUPPORTEDAPI(SQL_API_SQLPROCEDURECOLUMNS);
  CHECK_SUPPORTEDAPI(SQL_API_SQLPROCEDURES);
  CHECK_SUPPORTEDAPI(SQL_API_SQLSETPOS);
  CHECK_SUPPORTEDAPI(SQL_API_SQLTABLEPRIVILEGES);
  CHECK_SUPPORTEDAPI(SQL_API_SQLDESCRIBECOL);
  CHECK_SUPPORTEDAPI(SQL_API_SQLDESCRIBEPARAM);

  CHECK_UNSUPPORTEDAPI(SQL_API_SQLBROWSECONNECT);
  CHECK_UNSUPPORTEDAPI(SQL_API_SQLCANCEL);
  CHECK_UNSUPPORTEDAPI(SQL_API_SQLCOPYDESC);
  CHECK_UNSUPPORTEDAPI(SQL_API_SQLGETDESCFIELD);
  CHECK_UNSUPPORTEDAPI(SQL_API_SQLGETDESCREC);
  CHECK_UNSUPPORTEDAPI(SQL_API_SQLSETDESCFIELD);
  CHECK_UNSUPPORTEDAPI(SQL_API_SQLSETDESCREC);
  CHECK_UNSUPPORTEDAPI(SQL_API_SQLEXTENDEDFETCH);
  // unsupported APIs by driver but provided by driver manager
#ifdef TEST_DIRECT
  CHECK_UNSUPPORTEDAPI(SQL_API_SQLALLOCHANDLESTD);
  CHECK_UNSUPPORTEDAPI(SQL_API_SQLALLOCCONNECT);
  CHECK_UNSUPPORTEDAPI(SQL_API_SQLALLOCENV);
  CHECK_UNSUPPORTEDAPI(SQL_API_SQLDATASOURCES);
  CHECK_UNSUPPORTEDAPI(SQL_API_SQLBINDPARAM);
  CHECK_UNSUPPORTEDAPI(SQL_API_SQLERROR);
  CHECK_UNSUPPORTEDAPI(SQL_API_SQLFREECONNECT);
  CHECK_UNSUPPORTEDAPI(SQL_API_SQLFREEENV);
  CHECK_UNSUPPORTEDAPI(SQL_API_SQLGETCONNECTOPTION);
  CHECK_UNSUPPORTEDAPI(SQL_API_SQLGETSTMTOPTION);
  CHECK_UNSUPPORTEDAPI(SQL_API_SQLSETCONNECTOPTION);
  CHECK_UNSUPPORTEDAPI(SQL_API_SQLSETPARAM);
  CHECK_UNSUPPORTEDAPI(SQL_API_SQLTRANSACT);
  CHECK_UNSUPPORTEDAPI(SQL_API_SQLSETSTMTOPTION);
  CHECK_UNSUPPORTEDAPI(SQL_API_SQLDRIVERS);
  CHECK_UNSUPPORTEDAPI(SQL_API_SQLPARAMOPTIONS);
  CHECK_UNSUPPORTEDAPI(SQL_API_SQLSETSCROLLOPTIONS);
#else
  CHECK_SUPPORTEDAPI(SQL_API_SQLALLOCHANDLESTD);
  CHECK_SUPPORTEDAPI(SQL_API_SQLALLOCCONNECT);
  CHECK_SUPPORTEDAPI(SQL_API_SQLALLOCENV);
  CHECK_SUPPORTEDAPI(SQL_API_SQLDATASOURCES);
  CHECK_SUPPORTEDAPI(SQL_API_SQLBINDPARAM);
  CHECK_SUPPORTEDAPI(SQL_API_SQLERROR);
  CHECK_SUPPORTEDAPI(SQL_API_SQLFREECONNECT);
  CHECK_SUPPORTEDAPI(SQL_API_SQLFREEENV);
  CHECK_SUPPORTEDAPI(SQL_API_SQLGETCONNECTOPTION);
  CHECK_SUPPORTEDAPI(SQL_API_SQLGETSTMTOPTION);
  CHECK_SUPPORTEDAPI(SQL_API_SQLSETCONNECTOPTION);
  CHECK_SUPPORTEDAPI(SQL_API_SQLSETPARAM);
  CHECK_SUPPORTEDAPI(SQL_API_SQLTRANSACT);
  CHECK_SUPPORTEDAPI(SQL_API_SQLSETSTMTOPTION);
  CHECK_SUPPORTEDAPI(SQL_API_SQLDRIVERS);
  CHECK_SUPPORTEDAPI(SQL_API_SQLPARAMOPTIONS);
  // comes as unsupported on windows driver manager for some reason
#ifdef _WINDOWS
  CHECK_UNSUPPORTEDAPI(SQL_API_SQLSETSCROLLOPTIONS);
#else
  CHECK_SUPPORTEDAPI(SQL_API_SQLSETSCROLLOPTIONS);
#endif
#endif
}

void CheckODBC2APIS(SQLHDBC hdbc) {
  SQLUSMALLINT fExists[FUNCTIONS];
  SQLRETURN retcode = SQL_SUCCESS;

  retcode = SQLGetFunctions(hdbc, SQL_API_ALL_FUNCTIONS, fExists);
  EXPECT_NE(SQL_ERROR, retcode) << "SQLGetFunctions returned SQL_ERROR";

  // supported funstions
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLALLOCSTMT);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLBINDCOL);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLCOLUMNS);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLCONNECT);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLDISCONNECT);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLEXECDIRECT);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLEXECUTE);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLFETCH);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLFREESTMT);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLGETCURSORNAME);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLGETDATA);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLGETFUNCTIONS);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLGETINFO);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLGETTYPEINFO);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLNUMRESULTCOLS);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLPARAMDATA);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLPREPARE);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLPUTDATA);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLROWCOUNT);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLSETCURSORNAME);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLSPECIALCOLUMNS);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLSTATISTICS);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLTABLES);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLBINDPARAMETER);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLCOLUMNPRIVILEGES);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLDRIVERCONNECT);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLFOREIGNKEYS);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLMORERESULTS);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLNATIVESQL);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLNUMPARAMS);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLPRIMARYKEYS);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLPROCEDURECOLUMNS);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLPROCEDURES);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLSETPOS);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLTABLEPRIVILEGES);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLDESCRIBECOL);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLDESCRIBEPARAM);

  CHECK_ODBC2_UNSUPPORTED_API(SQL_API_SQLBROWSECONNECT);
  CHECK_ODBC2_UNSUPPORTED_API(SQL_API_SQLCANCEL);
  CHECK_ODBC2_UNSUPPORTED_API(SQL_API_SQLEXTENDEDFETCH);
  // unsupported APIs by driver but provided by driver manager
#ifdef TEST_DIRECT
  CHECK_ODBC2_UNSUPPORTED_API(SQL_API_SQLALLOCCONNECT);
  CHECK_ODBC2_UNSUPPORTED_API(SQL_API_SQLALLOCENV);
  CHECK_ODBC2_UNSUPPORTED_API(SQL_API_SQLERROR);
  CHECK_ODBC2_UNSUPPORTED_API(SQL_API_SQLDATASOURCES);
  CHECK_ODBC2_UNSUPPORTED_API(SQL_API_SQLFREECONNECT);
  CHECK_ODBC2_UNSUPPORTED_API(SQL_API_SQLFREEENV);
  CHECK_ODBC2_UNSUPPORTED_API(SQL_API_SQLGETCONNECTOPTION);
  CHECK_ODBC2_UNSUPPORTED_API(SQL_API_SQLGETSTMTOPTION);
  CHECK_ODBC2_UNSUPPORTED_API(SQL_API_SQLSETCONNECTOPTION);
  CHECK_ODBC2_UNSUPPORTED_API(SQL_API_SQLTRANSACT);
  CHECK_ODBC2_UNSUPPORTED_API(SQL_API_SQLSETPARAM);
  CHECK_ODBC2_UNSUPPORTED_API(SQL_API_SQLSETSTMTOPTION);
  CHECK_ODBC2_UNSUPPORTED_API(SQL_API_SQLSETSCROLLOPTIONS);
  CHECK_ODBC2_UNSUPPORTED_API(SQL_API_SQLDRIVERS);
  CHECK_ODBC2_UNSUPPORTED_API(SQL_API_SQLPARAMOPTIONS);
#else
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLALLOCCONNECT);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLALLOCENV);
  // comes as unsupported on windows driver manager for some reason
#ifdef _WINDOWS
  CHECK_ODBC2_UNSUPPORTED_API(SQL_API_SQLSETSCROLLOPTIONS);
#else
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLSETSCROLLOPTIONS);
#endif
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLERROR);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLDATASOURCES);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLFREECONNECT);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLFREEENV);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLGETCONNECTOPTION);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLGETSTMTOPTION);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLSETCONNECTOPTION);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLTRANSACT);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLSETPARAM);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLSETSTMTOPTION);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLDRIVERS);
  CHECK_ODBC2_SUPPORTED_API(SQL_API_SQLPARAMOPTIONS);
#endif
}

void CheckODBC3APIS(SQLHDBC hdbc) {
  SQLUSMALLINT fExists[SQL_API_ODBC3_ALL_FUNCTIONS_SIZE];
  SQLRETURN retcode = SQL_SUCCESS;

  retcode = SQLGetFunctions(hdbc, SQL_API_ODBC3_ALL_FUNCTIONS, fExists);
  EXPECT_NE(SQL_ERROR, retcode) << "SQLGetFunctions returned SQL_ERROR";

  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLALLOCHANDLE);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLALLOCSTMT);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLBINDCOL);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLBINDPARAMETER);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLCLOSECURSOR);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLCOLATTRIBUTE);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLCOLUMNS);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLCONNECT);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLDISCONNECT);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLENDTRAN);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLEXECDIRECT);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLEXECUTE);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLFETCH);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLFETCHSCROLL);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLFREEHANDLE);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLFREESTMT);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLGETCONNECTATTR);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLGETCURSORNAME);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLGETDATA);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLGETDIAGFIELD);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLGETDIAGREC);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLGETENVATTR);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLGETFUNCTIONS);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLGETINFO);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLGETSTMTATTR);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLGETTYPEINFO);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLNUMRESULTCOLS);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLPARAMDATA);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLPREPARE);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLPUTDATA);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLROWCOUNT);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLSETCONNECTATTR);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLSETCURSORNAME);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLSETENVATTR);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLSETSTMTATTR);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLSPECIALCOLUMNS);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLSTATISTICS);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLTABLES);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLBULKOPERATIONS);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLBINDPARAMETER);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLCOLUMNPRIVILEGES);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLDRIVERCONNECT);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLFOREIGNKEYS);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLMORERESULTS);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLNATIVESQL);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLNUMPARAMS);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLPRIMARYKEYS);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLPROCEDURECOLUMNS);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLPROCEDURES);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLSETPOS);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLTABLEPRIVILEGES);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLDESCRIBECOL);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLDESCRIBEPARAM);

  CHECK_ODBC3_UNSUPPORTED_API(SQL_API_SQLBROWSECONNECT);
  CHECK_ODBC3_UNSUPPORTED_API(SQL_API_SQLCANCEL);
  CHECK_ODBC3_UNSUPPORTED_API(SQL_API_SQLCOPYDESC);
  CHECK_ODBC3_UNSUPPORTED_API(SQL_API_SQLGETDESCFIELD);
  CHECK_ODBC3_UNSUPPORTED_API(SQL_API_SQLGETDESCREC);
  CHECK_ODBC3_UNSUPPORTED_API(SQL_API_SQLSETDESCFIELD);
  CHECK_ODBC3_UNSUPPORTED_API(SQL_API_SQLSETDESCREC);
  CHECK_ODBC3_UNSUPPORTED_API(SQL_API_SQLEXTENDEDFETCH);
  // unsupported APIs by driver but provided by driver manager
#ifdef TEST_DIRECT
  CHECK_ODBC3_UNSUPPORTED_API(SQL_API_SQLALLOCHANDLESTD);
  CHECK_ODBC3_UNSUPPORTED_API(SQL_API_SQLALLOCCONNECT);
  CHECK_ODBC3_UNSUPPORTED_API(SQL_API_SQLALLOCENV);
  CHECK_ODBC3_UNSUPPORTED_API(SQL_API_SQLDATASOURCES);
  CHECK_ODBC3_UNSUPPORTED_API(SQL_API_SQLBINDPARAM);
  CHECK_ODBC3_UNSUPPORTED_API(SQL_API_SQLERROR);
  CHECK_ODBC3_UNSUPPORTED_API(SQL_API_SQLFREECONNECT);
  CHECK_ODBC3_UNSUPPORTED_API(SQL_API_SQLFREEENV);
  CHECK_ODBC3_UNSUPPORTED_API(SQL_API_SQLGETCONNECTOPTION);
  CHECK_ODBC3_UNSUPPORTED_API(SQL_API_SQLGETSTMTOPTION);
  CHECK_ODBC3_UNSUPPORTED_API(SQL_API_SQLSETCONNECTOPTION);
  CHECK_ODBC3_UNSUPPORTED_API(SQL_API_SQLSETPARAM);
  CHECK_ODBC3_UNSUPPORTED_API(SQL_API_SQLTRANSACT);
  CHECK_ODBC3_UNSUPPORTED_API(SQL_API_SQLSETSTMTOPTION);
  CHECK_ODBC3_UNSUPPORTED_API(SQL_API_SQLDRIVERS);
  CHECK_ODBC3_UNSUPPORTED_API(SQL_API_SQLPARAMOPTIONS);
#else
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLALLOCHANDLESTD);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLALLOCCONNECT);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLALLOCENV);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLDATASOURCES);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLBINDPARAM);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLERROR);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLFREECONNECT);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLFREEENV);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLGETCONNECTOPTION);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLGETSTMTOPTION);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLSETCONNECTOPTION);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLSETPARAM);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLTRANSACT);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLSETSTMTOPTION);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLDRIVERS);
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLPARAMOPTIONS);
#ifdef _WINDOWS
  // shows unsupported with Windows driver manager (bug?)
  CHECK_ODBC3_UNSUPPORTED_API(SQL_API_SQLSETSCROLLOPTIONS);
#else
  CHECK_ODBC3_SUPPORTED_API(SQL_API_SQLSETSCROLLOPTIONS);
#endif

#endif
}

TEST(SQLGetFunctions, CheckAPISupport) {
  DECLARE_SQLHANDLES

  retcode = ::SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
  EXPECT_NE(SQL_ERROR, retcode) << "SQLAllocHandle returned SQL_ERROR";
  EXPECT_NE(nullptr, henv)
      << "SQLAllocHandle failed to return valid env handle";

  retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
  DIAGRECCHECK(SQL_HANDLE_ENV, henv, 1, SQL_SUCCESS, retcode,
      "SQLSetEnvAttr (HENV)");

  retcode = ::SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
  EXPECT_NE(SQL_ERROR, retcode) << "SQLAllocHandle returned SQL_ERROR";
  EXPECT_NE(nullptr, henv)
      << "SQLAllocHandle failed to return valid dbc handle";

  // create database connection
  retcode = SQLDriverConnect(hdbc, nullptr, (SQLCHAR*)SNAPPYCONNSTRING.c_str(),
      SQL_NTS, nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);
  EXPECT_EQ(SQL_SUCCESS, retcode) << "SQLDriverConnect call failed";

  CheckEachAPI(hdbc);
  CheckODBC2APIS(hdbc);
  CheckODBC3APIS(hdbc);

  retcode = ::SQLDisconnect(hdbc);
  EXPECT_NE(SQL_ERROR, retcode) << "SQLDisconnect returned SQL_ERROR";

  retcode = ::SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
  EXPECT_NE(SQL_ERROR, retcode) << "SQLFreeHandle returned SQL_ERROR";

  retcode = ::SQLFreeHandle(SQL_HANDLE_ENV, henv);
  EXPECT_NE(SQL_ERROR, retcode) << "SQLFreeHandle returned SQL_ERROR";
}

TEST(SQLGetFunctions, CheckAPITypes) {
  DECLARE_SQLHANDLES

  /* ------------------------------------------------------------------------- */
  SQLUSMALLINT fFunction;
  SQLUSMALLINT pfExists[100];

  CHAR buffer[MAX_NAME_LEN];

  /* ------------------------------------------------------------------------- */
  // This test will assume that the ODBC handles passed in
  //              are nullptr.  One could have this function do a connection
  //              and pass the handles to other test functions.
  /* - Connect ------------------------------------------------------- */

  //initialize the SQL handles and connect
  INIT_SQLHANDLES

  /* ----------------------------------------------------------------- */

  /* --- SQLGetFunctions ----------------------------------------------------- */

  fFunction = SQL_API_ALL_FUNCTIONS;
  retcode = SQLGetFunctions(hdbc, fFunction, pfExists);
  DIAGRECCHECK(SQL_HANDLE_DBC, hdbc, 1, SQL_SUCCESS, retcode,
      "SQLGetFunctions");

  /* --- Output -------------------------------------------------------------- */

  LOGF("Conformance Core --> ");

  /* --- Core Conformance -------------------------------------------- */
  Get_BoolValue((SWORD)pfExists[SQL_API_SQLALLOCCONNECT], buffer);
  LOGF("SQL_API_SQLALLOCCONNECT = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLALLOCENV], buffer);
  LOGF("SQL_API_SQLALLOCENV = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLALLOCSTMT], buffer);
  LOGF("SQL_API_SQLALLOCSTMT = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLBINDCOL], buffer);
  LOGF("SQL_API_SQLBINDCOL = '%s'", buffer);

  /* SW:
  Get_BoolValue((SWORD)pfExists[SQL_API_SQLCANCEL], buffer);
  LOGF("SQL_API_SQLCANCEL = '%s'", buffer);
  */

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLCOLATTRIBUTES], buffer);
  LOGF("SQL_API_SQLCOLATTRIBUTES = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLCONNECT], buffer);
  LOGF("SQL_API_SQLCONNECT = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLDESCRIBECOL], buffer);
  LOGF("SQL_API_SQLDESCRIBECOL = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLDISCONNECT], buffer);
  LOGF("SQL_API_SQLDISCONNECT = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLERROR], buffer);
  LOGF("SQL_API_SQLERROR = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLEXECDIRECT], buffer);
  LOGF("SQL_API_SQLEXECDIRECT = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLFETCH], buffer);
  LOGF("SQL_API_SQLFETCH = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLFREECONNECT], buffer);
  LOGF("SQL_API_SQLFREECONNECT = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLFREEENV], buffer);
  LOGF("SQL_API_SQLFREEENV = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLFREESTMT], buffer);
  LOGF("SQL_API_SQLFREESTMT = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLGETCURSORNAME], buffer);
  LOGF("SQL_API_SQLGETCURSORNAME = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLNUMRESULTCOLS], buffer);
  LOGF("SQL_API_SQLNUMRESULTCOLS = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLPREPARE], buffer);
  LOGF("SQL_API_SQLPREPARE = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLROWCOUNT], buffer);
  LOGF("SQL_API_SQLROWCOUNT = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLSETCURSORNAME], buffer);
  LOGF("SQL_API_SQLSETCURSORNAME = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLSETPARAM], buffer);
  LOGF("SQL_API_SQLSETPARAM = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLTRANSACT], buffer);
  LOGF("SQL_API_SQLTRANSACT = '%s'", buffer);

  /* ----------------------------------------------------------------- */
  LOG("\tConformance Level 1. --> ");

  /* --- Level 1 Conformance ----------------------------------------- */
  Get_BoolValue((SWORD)pfExists[SQL_API_SQLBINDPARAMETER], buffer);
  LOGF("SQL_API_SQLBINDPARAMETER = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLCOLUMNS], buffer);
  LOGF("SQL_API_SQLCOLUMNS = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLDRIVERCONNECT], buffer);
  LOGF("SQL_API_SQLDRIVERCONNECT = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLGETCONNECTOPTION], buffer);
  LOGF("SQL_API_SQLGETCONNECTOPTION = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLGETDATA], buffer);
  LOGF("SQL_API_SQLGETDATA = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLGETFUNCTIONS], buffer);
  LOGF("SQL_API_SQLGETFUNCTIONS = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLGETINFO], buffer);
  LOGF("SQL_API_SQLGETINFO = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLGETSTMTOPTION], buffer);
  LOGF("SQL_API_SQLGETSTMTOPTION = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLGETTYPEINFO], buffer);
  LOGF("SQL_API_SQLGETTYPEINFO = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLPARAMDATA], buffer);
  LOGF("SQL_API_SQLPARAMDATA = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLPUTDATA], buffer);
  LOGF("SQL_API_SQLPUTDATA = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLSETCONNECTOPTION], buffer);
  LOGF("SQL_API_SQLSETCONNECTOPTION = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLSETSTMTOPTION], buffer);
  LOGF("SQL_API_SQLSETSTMTOPTION = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLSPECIALCOLUMNS], buffer);
  LOGF("SQL_API_SQLSPECIALCOLUMNS = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLSTATISTICS], buffer);
  LOGF("SQL_API_SQLSTATISTICS = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLTABLES], buffer);
  LOGF("SQL_API_SQLTABLES = '%s'", buffer);

  /* ----------------------------------------------------------------- */
  LOG("\tConformance Level 2. --> ");

  /* --- Level 2 Conformance ----------------------------------------- */
  Get_BoolValue((SWORD)pfExists[SQL_API_SQLBROWSECONNECT], buffer);
  LOGF("SQL_API_SQLBROWSECONNECT = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLCOLUMNPRIVILEGES], buffer);
  LOGF("SQL_API_SQLCOLUMNPRIVILEGES = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLDATASOURCES], buffer);
  LOGF("SQL_API_SQLDATASOURCES = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLDESCRIBEPARAM], buffer);
  LOGF("SQL_API_SQLDESCRIBEPARAM = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLDRIVERS], buffer);
  LOGF("SQL_API_SQLDRIVERS = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLEXTENDEDFETCH], buffer);
  LOGF("SQL_API_SQLEXTENDEDFETCH = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLFOREIGNKEYS], buffer);
  LOGF("SQL_API_SQLFOREIGNKEYS = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLMORERESULTS], buffer);
  LOGF("SQL_API_SQLMORERESULTS = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLNATIVESQL], buffer);
  LOGF("SQL_API_SQLNATIVESQL = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLNUMPARAMS], buffer);
  LOGF("SQL_API_SQLNUMPARAMS = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLPARAMOPTIONS], buffer);
  LOGF("SQL_API_SQLPARAMOPTIONS = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLPRIMARYKEYS], buffer);
  LOGF("SQL_API_SQLPRIMARYKEYS = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLPROCEDURECOLUMNS], buffer);
  LOGF("SQL_API_SQLPROCEDURECOLUMNS = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLPROCEDURES], buffer);
  LOGF("SQL_API_SQLPROCEDURES = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLSETPOS], buffer);
  LOGF("SQL_API_SQLSETPOS = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLSETSCROLLOPTIONS], buffer);
  LOGF("SQL_API_SQLSETSCROLLOPTIONS = '%s'", buffer);

  Get_BoolValue((SWORD)pfExists[SQL_API_SQLTABLEPRIVILEGES], buffer);
  LOGF("SQL_API_SQLTABLEPRIVILEGES = '%s'", buffer);

  /* --- SQLGetFunctions 2. -------------------------------------------------- */

  fFunction = SQL_API_SQLFETCH;
  retcode = SQLGetFunctions(hdbc, fFunction, pfExists);
  DIAGRECCHECK(SQL_HANDLE_DBC, hdbc, 1, SQL_SUCCESS, retcode,
      "SQLGetFunctions");

  /* --- Output -------------------------------------------------------------- */

  Get_BoolValue((SWORD)*pfExists, buffer);
  LOGF("SQL_API_SQLFETCH = '%s'", buffer);

  /* --- SQLGetFunctions 3. -------------------------------------------------- */

  fFunction = SQL_API_SQLSETPOS;
  retcode = SQLGetFunctions(hdbc, fFunction, pfExists);
  DIAGRECCHECK(SQL_HANDLE_DBC, hdbc, 1, SQL_SUCCESS, retcode,
      "SQLGetFunctions");

  /* --- Output -------------------------------------------------------------- */

  Get_BoolValue((SWORD)*pfExists, buffer);
  LOGF("SQL_API_SQLSETPOS = '%s'", buffer);

  /* - Disconnect ---------------------------------------------------- */
  // free SQL handles
  FREE_SQLHANDLES
}
