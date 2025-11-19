#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
// #define SCREEN_ADDRESS 0x3D //Physical
#define SCREEN_ADDRESS 0x3C // Emulator

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define SDA_PIN 21
#define SCL_PIN 22

//Basic variables. Necessary for the rest of the variables to work.
int const PADDING = 1; // space between character and platform
int const PLATFORM_HEIGHT = 4;
int const PLATFORM_INTERVAL = 16;
int const JUMP = PLATFORM_INTERVAL + PLATFORM_HEIGHT;

struct Platform
{
  int x;
  int y;
  int length;
  int visible;
};

//Platform variables
int speed = 1; //speed at which the platforms move

// Array containing platforms
const int MAX_PLATFORMS = 6;
Platform platforms[MAX_PLATFORMS];

// Array containing all the possible heights at which a platform could be
const int MAX_LEVELS = 2;
int levels[MAX_LEVELS] = {
    SCREEN_HEIGHT - PLATFORM_HEIGHT,
    SCREEN_HEIGHT - PLATFORM_HEIGHT - JUMP};

//Hardware logic  Variables
int pinButtonUp = 13;
int pinButtonDown = 27;
int prevPinButtonUp = HIGH;

// Character variables
int CHARACTER_WIDTH = 8;
int CHARACTER_HEIGHT = 8;
int xCharacter;
int yCharacter;

//When the UP button is clicked, the character jumps up
void jump() 
{
  if (digitalRead(pinButtonUp) == LOW && prevPinButtonUp == HIGH)
  {
    yCharacter -= JUMP;
  }
}

// Returns true if the bit is white
boolean getPixel(int x, int y) {
  int width = SCREEN_WIDTH;
  int byteIndex = x + (y / 8) * width;
  uint8_t bitMask = 1 << (y & 7);
  return (oled.getBuffer()[byteIndex] & bitMask) != 0;
}

//Create a platform, if possible
void createPlatform()
{
  // Find an available slot in the platforms array
  for (int i = 0; i < MAX_PLATFORMS; i++)
  {
    if (!platforms[i].visible)
    {
      platforms[i].x = SCREEN_WIDTH; //start right off-screen, invisible
      platforms[i].length = random(10, 20);
      platforms[i].y = levels[random(MAX_LEVELS)];
      platforms[i].visible = true;
      break;
    }
  }
}

//Move and display platforms
void updateScene()
{
  for (int i = 0; i < MAX_PLATFORMS; i++)
  {
    if (platforms[i].visible)
    {
      platforms[i].x -= speed;

      // Handle platform disappearing left side
      if (platforms[i].x + platforms[i].length <= 0)
      {
        platforms[i].visible = false;
      }

      // Display platform
      oled.fillRect(platforms[i].x, platforms[i].y, platforms[i].length, PLATFORM_HEIGHT, WHITE);
    }
  }
}

void setup()
{
  Wire.begin(SDA_PIN, SCL_PIN); // Emulator
  Serial.begin(9600);

  pinMode(pinButtonUp, INPUT_PULLUP);
  pinMode(pinButtonDown, INPUT_PULLUP);

  if (!oled.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
  }

  //Create the starting platform to make sure that the character is not floating in the void
  Platform start;
  start.length = SCREEN_WIDTH;
  start.x = 0;
  start.y = SCREEN_HEIGHT - PLATFORM_HEIGHT;
  start.visible = true;

  platforms[0] = start;

  xCharacter = start.x + 4;
  yCharacter = start.y - CHARACTER_HEIGHT - PADDING;

  //Generate character initial position
  oled.fillRect(start.x + 4, start.y + PLATFORM_HEIGHT + 1, CHARACTER_WIDTH, CHARACTER_HEIGHT, WHITE);
}

void loop() {
  oled.clearDisplay();

  //TODO: handle lives/hearts

  //Handle scene (platforms)
  createPlatform();
  updateScene();

  // TODO: handle score

  // TODO: Handle character logic
  oled.fillRect(xCharacter, yCharacter, CHARACTER_WIDTH, CHARACTER_HEIGHT, WHITE);
  jump();
  // gravity();

  // TODO: handle coins

  // Handle enemies

  prevPinButtonUp = digitalRead(pinButtonUp);

  oled.display();
  delay(30);
}