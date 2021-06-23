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

#include "../unit/TestHelper.h"

#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include <boost/lexical_cast.hpp>
#include <boost/thread/barrier.hpp>

#if defined(_LINUX) && defined(VALGRIND)
extern "C" {
#include <valgrind/valgrind.h>
}
#else
#define RUNNING_ON_VALGRIND 0
#endif

void printStatementError(SQLHSTMT stmt, int line) {
  SQLCHAR sqlState[6];
  SQLINTEGER errorCode;
  SQLCHAR message[8192];
  SQLSMALLINT messageLen;
  ::SQLGetDiagRec(SQL_HANDLE_STMT, stmt, 1, sqlState, &errorCode, message,
      8191, &messageLen);
  std::cout << "Statement before line " << line << " failed with SQLState="
      << sqlState << ", ErrorCode=" << errorCode << ": " << message
      << std::endl;
}

class SelectTask {
private:
  SQLHENV m_env;
  std::string& m_connStr;
  boost::barrier& m_barrier;
  const int m_numRows;
  const int m_numRuns;

public:
  inline SelectTask(SQLHENV env, std::string& connStr, boost::barrier& barrier,
      int numRows, int numRuns) :
      m_env(env), m_connStr(connStr), m_barrier(barrier), m_numRows(numRows),
      m_numRuns(numRuns) {
  }

  void run() {
    SQLHDBC conn;
    SQLHSTMT stmt;

    ::SQLAllocHandle(SQL_HANDLE_DBC, m_env, &conn);

    if (!SQL_SUCCEEDED(
        ::SQLDriverConnect(conn, nullptr, (SQLCHAR*)m_connStr.c_str(),
            (SQLSMALLINT)m_connStr.size(), nullptr, 0, nullptr,
            SQL_DRIVER_NOPROMPT))) {
      // connection failed
      SQLCHAR sqlState[6];
      SQLINTEGER errorCode;
      SQLCHAR message[8192];
      SQLSMALLINT messageLen;
      ::SQLGetDiagRec(SQL_HANDLE_DBC, conn, 1, sqlState, &errorCode, message,
          8191, &messageLen);
      std::cout << "Connection failed for thread "
          << std::this_thread::get_id() << " with SQLState=" << sqlState
          << ", ErrorCode=" << errorCode << ": " << message << std::endl;
    }

    ::SQLAllocHandle(SQL_HANDLE_STMT, conn, &stmt);

    ::SQLPrepare(stmt, (SQLCHAR*)"SELECT * FROM new_order "
        "WHERE no_d_id = ? AND no_w_id = ? AND no_o_id = ?", SQL_NTS);

    std::cout << "Starting timed selects for thread "
        << std::this_thread::get_id() << std::endl;

    // wait for all threads
    m_barrier.wait();

    int rowNum, w_id;
    ::SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0,
        0, &w_id, 0, nullptr);
    ::SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0,
        0, &w_id, 0, nullptr);
    ::SQLBindParameter(stmt, 3, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0,
        0, &rowNum, 0, nullptr);
    for (int i = 1; i <= m_numRuns; i++) {
      rowNum = (i % m_numRows) + 1;
      w_id = (rowNum % 98);
      ::SQLExecute(stmt);

      int numResults = 0;
      while (SQL_SUCCEEDED(::SQLFetch(stmt))) {
        int o_id;
        //char* name = (char*)::malloc(100 * sizeof(char));
        ::SQLGetData(stmt, 1, SQL_C_LONG, &o_id, sizeof(o_id), nullptr);
        //::SQLGetData(stmt, 2, SQL_C_CHAR, name, 100, nullptr);
        //::free(name);
        numResults++;
      }
      ::SQLCloseCursor(stmt);
      if (numResults == 0) {
        std::cerr << "unexpected 0 results for w_id, d_id " << w_id
            << std::endl;
      }
    }

    ::SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    ::SQLDisconnect(conn);
    ::SQLFreeHandle(SQL_HANDLE_DBC, conn);
  }
};

/**
 * Simple performance test given n threads of execution.
 */
