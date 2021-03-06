#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiManager.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <BH1750.h>
#include <ArduinoOTA.h>

BH1750 lightMeter;

#define TasterPin     D7 //Taster gegen GND, um den Konfigurationsmodus zu aktivieren

bool LDRMODE = false;

char SendIntervalSeconds[8]  = "60";
char ccuip[16];
String Variable;
bool SerialDEBUG = true;

//WifiManager - don't touch
byte ConfigPortalTimeout = 180;
bool shouldSaveConfig        = false;
String configJsonFile        = "config.json";
bool wifiManagerDebugOutput = false;
char ip[16]      = "0.0.0.0";
char netmask[16] = "0.0.0.0";
char gw[16]      = "0.0.0.0";
boolean startWifiManager = false;

unsigned long lastSendMillis = 0;

void setup() {
  Serial.begin(115200);

  lightMeter.begin();
  LDRMODE = (lightMeter.readLightLevel() == 54612);

  Serial.println();
  String strMode =  ((LDRMODE) ? "LDR" : "BH1750");
  Serial.println("Modus: " + strMode);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(A0, INPUT);
  pinMode(TasterPin,    INPUT_PULLUP);
  digitalWrite(LED_BUILTIN, HIGH);

  if (digitalRead(TasterPin) == LOW) {
    startWifiManager = true;
    bool state = LOW;
    for (int i = 0; i < 7; i++) {
      state = !state;
      digitalWrite(LED_BUILTIN, state);
      delay(100);
    }
  }

  loadSystemConfig();

  if (doWifiConnect()) {
    printSerial("WLAN erfolgreich verbunden!");
    startOTAhandling();
  } else ESP.restart();
}

