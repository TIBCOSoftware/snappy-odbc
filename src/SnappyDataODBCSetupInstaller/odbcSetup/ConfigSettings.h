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

#ifndef _CONFIGSETTINGS_H_
#define _CONFIGSETTINGS_H_


//#include "stdafx.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_set>
//#include <cctype>


/// Odbc.ini subkeys.
/// NOTE: These keys must be inserted into the vector of configuration keys during construction,
///       so add new configuration keys to the vector via InitializeConfigurationKeys routine.
#define SETTING_DRIVER          "Driver"
#define SETTING_DSN             "DSN"
#define SETTING_DESCRIPTION     "DESCRIPTION"
#define SETTING_SERVER          "SERVER"
#define SETTING_PORT            "PORT"
#define SETTING_UID             "UID"
#define SETTING_PWD             "PWD"
#define SETTING_SETUP           "Setup"
// the defined value should match the string defined OdbcInitKeys.cpp
#define SETTING_CREDENTIALMANAGER "CredentialManager"
#define SETTING_DEFAULTSCHEMA   "DefaultSchema"
#define SETTING_LOADBALANCE     "LoadBalance"
#define SETTING_AUTORECONNECT   "AutoReconnect"
#define SETTING_AQPERROR        "spark.sql.aqp.error"
#define SETTING_AQPBEHAVIOR     "spark.sql.aqp.behavior"
#define SETTING_AQPCONFIDENCE   "spark.sql.aqp.confidence"
#define SETTING_SSL             "SSLMode"
#define SETTING_SSLPROPERTIES   "SSLProperties"

// following macros are just for UI fields
#define SETTING_TRUSTSTORE      "truststore"
#define SETTING_AQPCHECK        "aqp"
#define SETTING_TWOWAYSSLCHECK  "client-auth"
#define SETTING_CLIENTCERTIFICATE "certificate"
#define SETTING_CLIENTKEYSTORE "keystore"
#define SETTING_KEYSTOREPASSWORD "keystore-password"
#define SETTING_TRUSTSTOREPASSWORD "truststore-password"
#define SETTING_CIPHERS         "cipher-suites"


typedef size_t                      gemfirexd_handle;
typedef size_t                      gemfirexd_size_t;

/// Configuration Map
typedef std::map<std::string, std::string> ConfigurationMap;

/// @brief This class encapsulates the settings for the configuration dialog.
class ConfigSettings final
{
// Public ======================================================================================
public:
    /// @brief Constructor.
    ConfigSettings();

    /// @brief Destructor.
    ///
    /// Release locally held resources.
    ~ConfigSettings();

    /// @brief Get the DSN name.
    ///
    /// @return DSN name configured.
    const std::string& GetDSN();

    /// @brief Get the Driver name.
    ///
    /// @return Driver name configured.
    const std::string& GetDriver();

    /// @brief Get the description.
    ///
    /// @return Description configured.
    const std::string& GetDescription();

    /// @brief Get the server.
    ///
    /// @return Server configured.
    const std::string& GetSERVER();

    /// @brief Get the port.
    ///
    /// @return Server configured.
    const std::string& GetPORT();

    /// @return Password configured.
    const std::string& GetPWD();

    /// @brief Get the user name.
    ///
    /// @return User name configured.
    const std::string& GetUID();

    /// @brief Get the credential-manager value.
    ///
    /// @return If credential-manager is true or false.
    const std::string& GetCREDENTIALMANAGER();

    /// @brief Get the default-schema value.
    ///
    /// @return The default schema to be used.
    const std::string& GetDEFAULTSCHEMA();

    /// @brief Get the load-balance value.
    ///
    /// @return If load-balance is true or false.
    const std::string& GetLOADBALANCE();

    /// @brief Get the auto-reconnect value.
    ///
    /// @return If auto-reconnect is true or false.
    const std::string& GetAUTORECONNECT();

    /// @brief Get the AQP ERROR value.
    ///
    /// @return AQP ERROR configured.
    const std::string& GetAQPERROR();

    /// @brief Get the AQP BEHAVIOR value.
    ///
    /// @return AQP BEHAVIOR configured.
    const std::string& GetAQPBEHAVIOR();

    /// @brief Get the AQP CONIDENCE value.
    ///
    /// @return AQP CONIDENCE configured.
    const std::string& GetAQPCONFIDENCE();

    /// @brief Get the ssl value.
    ///
    /// @return User name configured.
    const std::string& GetSSL();

    /// @brief Get the trust certificate value.
    ///
    /// @return User name configured.
    const std::string& GetSSLTRUSTCERTIFICATE();

    /// @brief Get the AQL check box value.
    ///
    /// @return User name configured.
    const std::string& GetAQPCheck();

    /// @brief Get the two way ssl check box value.
    ///
    /// @return User name configured.
    const std::string& GetTwoWaySSL();

    /// @brief Get the client certificate path value.
    ///
    /// @return User name configured.
    const std::string& GetClientCertificate();

    /// @brief Get the client keystore path value.
    ///
    /// @return User name configured.
    const std::string& GetClientKeystore();

    /// @brief Get the client keystore password value.
    ///
    /// @return User name configured.
    const std::string& GetClientKeystorePwd();

