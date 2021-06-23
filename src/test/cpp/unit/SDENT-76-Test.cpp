/*
* Copyright (c) 2019 TIBCO Software, Inc. All rights reserved.
*/

#include "TestHelper.h"

#define MAX_NAME_LEN 256
#define TABLE "SDENT76"

TEST (SDENT76, NegativeVal_With_Precision_3_Scale_2) {
  DECLARE_SQLHANDLES
  /* ------------------------------------------------------------------------- */
  CHAR tabname[MAX_NAME_LEN + 1];
  CHAR create[MAX_NAME_LEN + 1];
  CHAR insert[MAX_NAME_LEN + 1];
  CHAR select[MAX_NAME_LEN + 1];
  CHAR drop[MAX_NAME_LEN + 1];
  SQLLEN pcbValue;
  CHAR rgbValueDecimal[MAX_NAME_LEN + 1];
  /* ---------------------------------------------------------------------har- */

  //init sql handles (stmt, dbc, env)
  INIT_SQLHANDLES
    /* --- Create Table --------------------------------------------- */
  strcpy(tabname, TABLE);
  strcpy(create, "CREATE TABLE IF NOT EXISTS ");
  strcat(create, tabname);
  strcat(create, " (id int, value Decimal(3,2) )");
  LOGF("Create Stmt = '%s'", create);

  retcode = SQLExecDirect(hstmt, (SQLCHAR*)create, SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
    "SQLExecDirect");

  /* --- Insert into Table --------------------------------------------- */
  strcpy(insert, "insert into ");
  strcat(insert, TABLE);
  strcat(insert, " values (1,-0.66),(2,-0.74),(3,-0.80), (4,0.10),(5,0.20)");
  LOGF("Insert Stmt = '%s'", insert);
  
  retcode = SQLExecDirect(hstmt, (SQLCHAR*)insert, SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
    "SQLExecDirect");

  /*---------- Test to fetch negative value-------------*/
  /* --- Select into Table --------------------------------------------- */
  strcpy(select, "select value from ");
  strcat(select, TABLE);
  strcat(select, " where id=1");
  LOGF("Select Stmt = '%s'", select);
  
  retcode = SQLExecDirect(hstmt, (SQLCHAR*)select, SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
    "SQLExecDirect");
  
  /* --- Fetch data --------------------------------------------- */
  retcode = SQLFetch(hstmt);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLFetch");
  /* Value 1. */
  LOGF(" Get Column 1. (Decimal) --> ");

  retcode = SQLGetData(hstmt, 1, SQL_C_CHAR, rgbValueDecimal,
    sizeof(rgbValueDecimal), &pcbValue);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLGetData");

  LOGF("Select Values Col.1: ->'%s'", rgbValueDecimal);
  bool val = !strcmp(rgbValueDecimal, "-0.66");
  ASSERT_EQ(true, val);

  memset(rgbValueDecimal, 0, sizeof(rgbValueDecimal));
  retcode = SQLFreeStmt(hstmt, SQL_CLOSE);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLFreeStmt");

  /* --- Select into Table --------------------------------------------- */
  strcpy(select, "select value from ");
  strcat(select, TABLE);
  strcat(select, " where id=2");
  LOGF("Select Stmt = '%s'", select);

  retcode = SQLExecDirect(hstmt, (SQLCHAR*)select, SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
    "SQLExecDirect");

  /* --- Fetch data --------------------------------------------- */
  retcode = SQLFetch(hstmt);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLFetch");
  /* Value 1. */
  LOGF(" Get Column 1. (Decimal) --> ");

  retcode = SQLGetData(hstmt, 1, SQL_C_CHAR, rgbValueDecimal,
    sizeof(rgbValueDecimal), &pcbValue);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLGetData");

  LOGF("Select Values Col.1: ->'%s'", rgbValueDecimal);
  val = !strcmp(rgbValueDecimal, "-0.74");
  ASSERT_EQ(true, val);
  
  memset(rgbValueDecimal, 0, sizeof(rgbValueDecimal));

  retcode = SQLFreeStmt(hstmt, SQL_CLOSE);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLFreeStmt");

  /* --- Select into Table --------------------------------------------- */
  strcpy(select, "select value from ");
  strcat(select, TABLE);
  strcat(select, " where id=3");
  LOGF("Select Stmt = '%s'", select);

  retcode = SQLExecDirect(hstmt, (SQLCHAR*)select, SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
    "SQLExecDirect");

  /* --- Fetch data --------------------------------------------- */
  retcode = SQLFetch(hstmt);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLFetch");
  /* Value 1. */
  LOGF(" Get Column 1. (Decimal) --> ");

  retcode = SQLGetData(hstmt, 1, SQL_C_CHAR, rgbValueDecimal,
    sizeof(rgbValueDecimal), &pcbValue);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLGetData");

  LOGF("Select Values Col.1: ->'%s'", rgbValueDecimal);
  val = !strcmp(rgbValueDecimal, "-0.80");
  ASSERT_EQ(true, val);

  memset(rgbValueDecimal, 0, sizeof(rgbValueDecimal));

  retcode = SQLFreeStmt(hstmt, SQL_CLOSE);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLFreeStmt");

  /* --- Drop Table ----------------------------------------------- */
  strcpy(drop, "DROP TABLE IF EXISTS ");
  strcat(drop, tabname);
  LOGF("Drop Stmt= '%s'", drop);

  retcode = SQLExecDirect(hstmt, (SQLCHAR*)drop, SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
    "SQLExecDirect");


  /* ---------------------------------------------------------------------har- */
  //free sql handles (stmt, dbc, env)
  FREE_SQLHANDLES
}

