#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <PubSubClient.h>         //https://github.com/knolleary/pubsubclient

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include "DHT.h"

// DHT11 Sensor
#define DHTPIN D4     // what pin we're connected to
#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPIN, DHTTYPE);

// Relay Pin
const int relayPin = D1;
long wateringTime = 4000;

// MQTT settings
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[128];
long interval = 30000;     // interval at which to send mqtt messages (milliseconds)

//define your default values here, if there are different values in config.json, they are overwritten.
char mqtt_server[60] = "192.168.1.100";
char mqtt_port[6] = "1883";
char username[34] = "default";
char password[34] = "default";
char deviceId[34] = "default";
char willTopic[100] = "home/relay/";
char willMessage[60] = "I am a relay";
char action_topic[100] = "home/relay/action/";
char state_topic[100] = "home/relay/state/";
char action_on[10] = "on";
char action_off[10] = "off";

const char* apName = "ESP8266-Termostate-AP";

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

// Message received through MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the relay if an 1 was received as first character on the water/ topic
  if (strcmp(topic, "plant/pump") == 0){
    Serial.println("Topic recognized");
    if ((char)payload[0] == '1') {
      Serial.println("Pump ON/OFF");
      client.publish("plant/pump/status", "ON");
      digitalWrite(relayPin, HIGH);   // Turn the Relay on
      delay(wateringTime);
      digitalWrite(relayPin, LOW);   // Turn the Relay off
      client.publish("plant/pump/status", "OFF");
    } else {
      Serial.println("Pump OFF");
      digitalWrite(relayPin, LOW);  // Turn the Relay off
      client.publish("plant/pump/status", "OFF");
    }

  }

}


void setup() {
  pinMode(relayPin, OUTPUT);     // Initialize the relayPin as an output
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();

  //clean FS, for testing
  //SPIFFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(username, json["username"]);
          strcpy(password, json["password"]);
          strcpy(deviceId, json["deviceid"]);
          strcpy(action_topic, json["action_topic"]);
          strcpy(state_topic, json["state_topic"]);
          strcpy(willTopic, json["willtopic"]);
          strcpy(willMessage, json["willmessage"]);
          strcpy(willTopic, json["willtopic"]);
          strcpy(action_on, json["action_on"]);
          strcpy(action_off, json["action_off"]);

        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 58);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 5);
  WiFiManagerParameter custom_username("username", "username", username, 32);
  WiFiManagerParameter custom_password("password", "password", password, 32);
  WiFiManagerParameter custom_deviceid("deviceid", "deviceid", deviceId, 32);
  WiFiManagerParameter custom_action_topic("action_topic", "action topic", action_topic, 98);
  WiFiManagerParameter custom_state_topic("state_topic", "state topic", state_topic, 98);
  WiFiManagerParameter custom_willtopic("willtopic", "willtopic", willTopic, 98);
  WiFiManagerParameter custom_willmessage("willmessage", "willmessage", willMessage, 58);
  WiFiManagerParameter custom_action_on("action_on", "action on", action_on, 8);
  WiFiManagerParameter custom_action_off("action_off", "action off", action_off, 8);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //set static ip
  //wifiManager.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  
  //add all your parameters here
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_username);
  wifiManager.addParameter(&custom_password);
  wifiManager.addParameter(&custom_deviceid);
  wifiManager.addParameter(&custom_action_topic);
  wifiManager.addParameter(&custom_state_topic);
  wifiManager.addParameter(&custom_willtopic);
  wifiManager.addParameter(&custom_willmessage);
  wifiManager.addParameter(&custom_action_on);
  wifiManager.addParameter(&custom_action_off);

  //reset settings
  //wifiManager.resetSettings();
  
  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  //wifiManager.setMinimumSignalQuality();
  
  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  //wifiManager.setTimeout(120);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect(apName)) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  //read updated parameters
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(username, custom_username.getValue());
  strcpy(password, custom_password.getValue());
  strcpy(deviceId, custom_deviceid.getValue());
  strcpy(action_topic, custom_action_topic.getValue());
  strcpy(state_topic, custom_state_topic.getValue());
  strcpy(willTopic, custom_willtopic.getValue());
  strcpy(willMessage, custom_willmessage.getValue());
  strcpy(action_on, custom_action_on.getValue());
  strcpy(action_off, custom_action_off.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["username"] = username;
    json["password"] = password;
  json["deviceid"] = deviceId;
  json["action_topic"] = action_topic;
  json["state_topic"] = state_topic;
  json["willtopic"] = willTopic;
  json["willmessage"] = willMessage;
  json["action_on"] = action_on;
  json["action_off"] = action_off;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  Serial.println("local ip");
  Serial.println(WiFi.localIP());

  // mqtt
  client.setServer(mqtt_server, atoi(mqtt_port)); // parseInt to the port
  client.setCallback(callback);

}


void reconnect() {
  // Loop until we're reconnected to the MQTT server
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(deviceId, username, password, willTopic, 0, 1, willMessage)) {
      Serial.println("connected");
      // Once connected, publish an announcement... (if not authorized to publish the connection is closed)
      client.publish("all", (String("Hello from ")+String(deviceId)).c_str());
      // ... and resubscribe
      client.subscribe((String(action_topic)+String(deviceId)+String("/")).c_str());
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}



void sendTemperature(){
  char hum[10];
  char temp[10];
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
  temperature = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }else{
    // Convert to String the values to be sent with mqtt
    dtostrf(h,4,2,hum);
    dtostrf(t,4,2,temp);    
  }

    // NB. msg e' definito come un array di 256 caratteri. Aumentare anche all'interno della libreria PubSubClient.h se necessario.
    sprintf (msg, "{\"humidity\": %s, \"temp_celsius\": %s }", hum, temp); // message formatting
    Serial.print("Info sent: ");
    Serial.println(msg);
    client.publish("plant/humidity", hum, true); // retained message
    client.publish("plant/temperature", temp, true); // retained message
}

 
void loop() {
  // put your main code here, to run repeatedly:

  if (!client.connected()) { // MQTT
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
 
  if(now - lastMsg > interval) {
    // save the last time you blinked the LED 
    lastMsg = now;
    sendTemperature();
  }
  
}

