#include <Adafruit_NeoPixel.h>

#if !(defined(ESP8266)) 
#error This code is intended to be run on the ESP8266
#endif

#define _WIFIMGR_LOGLEVEL_    4

#include <ArduinoJson.h>
#include <FS.h>

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>

#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;

#define USE_LITTLEFS      true

#include <LittleFS.h>

FS* filesystem = &LittleFS;
#define FileFS  LittleFS
#define FS_Name "LittleFS"
#define ESP_getChipId()   (ESP.getChipId())
#define ESP_DRD_USE_LITTLEFS    true
#define ESP_DRD_USE_SPIFFS      false
#define ESP_DRD_USE_EEPROM      false
#define ESP8266_DRD_USE_RTC     false
#define DOUBLERESETDETECTOR_DEBUG       true  //false

#include <ESP_DoubleResetDetector.h>

#define DRD_TIMEOUT 10
#define DRD_ADDRESS 0
DoubleResetDetector* drd;

String ssid = "ESP_" + String(ESP_getChipId(), HEX);
String password;

// SSID and PW for your Router
String Router_SSID;
String Router_Pass;

#define FORMAT_FILESYSTEM       false

#define MIN_AP_PASSWORD_SIZE    8

#define SSID_MAX_LEN            32
#define PASS_MAX_LEN            64

typedef struct
{
  char wifi_ssid[SSID_MAX_LEN];
  char wifi_pw  [PASS_MAX_LEN];
}  WiFi_Credentials;

typedef struct
{
  String wifi_ssid;
  String wifi_pw;
}  WiFi_Credentials_String;

#define NUM_WIFI_CREDENTIALS      2
#define TZNAME_MAX_LEN            50
#define TIMEZONE_MAX_LEN          50

typedef struct
{
  WiFi_Credentials  WiFi_Creds [NUM_WIFI_CREDENTIALS];
  char TZ_Name[TZNAME_MAX_LEN];     // "America/Toronto"
  char TZ[TIMEZONE_MAX_LEN];        // "EST5EDT,M3.2.0,M11.1.0"
  uint16_t checksum;
} WM_Config;

WM_Config         WM_config;

#define  CONFIG_FILENAME              F("/wifi_cred.dat")

bool initialConfig = false;

#define USE_AVAILABLE_PAGES     true
#define USE_ESP_WIFIMANAGER_NTP     true
#define USE_CLOUDFLARE_NTP          false
#define USING_CORS_FEATURE          true
#define USE_DHCP_IP     true
#define USE_CONFIGURABLE_DNS      true
#define USE_CUSTOM_AP_IP  false

#define USING_AFRICA        false
#define USING_AMERICA       true
#define USING_ANTARCTICA    false
#define USING_ASIA          false
#define USING_ATLANTIC      false
#define USING_AUSTRALIA     false
#define USING_EUROPE        false
#define USING_INDIAN        false
#define USING_PACIFIC       false
#define USING_ETC_GMT       false

IPAddress stationIP   = IPAddress(0, 0, 0, 0);
IPAddress gatewayIP   = IPAddress(192, 168, 2, 1);
IPAddress netMask     = IPAddress(255, 255, 255, 0);
IPAddress dns1IP      = gatewayIP;
IPAddress dns2IP      = IPAddress(8, 8, 8, 8);
IPAddress APStaticIP  = IPAddress(192, 168, 232, 1);
IPAddress APStaticGW  = IPAddress(192, 168, 232, 1);
IPAddress APStaticSN  = IPAddress(255, 255, 255, 0);

#include <ESP_WiFiManager.h>

#define LED_PIN    D8
#define MIC_PIN    A0

#define LED_COUNT 600

#define BASE_BPM 174.0 * 8.0

#define MAX_EFFECT_MODE 14

float beatMillis;
int effectMode=0;

// This is for the color walking routines. Adding many LEDs has made this a much longer cycle.

int firstPixelHue = 0;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_RGB + NEO_KHZ800);

uint8_t connectMultiWiFi();

