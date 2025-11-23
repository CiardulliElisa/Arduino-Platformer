#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <algorithm>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3D //Physical
// #define SCREEN_ADDRESS 0x3C // Emulator

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define SDA_PIN 21
#define SCL_PIN 22

// Hardware logic variables
int pinButtonUp = 13;
int pinButtonShoot = 27;
int prevPinButtonUp = HIGH;
int prevPinButtonShoot = HIGH;

// Game environment variables.
const int PADDING = 1;
const int LEFT_PADDING = 4;
const int PLATFORM_HEIGHT = 4;
const int PLATFORM_INTERVAL = 16;
const int JUMP = PLATFORM_INTERVAL + PLATFORM_HEIGHT;
const int UNIT = 15;
const int PLATFORM_DISTANCE = 10;
const int SPEED = 2; // speed at which the scene moves

struct Enemy
{
  int x;
  int y;
  bool visible;
};

// Platform variables
struct Platform
{
  int x;
  int y;
  int length;
  int visible;
};

// the end of the right most platform
int lastX;

// Array containing platforms
const int MAX_PLATFORMS = 16;
Platform platforms[MAX_PLATFORMS];

// Array containing all the possible heights at which a platform could be
const int MAX_LEVELS = 2;
int levels[MAX_LEVELS] = {
    SCREEN_HEIGHT - PLATFORM_HEIGHT,
    SCREEN_HEIGHT - PLATFORM_HEIGHT - JUMP};

// Lives variables
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

struct Heart
{
  int x;
  int y;
  bool visible;
};

void updateScene();

const int MAX_HEARTS = 2;
int furthestHeart = 0;
Heart hearts[MAX_HEARTS];

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

// Enemy variables
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
const int MAX_ENEMIES = 5;
Enemy enemies[MAX_ENEMIES];
int furthestEnemy = 0;

// Bullet variables
struct Bullet
{
  int initialPos;
  int x;
  int y;
  bool visible;
};

int BULLET_RADIUS = 2;
int MAX_BULLET_DISTANCE = SCREEN_WIDTH / 2;
const int MAX_BULLETS = 50;
Bullet bullets[MAX_BULLETS];

// Scoring variables
int score = 0;
const int SCORE_INCREMENT = 10;
const int ENEMY_KILLED_INCREMENT = 20;
const int SCORE_INTERVAL = 2000;
unsigned long lastScoreTime = 0;

// Created a heart for each life in the top right of the screen
void drawHearts(int lives)
{
  for (int i = 0; i < lives; i++)
  {
    int xHeart = SCREEN_WIDTH - (i + 1) * (heartWidth + 2);
    int yHeart = 2;
    oled.drawBitmap(xHeart, yHeart, heartBitmap, heartWidth, heartHeight, WHITE);
  }
}

// Updating the score game
void updateScore()
{
  // add points every 2s
  if (millis() - lastScoreTime >= SCORE_INTERVAL)
  {
    score += SCORE_INCREMENT;
    lastScoreTime = millis();
  }
}

// Displaying score on the screen
void drawScore()
{
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  oled.setCursor(2, 2);
  oled.print("Score: ");
  oled.print(score);
}

// Moves hearts, draws them on screen, and hides them if off-screen
void updateHearts()
{
  for (int h = 0; h < MAX_HEARTS; h++)
  {
    if (hearts[h].visible)
    {
      hearts[h].x -= SPEED;

      if (hearts[h].x + heartWidth <= 0)
      {
        hearts[h].visible = false;
      }

      if (hearts[h].visible)
      {
        oled.drawBitmap(hearts[h].x, hearts[h].y, heartBitmap, heartWidth, heartHeight, WHITE);
      }
    }
  }
}

// If the player touches a heart, it gets a life, the heart disappears
void heartCollision()
{
  for (int h = 0; h < MAX_HEARTS; h++)
  {
    if (hearts[h].visible)
    {

      bool horizontal =
          (xCharacter + CHARACTER_WIDTH > hearts[h].x) &&
          (xCharacter < hearts[h].x + heartWidth);

      bool vertical =
          (yCharacter + CHARACTER_HEIGHT > hearts[h].y) &&
          (yCharacter < hearts[h].y + heartHeight);

      if (horizontal && vertical)
      {
        if (lives < 3)
        {
          lives++;
        }
        hearts[h].visible = false;
      }
    }
  }
}

