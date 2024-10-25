#include <Arduino.h>

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <WiFi.h>
#include <PubSubClient.h>

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

// WiFi and MQTT
const char *ssid = "Redmi Note 12 Pro 5G";
const char *password = "AEB33014";
const char *mqtt_server = "test.mosquitto.org";

// MQTT client
const char *client_ID = "ESP32";
WiFiClient espClient;
PubSubClient client(espClient);

char message[16];

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

  // Initialize Wi-Fi connection task
  xTaskCreatePinnedToCore(WiFiTask, "WiFiTask", 4000, NULL, 1, NULL, 0);

  // Set up MQTT server
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttCallback);
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
    // Display message on LCD
    if (message != NULL)
    {
      lcd.setCursor(0, 0);
      lcd.print(message);
      // remove message from var
      message[0] = '\0';
      for (int i = 1; i < 16; i++)
      {
        message[i] = ' ';
      }
      vTaskDelay(5000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("---");
    }
  }
}

void buzzTask(void *pvParameters)
{
  while (true)
  {
    // tone(BUZZ_PIN, 1000);
    // vTaskDelay(1000);

    // noTone(BUZZ_PIN);
    // vTaskDelay(1000);
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

// Wi-Fi Task: Handles connection to Wi-Fi
void WiFiTask(void *pvParameters)
{
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    vTaskDelay(2000);
  }

  Serial.println("WiFi connected");

  // Start MQTT tasks only after Wi-Fi is connected
  xTaskCreatePinnedToCore(MQTTReconnectTask, "MQTTReconnectTask", 4000, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(MQTTSubscribeTask, "MQTTSubscribeTask", 4000, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(MQTTPublishTask, "MQTTPublishTask", 4000, NULL, 1, NULL, 1);

  // Delete WiFiTask to free resources
  vTaskDelete(NULL); // NULL deletes the current task
}

// MQTT Reconnect Task: Reconnects to the MQTT broker if disconnected
void MQTTReconnectTask(void *pvParameters)
{
  while (true)
  {
    if (!client.connected())
    {
      Serial.println("Attempting MQTT connection...");
      if (client.connect(client_ID))
      {
        Serial.println("MQTT connected");
        client.subscribe("test/server"); // Subscribe to a test topic
      }
      else
      {
        Serial.print("Failed to connect, retrying in 5 seconds...");
        vTaskDelay(5000); // Retry after 5 seconds
      }
    }
    vTaskDelay(1000); // Check connection status every second
  }
}

// MQTT Subscribe Task: Waits for incoming messages
void MQTTSubscribeTask(void *pvParameters)
{
  while (true)
  {
    if (client.connected())
    {
      client.loop();
    }
    vTaskDelay(10); // Small delay to avoid task overflow
  }
}

// MQTT Publish Task: Publishes a message every 10 seconds
void MQTTPublishTask(void *pvParameters)
{
  while (true)
  {
    if (client.connected())
    {
      client.publish("test/topic", "I'm ESP32");
      Serial.println("Message published");
    }
    vTaskDelay(10000); // Publish every 10 seconds
  }
}

// Callback function to handle incoming messages
void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.println(topic);

  // Write the message to the message buffer to display on the LCD
  for (int i = 0; i < 16; i++)
  {
    message[i] = (char)payload[i];
  }

  Serial.print("Message: ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}