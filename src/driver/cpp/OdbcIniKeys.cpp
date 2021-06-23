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

/**
 * OdbcIniKeys.cpp
 */

#include <ClientAttribute.h>

#include "OdbcIniKeys.h"
#include "StringFunctions.h"

#include <iostream>

using namespace io::snappydata;
using namespace io::snappydata::client;

// initialize the const char* keys in odbc.ini
const std::string OdbcIniKeys::DRIVER = "Driver";
const std::string OdbcIniKeys::DRIVERNAME = SNAPPY_DRIVER_SECTION;
const std::string OdbcIniKeys::DSN = "DSN";
const std::string OdbcIniKeys::DBNAME = "SnappyData";
const std::string OdbcIniKeys::SERVER = "Server";
const std::string OdbcIniKeys::PORT = "Port";
const std::string OdbcIniKeys::USER = "User";
const std::string OdbcIniKeys::USERNAME = "UserName";
const std::string OdbcIniKeys::UID = "UID";
const std::string OdbcIniKeys::PASSWORD = "Password";
const std::string OdbcIniKeys::PWD = "PWD";
const std::string OdbcIniKeys::CREDENTIAL_MANAGER = "CredentialManager";
const std::string OdbcIniKeys::LOAD_BALANCE = "LoadBalance";
const std::string OdbcIniKeys::SECONDARY_LOCATORS = "SecondaryLocators";
const std::string OdbcIniKeys::AUTO_RECONNECT = "AutoReconnect";
const std::string OdbcIniKeys::DEFAULT_SCHEMA = "DefaultSchema";
const std::string OdbcIniKeys::READ_TIMEOUT = "ReadTimeout";
const std::string OdbcIniKeys::KEEPALIVE_IDLE = "KeepAliveIdle";
const std::string OdbcIniKeys::KEEPALIVE_INTVL = "KeepAliveInterval";
const std::string OdbcIniKeys::KEEPALIVE_CNT = "KeepAliveCount";
const std::string OdbcIniKeys::SINGLE_HOP_ENABLED = "SingleHopEnabled";
const std::string OdbcIniKeys::SINGLE_HOP_MAX_CONNECTIONS =
    "SingleHopMaxConnections";
const std::string OdbcIniKeys::DISABLE_STREAMING = "DisableStreaming";
const std::string OdbcIniKeys::SKIP_LISTENERS = "SkipListeners";
const std::string OdbcIniKeys::SKIP_CONSTRAINT_CHECKS = "SkipConstraintChecks";
const std::string OdbcIniKeys::LOG_FILE = "LogFile";
const std::string OdbcIniKeys::LOG_LEVEL = "LogLevel";
const std::string OdbcIniKeys::SECURITY_MECHANISM = "SecurityMechanism";
const std::string OdbcIniKeys::SSL_MODE = "SSLMode";
const std::string OdbcIniKeys::SSL_PROPERTIES = "SSLProperties";
const std::string OdbcIniKeys::TX_SYNC_COMMITS = "TxSyncCommits";
const std::string OdbcIniKeys::DISABLE_TX_BATCHING = "DisableTXBatching";
const std::string OdbcIniKeys::QUERY_HDFS = "QueryHDFS";
const std::string OdbcIniKeys::DISABLE_THINCLIENT_CANCEL = "DisableCancel";
const std::string OdbcIniKeys::ROUTE_QUERY = "RouteQuery";
const std::string OdbcIniKeys::USE_BINARY_PROTOCOL = "BinaryProtocol";
const std::string OdbcIniKeys::USE_FRAMED_TRANSPORT = "FramedTransport";
const std::string OdbcIniKeys::SERVER_GROUPS = "ServerGroups";

const std::string OdbcIniKeys::AQP_ERROR = "spark.sql.aqp.error";
const std::string OdbcIniKeys::AQP_CONFIDENCE = "spark.sql.aqp.confidence";
const std::string OdbcIniKeys::AQP_BEHAVIOR = "spark.sql.aqp.behavior";

const std::string* OdbcIniKeys::ALL_PROPERTIES = nullptr;
size_t OdbcIniKeys::NUM_ALL_PROPERTIES = 0;
// population of the map is done in init()
OdbcIniKeys::KeyMap OdbcIniKeys::s_keyMap(50);
OdbcIniKeys::KeyList OdbcIniKeys::s_keyList(50);

static const char* DRIVER_NAME_LIST[] = { SNAPPY_DRIVER_SECTION, nullptr };

