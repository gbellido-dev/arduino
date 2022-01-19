#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <Ethernet.h>

// Helps with connecting to internet
#include <WiFiManager.h>

// HOSTNAME for OTA update
#define HOSTNAME "RELAY-"

#define RELAY_PIN  D1
#define LED_PIN    D4     // what pin we're connected to

const long intervalLedWifiConnected = 500;  // pause for two seconds
const long intervalPublishRelayState = 20000;  // pause for two seconds

int ledState = LOW;
int relayState = LOW;
unsigned long previousMillisLed = 0;
unsigned long previousMillisRelay = 0;

int temp = 0;
char str_temp[10];


//declaring prototypes
void configModeCallback (WiFiManager *myWiFiManager);

// Callback function header
void callback(char* topic, byte* payload, unsigned int length);


//WiFiManager
//Local intialization. Once its business is done, there is no need to keep it around
WiFiManager wifiManager;
boolean wifiConnected = false;


IPAddress server(192,168,0,15); // Change this to the ip address of the MQTT server 192.168.0.15
//const char* mqtt_server = "test.mosquitto.org";
WiFiClient  espClient;

PubSubClient client(server, 1883, callback, espClient);
//PubSubClient client(server);
  
void setup() {
  
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, relayState);
  
  pinMode(BUILTIN_LED, OUTPUT);  // initialize onboard LED as output
  digitalWrite(BUILTIN_LED, ledState);
  
  
  Serial.begin(115200);

  // Uncomment for testing wifi manager
  //wifiManager.resetSettings();
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setTimeout(60);

  wifiConnected = wifiManager.autoConnect();
  if(!wifiConnected) {
    Serial.println("failed to connect and hit timeout");
    delay(500);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    //delay(5000);
  } 

  //Manual Wifi
  //WiFi.begin(WIFI_SSID, WIFI_PWD);

  // OTA Setup
  String hostname(HOSTNAME);
  hostname += String(ESP.getChipId(), HEX);
  WiFi.hostname(hostname);
  ArduinoOTA.setHostname((const char *)hostname.c_str());
  Serial.println("HOSTNAME:");
  Serial.println((const char *)hostname.c_str());
  ArduinoOTA.begin();

  connect_to_MQTT();
}

// Called if WiFi has not been configured yet
void configModeCallback (WiFiManager *myWiFiManager) {
  
}


void callback(char* topic, byte* p, unsigned int length) {
  // In order to republish this payload, a copy must be made
  // as the orignal payload buffer will be overwritten whilst
  // constructing the PUBLISH packet.

  // Allocate the correct amount of memory for the payload copy
  char* payload = (char*)malloc(length);
  // Copy the payload to the new buffer
  memcpy(payload,p,length);

  //Serial.print(topic);
  //Serial.print(" => ");
  //Serial.println(payload);

  if (strcmp(topic, "myhome/relay/14222f") == 0) {
    if (strstr(payload, "on")) {
      relayState = HIGH;
      digitalWrite(RELAY_PIN, relayState);   // turn the RELAY on
      client.publish("myhome/relay/14222f_state","on");
    } else if (strstr(payload, "off")) {
      relayState = LOW;
      digitalWrite(RELAY_PIN, relayState);    // turn the RELAY off
      client.publish("myhome/relay/14222f_state","off");
    } else {
      Serial.print("I do not know what to do with ");
      Serial.print(payload);
      Serial.print(" on topic ");
      Serial.println(topic);
    }
  }

  
  // Free the memory
  free(payload);
}

void connect_to_MQTT() {

  if (client.connect("myhome_relay_14222f")) {
    Serial.println("(re)-connected to MQTT");
    client.subscribe("myhome/relay/14222f");
  } else {
    Serial.println("Could not connect to MQTT");
  }
}


void loop() {
    unsigned long currentMillis = millis();
    // Handle OTA update requests
    ArduinoOTA.handle();

    client.loop();

    if (!client.connected()) {
      Serial.println("Not connected to MQTT....");
      connect_to_MQTT();
      delay(5000);
    }

    

    

    // if enough millis have elapsed
    if (wifiConnected && (currentMillis - previousMillisLed > intervalLedWifiConnected)) {
      previousMillisLed = currentMillis;
    
      // toggle the relay
      ledState = !ledState;
      digitalWrite(BUILTIN_LED, ledState);
    }

    // if enough millis have elapsed
    if (currentMillis - previousMillisRelay > intervalPublishRelayState) {
      previousMillisRelay = currentMillis;

      if (relayState) {
         client.publish("myhome/relay/14222f_state","on");
      } else {
        client.publish("myhome/relay/14222f_state","off");
      }
      temp++;
      sprintf(str_temp, "%d", temp);
      client.publish("myhome/sensor/temperature_state",str_temp);
    }

}


