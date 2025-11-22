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

// Basic variables. Necessary for the rest of the variables to work.
const int PADDING = 1; // space between character and platform
const int PLATFORM_HEIGHT = 4;
const int PLATFORM_INTERVAL = 16;
const int JUMP = PLATFORM_INTERVAL + PLATFORM_HEIGHT;
const int UNIT = 10;
const int MAX_PLATFORMS = 16;

struct Platform
{
  int x;
  int y;
  int length;
  int visible;
};

struct Enemy
{
  int x;
  int y;
  bool visible;
};

// Platform variables
int speed = 2; // speed at which the scene moves
int lastX;     // the end of the furthest left platform

// Array containing platforms
Platform platforms[MAX_PLATFORMS];

// Array containing all the possible heights at which a platform could be
const int MAX_LEVELS = 2;
int levels[MAX_LEVELS] = {
    SCREEN_HEIGHT - PLATFORM_HEIGHT,
    SCREEN_HEIGHT - PLATFORM_HEIGHT - JUMP};

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
    0b00011000};

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
    0b11000011};

// Physics variables to handle jumps
float ySpeed = 0;   // vertical speed
float gravity = 1;  // gravity applied when falling down
float impulse = -8; // upward impulse

bool isJumping = false;
bool isFalling = false;

// enemy variables
const unsigned char enemyBitmap[] PROGMEM = {
    0b00011000, 0b00,
    0b00111100, 0b00,
    0b01111110, 0b00,
    0b11011011, 0b00,
    0b11111111, 0b00,
    0b10111101, 0b00,
    0b10100101, 0b00,
    0b11100111, 0b00,
    0b01000010, 0b00,
    0b10100101, 0b00};

int ENEMY_HEIGHT = 10;
int ENEMY_WIDTH = 10;
int const MAX_ENEMIES = 3;
Enemy enemies[MAX_ENEMIES];
int furthestEnemy = 0;

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

//Spawns an enemy on a random visible platform, off screen
void spawnEnemy()
{
  // See if there is an available slot in the enemies array
  for (int e = 0; e < MAX_ENEMIES; e++)
  {
    if (!enemies[e].visible)
    {
      for (int i = 0; i < MAX_PLATFORMS; i++)
      {
        //make sure the new enemy is placed both off screen and after the furthest enemy
        int startingPoint = SCREEN_WIDTH >= furthestEnemy ? SCREEN_WIDTH : furthestEnemy;
        startingPoint += UNIT;
        //Find a platform that has a point that can containg the new enemy 
        if (platforms[i].x + platforms[i].length >= startingPoint)
        {
          // spawn the enemy on a random coordinate of the platoform, that is off screen, only if it is at least a unit's distance from the furthest enemy
          enemies[e].x = random(startingPoint, platforms[i].x + platforms[i].length);
          enemies[e].y = platforms[i].y - PADDING - ENEMY_HEIGHT;
          oled.drawBitmap(enemies[e].x, enemies[e].y, enemyBitmap, ENEMY_WIDTH, ENEMY_HEIGHT, WHITE);
          enemies[e].visible = true;
          furthestEnemy = enemies[e].x + ENEMY_WIDTH;
          break;
        }
      }
    }
  }
}

void enemyCollision()
{
  for (int i = 0; i < MAX_ENEMIES; i++)
  {
    if (enemies[i].visible)
    {
      bool horizontalOverlap = (xCharacter + CHARACTER_WIDTH > enemies[i].x) &&
                               (xCharacter < enemies[i].x + ENEMY_WIDTH);
      bool verticalOverlap = (yCharacter + CHARACTER_HEIGHT > enemies[i].y) &&
                             (yCharacter < enemies[i].y + ENEMY_HEIGHT);

      if (horizontalOverlap && verticalOverlap)
      {
        lives--;
        enemies[i].visible = false;
        repositionPlayer();
      }
    }
  }
}

// function to draw the player
void drawGhost()
{
  oled.drawBitmap(xCharacter, yCharacter, ghostBitmap, CHARACTER_WIDTH, CHARACTER_HEIGHT, WHITE);
}

// When the UP button is clicked, the player jumps up
void jump()
{
  if (digitalRead(pinButtonUp) == LOW && prevPinButtonUp == HIGH)
  {
    if (!isFalling)
    { // player can jump only if on a platform
      ySpeed = impulse; //ets up upward movement
      isFalling = true;
      isJumping = true;
    }
  }
}

