#ifndef WM_CONFIGER_CPP
#define WM_CONFIGER_CPP

#include <WM_Configer.h>

#if !(defined(ESP8266)) 
#error This code is intended to be run on the ESP8266
#endif

#define _WIFIMGR_LOGLEVEL_          4

#define USE_LITTLEFS                true
#define ESP_DRD_USE_LITTLEFS        true
#define ESP_DRD_USE_SPIFFS          false
#define ESP_DRD_USE_EEPROM          false
#define ESP8266_DRD_USE_RTC         false
#define DOUBLERESETDETECTOR_DEBUG   true  //false
#define DRD_TIMEOUT                 10
#define DRD_ADDRESS                 0
#define FORMAT_FILESYSTEM           false
#define MIN_AP_PASSWORD_SIZE        8
#define CONFIG_FILENAME             F("/wifi_cred.dat")
#define USE_AVAILABLE_PAGES         true
#define USE_ESP_WIFIMANAGER_NTP     true
#define USE_CLOUDFLARE_NTP          false
#define USE_DHCP_IP                 true
#define USE_CONFIGURABLE_DNS        true
#define USE_CUSTOM_AP_IP            false

WM_Configer::WM_Configer() {

  stationIP   = IPAddress(0, 0, 0, 0);
  gatewayIP   = IPAddress(192, 168, 2, 1);
  netMask     = IPAddress(255, 255, 255, 0);
  dns1IP      = gatewayIP;
  dns2IP      = IPAddress(8, 8, 8, 8);
  APStaticIP  = IPAddress(192, 168, 232, 1);
  APStaticGW  = IPAddress(192, 168, 232, 1);
  APStaticSN  = IPAddress(255, 255, 255, 0);
  initialConfig = false;
  ssid = "LED_ESP_" + String(ESP.getChipId(), HEX);
  filesystem = &LittleFS;

  format_filesystem();
  config_tree();
}

void WM_Configer::dodrdloop() {
  drd->loop();
}

void WM_Configer::initAPIPConfigStruct(WiFi_AP_IPConfig &in_WM_AP_IPconfig) {
  in_WM_AP_IPconfig._ap_static_ip   = APStaticIP;
  in_WM_AP_IPconfig._ap_static_gw   = APStaticGW;
  in_WM_AP_IPconfig._ap_static_sn   = APStaticSN;
}

void WM_Configer::initSTAIPConfigStruct(WiFi_STA_IPConfig &in_WM_STA_IPconfig) {
  in_WM_STA_IPconfig._sta_static_ip   = stationIP;
  in_WM_STA_IPconfig._sta_static_gw   = gatewayIP;
  in_WM_STA_IPconfig._sta_static_sn   = netMask;
  in_WM_STA_IPconfig._sta_static_dns1 = dns1IP;
  in_WM_STA_IPconfig._sta_static_dns2 = dns2IP;
}

void WM_Configer::displayIPConfigStruct(WiFi_STA_IPConfig in_WM_STA_IPconfig) {
  LOGERROR3(F("stationIP ="), in_WM_STA_IPconfig._sta_static_ip, ", gatewayIP =", in_WM_STA_IPconfig._sta_static_gw);
  LOGERROR1(F("netMask ="), in_WM_STA_IPconfig._sta_static_sn);
  LOGERROR3(F("dns1IP ="), in_WM_STA_IPconfig._sta_static_dns1, ", dns2IP =", in_WM_STA_IPconfig._sta_static_dns2);
}

void WM_Configer::configWiFi(WiFi_STA_IPConfig in_WM_STA_IPconfig) {
  WiFi.config(in_WM_STA_IPconfig._sta_static_ip, in_WM_STA_IPconfig._sta_static_gw, in_WM_STA_IPconfig._sta_static_sn, in_WM_STA_IPconfig._sta_static_dns1, in_WM_STA_IPconfig._sta_static_dns2);  
}

