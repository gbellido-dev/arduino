/**The MIT License (MIT)
  Copyright (c) 2015 by Daniel Eichhorn
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
  See more at http://blog.squix.ch
*/

#include <Arduino.h>
#include "DHT.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ILI9341.h> // Hardware-specific library
#include <PubSubClient.h>         //https://github.com/knolleary/pubsubclient
#include <SPI.h>
#include <Wire.h>  // required even though we do not use I2C 
//#include "Adafruit_STMPE610.h"
#include "XPT2046_Touchscreen.h"
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson


// Additional UI functions
#include "GfxUi.h"

// Fonts created by http://oleddisplay.squix.ch/
#include "ArialRoundedMTBold_14.h"
#include "ArialRoundedMTBold_36.h"
#include "FreeSans24pt7b.h"

// Download helper
#include "WebResource.h"

#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>

// Helps with connecting to internet
#include <WiFiManager.h>

// check settings.h for adapting to your needs
#include "settings.h"
#include <JsonListener.h>
#include <WundergroundClient.h>
#include "TimeClient.h"

// HOSTNAME for OTA update
#define HOSTNAME "ESP8266-OTA-"

/*****************************
   Important: see settings.h to configure your settings!!!
 * ***************************/

// MQTT settings
WiFiClient espClient;
PubSubClient client(espClient);

char msg[128];

//define your default values here, if there are different values in config.json, they are overwritten.
char mqtt_server[60] = "192.168.1.100";
char mqtt_port[6] = "1883";
char username[34] = "default";
char password[34] = "default";
char deviceId[34] = "default";
char willTopic[100] = "home/termostate/";
char willMessage[60] = "I am a termostate";
char termostate_action_topic[100] = "home/termostate/action/";
char termostate_state_topic[100] = "home/termostate/state/";
char termostate_temperature_action_topic[100] = "home/termostate_temperature/action/";
char action_on[10] = "on";
char action_off[10] = "off";
char temperature_state_topic[100] = "home/temperature/state/";
char humidity_state_topic[100] = "home/humidity/state/";

const char* apName = "ESP8266-Termostate-AP";

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
GfxUi ui = GfxUi(&tft);

//Adafruit_STMPE610 spitouch = Adafruit_STMPE610(STMPE_CS);
//XPT2046_Touchscreen spitouch(TS_CS, TS_IRQ);
XPT2046_Touchscreen spitouch(TS_CS);

DHT dht;

WebResource webResource;
TimeClient timeClient(UTC_OFFSET);

// Set to false, if you prefere imperial/inches, Fahrenheit
WundergroundClient wunderground(IS_METRIC);

boolean termostateActive = true;
int programTemperature = -1;
float humidity = 0.0;
float temperature = 0.0;

const char * defaultMonthNames[] = {"Jan", "Feb", "Mar", "Apr", "May", "June", "July", "Aug", "Sept", "Oct", "Nov", "Dic"};
const char * customMonthNames[] = {"Enero", "Febrero", "Marzo", "Abril", "Mayo", "Junio", "Julio", "Agosto", "Septiembre", "Octubre", "Noviembre", "Diciembre"};


//WiFiManager
//Local intialization. Once its business is done, there is no need to keep it around
WiFiManager wifiManager;

//declaring prototypes
void configModeCallback (WiFiManager *myWiFiManager);
void downloadCallback(String filename, int16_t bytesDownloaded, int16_t bytesTotal);
ProgressCallback _downloadCallback = downloadCallback;
void downloadResources();
void updateData();
void drawProgress(uint8_t percentage, String text);
void drawTime();
void drawCurrentWeather();
void drawSensorData();
void drawForecast();
void drawForecastDetail(uint16_t x, uint16_t y, uint8_t dayIndex);
String getMeteoconIcon(String iconText);
void drawAstronomy();
void drawSeparator(uint16_t y);
void sleepNow(int wakeup);
void readTemperature();
void sendTemperature();

//flag for saving data
bool shouldSaveConfig = false;

long lastBacklightOn = 0;
long lastDownloadUpdate = 0;
long lastDrew = 0;
long lastSendTemperature = 0;
long lastReconnect = 0;
long lastReadSensor = 0;

