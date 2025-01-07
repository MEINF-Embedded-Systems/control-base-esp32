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
TaskHandle_t buttonTaskHandle = NULL;
TaskHandle_t WiFiTaskHandle = NULL;
TaskHandle_t MQTTReconnectTaskHandle = NULL;
TaskHandle_t MQTTSubscribeTaskHandle = NULL;
TaskHandle_t MQTTPublishTaskHandle = NULL;

// Task function declarations
void displayTask(void *pvParameters);
void buzzTask(void *pvParameters);
void ledTask(void *pvParameters);
void buttonTask(void *pvParameters);

void WiFiTask(void *pvParameters);
void MQTTReconnectTask(void *pvParameters);
void MQTTSubscribeTask(void *pvParameters);
void MQTTPublishTask(void *pvParameters);

// Helper functions
void mqttCallback(char *topic, byte *payload, unsigned int length);
void splitMessage(String sentence, String &message1, String &message2);
void printToLCD(LiquidCrystal_I2C &lcd, String msgTop, String msgDown);

// Handlers
void handleTurnMesssage(const char *message);
void handleLCDMessage(const char *message);
void handleBuzzerMessage(const char *message);

String player_id = "1";

// WiFi and MQTT
const char *mqtt_server = "192.168.173.140";
unsigned int mqtt_port = 1883;

// MQTT client
const char *client_ID = "ESP32-1";
WiFiClient espClient;
PubSubClient client(espClient);

// LCD message
const int maxMessages = 10;
const int maxLength = 16; // LCD can display 16 characters per row
QueueHandle_t queue;

// Buzzer message
const int maxTones = 20;
QueueHandle_t buzzerQueue;

// Define a struct to hold the message and display time
typedef struct {
  char top[maxLength + 1];
  char down[maxLength + 1];
  int timeMs = 500;
} LCDMessage;

typedef struct {
  int tones[20];
  int duration[20];
} BuzzerMessage;

// Game topics
String lcd_topic = "game/players/" + player_id + "/components/lcd";
String connection_topic = "game/players/" + player_id + "/connection";
String button_topic = "game/players/" + player_id + "/components/button";
String turn_topic = "game/players/" + player_id + "/turn";
String buzzer_topic = "game/players/" + player_id + "/components/buzzer";

// Meeple topics
String meeple_led_topic = "meeple/" + player_id + "/led";

void setup() {
  Serial.begin(115200);
  queue = xQueueCreate(maxMessages, sizeof(LCDMessage));
  buzzerQueue = xQueueCreate(maxTones, sizeof(BuzzerMessage));

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
  xTaskCreate(buttonTask, "Button Task", 2048, NULL, 1, &buttonTaskHandle);

  // Initialize Wi-Fi connection task
  xTaskCreatePinnedToCore(WiFiTask, "WiFiTask", 4000, NULL, 1, &WiFiTaskHandle, 0);

  // Set up MQTT server
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);

  // Let FreeRTOS take over
  vTaskDelete(NULL);
}

void loop() {
  // Empty. Tasks are running independently.
}

// Task functions
void displayTask(void *pvParameters) {
  LCDMessage msg;
  while (true) {
    // Display message on LCD
    if (uxQueueMessagesWaiting(queue) > 0) {
      xQueueReceive(queue, &msg, 0);
      printToLCD(lcd, msg.top, msg.down);
      if (msg.timeMs > 0) {
        vTaskDelay(pdMS_TO_TICKS(msg.timeMs));
        lcd.clear();
      }
    }
  }
}