uint8_t WM_Configer::connectMultiWiFi() {
  
#define WIFI_MULTI_1ST_CONNECT_WAITING_MS       2200L
#define WIFI_MULTI_CONNECT_WAITING_MS           500L

  uint8_t status;

  LOGERROR(F("ConnectMultiWiFi with :"));

  if ( (Router_SSID != "") && (Router_Pass != "") ) {
    LOGERROR3(F("* Flash-stored Router_SSID = "), Router_SSID, F(", Router_Pass = "), Router_Pass );
    LOGERROR3(F("* Add SSID = "), Router_SSID, F(", PW = "), Router_Pass );
    wifiMulti.addAP(Router_SSID.c_str(), Router_Pass.c_str());
  }

  for (uint8_t i = 0; i < NUM_WIFI_CREDENTIALS; i++) {
    if ( (String(myWM_config.WiFi_Creds[i].wifi_ssid) != "") && (strlen(myWM_config.WiFi_Creds[i].wifi_pw) >= MIN_AP_PASSWORD_SIZE) ) {
      LOGERROR3(F("* Additional SSID = "), myWM_config.WiFi_Creds[i].wifi_ssid, F(", PW = "), myWM_config.WiFi_Creds[i].wifi_pw );
    }
  }
  LOGERROR(F("Connecting MultiWifi..."));

  int i = 0;
  status = wifiMulti.run();
  delay(WIFI_MULTI_1ST_CONNECT_WAITING_MS);

  while ( ( i++ < 20 ) && ( status != WL_CONNECTED ) ) {
    status = WiFi.status();

    if ( status == WL_CONNECTED )
      break;
    else
      delay(WIFI_MULTI_CONNECT_WAITING_MS);
  }

  if ( status == WL_CONNECTED ) {
    LOGERROR1(F("WiFi connected after time: "), i);
    LOGERROR3(F("SSID:"), WiFi.SSID(), F(",RSSI="), WiFi.RSSI());
    LOGERROR3(F("Channel:"), WiFi.channel(), F(",IP address:"), WiFi.localIP() );
  } else {
    LOGERROR(F("WiFi not connected"));

    drd->loop();

    ESP.restart();
  }
  return status;
}

int WM_Configer::calcChecksum(uint8_t* address, uint16_t sizeToCalc) {
  uint16_t checkSum = 0;
  
  for (uint16_t index = 0; index < sizeToCalc; index++) {
    checkSum += * ( ( (byte*) address ) + index);
  }

  return checkSum;
}

bool WM_Configer::loadConfigData() {
  File file = LittleFS.open(CONFIG_FILENAME, "r");
  LOGERROR(F("LoadWiFiCfgFile "));

  memset((void*) &myWM_config,       0, sizeof(myWM_config));
  memset((void*) &myWM_STA_IPconfig, 0, sizeof(myWM_STA_IPconfig));

  if (file) {
    file.readBytes((char *) &myWM_config,   sizeof(myWM_config));
    file.readBytes((char *) &myWM_STA_IPconfig, sizeof(myWM_STA_IPconfig));

    file.close();
    LOGERROR(F("OK"));

    if ( myWM_config.checksum != calcChecksum( (uint8_t*) &myWM_config, sizeof(myWM_config) - sizeof(myWM_config.checksum) ) ) {
      LOGERROR(F("WM_config checksum wrong"));
      
      return false;
    }
    
    displayIPConfigStruct(myWM_STA_IPconfig);

    return true;
  } else {
    LOGERROR(F("failed"));

    return false;
  }
}

void WM_Configer::saveConfigData() {
  File file = LittleFS.open(CONFIG_FILENAME, "w");
  LOGERROR(F("SaveWiFiCfgFile "));

  if (file) {
    myWM_config.checksum = calcChecksum( (uint8_t*) &myWM_config, sizeof(myWM_config) - sizeof(myWM_config.checksum) );
    
    file.write((uint8_t*) &myWM_config, sizeof(myWM_config));

    displayIPConfigStruct(myWM_STA_IPconfig);

    file.write((uint8_t*) &myWM_STA_IPconfig, sizeof(myWM_STA_IPconfig));

    file.close();
    LOGERROR(F("OK"));
  } else {
    LOGERROR(F("failed"));
  }
}

void WM_Configer::format_filesystem() {
  if (FORMAT_FILESYSTEM)
    LittleFS.format();
  
  if (!LittleFS.begin()) {
    Serial.println(F("LittleFS failed! AutoFormatting."));
    LittleFS.format();
    if (!LittleFS.begin()) {
      delay(100);
      Serial.println(F("LittleFS failed!. Please use SPIFFS or EEPROM. Stay forever"));
      while (true) {
        delay(1);
      }
    }
  } else {
    Serial.println("FS already formatted!");
  }
}

