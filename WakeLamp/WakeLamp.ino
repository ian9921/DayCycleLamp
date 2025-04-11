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

const char* ssid = "DayCycleLamp192.168.4.1"; // IP = 192.168.4.1
const char* password = "123456789";

WiFiServer server(80);

int mainLight = 0;

int riseDur = 120; //how long you want full brightness to take in minutes
int riseHour = 6;
int riseMin = 30;
int setHour = 18;
int setMin = 30;

bool brighten = false;
bool darken = false;
uint16_t r = 0;
uint16_t g = 0;
uint16_t b = 0;
uint16_t pr = 0;
uint16_t pg = 0;
uint16_t pb = 0;
uint32_t master = 0;

bool manualOff = false;

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


  float drift = 43; // seconds plus or minus over oservation period - set to 0 to cancel previous calibration.
  float period_sec = (7 * 86400);  // total obsevation period in seconds (86400 = seconds in 1 day:  7 days = (7 * 86400) seconds )
  float deviation_ppm = (drift / period_sec * 1000000); //  deviation in parts per million (μs)
  float drift_unit = 4.34; // use with offset mode PCF8523_TwoHours
  // float drift_unit = 4.069; //For corrections every min the drift_unit is 4.069 ppm (use with offset mode PCF8523_OneMinute)
  int offset = round(deviation_ppm / drift_unit);

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

bool home = true;

void updateStrip(uint16_t r, uint16_t g, uint16_t b) {
  for (int i = 0; i < NeoPixel.numPixels(); i++)
  {
    NeoPixel.setPixelColor(i, r, g, b);
  }
  //NeoPixel.clear();
  NeoPixel.show();
}

