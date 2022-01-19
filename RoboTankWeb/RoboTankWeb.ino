/*
 * ESP8266 SPIFFS HTML Web Page with JPEG, PNG Image 
 * https://circuits4you.com
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>   //Include File System Headers


#define PIN_A_REVERSE     D0
#define PIN_A_NOT_REVERSE D1
#define PIN_A_ENABLE      D2

#define PIN_B_REVERSE     D3
#define PIN_B_NOT_REVERSE D4
#define PIN_B_ENABLE      D5

const char* htmlfile = "/index.html";

//WiFi Connection configuration
const char ssid[] = "NodeMCU-ESP8266-X";
const char password[] = "12345678"; 


ESP8266WebServer server(80);

int motor_a_value = 0;
int motor_b_value = 0;

unsigned long previousMillis = 0; 
#define INTERVAL 100

void handleRobot(){
  String message = "";
  for (uint8_t i=0; i<server.args(); i++){
    message += " NAME:"+server.argName(i) + "\n VALUE:" + server.arg(i) + "\n";
    if(server.argName(i).equals("motor_a")){
      motor_a_value = atoi(server.arg(i).c_str());
    }else if(server.argName(i).equals("motor_b")){
      motor_b_value = atoi(server.arg(i).c_str());
    }
  }
  Serial.println(message);
  //server.sendHeader("Location", "/index.html",true);   //Redirect to our html web page
  server.send(302, "text/plane","");
}

void handleRoot(){
  server.sendHeader("Location", "/index.html",true);   //Redirect to our html web page
  server.send(302, "text/plane","");
}

void handleWebRequests(){
  Serial.println(server.uri());
  if (server.uri().endsWith("move.do")){
    if(server.hasArg("motor_a") && server.hasArg("motor_b")){
      handleRobot();
    }
  }
  
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

  
  //Connect to wifi Network
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password); 
  Serial.println("");
 
  Serial.print("Direccion IP Access Point - por defecto: ");      //Imprime la dirección IP
  Serial.println(WiFi.softAPIP()); 
  Serial.print("Direccion MAC Access Point: ");                   //Imprime la dirección MAC
  Serial.println(WiFi.softAPmacAddress()); 

  //Initialize Webserver
  server.on("/",handleRoot);
  server.onNotFound(handleWebRequests); //Set setver all paths are not found so we can handle as per URI
  server.begin();  
}

void moveTank(){
  byte speed_a = 0;
  byte speed_b = 0;
  bool reverse_a = 0;
  bool reverse_b = 0;
  //Serial.println("Moving Tank!");

  if(motor_a_value > 0){
    speed_a = 1.023*motor_a_value;
    reverse_a = 0;
  }else if(motor_a_value < 0){
    speed_a = 1.023*motor_a_value;
    reverse_a = 1;
  }else{
    speed_a = 0;
  }

  if(motor_a_value > 0){
    speed_b = 1.023*motor_b_value;
    reverse_a = 0;
  }else if(motor_a_value < 0){
    speed_b = 1.023*motor_b_value;
    reverse_b = 1;
  }else{
    speed_b = 0;
  }

  digitalWrite(PIN_A_REVERSE, !reverse_a);
  digitalWrite(PIN_A_NOT_REVERSE, reverse_a);
  analogWrite(PIN_A_ENABLE, speed_a);

  digitalWrite(PIN_B_REVERSE, !reverse_b);
  digitalWrite(PIN_B_NOT_REVERSE, reverse_b);
  analogWrite(PIN_B_ENABLE, speed_b);
  
}

void loop() {
  unsigned long currentMillis = millis();
  server.handleClient();
  yield();

  if (currentMillis - previousMillis >= INTERVAL) {
    moveTank();
    
    // save the last time
    previousMillis = currentMillis;
  }
  yield();
 
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
  if (server.streamFile(dataFile, dataType) != dataFile.size()) {
  }

  dataFile.close();
  return true;
}
