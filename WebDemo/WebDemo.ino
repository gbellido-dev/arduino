/*
 * ESP8266 SPIFFS HTML Web Page with JPEG, PNG Image 
 *
 */
#include "DHTesp.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>   //Include File System Headers

#define INTERVAL 2000

const char* htmlfile = "/index.html";

//ESP AP Mode configuration
const char *ssid = "MyWifi";
const char *password = "12345678";

DHTesp dht;
ESP8266WebServer server(80);

void handleRoot(){
  server.sendHeader("Location", "/index.html",true);   //Redirect to our html web page
  server.send(302, "text/plane","");
}

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
  Serial.println();

  //Initialize File System
  SPIFFS.begin();
  Serial.println("File System Initialized");

  //Initialize AP Mode
  WiFi.softAP(ssid);  //Password not used
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("Web Server IP:");
  Serial.println(myIP);

  //Initialize Webserver
  server.on("/",handleRoot);
  server.onNotFound(handleWebRequests); //Set setver all paths are not found so we can handle as per URI
  server.begin();  
}

float humidity = 0;
float temperature = 0;

unsigned long previousMillis = 0;

void loop() {
 server.handleClient();

 unsigned long currentMillis = millis();


  if (currentMillis - previousMillis >= INTERVAL) {
    humidity = dht.getHumidity();
    temperature = dht.getTemperature();

    previousMillis = currentMillis;
  }
}

bool loadFromSpiffs(String path){
  String dataType = "text/plain";
  if(path.endsWith("/")) path += "index.htm";

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


  if(path.endsWith("sensordata.html")){
    String stringContent = dataFile.readString();

    char bufferTemp[10];
    char bufferHumi[10];
    sprintf(bufferHumi, "%f", humidity);
    sprintf(bufferTemp, "%f", temperature);
    stringContent.replace("**H**", bufferHumi);
    stringContent.replace("**T**", bufferTemp);
    
    Serial.println(stringContent);
    server.send(302, "text/html", stringContent);
  }else{
      if (server.streamFile(dataFile, dataType) != dataFile.size()) {
      }
  }


  dataFile.close();
  return true;
}