// Find a point that is off screen, to the right of both the furthest enemy and the furthest heart, where to spawn an object
Platform findSpawnPoint(int objWidth, int objHeight)
{
  Platform p;
  p.x = -1; // make the platform recognisable if it should not change
  p.y = -1;
  for (int i = 0; i < MAX_PLATFORMS; i++)
  {
    if (platforms[i].visible && platforms[i].x >= SCREEN_WIDTH)
    {
      // off screen, to the right of the furthest heart and the furthest enemy
      int startingPoint = max(SCREEN_WIDTH, furthestHeart);
      startingPoint = max(startingPoint, furthestEnemy);
      // add additional distance, so the spawned object is not too close to the last one
      startingPoint += UNIT;
      startingPoint = max(startingPoint, platforms[i].x);
      // the object must be on the platform in its entirety
      int endPoint = platforms[i].x + platforms[i].length - objWidth;
      if (endPoint >= startingPoint)
      {
        p.x = random(startingPoint, endPoint + 1);
        p.y = platforms[i].y - objHeight - PADDING;
        // Return the coordinates of the spawn point as a one point platform, this is done to avoid creating a new struct
        return p;
      }
    }
  }

  // case no off screen platfroms is found --> spawn on leftmost visible platform
  for (int i = 0; i < MAX_PLATFORMS; i++)
  {
    if (platforms[i].visible)
    {
      int startX = platforms[i].x;
      int endX = platforms[i].x + platforms[i].length - objWidth;
      if (endX >= startX)
      {
          p.x = startX + 2; 
          p.y = platforms[i].y - objHeight - PADDING;
          return p;
      }
    }
  }
  return p;
}

// Spawns a heart on a random platform, off screen, making sure it does not overlap other hearts or enemies
void spawnHeart()
{
  if (lives < 3)
  {
    // See if there is an available slot in the hearts array
    for (int h = 0; h < MAX_HEARTS; h++)
    {
      if (!hearts[h].visible)
      {
        Platform spawnPoint = findSpawnPoint(heartWidth, heartHeight);
        if (spawnPoint.x != -1 && spawnPoint.y != -1)
        {
          hearts[h].x = spawnPoint.x;
          hearts[h].y = spawnPoint.y;
          hearts[h].visible = true;
          // this becomes the last heart
          furthestHeart = hearts[h].x + heartWidth;
          break;
        }
      }
    }
  }
}

// Spawns an enemy on a random platform, off screen
void spawnEnemy()
{
  // See if there is an available slot in the enemies array
  for (int e = 0; e < MAX_ENEMIES; e++)
  {
    if (!enemies[e].visible)
    {
      Platform spawnPoint = findSpawnPoint(ENEMY_WIDTH, ENEMY_HEIGHT);
      if (spawnPoint.x != -1 && spawnPoint.y != -1)
      {
        enemies[e].x = spawnPoint.x;
        enemies[e].y = spawnPoint.y;
        enemies[e].visible = true;
        // this becomes the last enemy
        furthestEnemy = enemies[e].x + ENEMY_WIDTH;
        break;
      }
    }
  }
}

// When the user clicks the shooting button, a bullet is shot out from the character
void shoot()
{
  // shoot bullest when the shoot button is clicked
  if (digitalRead(pinButtonShoot) == LOW && prevPinButtonShoot == HIGH)
  {
    for (int i = 0; i < MAX_BULLETS; i++)
    {
      if (!bullets[i].visible)
      {
        bullets[i].x = xCharacter + CHARACTER_WIDTH + 4;
        bullets[i].y = yCharacter + (CHARACTER_HEIGHT / 2);
        bullets[i].visible = true;
        bullets[i].initialPos = bullets[i].x;
        break;
      }
    }
  }
}

// If the player touches an enemy, it loses a life, the enemy disappears
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
      }
    }
  }
}

