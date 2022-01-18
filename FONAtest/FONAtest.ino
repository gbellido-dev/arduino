#include <Arduino.h>
#include "Adafruit_FONA.h"
#include <SoftwareSerial.h>
#include <SPI.h>
#include <FS.h>
#include <IotWebConf.h>
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson


// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "Tele-door";

// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "smrtTHNG8266";

#define STRING_LEN 128
#define NUMBER_LEN 32

// -- Configuration specific key. The value should be modified if config structure was changed.
#define CONFIG_VERSION "2.1"

// -- When CONFIG_PIN is pulled to ground on startup, the Thing will use the initial
//      password to buld an AP. (E.g. in case of lost password)
//#define CONFIG_PIN D2

// -- Status indicator pin.
//      First it will light up (kept LOW), on Wifi connection it will blink,
//      when connected to the Wifi it will turn off (kept HIGH).
#define STATUS_PIN LED_BUILTIN

// -- Callback method declarations.
//void configSaved();
//boolean formValidator();

//DNSServer dnsServer;
//WebServer server(80);

char stringParamValue[STRING_LEN];
char intParamValue[NUMBER_LEN];
char floatParamValue[NUMBER_LEN];


#define FONA_RX D2
#define FONA_TX D3
#define FONA_RST D4


#define ADMIN_CALL_NUMBER "650895533"
#define SIM_CARD_PIN "2629"
#define TECLA_ABRIR_PUERTA '#'
#define PASSCODE "*12345#"
#define CALL_PHONE_NUMBER_A "650895533"
#define CALL_PHONE_NUMBER_B "650895533"
#define CALL_PHONE_NUMBER_C "650895533"


#define CONFIG_TRIGGER_PIN D7
#define PIN_ABRIR_PUERTA D8
#define PIN_LED_ABRIR_PUERTA D9

#define PIN_BUTTON_A D10
#define PIN_BUTTON_B D11
#define PIN_BUTTON_C D12 

// this is a large buffer for replies
char replybuffer[255];

SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;

// Use this for FONA 800 and 808s
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout);

uint8_t type;

unsigned long previousMillis = 0; 
const long interval = 5000;
unsigned char abrirPuerta = 0;

unsigned long previousMillisPasscode = 0;
const long intervalPasscode = 20000;
unsigned char passcodeIsReceiving = 0;
char passcodeReceived[8];
byte passcodeReceivedIndex = 0;
byte passcodeTimeoutDTMF[] = "111115";


byte longKeyPressCountMax = 40;    // 40 * 25 = 1000 ms
const unsigned long keySampleIntervalMs = 1000;
unsigned long keyPrevMillis_A = 0;
unsigned long keyPrevMillis_B = 0;
unsigned long keyPrevMillis_C = 0;
byte longKeyPressCount_A = 0;
byte longKeyPressCount_B = 0;
byte longKeyPressCount_C = 0;
byte prevKeyState_A = LOW;         // button is active low
byte prevKeyState_B = LOW;         // button is active low
byte prevKeyState_C = LOW;         // button is active low




