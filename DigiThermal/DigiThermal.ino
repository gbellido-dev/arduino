#include <WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_MLX90640.h>

// ----------- CONFIGURACIÓN ----------- //
#define WIFI_SSID     "LHAB40"
#define WIFI_PASSWORD "^m*PrqJ36#Elm3!d"

#define MQTT_SERVER_IP "172.17.17.1"
#define MQTT_SERVER_PORT 1883
#define MQTT_TOPIC "digi/thermal/03"

const char* mqtt_server = MQTT_SERVER_IP;
const int mqtt_port = MQTT_SERVER_PORT;
const char* mqtt_topic = MQTT_TOPIC;

// Pines de sensores y LEDs
#define DS18B20_1_PIN 25
#define DS18B20_2_PIN 26
#define LED_RED       13
#define LED_GREEN     12
#define LED_YELLOW    14

// Pines de selección de tiempo (entradas pull-up)
#define MODE_1_PIN 33
#define MODE_3_PIN 32
#define MODE_5_PIN 35

// Intervalo de envío
unsigned long sendInterval = 500;
unsigned long lastSend = 0;

// Estado parpadeo LED rojo (error MQTT)
bool ledRedState = false;
unsigned long lastRedBlink = 0;
const unsigned long redBlinkInterval = 500;

// DS18B20
OneWire oneWire1(DS18B20_1_PIN);
OneWire oneWire2(DS18B20_2_PIN);
DallasTemperature sensor1(&oneWire1);
DallasTemperature sensor2(&oneWire2);

// MLX90640
Adafruit_MLX90640 mlx;
float mlxFrame[32 * 24];

// MQTT
WiFiClient espClient;
PubSubClient client(espClient);

// ----------- FUNCIONES ----------- //
void setup_wifi() {
  Serial.print("Conectando a WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado");
}

void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Conectando a MQTT...");
    String clientId = "ESP32Client-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("Conectado a MQTT.");
    } else {
      Serial.print("Error MQTT: ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void elegirIntervalo() {
  if (digitalRead(MODE_1_PIN) == LOW) {
    sendInterval = 500;
  } else if (digitalRead(MODE_3_PIN) == LOW) {
    sendInterval = 3000;
  } else if (digitalRead(MODE_5_PIN) == LOW) {
    sendInterval = 5000;
  }
  Serial.println(sendInterval);
}

// ----------- SETUP ----------- //
void setup() {
  Serial.begin(115200);

  // LEDs
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);

  // Pines de selección de intervalo
  pinMode(MODE_1_PIN, INPUT_PULLUP);
  pinMode(MODE_3_PIN, INPUT_PULLUP);
  pinMode(MODE_5_PIN, INPUT_PULLUP);
  elegirIntervalo();

  // Sensores
  sensor1.begin();
  sensor2.begin();

  Wire.begin(); // SDA: GPIO 21, SCL: GPIO 22
  if (!mlx.begin()) {
    Serial.println("MLX90640 no detectado");
    while (1);
  }
  mlx.setMode(MLX90640_CHESS);
  mlx.setResolution(MLX90640_ADC_18BIT);
  mlx.setRefreshRate(MLX90640_2_HZ);

  // WiFi y MQTT
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

// ----------- LOOP PRINCIPAL ----------- //
void loop() {
  // Reconectar MQTT si se ha perdido
  if (!client.connected()) reconnect_mqtt();
  client.loop();

  // Estado WiFi → LED verde ON/OFF
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_GREEN, HIGH);
  } else {
    digitalWrite(LED_GREEN, LOW);
  }

  // Estado MQTT → LED rojo parpadea si hay error
  if (!client.connected()) {
    unsigned long now = millis();
    if (now - lastRedBlink >= redBlinkInterval) {
      lastRedBlink = now;
      ledRedState = !ledRedState;
      digitalWrite(LED_RED, ledRedState);
    }
  } else {
    digitalWrite(LED_RED, HIGH); // MQTT conectado → rojo apagado
  }

  // Intervalo ajustable por pines
  elegirIntervalo();

  // Enviar datos si ha pasado el intervalo
  unsigned long now = millis();
  if (now - lastSend >= sendInterval) {
    lastSend = now;

    // Leer DS18B20
    sensor1.requestTemperatures();
    sensor2.requestTemperatures();
    float temp1 = sensor1.getTempCByIndex(0);
    float temp2 = sensor2.getTempCByIndex(0);

    Serial.printf("DS18B20 (GPIO 25): %.2f °C\n", temp1);
    Serial.printf("DS18B20 (GPIO 26): %.2f °C\n", temp2);

    // Leer MLX90640
    if (mlx.getFrame(mlxFrame) != 0) {
      Serial.println("Error leyendo MLX90640");
    } else {
      Serial.println("Frame térmico OK");
    }

    digitalWrite(LED_YELLOW, HIGH);
    // Crear JSON con sensores y matriz térmica
    String json = "{\"width\":32,\"height\":24,";
    json += "\"sensor1\":" + String(temp1, 2) + ",";
    json += "\"sensor2\":" + String(temp2, 2);
    json += ",\"thermal\":[";

    for (int row = 0; row < 24; row++) {
      json += "[";
      for (int col = 0; col < 32; col++) {
        int idx = row * 32 + col;
        json += String(mlxFrame[idx], 1);
        if (col < 31) json += ",";
      }
      json += "]";
      if (row < 23) json += ",";
    }
    json += "]}";

    // Enviar MQTT
    client.publish(mqtt_topic, json.c_str());
    //Serial.println(json.c_str());

    // LED amarillo parpadea breve al enviar
    
    //delay(100);
    digitalWrite(LED_YELLOW, LOW);
  }
}
