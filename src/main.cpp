#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3D

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int pinButtonUp = 13;
int pinButtonDown = 27;

int prevPinButtonUp = HIGH;

int const PLATFORM_INTERVAL = 16;

// First Platform
int platformHeight = 4;
int platformLength1 = SCREEN_WIDTH;
int xPlatform1 = 0;
int yPlatform1 = SCREEN_HEIGHT - platformHeight;

// Second Platform
int platformLength2 = SCREEN_WIDTH;
int xPlatform2 = SCREEN_WIDTH / 2;
int yPlatform2 = yPlatform1 - PLATFORM_INTERVAL;

// Character
int characterHeight = 8;
int characterWidth = 8;
int xCharacter = 4;
int yCharacter = yPlatform1 - characterHeight - 1;

void jump()
{
  if (digitalRead(pinButtonUp) == LOW && prevPinButtonUp == HIGH)
  {
    yCharacter -= PLATFORM_INTERVAL - platformHeight - 1 - characterHeight;
  }
}

// Returns true if the bit is white
boolean getPixel(int x, int y)
{
  int width = SCREEN_WIDTH;
  int byteIndex = x + (y / 8) * width;
  uint8_t bitMask = 1 << (y & 7);
  return (oled.getBuffer()[byteIndex] & bitMask) != 0;
}

void gravity()
{
  if (!getPixel(xCharacter + characterHeight + 1, yCharacter + characterHeight + 1))
  {
    yCharacter += PLATFORM_INTERVAL + platformHeight + 1;
  }
}

void setup()
{
  Serial.begin(9600);

  pinMode(pinButtonUp, INPUT_PULLUP);
  pinMode(pinButtonDown, INPUT_PULLUP);

  if (!oled.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
  }
}

void loop()
{
  oled.clearDisplay();

  // handle lives

  // handle score

  // generate character
  oled.fillRect(xCharacter, yCharacter, characterWidth, characterHeight, WHITE);

  // generate platform(s)
  oled.fillRect(xPlatform1, yPlatform1, platformLength1, platformHeight, WHITE);
  oled.fillRect(xPlatform2, yPlatform2, platformLength2, platformHeight, WHITE);

  // generate coins

  // Handle jump
  jump();
  // gravity();

  prevPinButtonUp = digitalRead(pinButtonUp);

  oled.display();
  delay(30);
}