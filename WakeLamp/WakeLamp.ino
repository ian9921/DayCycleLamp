#include <RTClib.h>


/*
 * This ESP32 code is created by esp32io.com
 *
 * This ESP32 code is released in the public domain
 *
 * For more detail (instruction and wiring diagram), visit https://esp32io.com/tutorials/esp32-neopixel-led-strip
 */

#include <Adafruit_NeoPixel.h>

#define PIN_NEO_PIXEL 14  // The ESP32 pin GPIO16 connected to NeoPixel
#define NUM_PIXELS 144     // The number of LEDs (pixels) on NeoPixel LED strip

Adafruit_NeoPixel NeoPixel(NUM_PIXELS, PIN_NEO_PIXEL, NEO_GRB + NEO_KHZ800);

RTC_PCF8523 rtc;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

void setup() {
  Serial.begin(57600);

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

// void loop() {
//   uint32_t fullBright = 0;
//   NeoPixel.clear();  // set all pixel colors to 'off'. It only takes effect if pixels.show() is called

//   for (uint8_t bright = 120; bright >= 0; bright = bright - 1){
//     //NeoPixel.setBrightness(bright);
//     for (int pixel = 0; pixel < NUM_PIXELS; pixel++) {           // for each pixel
//       NeoPixel.setPixelColor(pixel, NeoPixel.Color(bright, bright, bright));  // it only takes effect if pixels.show() is called
//     }
//     NeoPixel.show();
//     delay(5);
//   }

// }

void loop() {
  brighten();
  darken();
  DateTime now = rtc.now();

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

  Serial.print(" since midnight 1/1/1970 = ");
  Serial.print(now.unixtime());
  Serial.print("s = ");
  Serial.print(now.unixtime() / 86400L);
  Serial.println("d");

  // calculate a date which is 7 days, 12 hours and 30 seconds into the future
  DateTime future (now + TimeSpan(7,12,30,6));

  Serial.print(" now + 7d + 12h + 30m + 6s: ");
  Serial.print(future.year(), DEC);
  Serial.print('/');
  Serial.print(future.month(), DEC);
  Serial.print('/');
  Serial.print(future.day(), DEC);
  Serial.print(' ');
  Serial.print(future.hour(), DEC);
  Serial.print(':');
  Serial.print(future.minute(), DEC);
  Serial.print(':');
  Serial.print(future.second(), DEC);
  Serial.println();

  Serial.println();
  delay(3000);
}

// 0 to 255
void brighten() {
  uint16_t i, j;

  for (j = 1; j <= 100; j++) {
    for (i = 0; i < NeoPixel.numPixels(); i++) {
      NeoPixel.setPixelColor(i, j, j, j);
    }
    NeoPixel.show();
    delay(10);
  }
  //delay(1500);
}

// 255 to 0
void darken() {
  //Serial.begin(9600);
  uint16_t i, j;

  for (j = 100; j >= 1; j--) {
    for (i = 0; i < NeoPixel.numPixels(); i++) {
      NeoPixel.setPixelColor(i, j, j, j);
    }
    NeoPixel.show();
    delay(10);
    Serial.println(j);
  }
  NeoPixel.clear();
  NeoPixel.show();
  delay(1500);
}