byte backlight = LOW;

void attachInterrupts() {
  attachInterrupt(digitalPinToInterrupt(BOTON_A), handleInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(BOTON_B), handleInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(BOTON_C), handleInterrupt, FALLING);
}

void dettachInterrupts() {
  detachInterrupt(digitalPinToInterrupt(BOTON_A));
  detachInterrupt(digitalPinToInterrupt(BOTON_B));
  detachInterrupt(digitalPinToInterrupt(BOTON_C));
}


void handleInterrupt() {
  Serial.println("handleInterrupt()");
  dettachInterrupts();
  if (digitalRead(BOTON_A) == LOW) {
    Serial.println("Tocado y undido! A");
    termostateActive = !termostateActive;
    drawTermostat();
  } else if (digitalRead(BOTON_B) == LOW) {
    Serial.println("Tocado y undido! B");
    if (programTemperature > TERMOSTATE_MIN_TEMP) {
      programTemperature --;
      drawTermostat();
    }
  } else if (digitalRead(BOTON_C) == LOW) {
    Serial.println("Tocado y undido! C");
    if (programTemperature < TERMOSTATE_MAX_TEMP) {
      programTemperature ++;
      drawTermostat();
    }
  }
  attachInterrupts();
}



//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}



// Message received through MQTT
void callback(char* topic, byte* p, unsigned int length) {
  // In order to republish this payload, a copy must be made
  // as the orignal payload buffer will be overwritten whilst
  // constructing the PUBLISH packet.

  // Allocate the correct amount of memory for the payload copy
  char* payload = (char*)malloc(length + 1);
  // Copy the payload to the new buffer
  memcpy(payload, p, length);
  payload[length] = '\0';


  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if (strcmp(topic, (const char*)termostate_action_topic) == 0) {
    if (strstr(payload, (const char*)action_on)) {
      termostateActive = true;
      drawTermostat();
    } else if (strstr(payload, (const char*)action_off)) {
      termostateActive = false;
      drawTermostat();
    } else {
      Serial.print("I do not know what to do with ");
      Serial.print(payload);
      Serial.print(" on topic ");
      Serial.println(topic);
    }
  } else if (strcmp(topic, (const char*)termostate_temperature_action_topic) == 0) {
    float newTempValue = atof(payload);
    if (newTempValue > TERMOSTATE_MIN_TEMP && programTemperature < TERMOSTATE_MAX_TEMP) {
      programTemperature = (int)newTempValue;
      drawTermostat();
    } else {
      Serial.print("I do not know what to do with value (limits [");
      Serial.print(TERMOSTATE_MIN_TEMP);
      Serial.print("-");
      Serial.print(TERMOSTATE_MAX_TEMP);
      Serial.print("]: ");
      Serial.print(payload);
      Serial.print(" on topic ");
      Serial.println(topic);
    }

  }

}




