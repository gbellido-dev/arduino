#include <Arduino.h>
#include <Bluepad32.h>

// ===================== Axis configuration =====================
#define STEER_INVERT -1    // -1 si gira al revés
#define THROTTLE_INVERT 1  // -1 si acelera al revés

#define USE_THROTTLE_LEFT_Y 1  // 1=axisY(), 0=axisRY()
#define USE_STEER_RIGHT_X 1    // 1=axisRX(), 0=axisX()

// ===================== Arm button selection =====================
#define ARM_USE_X 1
#define ARM_USE_Y 0
#define ARM_USE_A 0
#define ARM_USE_B 0

#define ARM_BUTTON_MASK 0x0008


// --- Auto reverse for car ESC (Forward/Brake/Reverse) ---
static const uint32_t AUTO_NEUTRAL_MS = 300;  // tiempo en neutro para habilitar reverse
static const uint32_t AUTO_BRAKE_MS = 120;     // breve freno al pasar de + a -
static const int REV_REQUEST_GATE = 60;       // cuanto stick atrás hace falta para pedir reverse (0..512)

// --- Auto reverse state ---
enum AutoRevState { AR_NORMAL, AR_BRAKE, AR_NEUTRAL };
AutoRevState arState = AR_NORMAL;
uint32_t arTs = 0;
int lastThr = 0;   // thr procesado anterior (-512..512)




// ===================== Pins =====================
static const int PIN_STEER = 25;
static const int PIN_ESC = 26;
static const int PIN_REPAIR = 27;  // botón a GND (INPUT_PULLUP)

// ===================== PWM (LEDC classic API) =====================
static const int PWM_FREQ = 50;
static const int PWM_RES = 16;
static const int CH_STEER = 0;
static const int CH_ESC = 1;

// ===================== Pulse ranges =====================
static const int STEER_MIN_US = 1000;
static const int STEER_MAX_US = 2000;
static const int STEER_CENTER_US = 1500;

static const int ESC_MIN_US = 1000;
static const int ESC_MAX_US = 2000;
static const int ESC_NEUTRAL_US = 1500;

// ===================== Stick shaping =====================
static const int DEADZONE = 30;
static const float EXPO_STEER = 0.35f;
static const float EXPO_THR = 0.25f;

// ===================== Reverse behavior (RC-like) =====================
static const int REVERSE_GATE = 70;
static const uint32_t BRAKE_HOLD_MS = 250;
static const uint32_t NEUTRAL_CONFIRM_MS = 180;

// ===================== Ramps / failsafe =====================
static const int ESC_SLEW_STEP_US = 8;
static const uint32_t LOOP_DT_MS = 5;
static const uint32_t INPUT_TIMEOUT_MS = 500;

// ===================== REPAIR / ARM via button =====================
static const uint32_t REPAIR_DEBOUNCE_MS = 30;
static const uint32_t REPAIR_HOLD_FOR_ARM_MS = 1500;     // ARM/DISARM de emergencia
static const uint32_t REPAIR_HOLD_FOR_REPAIR_MS = 3000;  // forget+reboot

// ===================== State =====================
ControllerPtr myCtl = nullptr;
bool armed = false;

enum EscMode { MODE_FWD,
               MODE_BRAKE,
               MODE_REV };
EscMode escMode = MODE_FWD;
uint32_t modeTs = 0;

int currentEscUs = ESC_NEUTRAL_US;
uint32_t lastInputTime = 0;

// ---------- PWM helpers ----------
uint32_t usToDuty(int us) {
  const int period_us = 1000000 / PWM_FREQ;  // 20000
  const uint32_t maxDuty = (1u << PWM_RES) - 1u;
  return (uint32_t)(((uint64_t)us * maxDuty) / period_us);
}

void writeUs(int channel, int us) {
  if (channel == CH_STEER) {
    us = constrain(us, STEER_MIN_US, STEER_MAX_US);
  } else {
    us = constrain(us, ESC_MIN_US, ESC_MAX_US);
  }
  ledcWrite(channel, usToDuty(us));
}

int mapCentered(int v, int minUs, int centerUs, int maxUs) {
  if (v >= 0) return centerUs + (int)(((long)v * (maxUs - centerUs)) / 512L);
  else return centerUs + (int)(((long)v * (centerUs - minUs)) / 512L);
}

int applyDeadzone(int v) {
  return (abs(v) < DEADZONE) ? 0 : v;
}

