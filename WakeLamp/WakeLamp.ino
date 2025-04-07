#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiClient.h>
#include <WiFiGeneric.h>
#include <WiFiMulti.h>
#include <WiFiSTA.h>
#include <WiFiScan.h>
#include <WiFiServer.h>
#include <WiFiType.h>
#include <WiFiUdp.h>

#include <RBDdimmer.h>

// #include <Arduino_BuiltIn.h>

#include <RTClib.h>


/*
 * This ESP32 code is created by esp32io.com
 *
 * This ESP32 code is released in the public domain
 *
 * For more detail (instruction and wiring diagram), visit https://esp32io.com/tutorials/esp32-neopixel-led-strip
 */

#include <Adafruit_NeoPixel.h>

// #include "RBDdimmer.h"

//#define USE_SERIAL  SerialUSB //Serial for boards whith USB serial port
//#define USE_SERIAL  Serial
#define outputPin  27
#define zerocross  39 // for boards with CHANGEBLE input pins

#define PIN_NEO_PIXEL 4  // The ESP32 pin GPIO16 connected to NeoPixel
#define NUM_PIXELS 144     // The number of LEDs (pixels) on NeoPixel LED strip

#define HEADER_BUFFER_SIZE 512
#define LINE_BUFFER_SIZE 128
#define STATUS_BUFFER_SIZE 64

Adafruit_NeoPixel NeoPixel(NUM_PIXELS, PIN_NEO_PIXEL, NEO_GRB + NEO_KHZ800);

RTC_PCF8523 rtc;

dimmerLamp dimmer(outputPin, zerocross); //initialase port for dimmer for MEGA, Leonardo, UNO, Arduino M0, Arduino Zero

const char* ssid = "DayCycleLamp";
const char* password = "123456789";

WiFiServer server(80);

int mainLight = 0;

const int riseDur = 120; //how long you want full brightness to take in minutes
int riseHour = 6;
int riseMin = 30;
int setHour = 18;
int setMin = 30;

bool brighten = false;
bool darken = false;
uint16_t r = 0;
uint16_t g = 0;
uint16_t b = 0;
uint32_t master = 0;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

char header[HEADER_BUFFER_SIZE] = {0};
char currentLine[LINE_BUFFER_SIZE] = {0};
char statusMessage[STATUS_BUFFER_SIZE] = "Ready";
char currentFunction[16] = "none";

// Indices for buffers
int headerIndex = 0;
int currentLineIndex = 0;