const ConnectionProperty& OdbcIniKeys::getIniKeyMappingAndCheck(
    const std::string& connProperty,
    std::unordered_set<std::string>& allConnProps) {
  const ConnectionProperty &prop = ConnectionProperty::getProperty(
      connProperty);
  if (allConnProps.size() > 0 && allConnProps.erase(connProperty) == 0) {
    throw GET_SQLEXCEPTION2(SQLStateMessage::INVALID_CONNECTION_PROPERTY_MSG,
        connProperty.c_str());
  }
  return prop;
}

// initialization for OdbcIniKeys members
SQLRETURN OdbcIniKeys::init() {
  uint32_t skippedProperties = 6;
  std::unordered_set<std::string> allConnProps =
      ConnectionProperty::getValidPropertyNames();
  std::string emptyProp;

  s_keyMap.clear();
  s_keyList.clear();
  // populate the map
  try {
    // first three pointers essentially are a "memory-leak" but does not
    // matter since s_keyMap is a static map
    insertKey(DRIVER, ConnectionProperty(emptyProp,
        "" /* don't display since it is added by default */,
        DRIVER_NAME_LIST, nullptr, ConnectionProperty::F_IS_DRIVER));
    insertKey(SERVER, ConnectionProperty(emptyProp,
        "Hostname or IP address of the network server to connect to", nullptr,
        "localhost",
        ConnectionProperty::F_IS_SERVER | ConnectionProperty::F_IS_UTF8));
    insertKey(PORT, ConnectionProperty(emptyProp,
        "Port of the network server to connect to", nullptr, "1527",
        ConnectionProperty::F_IS_PORT | ConnectionProperty::F_IS_UTF8));

    insertKey(USER, getIniKeyMappingAndCheck(ClientAttribute::USERNAME,
        allConnProps));
    insertKey(USERNAME, getIniKeyMappingAndCheck(ClientAttribute::USERNAME_ALT,
        allConnProps));
    insertKey(PASSWORD, getIniKeyMappingAndCheck(ClientAttribute::PASSWORD,
        allConnProps));
    // repeat names, so create directly
    insertKey(UID, ConnectionProperty(ClientAttribute::USERNAME,
        "" /* empty to skip displaying it */, nullptr, nullptr,
        ConnectionProperty::F_IS_USER | ConnectionProperty::F_IS_UTF8));
    insertKey(PWD, ConnectionProperty(ClientAttribute::PASSWORD,
        "" /* empty to skip displaying it */, nullptr, nullptr,
        ConnectionProperty::F_IS_PASSWD | ConnectionProperty::F_IS_UTF8));
    insertKey(CREDENTIAL_MANAGER, getIniKeyMappingAndCheck(
        ClientAttribute::CREDENTIAL_MANAGER, allConnProps));

    insertKey(LOG_FILE, getIniKeyMappingAndCheck(ClientAttribute::LOG_FILE,
        allConnProps));
    insertKey(LOG_LEVEL, getIniKeyMappingAndCheck(ClientAttribute::LOG_LEVEL,
        allConnProps));

    insertKey(LOAD_BALANCE, getIniKeyMappingAndCheck(
        ClientAttribute::LOAD_BALANCE, allConnProps));
    insertKey(SECONDARY_LOCATORS, getIniKeyMappingAndCheck(
        ClientAttribute::SECONDARY_LOCATORS, allConnProps));
    insertKey(AUTO_RECONNECT, getIniKeyMappingAndCheck(
        ClientAttribute::AUTO_RECONNECT, allConnProps));
    insertKey(SERVER_GROUPS, getIniKeyMappingAndCheck(
        ClientAttribute::SERVER_GROUPS, allConnProps));

    insertKey(DEFAULT_SCHEMA, getIniKeyMappingAndCheck(
        ClientAttribute::DEFAULT_SCHEMA, allConnProps));

    /* TODO: not implemented
    insertKey(SECURITY_MECHANISM, getIniKeyMappingAndCheck(
        ClientAttribute::SECURITY_MECHANISM, allConnProps));

    insertKey(SINGLE_HOP_ENABLED, getIniKeyMappingAndCheck(
        ClientAttribute::SINGLE_HOP_ENABLED, allConnProps));
    insertKey(SINGLE_HOP_MAX_CONNECTIONS, getIniKeyMappingAndCheck(
        ClientAttribute::SINGLE_HOP_MAX_CONNECTIONS, allConnProps));
    */

    insertKey(SSL_MODE, getIniKeyMappingAndCheck(
        ClientAttribute::SSL, allConnProps));
    insertKey(SSL_PROPERTIES, getIniKeyMappingAndCheck(
        ClientAttribute::SSL_PROPERTIES, allConnProps));

    insertKey(USE_FRAMED_TRANSPORT, getIniKeyMappingAndCheck(
        ClientAttribute::THRIFT_USE_FRAMED_TRANSPORT, allConnProps));
    insertKey(USE_BINARY_PROTOCOL, getIniKeyMappingAndCheck(
        ClientAttribute::THRIFT_USE_BINARY_PROTOCOL, allConnProps));

    insertKey(TX_SYNC_COMMITS, getIniKeyMappingAndCheck(
        ClientAttribute::TX_SYNC_COMMITS, allConnProps));
    insertKey(DISABLE_TX_BATCHING, getIniKeyMappingAndCheck(
        ClientAttribute::DISABLE_TX_BATCHING, allConnProps));

    insertKey(SKIP_LISTENERS, getIniKeyMappingAndCheck(
        ClientAttribute::SKIP_LISTENERS, allConnProps));
    insertKey(SKIP_CONSTRAINT_CHECKS, getIniKeyMappingAndCheck(
        ClientAttribute::SKIP_CONSTRAINT_CHECKS, allConnProps));
    insertKey(DISABLE_STREAMING, getIniKeyMappingAndCheck(
        ClientAttribute::DISABLE_STREAMING, allConnProps));
    insertKey(DISABLE_THINCLIENT_CANCEL, getIniKeyMappingAndCheck(
        ClientAttribute::DISABLE_THINCLIENT_CANCEL, allConnProps));

    insertKey(READ_TIMEOUT, getIniKeyMappingAndCheck(
        ClientAttribute::READ_TIMEOUT, allConnProps));
    insertKey(KEEPALIVE_IDLE, getIniKeyMappingAndCheck(
        ClientAttribute::KEEPALIVE_IDLE, allConnProps));
    insertKey(KEEPALIVE_INTVL, getIniKeyMappingAndCheck(
        ClientAttribute::KEEPALIVE_INTVL, allConnProps));
    insertKey(KEEPALIVE_CNT, getIniKeyMappingAndCheck(
        ClientAttribute::KEEPALIVE_CNT, allConnProps));

    insertKey(QUERY_HDFS, getIniKeyMappingAndCheck(
        ClientAttribute::QUERY_HDFS, allConnProps));
    insertKey(ROUTE_QUERY, getIniKeyMappingAndCheck(
        ClientAttribute::ROUTE_QUERY, allConnProps));

    // AQP properties
    insertKey(AQP_ERROR, ConnectionProperty(ClientAttribute::AQP_ERROR,
        "Maximum relative error for AQP", nullptr, nullptr, 0));
    insertKey(AQP_CONFIDENCE, ConnectionProperty(ClientAttribute::AQP_CONFIDENCE,
        "Confidence for error bounds for AQP", nullptr, nullptr, 0));
    insertKey(AQP_BEHAVIOR, ConnectionProperty(ClientAttribute::AQP_BEHAVIOR,
        "Action to be taken for AQP", nullptr, nullptr, 0));

  } catch (SQLException& sqle) {
    SnappyHandleBase::setGlobalException(sqle);
    return SQL_ERROR;
  } catch (std::exception& ex) {
    SnappyHandleBase::setGlobalException(__FILE__, __LINE__, ex);
    return SQL_ERROR;
  }

  if (allConnProps.size() != skippedProperties) {
    std::string reason("Unmapped property name(s) in initialization:");
    for (const auto &prop : allConnProps) {
      reason.append(" [").append(prop).append("]");
    }
    SnappyHandleBase::setGlobalException(
        GET_SQLEXCEPTION(SQLState::UNKNOWN_EXCEPTION, reason));
    return SQL_ERROR;
  }

  // now initialize the full property array using above map
  NUM_ALL_PROPERTIES = s_keyMap.size();
  std::string* allProps = new std::string[NUM_ALL_PROPERTIES];
  ALL_PROPERTIES = allProps;
  for (KeyMap::iterator it = s_keyMap.begin(); it != s_keyMap.end(); ++it) {
    *allProps++ = it->first;
  }
  return SQL_SUCCESS;
}

bool OdbcIniKeys::getConnPropertyName(const std::string& odbcPropName,
    std::string& returnConnPropName, int& returnFlags) {
  const KeyMap::const_iterator& search = s_keyMap.find(odbcPropName);
  if (search != s_keyMap.end()) {
    const ConnectionProperty &result = search->second;
    returnConnPropName = result.getPropertyName();
    returnFlags = result.getFlags();
    return true;
  } else if (odbcPropName.find("snappydata.") == 0 ||
      odbcPropName.find("spark.") == 0) {
    returnConnPropName = odbcPropName;
    returnFlags = 0;
    return true;
  } else {
    return false;
  }
}
