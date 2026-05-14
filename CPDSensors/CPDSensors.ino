#include <DHT.h>
#include <PubSubClient.h>

// ======================================================
// 1. CONFIGURACIÓN DE RED (Comenta/Descomenta según necesites)
// ======================================================
#define USE_ETHERNET        // Comenta para usar WiFi
//#define USE_STATIC_IP     // Comenta para usar DHCP

#ifdef USE_STATIC_IP
  IPAddress local_IP(192, 168, 0, 150);
  IPAddress gateway(192, 168, 0, 1);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress dns1(8, 8, 8, 8);
#endif

// --- Credenciales WiFi (solo si no usas Ethernet) ---
#ifndef USE_ETHERNET
  const char* ssid = "WLAN_5464";
  const char* password = "44330860Q";
#endif

// ======================================================
// 2. CONFIGURACIÓN DE HARDWARE Y MQTT
// ======================================================
#ifdef USE_ETHERNET
  #include <ETH.h>
  #define ETH_CLK_MODE    ETH_CLOCK_GPIO17_OUT
  #define ETH_PHY_ADDR    0
  #define ETH_PHY_TYPE    ETH_PHY_LAN8720
  #define ETH_PHY_POWER   5 
  #define ETH_PHY_MDC     23
  #define ETH_PHY_MDIO    18
#else
  #include <WiFi.h>
#endif

#define DHTPIN          32
#define DHTTYPE         DHT22
#define FOTO_PIN        34    // Sensor fotoeléctrico
const char* mqtt_server = "192.168.1.100";
const char* clientID    = "Olimex_Gateway_Industrial";

// --- Variables de Control y Tiempos ---
unsigned long intervaloEnvio = 10000; // Enviar cada 10 segundos
unsigned long lastTick       = 0;
bool objetoDetectadoEnCiclo  = false; // Memoria del intervalo

// Objetos de comunicación
WiFiClient netClient;
PubSubClient client(netClient);
DHT dht(DHTPIN, DHTTYPE);

// ======================================================
// 3. FUNCIONES DE SOPORTE
// ======================================================

void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    if (client.connect(clientID)) {
      Serial.println("¡Conectado!");
    } else {
      Serial.print("Falló, rc=");
      Serial.print(client.state());
      Serial.println(" intentando de nuevo en 5 segundos");
      delay(5000);
    }
  }
}

#ifdef USE_ETHERNET
void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START: Serial.println("ETH Iniciado"); break;
    case ARDUINO_EVENT_ETH_CONNECTED: Serial.println("ETH Conectado"); break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.print("IP Obtenida: "); Serial.println(ETH.localIP());
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED: Serial.println("ETH Desconectado"); break;
    default: break;
  }
}
#endif

// ======================================================
// 4. SETUP Y LOOP PRINCIPAL
// ======================================================

void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(FOTO_PIN, INPUT);

  #ifdef USE_ETHERNET
    WiFi.onEvent(WiFiEvent);
    #ifdef USE_STATIC_IP
      ETH.config(local_IP, gateway, subnet, dns1);
    #endif
    ETH.begin(ETH_PHY_ADDR, ETH_PHY_POWER, ETH_PHY_MDC, ETH_PHY_MDIO, ETH_PHY_TYPE, ETH_CLK_MODE);
  #else
    Serial.println("Iniciando WiFi...");
    #ifdef USE_STATIC_IP
      WiFi.config(local_IP, gateway, subnet, dns1);
    #endif
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
    Serial.println("\nWiFi Conectado");
  #endif

  client.setServer(mqtt_server, 1883);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  // 1. MONITOREO CONSTANTE (Captura eventos rápidos)
  // Si en algún momento del ciclo se detecta HIGH, guardamos el evento
  if (digitalRead(FOTO_PIN) == HIGH) { 
    objetoDetectadoEnCiclo = true; 
  }

  // 2. ENVÍO PERIÓDICO (Lógica de telemetría)
  unsigned long ahora = millis();
  if (ahora - lastTick > intervaloEnvio) {
    lastTick = ahora;

    float t = dht.readTemperature();
    float h = dht.readHumidity();
    
    // Decidimos el valor a enviar basado en si hubo detecciones en el intervalo
    int valorPresencia = objetoDetectadoEnCiclo ? 1 : 0;

    // Publicación MQTT
    if (!isnan(t) && !isnan(h)) {
      client.publish("cpd/temp", String(t).c_str());
      client.publish("cpd/humedad", String(h).c_str());
    }
    client.publish("cpd/presencia", String(valorPresencia).c_str());

    // Debug
    Serial.printf("Enviado -> T: %.2f C, Presencia en intervalo: %d\n", t, valorPresencia);

    // 3. REINICIO DE MEMORIA para el próximo intervalo
    objetoDetectadoEnCiclo = false;
  }
}