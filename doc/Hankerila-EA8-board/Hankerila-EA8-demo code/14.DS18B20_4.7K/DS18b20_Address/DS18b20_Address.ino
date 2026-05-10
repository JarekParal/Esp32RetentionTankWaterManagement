#include <OneWire.h>
#include <DallasTemperature.h>  

// GPIO pin connected to the DS18B20 sensor(s)
#define ONE_WIRE_BUS 15

// Create OneWire instance to communicate with any OneWire device
OneWire oneWire(ONE_WIRE_BUS);

// Create DallasTemperature instance
DallasTemperature sensors(&oneWire);

// Array to store device address
DeviceAddress deviceAddress;

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println("DS18B20 Address Scanner");
  Serial.println("Searching for DS18B20 sensors connected to GPIO15...");
  
  // Initialize DallasTemperature library
  sensors.begin();
}

void loop() {
  // Counter for detected sensors
  int sensorCount = 0;
  
  // Reset search
  oneWire.reset_search();
  
  Serial.println("\nStarting sensor scan...");
  
  // Search for all connected DS18B20 sensors
  while (oneWire.search(deviceAddress)) {
    sensorCount++;
    
    Serial.print("Sensor #");
    Serial.print(sensorCount);
    Serial.print(" Address: ");
    
    // Print first 7 bytes of ROM address
    for (uint8_t i = 0; i < 7; i++) {
      Serial.print("0x");
      if (deviceAddress[i] < 0x10) Serial.print("0");
      Serial.print(deviceAddress[i], HEX);
      Serial.print(", ");
    }
    
    // Print CRC byte (last byte)
    Serial.print("0x");
    if (deviceAddress[7] < 0x10) Serial.print("0");
    Serial.println(deviceAddress[7], HEX);
    
    // Check if device is a DS18B20
    if (deviceAddress[0] != 0x28) {
      Serial.println("Warning: Detected device may not be a DS18B20");
    }
  }
  
  if (sensorCount == 0) {
    Serial.println("No DS18B20 sensors found!");
    Serial.println("Check connections and power.");
  } else {
    Serial.print("Found ");
    Serial.print(sensorCount);
    Serial.println(" sensor(s) in total");
  }
  
  // Wait before scanning again
  delay(5000);
}
