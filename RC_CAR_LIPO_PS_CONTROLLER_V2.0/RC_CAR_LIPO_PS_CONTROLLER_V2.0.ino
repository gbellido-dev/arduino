#include <Arduino.h>
#include <Bluepad32.h>

// ===================== USER CONFIG =====================
#define USE_SERIAL_DEBUG    1   
#define USE_WIFI_DEBUG      1   // 1 = Dashboard WiFi y OTA con WEBSOCKETS
#define USE_POT_SPEED       0   
#define USE_POT_STEER_LIMIT 0   
#define USE_POT_STEER_TRIM  0   
#define USE_NEOPIXELS       1   

// ====== LIBRERÍAS DE RED (MOVIDAS ARRIBA PARA EVITAR ERROR WStype_t) ======
#if USE_WIFI_DEBUG
  #include <WiFi.h>
  #include <WebServer.h>
  #include <ArduinoOTA.h>
  #include <WebSocketsServer.h> 
#endif

// ---- Axes & inversion ----
#define STEER_INVERT       -1    
#define THROTTLE_INVERT     1    
#define ARM_BUTTON_MASK   0x0008   

// ---- Pines ----
static const int PIN_STEER   = 25;  
static const int PIN_ESC     = 26;  
static const int PIN_REPAIR  = 27;  

#if USE_POT_SPEED
  static const int PIN_POT_SPEED = 34; 
#endif
#if USE_POT_STEER_LIMIT
  static const int PIN_POT_LIMIT = 35; 
#endif
#if USE_POT_STEER_TRIM
  static const int PIN_POT_TRIM  = 32; 
#endif

#if USE_NEOPIXELS
  static const int PIN_NEOPIX  = 33;  
  static const int NUM_PIXELS  = 4;
#endif

// ===================== PWM & RANGOS =====================
static const int PWM_FREQ = 50;
static const int PWM_RES  = 16;
static const int CH_STEER = 0;
static const int CH_ESC   = 1;

static const int STEER_MIN_US    = 1000;
static const int STEER_MAX_US    = 2000;
static const int STEER_CENTER_US = 1500;

static const int ESC_MIN_US      = 1000;
static const int ESC_MAX_US      = 2000;
static const int ESC_NEUTRAL_US  = 1500;

// ===================== CONFIGURACIÓN FINA =====================
static const int   DEADZONE    = 30;
static const float EXPO_STEER  = 0.35f;
static const float EXPO_THR    = 0.25f;

static const int      ESC_SLEW_STEP_US   = 20;   
static const uint32_t LOOP_DT_MS         = 5;
static const uint32_t INPUT_TIMEOUT_MS   = 60000; 
static const uint32_t REPAIR_HOLD_FOR_REPAIR_MS = 3000; 

// ===================== LÓGICA REVERSA / FRENO =====================
static const int REV_REQUEST_GATE = 60;      
static const uint32_t T_BRAKETAP_MS   = 250;  
static const uint32_t T_NEUTRALTAP_MS = 100;  

static const int THR_BRAKE_US   = 1000;       
static const int THR_REV_MIN_US = 1350;       

// ===================== Variables Globales =====================
ControllerPtr myCtl = nullptr;
bool armed = false;
int currentEscUs = ESC_NEUTRAL_US;
uint32_t lastInputTime = 0;

enum RevState { RS_IDLE, RS_BRAKE, RS_NEUTRAL, RS_REV };
static RevState revState = RS_IDLE;
static uint32_t revTimer = 0;

int globalThr = 0, globalSteer = 0;
int globalSpeedPct = 100, globalLimitPct = 100, globalTrimUs = 0, globalSteerUs = 1500;
int globalHz = 0; 
String logMessage = "Sistema iniciado...";

void addLog(String msg) {
  logMessage = msg;
  #if USE_SERIAL_DEBUG
    Serial.println("\n[LOG] " + msg);
  #endif
}

// ===================== WIFI, OTA Y WEBSOCKETS INICIALIZACIÓN =====================
#if USE_WIFI_DEBUG
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81); 