// applying physics logic to the jump
void jumpPhysics()
{

  int yCharacter_prev = yCharacter;

  // apply gravity to the player
  ySpeed += gravity;
  yCharacter += ySpeed;

  // handling the upward collision with the floating platform
  for (int i = 0; i < MAX_PLATFORMS; i++) {
    if (!platforms[i].visible) {
      continue;
    }
    // horizontal overlap
    bool horizontalOverlap =
      (xCharacter + CHARACTER_WIDTH > platforms[i].x) &&
      (xCharacter < platforms[i].x + platforms[i].length);
    // check head collides with bottom edge floating platform
    bool headHitsPlatform =
      (yCharacter <= platforms[i].y + PLATFORM_HEIGHT) &&         
      (yCharacter >= platforms[i].y) && 
      (ySpeed < 0);                                                
    if (horizontalOverlap && headHitsPlatform) {
      yCharacter = platforms[i].y + PLATFORM_HEIGHT;
      ySpeed = 0;
      break;
    }
  }


  bool landed = false;
  // checking for collisions on platforms
  for (int i = 0; i < MAX_PLATFORMS; i++)
  {
    if (!platforms[i].visible)
    {
      continue;
    }

    // Check horizontal overlap between character feet and platform
    bool horizontalOverlap =
        (xCharacter + CHARACTER_WIDTH > platforms[i].x) &&
        (xCharacter < platforms[i].x + platforms[i].length);

    int characterFeet = yCharacter + CHARACTER_HEIGHT;
    int characterFeet_prev = yCharacter_prev + CHARACTER_HEIGHT;

    // condition for player to land on platform
    if (horizontalOverlap && ySpeed >= 0)
    {
      if (characterFeet_prev <= platforms[i].y - PADDING && characterFeet > platforms[i].y - PADDING)
      {
        // Collision detected. Snap player to the top of the platform.
        yCharacter = platforms[i].y - PADDING - CHARACTER_HEIGHT;
        ySpeed = 0;
        isFalling = false;
        isJumping = false;
        landed = true;
        break;
      }
    }
  }

  // no collision so player keeps falling down
  if (!landed)
  {
    isFalling = true;
  }
}

// respawn the player on the first ground platform available
void repositionPlayer()
{
  for (int i = 0; i < MAX_PLATFORMS; i++)
  {
    if (platforms[i].visible)
    {
      // Find the lowest visible platform (ground)
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
void handleFallAndLives()
{
  if (yCharacter > SCREEN_HEIGHT)
  {
    if (lives > 0)
    {
      lives--;
      repositionPlayer();
    }
  }
}

// game ends
void gameOver()
{
  oled.clearDisplay();
  oled.setTextSize(2);
  oled.setTextColor(WHITE);
  oled.setCursor(5, 20);
  oled.println("GAME OVER");
  oled.setCursor(10, 45);
  oled.println("Press UP");
  oled.display();
  delay(30);

  if (digitalRead(pinButtonUp) == LOW)
  {
      // Reset lives
      lives = 3;

      // Reset player position
      ySpeed = 0;
      isJumping = false;
      yCharacter = levels[0] - CHARACTER_HEIGHT;

      // Reset platform array
      for (int i = 0; i < MAX_PLATFORMS; i++)
      {
          platforms[i].visible = false;
      }

      // Regenerate initial ground platform
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
void createPlatform()
{

  // Find first available slot in the platforms array
  for (int i = 0; i < MAX_PLATFORMS; i++)
  {
    // stop if the generated platforms fill the screen width
    if (lastX < SCREEN_WIDTH)
    {
      if (!platforms[i].visible)
      {
        // There is a  lesser chance of there being a gap
        int randomX = random(10) == 0 ? 1 : 0;
        platforms[i].x = lastX + UNIT * randomX;
        platforms[i].length = UNIT * random(1, 5);
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
  // last X needs to be recalculated after every scene update
  lastX = 0;
  furthestEnemy = 0;

  //move platforms
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
  //Move enemies
  for (int i = 0; i < MAX_ENEMIES; i++) {
    enemies[i].x -= speed;
    // if part of the enemy is still visible, display it
    if (enemies[i].x + ENEMY_WIDTH >= 0)
    {
      oled.drawBitmap(enemies[i].x, enemies[i].y, enemyBitmap, ENEMY_WIDTH, ENEMY_HEIGHT, WHITE);
      if(enemies[i].x + ENEMY_WIDTH >= furthestEnemy) {
        furthestEnemy = enemies[i].x + ENEMY_WIDTH;
      }
    }
    else
    {
      enemies[i].visible = false;
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

  // player initial position
  xCharacter = start.x + 4;
  yCharacter = start.y - CHARACTER_HEIGHT - PADDING;
  
  //Hide enemies initially
  for (int i = 0; i < MAX_ENEMIES; i++) {
    enemies[i].x = 0 - ENEMY_WIDTH;
    enemies[i].visible = false;
  }
}

void loop()
{

  oled.clearDisplay();

  if (lives <= 0)
  {
    gameOver();
  }
  else
  {
    // --- Game Update Logic (Only runs if lives > 0) ---

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

    // Enemy logic
    spawnEnemy();
    enemyCollision();

    // Score logic

    // Coins logic

    prevPinButtonUp = digitalRead(pinButtonUp);
  }

  oled.display();
  delay(30);
}
