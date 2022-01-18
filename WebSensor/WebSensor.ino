/*
 * ESP8266 SPIFFS HTML Web Page with JPEG, PNG Image 
 *
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>   //Include File System Headers

#include "DHTesp.h"
DHTesp dht;
#define INTERVAL 2000
const char* htmlfile = "/index.html";
#define LED_PIN D7
#define LED_PIN2 D8
#define REL_PIN D3




//ESP AP Mode configuration
const char *ssid = "MGEP02";
const char *password = "12345678";

float ventana = 0;
float humidity = 0;
float minHum = 20;
float maxHum = 60;
float temperature = 0;
float minTem = 10;
float maxTem = 30;

ESP8266WebServer server(80);

void handleRoot(){
  server.sendHeader("Location", "/index.html",true);   //Redirect to our html web page
  server.send(302, "text/plane","");
}

//coger la temperatura/ humedad/ ventana y guardarla en el buffer

void handleTemperature(){
  char bufferTemp[10];
  sprintf(bufferTemp, "%f", temperature);
  server.send(200, "text/plane", bufferTemp);
}

void handleHumidity(){
  char bufferHumi[10];
  sprintf(bufferHumi, "%f", humidity);
  server.send(200, "text/plane", bufferHumi);
}

void handleVentana(){
  char bufferVent[10];
  sprintf(bufferVent, "%f", ventana);
  server.send(200, "text/plane", bufferVent);
}


//recibir respuesta de la web
void handleWebRequests(){
  if(loadFromSpiffs(server.uri())) return;
  String message = "File Not Detected\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " NAME:"+server.argName(i) + "\n VALUE:" + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  Serial.println(message);
}



void setup() {
  delay(1000);
  Serial.begin(115200);

  //Initialize File System
  SPIFFS.begin();
  Serial.println("File System Initialized");
  pinMode(LED_PIN, OUTPUT);
  pinMode(LED_PIN2, OUTPUT);
  pinMode(REL_PIN, OUTPUT);
  //Initialize AP Mode
  WiFi.softAP(ssid, password);  //Password not used
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("Web Server IP:");
  Serial.println(myIP);

  //Initialize Webserver
  server.on("/",handleRoot);
  server.on("/readhumidity",handleHumidity);
  server.on("/readtemperature",handleTemperature);
  server.on("/readventana",handleVentana);  
  server.onNotFound(handleWebRequests); //Set setver all paths are not found so we can handle as per URI
  server.begin();  
  dht.setup(D5, DHTesp::DHT11);
}






unsigned long previousMillis = 0;
unsigned long previousMillis1 = 0;
unsigned long previousMillis2 = 0;
unsigned long previousMillis3 = 0;
// intervalos en los que se encienden los leds
  long interval1 = 1000;
  long interval2 = 1000;
  bool estadoR = 0;

void loop() {
 server.handleClient();


unsigned long currentMillis = millis();

//bucle que coge los datos del sensor 
 if (currentMillis - previousMillis >= INTERVAL){
 humidity = dht.getHumidity();
 temperature = dht.getTemperature();

 
 previousMillis = currentMillis;

  if(humidity > minHum){
   if(humidity > maxHum){
     interval1= 2500;
   }
   else{ 
    interval1= 750;
   
   }
 }
 else{
  interval1= -1;
 }
 
 if(temperature > minTem){
   if(temperature > maxTem){
     interval2= 125;
   }
   else {
    interval2= 500;
   }
 }
 else{
  interval2= -1;
 }

 }


    unsigned long currentMillis1 = millis();

// encender - apagar led de humedad
    if (interval1 != -1){

      if (currentMillis1 - previousMillis1 >= interval1) {     
        previousMillis1 = currentMillis1;
        //enviar estado a la pagina
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      }
    }
    else {
      //enviar estado a la pagina
      digitalWrite(LED_PIN, 0);
    }

    // encender - apagar led de temperatura
    unsigned long currentMillis2 = millis();
    if (interval2 != -1){
      if (currentMillis2 - previousMillis2 >= interval2) {
        previousMillis2 = currentMillis2;
        //enviar estado a la pagina
        digitalWrite(LED_PIN2, !digitalRead(LED_PIN2));
      }
    }
    else {
      //enviar estado a la pagina
      digitalWrite(LED_PIN2, 0);
    }

    // abrir / cerrar ventana
    unsigned long currentMillis3 = millis();
    
    if(currentMillis3 - previousMillis3 >= INTERVAL){
      
      previousMillis3 = currentMillis3;
      if (humidity > maxHum){
        estadoR = 1;
        ventana = 1;
      }
      else{ 
        estadoR = 0;
        ventana = 0;
      }
      //enviar estado a la pagina
      digitalWrite(REL_PIN, estadoR);
    }  

     
}

bool loadFromSpiffs(String path){
  String dataType = "text/plain";
  Serial.println(path);
  if(path.endsWith("/")) path += "index.html";

  if(path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
  else if(path.endsWith(".html")) dataType = "text/html";
  else if(path.endsWith(".htm")) dataType = "text/html";
  else if(path.endsWith(".css")) dataType = "text/css";
  else if(path.endsWith(".js")) dataType = "application/javascript";
  else if(path.endsWith(".png")) dataType = "image/png";
  else if(path.endsWith(".gif")) dataType = "image/gif";
  else if(path.endsWith(".jpg")) dataType = "image/jpeg";
  else if(path.endsWith(".ico")) dataType = "image/x-icon";
  else if(path.endsWith(".xml")) dataType = "text/xml";
  else if(path.endsWith(".pdf")) dataType = "application/pdf";
  else if(path.endsWith(".zip")) dataType = "application/zip";

  
  File dataFile = SPIFFS.open(path.c_str(), "r");
  if (server.hasArg("download")) dataType = "application/octet-stream";

  if(path.endsWith("index.html")){

    String stringContent = dataFile.readString();

  
    char bufferTemp[10];
    char bufferHumi[10];
    sprintf(bufferHumi, "%f", humidity);
    sprintf(bufferTemp, "%f", temperature);
    stringContent.replace("**T**", bufferTemp);
    stringContent.replace("**H**", bufferHumi);

   Serial.println(stringContent);
   server.send(302, "text/html", stringContent);
  }else{
  if (server.streamFile(dataFile, dataType) != dataFile.size()) {
  }
  
  dataFile.close();
  return true;
}
}
