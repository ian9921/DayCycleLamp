
/*
 * This ESP32 code is created by esp32io.com
 *
 * This ESP32 code is released in the public domain
 *
 * For more detail (instruction and wiring diagram), visit https://esp32io.com/tutorials/esp32-neopixel-led-strip
 */

#include <Adafruit_NeoPixel.h>

#define PIN_NEO_PIXEL 16  // The ESP32 pin GPIO16 connected to NeoPixel
#define NUM_PIXELS 144     // The number of LEDs (pixels) on NeoPixel LED strip

Adafruit_NeoPixel NeoPixel(NUM_PIXELS, PIN_NEO_PIXEL, NEO_GRB + NEO_KHZ800);

void setup() {
  NeoPixel.begin();  // initialize NeoPixel strip object (REQUIRED)
  //NeoPixel.setBrightness(255);
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
  Serial.begin(9600);
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