void setup() {
  Serial.begin(115200);
  //pinMode(BACKLIGHT_PIN, OUTPUT);
  //digitalWrite(BACKLIGHT_PIN, backlight);

  dht.setup(D0);
  delay(100);
  Serial.println("DHT status:");
  Serial.println(dht.getStatusString());

  if (! spitouch.begin()) {
    Serial.println("STMPE not found?");
  }

  //pinMode(TS_IRQ, INPUT_PULLUP);


  //pinMode(BOTON_A, INPUT_PULLUP);
  //pinMode(BOTON_B, INPUT_PULLUP);
  //pinMode(BOTON_C, INPUT_PULLUP);
  //attachInterrupts();

  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setFont(&ArialRoundedMTBold_14);
  ui.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  ui.setTextAlignment(CENTER);
  ui.drawString(120, 160, "Connecting to WiFi");

  //clean FS, for testing
  //SPIFFS.format();
  //while(1) delay(10);

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
          strcpy(termostate_action_topic, json["termostate_action_topic"]);
          strcpy(termostate_state_topic, json["termostate_state_topic"]);
          strcpy(termostate_temperature_action_topic, json["termostate_temperature_action_topic"]);
          strcpy(willTopic, json["willtopic"]);
          strcpy(willMessage, json["willmessage"]);
          strcpy(willTopic, json["willtopic"]);
          strcpy(action_on, json["action_on"]);
          strcpy(action_off, json["action_off"]);
          strcpy(temperature_state_topic, json["temperature_state_topic"]);
          strcpy(humidity_state_topic, json["humidity_state_topic"]);

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
  WiFiManagerParameter custom_termostate_action_topic("termostate_action_topic", "termostate action topic", termostate_action_topic, 98);
  WiFiManagerParameter custom_termostate_state_topic("termostate_state_topic", "termostate state topic", termostate_state_topic, 98);
  WiFiManagerParameter custom_termostate_temperature_action_topic("termostate_temperature_action_topic", "termostate temperature state topic", termostate_temperature_action_topic, 98);
  WiFiManagerParameter custom_willtopic("willtopic", "willtopic", willTopic, 98);
  WiFiManagerParameter custom_willmessage("willmessage", "willmessage", willMessage, 58);
  WiFiManagerParameter custom_action_on("action_on", "action on", action_on, 8);
  WiFiManagerParameter custom_action_off("action_off", "action off", action_off, 8);
  WiFiManagerParameter custom_temperature_state_topic("temperature_state_topic", "temperature state topic", temperature_state_topic, 98);
  WiFiManagerParameter custom_humidity_state_topic("humidity_state_topic", "humidity state topic", humidity_state_topic, 98);



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
  wifiManager.addParameter(&custom_termostate_action_topic);
  wifiManager.addParameter(&custom_termostate_state_topic);
  wifiManager.addParameter(&custom_termostate_temperature_action_topic);
  wifiManager.addParameter(&custom_willtopic);
  wifiManager.addParameter(&custom_willmessage);
  wifiManager.addParameter(&custom_action_on);
  wifiManager.addParameter(&custom_action_off);
  wifiManager.addParameter(&custom_temperature_state_topic);
  wifiManager.addParameter(&custom_humidity_state_topic);

  //reset settings
  //wifiManager.resetSettings();

  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  //wifiManager.setMinimumSignalQuality();

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  //wifiManager.setTimeout(120);

  // Uncomment for testing wifi manager
  //wifiManager.resetSettings();
  wifiManager.setAPCallback(configModeCallback);

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
  strcpy(termostate_action_topic, custom_termostate_action_topic.getValue());
  strcpy(termostate_state_topic, custom_termostate_state_topic.getValue());
  strcpy(termostate_temperature_action_topic, custom_termostate_temperature_action_topic.getValue());
  strcpy(willTopic, custom_willtopic.getValue());
  strcpy(willMessage, custom_willmessage.getValue());
  strcpy(action_on, custom_action_on.getValue());
  strcpy(action_off, custom_action_off.getValue());
  strcpy(temperature_state_topic, custom_temperature_state_topic.getValue());
  strcpy(humidity_state_topic, custom_humidity_state_topic.getValue());

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
    json["termostate_action_topic"] = termostate_action_topic;
    json["termostate_state_topic"] = termostate_state_topic;
    json["termostate_temperature_action_topic"] = termostate_temperature_action_topic;
    json["willtopic"] = willTopic;
    json["willmessage"] = willMessage;
    json["action_on"] = action_on;
    json["action_off"] = action_off;
    json["temperature_state_topic"] = temperature_state_topic;
    json["humidity_state_topic"] = humidity_state_topic;


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

  //or use this for auto generated name ESP + ChipID
  //wifiManager.autoConnect();

  //Manual Wifi
  //WiFi.begin(WIFI_SSID, WIFI_PWD);

  // OTA Setup
  //String hostname(HOSTNAME);
  //hostname += String(ESP.getChipId(), HEX);
  //WiFi.hostname(hostname);
  //ArduinoOTA.setHostname((const char *)hostname.c_str());
  //ArduinoOTA.begin();
  //SPIFFS.begin();

  //Uncomment if you want to update all internet resources
  //SPIFFS.format();

  // download images from the net. If images already exist don't download
  downloadResources();

  // load the weather information
  updateData();
}

