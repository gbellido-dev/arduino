/**The MIT License (MIT)
Copyright (c) 2015 by Daniel Eichhorn
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
See more at http://blog.squix.ch
*/

// Setup
const int UPDATE_WEATHER_INTERVAL_SECS = 10 * 60;  // Update every 10 minutes
const int UPDATE_SENSOR_DATA_INTERVAL_SECS = 30;  // Update every 30 seconds
const int AWAKE_TIME_SECS = 30;                   // how many seconds to stay 'awake' before going back to zzz
const int MQTT_RECONECT_INTERVAL_SECS = 5;
const int MQTT_SEND_DATA_INTERVAL_SECS = 30;


/*#define PIN_D0 16
#define PIN_D1  5
#define PIN_D2  4
#define PIN_D3  0
#define PIN_D4  2
#define PIN_D5 14
#define PIN_D6 12
#define PIN_D7 13
#define PIN_D8 15
#define PIN_D9  3
#define PIN_D10 1

#define PIN_MOSI 8
#define PIN_MISO 7
#define PIN_SCLK 6
#define PIN_HWCS 0

#define PIN_D11  9
#define PIN_D12 106*/

// Pins for the ILI9341
#define TFT_CS D2
#define TFT_DC D1
#define TFT_RST D4

// pins for the touchscreen
#define TS_CS D3
#define TS_IRQ D8

#define TS_MINY 240
#define TS_MINX 160
#define TS_MAXY 3820
#define TS_MAXX 3650

#define BOTON_A D5
#define BOTON_B D6
#define BOTON_C D7

#define BACKLIGHT_PIN D8

#define DHTPIN D0     // what digital pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

#define TERMOSTATE_MIN_TEMP 10
#define TERMOSTATE_MAX_TEMP 38

// TimeClient settings
const float UTC_OFFSET = 1;

// Wunderground Settings
//http://api.wunderground.com/api/05ca9875279e906e/conditions/q/ES/Arrasate_Mondragon.json
//http://api.wunderground.com/api/05ca9875279e906e/conditions/lang:SP/q/pws:ISSURIBE2.json
const boolean IS_METRIC = true;
const String WUNDERGRROUND_API_KEY = "05ca9875279e906e";
const String WUNDERGRROUND_LANGUAGE = "ES";
const String WUNDERGROUND_COUNTRY = "SP";
const String WUNDERGROUND_CITY = "Arrasate-Mondragon";

//Thingspeak Settings
const String THINGSPEAK_CHANNEL_ID = "67284";
const String THINGSPEAK_API_READ_KEY = "L2VIW20QVNZJBLAK";

// List, so that the downloader knows what to fetch
String wundergroundIcons [] = {"chanceflurries","chancerain","chancesleet","chancesnow","clear","cloudy","flurries","fog","hazy","mostlycloudy","mostlysunny","partlycloudy","partlysunny","rain","sleet","snow","sunny","tstorms","unknown"};

/***************************
 * End Settings
 **************************/