void loop() {
  ArduinoOTA.handle();

  //Überlauf der millis() nach 49 Tagen abfangen
  if (lastSendMillis > millis())
    lastSendMillis = 0;

  if ((String(SendIntervalSeconds).toInt() > 0) && (lastSendMillis == 0 || millis() - lastSendMillis > ((String(SendIntervalSeconds).toInt()) * 1000))) {
    lastSendMillis = millis();
    printSerial("Setze CCU-Wert");

    uint16_t Helligkeit = LDRMODE ? analogRead(A0) : lightMeter.readLightLevel();

    printSerial("Helligkeit = " + String(Helligkeit));
    digitalWrite(LED_BUILTIN, LOW);
    setStateCCU(String(Helligkeit));
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

void setStateCCU(String value) {
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    String url = "http://" + String(ccuip) + ":8181/x.exe?ret=dom.GetObject(%22" + Variable + "%22).State(" + value + ")";
    printSerial("URL = " + url);
    http.begin(url);
    int httpCode = http.GET();
    printSerial("httpcode = " + String(httpCode));
    if (httpCode > 0) {
      //     String payload = http.getString();
    }
    if (httpCode != 200) {
      printSerial("HTTP fail " + String(httpCode));
    }
    http.end();
  } else ESP.restart();
}

void printSerial(String text) {
  if (SerialDEBUG) {
    Serial.println(text);
  }
}

bool doWifiConnect() {
  String _ssid = WiFi.SSID();
  String _psk = WiFi.psk();

  const char* ipStr = ip; byte ipBytes[4]; parseBytes(ipStr, '.', ipBytes, 4, 10);
  const char* netmaskStr = netmask; byte netmaskBytes[4]; parseBytes(netmaskStr, '.', netmaskBytes, 4, 10);
  const char* gwStr = gw; byte gwBytes[4]; parseBytes(gwStr, '.', gwBytes, 4, 10);

  WiFiManager wifiManager;
  wifiManager.setDebugOutput(wifiManagerDebugOutput);
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  WiFiManagerParameter custom_ccuip("ccu", "IP der CCU2", ccuip, 16);

  char chrVariable[50];
  Variable.toCharArray(chrVariable, 50);
  WiFiManagerParameter custom_Variablename("Variable", "Variablenname", chrVariable, 50);
  WiFiManagerParameter custom_sendinterval("sendinterval", "&Uuml;bertragung alle x Sekunden", SendIntervalSeconds, 8);

  WiFiManagerParameter custom_ip("custom_ip", "IP-Adresse", "", 16);
  WiFiManagerParameter custom_netmask("custom_netmask", "Netzmaske", "", 16);
  WiFiManagerParameter custom_gw("custom_gw", "Gateway", "", 16);
  WiFiManagerParameter custom_text("<br/><br>Statische IP (wenn leer, dann DHCP):");
  wifiManager.addParameter(&custom_ccuip);
  wifiManager.addParameter(&custom_Variablename);
  wifiManager.addParameter(&custom_sendinterval);
  wifiManager.addParameter(&custom_text);
  wifiManager.addParameter(&custom_ip);
  wifiManager.addParameter(&custom_netmask);
  wifiManager.addParameter(&custom_gw);

  String Hostname = "WemosD1-" + WiFi.macAddress();

  wifiManager.setConfigPortalTimeout(ConfigPortalTimeout);

  if (startWifiManager == true) {
    digitalWrite(LED_BUILTIN, LOW);
    if (_ssid == "" || _psk == "" ) {
      wifiManager.resetSettings();
    }
    else {
      if (!wifiManager.startConfigPortal(Hostname.c_str())) {
        printSerial("failed to connect and hit timeout");
        delay(1000);
        ESP.restart();
      }
    }
  }

  wifiManager.setSTAStaticIPConfig(IPAddress(ipBytes[0], ipBytes[1], ipBytes[2], ipBytes[3]), IPAddress(gwBytes[0], gwBytes[1], gwBytes[2], gwBytes[3]), IPAddress(netmaskBytes[0], netmaskBytes[1], netmaskBytes[2], netmaskBytes[3]));

  wifiManager.autoConnect(Hostname.c_str());

  printSerial("Wifi Connected");
  printSerial("CUSTOM STATIC IP: " + String(ip) + " Netmask: " + String(netmask) + " GW: " + String(gw));
  if (shouldSaveConfig) {
    SPIFFS.begin();
    printSerial("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    if (String(custom_ip.getValue()).length() > 5) {
      printSerial("Custom IP Address is set!");
      strcpy(ip, custom_ip.getValue());
      strcpy(netmask, custom_netmask.getValue());
      strcpy(gw, custom_gw.getValue());
    } else {
      strcpy(ip,      "0.0.0.0");
      strcpy(netmask, "0.0.0.0");
      strcpy(gw,      "0.0.0.0");
    }
    strcpy(ccuip, custom_ccuip.getValue());
    Variable = custom_Variablename.getValue();
    json["ip"] = ip;
    json["netmask"] = netmask;
    json["gw"] = gw;
    json["ccuip"] = ccuip;
    json["Variable"] = Variable;
    json["sendinterval"] = String(custom_sendinterval.getValue()).toInt();

    SPIFFS.remove("/" + configJsonFile);
    File configFile = SPIFFS.open("/" + configJsonFile, "w");
    if (!configFile) {
      printSerial("failed to open config file for writing");
    }

    if (SerialDEBUG) {
      json.printTo(Serial);
      Serial.println("");
    }
    json.printTo(configFile);
    configFile.close();

    SPIFFS.end();
    delay(100);
    ESP.restart();
  }
  return true;
}

void configModeCallback (WiFiManager *myWiFiManager) {
  printSerial("AP-Modus ist aktiv!");
  //Ausgabe, dass der AP Modus aktiv ist
}

void saveConfigCallback () {
  printSerial("Should save config");
  shouldSaveConfig = true;
}

void parseBytes(const char* str, char sep, byte* bytes, int maxBytes, int base) {
  for (int i = 0; i < maxBytes; i++) {
    bytes[i] = strtoul(str, NULL, base);
    str = strchr(str, sep);
    if (str == NULL || *str == '\0') {
      break;
    }
    str++;
  }
}

bool loadSystemConfig() {
  printSerial("mounting FS...");
  if (SPIFFS.begin()) {
    printSerial("mounted file system");
    if (SPIFFS.exists("/" + configJsonFile)) {
      printSerial("reading config file");
      File configFile = SPIFFS.open("/" + configJsonFile, "r");
      if (configFile) {
        printSerial("opened config file");
        size_t size = configFile.size();
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        if (SerialDEBUG) {
          json.printTo(Serial);
          Serial.println("");
        }
        if (json.success()) {
          printSerial("\nparsed json");
          strcpy(ip,         json["ip"]);
          strcpy(netmask,    json["netmask"]);
          strcpy(gw,         json["gw"]);
          strcpy(ccuip,      json["ccuip"]);
          Variable = (json["Variable"]).as<String>();
          itoa(json["sendinterval"], SendIntervalSeconds, 10);
        } else {
          printSerial("failed to load json config");
        }
      }
      return true;
    } else {
      printSerial("/" + configJsonFile + " not found.");
      return false;
    }
    SPIFFS.end();
  } else {
    printSerial("failed to mount FS");
    return false;
  }
}

void startOTAhandling() {
  ArduinoOTA.onStart([]() {
    Serial.println("Start updating");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  Serial.println("Starte OTA");

  String Hostname = ((LDRMODE) ? "LDR" : "BH1750");
  Hostname += "-OTA-" + WiFi.macAddress();
  Serial.println("Hostname = " + Hostname);
  ArduinoOTA.setHostname(Hostname.c_str());
  ArduinoOTA.begin();
}

