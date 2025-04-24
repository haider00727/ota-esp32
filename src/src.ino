#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Update.h>

// Wi-Fi credentials
const char* ssid = "test";
const char* password = "00000000";

// Versioning
const char* version_url = "https://haider00727.github.io/firmware.json";
const char* current_version = "1.2.0";
#define LED_BUILTIN 2;
// LED pin
const int ledPin = LED_BUILTIN;

unsigned long lastBlinkTime = 0;
unsigned long lastUpdateCheck = 0;

bool ledState = false;
const unsigned long updateInterval = 60000; // 60 seconds

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize LED
  pinMode(ledPin, OUTPUT);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");

  // First update check on boot
  checkForUpdates();
  lastUpdateCheck = millis();
}

void loop() {

 blinkLED();

  // Check for OTA update every 1 minute
  if (millis() - lastUpdateCheck >= updateInterval) {
    Serial.print("Checking for update \n");
    checkForUpdates();
    lastUpdateCheck = millis();
  }



  delay(100); // smooth loop
  Serial.print("Current Version ==== : %s\n ",current_version);
}

void blinkLED() {
  if (millis() - lastBlinkTime >= 2000) {
    ledState = !ledState;
    digitalWrite(ledPin, ledState);
    lastBlinkTime = millis();
  }
}

void checkForUpdates() {
  HTTPClient http;
  http.begin(version_url);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    Serial.println("Received version info:");
    Serial.println(payload);

    StaticJsonDocument<512> json;
    DeserializationError error = deserializeJson(json, payload);

    if (error) {
      Serial.println("Failed to parse JSON");
      return;
    }

    const char* newVersion = json["version"];
    const char* binURL = json["url"];

    if (strcmp(newVersion, current_version) != 0) {
      Serial.printf("New version available: %s\n", newVersion);
      Serial.println("Starting OTA update...");
      performOTA(binURL);
    } else {
      Serial.println("Already on latest version.");
    }
  } else {
    Serial.printf("Failed to get version info, HTTP code: %d\n", httpCode);
  }

  http.end();
}

void performOTA(const char* binURL) {
  WiFiClient client;
  HTTPClient http;

  http.begin(client, binURL);
  int httpCode = http.GET();

  if (httpCode == 200) {
    int contentLength = http.getSize();
    bool canBegin = Update.begin(contentLength);

    if (canBegin) {
      Serial.println("Begin OTA...");
      WiFiClient& stream = http.getStream();

      size_t written = Update.writeStream(stream);
      if (written == contentLength) {
        Serial.println("Written : " + String(written) + " successfully");
      } else {
        Serial.println("Written only : " + String(written) + "/" + String(contentLength));
      }

      if (Update.end()) {
        if (Update.isFinished()) {
          Serial.println("OTA done!");
          ESP.restart();
        } else {
          Serial.println("OTA not finished.");
        }
      } else {
        Serial.printf("Error Occurred. Error #: %d\n", Update.getError());
      }

    } else {
      Serial.println("Not enough space to begin OTA");
    }

  } else {
    Serial.printf("HTTP error code: %d\n", httpCode);
  }

  http.end();
}
