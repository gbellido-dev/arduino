#include <ESP8266WiFi.h>

#define MAX_CLIENT 20

// telnet defaults to port 23
WiFiServer server(23);
WiFiClient clients[MAX_CLIENT];


//WiFi Connection configuration
const char ssid[] = "ChatRoom2";
const char password[] = "12345678"; 

void setup() {
  delay(1000);
  Serial.begin(9600);
  Serial.println();


  struct softap_config config;
wifi_softap_get_config(&config); // Get config first.

os_memset(config.ssid, 0, 32);
os_memset(config.password, 0, 64);
os_memcpy(config.ssid, "ChatRoom2", 7);
os_memcpy(config.password, "12345678", 8);
config.authmode = AUTH_WPA_WPA2_PSK;
config.ssid_len = 0;// or its actual length
config.beacon_interval = 100;
config.max_connection = 8; // how many stations can connect to ESP8266 softAP at most.

wifi_softap_set_config(&config);// Set ESP8266 softap config
  
  //Connect to wifi Network
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password); 
  Serial.println("");
 
  Serial.print("Direccion IP Access Point - por defecto: ");      //Imprime la dirección IP
  Serial.println(WiFi.softAPIP()); 
  Serial.print("Direccion MAC Access Point: ");                   //Imprime la dirección MAC
  Serial.println(WiFi.softAPmacAddress()); 

  // start listening for clients
  server.begin();
}

void loop() {
  // wait for a new client:
  WiFiClient newClient = server.available();

  // when the client sends the first byte, say hello:
  if (newClient) {
    Serial.println(newClient.remoteIP());
    //check which of the existing clients can be overridden:
    for (byte i = 0; i < MAX_CLIENT; i++) {
      if (!clients[i] && clients[i] != newClient) {
        clients[i] = WiFiClient(newClient);
        // clead out the input buffer:
        newClient.flush();
        Serial.println("We have a new client");
        newClient.print("Hello, client number: ");
        newClient.print(i);
        newClient.println();
        break;
      }
    }
  }

  for (byte i = 0; i < MAX_CLIENT; i++) {
    if (clients[i] && clients[i].available() > 0) {
      // read the bytes incoming from the client:
      char thisChar = clients[i].read();
      // echo the bytes back to all other connected clients:
      for (byte j = 0; j < MAX_CLIENT; j++) {
        if (clients[j] && (clients[j].connected())) {
          clients[j].write(thisChar);
        }
      }
      // echo the bytes to the server as well:
      Serial.write(thisChar);
    }
  }
  
  for (byte i = 0; i < MAX_CLIENT; i++) {
    if (clients[i] && !(clients[i].connected())) {
      // client.stop() invalidates the internal socket-descriptor, so next use of == will allways return false;
      clients[i].stop();
    }
  }
}
