#include <Arduino.h>
#include <Bluepad32.h>

// ===================== USER CONFIG =====================

// ---- Axes & inversion ----
#define STEER_INVERT       -1    
#define THROTTLE_INVERT     1    
#define ARM_BUTTON_MASK   0x0008   

// ---- Pins ----
static const int PIN_STEER   = 25;  
static const int PIN_ESC     = 26;  
static const int PIN_REPAIR  = 27;  

// ---- NeoPixel ----
#define USE_NEOPIXELS     1
static const int PIN_NEOPIX  = 33;  
static const int NUM_PIXELS  = 4;

// ===================== PWM =====================
static const int PWM_FREQ = 50;
static const int PWM_RES  = 16;
static const int CH_STEER = 0;
static const int CH_ESC   = 1;

static const int STEER_MIN_US    = 1000;
static const int STEER_MAX_US    = 2000;
static const int STEER_CENTER_US = 1500;

static const int ESC_MIN_US      = 1250;
static const int ESC_MAX_US      = 1750;
static const int ESC_NEUTRAL_US  = 1500;

// ===================== CONFIGURACIÓN FINA =====================
static const int   DEADZONE    = 30;
static const float EXPO_STEER  = 0.35f;
static const float EXPO_THR    = 0.25f;

// Rampa suave para conducción normal
static const int      ESC_SLEW_STEP_US    = 20;   
static const uint32_t LOOP_DT_MS         = 5;
static const uint32_t INPUT_TIMEOUT_MS   = 10000; 

// ===================== REPAIR =====================
static const uint32_t REPAIR_DEBOUNCE_MS      = 30;
static const uint32_t REPAIR_HOLD_FOR_REPAIR_MS = 3000; 

// ===================== LÓGICA REVERSA / FRENO =====================
static const int REV_REQUEST_GATE = 60;      

// CAMBIO 1: Tiempos
// 250ms de freno asegura que la inercia se rompa.
// 100ms de neutro es suficiente para resetear el ESC.
static const uint32_t T_BRAKETAP_MS   = 250;  
static const uint32_t T_NEUTRALTAP_MS = 100;  

// CAMBIO 2: Fuerza del Freno
// Antes 1200. Ahora 1000 (Fuerza máxima). 
// Esto es lo que hará que el coche clave ruedas.
static const int THR_BRAKE_US   = 1000; 
static const int THR_REV_MIN_US = 1350; 

// ===================== Variables =====================
ControllerPtr myCtl = nullptr;
bool armed = false;
int currentEscUs = ESC_NEUTRAL_US;
uint32_t lastInputTime = 0;

enum RevState { RS_IDLE, RS_BRAKE, RS_NEUTRAL, RS_REV };
static RevState revState = RS_IDLE;
static uint32_t revTimer = 0;

#if USE_NEOPIXELS
  #include <Adafruit_NeoPixel.h>
  Adafruit_NeoPixel pixels(NUM_PIXELS, PIN_NEOPIX, NEO_GRB + NEO_KHZ800);
#endif

// ---------- Helpers ----------
static inline int clampi(int v, int lo, int hi) { return (v < lo) ? lo : (v > hi) ? hi : v; }

uint32_t usToDuty(int us) {
  const int period_us = 1000000 / PWM_FREQ; 
  const uint32_t maxDuty = (1u << PWM_RES) - 1u;
  return (uint32_t)(((uint64_t)us * maxDuty) / period_us);
}

void writeUs(int channel, int us) {
  if (channel == CH_STEER) us = clampi(us, STEER_MIN_US, STEER_MAX_US);
  else                     us = clampi(us, ESC_MIN_US, ESC_MAX_US);
  ledcWrite(channel, usToDuty(us));
}

int mapCentered(int v, int minUs, int centerUs, int maxUs) {
  v = clampi(v, -512, 512);
  if (v >= 0) return centerUs + (int)(((long)v * (maxUs - centerUs)) / 512L);
  else        return centerUs + (int)(((long)v * (centerUs - minUs)) / 512L);
}

int applyDeadzone(int v) { return (abs(v) < DEADZONE) ? 0 : v; }

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

// ---------- NeoPixel status ----------
#if USE_NEOPIXELS
uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) { return pixels.Color(r, g, b); }
void setAll(uint32_t c) { for (int i = 0; i < NUM_PIXELS; i++) pixels.setPixelColor(i, c); pixels.show(); }
void statusDisarmed() { setAll(rgb(50, 0, 0)); }
void statusArmed()    { setAll(rgb(0, 50, 0)); }
void statusBrake()    { setAll(rgb(255, 0, 0)); } // Rojo Intenso al frenar
void statusRev()      { setAll(rgb(0, 0, 100)); } // Azul
void statusFailsafe() { 
  static bool on = false; on=!on; setAll(on?rgb(80,0,0):rgb(0,0,0)); 
}
#endif

// ---------- REPAIR ----------
bool repairBtn() { return digitalRead(PIN_REPAIR) == LOW; }
void doRePair() {
  BP32.forgetBluetoothKeys();
  ESP.restart();
}