void buzzTask(void *pvParameters) {
  BuzzerMessage msg;
  while (true) {
    // tone(BUZZ_PIN, 500);
    // vTaskDelay(pdMS_TO_TICKS(2000));

    // noTone(BUZZ_PIN);
    // vTaskDelay(pdMS_TO_TICKS(2000));

    if (uxQueueMessagesWaiting(buzzerQueue) > 0) {
      xQueueReceive(buzzerQueue, &msg, 0);
      for (int i = 0; i < maxTones; i++) {
        if (msg.tones[i] == 0) break;
        tone(BUZZ_PIN, msg.tones[i]);
        vTaskDelay(pdMS_TO_TICKS(msg.duration[i]) * 1.3);
        noTone(BUZZ_PIN);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(100));
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

void buttonTask(void *pvParameters) {
  const int SHORT_PRESS_TIME = 500; // 500 milliseconds
  // Variables will change:
  int lastState = -1;  // the previous state from the input pin
  int currentState;     // the current reading from the input pin
  unsigned long pressedTime = 0;
  unsigned long releasedTime = 0;
  while (true) {
    if (client.connected()) {
      currentState = digitalRead(BUTTON_PIN);
      if (lastState == HIGH && currentState == LOW)        // button is pressed
        pressedTime = millis();
      else if (lastState == LOW && currentState == HIGH) { // button is released
        releasedTime = millis();

        long pressDuration = releasedTime - pressedTime;

        JsonDocument doc;
        doc["type"] = pressDuration < SHORT_PRESS_TIME ? "short" : "long";
        doc["duration"] = pressDuration;
        char buffer[256];
        serializeJson(doc, buffer);
        client.publish(button_topic.c_str(), buffer);
      }

      // save the the last state
      lastState = currentState;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// Wi-Fi Task: Handles connection to Wi-Fi
void WiFiTask(void *pvParameters) {
  LCDMessage msg;
  sprintf(msg.top, "Connecting to Wi-Fi...");
  Serial.println(msg.top);
  xQueueSend(queue, &msg, portMAX_DELAY);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    vTaskDelay(pdMS_TO_TICKS(2000));
  }

  sprintf(msg.top, "Connected to Wi-Fi!");
  Serial.println(msg.top);
  xQueueSend(queue, &msg, portMAX_DELAY);

  // Start MQTT tasks only after Wi-Fi is connected
  xTaskCreate(MQTTReconnectTask, "MQTTReconnectTask", 4000, NULL, 1, &MQTTReconnectTaskHandle);
  xTaskCreate(MQTTSubscribeTask, "MQTTSubscribeTask", 4000, NULL, 1, &MQTTSubscribeTaskHandle);
  xTaskCreate(MQTTPublishTask, "MQTTPublishTask", 4000, NULL, 1, &MQTTPublishTaskHandle);

  // Delete WiFiTask to free resources
  if (WiFiTaskHandle != NULL) vTaskDelete(WiFiTaskHandle); // NULL deletes the current task
}

// MQTT Reconnect Task: Reconnects to the MQTT broker if disconnected
void MQTTReconnectTask(void *pvParameters) {
  LCDMessage msg;
  while (true) {
    if (!client.connected()) {
      sprintf(msg.top, "Attempting MQTT connection...");
      Serial.println(msg.top);
      xQueueSend(queue, &msg, portMAX_DELAY);
      if (client.connect(client_ID)) {
        sprintf(msg.top, "Connected to MQTT!");
        Serial.println(msg.top);
        xQueueSend(queue, &msg, portMAX_DELAY);

        client.subscribe(lcd_topic.c_str());
        client.subscribe(buzzer_topic.c_str());
      }
      else {
        sprintf(msg.top, "Failed to connect");
        Serial.println(String(msg.top) + ". Retrying in 5 seconds");
        xQueueSend(queue, &msg, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(5000)); // Retry after 5 seconds
      }
    }
    vTaskDelay(pdMS_TO_TICKS(2000)); // Check connection status every second
  }
}

// MQTT Subscribe Task: Waits for incoming messages
void MQTTSubscribeTask(void *pvParameters) {
  client.subscribe(meeple_led_topic.c_str());
  while (true) {
    if (client.connected()) {
      client.loop();
    }
    vTaskDelay(pdMS_TO_TICKS(10)); // Small delay to avoid task overflow
  }
}

// MQTT Publish Task: Publishes a message every 10 seconds
void MQTTPublishTask(void *pvParameters) {
  while (true) {
    if (client.connected()) {
      client.publish(connection_topic.c_str(), "1");
      // Serial.println("LCDMessage published to " + connection_topic + ": 1");
    }
    vTaskDelay(pdMS_TO_TICKS(2000)); // Publish every 10 seconds
  }
}

// Callback function to handle incoming messages
void mqttCallback(char *topic, byte *payload, unsigned int length) {
  Serial.println("Something arrived on topic: " + String(topic));

  char buffer[256];
  memcpy(buffer, payload, length);
  buffer[length] = '\0';

  if (String(topic) == turn_topic) {
    handleTurnMesssage(buffer);
  }
  else if (String(topic) == lcd_topic) {
    handleLCDMessage(buffer);
  }
  else if (String(topic) == buzzer_topic) {
    handleBuzzerMessage(buffer);
  }

}

void handleTurnMesssage(const char *buffer) {
  client.publish(meeple_led_topic.c_str(), buffer);
}

void handleLCDMessage(const char *buffer) {
  LCDMessage msg;
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, buffer);

  if (error) {
    Serial.print("deserializeJson() returned ");
    Serial.println(error.c_str());
    return;
  }

  Serial.println("LCDMessage: " + String(buffer));

  const char *top = doc["top"];
  const char *down = doc["down"];
  int timeMs = doc["time"];

  sprintf(msg.top, "%s", top);
  sprintf(msg.down, "%s", down);
  msg.timeMs = timeMs;

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

void printToLCD(LiquidCrystal_I2C &lcd, String msgTop = "", String msgDown = "") {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(msgTop);
  lcd.setCursor(0, 1);
  lcd.print(msgDown);
}

void handleBuzzerMessage(const char *buffer) {
  BuzzerMessage msg;
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, buffer);

  if (error) {
    Serial.print("deserializeJson() returned ");
    Serial.println(error.c_str());
    return;
  }

  Serial.println("BuzzerMessage: " + String(buffer));

  JsonArray tones = doc["tones"];
  JsonArray duration = doc["duration"];

  for (int i = 0; i < tones.size(); i++) {
    msg.tones[i] = tones[i];
    msg.duration[i] = duration[i];
  }

  xQueueSend(buzzerQueue, &msg, portMAX_DELAY);
}
