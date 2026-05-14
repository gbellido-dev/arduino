#include <AccelStepper.h>

// ============================================
// Configuración de Pines
// ============================================

const int UART_RX = 19;
const int UART_TX = 18;

#define SERIAL_TIMEOUT 100

// Stepper 1
const int STEP_PIN1 = 16;
const int DIR_PIN1 = 17;

// Stepper 2
const int STEP_PIN2 = 5;
const int DIR_PIN2 = 21;

// Stepper 3
const int STEP_PIN3 = 22;
const int DIR_PIN3 = 23;

// Direcciones
#define ADDR1 0xE3
#define ADDR2 0xE1
#define ADDR3 0xE2

// Comandos
#define CMD_CUMULATIVE_PULSES 0x33
#define CMD_POSITION_ERROR    0x39

AccelStepper stepper1(AccelStepper::DRIVER, STEP_PIN1, DIR_PIN1);
AccelStepper stepper2(AccelStepper::DRIVER, STEP_PIN2, DIR_PIN2);
AccelStepper stepper3(AccelStepper::DRIVER, STEP_PIN3, DIR_PIN3);

// Configuración de Comandos
struct CommandInfo {
  byte cmd;
  const char* label;
  byte expectedBytes;
  bool isSigned;
};

CommandInfo commands[] = {
  { CMD_CUMULATIVE_PULSES, "Cumulative Pulses", 4, true },
  { CMD_POSITION_ERROR,    "Position Error",    2, true },
};

const int numCommands = sizeof(commands) / sizeof(commands[0]);

// Variables de lectura periódica
unsigned long lastReadingTime = 0;
const unsigned long readingInterval = 1000; // 2 segundos

// Variables para guardar las últimas lecturas
int32_t lastCumulativePulses1 = 0, lastCumulativePulses2 = 0, lastCumulativePulses3 = 0;
int16_t lastPositionError1 = 0, lastPositionError2 = 0, lastPositionError3 = 0;

// ============================================
// SETUP
// ============================================

void setup() {
  Serial.begin(115200);
  Serial1.begin(19200, SERIAL_8N1, UART_RX, UART_TX);

  stepper1.setMaxSpeed(8000);
  stepper1.setAcceleration(1000);

  stepper2.setMaxSpeed(8000);
  stepper2.setAcceleration(1000);

  stepper3.setMaxSpeed(8000);
  stepper3.setAcceleration(1000);

  Serial.println("\n=== 3 STEPPERS + MONITORING MENU ===");
  showMenu();
}

// ============================================
// LOOP
// ============================================

void loop() {
  if (Serial.available()) {
    char option = Serial.read();

    switch (option) {
      case '1': moveStepper(stepper1, "Stepper 1", 3200*16); break;
      case '2': moveStepper(stepper2, "Stepper 2", 3200*16); break;
      case '3': moveStepper(stepper3, "Stepper 3", 3200*16); break;
      case '0': showLastReadings(); break;
      case 'a': moveAllSteppersSync(); break;
      default: Serial.println("❌ Opción inválida."); break;
    }
    delay(300);
    showMenu();
  }

  stepper1.run();
  stepper2.run();
  stepper3.run();

  if (millis() - lastReadingTime >= readingInterval) {
    lastReadingTime = millis();
    updateAllReadings();
  }
}

// ============================================
// Funciones de Movimiento
// ============================================

void moveStepper(AccelStepper& motor, const char* motorName, long steps) {
  Serial.print("🚀 Moviendo ");
  Serial.println(motorName);
  motor.move(steps);
}

void moveAllSteppersSync() {
  Serial.println("🚀 Moviendo TODOS los steppers sincronizados...");
  stepper1.move(3200*24);
  stepper2.move(3200*16);
  stepper3.move(3200*8);
}

// ============================================
// Funciones de Lectura y Actualización
// ============================================

void updateAllReadings() {
  lastCumulativePulses1 = readCumulativePulses(ADDR1);
  lastPositionError1    = readPositionError(ADDR1);

  lastCumulativePulses2 = readCumulativePulses(ADDR2);
  lastPositionError2    = readPositionError(ADDR2);

  lastCumulativePulses3 = readCumulativePulses(ADDR3);
  lastPositionError3    = readPositionError(ADDR3);
}

int32_t readCumulativePulses(byte address) {
  return (int32_t) sendCommandAndRead(address, CMD_CUMULATIVE_PULSES);
}

int16_t readPositionError(byte address) {
  return (int16_t) sendCommandAndRead(address, CMD_POSITION_ERROR);
}

int32_t sendCommandAndRead(byte address, byte cmd) {
  byte checksum = address + cmd;
  Serial1.flush();
  while (Serial1.available()) Serial1.read();

  Serial1.write(address);
  Serial1.write(cmd);
  Serial1.write(checksum);

  unsigned long startTime = millis();
  int expectedBytes = 0;

  for (int i = 0; i < numCommands; i++) {
    if (commands[i].cmd == cmd) {
      expectedBytes = commands[i].expectedBytes;
      break;
    }
  }

  while (Serial1.available() < expectedBytes + 1) {
    if (millis() - startTime > SERIAL_TIMEOUT) {
      Serial.print("Timeout on addr 0x");Serial.println(address, HEX);
      return 0; // Retornar 0 en caso de timeout
    }
  }

  byte response[5] = {0};
  for (int i = 0; i < expectedBytes + 1; i++) {
    response[i] = Serial1.read();
  }

  if (expectedBytes == 2) {
    return readSigned16(response[1], response[2]);
  } else if (expectedBytes == 4) {
    return readSigned32(response[1], response[2], response[3], response[4]);
  }
  return 0;
}

void showLastReadings() {
  Serial.println("\n📊 Últimas lecturas:");

  Serial.print("[Stepper 1] Pulsos: ");
  Serial.print(lastCumulativePulses1);
  Serial.print(" (");
  Serial.print(lastCumulativePulses1 * (360.0 / 3200.0), 2);
  Serial.println("º)");

  Serial.print("[Stepper 1] Error: ");
  Serial.print(lastPositionError1);
  Serial.print(" (");
  Serial.print(lastPositionError1 * 0.00559, 2);
  Serial.println("º)");

  Serial.print("[Stepper 2] Pulsos: ");
  Serial.print(lastCumulativePulses2);
  Serial.print(" (");
  Serial.print(lastCumulativePulses2 * (360.0 / 3200.0), 2);
  Serial.println("º)");

  Serial.print("[Stepper 2] Error: ");
  Serial.print(lastPositionError2);
  Serial.print(" (");
  Serial.print(lastPositionError2 * 0.00559, 2);
  Serial.println("º)");

  Serial.print("[Stepper 3] Pulsos: ");
  Serial.print(lastCumulativePulses3);
  Serial.print(" (");
  Serial.print(lastCumulativePulses3 * (360.0 / 3200.0), 2);
  Serial.println("º)");

  Serial.print("[Stepper 3] Error: ");
  Serial.print(lastPositionError3);
  Serial.print(" (");
  Serial.print(lastPositionError3 * 0.00559, 2);
  Serial.println("º)");
}

// ============================================
// Funciones auxiliares
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
  Serial.println("1 - Mover Stepper 1 (8 vueltas adelante)");
  Serial.println("2 - Mover Stepper 2 (8 vueltas adelante)");
  Serial.println("3 - Mover Stepper 3 (8 vueltas adelante)");
  Serial.println("0 - Mostrar lecturas actuales");
  Serial.println("a - Mover TODOS los steppers sincronizados (8 vueltas)");
  Serial.println("----------------");
}