void reconnect() {
  // Loop until we're reconnected to the MQTT server
  if (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(deviceId, username, password, willTopic, 0, 1, willMessage)) {
      Serial.println("connected");
      // Once connected, publish an announcement... (if not authorized to publish the connection is closed)
      client.publish("all", (String("Hello from ") + String(deviceId)).c_str());
      // ... and resubscribe
      client.subscribe((String(termostate_action_topic) + String(deviceId) + String("/")).c_str());
      client.subscribe((String(termostate_temperature_action_topic) + String(deviceId) + String("/")).c_str());
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      //delay(5000);
    }
  }
}


void readTemperature() {

  char hum[10];
  char temp[10];

  Serial.print("DHT status: ");
  Serial.println(dht.getStatusString());
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  humidity = dht.getHumidity();
  // Read temperature as Celsius (the default)
  temperature = dht.getTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    humidity = -1.0;
    temperature = -1.0;
  }



  // Convert to String the values to be sent with mqtt
  dtostrf(humidity, 4, 2, hum);
  dtostrf(temperature, 4, 2, temp);

  // NB. msg e' definito come un array di 256 caratteri. Aumentare anche all'interno della libreria PubSubClient.h se necessario.
  sprintf (msg, "{\"humidity\": %s, \"temp_celsius\": %s }", hum, temp); // message formatting
  Serial.print("Info load: ");
  Serial.println(msg);
}

void sendTemperature() {
  char hum[10];
  char temp[10];

  // Convert to String the values to be sent with mqtt
  dtostrf(humidity, 4, 2, hum);
  dtostrf(temperature, 4, 2, temp);

  // NB. msg e' definito come un array di 256 caratteri. Aumentare anche all'interno della libreria PubSubClient.h se necessario.
  sprintf (msg, "{\"humidity\": %s, \"temp_celsius\": %s }", hum, temp); // message formatting
  Serial.print("Info sent: ");
  Serial.println(msg);

  client.publish(humidity_state_topic, hum, true); // retained message
  client.publish(temperature_state_topic, temp, true); // retained message
  if (termostateActive) {
    client.publish(termostate_state_topic, action_on, true); // retained message
  } else {
    client.publish(termostate_state_topic, action_off, true); // retained message
  }
}




void loop() {


  if (!client.connected()) { // MQTT
    //Check if we should update weather information
    if (millis() - lastReconnect > 1000 * MQTT_RECONECT_INTERVAL_SECS) {
      reconnect();
      lastReconnect = millis();
    }
  } else {
    client.loop();

    if (millis() - lastSendTemperature > 1000 * MQTT_SEND_DATA_INTERVAL_SECS) {
      sendTemperature();
      lastSendTemperature = millis();
    }
  }

  /*if (digitalRead(BOTON_A) == LOW) {
    Serial.println("Tocado y undido! A LOW");
    termostateActive = !termostateActive;
    drawTermostat();
  }else{
    Serial.println("Tocado y undido! A HIGH");
  }*/

  if (spitouch.touched()) {
    backlight = HIGH;
    //digitalWrite(BACKLIGHT_PIN, backlight);
    lastBacklightOn = millis();
  }

  //Check if we should update weather information
  if (millis() - lastReadSensor > 1000 * UPDATE_SENSOR_DATA_INTERVAL_SECS) {
    readTemperature();
    drawSensorData();
    lastReadSensor = millis();
  }

  // Check if we should update the clock
  if (millis() - lastDrew > 30000 && wunderground.getSeconds() == "00") {
    drawTime();
    lastDrew = millis();
  }

  // Check if we should update weather information
  if (millis() - lastDownloadUpdate > 1000 * UPDATE_WEATHER_INTERVAL_SECS) {
    updateData();
    lastDownloadUpdate = millis();
  }

  // Check if we should off backlight
  if (millis() - lastBacklightOn > 1000 * AWAKE_TIME_SECS) {
    backlight = LOW;
    //digitalWrite(BACKLIGHT_PIN, backlight);
    lastBacklightOn = millis();
  }

}


