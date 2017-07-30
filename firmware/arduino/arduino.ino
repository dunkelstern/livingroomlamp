#include <NeoPixelBus.h>

const uint16_t PixelCount = 104;
const uint8_t PixelPin = 2;
const uint8_t lowPowerRingPin = 6;
const uint8_t highPowerRingPin = 5;

// 0 - 255 warm white
// 256 - 511 add cold white
uint16_t brightness = 255;
uint16_t hue = 0;
uint8_t saturation = 255;

uint16_t lowPowerRing = 255;
uint16_t highPowerRing = 0;

typedef enum _mode {
  modeWhite = 0,
  modeCinema = 1,
  modeMoodlight = 2
} Mode;

Mode mode = modeWhite;

const uint8_t pixelKeepout[3][2] = {
  { 2,   14},
  { 38,  48},
  { 70,  82},
};

NeoPixelBus<NeoGrbwFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);

void updateLight(void) {
  switch(mode) {
    case modeWhite: {
      uint16_t ww = brightness > 255 ? 255 : brightness;
      uint16_t cw = brightness > 255 ? (brightness - 256) : 0;
      for (uint16_t pixel = 0; pixel < PixelCount; pixel++) {
          strip.SetPixelColor(pixel, RgbwColor(cw, cw, cw, ww));
      }
      break;
    }
    case modeCinema: {
      for (uint16_t pixel = 0; pixel < PixelCount; pixel++) {
        bool skip = false;
        for (uint8_t i = 0; i < 3; i++) {
          if ((pixel >= pixelKeepout[i][0]) && (pixel <= pixelKeepout[i][1])) {
            skip = true;
            break;
          }
        }
        if (skip) {
          strip.SetPixelColor(pixel, RgbwColor(0,0,0,0));
        } else {
          uint16_t ww = brightness > 255 ? 255 : brightness;
          uint16_t cw = brightness > 255 ? (brightness - 256) : 0;
          strip.SetPixelColor(pixel, RgbwColor(cw, cw, cw, ww));
        }
      }
      break;
    }
    case modeMoodlight: {
      uint8_t b = brightness > 255 ? 255 : brightness;
      RgbwColor color = HsbColor((float)hue / 255.0, (float)saturation / 255.0, (float)b / 255.0);
      for (uint16_t pixel = 0; pixel < PixelCount; pixel++) {
        strip.SetPixelColor(pixel, color);
      }
      break;
    }
  }
  strip.Show();

  analogWrite(lowPowerRingPin, lowPowerRing);
  analogWrite(highPowerRingPin, highPowerRing);
}

void parseValue(String key, uint16_t value) {
  if (key.equals("mode")) {
    mode = (Mode)value;
  } else if (key.equals("hue")) {
    hue = value;
  } else if (key.equals("saturation")) {
    saturation = value;
  } else if (key.equals("brightness")) {
    brightness = value;
  } else if (key.equals("lowpowerring")) {
    lowPowerRing = value;
  } else if (key.equals("highpowerring")) {
    highPowerRing = value;
    updateLight();
  }
//  Serial.print(key);
//  Serial.print("=");
//  Serial.println(value);
}

void setup() {
  Serial.begin(74880);
  Serial.println("Lamp online");
  strip.Begin();
  pinMode(lowPowerRingPin, OUTPUT);
  pinMode(highPowerRingPin, OUTPUT);
  updateLight();
}

void loop() { 
  if (Serial.available() > 0) {
    String key = Serial.readStringUntil('=');
    key.trim();
    uint16_t value = Serial.parseInt();

    parseValue(key, value);  
  }
}



