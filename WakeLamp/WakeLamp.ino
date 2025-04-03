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

Adafruit_NeoPixel NeoPixel(NUM_PIXELS, PIN_NEO_PIXEL, NEO_GRB + NEO_KHZ800);

RTC_PCF8523 rtc;

dimmerLamp dimmer(outputPin, zerocross); //initialase port for dimmer for MEGA, Leonardo, UNO, Arduino M0, Arduino Zero



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
uint16_t master = 0;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

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
}

void loop() {
  static DateTime nextStep = rtc.now();

  int totRiseStart = (riseHour * 60) + riseMin;
  int totRiseDone = totRiseStart + riseDur;
  int totSetStart = (setHour * 60) + setMin;
  int totSetDone = totSetStart + riseDur;
  int totCurTime = 0;

  uint32_t stepMsecs = (riseDur * 60 * 1000) / 255;
  uint32_t stepSecs = 0;

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
    master = 255;
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
        b = b - 3;
        if (g > 64){
          g = g - 3;
        }
      } else if (g > 0){
        g = g - 3;
      } else if (r > 0){
        r = r - 3;
      }
      if (master > 0){
        master = master - 1;
      }
    }
    else if (brighten == true){
      if (r < 255)
      {
        r = r + 3;
      } else if (b < 165) {
        b = b + 3;
      } else if (g < 255){
        g = g + 3;
        if (b < 255){
          b = b + 3;
        }
      }
      if (master < 255){
        master = master + 1;
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