## SnappyData ODBC Driver

This is an ODBC 3.x Driver for the SnappyData computational database based off the C++ driver available for SnappyData in the [snappy-store/native](https://github.com/TIBCOSoftware/snappy-store/tree/snappy/master/native) repository tree. It connects directly to a SnappyData cluster ("embedded mode") using it both as a storage and Apache Spark compatible computational engine. It does not support the "smart connector mode" where SnappyData is used only as a "smart" storage cluster used by another Apache Spark computational cluster as an external data source.

The locators and servers in a SnappyData cluster enable client connections using thrift protocol. The thrift IDL can be found [here](https://github.com/TIBCOSoftware/snappy-store/blob/snappy/master/gemfirexd/shared/src/main/java/io/snappydata/thrift/common/snappydata.thrift). This can be used to generate clients in a wide variety of languages supported by Apache Thrift which can use the thrift APIs directly. Optimized versions of java and C++ thrift clients are created in the snappy-store source tree with overrides for some classes for performance and/or additional features. The SnappyData JDBC client implementation builds itself on top of the thrift generated java client (with the mentioned overrides), and the SnappyData C++ driver implementation also likewise builds itself on top of the thrift generated C++ client.

The C++ driver API follows the JDBC APIs in its naming and scope while simplifying many of those APIs and removing some other APIs that are either redundant/obscure or unsupported by the server-side. C++ based applications may find using that API directly to be much more natural than the ODBC API. The ODBC driver in this source tree implements a thin layer on top of the C++ driver such that in terms of performance the difference between the two is negligible.

The ODBC driver is largely compliant with the ODBC 3.52 specification. There are some missing features notably connection pooling and asynchronous execution, which are reported by SQLGetInfo and other standard ODBC APIs that the driver managers will discover and report.

Builds for Windows 64-bit and 32-bit are available from TIBCO and can be provided for Linux 64-bit and 32-bit if required. As such the code can be built for the mentioned platforms out-of-the-box though the dependencies for the C++ driver are available out-of-the-box for selected Linux distributions that its gradle build requires. In most cases support for other distributions can be easily added by pointing to one of the available binaries or providing alternative downloads for the same. See more in the build docs of the C++ driver.

### Installation

The ODBC Driver can be installed from the MSI installer packages available for Windows, or binary tarballs for Linux (provided on request) or built from source. See the documents below for build instructions on the supported platforms.

[Build instructions for Windows platforms](docs/Building-Windows.md)

[Build instructions for Linux platforms](docs/Building-Linux.md)

MacOS platform is not supported neither tested, though the code and its dependencies should build on it. To really get going for MacOS, one will need to first build the C++ driver and its dependencies, then the ODBC driver after updating the build files appropriately.

### Features

The driver connects to the SnappyData cluster and exposes its SQL-based features including all of the extensions it provides over Apache Spark like:

* Connect all of the various data sources supported by Spark and insert/query them. Those supported out of the box include Parquet, ORC, JSON, CSV, JDBC.

* Hookup with external hive metastores supported by Spark, import or use them directly using SQL.

* Builtin Column tables for large data that are optimized for memory caching, processing with column format storage on disk providing much higher performance compared to Spark cache or similar options (apart from providing long term storage and backup options).

* Builtin Row tables for smaller data that provide many of the features of traditional databases like constraints, transactions etc. These can be stored as replicated on all servers for small reference data or partitioned for slightly larger amounts of data.

* Ability to colocate column tables and/or row tables, join them providing much higher performance (by avoiding Spark's Exchange/Shuffle operator) than possible in other Spark data sources. Many other performance improvements are available for column tables and some for row tables.

* Updates and deletes for column and row tables that is missing in other column storage data sources. Even when available, the performance implications are usually quite grave.

* Approximate Query Engine (AQP) for sampling and querying a vastly smaller subset of data providing results within specified error tolerance limits. It provides the error and confidence parameters for specifying the tolerance and provides closed-form analysis for a large number of queries and slower bootstrap analysis for others. Samples can be created and queries for any of the existing tables as well as streaming data sources using SQL.

* Creating streaming data sources ("stream tables"), register continuous queries.

* Install new JARs into the cluster providing new functionality like custom functions, stream transformation methods etc that can be done system-wide or user-specific.

* Access control using the traditional GRANT/REVOKE model not available on Spark otherwise.

* Execute scala code embedded in SQL directly using EXEC SCALA statement (subject to access control rules).

* Stream query results in chunks from lead nodes ("master" in Apache Spark) to the server and back to the ODBC client that can further break the chunks, if required, for large results (though very large results are limited by Apache Spark when transmitting from executors to master).

The ODBC driver provides a few additional features building on available server-side support:

* LoadBalance option to route to the least loaded node in the cluster.

* AutoReconnect option to automatically connect to an available server before next operation if current one fails due to network/server failure.

* CredentialManager option to lookup the passwords including those for SSL private keys from the system credential manager (keyring/kwallet in Linux).

* Provide detailed server-side stack traces for failures (and client-side stack traces too for Linux) at LogLevel >= fine.

* SSL configuration including mutual authentication with custom certificates or CA-signed.

### Documentation

<!--
[SnappyData ODBC Driver Reference](docs/Reference.md) in the docs directory for the driver specific documentation.
-->

[Setting up ODBC Driver in SnappyData documentation](https://tibcosoftware.github.io/snappydata/setting_up_odbc_driver)

[HowTo Connect Using ODBC Driver](https://tibcosoftware.github.io/snappydata/howto/connect_using_odbc_driver)

[MSDN ODBC API Reference](https://docs.microsoft.com/en-us/sql/odbc/reference/syntax/odbc-api-reference)

[SnappyData ODBC Supported API](https://tibcosoftware.github.io/snappydata/reference/API_Reference/odbc_supported_apis)

### Community

### Licensing