// ---------- Callbacks ----------
void onConnectedController(ControllerPtr ctl) {
  if (!myCtl) {
    myCtl = ctl;
    armed = false;
    revState = RS_IDLE;
    lastInputTime = millis();
    Serial.println("Connected.");
    #if USE_NEOPIXELS
    statusDisarmed();
    #endif
  }
}
void onDisconnectedController(ControllerPtr ctl) {
  if (ctl == myCtl) myCtl = nullptr;
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  pinMode(PIN_REPAIR, INPUT_PULLUP);
  
  ledcSetup(CH_STEER, PWM_FREQ, PWM_RES);
  ledcSetup(CH_ESC,   PWM_FREQ, PWM_RES);
  ledcAttachPin(PIN_STEER, CH_STEER);
  ledcAttachPin(PIN_ESC,   CH_ESC);
  writeUs(CH_STEER, STEER_CENTER_US);
  writeUs(CH_ESC,   ESC_NEUTRAL_US);

#if USE_NEOPIXELS
  pixels.begin(); pixels.setBrightness(200); setAll(0);
#endif

  BP32.setup(&onConnectedController, &onDisconnectedController);
  Serial.println("Ready.");
}

// ---------- Main Loop ----------
void loop() {
  BP32.update();
  uint32_t now = millis();

  // 1. Repair
  static uint32_t repStart = 0;
  if (repairBtn()) {
    if (repStart == 0) repStart = now;
    if (now - repStart > REPAIR_HOLD_FOR_REPAIR_MS) doRePair();
  } else repStart = 0;

  // 2. Failsafe
  if (!myCtl || !myCtl->isConnected() || (armed && (now - lastInputTime > INPUT_TIMEOUT_MS))) {
    writeUs(CH_STEER, STEER_CENTER_US);
    writeUs(CH_ESC, ESC_NEUTRAL_US); 
    #if USE_NEOPIXELS
    statusFailsafe();
    #endif
    delay(LOOP_DT_MS);
    return;
  }

  // 3. Inputs
  int rawY  = myCtl->axisY();
  int rawRX = myCtl->axisRX();
  if (abs(rawY) > DEADZONE || abs(rawRX) > DEADZONE || myCtl->buttons()) lastInputTime = now;

  int thr   = applyExpo(applyDeadzone(THROTTLE_INVERT * (-rawY)), EXPO_THR);
  int steer = applyExpo(applyDeadzone(STEER_INVERT * (rawRX)), EXPO_STEER);

  // 4. Arming
  static bool lastArmBtn = false;
  bool armBtn = (myCtl->buttons() & ARM_BUTTON_MASK);
  if (armBtn && !lastArmBtn) {
    if (!armed && abs(thr) < 10) { armed = true; revState = RS_IDLE; Serial.println("ARMED"); }
    else { armed = false; Serial.println("DISARMED"); }
  }
  lastArmBtn = armBtn;

  if (!armed) {
    writeUs(CH_STEER, STEER_CENTER_US);
    writeUs(CH_ESC, ESC_NEUTRAL_US);
    #if USE_NEOPIXELS
    statusDisarmed();
    #endif
    delay(LOOP_DT_MS);
    return;
  }

  // 5. Steering
  writeUs(CH_STEER, mapCentered(steer, STEER_MIN_US, STEER_CENTER_US, STEER_MAX_US));

  // 6. Throttle & Reverse Logic
  bool wantReverse = (thr < -REV_REQUEST_GATE);
  bool skipRamp = false;
  int targetEscUs = ESC_NEUTRAL_US;

  switch (revState) {
    case RS_IDLE:
      if (wantReverse) {
        revState = RS_BRAKE;
        revTimer = now;
        Serial.println(">> BRAKE (Max Force)");
      } else {
        if (thr < 0) thr = 0; 
        targetEscUs = mapCentered(thr, ESC_MIN_US, ESC_NEUTRAL_US, ESC_MAX_US);
        #if USE_NEOPIXELS
        statusArmed();
        #endif
      }
      break;

    case RS_BRAKE:
      // Freno a fondo (1000us)
      targetEscUs = THR_BRAKE_US; 
      skipRamp = true; 
      #if USE_NEOPIXELS
      statusBrake();
      #endif
      
      if (!wantReverse) revState = RS_IDLE;
      else if (now - revTimer > T_BRAKETAP_MS) {
        revState = RS_NEUTRAL;
        revTimer = now;
        Serial.println(">> NEUTRAL");
      }
      break;

    case RS_NEUTRAL:
      targetEscUs = ESC_NEUTRAL_US;
      skipRamp = true; 
      #if USE_NEOPIXELS
      statusBrake(); // Mantenemos rojo o cambiamos a otro si prefieres
      #endif

      if (!wantReverse) revState = RS_IDLE;
      else if (now - revTimer > T_NEUTRALTAP_MS) {
        revState = RS_REV;
        Serial.println(">> REVERSE");
      }
      break;

    case RS_REV:
      if (!wantReverse) {
        revState = RS_IDLE;
        targetEscUs = ESC_NEUTRAL_US;
      } else {
        int revMap = mapCentered(thr, ESC_MIN_US, ESC_NEUTRAL_US, ESC_MAX_US);
        if (revMap > THR_REV_MIN_US) revMap = THR_REV_MIN_US; 
        targetEscUs = revMap;
        #if USE_NEOPIXELS
        statusRev();
        #endif
      }
      break;
  }

  // 7. Output
  if (skipRamp) {
    currentEscUs = targetEscUs; // Salto instantáneo
  } else {
    currentEscUs = rampToTarget(currentEscUs, targetEscUs, ESC_SLEW_STEP_US);
  }
  
  writeUs(CH_ESC, currentEscUs);
  delay(LOOP_DT_MS);
}