void loop() {
  static DateTime nextStep = rtc.now();

  int totRiseStart = (riseHour * 60) + riseMin;
  int totRiseDone = totRiseStart + riseDur;
  int totSetStart = (setHour * 60) + setMin;
  int totSetDone = totSetStart + riseDur;
  int totCurTime = 0;

  int tempNewMins;
  int tempNewHours;

  uint32_t stepMsecs = (riseDur * 60 * 1000) / (255*3);
  uint32_t stepSecs = 0;

  int dispRiseHour = riseHour;
  int dispSetHour = setHour;

  if (stepMsecs >= 1000){
    stepSecs = stepMsecs / 1000;
    stepMsecs = stepMsecs % 1000;
  }

  DateTime now = rtc.now();

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

  if ((totCurTime >= totRiseStart) && (totCurTime < totRiseDone) && (manualOff == false)){
    //brighten();
    darken = false;
    if (manualOff == false)
    {
      brighten = true;
      Serial.println("Setting Brighten");
    }
  }
  else if ((totCurTime >= totRiseDone) && (totCurTime < totSetStart) && (manualOff == false)){
    brighten = false;
    darken = false;
    Serial.println("Clearing Brighten");
    r = 255;//set brightness to full
    g = 255;
    b = 255;
    master = 255*3;
  }
  else if ((totCurTime >= totSetStart) && (totCurTime < totSetDone) && (manualOff == false)){
    darken = true;
    brighten = false;
    Serial.println("Setting Darken");
    //manualOff = false;
  }
  else if ((totCurTime >= totSetDone) || (totCurTime < totRiseStart) || (manualOff == true)){
    if ((totCurTime >= totSetDone) || (totCurTime < totRiseStart)){
      manualOff = false;
    }
    darken = false;
    brighten = false;
    r = 0;//set to black
    g = 0;
    b = 0;
    master = 0;
    Serial.println("Clearing Darken");
  }


  if (now >= nextStep){
    if (darken == true){
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
      master = r + g + b;
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
      master = r + g + b;
    }
    delay(stepMsecs);
    nextStep = rtc.now() + TimeSpan(0,0,0,stepSecs);
  }

  Serial.println();

  mainLight = master / 7.65;
  Serial.print("Main Light: ");
  Serial.println(mainLight);

  if ((r != pr) || (b != pb) || (g != pg)){ //only update strip if there's been an actual change
    updateStrip(r, g, b);
    pr = r;
    pb = b;
    pg = g;
  }

  if ((mainLight >= 10) || (mainLight == 0)){
    dimmer.setPower(mainLight);
  }
  
  Serial.println(dimmer.getPower());

  WiFiClient client = server.available();
  if (client) {
    Serial.println(F("New Client."));
    memset(header, 0, HEADER_BUFFER_SIZE);
    headerIndex = 0;
    memset(currentLine, 0, LINE_BUFFER_SIZE);
    currentLineIndex = 0;

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);

        // Store header with bounds checking
        if (headerIndex < HEADER_BUFFER_SIZE - 1) {
          header[headerIndex++] = c;
          header[headerIndex] = '\0';
        }

        // Handle end of line
        if (c == '\n') {
          if (currentLineIndex == 0) {
            // Check for specific GET requests
            if (strncmp(header, "GET /status", 11) == 0) {
              // Serve the status message
              client.println(F("HTTP/1.1 200 OK"));
              client.println(F("Content-Type: text/plain"));
              client.println(F("Connection: close"));
              client.println();
              client.print(statusMessage);
            } else {
              // Send HTTP response
              client.println(F("HTTP/1.1 200 OK"));
              client.println(F("Content-type:text/html"));
              client.println(F("Connection: close"));
              client.println();

              // Display the improved HTML web page using F() macro
              client.println(F("<!DOCTYPE html><html>"));
              client.println(F("<head>"));
              client.println(F("<title>Control Panel</title>"));
              client.println(F("<style>"));
              client.println(F("body { font-family: Arial, sans-serif; text-align: center; background-color: #f0f0f0; }"));
              client.println(F("h1 { color: #333; }"));
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
              //safe reload button
              client.println(F(".button6 {"));
              client.println(F("  display: inline-block;"));
              client.println(F("  padding: 15px 30px;"));
              client.println(F("  margin: 10px;"));
              client.println(F("  font-size: 18px;"));
              client.println(F("  color: #fff;"));
              client.println(F("  background-color: #27f5f5;"));
              client.println(F("  border: none;"));
              client.println(F("  border-radius: 5px;"));
              client.println(F("  cursor: pointer;"));
              client.println(F("}"));
              client.println(F(".button6:hover { background-color: #27f5f5; }"));
              //manual off button
              client.println(F(".button7 {"));
              client.println(F("  display: inline-block;"));
              client.println(F("  padding: 15px 30px;"));
              client.println(F("  margin: 10px;"));
              client.println(F("  font-size: 18px;"));
              client.println(F("  color: #fff;"));
              client.println(F("  background-color: #f52746;"));
              client.println(F("  border: none;"));
              client.println(F("  border-radius: 5px;"));
              client.println(F("  cursor: pointer;"));
              client.println(F("}"));
              client.println(F(".button7:hover { background-color: #f52746; }"));
              client.println(F("#terminal {"));
              client.println(F("  width: 80%;"));
              client.println(F("  height: 150px;"));
              client.println(F("  margin: 20px auto;"));
              client.println(F("  padding: 10px;"));
              client.println(F("  background-color: #000;"));
              client.println(F("  color: #0f0;"));
              client.println(F("  font-family: monospace;"));
              client.println(F("  overflow-y: scroll;"));
              client.println(F("  border-radius: 5px;"));
              client.println(F("}"));
              client.println(F("</style>"));
              client.println(F("</head>"));
              client.println(F("<body>"));
              client.print("<body><h1>Current Time: ");
              client.print(now.hour());
              client.print(":");
              client.print(now.minute());
              client.print(":");
              client.print(now.second());
              client.println("</h1></body></html>");
              client.print("<body><h1>Sunrise Time: ");
              client.print(riseHour);
              client.print(":");
              client.print(riseMin);
              client.println("</h1></body></html>");   
              client.print("<body><h1>Sunset Time: ");         
              client.print(setHour);
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
              client.print("<body><h1>Manually Disabled: ");
              client.print(manualOff);
              client.println("</h1></body></html>");
              client.println(F("<h1>Control Panel</h1>"));
              client.println(F("<button class=\"button6\" onclick=\"location.href='/'\">Safe Reload</button6>"));
              client.println(F("<button class=\"button7\" onclick=\"location.href='/manOff'\">Manual Disable</button6>"));
              client.println(F("<button class=\"button\" onclick=\"location.href='/rhp1'\">Rise Hour +1</button>"));
              client.println(F("<button class=\"button\" onclick=\"location.href='/rhm1'\">Rise Hour -1</button>"));
              client.println(F("<button class=\"button\" onclick=\"location.href='/rmp1'\">Rise Minute +1</button>"));
              client.println(F("<button class=\"button\" onclick=\"location.href='/rmp2'\">Rise Minute +10</button>"));
              client.println(F("<button class=\"button2\" onclick=\"location.href='/shp1'\">Set Hour +1</button>"));
              client.println(F("<button class=\"button2\" onclick=\"location.href='/shm1'\">Set Hour -1</button>"));
              client.println(F("<button class=\"button2\" onclick=\"location.href='/smp1'\">Set Minute +1</button>"));
              client.println(F("<button class=\"button2\" onclick=\"location.href='/smp2'\">Set Minute +10</button>"));
              client.println(F("<button class=\"button3\" onclick=\"location.href='/rstp1'\">Rise/Set Time +1</button>"));
              client.println(F("<button class=\"button3\" onclick=\"location.href='/rstp2'\">Rise/Set Time +10</button>"));
              client.println(F("<button class=\"button3\" onclick=\"location.href='/rstp3'\">Rise/Set Time +30</button>"));
              client.println(F("<button class=\"button4\" onclick=\"location.href='/rstm1'\">Rise/Set Time -1</button>"));
              client.println(F("<button class=\"button4\" onclick=\"location.href='/rstm2'\">Rise/Set Time -10</button>"));
              client.println(F("<button class=\"button4\" onclick=\"location.href='/rstm3'\">Rise/Set Time -30</button>"));
              client.println(F("<button class=\"button5\" onclick=\"location.href='/chp1'\">Current Hour +1</button>"));
              client.println(F("<button class=\"button5\" onclick=\"location.href='/chm1'\">Current Hour -1</button>"));
              client.println(F("<button class=\"button5\" onclick=\"location.href='/cmp1'\">Current Minute +1</button>"));
              client.println(F("<button class=\"button5\" onclick=\"location.href='/cmp2'\">Current Minute +10</button>"));

              client.println();
            }
            break;
          } else {
            // Clear current line
            memset(currentLine, 0, LINE_BUFFER_SIZE);
            currentLineIndex = 0;
          }
        } else if (c != '\r') {
          // Store current line with bounds checking
          if (currentLineIndex < LINE_BUFFER_SIZE - 1) {
            currentLine[currentLineIndex++] = c;
            currentLine[currentLineIndex] = '\0';
          }
        }

        // Check for GET requests in header
        if (strstr(header, "GET /rhp1") != NULL) {
          if (home == true){
            riseHour++;
            home = false;
            Serial.println("We Are Doing a thing--------------------------------------");
          }
        } else if (strstr(header, "GET /rhm1") != NULL) {
          if (home == true){
            riseHour--;
            home = false;
          }
        } else if (strstr(header, "GET /rmp1") != NULL) {
          if (home == true){
            riseMin = riseMin + 1;
            home = false;
            if (riseMin >= 60){
              riseMin = riseMin - 60;
            }
          }
        } else if (strstr(header, "GET /rmp2") != NULL) {
          if (home == true){
            riseMin = riseMin + 10;
            home = false;
            if (riseMin >= 60){
              riseMin = riseMin - 60;
            }
          }
        } else if (strstr(header, "GET /shp1") != NULL) {
          if (home == true){
            setHour++;
            home = false;
            Serial.println("We Are Doing a thing--------------------------------------");
          }
        } else if (strstr(header, "GET /shm1") != NULL) {
          if (home == true){
            setHour--;
            home = false;
          }
        } else if (strstr(header, "GET /smp1") != NULL) {
          if (home == true){
            setMin = setMin + 1;
            if (setMin >= 60){
              setMin = setMin - 60;
            }
            home = false;
          }
        } else if (strstr(header, "GET /smp2") != NULL) {
          if (home == true){
            setMin = setMin + 10;
            if (setMin >= 60){
              setMin = setMin - 60;
            }
            home = false;
          }
        }else if (strstr(header, "GET /rstp1") != NULL) {
          if (home == true){
            riseDur++;
            home = false;
          }
        } else if (strstr(header, "GET /rstp2") != NULL) {
          if (home == true){
            riseDur = riseDur + 10;
            home = false;
          }
        } else if (strstr(header, "GET /rstp3") != NULL) {
          if (home == true){
            riseDur = riseDur + 30;
            home = false;
          }
        } else if (strstr(header, "GET /rstm1") != NULL) {
          if (home == true){
            riseDur--;
            if (riseDur <= 1){
              riseDur = 1;
            }
            home = false;
          }
        } else if (strstr(header, "GET /rstm2") != NULL) {
          if (home == true){
            riseDur = riseDur - 10;
            if (riseDur <= 1){
              riseDur = 1;
            }
            home = false;
          }
        } else if (strstr(header, "GET /rstm3") != NULL) {
          if (home == true){
            riseDur = riseDur - 30;
            if (riseDur <= 1){
              riseDur = 1;
            }
            home = false;
          }
        } else if (strstr(header, "GET /chp1") != NULL) {
          if (home == true){
            tempNewHours = now.hour() + 1;
            if (tempNewHours > 23){
              tempNewHours = 0;
            }
            rtc.adjust(DateTime(now.year(), now.month(), now.day(), tempNewHours, now.minute(), now.second()));
            home = false;
            Serial.println("We Are Doing a thing--------------------------------------");
          }
        } else if (strstr(header, "GET /chm1") != NULL) {
          if (home == true){
            tempNewHours = now.hour() - 1;
            if (tempNewHours < 0){
              tempNewHours = 23;
            }
            rtc.adjust(DateTime(now.year(), now.month(), now.day(), tempNewHours, now.minute(), now.second()));
            home = false;
          }
        } else if (strstr(header, "GET /cmp1") != NULL) {
          if (home == true){
            tempNewMins = now.minute() + 1;
            home = false;
            if (tempNewMins >= 60){
              tempNewMins = tempNewMins - 60;
            }
            rtc.adjust(DateTime(now.year(), now.month(), now.day(), now.hour(), tempNewMins, now.second()));
          }
        } else if (strstr(header, "GET /cmp2") != NULL) {
          if (home == true){
            tempNewMins = now.minute() + 10;
            home = false;
            if (tempNewMins >= 60){
              tempNewMins = tempNewMins - 60;
            }
            rtc.adjust(DateTime(now.year(), now.month(), now.day(), now.hour(), tempNewMins, now.second()));
          }
        } else if (strstr(header, "GET /manOff") != NULL) {
          if (home == true){
            manualOff = !manualOff;
          }
        } else if (strstr(header, "GET /rpsUpdate") != NULL) {
          strncpy(currentFunction, "rpsUpdate", sizeof(currentFunction) - 1);
          strncpy(statusMessage, "Running RPS Update Calculations", STATUS_BUFFER_SIZE - 1);
        } else {
          home = true;
        }
      }
    }
    client.stop();
    Serial.println(F("Client disconnected."));
    Serial.println();
  }
}