#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

// ====== CONFIGURA ESTO EN CADA MDUINO ======
const uint8_t MY_ID = 1;  // 1..4 (pon uno distinto en cada equipo)

// Ojo: usa MAC únicas (cambia el último byte al menos)
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, MY_ID };

// IP fija por nodo (ajusta a tu red)
IPAddress ip(192, 168, 1, 10 + MY_ID);   // 192.168.1.11, .12, .13, .14
IPAddress subnet(255, 255, 255, 0);

// Puerto UDP del chat
const unsigned int CHAT_PORT = 5000;

// ==========================================
EthernetUDP Udp;

// Buffer UDP
char udpBuf[512];

// Buffer de línea serie
String line;

// Calcula broadcast: ip OR (~subnet)
IPAddress broadcastIP() {
  return IPAddress(
    ip[0] | (uint8_t)~subnet[0],
    ip[1] | (uint8_t)~subnet[1],
    ip[2] | (uint8_t)~subnet[2],
    ip[3] | (uint8_t)~subnet[3]
  );
}

void sendChat(const String& msg) {
  // Formato simple (texto)
  // Ejemplo: "SRC=2;MSG=Hola"
  String packet = "SRC=" + String(MY_ID) + ";MSG=" + msg;

  IPAddress bcast = broadcastIP();
  Udp.beginPacket(bcast, CHAT_PORT);
  Udp.write((const uint8_t*)packet.c_str(), packet.length());
  Udp.endPacket();
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { /* para placas que lo necesiten */ }

  Serial.println();
  Serial.println("UDP Broadcast Chat - Ethernet.h");
  Serial.println("Escribe y pulsa Enter para enviar a todos.");
  
  Ethernet.begin(mac, ip, IPAddress(192,168,1,1), IPAddress(192,168,1,1), subnet);
  delay(500);

  Serial.print("IP: "); Serial.println(Ethernet.localIP());
  Serial.print("Broadcast: "); Serial.println(broadcastIP());

  // Escuchar en el puerto de chat
  Udp.begin(CHAT_PORT);
  Serial.print("Escuchando UDP puerto "); Serial.println(CHAT_PORT);

  line.reserve(200);
}

void loop() {
  // 1) RX UDP: imprime todo lo que llega (incluido lo tuyo)
  int packetSize = Udp.parsePacket();
  if (packetSize > 0) {
    int n = Udp.read(udpBuf, (int)sizeof(udpBuf) - 1);
    if (n > 0) {
      udpBuf[n] = '\0';
      Serial.print("[UDP] ");
      Serial.println(udpBuf);
    }
  }

  // 2) TX desde Serial: lee línea y la manda por broadcast
  while (Serial.available()) {
    char c = (char)Serial.read();

    // Ignora CR, usa LF como fin de línea
    if (c == '\r') continue;

    if (c == '\n') {
      line.trim();
      if (line.length() > 0) {
        sendChat(line);
      }
      line = "";
    } else {
      // Evita crecer infinito
      if (line.length() < 400) line += c;
    }
  }
}
