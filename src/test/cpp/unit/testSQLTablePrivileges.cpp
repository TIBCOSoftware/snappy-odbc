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

#define TESTNAME "SQLTABLEPRIVILEGES"
#define TABLE "TABSQLTABPRIV"

#define USER1 "snappydatauser3"
#define USER2 "snappydatauser4"

#define PASSWORD1 "snappydatauser3"
#define PASSWORD2 "snappydatauser4"

#define MAX_NAME_LEN 512

#define STR_LEN 254+1
#define REM_LEN 254+1

#define NULL_VALUE "<NULL>"

//*-------------------------------------------------------------------------

/* ------------------------------------------------------------------------- */
RETCODE lstTablePrivilegesInfo(SQLHENV henv, SQLHDBC hdbc, SQLHSTMT hstmt) {
  RETCODE retcode;
  /* ------------------------------------------------------------------------- */
  /* Declare storage locations for result set data */
  CHAR szTableQualifier[STR_LEN], szTableOwner[STR_LEN], szTableName[STR_LEN],
      szGrantor[STR_LEN], szGrantee[STR_LEN], szPrivilege[STR_LEN],
      szIsGrantable[STR_LEN];

  /* Declare storage locations for bytes available to return */
  SQLLEN cbTableQualifier, cbTableOwner, cbTableName, cbGrantor, cbGrantee,
      cbPrivilege, cbIsGrantable;

  SQLSMALLINT count = 0;
  /* ---------------------------------------------------------------------har- */
  /* Bind columns in result set to storage locations */
  SQLBindCol(hstmt, 1, SQL_C_CHAR, szTableQualifier, STR_LEN,
      &cbTableQualifier);
  SQLBindCol(hstmt, 2, SQL_C_CHAR, szTableOwner, STR_LEN, &cbTableOwner);
  SQLBindCol(hstmt, 3, SQL_C_CHAR, szTableName, STR_LEN, &cbTableName);
  SQLBindCol(hstmt, 4, SQL_C_CHAR, szGrantor, STR_LEN, &cbGrantor);
  SQLBindCol(hstmt, 5, SQL_C_CHAR, szGrantee, STR_LEN, &cbGrantee);
  SQLBindCol(hstmt, 6, SQL_C_CHAR, szPrivilege, STR_LEN, &cbPrivilege);
  SQLBindCol(hstmt, 7, SQL_C_CHAR, szIsGrantable, STR_LEN, &cbIsGrantable);

  retcode = lst_ColumnNames(henv, hdbc, hstmt, 7);

  while (TRUE) {
    count++;

    retcode = SQLFetch(hstmt);
    /* DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,"SQLFetch");*/

    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
      /* Process fetched data */
      if (cbTableQualifier == SQL_NULL_DATA) strcpy(szTableQualifier,
      NULL_VALUE);
      if (cbTableOwner == SQL_NULL_DATA) strcpy(szTableOwner, NULL_VALUE);
      if (cbTableName == SQL_NULL_DATA) strcpy(szTableName, NULL_VALUE);
      if (cbGrantor == SQL_NULL_DATA) strcpy(szGrantor, NULL_VALUE);
      if (cbGrantee == SQL_NULL_DATA) strcpy(szGrantee, NULL_VALUE);
      if (cbPrivilege == SQL_NULL_DATA) strcpy(szPrivilege, NULL_VALUE);
      if (cbIsGrantable == SQL_NULL_DATA) strcpy(szIsGrantable, NULL_VALUE);

      printf("\tTable %d : '%s','%s','%s','%s','%s','%s','%s'\r\n", count,
          szTableQualifier, szTableOwner, szTableName, szGrantor, szGrantee,
          szPrivilege, szIsGrantable);
    } else {
      /* DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,"SQLFetch"); */
      break;
    }
  }
  return TRUE;
}
/* ------------------------------------------------------------------------- */

TEST(SQLTablePrivileges, BasicCheck) {
  DECLARE_SQLHANDLES

  /* ------------------------------------------------------------------------- */
  CHAR tabname[MAX_NAME_LEN + 1];
  CHAR create[MAX_NAME_LEN + 1];
  CHAR grant[MAX_NAME_LEN + 1];
  CHAR drop[MAX_NAME_LEN + 1];

  /* ---------------------------------------------------------------------har- */

  //init sql handles (stmt, dbc, env)
  INIT_SQLHANDLES

  /* ------------------------------------------------------------------------- */

  /* --- Create Table  -------------------------------------------- */
  strcpy(tabname, TABLE);
  strcpy(create, "CREATE TABLE ");
  strcat(create, tabname);
  strcat(create, " (CUST_ID INTEGER, CUST_NAME CHAR(30) )");
  LOGF("Create Stmt (Table: " TABLE ")= '%s'\r\n", create);

  retcode = SQLExecDirect(hstmt, (SQLCHAR*)create, SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLExecDirect");

  /* --- Grant Table 1. (USER1) ------------------------------------- */
  strcpy(tabname, TABLE);
  strcpy(grant, "GRANT SELECT, INSERT,  UPDATE, DELETE ON ");
  strcat(grant, tabname);
  strcat(grant, " TO " USER1);
  LOGF("Grant Stmt 1.(Table: " TABLE ")= '%s'\r\n", grant);

  retcode = SQLExecDirect(hstmt, (SQLCHAR*)grant, SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLExecDirect");

  /* --- Grant Table 2. (USER2) ------------------------------------- */
  strcpy(tabname, TABLE);
  strcpy(grant, "GRANT ALL PRIVILEGES ON ");
  strcat(grant, tabname);
  strcat(grant, " TO " USER2);
  LOGF("Grant Stmt 2.(Table: " TABLE ")= '%s'\r\n", grant);

  retcode = SQLExecDirect(hstmt, (SQLCHAR*)grant, SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLExecDirect");
  /* ------------------------------------------------------------------------- */
  /* ***************************************************************** */
  /* *** I. SQLTablePrivileges *************************************** */
  /* ***************************************************************** */
  printf(
      "\tI.) SQLTablePrivileges -> (TableOwner: %s - TableName: " TABLE
      " )\r\n", ""/*lpSrvr->szValidLogin0*/);

  retcode = SQLTablePrivileges(hstmt, nullptr, 0, /* Table qualifier */
      nullptr, 0,// lpSrvr->szValidLogin0, SQL_NTS, /* Table owner     */
      (SQLCHAR*)TABLE, SQL_NTS); /* Table name      */
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLTablePrivileges");

  if (retcode == SQL_SUCCESS) lstTablePrivilegesInfo(henv, hdbc, hstmt);

  retcode = SQLFreeStmt(hstmt, SQL_CLOSE);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLFreeStmt");

  /* --- Drop Table ----------------------------------------------- */
  /* --- Drop Table 1. ------------------------------------------ */
  strcpy(tabname, TABLE);
  strcpy(drop, "DROP TABLE ");
  strcat(drop, tabname);
  LOGF("Drop Stmt (Table:" TABLE ")= '%s'\r\n", drop);

  retcode = SQLExecDirect(hstmt, (SQLCHAR*)drop, SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLExecDirect");
  /* ---------------------------------------------------------------------har- */
  //free sql handles (stmt, dbc, env)
  FREE_SQLHANDLES
}