void setup() {

  pinMode(PIN_BUTTON_A, INPUT_PULLUP);
  pinMode(PIN_BUTTON_B, INPUT_PULLUP);
  pinMode(PIN_BUTTON_C, INPUT_PULLUP);
  
  pinMode(PIN_ABRIR_PUERTA, OUTPUT);
  digitalWrite(PIN_ABRIR_PUERTA, LOW);
  
  pinMode(PIN_LED_ABRIR_PUERTA, OUTPUT);
  digitalWrite(PIN_LED_ABRIR_PUERTA, LOW);

  Serial.begin(115200);
  Serial.println(F("Tele-door system"));
  Serial.println(F("Initializing....(May take 3 seconds)"));

  fonaSerial->begin(4800);
  if (!fona.begin(*fonaSerial)) {
    Serial.println(F("Couldn't find FONA"));
    while (1);
  }
  
  delay(100);
  type = fona.type();
  Serial.println(F("FONA is OK"));
  Serial.print(F("Found "));
  switch (type) {
    case FONA800L:
      Serial.println(F("FONA 800L")); break;
    case FONA800H:
      Serial.println(F("FONA 800H")); break;
    case FONA808_V1:
      Serial.println(F("FONA 808 (v1)")); break;
    case FONA808_V2:
      Serial.println(F("FONA 808 (v2)")); break;
    case FONA3G_A:
      Serial.println(F("FONA 3G (American)")); break;
    case FONA3G_E:
      Serial.println(F("FONA 3G (European)")); break;
    default: 
      Serial.print(F("???:")); Serial.println(type); break;
  }

  
  // Print module IMEI number.
  char imei[16] = {0}; // MUST use a 16 character buffer for IMEI!
  delay(100);
  uint8_t imeiLen = fona.getIMEI(imei);
  if (imeiLen > 0) {
    Serial.print("Module IMEI: "); Serial.println(imei);
  }
  delay(100);
  if(fona.unlockSIM(SIM_CARD_PIN)){
    delay(3000);
    fona.setDTMF(1); 
  }
delay(1);
    // Set Headphone output
  if (! fona.setAudio(FONA_HEADSETAUDIO)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }
  fona.setMicVolume(FONA_HEADSETAUDIO, 15);
  
  // is configuration portal requested?
  //if ( digitalRead(CONFIG_TRIGGER_PIN) == LOW ) {
    //startConfigPortal();
  //}

  //printMenu();
}

 /* IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword, CONFIG_VERSION);
  IotWebConfParameter stringParam = IotWebConfParameter("String param", "stringParam", stringParamValue, STRING_LEN);
  IotWebConfSeparator separator1 = IotWebConfSeparator();
  IotWebConfParameter intParam = IotWebConfParameter("Int param", "intParam", intParamValue, NUMBER_LEN, "number", "1..100", NULL, "min='1' max='100' step='1'");
  // -- We can add a legend to the separator
  IotWebConfSeparator separator2 = IotWebConfSeparator("Calibration factor");
  IotWebConfParameter floatParam = IotWebConfParameter("Float param", "floatParam", floatParamValue, NUMBER_LEN, "number", "e.g. 23.4", NULL, "step='0.1'");
*/
/*void startConfigPortal() {

  iotWebConf.setStatusPin(STATUS_PIN);
  //iotWebConf.setConfigPin(CONFIG_PIN);
  iotWebConf.addParameter(&stringParam);
  iotWebConf.addParameter(&separator1);
  iotWebConf.addParameter(&intParam);
  iotWebConf.addParameter(&separator2);
  iotWebConf.addParameter(&floatParam);
  iotWebConf.setConfigSavedCallback(&configSaved);
  iotWebConf.setFormValidator(&formValidator);
  iotWebConf.getApTimeoutParameter()->visible = true;

  // -- Initializing the configuration.
  iotWebConf.init();

  // -- Set up required URL handlers on the web server.
  server.on("/", handleRoot);
  server.on("/config", []{ iotWebConf.handleConfig(); });
  server.onNotFound([](){ iotWebConf.handleNotFound(); });

  Serial.println("Ready.");
}*/


void printMenu(void) {
  Serial.println(F("-------------------------------------"));
  Serial.println(F("[?] Print this menu"));
  Serial.println(F("[a] read the ADC 2.8V max (FONA800 & 808)"));
  Serial.println(F("[b] read the Battery V and % charged"));
  Serial.println(F("[C] read the SIM CCID"));
  Serial.println(F("[U] Unlock SIM with PIN code"));
  Serial.println(F("[i] read RSSI"));
  Serial.println(F("[n] get Network status"));
  Serial.println(F("[v] set audio Volume"));
  Serial.println(F("[V] get Volume"));
  Serial.println(F("[H] set Headphone audio (FONA800 & 808)"));
  Serial.println(F("[e] set External audio (FONA800 & 808)"));
  Serial.println(F("[T] play audio Tone"));
  Serial.println(F("[P] PWM/Buzzer out (FONA800 & 808)"));

  // FM (SIM800 only!)
  Serial.println(F("[f] tune FM radio (FONA800)"));
  Serial.println(F("[F] turn off FM (FONA800)"));
  Serial.println(F("[m] set FM volume (FONA800)"));
  Serial.println(F("[M] get FM volume (FONA800)"));
  Serial.println(F("[q] get FM station signal level (FONA800)"));

  // Phone
  Serial.println(F("[c] make phone Call"));
  Serial.println(F("[A] get call status"));
  Serial.println(F("[h] Hang up phone"));
  Serial.println(F("[p] Pick up phone"));

  //DTMF
  Serial.println(F("[D] set DTMF"));

  // SMS
  Serial.println(F("[N] Number of SMSs"));
  Serial.println(F("[r] Read SMS #"));
  Serial.println(F("[R] Read All SMS"));
  Serial.println(F("[d] Delete SMS #"));
  Serial.println(F("[s] Send SMS"));
  Serial.println(F("[u] Send USSD"));
  
  // Time
  Serial.println(F("[y] Enable network time sync (FONA 800 & 808)"));
  Serial.println(F("[Y] Enable NTP time sync (GPRS FONA 800 & 808)"));
  Serial.println(F("[t] Get network time"));

  // GPRS
  Serial.println(F("[G] Enable GPRS"));
  Serial.println(F("[g] Disable GPRS"));
  Serial.println(F("[l] Query GSMLOC (GPRS)"));
  Serial.println(F("[w] Read webpage (GPRS)"));
  Serial.println(F("[W] Post to website (GPRS)"));

  // GPS
  if ((type == FONA3G_A) || (type == FONA3G_E) || (type == FONA808_V1) || (type == FONA808_V2)) {
    Serial.println(F("[O] Turn GPS on (FONA 808 & 3G)"));
    Serial.println(F("[o] Turn GPS off (FONA 808 & 3G)"));
    Serial.println(F("[L] Query GPS location (FONA 808 & 3G)"));
    if (type == FONA808_V1) {
      Serial.println(F("[x] GPS fix status (FONA808 v1 only)"));
    }
    Serial.println(F("[E] Raw NMEA out (FONA808)"));
  }
  
  Serial.println(F("[S] create Serial passthru tunnel"));
  Serial.println(F("-------------------------------------"));
  Serial.println(F(""));

}

