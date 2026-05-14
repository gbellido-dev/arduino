#include <WiFi.h>
#include <PubSubClient.h>

// WiFi
const char* WIFI_SSID = "LHAB40";
const char* WIFI_PASS = "^m*PrqJ36#Elm3!d";

// MQTT
const char* MQTT_HOST = "172.17.17.1";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_CLIENT_ID = "esp32-plc14-ai00";
const char* MQTT_TOPIC = "plc14/analog";

// Intervalo
const unsigned long PUBLISH_INTERVAL_MS = 1000;

// ADC
const float ADC_FULL_SCALE_V = 10.0f;   // 0–10 V
const float ADC_COUNTS      = 4095.0f;  // 12 bits

WiFiClient espClient;
PubSubClient mqtt(espClient);
unsigned long lastPub = 0;

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
}

void connectMQTT() {
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  while (!mqtt.connected()) {
    mqtt.connect(MQTT_CLIENT_ID) ? void() : delay(2000);
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(AI0_0, INPUT);
  pinMode(AI0_1, INPUT);
  connectWiFi();
  connectMQTT();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();

  unsigned long now = millis();
  if (now - lastPub >= PUBLISH_INTERVAL_MS) {
    lastPub = now;

    // Lecturas
    float voltage0 = (analogRead(AI0_0) / ADC_COUNTS) * ADC_FULL_SCALE_V;
    float voltage1 = (analogRead(AI0_1) / ADC_COUNTS) * ADC_FULL_SCALE_V;

    // JSON plano
    char payload[120];
    snprintf(payload, sizeof(payload),
             "{\"voltage0\":%.5f,\"voltage1\":%.5f}",
             voltage0, voltage1);

    mqtt.publish(MQTT_TOPIC, payload);
    Serial.println(payload);
  }
}
