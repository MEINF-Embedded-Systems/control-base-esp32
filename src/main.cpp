#include <Arduino.h>

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Pin definitions
const int SDA_PIN = 32;
const int SCL_PIN = 33;
const int BUZZ_PIN = 18;
const int BUTTON_PIN = 16;
const int LED_PIN = 23;

// LCD object
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Task handles
TaskHandle_t displayTaskHandle = NULL;
TaskHandle_t buzzTaskHandle = NULL;
TaskHandle_t ledTaskHandle = NULL;

// Task function declarations
void displayTask(void *pvParameters);
void buzzTask(void *pvParameters);
void ledTask(void *pvParameters);

void setup()
{
  Serial.begin(115200);

  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.init();
  lcd.backlight();
  lcd.clear();

  pinMode(BUZZ_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  // Create tasks
  xTaskCreate(displayTask, "Display Task", 2048, NULL, 1, &displayTaskHandle);
  xTaskCreate(buzzTask, "Buzz Task", 2048, NULL, 1, &buzzTaskHandle);
  xTaskCreate(ledTask, "LED Task", 2048, NULL, 1, &ledTaskHandle);
}

void loop()
{
  // Empty. Tasks are running independently.
}

// Task functions
void displayTask(void *pvParameters)
{
  while (true)
  {
    lcd.setCursor(0, 0);
    lcd.print("HELLO");
    vTaskDelay(1000);

    lcd.clear();
    vTaskDelay(1000);
  }
}

void buzzTask(void *pvParameters)
{
  while (true)
  {
    tone(BUZZ_PIN, 500);
    vTaskDelay(100);

    noTone(BUZZ_PIN);
    vTaskDelay(500);
  }
}

void ledTask(void *pvParameters)
{
  while (true)
  {
    digitalWrite(LED_PIN, HIGH);
    vTaskDelay(1000);

    digitalWrite(LED_PIN, LOW);
    vTaskDelay(1000);
  }
}