// called when key goes from pressed to not pressed
void keyRelease(char keyCode) {
    Serial.println("key release");
   
    if (keyCode == 'A' && longKeyPressCount_A >= longKeyPressCountMax) {
        fona.callPhone(CALL_PHONE_NUMBER_A);
    }else if (keyCode == 'B' && longKeyPressCount_B >= longKeyPressCountMax) {
        fona.callPhone(CALL_PHONE_NUMBER_B);
    }else if (keyCode == 'C' && longKeyPressCount_C >= longKeyPressCountMax) {
        fona.callPhone(CALL_PHONE_NUMBER_C);
    }
}


/**
 * Handle web requests to "/" path.
 */
/*void handleRoot()
{
  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }
  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  s += "<title>IotWebConf 03 Custom Parameters</title></head><body>Hello world!";
  s += "<ul>";
  s += "<li>String param value: ";
  s += stringParamValue;
  s += "<li>Int param value: ";
  s += atoi(intParamValue);
  s += "<li>Float param value: ";
  s += atof(floatParamValue);
  s += "</ul>";
  s += "Go to <a href='config'>configure page</a> to change values.";
  s += "</body></html>\n";

  server.send(200, "text/html", s);
}*/

/*void configSaved()
{
  Serial.println("Configuration was updated.");
}*/

/*boolean formValidator()
{
  Serial.println("Validating form.");
  boolean valid = true;

  int l = server.arg(stringParam.getId()).length();
  if (l < 3)
  {
    stringParam.errorMessage = "Please provide at least 3 characters for this test!";
    //valid = false;
  }

  return valid;
}*/


