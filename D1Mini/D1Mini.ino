/****************************************************
 * Wemos D1 mini + DHT Shield + WS2812B Shield
 * - WiFi: LHAB40
 * - MQTT: envía JSON con datos del DHT
 * - LED WS2812:
 *     Rojo = fallo WiFi / MQTT
 *     Verde = conectado y funcionando
 *     Azul breve = cada vez que publica
 ****************************************************/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Adafruit_NeoPixel.h>

/*************** CONFIGURACIÓN WIFI ******************/
const char* ssid     = "LHAB40";
const char* password = "^m*PrqJ36#Elm3!d";   // <-- pon aquí la contraseña

/*************** CONFIGURACIÓN MQTT ******************/
const char* mqtt_server   = "172.17.17.1";   // <-- cambia a IP o hostname de tu broker
const uint16_t mqtt_port  = 1883;            // puerto MQTT típico
const char* mqtt_clientId = "wemos_d1_dht_01";
const char* mqtt_topic    = "lhab40/sensors/dht";

// Si necesitas usuario/clave MQTT, descomenta y rellena:
// const char* mqtt_user     = "usuario";
// const char* mqtt_pass     = "password";

/*************** CONFIGURACIÓN DHT *******************/
// En el DHT Shield para Wemos D1 mini normalmente el pin es D4 (GPIO2)
#define DHTPIN  D4
// Cambia a DHT11 si tu shield lo usa
#define DHTTYPE DHT11   // o DHT11

DHT dht(DHTPIN, DHTTYPE);

/*************** CONFIGURACIÓN WS2812B ***************/
// En el WS2812B Shield el pin suele ser D2 (GPIO4)
#define LED_PIN    D2
#define LED_COUNT  4    // normalmente 1 LED en el shield

Adafruit_NeoPixel pixels(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

/*************** VARIABLES GLOBALES ******************/
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastPublish = 0;
const unsigned long publishInterval = 10000;  // ms entre publicaciones (10 s)

/*****************************************************
 * FUNCIONES AUXILIARES LED
 *****************************************************/

void setLedColor(uint8_t r, uint8_t g, uint8_t b) {
  pixels.setPixelColor(0, pixels.Color(r, g, b));
  pixels.show();
}

void ledWifiMqttError() {
  // Rojo fijo
  setLedColor(255, 0, 0);
}

void ledAllOk() {
  // Verde fijo
  setLedColor(0, 255, 0);
}

void ledFlashPublish() {
  // Pequeño destello azul
  setLedColor(0, 0, 255);
  delay(150);          // destello breve
  ledAllOk();          // vuelve a verde
}

/*****************************************************
 * CONEXIÓN WIFI
 *****************************************************/

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Conectando a WiFi ");
  Serial.print(ssid);
  Serial.println(" ...");

  ledWifiMqttError();  // mientras no haya WiFi, LED en rojo

  int intento = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    intento++;

    if (intento > 60) { // aprox 30 s
      Serial.println("\nNo se pudo conectar a la WiFi. Reintentando...");
      intento = 0;
      WiFi.disconnect();
      WiFi.begin(ssid, password);
    }
  }

  Serial.println("\nWiFi conectado.");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

/*****************************************************
 * CONEXIÓN MQTT
 *****************************************************/

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Conectando al broker MQTT...");
    ledWifiMqttError();   // en rojo mientras intenta conectar

    // Si tu broker necesita usuario/clave, usa la otra versión de connect():
    // bool connected = client.connect(mqtt_clientId, mqtt_user, mqtt_pass);

    bool connected = client.connect(mqtt_clientId);

    if (connected) {
      Serial.println(" conectado.");
      ledAllOk();   // al conectar, LED en verde
      // Suscripciones si las necesitas:
      // client.subscribe("mi/topic/#");
    } else {
      Serial.print(" fallo, rc=");
      Serial.print(client.state());
      Serial.println(" - reintentando en 5 segundos");
      delay(5000);
    }
  }
}

/*****************************************************
 * PUBLICAR DATOS DHT EN JSON
 *****************************************************/

void publishSensorData() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();      // en ºC
  float f = dht.readTemperature(true);  // en ºF
  float hic = dht.computeHeatIndex(t, h, false); // índice de calor en ºC

  // Comprobar lecturas
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Error leyendo del sensor DHT!");
    return;
  }

  long rssi = WiFi.RSSI();

  // Montamos JSON manualmente en un buffer
  // Ejemplo: {"temp":23.5,"hum":45.2,"heatIndex":25.1,"rssi":-60}
  char payload[200];
  snprintf(payload, sizeof(payload),
           "{\"temp\":%.2f,\"hum\":%.2f,\"heatIndex\":%.2f,\"rssi\":%ld}",
           t, h, hic, rssi);

  Serial.print("Publicando en MQTT: ");
  Serial.println(payload);

  if (client.publish(mqtt_topic, payload)) {
    Serial.println("Publicado OK.");
    ledFlashPublish();   // destello azul al publicar
  } else {
    Serial.println("Error publicando en MQTT.");
    ledWifiMqttError();  // si falla la publi, tratamos como error
  }
}

/*****************************************************
 * SETUP
 *****************************************************/

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println();
  Serial.println("Iniciando Wemos D1 mini DHT + WS2812B + MQTT...");

  // LED
  pixels.begin();
  pixels.clear();
  pixels.show();

  // DHT
  dht.begin();

  // WiFi
  connectWiFi();

  // MQTT
  client.setServer(mqtt_server, mqtt_port);

  // Intentar conectar al broker
  reconnectMQTT();
}

/*****************************************************
 * LOOP
 *****************************************************/

void loop() {
  // Comprobar WiFi y reconectar si es necesario
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectada, intentando reconectar...");
    ledWifiMqttError();
    connectWiFi();
    // Tras reconectar WiFi, asegurarse de reconectar MQTT
    reconnectMQTT();
  }

  // Comprobar conexión MQTT
  if (!client.connected()) {
    reconnectMQTT();
  }

  // Procesar la pila de MQTT
  client.loop();

  // Señal verde si todo está OK (WiFi + MQTT conectados)
  if (WiFi.status() == WL_CONNECTED && client.connected()) {
    // mantener verde si no estamos justo destellando
    // (el destello ya vuelve a verde al terminar)
    ledAllOk();
  }

  // Publicar periódicamente
  unsigned long now = millis();
  if (now - lastPublish > publishInterval) {
    lastPublish = now;
    if (client.connected()) {
      publishSensorData();
    }
  }
}
