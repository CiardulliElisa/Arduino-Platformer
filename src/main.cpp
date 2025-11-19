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

// Ball Settings
int xBallPos = SCREEN_WIDTH / 2;
int yBallPos = SCREEN_HEIGHT / 2;

int xSpeed = 2;
int ySpeed = 2;
int ballRadius = 4;

// Paddle Setting
int paddleWidth = 4;
int paddleHeight = 24;
int xPaddlePos = SCREEN_WIDTH - paddleWidth - 2;
int yPaddlePos = (SCREEN_HEIGHT - paddleHeight) / 2;
int paddleSpeed = 2;

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

  xBallPos += xSpeed;
  yBallPos += ySpeed;

  if (digitalRead(pinButtonUp) == LOW && yPaddlePos > 0)
  {
    yPaddlePos -= paddleSpeed;
  }

  if (digitalRead(pinButtonDown) == LOW && yPaddlePos + paddleHeight < SCREEN_HEIGHT)
  {
    yPaddlePos += paddleSpeed;
  }

  if (xBallPos - ballRadius <= 0)
  {
    xSpeed *= -1;
  }

  if (yBallPos - ballRadius <= 0 || yBallPos + ballRadius >= SCREEN_HEIGHT - 1)
  {
    ySpeed *= -1;
  }

  // Check with paddle
  if (xBallPos + ballRadius >= xPaddlePos)
  {
    if (yBallPos + ballRadius >= yPaddlePos && yBallPos + ballRadius <= yPaddlePos + paddleHeight)
    {
      xSpeed *= -1;
    }
    else
    {
      // Reset
      xBallPos = SCREEN_WIDTH / 2;
      yBallPos = SCREEN_HEIGHT / 2;
      xSpeed = 2;
      ySpeed = 2;
    }
  }

  oled.fillRect(xPaddlePos, yPaddlePos, paddleWidth, paddleHeight, SSD1306_WHITE);
  oled.fillCircle(xBallPos, yBallPos, ballRadius, SSD1306_INVERSE);
  oled.display();
  delay(30);
}