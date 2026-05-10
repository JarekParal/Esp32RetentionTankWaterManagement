#include <OneWire.h>
#include <DallasTemperature.h>

// Define the GPIO pin connected to DS18B20 sensors
#define ONE_WIRE_BUS 15

// Create OneWire instance
OneWire oneWire(ONE_WIRE_BUS);

// Create DallasTemperature instance with OneWire reference
DallasTemperature sensors(&oneWire);

// ROM addresses of the 5 sensors
DeviceAddress sensorsAddress[5] = {
  {0x28, 0x48, 0x1E, 0x6C, 0x00, 0x00, 0x00, 0x96},  // Sensor 1
  {0x28, 0x34, 0x03, 0x6B, 0x00, 0x00, 0x00, 0x65},  // Sensor 2
  {0x28, 0xAC, 0x35, 0x6B, 0x00, 0x00, 0x00, 0x6D},  // Sensor 3
  {0x28, 0x42, 0x83, 0x6A, 0x00, 0x00, 0x00, 0x0B},  // Sensor 4
  {0x28, 0x52, 0x58, 0x6B, 0x00, 0x00, 0x00, 0xF1}   // Sensor 5
};

// Array to store temperature readings
float temperatures[5];

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println("ESP32 DS18B20 Temperature Monitoring System Started");
  
  // Initialize temperature sensors
  sensors.begin();
  
  // Set resolution to 12 bits (highest precision)
  for (int i = 0; i < 5; i++) {
    sensors.setResolution(sensorsAddress[i], 12);
  }
  
  // Check connection status of all sensors
  checkSensorsConnection();
  
  // Wait 2 seconds before starting temperature readings
  delay(2000);
}

void loop() {
  // Read temperatures from all sensors
  readAllSensors();
  
  // Print all temperature readings
  printAllTemperatures();
  
  // Delay for 2 seconds
  delay(2000);
}

// Check connection status of all sensors
void checkSensorsConnection() {
  Serial.println("Checking sensor connections:");
  
  for (int i = 0; i < 5; i++) {
    bool present = sensors.isConnected(sensorsAddress[i]);
    if (present) {
      Serial.print("Sensor ");
      Serial.print(i + 1);
      Serial.println(" connected successfully");
    } else {
      Serial.print("Warning: Sensor ");
      Serial.print(i + 1);
      Serial.println(" not connected!");
    }
  }
  Serial.println("------------------------");
}

// Read temperatures from all sensors
void readAllSensors() {
  // Request all sensors to start temperature conversion
  sensors.requestTemperatures();
  
  // Read temperature from each sensor
  for (int i = 0; i < 5; i++) {
    temperatures[i] = sensors.getTempC(sensorsAddress[i]);
    
    // Check if read was successful
    if (temperatures[i] == DEVICE_DISCONNECTED_C) {
      Serial.print("Error: Could not read sensor ");
      Serial.println(i + 1);
      temperatures[i] = -999; // Mark as error value
    }
  }
}

// Print all temperature readings to serial monitor
void printAllTemperatures() {
  Serial.println("Current Temperature Readings (°C):");
  
  for (int i = 0; i < 5; i++) {
    Serial.print("Sensor ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(temperatures[i], 2); // Display with 2 decimal places
  }
  
  Serial.println("------------------------");
}