void WM_Configer::config_tree() {
  drd = new DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);
  unsigned long startedAt = millis();

  initAPIPConfigStruct(myWM_AP_IPconfig);
  initSTAIPConfigStruct(myWM_STA_IPconfig);

  ESP_WiFiManager ESP_wifiManager("ConfigOnDoubleReset");
  ESP_wifiManager.setMinimumSignalQuality(-1);
  ESP_wifiManager.setConfigPortalChannel(0);
  
  Router_SSID = ESP_wifiManager.WiFi_SSID();
  Router_Pass = ESP_wifiManager.WiFi_Pass();

  Serial.println("ESP Self-Stored: SSID = " + Router_SSID + ", Pass = " + Router_Pass);

  ssid.toUpperCase();
  bool configDataLoaded = false;

  if ( (Router_SSID != "") && (Router_Pass != "") ) {
    LOGERROR3(F("* Add SSID = "), Router_SSID, F(", PW = "), Router_Pass);
    wifiMulti.addAP(Router_SSID.c_str(), Router_Pass.c_str());

    ESP_wifiManager.setConfigPortalTimeout(120); //If no access point name has been previously entered disable timeout.
    Serial.println("Got stored Credentials. Timeout 120s for Config Portal");
  }

  if(loadConfigData()) {
    configDataLoaded = true;
    ESP_wifiManager.setConfigPortalTimeout(120); //If no access point name has been previously entered disable timeout.
    Serial.println(F("Got stored Credentials. Timeout 120s for Config Portal")); 
  } else {
    Serial.println("Open Config Portal without Timeout: No stored Credentials.");
    initialConfig = true;
  }

  if (drd->detectDoubleReset()) {
    // DRD, disable timeout.
    ESP_wifiManager.setConfigPortalTimeout(0);

    Serial.println("Open Config Portal without Timeout: Double Reset Detected");
    initialConfig = true;
  }

  if (initialConfig) {
    Serial.print("Starting configuration portal @ ");
    Serial.print(F("192.168.4.1"));
    Serial.print(F(", SSID = "));
    Serial.print(ssid);
    Serial.print(F(", PWD = "));
    Serial.println(password);

    if (!ESP_wifiManager.startConfigPortal((const char *) ssid.c_str()))
      Serial.println("Not connected to WiFi but continuing anyway.");
    else {
      Serial.println("WiFi connected...yeey :)");
    }

    memset(&myWM_config, 0, sizeof(myWM_config));

    for (uint8_t i = 0; i < NUM_WIFI_CREDENTIALS; i++) {
      String tempSSID = ESP_wifiManager.getSSID(i);
      String tempPW   = ESP_wifiManager.getPW(i);

      if (strlen(tempSSID.c_str()) < sizeof(myWM_config.WiFi_Creds[i].wifi_ssid) - 1)
        strcpy(myWM_config.WiFi_Creds[i].wifi_ssid, tempSSID.c_str());
      else
        strncpy(myWM_config.WiFi_Creds[i].wifi_ssid, tempSSID.c_str(), sizeof(myWM_config.WiFi_Creds[i].wifi_ssid) - 1);

      if (strlen(tempPW.c_str()) < sizeof(myWM_config.WiFi_Creds[i].wifi_pw) - 1)
        strcpy(myWM_config.WiFi_Creds[i].wifi_pw, tempPW.c_str());
      else
        strncpy(myWM_config.WiFi_Creds[i].wifi_pw, tempPW.c_str(), sizeof(myWM_config.WiFi_Creds[i].wifi_pw) - 1);

      // Don't permit NULL SSID and password len < MIN_AP_PASSWORD_SIZE (8)
      if ( (String(myWM_config.WiFi_Creds[i].wifi_ssid) != "") && (strlen(myWM_config.WiFi_Creds[i].wifi_pw) >= MIN_AP_PASSWORD_SIZE) ) {
        LOGERROR3(F("* Add SSID = "), myWM_config.WiFi_Creds[i].wifi_ssid, F(", PW = "), myWM_config.WiFi_Creds[i].wifi_pw );
        wifiMulti.addAP(myWM_config.WiFi_Creds[i].wifi_ssid, myWM_config.WiFi_Creds[i].wifi_pw);
      }
    }

    ESP_wifiManager.getSTAStaticIPConfig(myWM_STA_IPconfig);

    saveConfigData();
  }
  
  Serial.println("Stored: SSID = " + Router_SSID + ", Pass = " + Router_Pass);

  startedAt = millis();

  if (!initialConfig) {
    // Load stored data, the addAP ready for MultiWiFi reconnection
    if (!configDataLoaded)
      loadConfigData();

    for (uint8_t i = 0; i < NUM_WIFI_CREDENTIALS; i++) {
      // Don't permit NULL SSID and password len < MIN_AP_PASSWORD_SIZE (8)
      if ( (String(myWM_config.WiFi_Creds[i].wifi_ssid) != "") && (strlen(myWM_config.WiFi_Creds[i].wifi_pw) >= MIN_AP_PASSWORD_SIZE) ) {
        LOGERROR3(F("* Add SSID = "), myWM_config.WiFi_Creds[i].wifi_ssid, F(", PW = "), myWM_config.WiFi_Creds[i].wifi_pw );
        wifiMulti.addAP(myWM_config.WiFi_Creds[i].wifi_ssid, myWM_config.WiFi_Creds[i].wifi_pw);
      }
    }

    if ( WiFi.status() != WL_CONNECTED ) {
      Serial.println("ConnectMultiWiFi in setup");

      connectMultiWiFi();
    }
  }

  Serial.print("After waiting ");
  Serial.print((float) (millis() - startedAt) / 1000L);
  Serial.print(" secs more in setup(), connection result is ");

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("connected. Local IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(ESP_wifiManager.getStatus(WiFi.status()));
  }
}

#endif
