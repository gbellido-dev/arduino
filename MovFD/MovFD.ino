// ============================================
// Configuración General
// ============================================

#define MOTOR_BAUDRATE 38400
#define MOTOR_ADDR 0xE0   // Dirección de tu driver

// ============================================
// Variables
// ============================================

bool moving = false;

// ============================================
// Setup
// ============================================

void setup() {
  Serial.begin(115200);
  Serial1.begin(MOTOR_BAUDRATE, SERIAL_8N1, 19, 18);
  Serial.println("=== MKS SERVO42C MOVE (ESP32 style) ===");
  showMenu();
}

// ============================================
// Loop
// ============================================

void loop() {
  if (Serial.available()) {
    char option = Serial.read();

    switch (option) {
      case '1': requestMove(true); break;  // Adelante
      case '2': requestMove(false); break; // Reversa
      case 's': stopMotor(); break;
      case 'm': showMenu(); break;
      default:
        Serial.println("Invalid option. Press 'm' to show menu again.");
        break;
    }
    delay(300);
  }
}

// ============================================
// Funciones principales
// ============================================

void requestMove(bool forward) {
  Serial.println("Enter speed档位 (0-1279):");
  while (!Serial.available());
  int16_t speed = Serial.parseInt();  // puede ser negativo para dirección

  if (speed > 1279) speed = 1279;
  if (speed < -1279) speed = -1279;

  Serial.println("Enter acceleration (0-255):");
  while (!Serial.available());
  uint8_t acceleration = Serial.parseInt();

  Serial.println("Enter number of pulses:");
  while (!Serial.available());
  uint32_t pulses = Serial.parseInt();

  moveMotor(speed, acceleration, pulses, forward);
}

void moveMotor(int16_t speedLevel, uint8_t acceleration, uint32_t pulses, bool forward) {
  if (!forward) {
    speedLevel = -abs(speedLevel); // Forzar dirección reversa con signo
  } else {
    speedLevel = abs(speedLevel); // Direccion positiva
  }

  // Ahora empaquetamos como ESP32
  if (speedLevel < 0) {
    speedLevel = (-speedLevel) | (1 << 15); // Set bit15
  }

  uint8_t speedHigh = (speedLevel >> 8) & 0xFF; // bits 15-8
  uint8_t speedLow  = speedLevel & 0xFF;        // bits 7-0

  uint8_t pulseHigh = (pulses >> 16) & 0xFF;
  uint8_t pulseMid  = (pulses >> 8) & 0xFF;
  uint8_t pulseLow  = pulses & 0xFF;

  // Comando completo
  uint8_t message[9];
  message[0] = MOTOR_ADDR;
  message[1] = 0xFD;             // MOVIMIENTO comando
  message[2] = 0x14;
  message[3] = 0xFF;
  message[4] = acceleration;
  message[5] = pulseHigh;
  message[6] = pulseMid;
  message[7] = pulseLow;
  message[8] = calculateChecksum(message, 8);

  // Enviar
  Serial1.write(message, 9);

  Serial.println("✅ Move command sent:");
  debugPrint(message, 9);
  moving = true;
}

void stopMotor() {
  uint8_t checksum = MOTOR_ADDR + 0xF7;
  Serial1.write(MOTOR_ADDR);
  Serial1.write(0xF7);
  Serial1.write(checksum);

  Serial.println("🛑 Motor STOP sent.");
  moving = false;
}

void showMenu() {
  Serial.println("\n--- MKS MOVE (ESP32 Style) ---");
  Serial.println("Press:");
  Serial.println("1 - Move FORWARD");
  Serial.println("2 - Move REVERSE");
  Serial.println("s - STOP motor");
  Serial.println("m - Show this menu again");
  Serial.println("--------------------------------");
}

// ============================================
// Utilidades
// ============================================

uint8_t calculateChecksum(const uint8_t* buffer, uint8_t length) {
  uint8_t checksum = 0;
  for (uint8_t i = 0; i < length; i++) {
    checksum += buffer[i];
  }
  return checksum & 0xFF;
}

void debugPrint(const uint8_t* buffer, uint8_t length) {
  Serial.print("Sent bytes: ");
  for (uint8_t i = 0; i < length; i++) {
    Serial.print("0x");
    if (buffer[i] < 16) Serial.print("0");
    Serial.print(buffer[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}
