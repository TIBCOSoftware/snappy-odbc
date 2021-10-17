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

#include <chrono>
#include <inttypes.h>
#include <thread>

#include <boost/thread/barrier.hpp>

//*-------------------------------------------------------------------------

#define TESTNAME "SQLCancel"
#define TABLE "SQLCANCEL1"

#define ERROR_TEXT_LEN 511
#define MAX_NAME_LEN 256

class SQLCancelTask {
private:
  boost::barrier& m_barrier;
  SQLHSTMT m_hstmt;

public:
  inline SQLCancelTask(boost::barrier& barrier, SQLHSTMT hstmt) :
      m_barrier(barrier), m_hstmt(hstmt) {
  }

  void run() {
    try {
      LOG("Cancellation thread started");
      // wait for all threads
      m_barrier.wait();

      // cancel the statement a few times since query might be in an early stage
      // of scheduling where cancel will be ineffective
      for (int i = 0; i < 4; i++) {
        // sleep some time for the query execution to proceed
        std::this_thread::sleep_for(std::chrono::seconds(2));

        SQLRETURN retcode = SQLCancel(m_hstmt);
        DIAGRECCHECK(SQL_HANDLE_STMT, m_hstmt, 1, SQL_SUCCESS, retcode,
            "SQLCancel");
      }
    } catch (std::exception& ex) {
      LOGF("ERROR: failed with exception: %s", ex.what());
    } catch (...) {
      LOG("ERROR: failed with unknown exception...");
    }
  }
};

//*-------------------------------------------------------------------------

TEST(SQLCancel, Multithreaded) {
  DECLARE_SQLHANDLES

  // initialize the sql handles
  INIT_SQLHANDLES

  /* ------------------------ Create Table --------------------------------- */
  SQLExecDirect(hstmt, (SQLCHAR*)"DROP TABLE IF EXISTS SQLCANCEL2", SQL_NTS);
  const char* createStmt = "CREATE TABLE SQLCANCEL2(ID LONG, NAME VARCHAR(80))"
      " USING COLUMN options (buckets '2')";
  LOGF("Create Stmt = '%s'", createStmt);
  retcode = SQLExecDirect(hstmt, (SQLCHAR*)createStmt, SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLExecDirect");

  /* ------------------------ Insert values -------------------------------- */
  const char* insertStmt =
      "INSERT INTO SQLCANCEL2 SELECT ID, 'NAME_' || ID FROM RANGE(16000000)";
  LOGF("Insert Stmt= '%s'", insertStmt);
  retcode = SQLPrepare(hstmt, (SQLCHAR*)insertStmt, SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLPrepare");
  retcode = SQLExecute(hstmt);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLExecute");
  retcode = SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLFreeStmt");

  /* ------------------------- Query table --------------------------------- */
  // an expensive query
  const char* selectStmt = "select count(*), max(t2.name) from sqlcancel2 t1, "
      "sqlcancel2 t2 where substr(t1.name, 2) = substr(t2.name, 2) "
      "group by t1.id % ?";
  LOGF("\tSelect Stmt= '%s'", selectStmt);
  retcode = SQLPrepare(hstmt, (SQLCHAR*)selectStmt, SQL_NTS);
  int id = 100000;
  retcode = SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_LONG,
      SQL_INTEGER, 0, 0, &id, 0, nullptr);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLBindParameter (SQL_INTEGER)");

  int64_t count = 0;
  char maxName[80];
  retcode = SQLBindCol(hstmt, 1, SQL_C_SBIGINT, &count, 0, nullptr);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLBindCol (SQL_BIGINT)");
  retcode = SQLBindCol(hstmt, 1, SQL_C_CHAR, maxName, 80, nullptr);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode,
      "SQLBindCol (SQL_CHAR)");

  // creating cancel task for cancelling query
  LOG("Starting cancel task thread");
  // timed runs
  boost::barrier barrier(2);
  SQLCancelTask task(barrier, hstmt);
  std::thread thr(&SQLCancelTask::run, task);
  barrier.wait();
  retcode = SQLExecute(hstmt);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_ERROR, retcode, "SQLExecute");
  thr.join();

  if (retcode != SQL_ERROR) {
    do {
      retcode = SQLFetch(hstmt);
      LOGF("count = %" PRId64 " name = %s", count, maxName);
      LOGF("retcode is %d", retcode);
    } while (retcode != SQL_NO_DATA && retcode != SQL_ERROR);

    DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLFetch");
  }

  retcode = SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLFreeStmt");
  retcode = SQLFreeStmt(hstmt, SQL_CLOSE);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLFreeStmt");

  // std::this_thread::sleep_for(std::chrono::seconds(10000));

  /* ------------------- Drop table and disconnect -------------------------- */
  LOGF("Drop Stmt= 'DROP TABLE SQLCANCEL2'");
  retcode = SQLExecDirect(hstmt, (SQLCHAR*)"DROP TABLE SQLCANCEL2", SQL_NTS);
  DIAGRECCHECK(SQL_HANDLE_STMT, hstmt, 1, SQL_SUCCESS, retcode, "SQLExecDirect");

  // free sql handles
  FREE_SQLHANDLES
}
