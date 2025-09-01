#include <Arduino.h>
#include "WiFi.h"
#include <SPI.h>
#include <Wire.h>
#include <LoRa.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <PubSubClient.h>

// WiFi config
#define WIFI_NETWORK "Starlink Network"
#define WIFI_PASSWORD "caniconnect"
#define WIFI_TIMEOUT_MS 20000

// Define LoRa module connections
#define LORA_SCK 18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_SS 5
#define LORA_RST 16
#define LORA_DIO0 4

// OLED display config
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


// NTP server settings
const char* ntpServer = "asia.pool.ntp.org";
const long gmtOffset_sec = 19800;  // GMT+5:30 for Sri Lanka
const int daylightOffset_sec = 0;  // No daylight saving in Sri Lanka

const char* mqtt_server = "test.mosquitto.org";

int sensorValues[6];
String dateTime;

void displayMessage(String message);
void parseMessage(String message);
void connectToWiFi();
void reconnectBroker();
void publishMQTT();
void callback(char* topic, byte* payload, unsigned int length);
String getDateTime();

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(9600);
  while (!Serial);

  connectToWiFi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // Initialize NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  String dateTime = getDateTime();
  Serial.println(dateTime);

  // Start I2C display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // 0x3C is common I2C address
    Serial.println("OLED init failed");
    while (1);
  }

  // Display setup
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  // Start LoRa SPI
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed. Check wiring.");
    while (1);
  }

  Serial.println("LoRa Receiver Ready");

  display.println("LoRa Receiver Ready");
  display.display();
  delay(500);
  display.clearDisplay();
  display.display();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
  } else {
    Serial.println("WiFi not connected. Retrying...");
    connectToWiFi();
  }

  if (!client.connected()) {
    reconnectBroker();
  }
  client.loop();

  // Send REQ
  LoRa.beginPacket();
  LoRa.print("RQ");
  LoRa.endPacket();
  Serial.println("Sent request");

  // Wait for response (timeout: 2s)
  unsigned long start = millis();
  bool received = false;

   while (millis() - start < 3000) {
    int packetSize = LoRa.parsePacket();

    if (packetSize) {
      String message = LoRa.readString();

      Serial.print("Received: ");
      Serial.print(message);
      Serial.print(" | RSSI: ");
      Serial.println(LoRa.packetRssi());

      displayMessage(message);
      publishMQTT();
      received = true;
      break;
    }
  }

  if (!received) {
    Serial.println("No response received");
  }

  delay(10000);
}

void publishMQTT(){
  String testMessage = "Hello from AgroSense!";
  client.publish("agrosense_f5d65f65sd56/test", testMessage.c_str());

  client.publish("agrosense_f5d65f65sd56/remote/date_time", dateTime.c_str());
  client.publish("agrosense_f5d65f65sd56/remote/light", String(sensorValues[0]).c_str());
  client.publish("agrosense_f5d65f65sd56/remote/humidity", String(sensorValues[1]).c_str());
  client.publish("agrosense_f5d65f65sd56/remote/soil_moisture", String(sensorValues[2]).c_str());
  client.publish("agrosense_f5d65f65sd56/remote/rain", String(sensorValues[3]).c_str());
  client.publish("agrosense_f5d65f65sd56/remote/temperature", String(sensorValues[4]).c_str());
  client.publish("agrosense_f5d65f65sd56/remote/valve_status", String(sensorValues[5]).c_str());
}


void displayMessage(String message) {
  dateTime = getDateTime();
  Serial.print("Current DateTime: ");
  Serial.println(dateTime);

  parseMessage(message);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(dateTime);
  display.setCursor(0, 20);
  display.print("Light level: ");
  display.print(sensorValues[0]);
  display.println(" lux");
  display.print("Humidity: ");
  display.print(sensorValues[1]);
  display.println(" %");
  display.print("Soil moist: ");
  display.print(sensorValues[2]);
  display.println(" %");
  display.print("Rainfall: ");
  display.print(sensorValues[3]);
  display.println(" %");
  display.print("Temperature: ");
  display.print(sensorValues[4]);
  display.println(" C");
  display.display();
}

// Parse the incoming message and extract sensor values
void parseMessage(String message) {
  int sensorCount = 0;
  int index = 0;

  while (message.length() > 0 && index < 6) {
    int commaIndex = message.indexOf(',');
    if (commaIndex == -1) {
      sensorValues[index++] = message.toInt();
      break;
    } else {
      sensorValues[index++] = message.substring(0, commaIndex).toInt();
      message = message.substring(commaIndex + 1);
    }
  }
}

// Connect to WiFi network
void connectToWiFi(){
  Serial.print("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_NETWORK,WIFI_PASSWORD);

  unsigned long startAttemptTime = millis();

  while(WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT_MS){
    Serial.print(".");
    delay(100);
  }

  if(WiFi.status() != WL_CONNECTED){
    Serial.println(" Failed");
  }else{
    Serial.println("Connected!");
    Serial.println(WiFi.localIP());
  }
}

// Get the current date and time from NTP server
String getDateTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return String("Failed to obtain time");
  }

  // Format the DateTime as yyyy-mm-ddTHH:mm:ss
  char dateTimeString[30];
  sprintf(dateTimeString, "%04d-%02d-%02dT%02d:%02d:%02d", 
          timeinfo.tm_year + 1900,    // Year
          timeinfo.tm_mon + 1,        // Month
          timeinfo.tm_mday,           // Day
          timeinfo.tm_hour,           // Hour
          timeinfo.tm_min,            // Minutes
          timeinfo.tm_sec
          );

  return String(dateTimeString);
}

void reconnectBroker() {
  while (!client.connected()) {
    if (client.connect("ESP32Client-618736176371783612876317")) {
      Serial.println("Connected to MQTT Broker!");
      client.subscribe("agrosense_f5d65f65sd56/remote/command/valve");
      client.subscribe("agrosense_f5d65f65sd56/remote/config/soil_threshold");
      client.subscribe("agrosense_f5d65f65sd56/main/config/soil_threshold");
      client.subscribe("agrosense_f5d65f65sd56/remote/system/mode");
    } else {
      Serial.print("Failed to connect to MQTT Broker.");
      Serial.print(" State: ");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

void sendLoRaCommand(String command) {
  LoRa.beginPacket();
  LoRa.print(command);
  LoRa.endPacket();
  Serial.print("Sent command: ");
  Serial.println(command);
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  Serial.print("Message arrived on topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  for (int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.println(msg);
  if (String(topic) == "agrosense_f5d65f65sd56/remote/command/valve") {
    if (msg == "ON") {
      Serial.println("Valve ON command received");
      sendLoRaCommand("V_ON");
    } else if (msg == "OFF") {
      Serial.println("Valve OFF command received");
      sendLoRaCommand("V_OFF");
    }
  } else if (String(topic) == "agrosense_f5d65f65sd56/remote/config/soil_threshold") {
    Serial.print("Soil threshold config: ");
    Serial.println(msg);
    sendLoRaCommand("SM-" + msg);  // Send soil threshold command to LoRa
    // Handle soil threshold configuration
  } else if (String(topic) == "agrosense_f5d65f65sd56/main/config/soil_threshold") {
    Serial.print("Main soil threshold config: ");
    Serial.println(msg);
    // Handle main soil threshold configuration
  } else if (String(topic) == "agrosense_f5d65f65sd56/remote/system/mode") {
    Serial.print("System mode: ");
    Serial.println(msg);
    // Handle system mode change
  }
}