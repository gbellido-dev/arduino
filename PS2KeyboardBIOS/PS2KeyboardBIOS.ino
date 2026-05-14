#include <ps2dev.h>

// Configuración de Pines
// Nota: En Bluno Beetle, Serial usa D0(RX) y D1(TX) para el Bluetooth.
// Usamos D2 para Clock y D3 para Data del puerto PS/2.
PS2dev keyboard(2, 3); 
const int LED = LED_BUILTIN;

void setup() {
  // Esperar a que el puerto PS/2 tenga actividad (Init)
  keyboard.keyboard_init();

  // El Bluetooth del Beetle funciona a 115200 por defecto
  Serial.begin(115200); 
  pinMode(LED, OUTPUT);
  
  Serial.println("--- BLUNO BEETLE PS/2 DEBUG ---");
  Serial.println("Estado: Esperando energia del puerto PS/2...");


  
  // Pequeño bucle para confirmar que la BIOS detecta el teclado
  // Si la BIOS envía comandos (como reset 0xFF), los veremos pasar
  Serial.println("Estado: Intentando sincronizar con BIOS...");
  
  digitalWrite(LED, HIGH);
  delay(100); // Pausa de seguridad
  Serial.println("Estado: Iniciando secuencia automatica.");

  // --- INICIO DE SECUENCIA ---
  
  ejecutarAccion("Entrando a BIOS (SUPR)", 0x71, 20, true);
  delay(5000);

  ejecutarAccion("Bajando 2 posiciones", 0x72, 2, true);
  
  Serial.println("Accion: Presionando ENTER");
  enviarTeclaSimple(0x5A); 

  ejecutarAccion("Bajando 9 posiciones", 0x72, 9, true);

  Serial.println("Accion: Presionando ENTER");
  enviarTeclaSimple(0x5A);

  Serial.println("Accion: Subiendo 1 posicion");
  enviarTeclaEspecial(0x75);

  delay(1000);
  Serial.println("Accion: Confirmando con ENTER");
  enviarTeclaSimple(0x5A);

  Serial.println("Accion: Presionando F10 (Guardar)");
  enviarTeclaSimple(0x09);
  
  delay(1000);
  Serial.println("Accion: Confirmando con ENTER");
  enviarTeclaSimple(0x5A);

  Serial.println("--- PROCESO FINALIZADO ---");
  digitalWrite(LED, LOW);
}

void loop() {
  // Modo Monitor: Si la BIOS envía algo al Beetle, lo reenviamos al Bluetooth
  unsigned char comandoBios;
  if(keyboard.read(&comandoBios) == 0) {
    Serial.print("BIOS envio comando: 0x");
    Serial.println(comandoBios, HEX);
  }

  // Parpadeo de "Sistema Vivo"
  digitalWrite(LED, !digitalRead(LED));
  delay(1000);
}

// --- FUNCIONES MEJORADAS CON DEBUG ---

void ejecutarAccion(String msg, unsigned char code, int veces, bool especial) {
  Serial.print("Ejecutando: ");
  Serial.print(msg);
  Serial.print(" (x"); Serial.print(veces); Serial.println(")");
  
  for(int i = 0; i < veces; i++) {
    if(especial) enviarTeclaEspecial(code);
    else enviarTeclaSimple(code);
  }
}

void enviarTeclaSimple(unsigned char code) {
  keyboard.write(code);       
  delay(50);
  keyboard.write(0xF0);       
  keyboard.write(code);       
  delay(300);
}

void enviarTeclaEspecial(unsigned char code) {
  keyboard.write(0xE0);       
  keyboard.write(code);       
  delay(50);
  keyboard.write(0xE0);       
  keyboard.write(0xF0);       
  keyboard.write(code);       
  delay(200);
}