void setup() {
  Serial.begin(57600);
  dimmer.begin(NORMAL_MODE, ON);

  //NeoPixel.setBrightness(255);
  

  #ifndef ESP8266
    while (!Serial); // wait for serial port to connect. Needed for native USB
  #endif

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (! rtc.initialized() || rtc.lostPower()) {
    Serial.println("RTC is NOT initialized, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  rtc.start();

   // The PCF8523 can be calibrated for:
  //        - Aging adjustment
  //        - Temperature compensation
  //        - Accuracy tuning
  // The offset mode to use, once every two hours or once every minute.
  // The offset Offset value from -64 to +63. See the Application Note for calculation of offset values.
  // https://www.nxp.com/docs/en/application-note/AN11247.pdf
  // The deviation in parts per million can be calculated over a period of observation. Both the drift (which can be negative)
  // and the observation period must be in seconds. For accuracy the variation should be observed over about 1 week.
  // Note: any previous calibration should cancelled prior to any new observation period.
  // Example - RTC gaining 43 seconds in 1 week
  float drift = 43; // seconds plus or minus over oservation period - set to 0 to cancel previous calibration.
  float period_sec = (7 * 86400);  // total obsevation period in seconds (86400 = seconds in 1 day:  7 days = (7 * 86400) seconds )
  float deviation_ppm = (drift / period_sec * 1000000); //  deviation in parts per million (μs)
  float drift_unit = 4.34; // use with offset mode PCF8523_TwoHours
  // float drift_unit = 4.069; //For corrections every min the drift_unit is 4.069 ppm (use with offset mode PCF8523_OneMinute)
  int offset = round(deviation_ppm / drift_unit);
  // rtc.calibrate(PCF8523_TwoHours, offset); // Un-comment to perform calibration once drift (seconds) and observation period (seconds) are correct
  // rtc.calibrate(PCF8523_TwoHours, 0); // Un-comment to cancel previous calibration

  Serial.print("Offset is "); Serial.println(offset); // Print to control offset
  NeoPixel.begin();  // initialize NeoPixel strip object (REQUIRED)
  NeoPixel.show();
  //WiFi.config(IPAddress(10, 0, 0, 1));
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();

  server.begin();

  while(riseMin >= 60){
    riseMin = riseMin - 60;
    riseHour = riseHour + 1;
  }

  while(setMin >= 60){
    setMin = setMin - 60;
    setHour = setHour + 1;
  }
}

void loop() {
  static DateTime nextStep = rtc.now();

  int totRiseStart = (riseHour * 60) + riseMin;
  int totRiseDone = totRiseStart + riseDur;
  int totSetStart = (setHour * 60) + setMin;
  int totSetDone = totSetStart + riseDur;
  int totCurTime = 0;

  uint32_t stepMsecs = (riseDur * 60 * 1000) / (255*3);
  uint32_t stepSecs = 0;

  int dispRiseHour = riseHour;
  int dispSetHour = setHour;

  // if (dispRiseHour > 12){
  //   dispRiseHour = dispRiseHour - 12;
  // }
  // if(dispSetHour > 12){
  //   dispSetHour = dispSetHour - 12;
  // }

  if (stepMsecs >= 1000){
    stepSecs = stepMsecs / 1000;
    stepMsecs = stepMsecs % 1000;
  }
  // brighten();
  // darken();
  DateTime now = rtc.now();

  // analogWrite(2, 125);

  totCurTime = (now.hour()*60)+now.minute();

  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" (");
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  Serial.print(") ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();

  // Serial.print(" since midnight 1/1/1970 = ");
  // Serial.print(now.unixtime());
  // Serial.print("s = ");
  // Serial.print(now.unixtime() / 86400L);
  // Serial.println("d");

  Serial.print("Current Time: ");
  Serial.print(totCurTime);
  Serial.print(", Rise Time: ");
  Serial.print(totRiseStart);
  Serial.println();
  Serial.print("Rise Finish By: ");
  Serial.println(totRiseDone);

  Serial.print("Brighten: ");
  Serial.println(brighten);


  Serial.print("Set Time: ");
  Serial.print(totSetStart);
  Serial.println();
  Serial.print("Set Finish By: ");
  Serial.println(totSetDone);
  Serial.print("darken: ");
  Serial.println(darken);

  Serial.print("r = ");
  Serial.println(r);

  Serial.print("g = ");
  Serial.println(g);

  Serial.print("g = ");
  Serial.println(g);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  if (totCurTime >= totRiseStart && totCurTime < totRiseDone){
    //brighten();
    brighten = true;
    Serial.println("Setting Brighten");
  }
  else if ((totCurTime >= totRiseDone) && (totCurTime < totSetStart)){
    brighten = false;
    Serial.println("Clearing Brighten");
    r = 255;//set brightness to full
    g = 255;
    b = 255;
    master = 255*3;
  }
  else if ((totCurTime >= totSetStart) && (totCurTime < totSetDone)){
    darken = true;
    Serial.println("Setting Darken");
  }
  else if ((totCurTime >= totSetDone) || (totCurTime < totRiseStart)){
    darken == false;
    r = 0;//set to black
    g = 0;
    b = 0;
    master = 0;
    Serial.println("Clearing Darken");
  }


  if (now >= nextStep){
    if (darken == true){
      // if (r > 0)
      // {
      //   r = r - 1;
      //   if (b > 0)
      //   {
      //     g = g - 2;
      //     b = b - 2;
      //   }
      //   master = master - 1;
      // }
      if (b > 0){
        b = b - 1;
        if (g > 64){
          g = g - 1;
        }
      } else if (g > 0){
        g = g - 1;
      } else if (r > 0){
        r = r - 1;
      }
      if (master > 0){
        master = master - 3;
      }
    }
    else if (brighten == true){
      if (r < 255)
      {
        r = r + 1;
      } else if (g < 64) {
        g = g + 1;
      } else if (b < 255){
        b = b + 1;
        if (b < 255){
          g = g + 1;
        }
      }
      if (master < (255*3)){
        master = master + 3;
      }
    }
    delay(stepMsecs);
    nextStep = rtc.now() + TimeSpan(0,0,0,stepSecs);
  }

  Serial.println();
  // // updateStrip(r, g, b);
  // darkenF();
  // delay(3000);
  // brightenF();
  // delay(3000);
  mainLight = master / 2.55;
  Serial.print("Main Light: ");
  Serial.println(mainLight);
  updateStrip(r, g, b);
  if ((mainLight >= 10) || (mainLight == 0)){
    dimmer.setPower(mainLight);
  }
  
  Serial.println(dimmer.getPower());

  WiFiClient client = server.available();
  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        currentLine += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 2) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // Send your "Hello World" HTML response
            client.println("<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println(F("<style>"));
            client.println(F("body { font-family: Arial, sans-serif; text-align: center; background-color: #f0f0f0; }"));
            client.println(F("h1 { color: #333; }"));
            //rise buttons
            client.println(F(".button {"));
            client.println(F("  display: inline-block;"));
            client.println(F("  padding: 15px 30px;"));
            client.println(F("  margin: 10px;"));
            client.println(F("  font-size: 18px;"));
            client.println(F("  color: #fff;"));
            client.println(F("  background-color: #007BFF;"));
            client.println(F("  border: none;"));
            client.println(F("  border-radius: 5px;"));
            client.println(F("  cursor: pointer;"));
            client.println(F("}"));
            client.println(F(".button:hover { background-color: #007bff; }"));
            //Set Buttons
            client.println(F(".button2 {"));
            client.println(F("  display: inline-block;"));
            client.println(F("  padding: 15px 30px;"));
            client.println(F("  margin: 10px;"));
            client.println(F("  font-size: 18px;"));
            client.println(F("  color: #fff;"));
            client.println(F("  background-color: #eba434;"));
            client.println(F("  border: none;"));
            client.println(F("  border-radius: 5px;"));
            client.println(F("  cursor: pointer;"));
            client.println(F("}"));
            client.println(F(".button2:hover { background-color: #eba434; }"));
            //duration buttons Up
            client.println(F(".button3 {"));
            client.println(F("  display: inline-block;"));
            client.println(F("  padding: 15px 30px;"));
            client.println(F("  margin: 10px;"));
            client.println(F("  font-size: 18px;"));
            client.println(F("  color: #fff;"));
            client.println(F("  background-color: #24e037;"));
            client.println(F("  border: none;"));
            client.println(F("  border-radius: 5px;"));
            client.println(F("  cursor: pointer;"));
            client.println(F("}"));
            client.println(F(".button3:hover { background-color: #24e037; }"));
            //duration buttons Down
            client.println(F(".button4 {"));
            client.println(F("  display: inline-block;"));
            client.println(F("  padding: 15px 30px;"));
            client.println(F("  margin: 10px;"));
            client.println(F("  font-size: 18px;"));
            client.println(F("  color: #fff;"));
            client.println(F("  background-color: #1a471f;"));
            client.println(F("  border: none;"));
            client.println(F("  border-radius: 5px;"));
            client.println(F("  cursor: pointer;"));
            client.println(F("}"));
            client.println(F(".button4:hover { background-color: #1a471f; }"));
            //current time buttons
            client.println(F(".button5 {"));
            client.println(F("  display: inline-block;"));
            client.println(F("  padding: 15px 30px;"));
            client.println(F("  margin: 10px;"));
            client.println(F("  font-size: 18px;"));
            client.println(F("  color: #fff;"));
            client.println(F("  background-color: #e024ce;"));
            client.println(F("  border: none;"));
            client.println(F("  border-radius: 5px;"));
            client.println(F("  cursor: pointer;"));
            client.println(F("}"));
            client.println(F(".button5:hover { background-color: #e024ce; }"));
            client.println(F("</style>"));
            client.println(F("</head>"));
            //client.println("<body><h1>Hello World</h1></body></html>");
            client.print("<body><h1>Current Time: ");
            client.print(now.hour());
            client.print(":");
            client.print(now.minute());
            client.print(":");
            client.print(now.second());
            client.println("</h1></body></html>");
            client.print("<body><h1>Sunrise Time: ");
            client.print(dispRiseHour);
            client.print(":");
            client.print(riseMin);
            client.println("</h1></body></html>");   
            client.print("<body><h1>Sunset Time: ");         
            client.print(dispSetHour);
            client.print(":");
            client.print(setMin);
            client.println("</h1></body></html>");  
            client.print("<body><h1>Rise/Set Duration: ");
            client.print(riseDur);
            client.println("</h1></body></html>");
            client.print("<body><h1>Current Status: R-");
            client.print(r);
            client.print(" G-");
            client.print(g);
            client.print(" B-");
            client.print(b);
            client.print(" Main Light-");
            client.print(mainLight);
            client.println("</h1></body></html>");
            client.println(F("<button class=\"button\" onclick=\"location.href='/rhp1'\">Rise Hour +1</button>"));
            client.println(F("<button class=\"button\" onclick=\"location.href='/rhm1'\">Rise Hour -1</button>"));
            client.println(F("<button class=\"button\" onclick=\"location.href='/circleBounce'\">Rise Minute +1</button>"));
            client.println(F("<button class=\"button\" onclick=\"location.href='/cubeUpdate'\">Rise Minute +10</button>"));
            client.println(F("<button class=\"button2\" onclick=\"location.href='/discBounce'\">Set Hour +1</button>"));
            client.println(F("<button class=\"button2\" onclick=\"location.href='/discBounce'\">Set Hour -1</button>"));
            client.println(F("<button class=\"button2\" onclick=\"location.href='/circleBounce'\">Set Minute +1</button>"));
            client.println(F("<button class=\"button2\" onclick=\"location.href='/cubeUpdate'\">Set Minute +10</button>"));
            client.println(F("<button class=\"button3\" onclick=\"location.href='/discBounce'\">Rise/Set Time +1</button>"));
            client.println(F("<button class=\"button3\" onclick=\"location.href='/discBounce'\">Rise/Set Time +10</button>"));
            client.println(F("<button class=\"button3\" onclick=\"location.href='/discBounce'\">Rise/Set Time +30</button>"));
            client.println(F("<button class=\"button4\" onclick=\"location.href='/discBounce'\">Rise/Set Time -1</button>"));
            client.println(F("<button class=\"button4\" onclick=\"location.href='/discBounce'\">Rise/Set Time -10</button>"));
            client.println(F("<button class=\"button4\" onclick=\"location.href='/discBounce'\">Rise/Set Time -30</button>"));
            client.println(F("<button class=\"button5\" onclick=\"location.href='/discBounce'\">Current Hour +1</button>"));
            client.println(F("<button class=\"button5\" onclick=\"location.href='/discBounce'\">Current Hour -1</button>"));
            client.println(F("<button class=\"button5\" onclick=\"location.href='/circleBounce'\">Current Minute +1</button>"));
            client.println(F("<button class=\"button5\" onclick=\"location.href='/cubeUpdate'\">Current Minute +10</button>"));
            // client.println(F("<div id=\"terminal\"></div>"));
            // client.println(F("<script>"));
            // client.println(F("setInterval(function() {"));
            // client.println(F("  fetch('/status')"));
            // client.println(F("    .then(response => response.text())"));
            // client.println(F("    .then(data => {"));
            // client.println(F("      document.getElementById('terminal').innerText = data;"));
            // client.println(F("    });"));
            // client.println(F("}, 1000);"));
            // client.println(F("</script>"));
            // client.println(F("</body></html>"));
            // client.println();

            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        if (strstr(header, "GET /rhp1") != NULL) {
          riseHour = riseHour + 1;
          Serial.println("Adding Hour------------------------------------------------------------------------------------------------");
          Serial.println("Adding Hour------------------------------------------------------------------------------------------------");
          Serial.println("Adding Hour------------------------------------------------------------------------------------------------");
          Serial.println("Adding Hour------------------------------------------------------------------------------------------------");
          // strncpy(currentFunction, "discBounce", sizeof(currentFunction) - 1);
          // strncpy(statusMessage, "Running discBounce animation", STATUS_BUFFER_SIZE - 1);
        } else if (strstr(header, "GET /rhm1") != NULL) {
          riseHour = riseHour - 1;
          Serial.println("Subbing Hour-----------------------------------------------------------------------------------------------");
          Serial.println("Subbing Hour-----------------------------------------------------------------------------------------------");
          Serial.println("Subbing Hour-----------------------------------------------------------------------------------------------");
          Serial.println("Subbing Hour-----------------------------------------------------------------------------------------------");
          // strncpy(currentFunction, "circleBounce", sizeof(currentFunction) - 1);
          // strncpy(statusMessage, "Running circleBounce animation", STATUS_BUFFER_SIZE - 1);
        } else if (strstr(header, "GET /cubeUpdate") != NULL) {
          strncpy(currentFunction, "cubeUpdate", sizeof(currentFunction) - 1);
          strncpy(statusMessage, "Running cubeUpdate animation", STATUS_BUFFER_SIZE - 1);
        } else if (strstr(header, "GET /stop") != NULL) {
          strncpy(currentFunction, "stop", sizeof(currentFunction) - 1);
          strncpy(statusMessage, "Stopped animations", STATUS_BUFFER_SIZE - 1);
        } else if (strstr(header, "GET /rpsUpdate") != NULL) {
          strncpy(currentFunction, "rpsUpdate", sizeof(currentFunction) - 1);
          strncpy(statusMessage, "Running RPS Update Calculations", STATUS_BUFFER_SIZE - 1);
        }

      }
    }
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

void updateStrip(uint16_t r, uint16_t g, uint16_t b) {
  for (int i = 0; i < NeoPixel.numPixels(); i++)
  {
    NeoPixel.setPixelColor(i, r, g, b);
  }
  //NeoPixel.clear();
  NeoPixel.show();
}

// 0 to 255
void brightenF() {
  uint16_t i, j;

  for (j = 1; j <= 255; j++) {
    for (i = 0; i < NeoPixel.numPixels(); i++) {
      NeoPixel.setPixelColor(i, j, j, j);
    }
    NeoPixel.show();
    delay(50);
  }
  //delay(1500);
}

// 255 to 0
void darkenF() {
  //Serial.begin(9600);
  uint16_t i;
  uint8_t j;

  for (j = 255; j >= 1; j--) {
    for (i = 0; i < NeoPixel.numPixels(); i++) {
      NeoPixel.setPixelColor(i, j, j, j);
    }
    NeoPixel.show();
    delay(10);
    Serial.println(j);
  }
  delay(1500);
}