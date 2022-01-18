#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <FS.h>   //Include File System Headers

const char* htmlfile = "/index.html";

//SSID and Password of your WiFi router
const char* ssid = "BigBangTheory02";
const char* password = "12345678";

ESP8266WebServer server(80); //Server on port 80

int esperaResetJugadorId = 0;
int opcionesSeleccionadas[2] = {5, 5};
int ganador = 3;


IPAddress clients[2];




#define VALOR_PIEDRA  0
#define VALOR_PAPEL   1
#define VALOR_TIJERA  2
#define VALOR_LAGARTO 3
#define VALOR_SPOCK   4


//===============================================================
// This routine is executed when you open its IP in browser
//===============================================================
void handleRoot() {
 IPAddress client = server.client().remoteIP();
 if(!clients[0]){
	 clients[0] = client;
 }else if(clients[0] != client && !clients[1]){
	 clients[1] = client;
 }
  server.sendHeader("Location", htmlfile, true);   //Redirect to our html web page
  server.send(302, "text/plane","");
}

bool loadFromSpiffs(String path){
  String dataType = "text/plain";
  if(path.endsWith("/")) path += "index.html";

  IPAddress client = server.client().remoteIP();
  if(path.endsWith("index.html")){
     if(!clients[0]){
		 clients[0] = client;
	 }else if(clients[0] != client && !clients[1]){
		 clients[1] = client;
	 }
  }

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
  else if(path.endsWith(".woff")) dataType = "font/woff";
  else if(path.endsWith(".woff2")) dataType = "font/woff2";
  else if(path.endsWith(".ttf")) dataType = "font/ttf";
  else if(path.endsWith(".svg")) dataType = "image/svg+xml";
  else if(path.endsWith(".eot")) dataType = "application/vnd.ms-fontobject";
  
  File dataFile = SPIFFS.open(path.c_str(), "r");
  if(dataFile == NULL){
    Serial.println(path + " not found!");
  }
  
  if (server.hasArg("download")) dataType = "application/octet-stream";
  
  if (server.streamFile(dataFile, dataType) != dataFile.size()) {}

  dataFile.close();
  return true;
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




void handleEstado() {
 //xhttp.open("GET", "obtenerEstado", true);
 int playerNumber = 0;
 IPAddress client = server.client().remoteIP();
 if(clients[0] == client){
   playerNumber = 1;
 }else if(clients[1] == client){
   playerNumber = 0;
 }
 String estado = "";
 String valorContrario = String(opcionesSeleccionadas[playerNumber]);
/*reset-valorContrario-ganador
  reset: 0- no hacer nada
		 1, 2 probocar reset
  valorContrario: 0,1,2,3,4 seleccionar imagen
				  5 no hay movimiento contrario
  ganador: 0- empate
		   1- gana jugador 1
		   2- gana jugador 2
		   3- no hay movimiento contrario
*/

 estado = String(esperaResetJugadorId) + "-" + valorContrario + "-" + ganador;
 Serial.println("Estado: " + estado);
 server.send(200, "text/plane", estado);
}

void handleJugador() {
 int playerNumber = 0;
 String playerNumberStr = "";
 //xhttp.open("GET", "obtenerJugador", true);
 IPAddress client = server.client().remoteIP();
 if(clients[0] == client){
	 playerNumber = 1;
 }else if(clients[1] == client){
	 playerNumber = 2;
 }
 playerNumberStr = playerNumberStr + playerNumber;
 Serial.println("Player: " + playerNumberStr);
 server.send(200, "text/plane", playerNumberStr);
}

void handleReset() {
 //xhttp.open("GET", "reset", true);
 
 int playerNumber = 0;
 IPAddress client = server.client().remoteIP();
 if(clients[0] == client){
	 esperaResetJugadorId = 2;
 }else if(clients[1] == client){
	 esperaResetJugadorId = 1;
 }
 opcionesSeleccionadas[0] = 5;
 opcionesSeleccionadas[1] = 5;
 ganador = 3;
 server.send(200, "text/plane", "");
}

void handleFinReset() {
 //xhttp.open("GET", "finReset", true);
 
 int playerNumber = 0;
 IPAddress client = server.client().remoteIP();
 if(clients[0] == client){
	 esperaResetJugadorId = 0;
 }else if(clients[1] == client){
	 esperaResetJugadorId = 0;
 }

 server.send(200, "text/plane", "");
}


void handleMarcarSeleccion() {
 //xhttp.open("GET", "marcarSeleccion?objectId="+objectId, true); 
 int playerNumber = 0;
 int objectId = 0;
 IPAddress client = server.client().remoteIP();
 if(clients[0] == client){
	 playerNumber = 0;
 }else if(clients[1] == client){
	 playerNumber = 1;
 }
 
 String objectIdStr = server.arg("objectId");
 sscanf(objectIdStr.c_str(), "%d", &objectId);
 
 opcionesSeleccionadas[playerNumber] = objectId;
 
 server.send(200, "text/plane", "");
}



int calcularResultadoDuelo(int valorJugador1, int valorJugador2){
	int jugadorGanador = 0; // 1 o 2, empate 0
	
	/*
		Tijeras cortan papel. 
		Papel cubre piedra. 
		Piedra aplasta lagarto. 
		Lagarto envenena a Spock. 
		Spock rompe tijeras. 
		Tijeras decapitan lagarto. 
		Lagarto come papel. 
		Papel refuta a Spock. 
		Spock vaporiza piedra. 
		Y piedra aplasta tijeras.
	*/
	if(valorJugador1 == VALOR_PIEDRA){
		if(valorJugador2 == VALOR_PIEDRA){
			jugadorGanador = 0;
		}else if(valorJugador2 == VALOR_PAPEL){
			jugadorGanador = 2;
		}else if(valorJugador2 == VALOR_TIJERA){
			jugadorGanador = 1;
		}else if(valorJugador2 == VALOR_LAGARTO){
			jugadorGanador = 1;
		}else if(valorJugador2 == VALOR_SPOCK){
			jugadorGanador = 2;
		}
		
	}else if(valorJugador1 == VALOR_PAPEL){
		if(valorJugador2 == VALOR_PIEDRA){
			jugadorGanador = 1;
		}else if(valorJugador2 == VALOR_PAPEL){
			jugadorGanador = 0;
		}else if(valorJugador2 == VALOR_TIJERA){
			jugadorGanador = 2;
		}else if(valorJugador2 == VALOR_LAGARTO){
			jugadorGanador = 2;
		}else if(valorJugador2 == VALOR_SPOCK){
			jugadorGanador = 1;
		}
		
	}else if(valorJugador1 == VALOR_TIJERA){
		if(valorJugador2 == VALOR_PIEDRA){
			jugadorGanador = 2;
		}else if(valorJugador2 == VALOR_PAPEL){
			jugadorGanador = 1;
		}else if(valorJugador2 == VALOR_TIJERA){
			jugadorGanador = 0;
		}else if(valorJugador2 == VALOR_LAGARTO){
			jugadorGanador = 1;
		}else if(valorJugador2 == VALOR_SPOCK){
			jugadorGanador = 2;
		}
		
	}else if(valorJugador1 == VALOR_LAGARTO){
		if(valorJugador2 == VALOR_PIEDRA){
			jugadorGanador = 2;
		}else if(valorJugador2 == VALOR_PAPEL){
			jugadorGanador = 1;
		}else if(valorJugador2 == VALOR_TIJERA){
			jugadorGanador = 2;
		}else if(valorJugador2 == VALOR_LAGARTO){
			jugadorGanador = 0;
		}else if(valorJugador2 == VALOR_SPOCK){
			jugadorGanador = 1;
		}
		
	}else if(valorJugador1 == VALOR_SPOCK){
		if(valorJugador2 == VALOR_PIEDRA){
			jugadorGanador = 1;
		}else if(valorJugador2 == VALOR_PAPEL){
			jugadorGanador = 2;
		}else if(valorJugador2 == VALOR_TIJERA){
			jugadorGanador = 1;
		}else if(valorJugador2 == VALOR_LAGARTO){
			jugadorGanador = 2;
		}else if(valorJugador2 == VALOR_SPOCK){
			jugadorGanador = 0;
		}
	}
		
	return jugadorGanador;
}


//==============================================================
//                  SETUP
//==============================================================
void setup(void){
  Serial.begin(115200);

  //Initialize File System
  SPIFFS.begin();
  Serial.println("File System Initialized");
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password); 
  Serial.println("");
 
  Serial.print("Direccion IP Access Point - por defecto: ");      //Imprime la dirección IP
  Serial.println(WiFi.softAPIP()); 
  Serial.print("Direccion MAC Access Point: ");                   //Imprime la dirección MAC
  Serial.println(WiFi.softAPmacAddress()); 
 
  server.on("/", handleRoot);      //Which routine to handle at root location. This is display page
  server.on("/obtenerEstado", handleEstado);
  server.on("/obtenerJugador", handleJugador);
  server.on("/marcarSeleccion", handleMarcarSeleccion);
  server.on("/reset", handleReset);
  server.on("/finReset", handleFinReset);
  
  
  server.onNotFound(handleWebRequests); //Set setver all paths are not found so we can handle as per URI
  

  server.begin();                  //Start server
  Serial.println("HTTP server started");
}
//==============================================================
//                     LOOP
//==============================================================
void loop(void){
  server.handleClient();          //Handle client requests
  
  
  if(opcionesSeleccionadas[0] != 5 && opcionesSeleccionadas[1] != 5){
	  ganador = calcularResultadoDuelo(opcionesSeleccionadas[0], opcionesSeleccionadas[1]);
  }
  
}
