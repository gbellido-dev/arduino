#include <ESP8266WiFi.h>

const char* ssid     = "ChatRoom";
const char* password = "12345678";

const char* host = "192.168.4.1";
const uint16_t port = 23;

WiFiClient client;


void
makeRandomMac(uint8_t mac[6])
{
  for (size_t i = 0; i < 6; ++i) {
    mac[i] = random(256);
  }
  mac[0] = mac[0] & ~0x01;
}

bool
changeMac(const uint8_t mac[6])
{
  return wifi_set_macaddr(STATION_IF, const_cast<uint8*>(mac));
}

void setup() {
  uint8_t mac[6];
  
  Serial.begin(9600);

  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

    
  makeRandomMac(mac);
  changeMac(mac);
  Serial.print("MAC address is ");
  Serial.println(WiFi.macAddress());

  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  delay(100);

  Serial.print("Connecting to ");
  Serial.print(host);
  Serial.print(':');
  Serial.println(port);

  // Use WiFiClient class to create TCP connections
  if (!client.connect(host, port)) {
    Serial.println("connection failed");
    delay(1000);
    return;
  }else{
    Serial.println("connection ok!");
  }
}

void loop() {
  // if there are incoming bytes available
  // from the server, read them and print them:
  while (client.available()) {
    char c = client.read();
    Serial.print(c);
  }

  // as long as there are bytes in the serial queue,
  // read them and send them out the socket if it's open:
  while (Serial.available() > 0) {
    char inChar = Serial.read();
    if (client.connected()) {
      client.print(inChar);
    }else{
      Serial.println("connection failed, can´t send");
    }
  }

  // if the server's disconnected, stop the client:
  if (!client.connected()) {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
    // do nothing:
    while (true){
      yield();
      delay(10);
    }
  }
}
