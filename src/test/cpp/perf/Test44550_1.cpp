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

// ODBC_simple_testConsole.cpp : Defines the entry point for the console application.
//

#include "../unit/TestHelper.h"

#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include <boost/lexical_cast.hpp>
#include <boost/thread/barrier.hpp>

void printStatementError2(SQLHSTMT stmt, int line) {
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

class SelectTask2 {
private:
  SQLHENV m_env;
  std::string& m_connStr;
  boost::barrier& m_barrier;
  const int m_numRows;
  const int m_numRuns;
  const int m_numThreads;
  const int m_crudFlag;

public:
  inline SelectTask2(SQLHENV env, std::string& connStr, boost::barrier& barrier,
      int numRows, int numRuns, int numThreads, int crudFlag) :
      m_env(env), m_connStr(connStr), m_barrier(barrier), m_numRows(numRows),
      m_numRuns(numRuns), m_numThreads(numThreads), m_crudFlag(crudFlag) {
  }

  void run() {
    SQLHDBC conn;
    SQLHSTMT stmt;

    int lowerBound;
    int upperBound;
    int range;

    int custCount = 1000;
    int prodCount = 10000;

    ::SQLAllocHandle(SQL_HANDLE_DBC, m_env, &conn);

    if (!SQL_SUCCEEDED(::SQLDriverConnect(conn, nullptr,
        (SQLCHAR*)m_connStr.c_str(), (SQLSMALLINT)m_connStr.size(),
        nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT))) {
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

    // insert table data
    if (m_crudFlag) {
      if (!SQL_SUCCEEDED(::SQLPrepare(stmt,
          (SQLCHAR*)"insert into customer values " "(?, ?)", SQL_NTS))) {
        printStatementError2(stmt, __LINE__);
      }

      int id;
      SQLRETURN ret;
      ::SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0,
          0, &id, 0, nullptr);

      std::cout << "Starting inserts into customer table" << std::endl;
      for (id = 1; id <= custCount; id++) {
        char name[100];
        SQLLEN nameLen = SQL_NTS;
        ::sprintf(name, "customer%d", id);
        ::SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
            0, 0, name, 0, &nameLen);
        ret = ::SQLExecute(stmt);
        if (!SQL_SUCCEEDED(ret)) {
          printStatementError2(stmt, __LINE__);
        }

        SQLLEN count;
        if (!SQL_SUCCEEDED(::SQLRowCount(stmt, &count))) {
          printStatementError2(stmt, __LINE__);
        }
        if (count != 1) {
          std::cerr << "unexpected count for single insert: " << count
              << std::endl;
        }
        //if ((id % 500) == 0) {
        //  std::cout << "Completed " << id << " inserts ..." << std::endl;
        //}
      }

      if (!SQL_SUCCEEDED(::SQLPrepare(stmt,
          (SQLCHAR*)"insert into product values " "(?, ?)", SQL_NTS))) {
        printStatementError2(stmt, __LINE__);
      }

      ::SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0,
          0, &id, 0, nullptr);

      std::cout << "Starting inserts into product table" << std::endl;
      for (id = 1; id <= prodCount; id++) {
        char name[100];
        SQLLEN nameLen = SQL_NTS;
        ::sprintf(name, "product%d", id);
        ::SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
            0, 0, name, 0, &nameLen);
        ret = ::SQLExecute(stmt);
        if (!SQL_SUCCEEDED(ret)) {
          printStatementError2(stmt, __LINE__);
        }

        SQLLEN count;
        if (!SQL_SUCCEEDED(::SQLRowCount(stmt, &count))) {
          printStatementError2(stmt, __LINE__);
        }
        if (count != 1) {
          std::cerr << "unexpected count for single insert: " << count
              << std::endl;
        }
        //if ((id % 500) == 0) {
        //  std::cout << "Completed " << id << " inserts ..." << std::endl;
        //}
      }

      lowerBound = 1;
      upperBound = custCount * m_numThreads;

      if (!SQL_SUCCEEDED(::SQLPrepare(stmt,
          (SQLCHAR*)"insert into new_order values " "(?, ?, ?)", SQL_NTS))) {
        printStatementError2(stmt, __LINE__);
      }

      ::SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0,
          0, &id, 0, nullptr);

      std::cout << "Starting inserts into new_order table" << std::endl;
      for (id = 1; id <= m_numRows; id++) {
        char name[100];
        SQLLEN nameLen = SQL_NTS;
        range = rand() % (upperBound - lowerBound + 1) + lowerBound;
        ::SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER,
            0, 0, &range, 0, nullptr);
        ::sprintf(name, "customer%d-with-order%d", range, id);
        ::SQLBindParameter(stmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
            0, 0, name, 0, &nameLen);
        ret = ::SQLExecute(stmt);
        if (!SQL_SUCCEEDED(ret)) {
          printStatementError2(stmt, __LINE__);
        }

        SQLLEN count;
        if (!SQL_SUCCEEDED(::SQLRowCount(stmt, &count))) {
          printStatementError2(stmt, __LINE__);
        }
        if (count != 1) {
          std::cerr << "unexpected count for single insert: " << count
              << std::endl;
        }
        //if ((id % 500) == 0) {
        //  std::cout << "Completed " << id << " inserts ..." << std::endl;
        //}
      }

      lowerBound = 1;
      upperBound = prodCount * m_numThreads;

      if (!SQL_SUCCEEDED(::SQLPrepare(stmt,
          (SQLCHAR*)"insert into order_detail values " "(?, ?, ?)", SQL_NTS))) {
        printStatementError2(stmt, __LINE__);
      }

      std::cout << "Starting inserts into order_detail table" << std::endl;
      for (id = 1; id <= m_numRows; id++) {
        ::SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER,
            0, 0, &id, 0, nullptr);
        range = rand() % (upperBound - lowerBound + 1) + lowerBound;
        ::SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER,
            0, 0, &range, 0, nullptr);
        ::SQLBindParameter(stmt, 3, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER,
            0, 0, &id, 0, nullptr);
        ret = ::SQLExecute(stmt);
        if (!SQL_SUCCEEDED(ret)) {
          printStatementError2(stmt, __LINE__);
        }

        SQLLEN count;
        if (!SQL_SUCCEEDED(::SQLRowCount(stmt, &count))) {
          printStatementError2(stmt, __LINE__);
        }
        if (count != 1) {
          std::cerr << "unexpected count for single insert: " << count
              << std::endl;
        }
        //if ((id % 500) == 0) {
        //  std::cout << "Completed " << id << " inserts ..." << std::endl;
        //}
      }
    }

    //// wait for all threads
    m_barrier.wait();

    if (!m_crudFlag) {
      ::SQLPrepare(stmt, (SQLCHAR*)"SELECT * FROM order_detail od JOIN new_order"
          " n ON od.order_id = n.no_o_id FETCH NEXT ? ROWS ONLY", SQL_NTS);

      std::cout << "Starting timed selects for thread "
          << std::this_thread::get_id() << std::endl;

      int rowNum, count;
      ::SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0,
          0, &count, 0, nullptr);
      for (int i = 1; i <= m_numRuns; i++) {
        rowNum = (i % m_numRows) + 1;
        count = (rowNum % 98);
        if (i == 100 || i == 1000 || i == 10000 || i == 20000 || i == 30000
            || i == 40000 || i == 50000) count = 100000;
        //if (count > 90000)
        //	std::cout << "count is " << count << std::endl;
        ::SQLExecute(stmt);

        ::SQLCloseCursor(stmt);
      }
    }

    ::SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    ::SQLDisconnect(conn);
    ::SQLFreeHandle(SQL_HANDLE_DBC, conn);
  }
};

