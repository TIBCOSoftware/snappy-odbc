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

/*
 * TestHelper.h
 *
 *  Created on: Mar 29, 2013
 *      Author: shankarh
 */

#ifndef TESTHELPER_H_
#define TESTHELPER_H_

#include <common/Base.h>
#ifdef _WINDOWS
extern "C" {
#  include <windows.h>
#  include <process.h>
}
#else
extern "C" {
#  include <pthread.h>
#  include <errno.h>
#  include <dlfcn.h>
}
//#  define SQL_WCHART_CONVERT
#endif

extern "C" {
#include <sqltypes.h>
#include <sql.h>
#include <sqlext.h>
#include <stdio.h>
}

#include <gtest/gtest.h>
#include <string>
#include <iostream>

#ifndef _WINDOWS
#define TRUE true
#define FALSE false
#endif

//*************************  Utility Functions  ****************************
//*  This section contains internal utility functions
//**************************************************************************

#define LOG(y) std::cout << y << " in file " __FILE__ " at line " \
                   << __LINE__ << std::endl
#define LOGSTR(y) std::cout << y << std::endl

#ifdef _WINDOWS
#define LOGF(fmt, ...) {\
  char logbuff[16384];\
  sprintf(logbuff, fmt, __VA_ARGS__); \
  LOG(logbuff); \
}
#else
#define LOGF(format, args...)  {\
  char logbuff[16384];\
  sprintf(logbuff, format, ##args);\
  LOG(logbuff);\
}
#endif

/* OUTPUT Parameter */
#define OUTPUT 1
#define NO_OUTPUT -1
#define OUTPUTCH 63                     /* Zeichen : '?' */

/* BREAK Parameter */
#define PRG_BREAK 3

// **************************************************************************
#define DIAGRECCHECK(handletype, handle,  recnum, expected, actual, funcName) \
{\
  if (!DiagRec_Check(handletype, (SQLHANDLE) handle, recnum, \
                     expected, actual, (LPSTR) funcName, __LINE__)) { \
    EXPECT_TRUE(expected == actual || (expected == SQL_SUCCESS && \
        actual == SQL_SUCCESS_WITH_INFO)); \
  }\
}

extern char* getStringForSQLCODE(SQLRETURN inputCode, char* outputStr);

/* --------------------------------------------------------------------------
 | DiagRecCheck:
 |     This function will do a simple comparison of return codes and issue
 |     erros on failure.
 |
 | Returns:
 |
 --------------------------------------------------------------------------
 */
extern bool DiagRec_Check(SQLSMALLINT HandleType, SQLHANDLE Handle,
    SQLSMALLINT RecNumber, SQLRETURN expected, SQLRETURN actual,
    LPSTR szFuncName, int lineNumber);

extern std::string SNAPPYCONNSTRINGNODRIVER;
extern std::string SNAPPYCONNSTRING;
extern std::string SNAPPYCONNSTRINGSERVER;

/* ---------------------------------------------------------------------har- */
#define DECLARE_SQLHANDLES \
    SQLHENV henv = SQL_NULL_HENV; \
    SQLHDBC hdbc = SQL_NULL_HDBC; \
    SQLHSTMT hstmt = SQL_NULL_HSTMT; \
    SQLHDESC hdesc = SQL_NULL_HDESC; \
    SQLRETURN retcode = SQL_SUCCESS; \
    SKIP_UNUSED_WARNING(hdbc); \
    SKIP_UNUSED_WARNING(hstmt); \
    SKIP_UNUSED_WARNING(hdesc);

