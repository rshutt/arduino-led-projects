#ifndef WM_CONFIGER_H
#define WM_CONFIGER_H

#include <Arduino.h>
#include <FS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WiFiMulti.h>
#include <LittleFS.h>
#include <ESP_DoubleResetDetector.h>
#include <ESP_WiFiManager.h>

#if !(defined(ESP8266)) 
#error This code is intended to be run on the ESP8266
#endif

#define SSID_MAX_LEN            32
#define PASS_MAX_LEN            64
#define NUM_WIFI_CREDENTIALS      2

struct WiFi_Credentials {
  char wifi_ssid[SSID_MAX_LEN];
  char wifi_pw  [PASS_MAX_LEN];
};

struct WiFi_Credentials_String {
  String wifi_ssid;
  String wifi_pw;
};

struct WM_Config {
  WiFi_Credentials  WiFi_Creds [NUM_WIFI_CREDENTIALS];
  uint16_t checksum;
};

typedef struct WiFi_Credentials WiFi_Credentials;

typedef struct WiFi_Credentials_String WiFi_Credentials_String;

typedef struct WM_Config WM_Config;

class WM_Configer {
  WiFi_AP_IPConfig  myWM_AP_IPconfig;
  WiFi_STA_IPConfig myWM_STA_IPconfig;
  IPAddress stationIP;
  IPAddress gatewayIP;
  IPAddress netMask;
  IPAddress dns1IP;
  IPAddress dns2IP;
  IPAddress APStaticIP;
  IPAddress APStaticGW;
  IPAddress APStaticSN;
  bool initialConfig;
  WM_Config         myWM_config;
  String Router_SSID;
  String Router_Pass;
  DoubleResetDetector* drd;

  String ssid;
  String password;
    
  ESP8266WiFiMulti wifiMulti;
  FS* filesystem;

  public:
  WM_Configer();
  void dodrdloop();
  void config_tree();

  private:
  void initAPIPConfigStruct(WiFi_AP_IPConfig &in_WM_AP_IPconfig);
  void initSTAIPConfigStruct(WiFi_STA_IPConfig &in_WM_STA_IPconfig);
  void displayIPConfigStruct(WiFi_STA_IPConfig in_WM_STA_IPconfig);
  void configWiFi(WiFi_STA_IPConfig in_WM_STA_IPconfig);
  uint8_t connectMultiWiFi();
  int calcChecksum(uint8_t* address, uint16_t sizeToCalc);
  bool loadConfigData();
  void saveConfigData();
  void format_filesystem();
};

#endif