int main1(int argc, const char* argv[]) {
  std::cout << "Using argc=" << argc << std::endl;
  if (argc != 3 && argc != 4) {
    std::cerr << "Usage: <script> <server> <port> [<threads>]" << std::endl;
    return 1;
  }
  const char* server = argv[1];
  const char* port = argv[2];
  int numThreads = 1;
  if (argc == 4) {
    numThreads = boost::lexical_cast<int>(argv[3]);
    if (numThreads <= 0) {
      std::cerr << "unexpected number of threads " << numThreads << std::endl;
      return 1;
    }
  }

  std::cout << "Starting to connect" << std::endl;

  std::string connStr;
  connStr.append("Driver=SnappyData;");
  connStr.append("Server=").append(server).append(";Port=").append(port)
      .append(";User=app;Password=app");

  SQLHENV env;
  SQLHDBC conn;
  SQLHSTMT stmt;

  if (!SQL_SUCCEEDED(::SQLAllocHandle(SQL_HANDLE_ENV, nullptr, &env))) {
    SQLCHAR sqlState[6];
    SQLINTEGER errorCode;
    SQLCHAR message[8192];
    SQLSMALLINT messageLen;
    ::SQLGetDiagRec(SQL_HANDLE_ENV, env, 1, sqlState, &errorCode, message,
        8191, &messageLen);
    std::cout << "Initialization failed with SQLState=" << sqlState
        << ", ErrorCode=" << errorCode << ": " << message << std::endl;
    return 1;
  }

  if (!SQL_SUCCEEDED(
      ::SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0))) {
    SQLCHAR sqlState[6];
    SQLINTEGER errorCode;
    SQLCHAR message[8192];
    SQLSMALLINT messageLen;
    ::SQLGetDiagRec(SQL_HANDLE_ENV, env, 1, sqlState, &errorCode, message,
        8191, &messageLen);
    std::cout << "Initialization failed with SQLState=" << sqlState
        << ", ErrorCode=" << errorCode << ": " << message << std::endl;
    return 1;
  }

  ::SQLAllocHandle(SQL_HANDLE_DBC, env, &conn);

  std::cout << "Connecting to " << server << ":" << port
      << "; connection string: " << connStr << std::endl;
  if (!SQL_SUCCEEDED(
      ::SQLDriverConnect(conn, nullptr, (SQLCHAR*)connStr.c_str(),
          (SQLSMALLINT)connStr.size(), nullptr, 0, nullptr,
          SQL_DRIVER_NOPROMPT))) {
    // connection failed
    SQLCHAR sqlState[6];
    SQLINTEGER errorCode;
    SQLCHAR message[8192];
    SQLSMALLINT messageLen;
    ::SQLGetDiagRec(SQL_HANDLE_DBC, conn, 1, sqlState, &errorCode, message,
        8191, &messageLen);
    std::cout << "Connection failed with SQLState=" << sqlState
        << ", ErrorCode=" << errorCode << ": " << message << std::endl;
    return 1;
  }

  ::SQLAllocHandle(SQL_HANDLE_STMT, conn, &stmt);

  ::SQLExecDirect(stmt, (SQLCHAR*)"drop table if exists new_order", SQL_NTS);
  ::SQLExecDirect(stmt, (SQLCHAR*)"drop table if exists customer", SQL_NTS);

  // create the tables
  ::SQLExecDirect(stmt, (SQLCHAR*)"create table customer ("
      "c_w_id         integer        not null,"
      "c_d_id         integer        not null,"
      "c_id           integer        not null,"
      "c_discount     decimal(4,4),"
      "c_credit       char(2),"
      "c_last         varchar(16),"
      "c_first        varchar(16),"
      "c_credit_lim   decimal(12,2),"
      "c_balance      decimal(12,2),"
      "c_ytd_payment  float,"
      "c_payment_cnt  integer,"
      "c_delivery_cnt integer,"
      "c_street_1     varchar(20),"
      "c_street_2     varchar(20),"
      "c_city         varchar(20),"
      "c_state        char(2),"
      "c_zip          char(9),"
      "c_phone        char(16),"
      "c_since        timestamp,"
      "c_middle       char(2),"
      "c_data         varchar(500),"
      "primary key (c_w_id, c_d_id, c_id)"
      ") partition by (c_w_id) redundancy 1", SQL_NTS);
  ::SQLExecDirect(stmt, (SQLCHAR*)"create table new_order ("
      "no_w_id  integer   not null,"
      "no_d_id  integer   not null,"
      "no_o_id  integer   not null,"
      "no_name  varchar(100) not null,"
      "primary key (no_w_id, no_d_id, no_o_id)"
      ") partition by (no_w_id) colocate with (customer) redundancy 1",
      SQL_NTS);
  ::SQLExecDirect(stmt, (SQLCHAR*)"create index ndx_customer_name "
      "on customer (c_w_id, c_d_id, c_last)", SQL_NTS);
  ::SQLExecDirect(stmt, (SQLCHAR*)"create index ndx_neworder_w_id_d_id "
      "on new_order (no_w_id, no_d_id)", SQL_NTS);
  ::SQLExecDirect(stmt, (SQLCHAR*)"create index ndx_neworder_w_id_d_id_o_id "
      "on new_order (no_w_id, no_d_id, no_o_id)", SQL_NTS);

  std::cout << "Created tables\n";
  std::cout << "Will use " << numThreads << " threads for selects"
      << std::endl;

  const int numRows = 1000;

  if (!SQL_SUCCEEDED(::SQLPrepare(stmt,
      (SQLCHAR*)"insert into new_order values (?, ?, ?, ?)", SQL_NTS))) {
    printStatementError(stmt, __LINE__);
    return 2;
  }

  int id, w_id;
  SQLRETURN ret;
  ::SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0,
      &w_id, 0, nullptr);
  ::SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0,
      &w_id, 0, nullptr);
  ::SQLBindParameter(stmt, 3, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0,
      &id, 0, nullptr);

  std::cout << "Starting inserts" << std::endl;
  for (id = 1; id <= numRows; id++) {
    char name[100];
    SQLLEN nameLen = SQL_NTS;
    w_id = (id % 98);
    ::sprintf(name, "customer-with-order%d%d", id, w_id);
    ::SQLBindParameter(stmt, 4, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 0, 0,
        name, 0, &nameLen);
    ret = ::SQLExecute(stmt);
    if (!SQL_SUCCEEDED(ret)) {
      printStatementError(stmt, __LINE__);
      return 2;
    }

    SQLLEN count;
    if (!SQL_SUCCEEDED(::SQLRowCount(stmt, &count))) {
      printStatementError(stmt, __LINE__);
      return 2;
    }
    if (count != 1) {
      std::cerr << "unexpected count for single insert: " << count
          << std::endl;
      return 2;
    }
    if ((id % 500) == 0) {
      std::cout << "Completed " << id << " inserts ..." << std::endl;
    }
  }

  ::SQLPrepare(stmt, (SQLCHAR*)"SELECT * FROM new_order "
      "WHERE no_d_id = ? AND no_w_id = ? AND no_o_id = ?", SQL_NTS);

  std::cout << "Starting warmup selects" << std::endl;
  const int numRuns = RUNNING_ON_VALGRIND > 0 ? 2000 : 50000;
  int rowNum;

  ::SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0,
      &w_id, 0, nullptr);
  ::SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0,
      &w_id, 0, nullptr);
  ::SQLBindParameter(stmt, 3, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0,
      &rowNum, 0, nullptr);
  // warmup for the selects
  for (int i = 1; i <= numRuns; i++) {
    rowNum = (i % numRows) + 1;
    w_id = (rowNum % 98);
    if (!SQL_SUCCEEDED(::SQLExecute(stmt))) {
      printStatementError(stmt, __LINE__);
      return 2;
    }

    int numResults = 0;
    while (SQL_SUCCEEDED(::SQLFetch(stmt))) {
      int o_id;
      //char* name = (char*)::malloc(100 * sizeof(char));
      ::SQLGetData(stmt, 1, SQL_C_LONG, &o_id, sizeof(o_id), nullptr);
      //::SQLGetData(stmt, 2, SQL_C_CHAR, name, 100, nullptr);
      //::free(name);
      numResults++;
    }
    ::SQLCloseCursor(stmt);
    if (numResults == 0) {
      std::cerr << "unexpected 0 results for w_id, d_id " << w_id << std::endl;
      return 2;
    }
    if ((i % 500) == 0) {
      std::cout << "Completed " << i << " warmup selects ..." << std::endl;
    }
  }

  std::cout << "Starting timed selects with " << numThreads << " threads"
      << std::endl;
  // timed runs
  boost::barrier barrier(numThreads);
  std::vector<std::thread> tasks;

  if (numThreads > 1) {
    // create the other threads
    for (int i = 1; i < numThreads; i++) {
      tasks.emplace_back(&SelectTask::run,
          SelectTask(env, connStr, barrier, numRows, numRuns));
    }
  }
  barrier.wait();
  auto start =  std::chrono::high_resolution_clock::now();
  for (int i = 1; i <= numRuns; i++) {
    rowNum = (i % numRows) + 1;
    w_id = (rowNum % 98);
    ::SQLExecute(stmt);

    int numResults = 0;
    while (SQL_SUCCEEDED(::SQLFetch(stmt))) {
      int o_id;
      //char* name = (char*)::malloc(100 * sizeof(char));
      ::SQLGetData(stmt, 1, SQL_C_LONG, &o_id, sizeof(o_id), nullptr);
      //::SQLGetData(stmt, 2, SQL_C_CHAR, name, 100, nullptr);
      //::free(name);
      numResults++;
    }
    ::SQLCloseCursor(stmt);
    if (numResults == 0) {
      std::cerr << "unexpected 0 results for w_id, d_id " << w_id << std::endl;
    }
  }
  if (numThreads > 1) {
    // wait for other threads to join
    while (!tasks.empty()) {
      tasks.back().join();
      tasks.pop_back();
    }
  }
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
      end - start);

  std::cout << "Time taken: " << (duration.count() / 1000000.0) << "ms"
      << std::endl;

  ::SQLFreeStmt(stmt, SQL_RESET_PARAMS);
  ::SQLExecDirect(stmt, (SQLCHAR*)"drop table if exists new_order", SQL_NTS);
  ::SQLExecDirect(stmt, (SQLCHAR*)"drop table if exists customer", SQL_NTS);

  ::SQLFreeHandle(SQL_HANDLE_STMT, stmt);
  ::SQLDisconnect(conn);
  ::SQLFreeHandle(SQL_HANDLE_DBC, conn);
  ::SQLFreeHandle(SQL_HANDLE_ENV, env);

  return 0;
}

TEST(Test44550, DISABLED_Test1) {
  const char* serverHost = ::getenv("SERVERHOST");
  if (!serverHost || serverHost[0] == '\0') serverHost = "localhost";
  const char* args[] = { "test44550", serverHost, "1527", "8" };
  std::cout << "Starting test" << std::endl;
  ASSERT_EQ(0, main1(4, args)) << "unexpected exit value";
}
