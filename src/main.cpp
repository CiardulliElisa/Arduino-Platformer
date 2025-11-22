#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
//#define SCREEN_ADDRESS 0x3D //Physical
#define SCREEN_ADDRESS 0x3C // Emulator

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define SDA_PIN 21
#define SCL_PIN 22

// Basic variables. Necessary for the rest of the variables to work.
const int PADDING = 1; // space between character and platform               
const int PLATFORM_HEIGHT = 4;      
const int PLATFORM_INTERVAL = 16;   
const int JUMP = PLATFORM_INTERVAL + PLATFORM_HEIGHT;
const int UNIT = 8;
const int MAX_PLATFORMS = 16;        

struct Platform {
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
  SCREEN_HEIGHT - PLATFORM_HEIGHT - JUMP
};

// Hardware logic variables
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
  0b00011000
};

// Player variables
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
  0b11000011  
};

// Physics variables to handle jumps
float ySpeed = 0;     // vertical speed
float gravity = 0.8;  // gravity applied when falling down
float impulse = -7;   // upward impulse

bool isJumping = false;
bool isFalling = false;

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

// function to draw the player
void drawGhost() {
  oled.drawBitmap(xCharacter, yCharacter, ghostBitmap, CHARACTER_WIDTH, CHARACTER_HEIGHT, WHITE);
}

// When the UP button is clicked, the player jumps up 
void jump() {
  if (digitalRead(pinButtonUp) == LOW && prevPinButtonUp == HIGH) {
    if (!isFalling) {     // player can jump only if on a platform
      ySpeed = impulse;    
      isFalling = true;           
      isJumping = true;
    }
  }
}

// applying physics logic to the jump
void jumpPhysics() {
  
  // apply gravity to the player
  ySpeed += gravity;
  yCharacter += ySpeed;

  bool landed = false;
  // checking for collisions on platforms
  for (int i = 0; i < MAX_PLATFORMS; i++) {
    if (!platforms[i].visible) {
      continue;
    } 

    // Check horizontal overlap between character feet and platform
    bool horizontalOverlap =
      (xCharacter + CHARACTER_WIDTH > platforms[i].x) &&
      (xCharacter < platforms[i].x + platforms[i].length);

    // Check if character feet intersect platform top
    bool feetTouchPlatform =
      (yCharacter + CHARACTER_HEIGHT >= platforms[i].y - PADDING) &&
      (yCharacter + CHARACTER_HEIGHT <= platforms[i].y + PLATFORM_HEIGHT);

    // condition for player to land on platform
    if (horizontalOverlap && feetTouchPlatform && ySpeed >= 0) {
      yCharacter = platforms[i].y - CHARACTER_HEIGHT - PADDING;
      ySpeed = 0;
      isFalling = false;
      isJumping = false;
      landed = true;
      break;
    }
  }

  // no collision so player keeps falling down
  if (!landed) {
    isFalling = true;
  }
}

// respawn the player on the first ground platform available
void repositionPlayer() {
  for (int i = 0; i < MAX_PLATFORMS; i++) {
    if (platforms[i].visible && platforms[i].y == levels[0]) { // ground platform identified
      xCharacter = platforms[i].x + 4; 
      yCharacter = platforms[i].y - CHARACTER_HEIGHT - PADDING; 
      ySpeed = 0;
      isFalling = false;
      isJumping = false;
      break;
    }
  }
}

// handle player falling and losing lives
void handleFallAndLives() {
  if (yCharacter > SCREEN_HEIGHT) { 
    if (lives > 0) {
      lives--;          
      repositionPlayer(); 
    }
  }
}

// game ends
void gameOver() {
  oled.clearDisplay();
  oled.setTextSize(2);
  oled.setTextColor(WHITE);
  oled.setCursor(5, 20);
  oled.println("GAME OVER");
  oled.setCursor(10, 45);
  oled.println("Press UP");
  oled.display();
  delay(30);

  if (digitalRead(pinButtonUp) == LOW) {
    // Reset lives
    lives = 3;

    // Reset player position
    ySpeed = 0;
    isJumping = false;
    yCharacter = levels[0] - CHARACTER_HEIGHT;

    // Reset platform array
    for (int i = 0; i < MAX_PLATFORMS; i++) {
        platforms[i].visible = false;
    }

    // regenerate initial ground platform
    Platform start;
    start.length = 50;
    start.x = 0;
    start.y = SCREEN_HEIGHT - PLATFORM_HEIGHT;
    start.visible = true;
    platforms[0] = start;
    lastX = start.x + start.length;
    xCharacter = start.x + 4;
  }
} 

// Create a platform when possible
void createPlatform() {

  // Find first available slot in the platforms array
  for (int i = 0; i < MAX_PLATFORMS; i++) {
    // stop if the generated platforms fill the screen width
    if (lastX < SCREEN_WIDTH) {
      if (!platforms[i].visible) {
        // There is a  lesser chance of there being a gap
        int randomX = random(6) == 0 ? 1 : 0;
        platforms[i].x = lastX + UNIT * randomX;
        platforms[i].length = UNIT;
        platforms[i].y = levels[random(MAX_LEVELS)];
        platforms[i].visible = true;
        // Update last x, if the new platform is further right than any other platform
        if (lastX <= platforms[i].x + platforms[i].length) {
          lastX = platforms[i].x + platforms[i].length;
        }
      }
    }
  }
}

// Move and display platforms
void updateScene() {
  // last X needs to be recalculated after every scene update
  lastX = 0;

  for (int i = 0; i < MAX_PLATFORMS; i++) {
    if (platforms[i].visible) {
      platforms[i].x -= speed;
      // Handle platform disappearing left side
      if (platforms[i].x + platforms[i].length <= 0) {
        platforms[i].visible = false;
      }
      else if (platforms[i].x + platforms[i].length >= lastX) {
        lastX = platforms[i].x + platforms[i].length;
      }

      // Display platform
      oled.fillRect(platforms[i].x, platforms[i].y, platforms[i].length, PLATFORM_HEIGHT, WHITE);
    }
  }
}


void setup() {
  Wire.begin(SDA_PIN, SCL_PIN); // Emulator
  Serial.begin(9600);

  pinMode(pinButtonUp, INPUT_PULLUP);
  pinMode(pinButtonDown, INPUT_PULLUP);

  if (!oled.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
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

  // player initial position
  xCharacter = start.x + 4;
  yCharacter = start.y - CHARACTER_HEIGHT - PADDING;

  // Generate character initial position
  // oled.fillRect(start.x + 4, start.y + PLATFORM_HEIGHT + 1, CHARACTER_WIDTH, CHARACTER_HEIGHT, WHITE);
}

void loop() {

  // checking the game over condition
  if (lives <= 0) {
    gameOver();
    return;
  }

  oled.clearDisplay();

  // Display hearts
  drawHearts(lives);
  // Display player
  drawGhost();

  // Handle scene (platforms)
  createPlatform();
  updateScene();

  // Handle character logic
  jump();
  jumpPhysics(); 
  handleFallAndLives();
  drawEnemy();

  prevPinButtonUp = digitalRead(pinButtonUp);

  oled.display();
  delay(30);
}