int main2(int argc, const char* argv[]) {
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

  std::string connStr;
#if defined(__LP64__) || defined(_LP64)
  connStr.append("Driver=SnappyData64;");
#else
  connStr.append("Driver=SnappyData;");
#endif
  connStr.append("Server=").append(server).append(";Port=").append(port)
      .append(";User=snappyodbc;Password=snappyodbc");

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

  ::SQLExecDirect(stmt, (SQLCHAR*)"drop table if exists order_detail",
      SQL_NTS);
  ::SQLExecDirect(stmt, (SQLCHAR*)"drop table if exists product", SQL_NTS);
  ::SQLExecDirect(stmt, (SQLCHAR*)"drop table if exists new_order", SQL_NTS);
  ::SQLExecDirect(stmt, (SQLCHAR*)"drop table if exists customer", SQL_NTS);

  // create the tables
  ::SQLExecDirect(stmt, (SQLCHAR*)"create table customer ("
      "c_id           integer generated by default as identity,"
      "c_name         varchar(100)"
      ") partition by (c_id) redundancy 1", SQL_NTS);
  ::SQLExecDirect(stmt, (SQLCHAR*)"create table new_order ("
      "no_o_id  integer generated by default as identity,"
      "c_id			integer	null,"
      "no_name  varchar(100) not null"
      ") partition by (no_o_id) colocate with (customer) redundancy 1",
      SQL_NTS);

  ::SQLExecDirect(stmt, (SQLCHAR*)"create index ndx_customer_name "
      "on customer (c_id, c_name)", SQL_NTS);

  ::SQLExecDirect(stmt, (SQLCHAR*)"create index ndx_neworder_o_id "
      "on new_order (no_o_id)", SQL_NTS);

  ::SQLExecDirect(stmt,
      (SQLCHAR*)"insert into customer values (1, 'customer1')", SQL_NTS);
  ::SQLExecDirect(stmt,
      (SQLCHAR*)"insert into customer values (DEFAULT, 'customer1')", SQL_NTS);

  ::SQLExecDirect(stmt,
      (SQLCHAR*)"insert into new_order values (1, 1, 'customer1-with-order1')",
      SQL_NTS);
  ::SQLExecDirect(stmt,
      (SQLCHAR*)"insert into new_order values (DEFAULT, 'customer1-with-order1')",
      SQL_NTS);

  ::SQLExecDirect(stmt, (SQLCHAR*)"create table product ("
      "no_p_id  integer generated by default as identity,"
      "no_productname  varchar(100) not null"
      ") partition by (no_p_id) redundancy 1", SQL_NTS);
  ::SQLExecDirect(stmt,
      (SQLCHAR*)"insert into product values (1, 'product1')", SQL_NTS);
  ::SQLExecDirect(stmt,
      (SQLCHAR*)"insert into product values (DEFAULT, 'product1')", SQL_NTS);

  ::SQLExecDirect(stmt, (SQLCHAR*)"create table order_detail ("
      "order_id			integer not null,"
      "product_id		integer	not null,"
      "quantity			integer not null"
      ") partition by (order_id) colocate with (new_order) redundancy 1",
      SQL_NTS);

  std::cout << "Created tables\n";
  std::cout << "Will use " << numThreads - 1
      << " threads for inserts and selects" << std::endl;

  const int numRows = 150000;
  const int numRuns = 50000;

  // timed runs
  boost::barrier barrier(numThreads);
  std::vector<std::thread> tasks;

  if (numThreads > 1) {
    // create the other threads
    for (int i = 1; i < numThreads; i++) {
      tasks.emplace_back(&SelectTask2::run, SelectTask2(env, connStr,
          barrier, numRows, numRuns, numThreads - 1, 1));
    }
  }

  barrier.wait();

  if (numThreads > 1) {
    // wait for other threads to join
    for (int i = 1; i < numThreads; i++) {
      tasks.back().join();
      tasks.pop_back();
    }
  }

  if (numThreads > 1) {
    // create the other threads
    for (int i = 1; i < numThreads; i++) {
      tasks.emplace_back(&SelectTask2::run, SelectTask2(env, connStr,
          barrier, numRows, numRuns, numThreads - 1, 0));
    }
  }

  barrier.wait();

  auto start = std::chrono::high_resolution_clock::now();
  if (numThreads > 1) {
    // wait for other threads to join
    for (int i = 1; i < numThreads; i++) {
      tasks.back().join();
      tasks.pop_back();
    }
  }
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
      end - start);

  std::cout << "Total Time taken for Selects: "
      << (duration.count() / 1000000.0) << "ms" << std::endl;

  ::SQLFreeStmt(stmt, SQL_RESET_PARAMS);
  ::SQLExecDirect(stmt, (SQLCHAR*)"drop table if exists order_detail",
      SQL_NTS);
  ::SQLExecDirect(stmt, (SQLCHAR*)"drop table if exists product", SQL_NTS);
  ::SQLExecDirect(stmt, (SQLCHAR*)"drop table if exists new_order", SQL_NTS);
  ::SQLExecDirect(stmt, (SQLCHAR*)"drop table if exists customer", SQL_NTS);

  ::SQLFreeHandle(SQL_HANDLE_STMT, stmt);
  ::SQLDisconnect(conn);
  ::SQLFreeHandle(SQL_HANDLE_DBC, conn);
  ::SQLFreeHandle(SQL_HANDLE_ENV, env);

  return 0;
}

TEST(Test44550, DISABLED_Test2) {
  const char* serverHost = ::getenv("SERVERHOST");
  if (!serverHost || serverHost[0] == '\0') serverHost = "localhost";
  const char* args[] = { "test44550", serverHost, "1527", "8" };
  std::cout << "Starting test" << std::endl;
  ASSERT_EQ(0, main2(4, args)) << "unexpected exit value";
}
