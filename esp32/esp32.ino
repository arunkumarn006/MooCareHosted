#include <WiFi.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <MAX30105.h>
#include <heartRate.h>
#include <spo2_algorithm.h>
#include <HTTPClient.h>

MAX30105 particleSensor;

const byte RATE_SIZE = 4; // Increase this for more precision, but slower updates
byte rates[RATE_SIZE]; // Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; // Time at which the last beat occurred
float beatsPerMinute;
int beatAvg;

int32_t bufferLength; // Buffer length for heart rate and SpO2 calculation
uint32_t irBuffer[100]; // IR LED sensor data
uint32_t redBuffer[100]; // Red LED sensor data
int32_t spo2; // SPO2 value
int8_t validSPO2; // SPO2 validity
int32_t heartRate; // Heart rate value
int8_t validHeartRate; // Heart rate validity

const char* ssid = "Thanay";
const char* password = "1234567890";

const char* host = "192.168.43.246"; // IP address of your Flask server
const int httpPort = 5001; // Port of the Flask server

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");

  Serial.print("Connecting to ");
  Serial.println(host);

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 was not found. Please check wiring/power.");
    while (1);
  }
  particleSensor.setup(); // Configure sensor with default settings

  Serial.println("Place your finger on the sensor to start reading...");

}

void loop() {
  // Read data from MAX30102
  bufferLength = 100; // Sample 100 data points
  
  // Read 100 samples from the sensor
  for (byte i = 0; i < bufferLength; i++) {
    while (!particleSensor.available()) {
      particleSensor.check(); // Check the sensor for new data
    }
    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample(); // Move to the next sample
  }

  // Calculate heart rate and SpO2
  maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

  // Determine cow status
  String cowStatus = "Unknown";
  if (validHeartRate && validSPO2) {
    if (heartRate < 40 || heartRate > 100 || spo2 < 90) {
      cowStatus = "Critical";
    } else {
      cowStatus = "Normal";
    }
  }

  // Print data to Serial Monitor
  Serial.print("HR: "); Serial.print(heartRate);
  Serial.print(", SpO2: "); Serial.print(spo2);
  Serial.print(", Status: "); Serial.println(cowStatus);
  WiFiClient client;

  if (!client.connect(host, httpPort)) {
    Serial.println("Connection failed");
    return;
  }

  String url = "/";
  String postData = "hr=" + String(heartRate) + "&spo2=" + String(spo2) + "&status=" + String(cowStatus);

  client.println("POST " + url + " HTTP/1.1");
  client.println("Host: " + String(host));
  client.println("User-Agent: ESP32");
  client.println("Connection: close");
  client.println("Content-Type: application/x-www-form-urlencoded");
  client.print("Content-Length: ");
  client.println(postData.length());
  client.println();
  client.println(postData);

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("Headers received");
      break;
    }
  }

  String line = client.readStringUntil('\n');
  Serial.println("Reply was:");
  Serial.println("==========");
  Serial.println(line);
  Serial.println("==========");
  Serial.println("Closing connection");
}