void loop() {
  
  char * p;
  char received[256], copy_of_received[256];

   // -- doLoop should be called as frequently as possible.
  //iotWebConf.doLoop();

  // key management section
  /*unsigned long currentMillis_A = millis();
  if (currentMillis_A - keyPrevMillis_A >= keySampleIntervalMs) {
      keyPrevMillis_A = currentMillis_A;
     
      byte currKeyState_A = digitalRead(PIN_BUTTON_A);
     
      if ((prevKeyState_A == LOW) && (currKeyState_A == HIGH)) {
          //keyPress();
      } else if ((prevKeyState_A == HIGH) && (currKeyState_A == LOW)) {
          keyRelease('A');
      } else if (currKeyState_A == HIGH) {
          longKeyPressCount_A++;
      }
     
      prevKeyState_A = currKeyState_A;
  }*/

  /*unsigned long currentMillis_B = millis();
  if (currentMillis_B - keyPrevMillis_B >= keySampleIntervalMs) {
      keyPrevMillis_B = currentMillis_B;
     
      byte currKeyState_B = digitalRead(PIN_BUTTON_B);

      if ((prevKeyState_B == LOW) && (currKeyState_B == HIGH)) {
          //keyPress();
      } else if ((prevKeyState_B == HIGH) && (currKeyState_B == LOW)) {
          keyRelease('B');
      } else if (currKeyState_B == HIGH) {
          longKeyPressCount_B++;
      }
      
      prevKeyState_B = currKeyState_B;
  }*/

  /*unsigned long currentMillis_C = millis();
  if (currentMillis_C - keyPrevMillis_C >= keySampleIntervalMs) {
      keyPrevMillis_C = currentMillis_C;

      byte currKeyState_C = digitalRead(PIN_BUTTON_C);

      if ((prevKeyState_C == LOW) && (currKeyState_C == HIGH)) {
          //keyPress();
      } else if ((prevKeyState_C == HIGH) && (currKeyState_C == LOW)) {
          keyRelease('C');
      } else if (currKeyState_C == HIGH) {
          longKeyPressCount_C++;
      }

      prevKeyState_C = currKeyState_C;
  }*/
  unsigned long currentMillis_A = millis();
  if (currentMillis_A - keyPrevMillis_A >= keySampleIntervalMs) {
      keyPrevMillis_A = currentMillis_A;
      byte currKeyState_A = digitalRead(PIN_BUTTON_A);
      Serial.print("Key: ");
      Serial.println(currKeyState_A);
      if(currKeyState_A == 1){
        fona.callPhone(CALL_PHONE_NUMBER_A);
      }
      prevKeyState_A = currKeyState_A;
  }

  /*unsigned long currentMillisPasscode = millis();
  if ((passcodeIsReceiving == 1) && (currentMillisPasscode - previousMillisPasscode >= intervalPasscode)) {
    previousMillisPasscode = currentMillisPasscode;
    passcodeIsReceiving = 0;
    passcodeReceivedIndex = 0;
    strcpy(passcodeReceived, "");
    for(byte i = 0; i<6; i++){
      fona.setDTMF(passcodeTimeoutDTMF[i]);
      delay(10);
    }
  }*/
  delay(1);

  unsigned long currentMillis = millis();
  //Serial.print("Time: ");
  //Serial.println(currentMillis - previousMillis);
  if ((abrirPuerta == 1) && (currentMillis - previousMillis >= interval)) {
    previousMillis = currentMillis;
    digitalWrite(PIN_ABRIR_PUERTA, LOW);
    digitalWrite(PIN_LED_ABRIR_PUERTA, LOW);
    abrirPuerta = 0;
    Serial.println("Fin abriendo puerta!!");
  }


  if (abrirPuerta == 1){
    Serial.println("...puerta abierta!");
  }

  //Serial.print(F("FONA> "));
  if (! Serial.available() ) {
    int i = 0;
    while(fona.available()){
      delay(2);
      copy_of_received[i] = received[i] = fona.read();
      Serial.write(received[i]);
      i++;
    }

    if(i>0){
      copy_of_received[i] = received[i] = 0;
      i=0;

      //Serial.print("FONA1> ");
      //Serial.println(received);
      //Serial.println(copy_of_received);

      //+DTMF: #
      if(p = strstr (received, "+DTMF: ")){
        char botonPulsado = *(p+7);
        Serial.print("Boton pulsado: ");
        Serial.write(botonPulsado);
        Serial.println("");
        if(botonPulsado == TECLA_ABRIR_PUERTA){
          abrirPuerta = 1;
          digitalWrite(PIN_ABRIR_PUERTA, HIGH);
          digitalWrite(PIN_LED_ABRIR_PUERTA, HIGH);
          Serial.println("Abriendo puerta!!");
          previousMillis = millis();
        }
      }
      delay(1);
      /*if(p = strstr (received, "+DTMF: ")){
        char botonPulsado = *(p+7);
        Serial.print("Boton pulsado: ");
        Serial.write(botonPulsado);
        Serial.println("");
        if(botonPulsado == '*' && passcodeReceivedIndex == 0){
          passcodeIsReceiving = 1;
        }else{
          passcodeIsReceiving = 0;
          passcodeReceivedIndex = 0;
          strcpy(passcodeReceived, "");
          for(byte i = 0; i<6; i++){
            fona.setDTMF(passcodeTimeoutDTMF[i]);
            delay(10);
          }
        }

        if(passcodeIsReceiving == 1){
          passcodeReceived[passcodeReceivedIndex] = botonPulsado;
          passcodeReceivedIndex++;
          if(passcodeReceivedIndex == 6 && passcodeReceived[passcodeReceivedIndex-1] != '#'){
            passcodeIsReceiving = 0;
            passcodeReceivedIndex = 0;
            strcpy(passcodeReceived, "");
            for(byte i = 0; i<6; i++){
              fona.setDTMF(passcodeTimeoutDTMF[i]);
              delay(10);
            }
          }
        }
      }*/

      /*if(p = strstr (passcodeReceived, PASSCODE)){
        abrirPuerta = 1;
      }*/
      

      //+CLIP: "666333444",129,"",0,"",0
      if(p = strstr (received, "+CLIP:")){
        char incomingCallNumber[32];
        sscanf(p, "+CLIP: \"%9s[^\"]", incomingCallNumber);
        Serial.print("FONA2> ");
        Serial.println(incomingCallNumber);
        if(p = strstr (incomingCallNumber, ADMIN_CALL_NUMBER)){
          fona.pickUp();
        }
      }
    }
  }
    
  delay(1);
  if(Serial.available()){
    Serial.print(F("FONA> "));
    char command = Serial.read();
    Serial.println(command);
    
    switch (command) {
      case '?': {
          printMenu();
          break;
        }
  
      case 'a': {
          // read the ADC
          uint16_t adc;
          if (! fona.getADCVoltage(&adc)) {
            Serial.println(F("Failed to read ADC"));
          } else {
            Serial.print(F("ADC = ")); Serial.print(adc); Serial.println(F(" mV"));
          }
          break;
        }
  
      case 'b': {
          // read the battery voltage and percentage
          uint16_t vbat;
          if (! fona.getBattVoltage(&vbat)) {
            Serial.println(F("Failed to read Batt"));
          } else {
            Serial.print(F("VBat = ")); Serial.print(vbat); Serial.println(F(" mV"));
          }
  
  
          if (! fona.getBattPercent(&vbat)) {
            Serial.println(F("Failed to read Batt"));
          } else {
            Serial.print(F("VPct = ")); Serial.print(vbat); Serial.println(F("%"));
          }
  
          break;
        }
  
      case 'U': {
          // Unlock the SIM with a PIN code
          char PIN[5];
          flushSerial();
          Serial.println(F("Enter 4-digit PIN"));
          readline(PIN, 3, 0);
          Serial.println(PIN);
          Serial.print(F("Unlocking SIM card: "));
          if (! fona.unlockSIM(PIN)) {
            Serial.println(F("Failed"));
          } else {
            Serial.println(F("OK!"));
          }
          break;
        }
  
      case 'C': {
          // read the CCID
          fona.getSIMCCID(replybuffer);  // make sure replybuffer is at least 21 bytes!
          Serial.print(F("SIM CCID = ")); Serial.println(replybuffer);
          break;
        }
  
      case 'i': {
          // read the RSSI
          uint8_t n = fona.getRSSI();
          int8_t r;
  
          Serial.print(F("RSSI = ")); Serial.print(n); Serial.print(": ");
          if (n == 0) r = -115;
          if (n == 1) r = -111;
          if (n == 31) r = -52;
          if ((n >= 2) && (n <= 30)) {
            r = map(n, 2, 30, -110, -54);
          }
          Serial.print(r); Serial.println(F(" dBm"));
  
          break;
        }
  
      case 'n': {
          // read the network/cellular status
          uint8_t n = fona.getNetworkStatus();
          Serial.print(F("Network status "));
          Serial.print(n);
          Serial.print(F(": "));
          if (n == 0) Serial.println(F("Not registered"));
          if (n == 1) Serial.println(F("Registered (home)"));
          if (n == 2) Serial.println(F("Not registered (searching)"));
          if (n == 3) Serial.println(F("Denied"));
          if (n == 4) Serial.println(F("Unknown"));
          if (n == 5) Serial.println(F("Registered roaming"));
          break;
        }
  
      /*** Audio ***/
      case 'v': {
          // set volume
          flushSerial();
          if ( (type == FONA3G_A) || (type == FONA3G_E) ) {
            Serial.print(F("Set Vol [0-8] "));
          } else {
            Serial.print(F("Set Vol % [0-100] "));
          }
          uint8_t vol = readnumber();
          Serial.println();
          if (! fona.setVolume(vol)) {
            Serial.println(F("Failed"));
          } else {
            Serial.println(F("OK!"));
          }
          break;
        }
  
      case 'V': {
          uint8_t v = fona.getVolume();
          Serial.print(v);
          if ( (type == FONA3G_A) || (type == FONA3G_E) ) {
            Serial.println(" / 8");
          } else {
            Serial.println("%");
          }
          break;
        }
  
      case 'H': {
          // Set Headphone output
          if (! fona.setAudio(FONA_HEADSETAUDIO)) {
            Serial.println(F("Failed"));
          } else {
            Serial.println(F("OK!"));
          }
          fona.setMicVolume(FONA_HEADSETAUDIO, 15);
          break;
        }
      case 'e': {
          // Set External output
          if (! fona.setAudio(FONA_EXTAUDIO)) {
            Serial.println(F("Failed"));
          } else {
            Serial.println(F("OK!"));
          }
  
          fona.setMicVolume(FONA_EXTAUDIO, 10);
          break;
        }
  
      case 'T': {
          // play tone
          flushSerial();
          Serial.print(F("Play tone #"));
          uint8_t kittone = readnumber();
          Serial.println();
          // play for 1 second (1000 ms)
          if (! fona.playToolkitTone(kittone, 1000)) {
            Serial.println(F("Failed"));
          } else {
            Serial.println(F("OK!"));
          }
          break;
        }
  
      /*** FM Radio ***/
  
      case 'f': {
          // get freq
          flushSerial();
          Serial.print(F("FM Freq (eg 1011 == 101.1 MHz): "));
          uint16_t station = readnumber();
          Serial.println();
          // FM radio ON using headset
          if (fona.FMradio(true, FONA_HEADSETAUDIO)) {
            Serial.println(F("Opened"));
          }
          if (! fona.tuneFMradio(station)) {
            Serial.println(F("Failed"));
          } else {
            Serial.println(F("Tuned"));
          }
          break;
        }
      case 'F': {
          // FM radio off
          if (! fona.FMradio(false)) {
            Serial.println(F("Failed"));
          } else {
            Serial.println(F("OK!"));
          }
          break;
        }
      case 'm': {
          // Set FM volume.
          flushSerial();
          Serial.print(F("Set FM Vol [0-6]:"));
          uint8_t vol = readnumber();
          Serial.println();
          if (!fona.setFMVolume(vol)) {
            Serial.println(F("Failed"));
          } else {
            Serial.println(F("OK!"));
          }
          break;
        }

      case 'D': {
          // Set DTMF.
          flushSerial();
          Serial.print(F("Set DTMF [0-1]:"));
          uint8_t i = readnumber();
          Serial.println();
          if (!fona.setDTMF(i)) {
            Serial.println(F("Failed"));
          } else {
            Serial.println(F("OK!"));
          }
          break;
        }
      case 'M': {
          // Get FM volume.
          uint8_t fmvol = fona.getFMVolume();
          if (fmvol < 0) {
            Serial.println(F("Failed"));
          } else {
            Serial.print(F("FM volume: "));
            Serial.println(fmvol, DEC);
          }
          break;
        }
      case 'q': {
          // Get FM station signal level (in decibels).
          flushSerial();
          Serial.print(F("FM Freq (eg 1011 == 101.1 MHz): "));
          uint16_t station = readnumber();
          Serial.println();
          int8_t level = fona.getFMSignalLevel(station);
          if (level < 0) {
            Serial.println(F("Failed! Make sure FM radio is on (tuned to station)."));
          } else {
            Serial.print(F("Signal level (dB): "));
            Serial.println(level, DEC);
          }
          break;
        }
  
      /*** PWM ***/
  
      case 'P': {
          // PWM Buzzer output @ 2KHz max
          flushSerial();
          Serial.print(F("PWM Freq, 0 = Off, (1-2000): "));
          uint16_t freq = readnumber();
          Serial.println();
          if (! fona.setPWM(freq)) {
            Serial.println(F("Failed"));
          } else {
            Serial.println(F("OK!"));
          }
          break;
        }
  
      /*** Call ***/
      case 'c': {
          // call a phone!
          char number[30];
          flushSerial();
          Serial.print(F("Call #"));
          readline(number, 30, 0);
          Serial.println();
          Serial.print(F("Calling ")); Serial.println(number);
          if (!fona.callPhone(number)) {
            Serial.println(F("Failed"));
          } else {
            Serial.println(F("Sent!"));
          }
  
          break;
        }
      case 'A': {
          // get call status
          int8_t callstat = fona.getCallStatus();
          switch (callstat) {
            case 0: Serial.println(F("Ready")); break;
            case 1: Serial.println(F("Could not get status")); break;
            case 3: Serial.println(F("Ringing (incoming)")); break;
            case 4: Serial.println(F("Ringing/in progress (outgoing)")); break;
            default: Serial.println(F("Unknown")); break;
          }
          break;
        }
        
      case 'h': {
          // hang up!
          if (! fona.hangUp()) {
            Serial.println(F("Failed"));
          } else {
            Serial.println(F("OK!"));
          }
          break;
        }
  
      case 'p': {
          // pick up!
          if (! fona.pickUp()) {
            Serial.println(F("Failed"));
          } else {
            Serial.println(F("OK!"));
          }
          break;
        }
  
      /*** SMS ***/
  
      case 'N': {
          // read the number of SMS's!
          int8_t smsnum = fona.getNumSMS();
          if (smsnum < 0) {
            Serial.println(F("Could not read # SMS"));
          } else {
            Serial.print(smsnum);
            Serial.println(F(" SMS's on SIM card!"));
          }
          break;
        }
      case 'r': {
          // read an SMS
          flushSerial();
          Serial.print(F("Read #"));
          uint8_t smsn = readnumber();
          Serial.print(F("\n\rReading SMS #")); Serial.println(smsn);
  
          // Retrieve SMS sender address/phone number.
          if (! fona.getSMSSender(smsn, replybuffer, 250)) {
            Serial.println("Failed!");
            break;
          }
          Serial.print(F("FROM: ")); Serial.println(replybuffer);
  
          // Retrieve SMS value.
          uint16_t smslen;
          if (! fona.readSMS(smsn, replybuffer, 250, &smslen)) { // pass in buffer and max len!
            Serial.println("Failed!");
            break;
          }
          Serial.print(F("***** SMS #")); Serial.print(smsn);
          Serial.print(" ("); Serial.print(smslen); Serial.println(F(") bytes *****"));
          Serial.println(replybuffer);
          Serial.println(F("*****"));
  
          break;
        }
      case 'R': {
          // read all SMS
          int8_t smsnum = fona.getNumSMS();
          uint16_t smslen;
          int8_t smsn;
  
          if ( (type == FONA3G_A) || (type == FONA3G_E) ) {
            smsn = 0; // zero indexed
            smsnum--;
          } else {
            smsn = 1;  // 1 indexed
          }
  
          for ( ; smsn <= smsnum; smsn++) {
            Serial.print(F("\n\rReading SMS #")); Serial.println(smsn);
            if (!fona.readSMS(smsn, replybuffer, 250, &smslen)) {  // pass in buffer and max len!
              Serial.println(F("Failed!"));
              break;
            }
            // if the length is zero, its a special case where the index number is higher
            // so increase the max we'll look at!
            if (smslen == 0) {
              Serial.println(F("[empty slot]"));
              smsnum++;
              continue;
            }
  
            Serial.print(F("***** SMS #")); Serial.print(smsn);
            Serial.print(" ("); Serial.print(smslen); Serial.println(F(") bytes *****"));
            Serial.println(replybuffer);
            Serial.println(F("*****"));
          }
          break;
        }
  
      case 'd': {
          // delete an SMS
          flushSerial();
          Serial.print(F("Delete #"));
          uint8_t smsn = readnumber();
  
          Serial.print(F("\n\rDeleting SMS #")); Serial.println(smsn);
          if (fona.deleteSMS(smsn)) {
            Serial.println(F("OK!"));
          } else {
            Serial.println(F("Couldn't delete"));
          }
          break;
        }
  
      case 's': {
          // send an SMS!
          char sendto[21], message[141];
          flushSerial();
          Serial.print(F("Send to #"));
          readline(sendto, 20, 0);
          Serial.println(sendto);
          Serial.print(F("Type out one-line message (140 char): "));
          readline(message, 140, 0);
          Serial.println(message);
          if (!fona.sendSMS(sendto, message)) {
            Serial.println(F("Failed"));
          } else {
            Serial.println(F("Sent!"));
          }
  
          break;
        }
  
      case 'u': {
        // send a USSD!
        char message[141];
        flushSerial();
        Serial.print(F("Type out one-line message (140 char): "));
        readline(message, 140, 0);
        Serial.println(message);
  
        uint16_t ussdlen;
        if (!fona.sendUSSD(message, replybuffer, 250, &ussdlen)) { // pass in buffer and max len!
          Serial.println(F("Failed"));
        } else {
          Serial.println(F("Sent!"));
          Serial.print(F("***** USSD Reply"));
          Serial.print(" ("); Serial.print(ussdlen); Serial.println(F(") bytes *****"));
          Serial.println(replybuffer);
          Serial.println(F("*****"));
        }
      }
  
      /*** Time ***/
  
      case 'y': {
          // enable network time sync
          if (!fona.enableNetworkTimeSync(true))
            Serial.println(F("Failed to enable"));
          break;
        }
  
      case 'Y': {
          // enable NTP time sync
          if (!fona.enableNTPTimeSync(true, F("pool.ntp.org")))
            Serial.println(F("Failed to enable"));
          break;
        }
  
      case 't': {
          // read the time
          char buffer[23];
  
          fona.getTime(buffer, 23);  // make sure replybuffer is at least 23 bytes!
          Serial.print(F("Time = ")); Serial.println(buffer);
          break;
        }
  
  
      /*********************************** GPS (SIM808 only) */
  
      case 'o': {
          // turn GPS off
          if (!fona.enableGPS(false))
            Serial.println(F("Failed to turn off"));
          break;
        }
      case 'O': {
          // turn GPS on
          if (!fona.enableGPS(true))
            Serial.println(F("Failed to turn on"));
          break;
        }
      case 'x': {
          int8_t stat;
          // check GPS fix
          stat = fona.GPSstatus();
          if (stat < 0)
            Serial.println(F("Failed to query"));
          if (stat == 0) Serial.println(F("GPS off"));
          if (stat == 1) Serial.println(F("No fix"));
          if (stat == 2) Serial.println(F("2D fix"));
          if (stat == 3) Serial.println(F("3D fix"));
          break;
        }
  
      case 'L': {
          // check for GPS location
          char gpsdata[120];
          fona.getGPS(0, gpsdata, 120);
          if (type == FONA808_V1)
            Serial.println(F("Reply in format: mode,longitude,latitude,altitude,utctime(yyyymmddHHMMSS),ttff,satellites,speed,course"));
          else 
            Serial.println(F("Reply in format: mode,fixstatus,utctime(yyyymmddHHMMSS),latitude,longitude,altitude,speed,course,fixmode,reserved1,HDOP,PDOP,VDOP,reserved2,view_satellites,used_satellites,reserved3,C/N0max,HPA,VPA"));
          Serial.println(gpsdata);
  
          break;
        }
  
      case 'E': {
          flushSerial();
          if (type == FONA808_V1) {
            Serial.print(F("GPS NMEA output sentences (0 = off, 34 = RMC+GGA, 255 = all)"));
          } else {
            Serial.print(F("On (1) or Off (0)? "));
          }
          uint8_t nmeaout = readnumber();
  
          // turn on NMEA output
          fona.enableGPSNMEA(nmeaout);
  
          break;
        }
  
      /*********************************** GPRS */
  
      case 'g': {
          // turn GPRS off
          if (!fona.enableGPRS(false))
            Serial.println(F("Failed to turn off"));
          break;
        }
      case 'G': {
          // turn GPRS on
          if (!fona.enableGPRS(true))
            Serial.println(F("Failed to turn on"));
          break;
        }
      case 'l': {
          // check for GSMLOC (requires GPRS)
          uint16_t returncode;
  
          if (!fona.getGSMLoc(&returncode, replybuffer, 250))
            Serial.println(F("Failed!"));
          if (returncode == 0) {
            Serial.println(replybuffer);
          } else {
            Serial.print(F("Fail code #")); Serial.println(returncode);
          }
  
          break;
        }
      case 'w': {
          // read website URL
          uint16_t statuscode;
          int16_t length;
          char url[80];
  
          flushSerial();
          Serial.println(F("NOTE: in beta! Use small webpages to read!"));
          Serial.println(F("URL to read (e.g. wifitest.adafruit.com/testwifi/index.html):"));
          Serial.print(F("http://")); readline(url, 79, 0);
          Serial.println(url);
  
          Serial.println(F("****"));
          if (!fona.HTTP_GET_start(url, &statuscode, (uint16_t *)&length)) {
            Serial.println("Failed!");
            break;
          }
          while (length > 0) {
            while (fona.available()) {
              char c = fona.read();
  
              // Serial.write is too slow, we'll write directly to Serial register!
  #if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
              loop_until_bit_is_set(UCSR0A, UDRE0); /* Wait until data register empty. */
              UDR0 = c;
  #else
              Serial.write(c);
  #endif
              length--;
              if (! length) break;
            }
          }
          Serial.println(F("\n****"));
          fona.HTTP_GET_end();
          break;
        }
  
      case 'W': {
          // Post data to website
          uint16_t statuscode;
          int16_t length;
          char url[80];
          char data[80];
  
          flushSerial();
          Serial.println(F("NOTE: in beta! Use simple websites to post!"));
          Serial.println(F("URL to post (e.g. httpbin.org/post):"));
          Serial.print(F("http://")); readline(url, 79, 0);
          Serial.println(url);
          Serial.println(F("Data to post (e.g. \"foo\" or \"{\"simple\":\"json\"}\"):"));
          readline(data, 79, 0);
          Serial.println(data);
  
          Serial.println(F("****"));
          if (!fona.HTTP_POST_start(url, F("text/plain"), (uint8_t *) data, strlen(data), &statuscode, (uint16_t *)&length)) {
            Serial.println("Failed!");
            break;
          }
          while (length > 0) {
            while (fona.available()) {
              char c = fona.read();
  
  #if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
              loop_until_bit_is_set(UCSR0A, UDRE0); /* Wait until data register empty. */
              UDR0 = c;
  #else
              Serial.write(c);
  #endif
  
              length--;
              if (! length) break;
            }
          }
          Serial.println(F("\n****"));
          fona.HTTP_POST_end();
          break;
        }
      /*****************************************/
  
      case 'S': {
          Serial.println(F("Creating SERIAL TUBE"));
          while (1) {
            while (Serial.available()) {
              delay(1);
              fona.write(Serial.read());
            }
            if (fona.available()) {
              Serial.write(fona.read());
            }
          }
          break;
        }
  
      default: {
          Serial.println(F("Unknown command"));
          printMenu();
          break;
        }
    }
  }
  // flush input
  flushSerial();
  while (fona.available()) {
    Serial.write(fona.read());
  }

}