const char webpage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>RC Telemetría WS</title>
  <style>
    body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background: #121212; color: #fff; padding: 15px; max-width: 500px; margin: auto; }
    .card { background: #1e1e1e; padding: 15px; border-radius: 8px; margin-bottom: 15px; border: 1px solid #333; }
    h2 { margin-top: 0; color: #4CAF50; text-align: center; }
    .bar-bg { background: #333; width: 100%; height: 25px; border-radius: 4px; position: relative; overflow: hidden; margin-top: 5px; }
    .bar-fill { height: 100%; transition: width 0.05s linear, background 0.1s; position: absolute; left: 50%; }
    .badge { padding: 5px 10px; border-radius: 4px; font-weight: bold; }
    .on { background: #4CAF50; color: #000; } .off { background: #f44336; color: #fff; }
    .ws-status { float: right; font-size: 0.8em; padding: 3px 6px; border-radius: 3px; }
    .ws-on { background: #009688; color: #fff; } .ws-off { background: #E91E63; color: #fff; }
    .log-box { background: #000; color: #0f0; font-family: monospace; padding: 10px; height: 120px; overflow-y: scroll; font-size: 13px; border-radius: 4px; border: 1px solid #444; }
    .grid { display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 10px; text-align: center; }
    .val-box { background: #2a2a2a; padding: 5px; border-radius: 4px; font-size: 1.1em; color: #03A9F4; }
  </style>
</head>
<body>
  <h2>⚙️ ESP32 RC Debugger <span id="wsBadge" class="ws-status ws-off">WS Disconnected</span></h2>
  
  <div class="card" style="text-align: center;">
    <span id="armedBadge" class="badge off">DISARMED</span> 
    <span style="margin: 0 10px;">|</span>
    Estado: <span id="revState" style="color:#ffeb3b; font-weight:bold; letter-spacing: 1px;">IDLE</span>
    <span style="margin: 0 10px;">|</span>
    Loop: <span id="hzVal" style="color:#03A9F4; font-weight:bold;">0</span> Hz
  </div>

  <div class="card">
    <p style="margin: 0 0 5px 0;">Gatillo (Acel / Freno): <b id="thrVal" style="float:right;">0</b></p>
    <div class="bar-bg"><div id="thrBar" class="bar-fill" style="background:#2196F3; width:0%;"></div></div>
    
    <p style="margin: 15px 0 5px 0;">Volante (Dirección): <b id="steerVal" style="float:right;">0</b></p>
    <div class="bar-bg"><div id="steerBar" class="bar-fill" style="background:#FF9800; width:0%;"></div></div>
  </div>

  <div class="card grid">
    <div><b>Pot 1 (Veloc)</b><div class="val-box" id="pot1">100%</div></div>
    <div><b>Pot 2 (Giro)</b><div class="val-box" id="pot2">100%</div></div>
    <div><b>Pot 3 (Trim)</b><div class="val-box" id="pot3">0us</div></div>
  </div>

  <div class="card grid" style="grid-template-columns: 1fr 1fr;">
    <div><b>ESC (Motor)</b><br><span id="escUs" style="font-size: 1.2em; color:#fff;">1500</span> us</div>
    <div><b>SERVO (Dir)</b><br><span id="steerUs" style="font-size: 1.2em; color:#fff;">1500</span> us</div>
  </div>

  <div class="card">
    <div style="margin-bottom: 5px;"><b>System Log</b></div>
    <div id="logBox" class="log-box"></div>
  </div>

  <script>
    const logBox = document.getElementById('logBox');
    let lastLog = "";

    function updateBar(elementId, value, min, max, center) {
      let percent = 0;
      let elem = document.getElementById(elementId);
      if(value >= center) {
        percent = ((value - center) / (max - center)) * 50;
        elem.style.left = '50%'; elem.style.width = percent + '%'; elem.style.transform = 'none';
      } else {
        percent = ((center - value) / (center - min)) * 50;
        elem.style.left = '50%'; elem.style.width = percent + '%'; elem.style.transform = 'translateX(-100%)';
      }
    }

    var ws;
    function initWebSocket() {
      ws = new WebSocket('ws://' + window.location.hostname + ':81/');
      
      ws.onopen = function() {
        document.getElementById('wsBadge').className = 'ws-status ws-on';
        document.getElementById('wsBadge').innerText = 'WS Connected';
      };
      
      ws.onclose = function() {
        document.getElementById('wsBadge').className = 'ws-status ws-off';
        document.getElementById('wsBadge').innerText = 'WS Disconnected';
        setTimeout(initWebSocket, 2000); 
      };
      
      ws.onmessage = function(event) {
        let data = JSON.parse(event.data);
        
        document.getElementById('armedBadge').className = data.armed ? 'badge on' : 'badge off';
        document.getElementById('armedBadge').innerText = data.armed ? 'ARMED' : 'DISARMED';
        
        const states = ["FORWARD", "BRAKING", "NEUTRAL", "REVERSE"];
        document.getElementById('revState').innerText = states[data.revState];
        document.getElementById('hzVal').innerText = data.hz;

        document.getElementById('thrVal').innerText = data.thr;
        document.getElementById('steerVal').innerText = data.steer;
        document.getElementById('escUs').innerText = data.escUs;
        document.getElementById('steerUs').innerText = data.steerUs;
        document.getElementById('pot1').innerText = data.speedPct + '%';
        document.getElementById('pot2').innerText = data.limitPct + '%';
        document.getElementById('pot3').innerText = (data.trimUs > 0 ? '+' : '') + data.trimUs + 'us';

        updateBar('thrBar', data.thr, -512, 512, 0);
        updateBar('steerBar', data.steer, -512, 512, 0);

        if(data.log !== lastLog) {
          logBox.innerHTML += `> ${data.log}<br>`;
          logBox.scrollTop = logBox.scrollHeight;
          lastLog = data.log;
        }
      };
    }
    window.onload = initWebSocket;
  </script>
</body>
</html>
)=====";

void handleRoot() { server.send(200, "text/html", webpage); }

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if(type == WStype_DISCONNECTED) {
  } else if(type == WStype_CONNECTED) {
  }
}
#endif

// ---------- NeoPixel status ----------
#if USE_NEOPIXELS
  #include <Adafruit_NeoPixel.h>
  Adafruit_NeoPixel pixels(NUM_PIXELS, PIN_NEOPIX, NEO_GRB + NEO_KHZ800);
  uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) { return pixels.Color(r, g, b); }
  void setAll(uint32_t c) { for (int i = 0; i < NUM_PIXELS; i++) pixels.setPixelColor(i, c); pixels.show(); }
  void statusDisarmed() { setAll(rgb(50, 0, 0)); } 
  void statusArmed()    { setAll(rgb(0, 50, 0)); } 
  void statusBrake()    { setAll(rgb(255, 0, 0)); } 
  void statusRev()      { setAll(rgb(0, 0, 100)); } 
  void statusFailsafe() { 
    static uint32_t lastToggle = 0;
    static bool on = false; 
    if (millis() - lastToggle > 250) { 
      lastToggle = millis();
      on = !on; 
      setAll(on ? rgb(255, 0, 0) : rgb(0, 0, 0)); 
    }
  }
#endif

// ---------- Helpers & Callbacks ----------
static inline int clampi(int v, int lo, int hi) { return (v < lo) ? lo : (v > hi) ? hi : v; }
uint32_t usToDuty(int us) { return (uint32_t)(((uint64_t)us * ((1u << PWM_RES) - 1u)) / (1000000 / PWM_FREQ)); }
void writeUs(int channel, int us) { ledcWrite(channel, usToDuty(clampi(us, 1000, 2000))); }

int mapCentered(int v, int minUs, int centerUs, int maxUs) {
  v = clampi(v, -512, 512);
  if (v >= 0) return centerUs + (int)(((long)v * (maxUs - centerUs)) / 512L);
  else        return centerUs + (int)(((long)v * (centerUs - minUs)) / 512L);
}

int applyDeadzone(int v) { return (abs(v) < DEADZONE) ? 0 : v; }
int applyExpo(int v, float expo) {
  float x = (float)v / 512.0f;
  return (int)(((1.0f - expo) * x + expo * x * x * x) * 512.0f);
}
int rampToTarget(int current, int target, int stepUs) {
  if (target > current) return min(current + stepUs, target);
  if (target < current) return max(current - stepUs, target);
  return current;
}

bool repairBtn() { return digitalRead(PIN_REPAIR) == LOW; }
void doRePair() {
  addLog("Borrando llaves Bluetooth...");
  BP32.forgetBluetoothKeys();
  delay(500);
  ESP.restart();
}

void onConnectedController(ControllerPtr ctl) {
  if (!myCtl) {
    myCtl = ctl;
    armed = false;
    revState = RS_IDLE;
    lastInputTime = millis();
    addLog("Mando conectado. Pulsa ARM.");
    #if USE_NEOPIXELS
      statusDisarmed();
    #endif
  }
}
void onDisconnectedController(ControllerPtr ctl) {
  if (ctl == myCtl) { myCtl = nullptr; addLog("Mando desc. FAILSAFE."); }
}

// ---------- Setup ----------
void setup() {
  #if USE_SERIAL_DEBUG
    Serial.begin(115200);
  #endif
  
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
  addLog("Bluepad32 listo.");

#if USE_WIFI_DEBUG
  WiFi.softAP("RC_TELEMETRY", "12345678");
  server.on("/", handleRoot);
  server.begin();
  
  // Iniciar WebSocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  addLog("AP: RC_TELEMETRY | Pass: 12345678");
  
  ArduinoOTA.setPassword("12345678"); 
  ArduinoOTA.onStart([]() {
    addLog("Iniciando OTA...");
    writeUs(CH_ESC, ESC_NEUTRAL_US);
    writeUs(CH_STEER, STEER_CENTER_US);
    #if USE_NEOPIXELS
      setAll(rgb(255, 165, 0)); 
    #endif
  });
  ArduinoOTA.onEnd([]() { addLog("OTA finalizada!"); });
  ArduinoOTA.begin();
#endif
}

// ---------- Main Loop ----------
void loop() {
  uint32_t now = millis();
  
  static uint32_t loopCount = 0;
  static uint32_t lastHzTimer = 0;
  loopCount++;
  if (now - lastHzTimer >= 1000) {
    globalHz = loopCount;
    loopCount = 0;
    lastHzTimer = now;
  }

  BP32.update();

  #if USE_WIFI_DEBUG
    server.handleClient(); // Sirve el HTML inicial
    webSocket.loop();      // Mantiene vivo el túnel de datos
    ArduinoOTA.handle();   // OTA
  #endif

  // 1. Repair Button
  static uint32_t repStart = 0;
  if (repairBtn()) {
    if (repStart == 0) repStart = now;
    if (now - repStart > REPAIR_HOLD_FOR_REPAIR_MS) doRePair();
  } else repStart = 0;

  // 2. Failsafe
  if (!myCtl || !myCtl->isConnected()) {
    writeUs(CH_STEER, STEER_CENTER_US);
    writeUs(CH_ESC, ESC_NEUTRAL_US); 
    globalSteerUs = STEER_CENTER_US;
    #if USE_NEOPIXELS
      statusFailsafe(); 
    #endif
    
    #if USE_WIFI_DEBUG
      static uint32_t lastFailWs = 0;
      if (now - lastFailWs > 200) {
        lastFailWs = now;
        String json = "{\"armed\":false,\"revState\":0,\"thr\":0,\"steer\":0,\"speedPct\":0,\"limitPct\":0,\"trimUs\":0,\"escUs\":1500,\"steerUs\":1500,\"hz\":" + String(globalHz) + ",\"log\":\"" + logMessage + "\"}";
        webSocket.broadcastTXT(json);
      }
    #endif

    delay(LOOP_DT_MS);
    return; 
  }

  // 3. Inputs
  int rawY  = myCtl->axisY();
  int rawRX = myCtl->axisRX();
  
  if (abs(rawY) > DEADZONE || abs(rawRX) > DEADZONE || myCtl->buttons()) {
      lastInputTime = now;
  }

  if (armed && (now - lastInputTime > INPUT_TIMEOUT_MS)) {
      armed = false; 
      addLog("Desarmado por INACTIVIDAD");
  }

  int thr   = applyExpo(applyDeadzone(THROTTLE_INVERT * (-rawY)), EXPO_THR);
  int steer = applyExpo(applyDeadzone(STEER_INVERT * (rawRX)), EXPO_STEER);

  // 4. POTS
  globalSpeedPct = 100; globalLimitPct = 100; globalTrimUs = 0;
  thr = (thr * globalSpeedPct) / 100;
  steer = (steer * globalLimitPct) / 100;
  globalThr = thr; globalSteer = steer;

  // 5. Armado
  static bool lastArmBtn = false;
  bool armBtn = (myCtl->buttons() & ARM_BUTTON_MASK);
  if (armBtn && !lastArmBtn) {
    if (!armed && abs(thr) < 10) { 
        armed = true; revState = RS_IDLE; addLog("SISTEMA ARMADO"); 
    } else { 
        armed = false; addLog("SISTEMA DESARMADO"); 
    }
  }
  lastArmBtn = armBtn;

  if (!armed) {
    writeUs(CH_STEER, STEER_CENTER_US);
    writeUs(CH_ESC, ESC_NEUTRAL_US);
    globalSteerUs = STEER_CENTER_US;
    #if USE_NEOPIXELS
      statusDisarmed();
    #endif
  } else {
    // 6. Dirección
    globalSteerUs = mapCentered(steer, STEER_MIN_US, STEER_CENTER_US, STEER_MAX_US) + globalTrimUs;
    writeUs(CH_STEER, globalSteerUs);

    // 7. Acelerador / Freno
    bool wantReverse = (thr < -REV_REQUEST_GATE);
    bool skipRamp = false;
    int targetEscUs = ESC_NEUTRAL_US;

    switch (revState) {
      case RS_IDLE:
        if (wantReverse) { revState = RS_BRAKE; revTimer = now; addLog("FRENO A FONDO"); } 
        else {
          if (thr < 0) thr = 0; 
          targetEscUs = mapCentered(thr, ESC_MIN_US, ESC_NEUTRAL_US, ESC_MAX_US);
          #if USE_NEOPIXELS
            statusArmed();
          #endif
        }
        break;
      case RS_BRAKE:
        targetEscUs = THR_BRAKE_US; skipRamp = true; 
        #if USE_NEOPIXELS
          statusBrake();
        #endif
        if (!wantReverse) revState = RS_IDLE;
        else if (now - revTimer > T_BRAKETAP_MS) { revState = RS_NEUTRAL; revTimer = now; }
        break;
      case RS_NEUTRAL:
        targetEscUs = ESC_NEUTRAL_US; skipRamp = true; 
        #if USE_NEOPIXELS
          statusBrake(); 
        #endif
        if (!wantReverse) revState = RS_IDLE;
        else if (now - revTimer > T_NEUTRALTAP_MS) { revState = RS_REV; addLog("MARCHA ATRAS"); }
        break;
      case RS_REV:
        if (!wantReverse) { revState = RS_IDLE; targetEscUs = ESC_NEUTRAL_US; } 
        else {
          int revMap = mapCentered(thr, ESC_MIN_US, ESC_NEUTRAL_US, ESC_MAX_US);
          if (revMap > THR_REV_MIN_US) revMap = THR_REV_MIN_US; 
          targetEscUs = revMap;
          #if USE_NEOPIXELS
            statusRev();
          #endif
        }
        break;
    }

    if (skipRamp) currentEscUs = targetEscUs; 
    else currentEscUs = rampToTarget(currentEscUs, targetEscUs, ESC_SLEW_STEP_US); 
    
    writeUs(CH_ESC, currentEscUs);
  }

  // --- 8. ENVÍO DE DATOS POR WEBSOCKET (Cada 50ms = 20 FPS) ---
  #if USE_WIFI_DEBUG
    static uint32_t lastWsSend = 0;
    if (now - lastWsSend > 50) {
      lastWsSend = now;
      String json = "{";
      json += "\"armed\":" + String(armed ? "true" : "false") + ",";
      json += "\"revState\":" + String(revState) + ",";
      json += "\"thr\":" + String(globalThr) + ",";
      json += "\"steer\":" + String(globalSteer) + ",";
      json += "\"speedPct\":" + String(globalSpeedPct) + ",";
      json += "\"limitPct\":" + String(globalLimitPct) + ",";
      json += "\"trimUs\":" + String(globalTrimUs) + ",";
      json += "\"escUs\":" + String(currentEscUs) + ",";
      json += "\"steerUs\":" + String(globalSteerUs) + ",";
      json += "\"hz\":" + String(globalHz) + ",";
      json += "\"log\":\"" + logMessage + "\"";
      json += "}";
      webSocket.broadcastTXT(json); 
    }
  #endif

  // 9. Serial Debug
  #if USE_SERIAL_DEBUG
    static uint32_t lastSerialTrace = 0;
    if (now - lastSerialTrace > 100) { 
      lastSerialTrace = now;
      char buffer[120];
      sprintf(buffer, "HZ: %3d | THR: %4d | STR: %4d | ESC: %4dus | SRV: %4dus", 
              globalHz, globalThr, globalSteer, currentEscUs, globalSteerUs);
      Serial.println(buffer);
    }
  #endif

  delay(LOOP_DT_MS);
}