int applyExpo(int v, float expo) {
  float x = (float)v / 512.0f;
  float y = (1.0f - expo) * x + expo * x * x * x;
  return (int)(y * 512.0f);
}

int rampToTarget(int current, int target, int stepUs) {
  if (target > current) return min(current + stepUs, target);
  if (target < current) return max(current - stepUs, target);
  return current;
}

// ---------- Bluepad callbacks ----------
void onConnectedController(ControllerPtr ctl) {
  // Acepta solo 1 (el primero)
  if (!myCtl) {
    myCtl = ctl;
    armed = false;
    escMode = MODE_FWD;
    currentEscUs = ESC_NEUTRAL_US;
    lastInputTime = millis();

    auto p = ctl->getProperties();
    Serial.printf("Controller connected. Class=%d  addr=%02X:%02X:%02X:%02X:%02X:%02X\n",
                  ctl->getClass(),
                  p.btaddr[0], p.btaddr[1], p.btaddr[2], p.btaddr[3], p.btaddr[4], p.btaddr[5]);
    Serial.println("DISARMED. Press ARM button (X/A/B/Y) or hold REPAIR 1.5s.");
  } else {
    Serial.println("Another controller connected (ignored).");
  }
}

void onDisconnectedController(ControllerPtr ctl) {
  if (ctl == myCtl) {
    myCtl = nullptr;
    armed = false;
    escMode = MODE_FWD;
    currentEscUs = ESC_NEUTRAL_US;
    Serial.println("Controller disconnected -> FAILSAFE.");
  }
}

// Arm/disarm button (configurable)
bool isArmTogglePressed(ControllerPtr ctl) {
  uint16_t b = ctl->buttons();
  return (b & ARM_BUTTON_MASK) != 0;
}


// ---------- REPAIR button ----------
bool repairButtonPressedRaw() {
  return digitalRead(PIN_REPAIR) == LOW;
}

bool repairButtonPressedDebounced() {
  static bool last = false;
  static uint32_t lastChange = 0;

  bool now = repairButtonPressedRaw();
  if (now != last) {
    lastChange = millis();
    last = now;
  }
  if (millis() - lastChange >= REPAIR_DEBOUNCE_MS) return now;
  return last;
}

void doRePairAndReboot() {
  Serial.println("REPAIR: forgetting BT keys and rebooting...");
  writeUs(CH_STEER, STEER_CENTER_US);
  writeUs(CH_ESC, ESC_NEUTRAL_US);
  delay(80);
  BP32.forgetBluetoothKeys();
  delay(150);
  ESP.restart();
}

void setupPwm() {
  ledcSetup(CH_STEER, PWM_FREQ, PWM_RES);
  ledcSetup(CH_ESC, PWM_FREQ, PWM_RES);
  ledcAttachPin(PIN_STEER, CH_STEER);
  ledcAttachPin(PIN_ESC, CH_ESC);
  writeUs(CH_STEER, STEER_CENTER_US);
  writeUs(CH_ESC, ESC_NEUTRAL_US);
}

void setup() {
  Serial.begin(115200);
  delay(150);

  pinMode(PIN_REPAIR, INPUT_PULLUP);
  setupPwm();

  BP32.setup(&onConnectedController, &onDisconnectedController);

  Serial.println("Bluepad32 ready.");
  Serial.println("Tip: keep phone BT OFF during pairing to avoid noise devices.");
}