// When a bullet hits an enemy, both the enemy and the bullet disappear
void enemyKill()
{
  for (int i = 0; i < MAX_ENEMIES; i++)
  {
    if (enemies[i].visible)
    {
      for (int j = 0; j < MAX_BULLETS; j++)
      {
        if (bullets[j].visible)
        {
          // Check collision between bullet and enemy
          bool horizontalOverlap = (bullets[j].x + BULLET_RADIUS > enemies[i].x) &&
                                   (bullets[j].x - BULLET_RADIUS < enemies[i].x + ENEMY_WIDTH);
          bool verticalOverlap = (bullets[j].y + BULLET_RADIUS > enemies[i].y) &&
                                 (bullets[j].y - BULLET_RADIUS < enemies[i].y + ENEMY_HEIGHT);

          if (horizontalOverlap && verticalOverlap)
          {
            enemies[i].visible = false;
            bullets[j].visible = false;
            score += ENEMY_KILLED_INCREMENT;
            break;
          }
        }
      }
    }
  }
}

// Draws the player
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
    {                   // player can jump only if on a platform
      ySpeed = impulse; // zets up upward movement
      isFalling = true;
      isJumping = true;
    }
  }
}

// applying physics logic to the jump, so the character falls down with gravity
void jumpPhysics()
{

  // character before fall increment
  int yCharacter_prev = yCharacter;

  // apply gravity to the player
  ySpeed += gravity;
  yCharacter += ySpeed;

  // handling the upward collision with the floating platform
  for (int i = 0; i < MAX_PLATFORMS; i++)
  {
    if (!platforms[i].visible)
    {
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
    if (horizontalOverlap && headHitsPlatform)
    {
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

// reposition the player on the first platform (either ground ir floating) that appears after the fall
void repositionPlayer()
{
  int xFall = xCharacter; // position of the player fall
  int platformIndex = -1; // right platform on where to reposition the player
  bool platformFound = false;

  // Check all platforms looking for the one that covers the initial position of the character
  while(platformIndex == -1) {
    for (int i = 0; i < MAX_PLATFORMS; i++)
    {
      if (!platforms[i].visible || platforms[i].x >= SCREEN_WIDTH)
      {
        continue;
      }
      if (platforms[i].x <= LEFT_PADDING && LEFT_PADDING <= platforms[i].x + platforms[i].length)
      {
        platformIndex = i;
        break;
      }
    }
    // If no suitable platform is found, move them all, until a suitable one is found
    if (platformIndex == -1)
    {
      updateScene();
    }
  }

  // reposition player after the fall
  if (platformIndex != -1)
  {
    xCharacter = LEFT_PADDING;
    yCharacter = platforms[platformIndex].y - CHARACTER_HEIGHT - PADDING;
    ySpeed = 0;
    isFalling = false;
    isJumping = false;
  }
}

// When the player falls off screen but still has lives, they lose a life. If the character is repositioned and the game continues
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

// Resets all variables and created the first scene
void resetGame()
{
  // Reset score
  score = 0;
  // Reset Lives
  lives = 3;
  // Reset platforms
  lastX = 0;
  for (int i = 0; i < MAX_PLATFORMS; i++)
  {
    platforms[i].visible = false;
  }
  //  Create the starting platform to make sure that the character is not floating in the void
  Platform start;
  start.length = 50;
  start.x = 0;
  start.y = SCREEN_HEIGHT - PLATFORM_HEIGHT;
  start.visible = true;
  lastX = start.x + start.length;
  platforms[0] = start;

  // Create the character
  xCharacter = start.x + 4;
  yCharacter = start.y - CHARACTER_HEIGHT - PADDING;

  // Reset enemies
  furthestEnemy = 0;
  for (int i = 0; i < MAX_ENEMIES; i++)
  {
    enemies[i].x = 0 - ENEMY_WIDTH;
    enemies[i].visible = false;
  }

  // Reset bullets
  for (int i = 0; i < MAX_BULLETS; i++)
  {
    bullets[i].visible = false;
  }

  // Reset hearts
  furthestHeart = 0;
  for (int i = 0; i < MAX_HEARTS; i++)
  {
    hearts[i].visible = false;
    hearts[i].x = 0;
    hearts[i].y = 0;
  }
}

// A Game over screen is shown. The player can press one of the buttons to restart the game
void gameOver()
{
  // Show game over screen
  oled.setTextSize(2);
  oled.setTextColor(WHITE);

  // Calculate center for x (rough approximation)
  int16_t x1, y1;
  uint16_t w, h;
  oled.getTextBounds("GAME OVER", 0, 0, &x1, &y1, &w, &h);
  oled.setCursor((SCREEN_WIDTH - w) / 2, 15);
  oled.println("GAME OVER");
  oled.setTextSize(1);
  oled.getTextBounds("Press button", 0, 0, &x1, &y1, &w, &h);
  oled.setCursor((SCREEN_WIDTH - w) / 2, 50);
  oled.println("Press button");
  oled.display();
  delay(30);

  // Restart the game
  if (digitalRead(pinButtonUp) == LOW)
  {
    resetGame();
  }
}

// Create a platform
void createPlatform()
{
  // Find first available slot in the platforms array
  for (int i = 0; i < MAX_PLATFORMS; i++)
  {
    if (!platforms[i].visible)
    {
      // There is a  lesser chance of there being a gap
      int randomX = random(4) == 0 ? 1 : 0;
      platforms[i].x = lastX + PLATFORM_DISTANCE * randomX;
      platforms[i].length = UNIT * random(1, 5);
      platforms[i].y = levels[random(MAX_LEVELS)];
      platforms[i].visible = true;
      // Update last x, if the new platform is further right than any other platform
      if (lastX <= platforms[i].x + platforms[i].length)
      {
        lastX = platforms[i].x + platforms[i].length;
      }
      break;
    }
  }
}

// Move the bullets forward and remove them if they are no longer visible or if they have reached maximum travelling distance
void updateBullets()
{
  for (int i = 0; i < MAX_BULLETS; i++)
  {
    if (bullets[i].visible)
    {
      bullets[i].x += SPEED + 2;
    }
    if (bullets[i].x <= 0 || bullets[i].x >= SCREEN_WIDTH || bullets[i].x - bullets[i].initialPos >= MAX_BULLET_DISTANCE)
    {
      bullets[i].visible = false;
    }
    if (bullets[i].visible)
    {
      oled.fillCircle(bullets[i].x, bullets[i].y, BULLET_RADIUS, WHITE);
    }
  }
}

// Move platforms to the left according to the scene speed. Remove them when they completely disappear off screen
void updatePlatforms()
{
  lastX = 0;

  for (int i = 0; i < MAX_PLATFORMS; i++)
  {
    if (platforms[i].visible)
    {
      platforms[i].x -= SPEED;
      if (platforms[i].x + platforms[i].length <= 0)
      {
        platforms[i].visible = false;
      }
      else if (platforms[i].x + platforms[i].length >= lastX)
      {
        lastX = platforms[i].x + platforms[i].length;
      }
      oled.fillRect(platforms[i].x, platforms[i].y, platforms[i].length, PLATFORM_HEIGHT, WHITE);
    }
  }
}

// Move platforms to the left according to the scene speed. Remove them when they completely disappear off screen
void updateEnemies()
{
  furthestEnemy = 0;
  for (int i = 0; i < MAX_ENEMIES; i++)
  {
    if (enemies[i].visible)
    {
      enemies[i].x -= SPEED;
    }
    if (enemies[i].x + ENEMY_WIDTH <= 0)
    {
      enemies[i].visible = false;
    }
    if (enemies[i].visible)
    {
      oled.drawBitmap(enemies[i].x, enemies[i].y, enemyBitmap, ENEMY_WIDTH, ENEMY_HEIGHT, WHITE);
      if (enemies[i].x + ENEMY_WIDTH >= furthestEnemy)
      {
        furthestEnemy = enemies[i].x + ENEMY_WIDTH;
      }
    }
  }
}

// Updates/Moves all elements in the scene, except the player, that is stationary
void updateScene()
{
  updateBullets();

  updatePlatforms();

  updateEnemies();

  updateHearts();
}

void setup()
{
  // Hardware setup
  //Wire.begin(SDA_PIN, SCL_PIN); // Emulator
  Serial.begin(9600);

  pinMode(pinButtonUp, INPUT_PULLUP);
  pinMode(pinButtonShoot, INPUT_PULLUP);

  if (!oled.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
  }
  resetGame();
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

    // Score logic
    updateScore();

    // Set up scene
    drawHearts(lives);
    drawScore();
    drawGhost();
    createPlatform();

    // Player logic
    jump();
    jumpPhysics();
    handleFallAndLives();
    shoot();
    enemyKill();

    // Enemy logic
    spawnEnemy();
    enemyCollision();

    //Hearts logic
    spawnHeart();
    heartCollision();

    updateScene();

    prevPinButtonUp = digitalRead(pinButtonUp);
    prevPinButtonShoot = digitalRead(pinButtonShoot);
  }
  oled.display();
}
