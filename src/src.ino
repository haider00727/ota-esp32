#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Update.h>

// Wi-Fi credentials
const char* ssid = "test";
const char* password = "00000000";

// Current firmware version
const char* current_version = "1.9.2";

// URL to check for latest version info (JSON)
const char* version_url = "https://raw.githubusercontent.com/haider00727/ota-esp32/main/files/firmware.json";
// LED settings
#define LED_BUILTIN 2
const int ledPin = LED_BUILTIN;
unsigned long lastBlinkTime = 0;
bool ledState = false;

// Update check interval
const unsigned long updateInterval = 60000; // 60 seconds
unsigned long lastUpdateCheck = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // LED setup
  pinMode(ledPin, OUTPUT);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");

  // Initial update check
  checkForUpdates();
  lastUpdateCheck = millis();
}

void loop() {
  blinkLED();

  if (millis() - lastUpdateCheck >= updateInterval) {
    Serial.println("Checking for update...");
    checkForUpdates();
    lastUpdateCheck = millis();
  }

 Serial.println("Test  2");
  delay(100);
}

void blinkLED() {
  if (millis() - lastBlinkTime >= 1000) {
    ledState = !ledState;
    digitalWrite(ledPin, ledState);
    lastBlinkTime = millis();
  }
}

void checkForUpdates() {
  HTTPClient http;
  http.begin(version_url);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);  // Follow GitHub redirect

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
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);  // Follow GitHub redirect
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
          Serial.println("OTA done! Rebooting...");
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
