#include <Arduino.h>
#include <RotaryEncoder.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "RTClib.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SDA_PIN 21
#define SCL_PIN 22
#define BUTTON_PIN 2  // Replace with the actual pin connected to the button
#define A0 32
#define A1 35
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
RTC_DS3231 rtc;
RotaryEncoder encoder(A0, A1, RotaryEncoder::LatchMode::FOUR3);

const char *menu[] = {"Time", "Date", "Temperature", "Settings", "About"};
#define MENU_ANZ 5

int selectedMenu = 0;
long pos = 0;
bool buttonPressed = false;


void handleButtonPress();

void setup()
{
  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }

  if (!rtc.begin())
  {
    Serial.println(F("Couldn't find RTC"));
    while (1)
      ;
  }

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  delay(10);

  display.clearDisplay();
  display.display();
}

void loop()
{
  encoder.tick();
  int newPos = encoder.getPosition();
  int buttonState = digitalRead(BUTTON_PIN);

  if (buttonState == LOW && !buttonPressed)
  {
    handleButtonPress();
    buttonPressed = true;
  }
  else if (buttonState == HIGH)
  {
    buttonPressed = false;
  }

  if (pos != newPos)
  {
    pos = newPos;
    selectedMenu = pos % MENU_ANZ;
    if (selectedMenu < 0)
    {
      selectedMenu += MENU_ANZ;
    }

    displayMenu();
  }
}

void handleButtonPress()
{
  display.clearDisplay();

  if (selectedMenu == 0) // Time
  {
    DateTime now = rtc.now();

    display.setTextSize(2);
    display.setCursor(0, 10);
    display.print("Time: ");
    display.print(now.hour());
    display.print(":");
    display.println(now.minute());
  }
  else if (selectedMenu == 1) // Date
  {
    DateTime now = rtc.now();

    display.setTextSize(2);
    display.setCursor(0, 10);
    display.print("Date: ");
    display.print(now.day());
    display.print("/");
    display.print(now.month());
    display.print("/");
    display.println(now.year());
  }
  else if (selectedMenu == 2) // Temperature (requires the sensor to be connected)
  {
    float temperature = rtc.getTemperature();

    display.setTextSize(2);
    display.setCursor(0, 10);
    display.print("Temperature: ");
    display.print(temperature);
    display.println(" C");
  }
  else
  {
    // Placeholder for additional menu options
    display.setTextSize(2);
    display.setCursor(0, 10);
    display.print("Option ");
    display.println(selectedMenu + 1);
  }

  display.display();
  delay(2000); // Display for 2 seconds before returning to the menu
}

void displayMenu()
{
  display.clearDisplay();

  for (int i = 0; i < MENU_ANZ; i++)
  {
    if (i == selectedMenu)
    {
      int frameSize = 2;
      int xPos = 0;
      int yPos = i * 10;
      int width = SCREEN_WIDTH;
      int height = 10;

      display.drawRect(xPos, yPos, width, height, SSD1306_WHITE);
    }

    display.setTextSize(1);
    display.setCursor(2, i * 10 + 2);
    display.println(menu[i]);
  }

  display.display();
}



void displayMenu()
{
  display.clearDisplay();

  for (int i = 0; i < MENU_ANZ; i++)
  {
    if (i == selectedMenu)
    {
      int frameSize = 2;
      int xPos = 0;
      int yPos = i * 10;
      int width = SCREEN_WIDTH;
      int height = 10;

      display.drawRect(xPos, yPos, width, height, SSD1306_WHITE);
    }

    display.setCursor(2, i * 10 + 2);
    display.println(menu[i]);
  }

  display.display();
}
