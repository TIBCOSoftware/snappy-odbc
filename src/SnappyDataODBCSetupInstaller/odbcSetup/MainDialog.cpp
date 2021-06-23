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

#include "stdafx.h"
#include "MainDialog.h"
#include "ConfigSettings.h"
#include "AutoArrayPtr.h"
#include "resource.h"

#include <ShlObj.h>
#include <odbcinst.h>
#include <Commdlg.h>

#define LOADBALANCEENABLEUI 1
 
 // Public ==========================================================================================
 ////////////////////////////////////////////////////////////////////////////////////////////////////
MainDialog::MainDialog()
{
  ; // Do nothing.
}

////////////////////////////////////////////////////////////////////////////////////////////////////
INT_PTR MainDialog::ActionProc(
  HWND hwndDlg,
  UINT message,
  WPARAM wParam,
  LPARAM lParam)
{
  // This static variable might be disturbing to you.
  // However, the lParam (used to pass the dialog context) is only available during the 
  // WM_INITDIALOG message.
  // All other messages (WM_COMMAND, WM_DESTROY, etc.) do not have access to this.
  // Therefore, we will preserve a static pointer accessible only within this procedure, to 
  // permit access to the context during subsequent messaging from the dialog box.
  // See Programming Windows 5th Edition pg 509
  static ConfigSettings* configSettings = nullptr;

  INT_PTR returnValue = static_cast<INT_PTR> (true);

  // Switch through the different messages that can be sent to the dialog by Windows and take the
  // appropriate action.
  switch (message)
  {
  case WM_INITDIALOG:
  {
    configSettings = reinterpret_cast<ConfigSettings*>(lParam);
    MainDialog::Initialize(hwndDlg, configSettings);
    break;
  }

  case WM_COMMAND:
  {
    // The user has done some action on the dialog box.
    WORD controlIdentifier = LOWORD(wParam);
    WORD controlNotification = HIWORD(wParam);

    switch (controlIdentifier)
    {
    case IDOK:
    {
      // OK button pressed.
      DoOkAction(hwndDlg, configSettings);
      break;
    }

    case IDCANCEL:
    {
      // Cancel button pressed.
      DoCancelAction(hwndDlg);
      break;
    }

    case IDTESTCONN:
    {
      // Test Connection button pressed.
      DoTestConnectAction(hwndDlg, configSettings);
      break;
    }

    case IDC_BROWSE:
    {
      DoBrowseAction(hwndDlg, IDC_TRUSTCERTFICATE);
      break;
    }
    case IDC_BROWSE_CLIENTCERT:
    {
      DoBrowseAction(hwndDlg, IDC_CLIENTCERTFICATE);
      break;
    }
    case IDC_BROWSE_CLIENTPRIVATEKEY:
    {
      DoBrowseAction(hwndDlg, IDC_CLIENTPRIVATEKEY);
      break;
    }

    case IDC_SSLEDIT:
    {
      EnableSSLUI(hwndDlg);
      break;
    }
    case IDC_AQP:
    {
      EnableAQPUI(hwndDlg);
    }
    case IDC_TWOWAYSSLCHECK:
    {
      EnableTwoWaySSL(hwndDlg);
    }
    case IDC_DSNEDIT:
    case IDC_UIDEDIT:
    case IDC_PWDEDIT:
    {
      // Only changes to required fields will be checked for disabling/enabling of 
      // OK button.
      if (EN_CHANGE == controlNotification)
      {
        // Enable/Disable the OK button if required fields are filled/empty.
        CheckEnableOK(hwndDlg);
      }

      break;
    }

    default:
    {
      // Unknown command.
      returnValue = static_cast<INT_PTR> (false);
      break;
    }
    }
    break;
  }

  case WM_DESTROY:
  case WM_NCDESTROY:
  {
    // WM_DESTROY - Destroy the dialog box. No action needs to be taken.
    // WM_NCDESTROY - Sent after the dialog box has been destroyed.  No action needs to be taken.
    break;
  }

  default:
  {
    // Unrecognized message.
    returnValue = static_cast<INT_PTR> (false);
    break;
  }
  }

  return returnValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MainDialog::Dispose(HWND in_dialogHandle)
{
  // Avoid compiler warnings.
  //UNUSED(in_dialogHandle);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool MainDialog::Show(
  HWND in_parentWindow,
  gemfirexd_handle in_moduleHandle,
  ConfigSettings& in_configSettings,
  bool edit_DSN)
{
  // Use the windows functions to show a dialog box which returns true if the user OK'ed it, 
  // false otherwise.

  // Show the dialog box and get if the user pressed OK or cancel.
  INT_PTR userOK;

  if (edit_DSN)
  {
    userOK = DialogBoxParam(
      reinterpret_cast<HINSTANCE>(in_moduleHandle),
      MAKEINTRESOURCE(IDD_ConnectDLG_EDIT),
      in_parentWindow,
      reinterpret_cast<DLGPROC>(ActionProc),
      reinterpret_cast<LPARAM>(&in_configSettings));
  }
  else
  {
    userOK = DialogBoxParam(
      reinterpret_cast<HINSTANCE>(in_moduleHandle),
      MAKEINTRESOURCE(IDD_ConnectDLG),
      in_parentWindow,
      reinterpret_cast<DLGPROC>(ActionProc),
      reinterpret_cast<LPARAM>(&in_configSettings));
  }

  // Return true/false depending on what the user pressed in the dialog box.
  if (1 == userOK)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

// Private =========================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////
void MainDialog::Initialize(HWND in_dialogHandle, ConfigSettings* in_configSettings)
{
  CenterDialog(in_dialogHandle);

  SetWindowText(in_dialogHandle, "SnappyData ODBC Configuration Dialog");

  if (nullptr != in_configSettings)
  {
    // Set the DSN textbox.
    SetDlgItemText(
      in_dialogHandle,
      IDC_DSNEDIT,
      (LPCSTR)in_configSettings->GetDSN().c_str());

    // Set the DSN textbox.
    SetDlgItemText(
      in_dialogHandle,
      IDC_SERVEREDIT,
      (LPCSTR)in_configSettings->GetSERVER().c_str());

    // Set the DSN textbox.
    SetDlgItemText(
      in_dialogHandle,
      IDC_PORTEDIT,
      (LPCSTR)in_configSettings->GetPORT().c_str());

    // Set the PWD textbox.
    SetDlgItemText(
      in_dialogHandle,
      IDC_PWDEDIT,
      (LPCSTR)in_configSettings->GetPWD().c_str());

    // Set the UID textbox.
    SetDlgItemText(
      in_dialogHandle,
      IDC_UIDEDIT,
      (LPCSTR)in_configSettings->GetUID().c_str());

    // Set the CredentialManager checkbox
    bool bsigned = (in_configSettings->toLowerCase(
          in_configSettings->GetCREDENTIALMANAGER()) == "true");
    CheckDlgButton(in_dialogHandle, IDC_CREDENTIALMANAGEREDIT, bsigned);

    // Set the DefaultSchema textbox.
    SetDlgItemText(
      in_dialogHandle,
      IDC_DEFAULTSCHEMAEDIT,
      (LPCSTR)in_configSettings->GetDEFAULTSCHEMA().c_str());

    // Set the SSL checkbox
    bsigned = (in_configSettings->toLowerCase(in_configSettings->GetSSL()) == "true");
    CheckDlgButton(in_dialogHandle, IDC_SSLEDIT, bsigned);

    // Set the trustcertificate textbox.
    SetDlgItemText(
      in_dialogHandle,
      IDC_TRUSTCERTFICATE,
      (LPCSTR)in_configSettings->GetSSLTRUSTCERTIFICATE().c_str());

    // Set the Load-Balance checkbox (enabled by default)
#if LOADBALANCEENABLEUI
    bsigned = (in_configSettings->toLowerCase(in_configSettings->GetLOADBALANCE()) != "false");
    CheckDlgButton(in_dialogHandle, IDC_LOADBALANCEEDIT, bsigned);
#endif

    // Set the AutoReconnect checkbox
    bsigned = (in_configSettings->toLowerCase(in_configSettings->GetAUTORECONNECT()) == "true");
    CheckDlgButton(in_dialogHandle, IDC_AUTORECONNECTEDIT, bsigned);

    InitializeAQPUI(in_dialogHandle, in_configSettings);
    InitializeTwoWaySSLUI(in_dialogHandle, in_configSettings);
    EnableSSLUI(in_dialogHandle);
    EnableAQPUI(in_dialogHandle);
    EnableTwoWaySSL(in_dialogHandle);
  }

  // Check to make sure that the required fields are filled.
  CheckEnableOK(in_dialogHandle);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MainDialog::DoCancelAction(HWND in_dialogHandle)
{
  EndDialog(in_dialogHandle, false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MainDialog::DoOkAction(HWND in_dialogHandle, ConfigSettings* in_configSettings)
{
  // End dialog at the end of the OK action, unless there is a recoverable exception.
  bool endDialog = true;

  if (nullptr != in_configSettings)
  {
    try
    {
      // Verify and set the DSN
      std::string text = MainDialog::GetEditText(IDC_DSNEDIT, in_dialogHandle);

      if (SQLValidDSN((LPCSTR)text.c_str()))
      {
        in_configSettings->SetDSN(text);
      }
      else
      {
        throw;
      }

      // Set the SERVER
      in_configSettings->SetSERVER(GetEditText(IDC_SERVEREDIT, in_dialogHandle));

      // Set the PORT
      in_configSettings->SetPORT(GetEditText(IDC_PORTEDIT, in_dialogHandle));

      // Set the PWD.
      in_configSettings->SetPWD(GetEditText(IDC_PWDEDIT, in_dialogHandle));

      // Set the UID.
      in_configSettings->SetUID(GetEditText(IDC_UIDEDIT, in_dialogHandle));

      // Set the CredentialManager checkbox.
      in_configSettings->SetCREDENTIALMANAGER(GetEditCheckBox(
          IDC_CREDENTIALMANAGEREDIT, in_dialogHandle));

      // Set the DefaultSchema.
      in_configSettings->SetDEFAULTSCHEMA(GetEditText(
          IDC_DEFAULTSCHEMAEDIT, in_dialogHandle));

      // Set the load-balance.
#if LOADBALANCEENABLEUI
      in_configSettings->SetLOADBALANCE(GetEditCheckBox(IDC_LOADBALANCEEDIT, in_dialogHandle));
#endif

      // Set the AutoReconnect checkbox.
      in_configSettings->SetAUTORECONNECT(GetEditCheckBox(
          IDC_AUTORECONNECTEDIT, in_dialogHandle));

      // Set the AQP checkbox.
      in_configSettings->SetAQLCheck(GetEditCheckBox(IDC_AQP, in_dialogHandle));

      // Set the AQP Error.
      in_configSettings->SetAQPERROR(GetEditText(IDC_AQPERROREDIT, in_dialogHandle));

      // Set the AQP Behavior.
      in_configSettings->SetAQPBEHAVIOR(GetEditText(IDC_AQPBEHAVIOREDIT, in_dialogHandle));

      // Set the AQP Confidence.
      in_configSettings->SetAQPCONFIDENCE(GetEditText(IDC_AQPCONFIDENCEEDIT, in_dialogHandle));
     
      // Check the ssl.
      auto isChecked = IsDlgButtonChecked(in_dialogHandle, IDC_SSLEDIT);
      if (isChecked == BST_CHECKED) {
        in_configSettings->SetSSL("true");
        // set the truststore
        in_configSettings->SetTRUSTCERTIFICATE(GetEditText(IDC_TRUSTCERTFICATE, in_dialogHandle));
        // set cipher suites
        in_configSettings->SetCipherSuites(GetEditText(IDC_CIPHERS, in_dialogHandle));

        isChecked = IsDlgButtonChecked(in_dialogHandle, IDC_TWOWAYSSLCHECK);
        if (isChecked == BST_CHECKED) {
          in_configSettings->SetTwoWayCheck("true");
          // set the client certificate
          in_configSettings->SetClientCertificate(GetEditText(IDC_CLIENTCERTFICATE, in_dialogHandle));
          // set the keystore path
          in_configSettings->SetClientKeyStore(GetEditText(IDC_CLIENTPRIVATEKEY, in_dialogHandle));
          // set the keystore password
          in_configSettings->SetClientKeyStorePwd(GetEditText(IDC_CLIENTPRIVATEKEYPASSWORD, in_dialogHandle));
        } else {
          in_configSettings->SetTwoWayCheck("false");
        }
        // Set the ssl-properties
        in_configSettings->SetSSLPropeties();
      } else {
        in_configSettings->SetSSL("false");
      }

      // Note: SetAQPCheck and SetTRUSTCERTIFICATE used to make entry in system DSN regitry entry
      // so that while opening DSN next time, it should pickup these value and set the UI field accordingly
      // these both nothing to do with connection properties.
      // Save the settings to the registry.
      in_configSettings->WriteConfiguration("ODBC.INI");
    } catch(const char *const str) {
      // Display the error message.
      MessageBox(
        nullptr,
        str,
        in_configSettings->GetDSN().c_str(),
        MB_ICONEXCLAMATION | MB_OK);

      endDialog = false;
    }
  }

  if (endDialog)
  {
    EndDialog(in_dialogHandle, true);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Check the lengths of the required fields.
////////////////////////////////////////////////////////////////////////////////////////////////////
void MainDialog::CheckEnableOK(HWND in_dialogHandle)
{
  std::string controlText = GetEditText(IDC_DSNEDIT, in_dialogHandle);
  bool enableOK = (0 != controlText.length());

  // If any required fields are empty, then disable the OK button.
  EnableWindow(GetDlgItem(in_dialogHandle, IDOK), enableOK);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
std::string MainDialog::GetODBCError(SQLSMALLINT handleType, SQLHANDLE handle) {
  SQLCHAR sqlState[10];
  SQLINTEGER errorCode;
  SQLCHAR errorText[1024];
  SQLSMALLINT errorLen;

  if (::SQLGetDiagRec(handleType, handle, 1, sqlState, &errorCode, errorText,
    (SQLSMALLINT)1024, &errorLen) == SQL_NO_DATA_FOUND) {
    return "Unknown Error";
  } else {
    std::string msg;
    msg.append((char*)errorText, errorLen);
    return msg;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
std::string MainDialog::CreateAndReleaseConnection(std::string driver, std::string serverHost,
  std::string serverPort, std::string user, std::string pwd, std::string useCredentialManager,
  std::string defaultSchema, std::string loadbalance, std::string autoReconnect,
  std::string aqpcheck, std::string aqpError, std::string aqpBehavior, std::string aqpConfidence,
  std::string ssl, std::string trustedCert, std::string clientAuth, std::string clientCert,
  std::string ciphers, std::string clientKeystore, std::string clientKeystorePwd) {
  SQLHENV henv;
  SQLHDBC hdbc;
  SQLRETURN retcode = ::SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

  retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
  if (retcode != SQL_SUCCESS) {
    return GetODBCError(SQL_HANDLE_ENV, henv);
  }

  retcode = ::SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
  if (retcode != SQL_SUCCESS) {
    return GetODBCError(SQL_HANDLE_DBC, hdbc);
  }

  std::string connectionString;
  connectionString.append("driver=").append(driver).append(";server=").append(serverHost)
    .append(";port=").append(serverPort).append(";user=").append(user)
    .append(";password=").append(pwd);

  if (!useCredentialManager.empty()) {
    connectionString.append(";" SETTING_CREDENTIALMANAGER "=").append(
        useCredentialManager);
  }
  if (!defaultSchema.empty()) {
    connectionString.append(";" SETTING_DEFAULTSCHEMA "=").append(defaultSchema);
  }

  if (!loadbalance.empty()) {
    connectionString.append(";" SETTING_LOADBALANCE "=").append(loadbalance);
  }
  if (!autoReconnect.empty()) {
    connectionString.append(";" SETTING_AUTORECONNECT "=").append(autoReconnect);
  }
  if (aqpcheck == "true") {
    if (!aqpError.empty()) {
      connectionString.append(";").append(SETTING_AQPERROR).append("=").append(aqpError);
    }
    if (!aqpBehavior.empty()) {
      connectionString.append(";").append(SETTING_AQPBEHAVIOR).append("=").append(aqpBehavior);
    }
    if (!aqpConfidence.empty()) {
      connectionString.append(";").append(SETTING_AQPCONFIDENCE).append("=").append(aqpConfidence);
    }
  }

  if (ssl == "true") {
    connectionString.append(";").append(SETTING_SSL).append("=").append(ssl);
    connectionString.append(";").append(SETTING_SSLPROPERTIES).append("=");
    connectionString.append(SETTING_TWOWAYSSLCHECK).append("=").append(clientAuth);
    if (!trustedCert.empty()) {
      connectionString.append(",");
      connectionString.append(SETTING_TRUSTSTORE).append("=").append(trustedCert);
    }
    if (!ciphers.empty()) {
      connectionString.append(",");
      connectionString.append(SETTING_CIPHERS).append("=").append(ciphers);
    }
    if (clientAuth == "true") {
      connectionString.append(",");
      connectionString.append(SETTING_CLIENTCERTIFICATE).append("=").append(clientCert);
      connectionString.append(",");
      connectionString.append(SETTING_CLIENTKEYSTORE).append("=").append(clientKeystore);
      if (!clientKeystorePwd.empty()) {
        connectionString.append(",");
        connectionString.append(SETTING_KEYSTOREPASSWORD).append("=").append(clientKeystorePwd);
      }
    }
  }

  retcode = ::SQLDriverConnect(hdbc, nullptr, (SQLCHAR*)connectionString.c_str(),
    (SQLSMALLINT)connectionString.size(), nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);
  if (retcode != SQL_SUCCESS) {
    return GetODBCError(SQL_HANDLE_DBC, hdbc);
  }

  retcode = ::SQLDisconnect(hdbc);
  if (retcode != SQL_SUCCESS) {
    return GetODBCError(SQL_HANDLE_DBC, hdbc);
  }

  retcode = ::SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
  if (retcode != SQL_SUCCESS) {
    return GetODBCError(SQL_HANDLE_DBC, hdbc);
  }
  retcode = ::SQLFreeHandle(SQL_HANDLE_ENV, henv);
  if (retcode != SQL_SUCCESS) {
    return GetODBCError(SQL_HANDLE_ENV, henv);
  }

  return "";
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MainDialog::DoTestConnectAction(HWND in_dialogHandle, ConfigSettings* in_configSettings)
{
  std::string dsName = GetEditText(IDC_DSNEDIT, in_dialogHandle);
  std::string serverHost = GetEditText(IDC_SERVEREDIT, in_dialogHandle);
  std::string serverPort = GetEditText(IDC_PORTEDIT, in_dialogHandle);
  std::string user = GetEditText(IDC_UIDEDIT, in_dialogHandle);
  std::string pwd = GetEditText(IDC_PWDEDIT, in_dialogHandle);
  std::string useCredentialManager = GetEditCheckBox(
      IDC_CREDENTIALMANAGEREDIT, in_dialogHandle);
  std::string defaultSchema = GetEditText(IDC_DEFAULTSCHEMAEDIT, in_dialogHandle);
  std::string loadbalance = "";
#if LOADBALANCEENABLEUI
  loadbalance = GetEditCheckBox(IDC_LOADBALANCEEDIT, in_dialogHandle);
#endif // LOAD
  std::string autoReconnect = GetEditCheckBox(IDC_AUTORECONNECTEDIT, in_dialogHandle);
  std::string aqpcheck = GetEditCheckBox(IDC_AQP, in_dialogHandle);
  std::string aqpError = GetEditText(IDC_AQPERROREDIT, in_dialogHandle);
  std::string aqpBehavior = GetEditText(IDC_AQPBEHAVIOREDIT, in_dialogHandle);
  std::string aqpConfidence = GetEditText(IDC_AQPCONFIDENCEEDIT, in_dialogHandle);
  std::string ssl = GetEditCheckBox(IDC_SSLEDIT, in_dialogHandle);
  std::string ciphers = GetEditText(IDC_CIPHERS, in_dialogHandle);
  std::string trustedCert = GetEditText(IDC_TRUSTCERTFICATE, in_dialogHandle);
  std::string client_auth = GetEditCheckBox(IDC_TWOWAYSSLCHECK, in_dialogHandle);
  std::string clientCert = GetEditText(IDC_CLIENTCERTFICATE, in_dialogHandle);
  std::string clientKeystore = GetEditText(IDC_CLIENTPRIVATEKEY, in_dialogHandle);
  std::string clientKeytstorePwd = GetEditText(IDC_CLIENTPRIVATEKEYPASSWORD, in_dialogHandle);
  
  std::string errorMsg = CreateAndReleaseConnection(in_configSettings->GetDriver(),
    serverHost, serverPort, user, pwd, useCredentialManager, defaultSchema,
    loadbalance, autoReconnect, aqpcheck, aqpError,
    aqpBehavior, aqpConfidence, ssl, trustedCert, client_auth, clientCert, ciphers,
    clientKeystore, clientKeytstorePwd);

  // Display the message.
  if (errorMsg.empty()) {
    MessageBox(
      nullptr,
      "Connection Successful",
      dsName.c_str(),
      MB_ICONINFORMATION | MB_OK);
  } else {
    std::string errorMsg2 = "Connection Failed.\n"
        "Please check server details supplied are correct and server is running.";
    errorMsg2.append("\n\nDetails:\n\n");
    if (errorMsg.find("Unknown Error") != std::string::npos) {
      errorMsg2.append("Connection failed. Unreachable server.");
    } else {
      errorMsg2.append(errorMsg);
    }
    MessageBox(
      nullptr,
      errorMsg2.c_str(),
      dsName.c_str(),
      MB_ICONERROR | MB_OK);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void MainDialog::CenterDialog(HWND in_dialogHandle)
{
  // Get the parent window, or the desktop if there is no parent.
  HWND parentWindowHandle = GetParent(in_dialogHandle);

  if (nullptr == parentWindowHandle)
  {
    parentWindowHandle = GetDesktopWindow();
  }

  RECT parentRect;
  RECT dialogRect;

  // Get the rectangles for the parent and the dialog.
  GetWindowRect(parentWindowHandle, &parentRect);
  GetWindowRect(in_dialogHandle, &dialogRect);

  // Get the height and width of the dialog.
  int width = dialogRect.right - dialogRect.left;
  int height = dialogRect.bottom - dialogRect.top;

  // Determine the top left point for the new centered position of the dialog.
  // The computations are a bit odd to avoid negative numbers, which would result in overflow.
  int leftPoint =
    ((parentRect.left * 2) + (parentRect.right - parentRect.left) - width) / 2;

  int topPoint =
    ((parentRect.top * 2) + (parentRect.bottom - parentRect.top) - height) / 2;

  // Ensure that the dialog stays on the screen.
  RECT desktopRect;

  GetWindowRect(GetDesktopWindow(), &desktopRect);

  // Horizontal adjustment.
  if (desktopRect.left > leftPoint)
  {
    leftPoint = desktopRect.left;
  }
  else if (desktopRect.right < (leftPoint + width))
  {
    leftPoint = desktopRect.right - width;
  }

  // Vertical adjustment.
  if (desktopRect.top > topPoint)
  {
    topPoint = desktopRect.top;
  }
  else if (desktopRect.bottom < (topPoint + height))
  {
    topPoint = desktopRect.top - height;
  }

  // Place the dialog.
  MoveWindow(in_dialogHandle, leftPoint, topPoint, width, height, true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
std::string MainDialog::GetEditText(int in_component, HWND in_dialogHandle)
{
  // Get the length of the selection.
  LRESULT textLength = SendDlgItemMessage(
    in_dialogHandle,
    in_component,
    WM_GETTEXTLENGTH,
    0,
    0);
  if (CB_ERR == textLength)
  {
    // An error happened, so just return an empty string.
    return "";
  }

  // Allocate a buffer for the selection.
  AutoArrayPtr<char> buffer(textLength + 1);

  int textLength2 = (int)(textLength);

  // Retrieve the selected text.
  GetDlgItemText(
    in_dialogHandle,
    in_component,
    buffer.Get(),
    textLength2 + 1);

  // Trim off spaces.
  std::string text = MainDialog::Trim(buffer.Get(), " \t");

  return text.c_str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
std::string MainDialog::Trim(const std::string& in_string, const std::string& in_what)
{
  if (0 == in_string.length())
  {
    return in_string;
  }

  gemfirexd_size_t beginning = in_string.find_first_not_of(in_what);
  gemfirexd_size_t ending = in_string.find_last_not_of(in_what);

  if (-1 == beginning)
  {
    return "";
  }

  return std::string(in_string, beginning, ending - beginning + 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
std::string MainDialog::GetEditCheckBox(int in_component, HWND in_dialogHandle)
{
  auto isEnabled = IsDlgButtonChecked(in_dialogHandle, in_component);
  return ((isEnabled == BST_CHECKED) ? "true" : "false");
}

void MainDialog::DoBrowseAction(HWND in_dialogHandle, int component)
{
  char szFile[2048];
  OPENFILENAME ofn;
  ZeroMemory(&ofn, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = in_dialogHandle;
  ofn.lpstrFilter = "PEM\0*.pem\0";// "\0PEM\0*.pem\0\0";//;// 
  ofn.nFilterIndex = 1;
  ofn.lpstrFile = szFile;
  ofn.lpstrFile[0] = '\0';
  ofn.nMaxFile = 2048;
  ofn.lpstrFileTitle = "Certificate File Selection";
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = nullptr;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
  if (GetOpenFileName(&ofn)) {
    SetDlgItemText(
      in_dialogHandle,
      component,
      ofn.lpstrFile);
  }
}

void MainDialog::EnableSSLUI(HWND in_dialogHandle)
{
  auto isChecked = IsDlgButtonChecked(in_dialogHandle, IDC_SSLEDIT);
  bool isEnabled = (isChecked == BST_CHECKED);
  EnableWindow(GetDlgItem(in_dialogHandle, IDC_CIPHERS), isEnabled);
  EnableWindow(GetDlgItem(in_dialogHandle, IDC_TRUSTCERTFICATE), isEnabled);
  EnableWindow(GetDlgItem(in_dialogHandle, IDC_BROWSE), isEnabled);
  EnableWindow(GetDlgItem(in_dialogHandle, IDC_TWOWAYSSLCHECK), isEnabled);

  //Note: Has to empty the selection on uncheck, it will update the registry entry and 
  // it will avoid set aqp properties in uncheck condition while firing query
  if (!isEnabled) {
    SetDlgItemText(
        in_dialogHandle,
        IDC_TRUSTCERTFICATE,
        (LPCSTR)"");
    SetDlgItemText(
        in_dialogHandle,
        IDC_CIPHERS,
        (LPCSTR)"");
    CheckDlgButton(in_dialogHandle, IDC_TWOWAYSSLCHECK, false);
    EnableTwoWaySSL(in_dialogHandle);
  }
}

void MainDialog::EnableAQPUI(HWND in_dialogHandle)
{
  auto isChecked = IsDlgButtonChecked(in_dialogHandle, IDC_AQP);
  bool isEnabled = ((isChecked == BST_CHECKED) ? true : false);
  EnableWindow(GetDlgItem(in_dialogHandle, IDC_AQPERROREDIT), isEnabled);
  EnableWindow(GetDlgItem(in_dialogHandle, IDC_AQPBEHAVIOREDIT), isEnabled);
  EnableWindow(GetDlgItem(in_dialogHandle, IDC_AQPCONFIDENCEEDIT), isEnabled);

  //Note: Has to empty the selection on uncheck, it will update the registry entry and 
  // it will avoid set aqp properties in uncheck condition while firing query
  if (!isEnabled) {
    SetDlgItemText(
      in_dialogHandle,
      IDC_AQPERROREDIT,
      (LPCSTR)"");
    SetDlgItemText(
      in_dialogHandle,
      IDC_AQPCONFIDENCEEDIT,
      (LPCSTR)"");
    SendMessage(GetDlgItem(in_dialogHandle, IDC_AQPBEHAVIOREDIT), CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
  }
}

void MainDialog::InitializeAQPUI(HWND in_dialogHandle, ConfigSettings* in_configSettings)
{
  bool bsigned = (in_configSettings->toLowerCase(in_configSettings->GetAQPCheck()) == "true") ? true : false;

  CheckDlgButton(in_dialogHandle, IDC_AQP, bsigned);

  // Set the AQP Behavior comboBox.
  std::vector<TCHAR*> behValVec;
  behValVec.push_back(" ");
  behValVec.push_back("do_nothing");
  behValVec.push_back("strict");
  behValVec.push_back("local_omit");
  behValVec.push_back("run_on_full_table");
  behValVec.push_back("partial_run_on_base_table");

  auto hWndComboBox = GetDlgItem(in_dialogHandle, IDC_AQPBEHAVIOREDIT);

  for (size_t i = 0; i < behValVec.size(); ++i) {
    SendMessage(hWndComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)behValVec.at(i));
  }
  // Send the CB_SETCURSEL message to display an initial item 
  //  in the selection field 
  auto setVal = in_configSettings->GetAQPBEHAVIOR();
  ptrdiff_t position = std::distance(behValVec.begin(), find(behValVec.begin(), behValVec.end(), setVal));
  SendMessage(hWndComboBox, CB_SETCURSEL, (WPARAM)position, (LPARAM)0);

  SetDlgItemText(
    in_dialogHandle,
    IDC_AQPERROREDIT,
    (LPCSTR)in_configSettings->GetAQPERROR().c_str());

  SetDlgItemText(
    in_dialogHandle,
    IDC_AQPCONFIDENCEEDIT,
    (LPCSTR)in_configSettings->GetAQPCONFIDENCE().c_str());
}

void MainDialog::EnableTwoWaySSL(HWND in_dialogHandle)
{
  auto isChecked = IsDlgButtonChecked(in_dialogHandle, IDC_TWOWAYSSLCHECK);
  bool isEnabled = ((isChecked == BST_CHECKED) ? true : false);
  EnableWindow(GetDlgItem(in_dialogHandle, IDC_CLIENTCERTFICATE), isEnabled);
  EnableWindow(GetDlgItem(in_dialogHandle, IDC_CLIENTPRIVATEKEY), isEnabled);
  EnableWindow(GetDlgItem(in_dialogHandle, IDC_BROWSE_CLIENTCERT), isEnabled);
  EnableWindow(GetDlgItem(in_dialogHandle, IDC_BROWSE_CLIENTPRIVATEKEY), isEnabled);
  EnableWindow(GetDlgItem(in_dialogHandle, IDC_CLIENTPRIVATEKEYPASSWORD), isEnabled);

  //Note: Has to empty the selection on uncheck, it will update the registry entry and 
  // it will avoid set SSL properties in uncheck condition while firing query
  if (!isEnabled) {
    SetDlgItemText(
      in_dialogHandle,
      IDC_CLIENTCERTFICATE,
      (LPCSTR)"");
    SetDlgItemText(
      in_dialogHandle,
      IDC_CLIENTPRIVATEKEY,
      (LPCSTR)"");
    SetDlgItemText(
      in_dialogHandle,
      IDC_CLIENTPRIVATEKEYPASSWORD,
      (LPCSTR)"");
  }
}
void MainDialog::InitializeTwoWaySSLUI(HWND in_dialogHandle, ConfigSettings* in_configSettings)
{
  // set the two way ssl checkbox
  bool bsigned = (in_configSettings->toLowerCase(in_configSettings->GetTwoWaySSL()) == "true");
  bool bsslchecked = (in_configSettings->toLowerCase(in_configSettings->GetSSL()) == "true");
  if (bsslchecked) 
  {
    CheckDlgButton(in_dialogHandle, IDC_TWOWAYSSLCHECK, bsigned);

    SetDlgItemText(
      in_dialogHandle,
      IDC_CLIENTCERTFICATE,
      (LPCSTR)in_configSettings->GetClientCertificate().c_str());
    SetDlgItemText(
      in_dialogHandle,
      IDC_CLIENTPRIVATEKEY,
      (LPCSTR)in_configSettings->GetClientKeystore().c_str());
    SetDlgItemText(
      in_dialogHandle,
      IDC_CLIENTPRIVATEKEYPASSWORD,
      (LPCSTR)in_configSettings->GetClientKeystorePwd().c_str());
  }
  
}