WiFi_AP_IPConfig  WM_AP_IPconfig;
WiFi_STA_IPConfig WM_STA_IPconfig;

#define HTTP_REST_PORT 8080
ESP8266WebServer server(HTTP_REST_PORT);

void initAPIPConfigStruct(WiFi_AP_IPConfig &in_WM_AP_IPconfig) {
  in_WM_AP_IPconfig._ap_static_ip   = APStaticIP;
  in_WM_AP_IPconfig._ap_static_gw   = APStaticGW;
  in_WM_AP_IPconfig._ap_static_sn   = APStaticSN;
}

void initSTAIPConfigStruct(WiFi_STA_IPConfig &in_WM_STA_IPconfig) {
  in_WM_STA_IPconfig._sta_static_ip   = stationIP;
  in_WM_STA_IPconfig._sta_static_gw   = gatewayIP;
  in_WM_STA_IPconfig._sta_static_sn   = netMask;
  in_WM_STA_IPconfig._sta_static_dns1 = dns1IP;
  in_WM_STA_IPconfig._sta_static_dns2 = dns2IP;
}

void displayIPConfigStruct(WiFi_STA_IPConfig in_WM_STA_IPconfig) {
  LOGERROR3(F("stationIP ="), in_WM_STA_IPconfig._sta_static_ip, ", gatewayIP =", in_WM_STA_IPconfig._sta_static_gw);
  LOGERROR1(F("netMask ="), in_WM_STA_IPconfig._sta_static_sn);
  LOGERROR3(F("dns1IP ="), in_WM_STA_IPconfig._sta_static_dns1, ", dns2IP =", in_WM_STA_IPconfig._sta_static_dns2);
}

void configWiFi(WiFi_STA_IPConfig in_WM_STA_IPconfig) {
    WiFi.config(in_WM_STA_IPconfig._sta_static_ip, in_WM_STA_IPconfig._sta_static_gw, in_WM_STA_IPconfig._sta_static_sn, in_WM_STA_IPconfig._sta_static_dns1, in_WM_STA_IPconfig._sta_static_dns2);  
}

