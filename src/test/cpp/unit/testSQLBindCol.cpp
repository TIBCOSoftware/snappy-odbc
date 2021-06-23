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
#define TESTNAME "SQLBindCol"
#define TABLE "BINDCOL"

#define MAX_NAME_LEN 256

//*-------------------------------------------------------------------------

TEST(SQLBindCol, BasicBinding) {
  DECLARE_SQLHANDLES

  /*------------------------------------------------------------------------- */
  CHAR create[MAX_NAME_LEN + MAX_NAME_LEN + 1];
  CHAR insert[MAX_NAME_LEN + MAX_NAME_LEN + 1];
  CHAR select[MAX_NAME_LEN + MAX_NAME_LEN + 1];
  CHAR drop[MAX_NAME_LEN + 1];
  CHAR tabname[MAX_NAME_LEN];

  CHAR szChar[MAX_NAME_LEN + 1], szFixed[MAX_NAME_LEN + 1];
  double sFloat;

  SQLLEN cbChar, cbFloat, cbFixed;

  //initialize the sql handles
  INIT_SQLHANDLES

  /* --- Create Table --------------------------------------------- */
  SQLExecDirect(hstmt, (SQLCHAR*)"drop table if exists " TABLE, SQL_NTS);
  strcpy(tabname, TABLE);
  strcpy(create, "CREATE TABLE ");
  strcat(create, tabname);
  strcat(create, " (TYP_CHAR CHAR(30), TYP_FLOAT FLOAT(15),");
  strcat(create, " TYP_DEC DECIMAL(15,2) )");
  LOGF("Create Stmt = '%s'", create);

  retcode = SQLExecDirect(hstmt, (SQLCHAR*)create, SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLExecDirect");

  /* --- Insert Table --------------------------------------------- */
  /* --- 1. ---*/
  strcpy(insert, "INSERT INTO ");
  strcat(insert, tabname);
  strcat(insert, " VALUES ('Dies ein Datatype-Test.', 123456789,");
  strcat(insert, " 98765321.99)");
  LOGF("Insert Stmt = '%s'", insert);

  retcode = SQLExecDirect(hstmt, (SQLCHAR*)insert, SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLExecDirect");

  /* --- Select Table --------------------------------------------- */
  /* --- 1. --- */
  strcpy(select, "SELECT ");
  strcat(select, "TYP_CHAR, TYP_FLOAT, TYP_DEC ");
  strcat(select, " FROM ");
  strcat(select, tabname);
  LOGF("Select Stmt= '%s'", select);

  retcode = SQLExecDirect(hstmt, (SQLCHAR*)select, SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLExecDirect");

  retcode = SQLBindCol(hstmt, 1, SQL_C_CHAR, szChar, MAX_NAME_LEN, &cbChar);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLBindCol");

  retcode = SQLBindCol(hstmt, 2, SQL_C_DOUBLE, &sFloat, 0, &cbFloat);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLBindCol");

  retcode = SQLBindCol(hstmt, 3, SQL_C_CHAR, szFixed, MAX_NAME_LEN, &cbFixed);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLBindCol");

  while (1) {
    retcode = SQLFetch(hstmt);
    if (retcode == SQL_NO_DATA_FOUND) break;
    DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLFetch");
    if (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO) break;
  }
  retcode = SQLFreeStmt(hstmt, SQL_CLOSE);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLFreeStmt");

  /* --- Drop Table ----------------------------------------------- */
  strcpy(drop, "DROP TABLE ");
  strcat(drop, tabname);
  LOGF("Drop Stmt= '%s'", drop);

  retcode = SQLExecDirect(hstmt, (SQLCHAR*)drop, SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLExecDirect");
  /* ---------------------------------------------------------------------har- */

  /* - Disconnect ---------------------------------------------------- */
  //free sql handles
  FREE_SQLHANDLES

}

#define ROW_ARRAY_SIZE 5000

TEST(SQLBindCol, ColumnBinding) {
  DECLARE_SQLHANDLES

  SQLUINTEGER OrderIDArray[ROW_ARRAY_SIZE];
  SQLULEN numRowsFetched;
  SQLCHAR salesPersonArray[ROW_ARRAY_SIZE][20],
      statusArray[ROW_ARRAY_SIZE][10];
  SQLLEN orderIDIndArray[ROW_ARRAY_SIZE],
      salesPersonLenOrIndArray[ROW_ARRAY_SIZE],
      statusLenOrIndArray[ROW_ARRAY_SIZE];
  SQLUSMALLINT RowStatusArray[ROW_ARRAY_SIZE];

  //initialize sql hanldles
  INIT_SQLHANDLES

  retcode = SQLExecDirect(hstmt, (SQLCHAR*)"drop table if exists Orders",
      SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLExecDirect");

  retcode = SQLExecDirect(hstmt, (SQLCHAR*)"create table Orders(OrderID int, "
      "SalesPerson VARCHAR(20), Status VARCHAR(10))", SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLExecDirect");

  const char* persons[] = { "Abishek", "Ajay", "Amol", "Amey", "Amogh",
                            "Avinash", "Deepak", "Sachin", "Virat", "Ajinkya",
                            "Ishan" };
  const char* statuses[] = { "CONFIRM", "CANCEL", "CONFIRM", "CANCEL", "CANCEL",
                             "CONFIRM", "CONFIRM", "CANCEL", "CONFIRM",
                             "CONFIRM", "CANCEL" };

  int numEntries = sizeof(persons) / sizeof(char*);
  for (int i = 0; i < numEntries; i++) {
    std::ostringstream ostr;
    ostr << "insert into Orders values(" << (i + 1) << ", '" << persons[i]
        << "', '" << statuses[i] << "')";
    const std::string &insertQ = ostr.str();
    retcode = SQLExecDirect(hstmt, (SQLCHAR*)insertQ.c_str(), (SQLINTEGER) insertQ.size());
    DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
        "SQLExecDirect");
  }
  for (int i = 0; i < ROW_ARRAY_SIZE; i++) {
    std::ostringstream ostr;
    ostr << "insert into Orders values(" << (i + numEntries + 4)
        << ", 'Atul', 'CANCEL')";
    retcode = SQLExecDirect(hstmt, (SQLCHAR*)ostr.str().c_str(), SQL_NTS);
    DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
        "SQLExecDirect?");
  }

  // Set the SQL_ATTR_ROW_BIND_TYPE statement attribute to use
  // column-wise binding. Declare the rowset size with the
  // SQL_ATTR_ROW_ARRAY_SIZE statement attribute. Set the
  // SQL_ATTR_ROW_STATUS_PTR statement attribute to point to the
  // row status array. Set the SQL_ATTR_ROWS_FETCHED_PTR statement
  // attribute to point to cRowsFetched.
  retcode = SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_BIND_TYPE,
      SQL_BIND_BY_COLUMN, 0);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLSetStmtAttr");
  retcode = SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_ARRAY_SIZE,
      (SQLPOINTER)ROW_ARRAY_SIZE, 0);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLSetStmtAttr");
  retcode = SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_STATUS_PTR, RowStatusArray, 0);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLSetStmtAttr");
  retcode = SQLSetStmtAttr(hstmt, SQL_ATTR_ROWS_FETCHED_PTR, &numRowsFetched,
      0);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLSetStmtAttr");

  // Bind arrays to the OrderID, SalesPerson, and Status columns.
  retcode = SQLBindCol(hstmt, 1, SQL_C_ULONG, OrderIDArray,
      sizeof(OrderIDArray[0]), orderIDIndArray);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLBindCol");
  retcode = SQLBindCol(hstmt, 2, SQL_C_CHAR, salesPersonArray,
      sizeof(salesPersonArray[0]), salesPersonLenOrIndArray);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLBindCol");
  retcode = SQLBindCol(hstmt, 3, SQL_C_CHAR, statusArray,
      sizeof(statusArray[0]), statusLenOrIndArray);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLBindCol");

  // Execute a statement to retrieve rows from the Orders table.
  retcode = SQLExecDirect(hstmt,
      (SQLCHAR*)"SELECT OrderID, SalesPerson, Status "
          "FROM Orders order by OrderID", SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLSetStmtAttr");

  retcode = SQLFetch(hstmt);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLFetch");
  LOGF("Rows Fetched count is %ld", numRowsFetched);
  for (int i = 0; i < ROW_ARRAY_SIZE; i++) {
    if (i < numEntries) {
      EXPECT_EQ(i + 1, (int)OrderIDArray[i]) << "order ID retrieved is incorrect";
      EXPECT_EQ(std::string(persons[i]), std::string((char*)salesPersonArray[i]))
          << "SalesPersonArray retrieved is incorrect";
      EXPECT_EQ(std::string(statuses[i]), std::string((char*)statusArray[i]))
          << "StatusArray retrieved is incorrect";
    } else {
      EXPECT_EQ(i + 4, (int)OrderIDArray[i]) << "order ID retrieved is incorrect";
      EXPECT_EQ(std::string("Atul"), std::string((char*)salesPersonArray[i]))
          << "SalesPersonArray retrieved is incorrect";
      EXPECT_EQ(std::string("CANCEL"), std::string((char*)statusArray[i]))
          << "StatusArray retrieved is incorrect";
    }
  }

  LOGF("Rows Fetched count is %ld", numRowsFetched);
  EXPECT_EQ(ROW_ARRAY_SIZE, (int)numRowsFetched)
      << "Number of fetched rows is incorrect";

  // fetch the rest

  retcode = SQLFetch(hstmt);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLFetch");
  EXPECT_EQ(numEntries, (int)numRowsFetched)
      << "Number of fetched rows is incorrect";
  for (int i = 0; i < numEntries; i++) {
    EXPECT_EQ(i + ROW_ARRAY_SIZE + 4, (int)OrderIDArray[i])
        << "order ID retrieved is incorrect";
    EXPECT_EQ(std::string("Atul"), std::string((char*)salesPersonArray[i]))
        << "SalesPersonArray retrieved is incorrect";
    EXPECT_EQ(std::string("CANCEL"), std::string((char*)statusArray[i]))
        << "StatusArray retrieved is incorrect";
  }

  retcode = SQLFetch(hstmt);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_NO_DATA, retcode, "SQLFetch");

  //free sql handles
  FREE_SQLHANDLES
}

TEST(SQLBindCol, RowBinding) {
  DECLARE_SQLHANDLES

  // Define the ORDERINFO struct and allocate an array of 10 structs.
  typedef struct {
    SQLUINTEGER OrderID;
    SQLLEN OrderIDInd;
    SQLCHAR SalesPerson[20];
    SQLLEN SalesPersonLenOrInd;
    SQLCHAR Status[10];
    SQLLEN StatusLenOrInd;
  } ORDERINFO;
  ORDERINFO orderInfoArray[ROW_ARRAY_SIZE];

  SQLULEN numRowsFetched;
  SQLUSMALLINT RowStatusArray[ROW_ARRAY_SIZE];

  //initialize sql hanldles
  INIT_SQLHANDLES

  retcode = SQLExecDirect(hstmt, (SQLCHAR*)"drop table if exists Orders",
      SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLExecDirect");

  retcode = SQLExecDirect(hstmt, (SQLCHAR*)"create table Orders(OrderID int, "
      "SalesPerson VARCHAR(20), Status VARCHAR(10))", SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLExecDirect");

  retcode = SQLExecDirect(hstmt,
      (SQLCHAR*)"insert into Orders values(1, 'Abhishek', 'CONFIRM')",
      SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLExecDirect");

  retcode = SQLExecDirect(hstmt,
      (SQLCHAR*)"insert into Orders values(2, 'Ajay', 'CANCEL')", SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLExecDirect");

  retcode = SQLExecDirect(hstmt,
      (SQLCHAR*)"insert into Orders values(3, 'Amol', 'CONFIRM')",
      SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLExecDirect");

  retcode = SQLExecDirect(hstmt,
      (SQLCHAR*)"insert into Orders values(4, 'Amey', 'CANCEL')", SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLExecDirect");

  retcode = SQLExecDirect(hstmt,
      (SQLCHAR*)"insert into Orders values(5, 'Amogh', 'CANCEL')",
      SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLExecDirect");

  retcode = SQLExecDirect(hstmt,
      (SQLCHAR*)"insert into Orders values(6, 'Avinash', 'CONFIRM')",
      SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLExecDirect");

  retcode = SQLExecDirect(hstmt,
      (SQLCHAR*)"insert into Orders values(7, 'Deepak', 'CONFIRM')",
      SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLExecDirect");

  retcode = SQLExecDirect(hstmt,
      (SQLCHAR*)"insert into Orders values(8, 'Sachin', 'CANCEL')",
      SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLExecDirect");

  retcode = SQLExecDirect(hstmt,
      (SQLCHAR*)"insert into Orders values(9, 'Virat', 'CONFIRM')",
      SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLExecDirect");

  retcode = SQLExecDirect(hstmt,
      (SQLCHAR*)"insert into Orders values(10, 'Ajinkya', 'CONFIRM')",
      SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLExecDirect");

  retcode = SQLExecDirect(hstmt,
      (SQLCHAR*)"insert into Orders values(11, 'Ishan', 'CANCEL')",
      SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLExecDirect");

  for (int i = 0; i < ROW_ARRAY_SIZE; i++) {
    retcode = SQLExecDirect(hstmt,
        (SQLCHAR*)"insert into Orders values(12, 'Atul', 'CANCEL')",
        SQL_NTS);
    DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
        "SQLExecDirect?");
  }

  // Specify the size of the structure with the SQL_ATTR_ROW_BIND_TYPE
  // statement attribute. This also declares that row-wise binding will
  // be used. Declare the rowset size with the SQL_ATTR_ROW_ARRAY_SIZE
  // statement attribute. Set the SQL_ATTR_ROW_STATUS_PTR statement
  // attribute to point to the row status array. Set the
  // SQL_ATTR_ROWS_FETCHED_PTR statement attribute to point to
  // NumRowsFetched.
  SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_BIND_TYPE, (SQLPOINTER)sizeof(ORDERINFO),
      0);
  SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)ROW_ARRAY_SIZE,
      0);
  SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_STATUS_PTR, RowStatusArray, 0);
  SQLSetStmtAttr(hstmt, SQL_ATTR_ROWS_FETCHED_PTR, &numRowsFetched, 0);

// Bind elements of the first structure in the array to the OrderID,
// SalesPerson, and Status columns.
  SQLBindCol(hstmt, 1, SQL_C_ULONG, &orderInfoArray[0].OrderID,
      sizeof(orderInfoArray[0].OrderID), &orderInfoArray[0].OrderIDInd);
  SQLBindCol(hstmt, 2, SQL_C_CHAR, orderInfoArray[0].SalesPerson,
      sizeof(orderInfoArray[0].SalesPerson),
      &orderInfoArray[0].SalesPersonLenOrInd);
  SQLBindCol(hstmt, 3, SQL_C_CHAR, orderInfoArray[0].Status,
      sizeof(orderInfoArray[0].Status), &orderInfoArray[0].StatusLenOrInd);

  // Execute a statement to retrieve rows from the Orders table.
  SQLExecDirect(hstmt,
      (SQLCHAR*)"SELECT OrderID, SalesPerson, Status FROM Orders order by OrderID",
      SQL_NTS);

  // Fetch up to the rowset size number of rows at a time. Print the actual
  // number of rows fetched; this number is returned in NumRowsFetched.
  // Check the row status array to print only those rows successfully
  // fetched. Code to check if rc equals SQL_SUCCESS_WITH_INFO or
  // SQL_ERRORnot shown.

  retcode = SQLFetch(hstmt);

  for (int i = 0; i < ROW_ARRAY_SIZE; i++) {
    if ((i % 100) == 0) {
      LOGF("OrderID = %d SalesPerson= %s Status= %s",
          orderInfoArray[i].OrderID, (char* )orderInfoArray[i].SalesPerson,
          (char* )orderInfoArray[i].Status);
      LOGF("Order ID indicator = %ld SalesPerson Indicator = %ld "
          "Status Indicator = %ld", orderInfoArray[i].OrderIDInd,
          orderInfoArray[i].SalesPersonLenOrInd,
          orderInfoArray[i].StatusLenOrInd);
    }
  }

  EXPECT_EQ(1U, orderInfoArray[0].OrderID)
      << "order ID retrieved is incorrect";
  EXPECT_EQ(std::string("Abhishek"), std::string((char*)orderInfoArray[0].
      SalesPerson)) << "SalesPersonArray retrieved is incorrect";
  EXPECT_EQ(std::string("CONFIRM"), std::string((char*)orderInfoArray[0].
      Status)) << "StatusArray retrieved is incorrect";

  EXPECT_EQ(2U, orderInfoArray[1].OrderID);
  EXPECT_EQ(std::string("Ajay"), std::string((char*)orderInfoArray[1].
      SalesPerson)) << "SalesPersonArray retrieved is incorrect";
  EXPECT_EQ(std::string("CANCEL"), std::string((char*)orderInfoArray[1].
      Status)) << "StatusArray retrieved is incorrect";

  LOGF("Rows Fetched count is %ld", numRowsFetched);

  EXPECT_EQ(ROW_ARRAY_SIZE, (int)numRowsFetched)
      << "Number of fetched rows is incorrect";

  // free sql handles
  FREE_SQLHANDLES
}