// ---------- Loop ----------
void loop() {
  BP32.update();

  // ---------- DEBUG específico coche ----------
  if (myCtl && myCtl->isConnected()) {
    static uint32_t tDbg = 0;
    if (millis() - tDbg > 100) {
      tDbg = millis();

      int rawY = myCtl->axisY();
      int rawRX = myCtl->axisRX();

      int thrRaw = THROTTLE_INVERT * (-rawY);
      int steerRaw = STEER_INVERT * rawRX;

      int thrProc = applyExpo(applyDeadzone(thrRaw), EXPO_THR);
      int steerProc = applyExpo(applyDeadzone(steerRaw), EXPO_STEER);

      int steerUs = mapCentered(steerProc, STEER_MIN_US, STEER_CENTER_US, STEER_MAX_US);
      int escUs = mapCentered(thrProc, ESC_MIN_US, ESC_NEUTRAL_US, ESC_MAX_US);

      Serial.printf("Y=%4d RX=%4d | thrRaw=%4d steerRaw=%4d | thrProc=%4d steerProc=%4d | steerUs=%4d escUs=%4d | armed=%d\n",
                    rawY, rawRX,
                    thrRaw, steerRaw,
                    thrProc, steerProc,
                    steerUs, escUs,
                    armed ? 1 : 0);
    }
  }

  const uint32_t now = millis();

  // ---------- REPAIR / ARM via button ----------
  static uint32_t holdStart = 0;
  static bool wasPressed = false;

  bool pressed = repairButtonPressedDebounced();
  if (pressed && !wasPressed) {
    holdStart = now;
  } else if (!pressed && wasPressed) {
    holdStart = 0;
  }
  wasPressed = pressed;

  if (pressed && holdStart != 0) {
    uint32_t held = now - holdStart;

    // 3s -> full repair
    if (held >= REPAIR_HOLD_FOR_REPAIR_MS) {
      doRePairAndReboot();
    }

    // 1.5s -> emergency ARM toggle (sin borrar keys)
    static bool armToggledThisHold = false;
    if (!armToggledThisHold && held >= REPAIR_HOLD_FOR_ARM_MS) {
      armToggledThisHold = true;
      if (myCtl && myCtl->isConnected()) {
        armed = !armed;
        Serial.println(armed ? "ARMED (via REPAIR hold)" : "DISARMED (via REPAIR hold)");
        writeUs(CH_STEER, STEER_CENTER_US);
        currentEscUs = ESC_NEUTRAL_US;
        writeUs(CH_ESC, currentEscUs);
        delay(200);
      } else {
        Serial.println("No controller connected -> can't arm.");
      }
    }
    if (!pressed) armToggledThisHold = false;
  }

  // ---------- Status log: why HOLD ----------
  static uint32_t lastStatus = 0;
  if (now - lastStatus > 1000) {
    lastStatus = now;
    if (!myCtl) Serial.println("HOLD: no controller yet.");
    else if (!myCtl->isConnected()) Serial.println("HOLD: controller object but not connected.");
    else if (!armed) Serial.println("HOLD: connected but DISARMED.");
  }

  // Hard failsafe if not connected or disarmed
  /*if (!myCtl || !myCtl->isConnected() || !armed) {
    writeUs(CH_STEER, STEER_CENTER_US);
    currentEscUs = rampToTarget(currentEscUs, ESC_NEUTRAL_US, ESC_SLEW_STEP_US);
    writeUs(CH_ESC, currentEscUs);
    delay(LOOP_DT_MS);
    return;
  }*/

  // ---------- Si hay mando conectado, mostrar botones SIEMPRE ----------
  if (myCtl && myCtl->isConnected()) {
    static uint16_t lastB = 0xFFFF;
    uint16_t braw = myCtl->buttons();
    if (braw != lastB) {
      lastB = braw;
      Serial.printf("buttons raw=0x%04X\n", braw);
    }

    // ARM toggle from controller (aunque esté DISARMED)
    static bool lastArmBtn = false;
    bool armBtn = isArmTogglePressed(myCtl);

    // Nota: para armar exigimos gas en neutro (seguridad)
    int rawThrottle;
#if USE_THROTTLE_LEFT_Y
    rawThrottle = myCtl->axisY();
#else
    rawThrottle = myCtl->axisRY();
#endif
    int thr = THROTTLE_INVERT * (-rawThrottle);
    thr = applyDeadzone(thr);

    if (armBtn && !lastArmBtn) {
      if (!armed) {
        if (abs(thr) <= DEADZONE) {
          armed = true;
          escMode = MODE_FWD;
          modeTs = now;
          lastInputTime = now;
          Serial.println("ARMED (via controller)");
        } else {
          Serial.println("Not armed: put throttle at neutral.");
        }
      } else {
        armed = false;
        escMode = MODE_FWD;
        Serial.println("DISARMED (via controller)");
      }

      writeUs(CH_STEER, STEER_CENTER_US);
      currentEscUs = ESC_NEUTRAL_US;
      writeUs(CH_ESC, currentEscUs);
      delay(180);
    }
    lastArmBtn = armBtn;
  }

  // ---------- Hard failsafe if not connected or disarmed ----------
  if (!myCtl || !myCtl->isConnected() || !armed) {
    writeUs(CH_STEER, STEER_CENTER_US);
    currentEscUs = rampToTarget(currentEscUs, ESC_NEUTRAL_US, ESC_SLEW_STEP_US);
    writeUs(CH_ESC, currentEscUs);
    delay(LOOP_DT_MS);
    return;
  }


  // ===== Read sticks =====
  int rawThrottle, rawSteer;

#if USE_THROTTLE_LEFT_Y
  rawThrottle = myCtl->axisY();
#else
  rawThrottle = myCtl->axisRY();
#endif

#if USE_STEER_RIGHT_X
  rawSteer = myCtl->axisRX();
#else
  rawSteer = myCtl->axisX();
#endif

  int thr = THROTTLE_INVERT * (-rawThrottle);
  int steer = STEER_INVERT * rawSteer;

  thr = applyExpo(applyDeadzone(thr), EXPO_THR);
  steer = applyExpo(applyDeadzone(steer), EXPO_STEER);

  if (abs(thr) > DEADZONE || abs(steer) > DEADZONE) lastInputTime = now;

  // Arm toggle from controller
  static bool lastArmBtn = false;
  bool armBtn = isArmTogglePressed(myCtl);
  static uint16_t lastB = 0;
  uint16_t braw = myCtl->buttons();
  if (braw != lastB) {
    lastB = braw;
    Serial.printf("buttons raw=0x%04X\n", braw);
  }

  if (armBtn && !lastArmBtn) {
    if (!armed) {
      if (abs(thr) <= DEADZONE) {
        armed = true;
        escMode = MODE_FWD;
        modeTs = now;
        lastInputTime = now;
        Serial.println("ARMED (via controller)");
      } else {
        Serial.println("Not armed: put throttle at neutral.");
      }
    } else {
      armed = false;
      escMode = MODE_FWD;
      Serial.println("DISARMED (via controller)");
    }
    writeUs(CH_STEER, STEER_CENTER_US);
    currentEscUs = ESC_NEUTRAL_US;
    writeUs(CH_ESC, currentEscUs);
    delay(180);
  }
  lastArmBtn = armBtn;

  if (!armed) {
    delay(LOOP_DT_MS);
    return;
  }

  // Failsafe timeout
  if (now - lastInputTime > INPUT_TIMEOUT_MS) {
    writeUs(CH_STEER, STEER_CENTER_US);
    currentEscUs = rampToTarget(currentEscUs, ESC_NEUTRAL_US, ESC_SLEW_STEP_US);
    writeUs(CH_ESC, currentEscUs);
    delay(LOOP_DT_MS);
    return;
  }

  // Steering
  int steerUs = mapCentered(steer, STEER_MIN_US, STEER_CENTER_US, STEER_MAX_US);
  writeUs(CH_STEER, steerUs);

  // ---------- Throttle: Auto-reverse (sin doble toque manual) ----------
  int targetEscUs = ESC_NEUTRAL_US;

  // Detectar petición de reverse fuerte
  bool wantReverse = (thr < -REV_REQUEST_GATE);

  // Detectar si veníamos de avance y ahora piden atrás
  bool forwardToReverse = (lastThr > +REV_REQUEST_GATE) && wantReverse;

  // Máquina de estados de auto-secuencia
  switch (arState) {
    case AR_NORMAL:
      if (forwardToReverse) {
        arState = AR_BRAKE;
        arTs = now;
      } else {
        // normal: proporcional bidireccional directo
        targetEscUs = mapCentered(thr, ESC_MIN_US, ESC_NEUTRAL_US, ESC_MAX_US);
      }
      break;

    case AR_BRAKE:
      // mandar un freno corto (pulsito)
      targetEscUs = mapCentered(-200, ESC_MIN_US, ESC_NEUTRAL_US, ESC_MAX_US);  // freno moderado
      if (now - arTs >= AUTO_BRAKE_MS) {
        arState = AR_NEUTRAL;
        arTs = now;
      }
      break;

    case AR_NEUTRAL:
      // confirmar neutro un momento para que el ESC habilite reverse
      targetEscUs = ESC_NEUTRAL_US;
      if (now - arTs >= AUTO_NEUTRAL_MS) {
        arState = AR_NORMAL;
        // al salir, aplicamos el reverse real (proporcional)
        targetEscUs = mapCentered(thr, ESC_MIN_US, ESC_NEUTRAL_US, ESC_MAX_US);
      }
      break;
  }

  // Guardar thr para detectar transiciones
  lastThr = thr;

  // Ramp y salida
  currentEscUs = rampToTarget(currentEscUs, targetEscUs, ESC_SLEW_STEP_US);
  writeUs(CH_ESC, currentEscUs);


  delay(LOOP_DT_MS);
}