void flushSerial() {
  while (Serial.available())
    Serial.read();
}

char readBlocking() {
  while (!Serial.available());
  return Serial.read();
}
uint16_t readnumber() {
  uint16_t x = 0;
  char c;
  while (! isdigit(c = readBlocking())) {
    //Serial.print(c);
  }
  Serial.print(c);
  x = c - '0';
  while (isdigit(c = readBlocking())) {
    Serial.print(c);
    x *= 10;
    x += c - '0';
  }
  return x;
}

uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout) {
  uint16_t buffidx = 0;
  boolean timeoutvalid = true;
  if (timeout == 0) timeoutvalid = false;

  while (true) {
    if (buffidx > maxbuff) {
      //Serial.println(F("SPACE"));
      break;
    }

    while (Serial.available()) {
      char c =  Serial.read();

      //Serial.print(c, HEX); Serial.print("#"); Serial.println(c);

      if (c == '\r') continue;
      if (c == 0xA) {
        if (buffidx == 0)   // the first 0x0A is ignored
          continue;

        timeout = 0;         // the second 0x0A is the end of the line
        timeoutvalid = true;
        break;
      }
      buff[buffidx] = c;
      buffidx++;
    }

    if (timeoutvalid && timeout == 0) {
      //Serial.println(F("TIMEOUT"));
      break;
    }
    delay(1);
  }
  buff[buffidx] = 0;  // null term
  return buffidx;
}
