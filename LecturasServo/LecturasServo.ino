#include <AccelStepper.h>

// ============================================
// Configuración de Pines
// ============================================

const int EN_PIN = 15;
const int STEP_PIN = 16;
const int DIR_PIN = 17;

#define SERVO_ADDR 0xE0

#define CMD_ENCODER_VALUE     0x30
#define CMD_CUMULATIVE_PULSES 0x33
#define CMD_MOTOR_POSITION    0x36
#define CMD_POSITION_ERROR    0x39

AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

// ============================================
// Estructura de comandos inteligentes
// ============================================

struct CommandInfo {
  byte cmd;
  const char* label;
  byte expectedBytes;
  bool isSigned;
};

CommandInfo commands[] = {
  { CMD_ENCODER_VALUE,     "Encoder Value",     2, true },
  { CMD_CUMULATIVE_PULSES, "Cumulative Pulses", 4, true },
  { CMD_MOTOR_POSITION,    "Motor Position",    4, true },
  { CMD_POSITION_ERROR,    "Position Error",    2, true },
};

const int numCommands = sizeof(commands) / sizeof(commands[0]);

// ============================================
// Variables de control de lectura periódica
// ============================================

bool periodicReadingEnabled = false;
unsigned long lastReadingTime = 0;
const unsigned long readingInterval = 1000; // 1 segundo

// ============================================
// SETUP
// ============================================

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, 19, 18); // RX=19, TX=18

  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW); // Activar driver

  stepper.setMaxSpeed(600);
  stepper.setAcceleration(300);

  Serial.println("\n=== ACCEL+AUTO-DECODING MENU ===");
  showMenu();
}

// ============================================
// LOOP
// ============================================

void loop() {
  if (Serial.available()) {
    char option = Serial.read();

    switch (option) {
      case '1': moveRelative(3200); break;      // Una vuelta adelante
      case '2': moveRelative(-3200); break;     // Una vuelta atrás
      case '3': homeMotor(); break;             // Ir a home
      case '4': stopMotor(); break;             // Parar motor
      case '5': sendCommandAndAutoDisplay(CMD_ENCODER_VALUE); break;
      case '6': sendCommandAndAutoDisplay(CMD_CUMULATIVE_PULSES); break;
      case '7': sendCommandAndAutoDisplay(CMD_MOTOR_POSITION); break;
      case '8': sendCommandAndAutoDisplay(CMD_POSITION_ERROR); break;
      case '9': togglePeriodicReading(); break;
      default: Serial.println("❌ Opción inválida."); break;
    }
    delay(300);
    showMenu();
  }

  stepper.run();

  if (periodicReadingEnabled && (millis() - lastReadingTime >= readingInterval)) {
    lastReadingTime = millis();
    readAllStatus();
  }
}

// ============================================
// Funciones de Movimiento
// ============================================

void moveRelative(long steps) {
  Serial.print("🚀 Moviendo ");
  Serial.print(steps);
  Serial.println(" pasos.");
  stepper.move(steps);
}

void homeMotor() {
  Serial.println("🏠 Volviendo a Home (0)...");
  stepper.moveTo(0);
}

void stopMotor() {
  Serial.println("🛑 Deteniendo motor...");
  stepper.stop();
}

// ============================================
// Funciones de Lectura Inteligente
// ============================================

void sendCommandAndAutoDisplay(byte cmd) {
  byte checksum = SERVO_ADDR + cmd;
  Serial1.write(SERVO_ADDR);
  Serial1.write(cmd);
  Serial1.write(checksum);
  delay(50);

  CommandInfo* info = nullptr;
  for (int i = 0; i < numCommands; i++) {
    if (commands[i].cmd == cmd) {
      info = &commands[i];
      break;
    }
  }

  if (info == nullptr) {
    Serial.println("❌ Comando desconocido.");
    return;
  }

  int expectedBytes = info->expectedBytes;
  if (Serial1.available() < expectedBytes + 1) {
    Serial.println("❌ Sin respuesta o incompleta.");
    return;
  }

  byte response[5] = {0};
  for (int i = 0; i < expectedBytes + 1; i++) {
    response[i] = Serial1.read();
  }

  Serial.print(info->label);
  Serial.print(": ");

  if (expectedBytes == 2) {
    if (info->isSigned) {
      int16_t value = readSigned16(response[1], response[2]);
      Serial.println(value);
    } else {
      uint16_t value = read16(response[1], response[2]);
      Serial.println(value);
    }
  }
  else if (expectedBytes == 4) {
    if (info->isSigned) {
      int32_t value = readSigned32(response[1], response[2], response[3], response[4]);
      Serial.println(value);
    } else {
      uint32_t value = read32(response[1], response[2], response[3], response[4]);
      Serial.println(value);
    }
  }
}

void readAllStatus() {
  Serial.println("\n--- 📡 Lectura periódica ---");
  sendCommandAndAutoDisplay(CMD_ENCODER_VALUE);
  sendCommandAndAutoDisplay(CMD_CUMULATIVE_PULSES);
  sendCommandAndAutoDisplay(CMD_MOTOR_POSITION);
  sendCommandAndAutoDisplay(CMD_POSITION_ERROR);
  Serial.println("------------------------------\n");
}

void togglePeriodicReading() {
  periodicReadingEnabled = !periodicReadingEnabled;
  if (periodicReadingEnabled) {
    Serial.println("✅ Lectura periódica ACTIVADA.");
    lastReadingTime = millis();
  } else {
    Serial.println("🛑 Lectura periódica DESACTIVADA.");
  }
}

// ============================================
// Funciones de Utilidades
// ============================================

uint16_t read16(byte highByte, byte lowByte) {
  return ((uint16_t)highByte << 8) | lowByte;
}

int16_t readSigned16(byte highByte, byte lowByte) {
  return (int16_t)(((uint16_t)highByte << 8) | lowByte);
}

uint32_t read32(byte b1, byte b2, byte b3, byte b4) {
  return ((uint32_t)b1 << 24) | ((uint32_t)b2 << 16) | ((uint32_t)b3 << 8) | b4;
}

int32_t readSigned32(byte b1, byte b2, byte b3, byte b4) {
  return (int32_t)(((uint32_t)b1 << 24) | ((uint32_t)b2 << 16) | ((uint32_t)b3 << 8) | b4);
}

void showMenu() {
  Serial.println("\n--- MENÚ ---");
  Serial.println("1 - Mover 1 vuelta adelante");
  Serial.println("2 - Mover 1 vuelta atrás");
  Serial.println("3 - Ir a Home (0)");
  Serial.println("4 - STOP motor");
  Serial.println("5 - Leer Encoder Value");
  Serial.println("6 - Leer Cumulative Pulses");
  Serial.println("7 - Leer Motor Position");
  Serial.println("8 - Leer Position Error");
  Serial.println("9 - Activar/Desactivar Lectura Periódica (1s)");
  Serial.println("----------------");
}
