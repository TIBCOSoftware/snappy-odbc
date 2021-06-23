/*
 * Copyright (c) 2018 SnappyData, Inc. All rights reserved.
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

package tests;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.Statement;
import java.util.ArrayList;
import java.util.concurrent.CyclicBarrier;

public class Test44550 {

  static class SelectTask extends Thread {

    private final String m_connStr;
    private final CyclicBarrier m_barrier;
    private final int m_numRows;
    private final int m_numRuns;

    public SelectTask(String connStr, CyclicBarrier barrier, int numRows,
        int numRuns) {
      m_connStr = connStr;
      m_barrier = barrier;
      m_numRows = numRows;
      m_numRuns = numRuns;
      start();
    }

    @Override
    public void run() {
      Connection conn;
      PreparedStatement pstmt;

      try {
        conn = DriverManager.getConnection(m_connStr);
        pstmt = conn.prepareStatement("SELECT * FROM new_order "
            + "WHERE no_d_id = ? AND no_w_id = ? AND no_o_id = ?");

        System.out.println("Starting timed selects for thread " + getId());

        // wait for all threads
        m_barrier.await();

        int rowNum, w_id;
        for (int i = 1; i <= m_numRuns; i++) {
          rowNum = (i % m_numRows) + 1;
          w_id = (rowNum % 98);
          pstmt.setInt(1, w_id);
          pstmt.setInt(2, w_id);
          pstmt.setInt(3, rowNum);

          ResultSet rs = pstmt.executeQuery();
          int numResults = 0;
          while (rs.next()) {
            int o_id = rs.getInt(1);
            assert o_id >= 0;
            numResults++;
          }
          rs.close();
          if (numResults == 0) {
            System.out.println("unexpected 0 results for w_id, d_id " + w_id);
          }
        }

        pstmt.close();
        conn.close();
      } catch (RuntimeException re) {
        throw re;
      } catch (Exception e) {
        throw new RuntimeException(e);
      }
    }
  }

  /**
   * Simple performance test given n threads of execution.
   */
  public static void main(String[] args) throws Exception {
    int argc = args.length;
    System.out.println("Using argc=" + argc);
    if (argc != 2 && argc != 3) {
      System.err.println("Usage: <script> <server> <port> [<threads>]");
      System.exit(1);
    }
    String server = args[0];
    String port = args[1];
    int numThreads = 1;
    if (argc == 3) {
      numThreads = Integer.parseInt(args[2]);
      if (numThreads <= 0) {
        System.err.println("unexpected number of threads " + numThreads);
        System.exit(1);
      }
    }

    System.out.println("Starting to connect");

    String connStr = "jdbc:snappydata://" + server + ":" + port;

    Connection conn;
    Statement stmt;
    PreparedStatement pstmt;

    System.out.println("Connecting to " + server + ":" + port +
        "; connection string: " + connStr);
    conn = DriverManager.getConnection(connStr);
    stmt = conn.createStatement();

    stmt.execute("drop table if exists new_order");
    stmt.execute("drop table if exists customer");

    // create the tables
    stmt.execute("create table customer (" +
        "c_w_id         integer        not null," +
        "c_d_id         integer        not null," +
        "c_id           integer        not null," +
        "c_discount     decimal(4,4)," +
        "c_credit       char(2)," +
        "c_last         varchar(16)," +
        "c_first        varchar(16)," +
        "c_credit_lim   decimal(12,2)," +
        "c_balance      decimal(12,2)," +
        "c_ytd_payment  float," +
        "c_payment_cnt  integer," +
        "c_delivery_cnt integer," +
        "c_street_1     varchar(20)," +
        "c_street_2     varchar(20)," +
        "c_city         varchar(20)," +
        "c_state        char(2)," +
        "c_zip          char(9)," +
        "c_phone        char(16)," +
        "c_since        timestamp," +
        "c_middle       char(2)," +
        "c_data         varchar(500)," +
        "primary key (c_w_id, c_d_id, c_id)" +
        ") partition by (c_w_id) redundancy 1");
    stmt.execute("create table new_order (" +
        "no_w_id  integer   not null," +
        "no_d_id  integer   not null," +
        "no_o_id  integer   not null," +
        "no_name  varchar(100) not null," +
        "primary key (no_w_id, no_d_id, no_o_id)" +
        ") partition by (no_w_id) colocate with (customer) redundancy 1");
    stmt.execute("create index ndx_customer_name " +
        "on customer (c_w_id, c_d_id, c_last)");
    stmt.execute("create index ndx_neworder_w_id_d_id " +
        "on new_order (no_w_id, no_d_id)");
    stmt.execute("create index ndx_neworder_w_id_d_id_o_id " +
        "on new_order (no_w_id, no_d_id, no_o_id)");

    System.out.println("Created tables");
    System.out.println("Will use " + numThreads + " threads for selects");

    final int numRows = 1000;

    pstmt = conn.prepareStatement("insert into new_order values (?, ?, ?, ?)");

    int id, w_id;

    System.out.println("Starting inserts");
    for (id = 1; id <= numRows; id++) {
      w_id = (id % 98);
      String name = "customer-with-order" + id + w_id;
      pstmt.setInt(1, w_id);
      pstmt.setInt(2, w_id);
      pstmt.setInt(3, id);
      pstmt.setString(4, name);

      int count = pstmt.executeUpdate();
      if (count != 1) {
        System.err.println("unexpected count for single insert: " + count);
        System.exit(2);
      }
      if ((id % 500) == 0) {
        System.out.println("Completed " + id + " inserts ...");
      }
    }

    pstmt = conn.prepareStatement("SELECT * FROM new_order "
        + "WHERE no_d_id = ? AND no_w_id = ? AND no_o_id = ?");

    System.out.println("Starting warmup selects");

    final int numRuns = 50000;
    int rowNum;

    // warmup for the selects
    for (int i = 1; i <= numRuns; i++) {
      rowNum = (i % numRows) + 1;
      w_id = (rowNum % 98);
      pstmt.setInt(1, w_id);
      pstmt.setInt(2, w_id);
      pstmt.setInt(3, rowNum);

      ResultSet rs = pstmt.executeQuery();
      int numResults = 0;
      while (rs.next()) {
        int o_id = rs.getInt(1);
        assert o_id >= 0;
        numResults++;
      }
      rs.close();
      if (numResults == 0) {
        System.err.println("unexpected 0 results for w_id, d_id " + w_id);
        System.exit(2);
      }
      if ((i % 500) == 0) {
        System.out.println("Completed " + i + " warmup selects ...");
      }
    }

    System.out
        .println("Starting timed selects with " + numThreads + " threads");
    // timed runs
    CyclicBarrier barrier = new CyclicBarrier(numThreads);
    ArrayList<SelectTask> tasks = new ArrayList<>(numThreads - 1);

    if (numThreads > 1) {
      // create the other threads
      for (int i = 1; i < numThreads; i++) {
        tasks.add(new SelectTask(connStr, barrier, numRows, numRuns));
      }
    }
    barrier.await();
    long start = System.nanoTime();
    for (int i = 1; i <= numRuns; i++) {
      rowNum = (i % numRows) + 1;
      w_id = (rowNum % 98);
      pstmt.setInt(1, w_id);
      pstmt.setInt(2, w_id);
      pstmt.setInt(3, rowNum);

      ResultSet rs = pstmt.executeQuery();
      int numResults = 0;
      while (rs.next()) {
        int o_id = rs.getInt(1);
        assert o_id >= 0;
        numResults++;
      }
      rs.close();
      if (numResults == 0) {
        System.out.print("unexpected 0 results for w_id, d_id " + w_id);
      }
    }
    if (numThreads > 1) {
      // wait for other threads to join
      for (SelectTask task : tasks) {
        task.join();
      }
      tasks.clear();
    }
    long end = System.nanoTime();
    double duration = end - start;

    System.out.println("Time taken: " + (duration / 1000000.0) + "ms");

    stmt.execute("drop table if exists new_order");
    stmt.execute("drop table if exists customer");

    pstmt.close();
    stmt.close();
    conn.close();
  }
}