uint8_t connectMultiWiFi(void) {
  
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
    if ( (String(WM_config.WiFi_Creds[i].wifi_ssid) != "") && (strlen(WM_config.WiFi_Creds[i].wifi_pw) >= MIN_AP_PASSWORD_SIZE) ) {
      LOGERROR3(F("* Additional SSID = "), WM_config.WiFi_Creds[i].wifi_ssid, F(", PW = "), WM_config.WiFi_Creds[i].wifi_pw );
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

void printLocalTime() {
  static time_t now;
  
  now = time(nullptr);
  
  if ( now > 1451602800 )
  {
    Serial.print("Local Date/Time: ");
    Serial.print(ctime(&now));
  }
}

void heartBeatPrint()
{
  printLocalTime();
  static int num = 1;

  if (WiFi.status() == WL_CONNECTED)
    Serial.print(F("H"));        // H means connected to WiFi
  else
    Serial.print(F("F"));        // F means not connected to WiFi

  if (num == 80)
  {
    Serial.println();
    num = 1;
  }
  else if (num++ % 10 == 0)
  {
    Serial.print(F(" "));
  }
}

void check_WiFi(void) {
  if ( (WiFi.status() != WL_CONNECTED) ) {
    Serial.println("\nWiFi lost. Call connectMultiWiFi in loop");
    connectMultiWiFi();
  }
}

void check_status(void) {
  static ulong checkstatus_timeout  = 0;
  static ulong checkwifi_timeout    = 0;

  static ulong current_millis;

#define WIFICHECK_INTERVAL    1000L
#define HEARTBEAT_INTERVAL    60000L

  current_millis = millis();

  // Check WiFi every WIFICHECK_INTERVAL (1) seconds.
  if ((current_millis > checkwifi_timeout) || (checkwifi_timeout == 0)) {
    check_WiFi();
    checkwifi_timeout = current_millis + WIFICHECK_INTERVAL;
  }

  // Print hearbeat every HEARTBEAT_INTERVAL (10) seconds.
  if ((current_millis > checkstatus_timeout) || (checkstatus_timeout == 0)) {
    heartBeatPrint();
    checkstatus_timeout = current_millis + HEARTBEAT_INTERVAL;
  }
}

int calcChecksum(uint8_t* address, uint16_t sizeToCalc) {
  uint16_t checkSum = 0;
  
  for (uint16_t index = 0; index < sizeToCalc; index++) {
    checkSum += * ( ( (byte*) address ) + index);
  }

  return checkSum;
}


bool loadConfigData()
{
  File file = FileFS.open(CONFIG_FILENAME, "r");
  LOGERROR(F("LoadWiFiCfgFile "));

  memset((void*) &WM_config,       0, sizeof(WM_config));
  memset((void*) &WM_STA_IPconfig, 0, sizeof(WM_STA_IPconfig));

  if (file) {
    file.readBytes((char *) &WM_config,   sizeof(WM_config));
    file.readBytes((char *) &WM_STA_IPconfig, sizeof(WM_STA_IPconfig));

    file.close();
    LOGERROR(F("OK"));

    if ( WM_config.checksum != calcChecksum( (uint8_t*) &WM_config, sizeof(WM_config) - sizeof(WM_config.checksum) ) ) {
      LOGERROR(F("WM_config checksum wrong"));
      
      return false;
    }
    
    displayIPConfigStruct(WM_STA_IPconfig);

    return true;
  } else {
    LOGERROR(F("failed"));

    return false;
  }
}

void saveConfigData() {
  File file = FileFS.open(CONFIG_FILENAME, "w");
  LOGERROR(F("SaveWiFiCfgFile "));

  if (file) {
    WM_config.checksum = calcChecksum( (uint8_t*) &WM_config, sizeof(WM_config) - sizeof(WM_config.checksum) );
    
    file.write((uint8_t*) &WM_config, sizeof(WM_config));

    displayIPConfigStruct(WM_STA_IPconfig);

    file.write((uint8_t*) &WM_STA_IPconfig, sizeof(WM_STA_IPconfig));

    file.close();
    LOGERROR(F("OK"));
  } else {
    LOGERROR(F("failed"));
  }
}

void restServerRouting() {
  server.on("/effectMode", HTTP_GET, []() {
    DynamicJsonDocument doc(512);

    doc["effectMode"] = effectMode;

    String buf;
    serializeJson(doc, buf);
    server.send(200, F("application/json"), buf);
  });
  server.on("/effectMode", HTTP_POST, []() {
    String postBody = server.arg("plain");

    DynamicJsonDocument doc(512);

    DeserializationError error = deserializeJson(doc, postBody);
    if (error) {
      Serial.print(F("Error parsing JSON "));
      Serial.println(error.c_str());
 
      String msg = error.c_str();
 
      server.send(400, F("text/html"),
        "Error parsing json body!<br>\n" + msg);
    } else {
      JsonObject postObj = doc.as<JsonObject>();
      String foo;
      serializeJson(postObj, foo);
      Serial.println(foo);

      DynamicJsonDocument responseDoc(512);
         
      if(postObj.containsKey("effectMode")) {
        int newEffectMode = postObj["effectMode"];

        if(newEffectMode >= 0 && newEffectMode <= MAX_EFFECT_MODE) {
          Serial.println("Setting effect mode to: " + String(newEffectMode));

          effectMode = newEffectMode;
          
          responseDoc["status"] = "OK";
          responseDoc["effectMode"] = newEffectMode;
          String buf;
          serializeJson(responseDoc, buf);
          server.send(201, F("application/json"), buf);
        } else {
          Serial.println("Bad newEffectMode received: " + String(newEffectMode));
          responseDoc["status"] = "FAIL";
          responseDoc["reason"] = "No such effect mode: " + String(newEffectMode);
          String buf;
          serializeJson(responseDoc, buf);
          server.send(400, F("application/json"), buf);         
        }
      } else {
        responseDoc["status"] = "FAIL";
        responseDoc["reason"] = "No effectMode argument";
        String buf;
        serializeJson(responseDoc, buf);
        server.send(400, F("application/json"),  buf);
      }
    }
  });
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup() {
  pinMode(MIC_PIN, INPUT);
  
  Serial.begin(115200);

  while(!Serial) {
    ;
  }

  Serial.print("Serial Ready!\n");

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(255); // Set BRIGHTNESS  

  Serial.print("\nStarting ConfigOnDoubleReset with DoubleResetDetect using " + String(FS_Name));
  Serial.println(" on " + String(ARDUINO_BOARD));
  Serial.println("ESP_WiFiManager Version " + String(ESP_WIFIMANAGER_VERSION));
  Serial.println("ESP_DoubleResetDetector Version " + String(ESP_DOUBLE_RESET_DETECTOR_VERSION));

  if (FORMAT_FILESYSTEM)
    FileFS.format();
  
  if (!FileFS.begin()) {
    Serial.print(FS_Name);
    Serial.println(F(" failed! AutoFormatting."));
    FileFS.format();
    if (!FileFS.begin()) {
      delay(100);
      Serial.println(F("LittleFS failed!. Please use SPIFFS or EEPROM. Stay forever"));

      while (true) {
        delay(1);
      }
    }
  } else {
    Serial.println("FS already formatted!");
  }

  drd = new DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);
  unsigned long startedAt = millis();

  initAPIPConfigStruct(WM_AP_IPconfig);
  initSTAIPConfigStruct(WM_STA_IPconfig);

  ESP_WiFiManager ESP_wifiManager("ConfigOnDoubleReset");
  ESP_wifiManager.setMinimumSignalQuality(-1);
  ESP_wifiManager.setConfigPortalChannel(0);
  ESP_wifiManager.setCORSHeader("Your Access-Control-Allow-Origin");
  
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
    if ( strlen(WM_config.TZ_Name) > 0 )
    {
      LOGERROR3(F("Current TZ_Name ="), WM_config.TZ_Name, F(", TZ = "), WM_config.TZ);
      configTime(WM_config.TZ, "pool.ntp.org"); 
    }
    else {
      Serial.println(F("Current Timezone is not set. Enter Config Portal to set."));
    } 
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

    // Stored  for later usage, from v1.1.0, but clear first
    memset(&WM_config, 0, sizeof(WM_config));

    for (uint8_t i = 0; i < NUM_WIFI_CREDENTIALS; i++)
    {
      String tempSSID = ESP_wifiManager.getSSID(i);
      String tempPW   = ESP_wifiManager.getPW(i);

      if (strlen(tempSSID.c_str()) < sizeof(WM_config.WiFi_Creds[i].wifi_ssid) - 1)
        strcpy(WM_config.WiFi_Creds[i].wifi_ssid, tempSSID.c_str());
      else
        strncpy(WM_config.WiFi_Creds[i].wifi_ssid, tempSSID.c_str(), sizeof(WM_config.WiFi_Creds[i].wifi_ssid) - 1);

      if (strlen(tempPW.c_str()) < sizeof(WM_config.WiFi_Creds[i].wifi_pw) - 1)
        strcpy(WM_config.WiFi_Creds[i].wifi_pw, tempPW.c_str());
      else
        strncpy(WM_config.WiFi_Creds[i].wifi_pw, tempPW.c_str(), sizeof(WM_config.WiFi_Creds[i].wifi_pw) - 1);

      // Don't permit NULL SSID and password len < MIN_AP_PASSWORD_SIZE (8)
      if ( (String(WM_config.WiFi_Creds[i].wifi_ssid) != "") && (strlen(WM_config.WiFi_Creds[i].wifi_pw) >= MIN_AP_PASSWORD_SIZE) )
      {
        LOGERROR3(F("* Add SSID = "), WM_config.WiFi_Creds[i].wifi_ssid, F(", PW = "), WM_config.WiFi_Creds[i].wifi_pw );
        wifiMulti.addAP(WM_config.WiFi_Creds[i].wifi_ssid, WM_config.WiFi_Creds[i].wifi_pw);
      }
    }

    String tempTZ   = ESP_wifiManager.getTimezoneName();

    if (strlen(tempTZ.c_str()) < sizeof(WM_config.TZ_Name) - 1)
      strcpy(WM_config.TZ_Name, tempTZ.c_str());
    else
      strncpy(WM_config.TZ_Name, tempTZ.c_str(), sizeof(WM_config.TZ_Name) - 1);

    const char * TZ_Result = ESP_wifiManager.getTZ(WM_config.TZ_Name);
    
    if (strlen(TZ_Result) < sizeof(WM_config.TZ) - 1)
      strcpy(WM_config.TZ, TZ_Result);
    else
      strncpy(WM_config.TZ, TZ_Result, sizeof(WM_config.TZ_Name) - 1);
         
    if ( strlen(WM_config.TZ_Name) > 0 ) {
      LOGERROR3(F("Saving current TZ_Name ="), WM_config.TZ_Name, F(", TZ = "), WM_config.TZ);
      configTime(WM_config.TZ, "pool.ntp.org"); 
    } else {
      LOGERROR(F("Current Timezone Name is not set. Enter Config Portal to set."));
    }

    ESP_wifiManager.getSTAStaticIPConfig(WM_STA_IPconfig);

    saveConfigData();
  }
  
  Serial.println("Stored: SSID = " + Router_SSID + ", Pass = " + Router_Pass);

  startedAt = millis();

  if (!initialConfig)
  {
    // Load stored data, the addAP ready for MultiWiFi reconnection
    if (!configDataLoaded)
      loadConfigData();

    for (uint8_t i = 0; i < NUM_WIFI_CREDENTIALS; i++)
    {
      // Don't permit NULL SSID and password len < MIN_AP_PASSWORD_SIZE (8)
      if ( (String(WM_config.WiFi_Creds[i].wifi_ssid) != "") && (strlen(WM_config.WiFi_Creds[i].wifi_pw) >= MIN_AP_PASSWORD_SIZE) )
      {
        LOGERROR3(F("* Add SSID = "), WM_config.WiFi_Creds[i].wifi_ssid, F(", PW = "), WM_config.WiFi_Creds[i].wifi_pw );
        wifiMulti.addAP(WM_config.WiFi_Creds[i].wifi_ssid, WM_config.WiFi_Creds[i].wifi_pw);
      }
    }

    if ( WiFi.status() != WL_CONNECTED )
    {
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
  
  beatMillis = (1.0 / (BASE_BPM / 60.0)) * 1000.0;
  
  Serial.print("Beat should be every: ");
  Serial.print(beatMillis);
  Serial.print(" milliseconds\n");

  // Set server routing
  restServerRouting();
  // Set not found response
  server.onNotFound(handleNotFound);
  // Start server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  int micSensor = 0;

  drd->loop();
  server.handleClient();
  
  micSensor = analogRead(MIC_PIN);
  Serial.print("mic val: ");
  Serial.print(micSensor);
  Serial.print("\n");

  if (Serial.available() > 0) {
    int input = Serial.read();

    Serial.print("Received ");
    Serial.print(input);
    Serial.print(" on serial\n");

    if(input != -1) {
      if(input == '+' && effectMode <  MAX_EFFECT_MODE) {
        effectMode++; 
      } else if(input == '-' && effectMode > 0) {
        effectMode--;
      }
      
      Serial.print("Switching to effect mode ");
      Serial.print(effectMode);
      Serial.print("\n");
    }
  }

  switch(effectMode) {
    case 0:
      color_walk_loop();
      break;
    case 1:
      greyscale_walk_loop();
      break;
    case 2:
      greyscale_random_loop();
      break;
    case 3:
      solid_color_walk_loop(0, 0, 255);
      break;
    case 4:
      solid_color_walk_loop(0, 255, 0);
      break;
    case 5:
      solid_color_walk_loop(255, 0, 0);
      break;
    case 6:
      nearly_solid_color(192, 192, 192);
      break;
    case 7:
      nearly_solid_color(192, 0, 0);
      break;
    case 8:
      nearly_solid_color(0, 192, 0);
      break;
    case 9:
      nearly_solid_color(0, 0, 192);
      break;
    case 10:
      solid_color(255, 255, 255);
      break; 
    case 11:
      breathe(255, 255, 255);
      break;
    case 12:
      breathe(255, 0, 0);
      break;
    case 13:
      breathe(0, 255, 0);
      break;
    case 14:
      breathe(0,  0, 255);
      break;
    default:
      // noop
      strip.clear();
      strip.show();
      delay(beatMillis * 4);
      break;
  }
}

void breathe(int red, int green, int blue) {
  Serial.print("Entering breathe with color: ");
  Serial.print((red << 16) + (green << 8) + blue, HEX);
  Serial.print("\n"); 

  strip.clear();
  float max_brightness = 200;
  float smoothness_pts = 192;
  float gamma = 0.18;
  float beta = 0.5;

  for(int a=0; a<smoothness_pts; a++) {
    unsigned long startMillis = millis();
    
    float intensity = (max_brightness - 20) * (exp(-(pow(((a/smoothness_pts)-beta)/gamma,2.0))/2.0));
    strip.setBrightness(intensity + 20);

    // Serial.print("Intensity is: ");
    // Serial.print(intensity);
    // Serial.print("\n");
      
    for(int c=0; c<strip.numPixels();c++) {
      strip.setPixelColor(c, red, green, blue);
    }

    strip.show();
    unsigned long endMillis = millis();
    float durationMillis = endMillis - startMillis;
    // Serial.println(durationMillis);
    float leftoverMillis = beatMillis - durationMillis;
    // Serial.println(leftoverMillis);
    
    // Serial.println(beatMillis);
    
    if(leftoverMillis > 0) {     
      delay(round(leftoverMillis));
    }
  }

  // Reset the max brightness!
  
  strip.setBrightness(255);
   
}


void solid_color(int red, int green, int blue) {
  Serial.print("Entering solid color with color: ");
  Serial.print((red << 16) + (green << 8) + blue, HEX);
  Serial.print("\n");

  if(red != 255 && green != 255 && blue != 255) {
    // Power can go crazy here!!! 
    if(red > 200) {
      red = 200;
    }

    if(green > 200) {
      green = 200;
    }

    if(blue > 200) {
      blue = 200;
    }
  }
  
  strip.clear();
  for(int c=0;c<strip.numPixels();c++) {
    strip.setPixelColor(c, red, green, blue);
  }
  strip.show();
  delay(beatMillis * 4 * 4);
}


void nearly_solid_color(int red, int green, int blue) {
  Serial.print("Entering nearly solid color with color: ");
  Serial.print((red << 16) + (green << 8) + blue, HEX);
  Serial.print("\n");

  for(int a=0; a<4; a++) {
    for(int b=0; b<4; b++) {
      unsigned long startMillis = millis();
      strip.clear();
      for(int c=0; c<strip.numPixels();c++) {
        if(random(3) != 0) {
          strip.setPixelColor(c+1, red, green, blue);
          c++;
        } else if(random(3) != 0) {
          strip.setPixelColor(c, red, green, blue);
        }
      }
      strip.show();
      unsigned long endMillis = millis();
      float durationMillis = endMillis - startMillis;
      float leftoverMillis = beatMillis - durationMillis;
      
      if(leftoverMillis > 0) {     
        delay(round(leftoverMillis));
      } 
    }
  }
}

void solid_color_walk_loop(int red, int green, int blue) {
  Serial.print("Entering solid color walk loop with color: ");
  Serial.print((red << 16) + (green << 8) + blue, HEX);
  Serial.print("\n");
  
  for(int a=0; a<4; a++) {
    for(int b=0; b<4; b++) {
      unsigned long startMillis = millis();
      strip.clear();
      for(int c=b; c<strip.numPixels(); c += 1) {
        // Every 3rd pixel will be shifted forward 1
        // or maybe they just don't show at all?
        
        if(random(3) != 0) {
          strip.setPixelColor(c+1, red, green, blue);
          c+=4;
        } else if(random(3) != 0) {
          strip.setPixelColor(c, red, green, blue);
        }
      }
      strip.show();
      unsigned long endMillis = millis();
      float durationMillis = endMillis - startMillis;
      float leftoverMillis = beatMillis - durationMillis;
      
      if(leftoverMillis > 0) {     
        delay(round(leftoverMillis));
      } 
    }
  }
}

void color_walk_loop() {
  Serial.println("Entering color_walk_loop");
  
  for(int a=0; a<4; a++) {
    for(int b=0; b<4; b++) {
      unsigned long startMillis = millis();
      strip.clear();
      for(int c=b; c<strip.numPixels(); c += 1) {
        int hue = (firstPixelHue + c * 65536L / strip.numPixels()) % 65536;
        // Serial.print("Pixel hue: ");
        // Serial.println(hue);
        uint32_t color = strip.gamma32(strip.ColorHSV(hue)); 

        // Every 3rd pixel will be shifted forward 1
        // or maybe they just don't show at all?
        
        if(random(3) != 0) {
          strip.setPixelColor(c+1, color);
          c+=4;
        } else if(random(3) != 0) {
          strip.setPixelColor(c, color);
        }
      }
      strip.show();

      // This is what causes the overall "color walk" wherein it
      // walks the starting color through the entire strip, one 
      // LED at a time.
      
      firstPixelHue += 65536 / (LED_COUNT - 1);
      if(firstPixelHue >= 65536) {
        firstPixelHue = firstPixelHue % 65536;
      }
      // Serial.print("firstPixelHue: ");
      // Serial.println(firstPixelHue);
      
      unsigned long endMillis = millis();
      float durationMillis = endMillis - startMillis;
      float leftoverMillis = beatMillis - durationMillis;
      
      if(leftoverMillis > 0) {     
        delay(round(leftoverMillis));
      } 
    }
  }  
}

void greyscale_walk_loop() {
  Serial.println("Entering greyscale_walk_loop");
  int firstPixelHue = 0;
  for(int a=0; a<4; a++) {
    for(int b=0; b<4; b++) {
      unsigned long startMillis = millis();
      strip.clear();
      for(int c=b; c<strip.numPixels(); c += 1) {
        // Every 3rd pixel will be shifted forward 1
        // or maybe they just don't show at all?
        
        if(random(3) != 0) {
          strip.setPixelColor(c+1, 255, 255, 255);
          c+=4;
        } else if(random(3) != 0) {
          strip.setPixelColor(c, 255, 255, 255);
        }
      }
      strip.show();

      // This is what causes the overall "color walk" wherein it
      // walks the starting color through the entire strip, one 
      // LED at a time.
      
      firstPixelHue += 65536 / (LED_COUNT - 1);
      unsigned long endMillis = millis();
      float durationMillis = endMillis - startMillis;
      float leftoverMillis = beatMillis - durationMillis;

      if(leftoverMillis > 0) {     
        delay(round(leftoverMillis));
      }     
    }
  }  
}

void greyscale_random_loop() {
  Serial.println("Entering greyscale_random_loop");

  for(int a=0; a<4; a++) {
    for(int b=0; b<4; b++) {
      unsigned long startMillis = millis();
      strip.clear();
      for(int c=0; c<strip.numPixels(); c += 1) {
        int greyscale = random(5);
        
        strip.setPixelColor(c, greyscale * (256/4), greyscale * (256/4), greyscale * (256/4));
      }
      strip.show();

      unsigned long endMillis = millis();
      float durationMillis = endMillis - startMillis;
      float leftoverMillis = beatMillis - durationMillis;

      if(leftoverMillis > 0) {     
        delay(round(leftoverMillis));
      } 
    }
  }
}
