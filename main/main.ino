#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "test";
const char* password = "00000000";

// OTA Update configuration
const char* firmwareUrl = "https://github.com/haider00727/ota-esp32/releases/download/v1.0.0/firmware.bin";
const char* firmwareVersionUrl = "https://haider00727.github.io/firmware.json";
const String currentVersion = "1.0.0"; // Current firmware version

void setupOTA() {
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();
  Serial.println("OTA Ready");
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void checkForUpdates() {
  Serial.println("Checking for updates...");
  
  HTTPClient http;
  http.begin(firmwareVersionUrl);
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    
    String latestVersion = doc["version"];
    String firmwareUrl = doc["url"];
    
    Serial.println("Current version: " + currentVersion);
    Serial.println("Latest version: " + latestVersion);
    
    if (latestVersion != currentVersion) {
      Serial.println("New firmware available! Updating...");
      performUpdate(firmwareUrl);
    } else {
      Serial.println("Already running the latest version");
    }
  } else {
    Serial.printf("Failed to check for updates. HTTP error code: %d\n", httpCode);
  }
  
  http.end();
}

void performUpdate(String url) {
  WiFiClient client;
  HTTPClient http;
  
  http.begin(client, url);
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    int contentLength = http.getSize();
    Serial.printf("Firmware size: %d bytes\n", contentLength);
    
    bool canBegin = Update.begin(contentLength);
    if (!canBegin) {
      Serial.println("Not enough space to begin OTA");
      return;
    }
    
    WiFiClient* stream = http.getStreamPtr();
    size_t written = Update.writeStream(*stream);
    
    if (written == contentLength) {
      Serial.println("Written: " + String(written) + " bytes");
    } else {
      Serial.println("Written only: " + String(written) + "/" + String(contentLength) + " bytes");
    }
    
    if (Update.end()) {
      Serial.println("OTA done!");
      if (Update.isFinished()) {
        Serial.println("Update successfully completed. Rebooting...");
        ESP.restart();
      } else {
        Serial.println("Update not finished? Something went wrong!");
      }
    } else {
      Serial.println("Error Occurred. Error #: " + String(Update.getError()));
    }
  } else {
    Serial.printf("Failed to download firmware. HTTP error code: %d\n", httpCode);
  }
  
  http.end();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  connectToWiFi();
  setupOTA();
  
  // Check for updates immediately on startup
  checkForUpdates();
}

void loop() {
  ArduinoOTA.handle();
  
  // Check for updates periodically (every 6 hours)
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 21600000) { // 6 hours in milliseconds
    checkForUpdates();
    lastCheck = millis();
  }
  
  // Your normal application code here
  // For example, blink the built-in LED
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 1000) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    lastBlink = millis();
  }
}