String customizeDateString(String dateString) {
  String defaultStr;
  String customStr;
  for (int cnt = 0; cnt < 12; cnt++) {
    dateString.replace(defaultMonthNames[cnt], customMonthNames[cnt]);
  }
  return dateString;
}


// Called if WiFi has not been configured yet
void configModeCallback (WiFiManager *myWiFiManager) {
  ui.setTextAlignment(CENTER);
  tft.setFont(&ArialRoundedMTBold_14);
  tft.setTextColor(ILI9341_CYAN);
  ui.drawString(120, 28, "Wifi Manager");
  ui.drawString(120, 42, "Please connect to AP");
  tft.setTextColor(ILI9341_WHITE);
  ui.drawString(120, 56, myWiFiManager->getConfigPortalSSID());
  tft.setTextColor(ILI9341_CYAN);
  ui.drawString(120, 70, "To setup Wifi Configuration");

  delay(5000);


}

// callback called during download of files. Updates progress bar
void downloadCallback(String filename, int16_t bytesDownloaded, int16_t bytesTotal) {
  Serial.println(String(bytesDownloaded) + " / " + String(bytesTotal));

  int percentage = 100 * bytesDownloaded / bytesTotal;
  if (percentage == 0) {
    ui.drawString(120, 160, filename);
  }
  if (percentage % 5 == 0) {
    ui.setTextAlignment(CENTER);
    ui.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
    //ui.drawString(120, 160, String(percentage) + "%");
    ui.drawProgressBar(10, 165, 240 - 20, 15, percentage, ILI9341_WHITE, ILI9341_BLUE);
  }

}

// Download the bitmaps
void downloadResources() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setFont(&ArialRoundedMTBold_14);
  char id[5];
  for (int i = 0; i < 21; i++) {
    sprintf(id, "%02d", i);
    tft.fillRect(0, 120, 240, 44, ILI9341_BLACK);
    webResource.downloadFile("http://www.squix.org/blog/wunderground/" + wundergroundIcons[i] + ".bmp", wundergroundIcons[i] + ".bmp", _downloadCallback);
  }
  for (int i = 0; i < 21; i++) {
    sprintf(id, "%02d", i);
    tft.fillRect(0, 120, 240, 44, ILI9341_BLACK);
    webResource.downloadFile("http://www.squix.org/blog/wunderground/mini/" + wundergroundIcons[i] + ".bmp", "/mini/" + wundergroundIcons[i] + ".bmp", _downloadCallback);
  }
  for (int i = 0; i < 23; i++) {
    tft.fillRect(0, 120, 240, 44, ILI9341_BLACK);
    webResource.downloadFile("http://www.squix.org/blog/moonphase_L" + String(i) + ".bmp", "/moon" + String(i) + ".bmp", _downloadCallback);
  }

  /*for (int i = 0; i < 21; i++) {
    sprintf(id, "%02d", i);
    Serial.println("http://www.squix.org/blog/wunderground/" + wundergroundIcons[i] + ".bmp");
  }
  for (int i = 0; i < 21; i++) {
    sprintf(id, "%02d", i);
    Serial.println("http://www.squix.org/blog/wunderground/mini/" + wundergroundIcons[i] + ".bmp");
  }
  for (int i = 0; i < 23; i++) {
    Serial.println("http://www.squix.org/blog/moonphase_L" + String(i) + ".bmp");
  }*/
}