TEST(SDENT76, NegativeVal_With_Precision_6_Scale_4) {
  DECLARE_SQLHANDLES
  /* ------------------------------------------------------------------------- */
  CHAR tabname[MAX_NAME_LEN + 1];
  CHAR create[MAX_NAME_LEN + 1];
  CHAR insert[MAX_NAME_LEN + 1];
  CHAR select[MAX_NAME_LEN + 1];
  CHAR drop[MAX_NAME_LEN + 1];
  SQLLEN pcbValue;
  CHAR rgbValueDecimal[MAX_NAME_LEN + 1];
  /* ---------------------------------------------------------------------har- */

  //init sql handles (stmt, dbc, env)
  INIT_SQLHANDLES
    /* --- Create Table --------------------------------------------- */
    strcpy(tabname, TABLE);
  strcpy(create, "CREATE TABLE IF NOT EXISTS ");
  strcat(create, tabname);
  strcat(create, " (id int, value Decimal(6,4) )");
  LOGF("Create Stmt = '%s'", create);

  retcode = SQLExecDirect(hstmt, (SQLCHAR*)create, SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
    "SQLExecDirect");

  /* --- Insert into Table --------------------------------------------- */
  strcpy(insert, "insert into ");
  strcat(insert, TABLE);
  strcat(insert, " values (1,-0.6612),(2,-0.7421),(3,-0.8015), (4,0.1000),(5,0.2090)");
  LOGF("Insert Stmt = '%s'", insert);

  retcode = SQLExecDirect(hstmt, (SQLCHAR*)insert, SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
    "SQLExecDirect");

  /*---------- Test to fetch negative value-------------*/
  /* --- Select into Table --------------------------------------------- */
  strcpy(select, "select value from ");
  strcat(select, TABLE);
  strcat(select, " where id=1");
  LOGF("Select Stmt = '%s'", select);

  retcode = SQLExecDirect(hstmt, (SQLCHAR*)select, SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
    "SQLExecDirect");

  /* --- Fetch data --------------------------------------------- */
  retcode = SQLFetch(hstmt);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLFetch");
  /* Value 1. */
  LOGF(" Get Column 1. (Decimal) --> ");

  retcode = SQLGetData(hstmt, 1, SQL_C_CHAR, rgbValueDecimal,
    sizeof(rgbValueDecimal), &pcbValue);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLGetData");

  LOGF("Select Values Col.1: ->'%s'", rgbValueDecimal);
  bool val = !strcmp(rgbValueDecimal, "-0.6612");
  ASSERT_EQ(true, val);
  memset(rgbValueDecimal, 0, sizeof(rgbValueDecimal));

  retcode = SQLFreeStmt(hstmt, SQL_CLOSE);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLFreeStmt");

  /* --- Select into Table --------------------------------------------- */
  strcpy(select, "select value from ");
  strcat(select, TABLE);
  strcat(select, " where id=2");
  LOGF("Select Stmt = '%s'", select);

  retcode = SQLExecDirect(hstmt, (SQLCHAR*)select, SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
    "SQLExecDirect");

  /* --- Fetch data --------------------------------------------- */
  retcode = SQLFetch(hstmt);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLFetch");
  /* Value 1. */
  LOGF(" Get Column 1. (Decimal) --> ");

  retcode = SQLGetData(hstmt, 1, SQL_C_CHAR, rgbValueDecimal,
    sizeof(rgbValueDecimal), &pcbValue);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLGetData");

  LOGF("Select Values Col.1: ->'%s'", rgbValueDecimal);
  val = !strcmp(rgbValueDecimal, "-0.7421");
  ASSERT_EQ(true, val);
  memset(rgbValueDecimal, 0, sizeof(rgbValueDecimal));

  retcode = SQLFreeStmt(hstmt, SQL_CLOSE);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLFreeStmt");



  /* --- Select into Table --------------------------------------------- */
  strcpy(select, "select value from ");
  strcat(select, TABLE);
  strcat(select, " where id=3");
  LOGF("Select Stmt = '%s'", select);

  retcode = SQLExecDirect(hstmt, (SQLCHAR*)select, SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
    "SQLExecDirect");

  /* --- Fetch data --------------------------------------------- */
  retcode = SQLFetch(hstmt);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLFetch");
  /* Value 1. */
  LOGF(" Get Column 1. (Decimal) --> ");

  retcode = SQLGetData(hstmt, 1, SQL_C_CHAR, rgbValueDecimal,
    sizeof(rgbValueDecimal), &pcbValue);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLGetData");

  LOGF("Select Values Col.1: ->'%s'", rgbValueDecimal);
  val = !strcmp(rgbValueDecimal, "-0.8015");
  ASSERT_EQ(true, val);
  memset(rgbValueDecimal, 0, sizeof(rgbValueDecimal));
  
  retcode = SQLFreeStmt(hstmt, SQL_CLOSE);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLFreeStmt");

  /* --- Drop Table ----------------------------------------------- */
  strcpy(drop, "DROP TABLE IF EXISTS ");
  strcat(drop, tabname);
  LOGF("Drop Stmt= '%s'", drop);

  retcode = SQLExecDirect(hstmt, (SQLCHAR*)drop, SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
    "SQLExecDirect");


  /* ---------------------------------------------------------------------har- */
  //free sql handles (stmt, dbc, env)
  FREE_SQLHANDLES
}
