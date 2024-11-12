#include <Arduino.h>
#include "config.h"

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>


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

void WiFiTask(void *pvParameters);
void MQTTReconnectTask(void *pvParameters);
void MQTTSubscribeTask(void *pvParameters);
void MQTTPublishTask(void *pvParameters);

// Helper functions
void mqttCallback(char *topic, byte *payload, unsigned int length);
void splitMessage(String sentence, String &message1, String &message2);
void printToLCD(LiquidCrystal_I2C &lcd, String message);

// WiFi and MQTT
const char *mqtt_server = "test.mosquitto.org";

// MQTT client
const char *client_ID = "ESP32";
WiFiClient espClient;
PubSubClient client(espClient);

// LCD message
const int maxMessages = 10;
const int maxLength = 32; // LCD can display 16 characters per row
QueueHandle_t queue;

// Define a struct to hold the message and display time
typedef struct {
  char message[maxLength + 1];
  int timeMs = 200;
} Message;

void setup() {
  Serial.begin(115200);
  queue = xQueueCreate(maxMessages, sizeof(Message));

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

  // Initialize Wi-Fi connection task
  xTaskCreatePinnedToCore(WiFiTask, "WiFiTask", 4000, NULL, 1, NULL, 0);

  // Set up MQTT server
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttCallback);

  // Let FreeRTOS take over
  vTaskDelete(NULL);
}

void loop() {
  // Empty. Tasks are running independently.
}

// Task functions
void displayTask(void *pvParameters) {
  client.subscribe("game/players/1/components/lcd");
  Message msg;
  while (true) {
    // Display message on LCD
    if (uxQueueMessagesWaiting(queue) > 0) {
      xQueueReceive(queue, &msg, 0);
      Serial.println("Message received from queue: " + String(msg.message) + " for " + String(msg.timeMs) + "ms");
      printToLCD(lcd, msg.message);
    }
    else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Waiting msg...");
    }
    vTaskDelay(pdMS_TO_TICKS(msg.timeMs)); // Small delay to avoid task overflow
  }
}

void buzzTask(void *pvParameters) {
  while (true) {
    // tone(BUZZ_PIN, 500);
    // vTaskDelay(pdMS_TO_TICKS(2000));

    // noTone(BUZZ_PIN);
    // vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void ledTask(void *pvParameters) {
  while (true) {
    digitalWrite(LED_PIN, HIGH);
    vTaskDelay(pdMS_TO_TICKS(1000));

    digitalWrite(LED_PIN, LOW);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// Wi-Fi Task: Handles connection to Wi-Fi
void WiFiTask(void *pvParameters) {
  Message msg;
  while (WiFi.status() != WL_CONNECTED) {
    sprintf(msg.message, "Connecting to Wi-Fi...");
    Serial.print(msg.message);
    xQueueSend(queue, &msg, portMAX_DELAY);
    WiFi.begin(ssid, password);
    vTaskDelay(pdMS_TO_TICKS(2000));
  }

  sprintf(msg.message, "Connected to Wi-Fi!");
  Serial.println(msg.message);
  xQueueSend(queue, &msg, portMAX_DELAY);

  // Start MQTT tasks only after Wi-Fi is connected
  xTaskCreatePinnedToCore(MQTTReconnectTask, "MQTTReconnectTask", 4000, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(MQTTSubscribeTask, "MQTTSubscribeTask", 4000, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(MQTTPublishTask, "MQTTPublishTask", 4000, NULL, 1, NULL, 1);

  // Delete WiFiTask to free resources
  vTaskDelete(NULL); // NULL deletes the current task
}

// MQTT Reconnect Task: Reconnects to the MQTT broker if disconnected
void MQTTReconnectTask(void *pvParameters) {
  Message msg;
  while (true) {
    if (!client.connected()) {
      sprintf(msg.message, "Attempting MQTT connection...");
      Serial.println(msg.message);
      xQueueSend(queue, &msg, portMAX_DELAY);
      if (client.connect(client_ID)) {
        sprintf(msg.message, "Connected to MQTT!");
        Serial.println(msg.message);
        xQueueSend(queue, &msg, portMAX_DELAY);
        client.subscribe("test/server");
      }
      else {
        sprintf(msg.message, "Failed to connect");
        Serial.println(String(msg.message) + ". Retrying in 5 seconds");
        xQueueSend(queue, &msg, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(5000)); // Retry after 5 seconds
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1000)); // Check connection status every second
  }
}

// MQTT Subscribe Task: Waits for incoming messages
void MQTTSubscribeTask(void *pvParameters) {
  while (true) {
    if (client.connected()) client.loop();
    vTaskDelay(pdMS_TO_TICKS(10)); // Small delay to avoid task overflow
  }
}

// MQTT Publish Task: Publishes a message every 10 seconds
void MQTTPublishTask(void *pvParameters) {
  while (true) {
    if (client.connected()) {
      client.publish("game/players/1/connection", "1");
      Serial.println("Message published to game/players/1/connection: 1");
    }
    vTaskDelay(pdMS_TO_TICKS(2000)); // Publish every 10 seconds
  }
}

// Callback function to handle incoming messages
void mqttCallback(char *topic, byte *payload, unsigned int length) {
  Serial.println("Message arrived on topic: " + String(topic));

  // Write the message to the message buffer to display on the LCD
  Message msg;
  if (length > maxLength)
    memcpy(msg.message, "Message too long!", length = 17);
  else
    memcpy(msg.message, payload, length);
  msg.message[length] = '\0'; // Null-terminate the message
  msg.timeMs = 2000; // Display for 2 seconds
  xQueueSend(queue, &msg, portMAX_DELAY);
}

void splitMessage(String sentence, String &message1, String &message2) {
  // Maximum characters per row
  const int maxChars = 16;

  // Clear the output messages
  message1 = "";
  message2 = "";

  // Check if the sentence fits in the first row
  if (sentence.length() <= maxChars) {
    message1 = sentence; // Fits entirely in the first row
  }
  else {
    // Find the last space within the maximum character limit
    int splitIndex = sentence.lastIndexOf(' ', maxChars);
    if (splitIndex == -1) {
      // If there are no spaces, just take the first part
      message1 = sentence.substring(0, maxChars);
      message2 = sentence.substring(maxChars);
    }
    else {
      // Split at the last space found within the limit
      message1 = sentence.substring(0, splitIndex);
      message2 = sentence.substring(splitIndex + 1); // Skip the space
    }
  }
}

void printToLCD(LiquidCrystal_I2C &lcd, String msg) {
  String message1;
  String message2;
  splitMessage(msg, message1, message2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(message1);
  lcd.setCursor(0, 1);
  lcd.print(message2);
}