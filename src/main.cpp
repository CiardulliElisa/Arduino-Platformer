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

#define SDA_PIN 21 // Emulator
#define SCL_PIN 22 // Emulator

// Basic variables. Necessary for the rest of the variables to work.
int const PADDING = 1; // space between character and platform
int const PLATFORM_HEIGHT = 4;
int const PLATFORM_INTERVAL = 16;
int const JUMP = PLATFORM_INTERVAL + PLATFORM_HEIGHT;
int const UNIT = 16;
int const MAX_PLATFORMS = (SCREEN_WIDTH / UNIT) * 2;

struct Platform
{
  int x;
  int y;
  int length;
  int visible;
};

// Platform variables
int speed = 1; // speed at which the platforms move
int lastX;     // the end of the furthest left platform

// Array containing platforms
Platform platforms[MAX_PLATFORMS];

// Array containing all the possible heights at which a platform could be
const int MAX_LEVELS = 2;
int levels[MAX_LEVELS] = {
    SCREEN_HEIGHT - PLATFORM_HEIGHT,
    SCREEN_HEIGHT - PLATFORM_HEIGHT - JUMP};

// Hardware logic  Variables
int pinButtonUp = 13;
int pinButtonDown = 27;
int prevPinButtonUp = HIGH;

// Heart variables
const int heartWidth = 8;
const int heartHeight = 7;
int lives = 3;
const unsigned char heartBitmap[] PROGMEM = {
    0b01100110,
    0b11111111,
    0b11111111,
    0b11111111,
    0b01111110,
    0b00111100,
    0b00011000};

// Character variables
const int CHARACTER_WIDTH = 8;
const int CHARACTER_HEIGHT = 10;
int xCharacter;
int yCharacter;
const unsigned char ghostBitmap[] PROGMEM = {
    0b00111100,
    0b01111110,
    0b11111111,
    0b11011011,
    0b11011011,
    0b11011011,
    0b11111111,
    0b11110111,
    0b11100111,
    0b11000011};

// enemy variables
const unsigned char enemyBitmap[] PROGMEM = {
    0b00011000, // ...##...
    0b00111100, // ..####..
    0b01111110, // .######.
    0b11111100, // ######.. <-- Mouth here
    0b11111000, // #####... <-- Mouth here
    0b11111100, // ######.. <-- Mouth here
    0b01111110, // .######.
    0b00111100, // ..####..
    0b00011000, // ...##...
    0b00000000  // ........
};
int xEnemy;
int yEnemy;
int ENEMY_HEIGHT = 10;
int ENEMY_WIDTH = 8;

// function to draw the hearts
void drawHearts(int lives)
{
  for (int i = 0; i < lives; i++)
  {
    int xHeart = SCREEN_WIDTH - (i + 1) * (heartWidth + 2);
    int yHeart = 2;
    oled.drawBitmap(xHeart, yHeart, heartBitmap, heartWidth, heartHeight, WHITE);
  }
}

void drawEnemy()
{
  xEnemy = 2;
  yEnemy = 2;
  oled.drawBitmap(xEnemy, yEnemy, enemyBitmap, ENEMY_WIDTH, ENEMY_HEIGHT, WHITE);
}

// function to draw the pacman-like character
void drawGhost()
{
  oled.drawBitmap(xCharacter, yCharacter, ghostBitmap, CHARACTER_WIDTH, CHARACTER_HEIGHT, WHITE);
}

// When the UP button is clicked, the character jumps up
void jump()
{
  if (digitalRead(pinButtonUp) == LOW && prevPinButtonUp == HIGH)
  {
    yCharacter -= JUMP;
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

// Create a platform, if possible
void createPlatform()
{
  // Find first available slot in the platforms array (the platforms should be ordered in such a way that )
  for (int i = 0; i < MAX_PLATFORMS; i++)
  {
    // stop if the generated platforms fill the screen width
    if (lastX < SCREEN_WIDTH * 2)
    {
      if (!platforms[i].visible)
      {
        // There is a  lesser chance of there being a gap
        int randomX = random(6) == 0 ? 1 : 0;
        platforms[i].x = lastX + UNIT * randomX;
        platforms[i].length = UNIT;
        platforms[i].y = levels[random(MAX_LEVELS)];
        platforms[i].visible = true;
        // Update last x, if the new platform is further right than any other platform
        if (lastX <= platforms[i].x + platforms[i].length)
        {
          lastX = platforms[i].x + platforms[i].length;
        }
      }
    }
  }
}

// Move and display platforms
void updateScene()
{
  // last X needs to be recalculated after every scende update
  lastX = 0;

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
      else if (platforms[i].x + platforms[i].length >= lastX)
      {
        lastX = platforms[i].x + platforms[i].length;
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

  // Create the starting platform to make sure that the character is not floating in the void
  Platform start;
  start.length = 50;
  start.x = 0;
  start.y = SCREEN_HEIGHT - PLATFORM_HEIGHT;
  start.visible = true;
  lastX = start.x + start.length;

  platforms[0] = start;

  xCharacter = start.x + 4;
  yCharacter = start.y - CHARACTER_HEIGHT - PADDING;

  // Generate character initial position
  // oled.fillRect(start.x + 4, start.y + PLATFORM_HEIGHT + 1, CHARACTER_WIDTH, CHARACTER_HEIGHT, WHITE);
}

void loop()
{
  oled.clearDisplay();

  // Display hearts
  drawHearts(lives);
  // Display character
  drawGhost();

  // TODO: handle lives/hearts

  // Handle scene (platforms)
  createPlatform();
  updateScene();

  // TODO: handle score

  // TODO: Handle character logic
  jump();
  // gravity();

  // TODO: handle coins

  // Handle enemies
  drawEnemy();

  prevPinButtonUp = digitalRead(pinButtonUp);

  oled.display();
  delay(30);
}