#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <DHT.h>
#include <Servo.h>

int SOIL_MOISTURE_THRESHOLD = 30;
int RAIN_DROP_THRESHOLD = 30;
int LIGHT_INTENSITY_UPPER_THRESHOLD = 75;
int LIGHT_INTENSITY_LOWER_THRESHOLD = 30;

#define SS 53
#define RST 11
#define DIO0 10

// Sensor Pins
#define ldrPin A0
#define soilMoisturePin A2
#define rainDropPin A3
#define DHTPIN 2 
#define DHTTYPE DHT11   // DHT11 sensor
#define SERVO_PIN 3

DHT dht(DHTPIN, DHTTYPE);

void rotateServo(int angle);
void irrigationControl(int soilMoisturePercentage, int rainDropPercentage, int lightIntensity);
void valveON();
void valveOFF();

bool isIrrigationActive = false;
bool isManualOverride = false;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  dht.begin();

  // Setup LoRa pins
  LoRa.setPins(SS, RST, DIO0);

  // Initialize LoRa
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  Serial.println("LoRa Initializing Succeeded!");
}

void loop() {
  int ldrValue = analogRead(ldrPin);
  int soilMoistureValue = analogRead(soilMoisturePin);
  int rainDropValue = analogRead(rainDropPin);

  int humidity = dht.readHumidity();
  int temperature = dht.readTemperature(); // Celsius

  int soilMoisturePercentage = map(soilMoistureValue, 0, 1023, 100, 0);
  int rainDropPercentage = map(rainDropValue, 0, 1023, 100, 0);

  int lightIntensity = map(ldrValue, 0, 1023, 0, 100);

  int packetSize = LoRa.parsePacket();


  if (packetSize) {
    String incoming = "";
    while (LoRa.available()) {
      incoming += (char)LoRa.read();
    }

    Serial.print("Received packet: ");
    Serial.println(incoming);

    if (incoming == "RQ") {
      // Respond with the current sensor readings
      LoRa.beginPacket();
      LoRa.print(lightIntensity);
      LoRa.print(",");
      LoRa.print(humidity);
      LoRa.print(",");
      LoRa.print(soilMoisturePercentage);
      LoRa.print(",");
      LoRa.print(rainDropPercentage);
      LoRa.print(",");
      LoRa.print(temperature);
      LoRa.print(",");
      LoRa.print(isIrrigationActive ? "1" : "0"); // Send irrigation status
      LoRa.endPacket();
      Serial.println("Sent sensor data.");

    } else if(incoming == "V_ON"){
      valveON();
      isManualOverride = true; // Set manual override flag
    } else if(incoming == "V_OFF"){
      valveOFF();
      isManualOverride = true; // Set manual override flag
    } else if(incoming.substring(0,2) == "SM") {
      String newSoilMoistureTH = incoming.substring(3);
      Serial.print("Setting soil moisture threshold to: ");
      Serial.println(newSoilMoistureTH);
      SOIL_MOISTURE_THRESHOLD = newSoilMoistureTH.toInt();
    }
  }

  irrigationControl(soilMoisturePercentage, rainDropPercentage, lightIntensity);
  delay(50);
}

// Irrigation control logic
void irrigationControl(int soilMoisturePercentage, int rainDropPercentage, int lightIntensity) {
  bool irrigationLogic = soilMoisturePercentage < SOIL_MOISTURE_THRESHOLD && rainDropPercentage < RAIN_DROP_THRESHOLD && 
      lightIntensity > LIGHT_INTENSITY_LOWER_THRESHOLD && lightIntensity < LIGHT_INTENSITY_UPPER_THRESHOLD;
  if (!isManualOverride && !isIrrigationActive && irrigationLogic) {
    valveON();
  } else if(!isManualOverride && isIrrigationActive && !irrigationLogic) {
    valveOFF();
  }
  isManualOverride = false; // Reset manual override flag
}

void valveON() {
  Serial.println("Activating irrigation system.");
  rotateServo(-90);
  isIrrigationActive = true;
}

void valveOFF() {
  Serial.println("Deactivating irrigation system.");
  rotateServo(180);
  isIrrigationActive = false;
}

// Rotate the servo motor
void rotateServo(int angle) {
  Servo myServo;
  myServo.attach(SERVO_PIN);
  myServo.write(angle); 
  delay(1000); 
  myServo.detach(); 
}



  // Serial.print(" | RSSI: ");
  // Serial.println(LoRa.packetRssi());

      //   // Print data to Serial Monitor
      // Serial.print("LDR: ");
      // Serial.print(lightIntensity);
      // Serial.print(" | Humidity: ");
      // Serial.print(humidity);
      // Serial.print(" | Soil Moisture: ");
      // Serial.print(soilMoisturePercentage);
      // Serial.print(" | Rain Drop: ");
      // Serial.print(rainDropPercentage);
      // Serial.print(" | Temperature: ");
      // Serial.print(temperature);
      // Serial.println();