// Update the internet based information and update screen
void updateData() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setFont(&ArialRoundedMTBold_14);
  drawProgress(20, "Updating time...");
  timeClient.updateTime();
  drawProgress(50, "Updating conditions...");
  //wunderground.updateConditions(WUNDERGRROUND_API_KEY, WUNDERGRROUND_LANGUAGE, WUNDERGROUND_COUNTRY, WUNDERGROUND_CITY);
  wunderground.updateConditionsPWS(WUNDERGRROUND_API_KEY, WUNDERGRROUND_LANGUAGE, WUNDERGROUND_PWS);
  drawProgress(70, "Updating forecasts...");
  //wunderground.updateForecast(WUNDERGRROUND_API_KEY, WUNDERGRROUND_LANGUAGE, WUNDERGROUND_COUNTRY, WUNDERGROUND_CITY);
  wunderground.updateForecastPWS(WUNDERGRROUND_API_KEY, WUNDERGRROUND_LANGUAGE, WUNDERGROUND_PWS);
  drawProgress(90, "Updating astronomy...");
  //wunderground.updateAstronomy(WUNDERGRROUND_API_KEY, WUNDERGRROUND_LANGUAGE, WUNDERGROUND_COUNTRY, WUNDERGROUND_CITY);
  wunderground.updateAstronomyPWS(WUNDERGRROUND_API_KEY, WUNDERGRROUND_LANGUAGE, WUNDERGROUND_PWS);
  //lastUpdate = timeClient.getFormattedTime();
  //readyForWeatherUpdate = false;
  drawProgress(100, "Done...");
  delay(1000);
  tft.fillScreen(ILI9341_BLACK);
  readTemperature();
  drawCurrentWeather();
  drawTime();
  drawSensorData();
  drawTermostat();
  drawForecast();
  drawAstronomy();
}

// Progress bar helper
void drawProgress(uint8_t percentage, String text) {
  ui.setTextAlignment(CENTER);
  ui.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  tft.fillRect(0, 140, 240, 45, ILI9341_BLACK);
  ui.drawString(120, 160, text);
  ui.drawProgressBar(10, 165, 240 - 20, 15, percentage, ILI9341_WHITE, ILI9341_BLUE);
}

// draws the clock
void drawTime() {
  ui.setTextAlignment(CENTER);
  ui.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setFont(&ArialRoundedMTBold_14);
  String date = timeClient.getHours() + ":" + timeClient.getMinutes() + " " + customizeDateString(wunderground.getDate());
  ui.drawString(120, 14, date);

  /*ui.setTextAlignment(CENTER);
    tft.setFont(&ArialRoundedMTBold_36);
    String time = timeClient.getHours() + ":" + timeClient.getMinutes();
    ui.drawString(54, 120, time);
    drawSeparator(65);*/
}

// draws current weather information
void drawCurrentWeather() {
  // Weather Icon
  String weatherIcon = getMeteoconIcon(wunderground.getTodayIcon());
  ui.drawBmp(weatherIcon + ".bmp", 0, 0);

  // Weather Text
  tft.setFont(&ArialRoundedMTBold_14);
  ui.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  //ui.setTextAlignment(RIGHT);
  ui.setTextAlignment(LEFT);
  //ui.drawString(160, 80, wunderground.getWeatherText());
  ui.drawString(120, 30, "Outdoor");

  tft.setFont(&ArialRoundedMTBold_36);
  ui.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  //ui.setTextAlignment(RIGHT);
  ui.setTextAlignment(LEFT);
  String temp = wunderground.getCurrentTemp() + "C";
  ui.drawString(120, 66, temp);
  drawSeparator(135);
}

void drawTermostat() {
  char programTemperatureStr[6];

  itoa(programTemperature, programTemperatureStr, 10);

  ui.setTextAlignment(LEFT);
  if (termostateActive) {
    tft.fillRect(2, 92, 100, 80, ILI9341_WHITE);
  } else {
    tft.fillRect(2, 92, 100, 80, ILI9341_WHITE);
  }
  tft.fillRect(4, 94, 96, 40, ILI9341_BLACK);
  tft.fillRect(4, 136, 96, 34, ILI9341_BLACK);


  tft.setFont(&ArialRoundedMTBold_36);
  if (termostateActive) {
    ui.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  } else {
    ui.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  }

  ui.setTextAlignment(CENTER);
  ui.drawString(52, 126, String(programTemperatureStr) + "C");


  tft.setFont(&FreeSansBold18pt7b);
  ui.setTextAlignment(CENTER);

  if (termostateActive) {
    ui.setTextColor(ILI9341_RED, ILI9341_BLACK);
    ui.drawString(50, 165, "ON");
  } else {
    ui.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    ui.drawString(50, 165, "OFF");
  }

}

