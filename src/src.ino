#include <WiFi.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Update.h>

// Wi-Fi credentials
const char* ssid = "test";
const char* password = "00000000";

// MQTT Broker
const char* mqtt_server = "test.mosquitto.org";


// OTA variables
const char* current_version_new = "0.0.2";
const char* version_url = "https://devtestego.oss-me-central-1.aliyuncs.com/firmware.json";

// LED settings
#define LED_BUILTIN 2
const int ledPin = LED_BUILTIN;
unsigned long lastBlinkTime = 0;
bool ledState = false;

// Update check interval
const unsigned long updateInterval = 60000; // 60 seconds
unsigned long lastUpdateCheck = 0;

// MQTT setup
WiFiClient espClient;
PubSubClient client(espClient);

// Timing for MQTT publishing
static unsigned long lastMsg = 0;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void mqttCallback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe("esp32/waterlevel");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
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
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

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

    if (strcmp(newVersion, current_version_new) != 0) {
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
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
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

void setup() {
  Serial.begin(115200);
  delay(1000);

  // LED setup
  pinMode(ledPin, OUTPUT);

  // Connect Wi-Fi
  setup_wifi();

  // MQTT setup
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttCallback);

  // Seed random number generator
  randomSeed(analogRead(0));

  // First OTA check
  checkForUpdates();
  lastUpdateCheck = millis();
}

void loop() {
  blinkLED();

  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  unsigned long now = millis();

  // MQTT publish every 3 seconds
  if (now - lastMsg > 3000) {
    lastMsg = now;
    int waterLevel = random(0, 101);
    String payload = "{\"water_level\": " + String(waterLevel) + ",\"ota_version\": " + String(current_version_new) + "}";
    Serial.print("Publishing water level: ");
    Serial.println(payload);
    client.publish("esp32/waterlevel", payload.c_str());
  }

  // OTA check every 60 seconds
  if (now - lastUpdateCheck >= updateInterval) {
    Serial.println("Checking for update...");
    checkForUpdates();
    lastUpdateCheck = millis();
  }
}


// #include <WiFi.h>
// #include <HTTPClient.h>
// #include <ArduinoJson.h>
// #include <Update.h>

// // Wi-Fi credentials
// const char* ssid = "test";
// const char* password = "00000000";

// // Current firmware version

// // URL to check for latest version info (JSON) from Alibaba OSS
// const char* version_url = "https://devtestego.oss-me-central-1.aliyuncs.com/firmware.json";  // Change to OSS URL
// // LED settings
// #define LED_BUILTIN 2
// const int ledPin = LED_BUILTIN;
// unsigned long lastBlinkTime = 0;
// bool ledState = false;

// // Update check interval
// const unsigned long updateInterval = 60000; // 60 seconds
// unsigned long lastUpdateCheck = 0;

// void setup() {
//   Serial.begin(115200);
//   delay(1000);

//   // LED setup
//   pinMode(ledPin, OUTPUT);

//   // Connect to Wi-Fi
//   WiFi.begin(ssid, password);
//   Serial.print("Connecting to WiFi");
//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//     Serial.print(".");
//   }
//   Serial.println("\nConnected to WiFi");

//   // Initial update check
//   checkForUpdates();
//   lastUpdateCheck = millis();
// }

// void loop() {
//   blinkLED();

//   if (millis() - lastUpdateCheck >= updateInterval) {
//     Serial.println("Checking for update...");
//     checkForUpdates();
//     lastUpdateCheck = millis();
//   }


// }

// void blinkLED() {
//   if (millis() - lastBlinkTime >= 1000) {
//     ledState = !ledState;
//     digitalWrite(ledPin, ledState);
//     lastBlinkTime = millis();
//   }
// }

// void checkForUpdates() {
//   HTTPClient http;
//   http.begin(version_url);
//   http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);  // Follow OSS redirect

//   int httpCode = http.GET();
//   if (httpCode == 200) {
//     String payload = http.getString();
//     Serial.println("Received version info:");
//     Serial.println(payload);

//     StaticJsonDocument<512> json;
//     DeserializationError error = deserializeJson(json, payload);

//     if (error) {
//       Serial.println("Failed to parse JSON");
//       return;
//     }

//     const char* newVersion = json["version"];
//     const char* binURL = json["url"];

//     if (strcmp(newVersion, current_version_new) != 0) {
//       Serial.printf("New version available: %s\n", newVersion);
//       Serial.println("Starting OTA update...");
//       performOTA(binURL);
//     } else {
//       Serial.println("Already on latest version.");
//     }

//   } else {
//     Serial.printf("Failed to get version info, HTTP code: %d\n", httpCode);
//   }

//   http.end();
// }

// void performOTA(const char* binURL) {
//   WiFiClient client;
//   HTTPClient http;

//   http.begin(client, binURL);
//   http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);  // Follow OSS redirect
//   int httpCode = http.GET();

//   if (httpCode == 200) {
//     int contentLength = http.getSize();
//     bool canBegin = Update.begin(contentLength);

//     if (canBegin) {
//       Serial.println("Begin OTA...");
//       WiFiClient& stream = http.getStream();

//       size_t written = Update.writeStream(stream);
//       if (written == contentLength) {
//         Serial.println("Written : " + String(written) + " successfully");
//       } else {
//         Serial.println("Written only : " + String(written) + "/" + String(contentLength));
//       }

//       if (Update.end()) {
//         if (Update.isFinished()) {
//           Serial.println("OTA done! Rebooting...");
//           ESP.restart();
//         } else {
//           Serial.println("OTA not finished.");
//         }
//       } else {
//         Serial.printf("Error Occurred. Error #: %d\n", Update.getError());
//       }

//     } else {
//       Serial.println("Not enough space to begin OTA");
//     }

//   } else {
//     Serial.printf("HTTP error code: %d\n", httpCode);
//   }

//   http.end();
// }