#define INIT_SQLHANDLES retcode = SQLAllocHandle(SQL_HANDLE_ENV, nullptr, &henv); \
    DIAGRECCHECK(SQL_HANDLE_ENV, henv, 1, SQL_SUCCESS, retcode,"SQLAllocHandle (HENV)"); \
    \
    retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0); \
    DIAGRECCHECK(SQL_HANDLE_ENV, henv, 1, SQL_SUCCESS, retcode,"SQLSetEnvAttr (HENV)"); \
    \
    retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc); \
    DIAGRECCHECK(SQL_HANDLE_DBC, hdbc, 1, SQL_SUCCESS, retcode,"SQLAllocHandle (HDBC)"); \
    \
    /* retcode = SQLConnect(hdbc, (SQLCHAR*)"snappydsn", SQL_NTS, nullptr, SQL_NTS, nullptr, SQL_NTS); */\
    retcode = SQLDriverConnect(hdbc, nullptr, (SQLCHAR*)SNAPPYCONNSTRING.c_str(), \
           SQL_NTS, nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT); \
    DIAGRECCHECK(SQL_HANDLE_DBC, hdbc, 1, SQL_SUCCESS, retcode,"SQLDriverConnect"); \
    \
    retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt); \
    DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,"SQLAllocHandle (HSTMT)");

/* ---------------------------------------------------------------------har- */

/* ---------------------------------------------------------------------har- */
#define FREE_SQLHANDLES  retcode = SQLFreeHandle(SQL_HANDLE_STMT, hstmt); \
    DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,"SQLFreeHandle (HSTMT)");  \
    \
    retcode = SQLDisconnect(hdbc); \
    DIAGRECCHECK(SQL_HANDLE_DBC, hdbc, 1, SQL_SUCCESS, retcode,"SQLDisconnect"); \
    \
    retcode = SQLFreeHandle(SQL_HANDLE_DBC, hdbc); \
    DIAGRECCHECK(SQL_HANDLE_DBC, hdbc, 1, SQL_SUCCESS, retcode,"SQLFreeHandle (HDBC)");\
    \
    retcode = SQLFreeHandle(SQL_HANDLE_ENV, henv); \
    DIAGRECCHECK(SQL_HANDLE_ENV, henv, 1, SQL_SUCCESS, retcode,"SQLFreeHandle (HENV)");

/* ---------------------------------------------------------------------har- */

//*---------------------------------------------------------------------------------
//| Get_pfSqlType:
//|
//|
//| Returns:
//*---------------------------------------------------------------------------------
extern RETCODE Get_pfSqlType(SQLSMALLINT pfSqlType, CHAR *buffer);

//*---------------------------------------------------------------------------------
//| Get_pfNullable:
//|
//|
//| Returns:
//*---------------------------------------------------------------------------------
extern RETCODE Get_pfNullable(SQLSMALLINT pfNullable, CHAR *buffer);

//*---------------------------------------------------------------------------------
//| Get_BoolValue:
//|
//|
//| Returns:
//*---------------------------------------------------------------------------------
extern RETCODE Get_BoolValue(SQLSMALLINT pfDesc, CHAR *buffer);

//*---------------------------------------------------------------------------------
//| Get_Searchable:
//|
//|
//| Returns:
//*---------------------------------------------------------------------------------
extern RETCODE Get_Searchable(SQLSMALLINT pfDesc, CHAR *buffer);

//*---------------------------------------------------------------------------------
//| Get_Updatable:
//|
//|
//| Returns:
//*---------------------------------------------------------------------------------
extern RETCODE Get_Updatable(SQLSMALLINT pfDesc, CHAR *buffer);

//*---------------------------------------------------------------------------------
//| lst_ColumnNames:
//|
//|
//| Returns:
//*---------------------------------------------------------------------------------
extern RETCODE lst_ColumnNames(SQLHENV henv, SQLHDBC hdbc, SQLHSTMT hstmt,
    int outcol);

/**
 * Install or replace a given named jar on the server.
 */
extern RETCODE installOrReplaceJar(SQLHENV henv, SQLHDBC hdbc, SQLHSTMT hstmt,
    const char* jarFile, const char* jarInstallName);

/* ------------------------------------------------------------------------- */

#endif /* TESTHELPER_H_ */