void drawSensorData() {
  char tempStr[6];
  char humStr[6];

  dtostrf(humidity, 3, 1, humStr);
  dtostrf(temperature, 3, 1, tempStr);

  // Weather Text
  tft.setFont(&ArialRoundedMTBold_14);
  ui.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  //ui.setTextAlignment(RIGHT);
  ui.setTextAlignment(LEFT);
  //ui.drawString(160, 80, wunderground.getWeatherText());
  ui.drawString(120, 88, "Indoor");

  tft.setFont(&ArialRoundedMTBold_36);
  ui.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  //ui.setTextAlignment(RIGHT);
  ui.setTextAlignment(LEFT);
  String degreeSign = "F";
  if (IS_METRIC) {
    degreeSign = "C";
  }
  String temp = tempStr + degreeSign;
  ui.drawString(120, 124, temp);

  String hum = humStr + String("%");
  ui.drawString(120, 164, hum);
  drawSeparator(135);
}



// draws the three forecast columns
void drawForecast() {
  uint16_t y = 190;
  drawForecastDetail(10, y, 0);
  drawForecastDetail(95, y, 2);
  drawForecastDetail(180, y, 4);
  drawSeparator(165 + 65 + 10);
}

// helper for the forecast columns
void drawForecastDetail(uint16_t x, uint16_t y, uint8_t dayIndex) {
  ui.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  tft.setFont(&ArialRoundedMTBold_14);
  ui.setTextAlignment(CENTER);
  String day = wunderground.getForecastTitle(dayIndex).substring(0, 3);
  day.toUpperCase();
  ui.drawString(x + 25, y, day);

  ui.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  ui.drawString(x + 25, y + 14, wunderground.getForecastLowTemp(dayIndex) + "|" + wunderground.getForecastHighTemp(dayIndex));

  String weatherIcon = getMeteoconIcon(wunderground.getForecastIcon(dayIndex));
  ui.drawBmp("/mini/" + weatherIcon + ".bmp", x, y + 15);

}

// draw moonphase and sunrise/set and moonrise/set
void drawAstronomy() {
  int moonAgeImage = 24 * wunderground.getMoonAge().toInt() / 30.0;
  ui.drawBmp("/moon" + String(moonAgeImage) + ".bmp", 120 - 30, 255);

  ui.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setFont(&ArialRoundedMTBold_14);
  ui.setTextAlignment(LEFT);
  ui.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  ui.drawString(20, 270, "Sun");
  ui.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  ui.drawString(20, 285, wunderground.getSunriseTime());
  ui.drawString(20, 300, wunderground.getSunsetTime());

  ui.setTextAlignment(RIGHT);
  ui.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  ui.drawString(220, 270, "Moon");
  ui.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  ui.drawString(220, 285, wunderground.getMoonriseTime());
  ui.drawString(220, 300, wunderground.getMoonsetTime());

}

// Helper function, should be part of the weather station library and should disappear soon
String getMeteoconIcon(String iconText) {
  if (iconText == "F") return "chanceflurries";
  if (iconText == "Q") return "chancerain";
  if (iconText == "W") return "chancesleet";
  if (iconText == "V") return "chancesnow";
  if (iconText == "S") return "chancetstorms";
  if (iconText == "B") return "clear";
  if (iconText == "Y") return "cloudy";
  if (iconText == "F") return "flurries";
  if (iconText == "M") return "fog";
  if (iconText == "E") return "hazy";
  if (iconText == "Y") return "mostlycloudy";
  if (iconText == "H") return "mostlysunny";
  if (iconText == "H") return "partlycloudy";
  if (iconText == "J") return "partlysunny";
  if (iconText == "W") return "sleet";
  if (iconText == "R") return "rain";
  if (iconText == "W") return "snow";
  if (iconText == "B") return "sunny";
  if (iconText == "0") return "tstorms";


  return "unknown";
}

// if you want separators, uncomment the tft-line
void drawSeparator(uint16_t y) {
  //tft.drawFastHLine(10, y, 240 - 2 * 10, 0x4228);
}