    /// @brief Get the client truststore key password value.
    ///
    /// @return User name configured.
    const std::string& GetTrustStorePassword();

    /// @brief Get the client truststore key password value.
    ///
    /// @return User name configured.
    const std::string& GetCipherSuites();

    /// @brief Set the DSN name.
    ///
    /// @param in_dsn                       DSN name to configure.
    ///
    /// @exception ErrorException
    void SetDSN(const std::string& in_dsn);

    /// @brief Set the description.
    ///
    /// @param in_description               Description to configure.
    void SetDescription(const std::string& in_description);

    /// @brief Set the PWD.
    ///
    /// @param in_pwd            Password to configure.
    void SetPWD(const std::string& in_pwd);

    /// @brief Set the SERVER.
    ///
    /// @param in_server            The server ip to configure.
    void SetSERVER(const std::string& in_server);

    /// @brief Set the PORT.
    ///
    /// @param in_port            The server port to configure.
    void SetPORT(const std::string& in_port);

    /// @param in_uid            The user id to configure.
    void SetUID(const std::string& in_uid);

    /// @param isEnabled  Whether to enable or disable credential-manager.
    void SetCREDENTIALMANAGER(const std::string& isEnabled);

    /// @param in_schema  The default-schema to configure.
    void SetDEFAULTSCHEMA(const std::string& in_schema);

    /// @param isEnabled        The load-balance to configure.
    void SetLOADBALANCE(const std::string& isEnabled);

    /// @param isEnabled  Whether to enable or disable auto-reconnect.
    void SetAUTORECONNECT(const std::string& isEnabled);

    /// @param aqpError        The aqp.error to configure.
    void SetAQPERROR(const std::string& aqpError);

    /// @param aqp.behavior        The aqp.behavior to configure.
    void SetAQPBEHAVIOR(const std::string& aqpBehavior);

    /// @param aqp.confidence      The aqp.confidence to configure.
    void SetAQPCONFIDENCE(const std::string& aqpConfidence);

    /// @param isEnabled        The SSL to configure.
    void SetSSL(const std::string& isEnabled);

    /// @brief Set the SSL Properties.
    ///
    /// @param properties            SSL properties.
    void SetSSLPropeties();
    /// @brief Set the SERVER.
    ///
    /// @param in_server            The set truststore key.
    void SetTRUSTCERTIFICATE(const std::string& in_trustcert);

    /// @param isEnabled        The set AQP checkbox.
    void SetAQLCheck(const std::string& isEnabled);


    /// @param isEnabled        The set client-auth checkbox.
    void SetTwoWayCheck(const std::string& isEnabled);

    /// @param isEnabled        The set client certificate.
    void SetClientCertificate(const std::string& in_clientCert);

    /// @param isEnabled        The set client keystore.
    void SetClientKeyStore(const std::string& in_clientKeystore);

    /// @param isEnabled        The set client keystore password.
    void SetClientKeyStorePwd(const std::string& in_clientKeystorePwd);

    /// @param isEnabled        The set trust store password.
    void SetTrustStorePassword(const std::string& in_truststorePwd);

    /// @param isEnabled        The set cipher suites.
    void SetCipherSuites(const std::string& in_ciphersuites);

    /// @brief Updates the member variables and configuration map with odbc.ini configurations.
    ///
    /// The configuration map will contain the delta, whereas the custom configuration map
    /// will be completely overwritten.
    void ReadConfigurationDriver(const char* fileName);

    /// @brief Updates the member variables and configuration map with odbc.ini configurations.
    ///
    /// The configuration map will contain the delta, whereas the custom configuration map
    /// will be completely overwritten.
    void ReadConfigurationDSN();

    /// @brief Updates the odbc.ini configurations from the configuration map.
    void WriteConfiguration(const char* fileName);

    /// @brief Clears current configuration, resets all private data members to their default
    /// values, then updates the configuration map with those configurations specified in the
    /// in_dsnConfigurationMap.
    ///
    /// (No implicit update to odbc.ini)
    ///
    /// @param in_dsnConfigurationMap       This will at least hold the DSN entry, which can be
    ///                                     used to load the other settings.
    void UpdateConfiguration(ConfigurationMap& in_dsnConfigurationMap);


    std::string toLowerCase(const std::string& src) {
      std::string dest;
      if (src.empty()) return dest;
      dest.resize(src.size());
      std::transform(src.begin(), src.end(), dest.begin(), ::tolower);
      return dest;
    }

// Private =====================================================================================
private:
    /// keys that do not directly go into registry (e.g. components of ssl-properties)
    static std::unordered_set<std::string> s_nonRegistryKeys;

    /// Configuration map of odbc.ini subkeys and values
    ConfigurationMap m_configuration;

    /// The driver for this DSN.
    std::string m_driver;

    /// The DSN name.
    std::string m_dsn;

    /// The original DSN name; used to determine if the DSN name has changed.
    std::string m_dsnOrig;

    /// @brief Reads all the settings for the given section
    ///
    /// @param sectionName                  The DSN or Driver section name
    /// @param filename                     The filename to read from, should be odbc.ini or odbcinst.ini
    ///
    /// @return A map of all settings in the section
    ConfigurationMap ReadSettings(std::string sectionName, std::string filename);
};


#endif
