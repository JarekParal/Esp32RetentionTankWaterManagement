#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <WebServer.h>

// WiFi configuration
const char* ssid = "HANKER";
const char* password = "a12345678";

// Web server port definition
WebServer server(80);

// GPIO pin connected to DS18B20 sensors
#define ONE_WIRE_BUS 15

// Create OneWire communication instance
OneWire oneWire(ONE_WIRE_BUS);

// Create DallasTemperature sensor instance
DallasTemperature sensors(&oneWire);

// ROM addresses of the 5 temperature sensors
DeviceAddress sensorsAddress[5] = {
  {0x28, 0x48, 0x1E, 0x6C, 0x00, 0x00, 0x00, 0x96},  // Sensor 1
  {0x28, 0x34, 0x03, 0x6B, 0x00, 0x00, 0x00, 0x65},  // Sensor 2
  {0x28, 0xAC, 0x35, 0x6B, 0x00, 0x00, 0x00, 0x6D},  // Sensor 3
  {0x28, 0x42, 0x83, 0x6A, 0x00, 0x00, 0x00, 0x0B},  // Sensor 4
  {0x28, 0x52, 0x58, 0x6B, 0x00, 0x00, 0x00, 0xF1}   // Sensor 5
};

// Arrays to store temperature data
float temperatures[5];         // Current temperature readings
float prevTemperatures[5];      // Previous temperature readings
bool temperatureChanged[5] = {false};  // Track temperature changes

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println("ESP32 DS18B20 Temperature Monitoring System Started");
  
  // Initialize temperature sensor library
  sensors.begin();
  
  // Set sensor resolution to 12-bit (highest precision)
  for (int i = 0; i < 5; i++) {
    sensors.setResolution(sensorsAddress[i], 12);
    prevTemperatures[i] = -999; // Initialize with invalid value
  }
  
  // Check sensor connection status
  checkSensorsConnection();
  
  // Connect to WiFi network
  connectToWiFi();
  
  // Set up web server routes
  setupWebServer();
  
  // Start web server
  server.begin();
  Serial.println("HTTP server started");
  
  // Delay before starting temperature readings
  delay(2000);
}

void loop() {
  // Handle web server client requests
  server.handleClient();
  
  // Read temperature data from all sensors
  readAllSensors();
  
  // Check for temperature changes
  checkTemperatureChanges();
  
  // Print temperature readings to serial monitor
  printAllTemperatures();
  
  // Delay between measurement cycles
  delay(2000);
}

// Connect to WiFi network
void connectToWiFi() {
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nConnected to WiFi network");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// Set up web server routes and handlers
void setupWebServer() {
  // Root route handler
  server.on("/", HTTP_GET, []() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>ESP32 DS18B20 Temperature Monitor</title>";
    html += "<style>";
    html += "body { font-family: Arial; text-align: center; margin:0px auto; padding-top: 30px; }";
    html += ".sensor { margin: 20px; padding: 15px; border-radius: 10px; box-shadow: 0 4px 8px 0 rgba(0,0,0,0.2); }";
    html += ".changed { background-color: #ffcccc; }";
    html += "h1 { color: #444; }";
    html += ".temp { font-size: 24px; font-weight: bold; }";
    html += "</style>";
    html += "</head><body>";
    html += "<h1>ESP32 DS18B20 Temperature Monitor</h1>";
    
    // Generate sensor display elements
    for (int i = 0; i < 5; i++) {
      String sensorClass = temperatureChanged[i] ? "sensor changed" : "sensor";
      html += "<div class='" + sensorClass + "'>";
      html += "<h3>Sensor " + String(i + 1) + "(Celsius)</h3>";
      html += "<p class='temp'>" + String(temperatures[i], 2) + "</p>";
      html += "</div>";
    }
    
    // Add JavaScript for real-time updates
    html += "<script>";
    html += "setInterval(function() {";
    html += "  fetch('/data')";
    html += "    .then(response => response.json())";
    html += "    .then(data => {";
    html += "      data.forEach((sensor, index) => {";
    html += "        const sensorDiv = document.querySelectorAll('.sensor')[index];";
    html += "        const tempElement = sensorDiv.querySelector('.temp');";
    html += "        tempElement.textContent = sensor.temp ;";
    html += "        if (sensor.changed) {";
    html += "          sensorDiv.classList.add('changed');";
    html += "          setTimeout(() => sensorDiv.classList.remove('changed'), 2000);";
    html += "        }";
    html += "      });";
    html += "    });";
    html += "}, 2000);";
    html += "</script>";
    html += "</body></html>";
    
    // Send HTML response
    server.send(200, "text/html", html);
  });
  
  // Data API route handler
  server.on("/data", HTTP_GET, []() {
    String json = "[";
    // Generate JSON data for all sensors
    for (int i = 0; i < 5; i++) {
      json += "{\"id\":" + String(i + 1) + ",\"temp\":" + String(temperatures[i], 2) + ",\"changed\":" + String(temperatureChanged[i] ? "true" : "false") + "}";
      if (i < 4) json += ",";
    }
    json += "]";
    
    // Send JSON response
    server.send(200, "application/json", json);
  });
}

// Check for temperature changes between readings
void checkTemperatureChanges() {
  for (int i = 0; i < 5; i++) {
    // Only compare valid temperature values
    if (temperatures[i] != -999 && prevTemperatures[i] != -999) {
      float change = abs(temperatures[i] - prevTemperatures[i]);
      
      // Detect significant temperature change
      if (change > 0.5) {
        temperatureChanged[i] = true;
        Serial.print("Temperature change detected on Sensor ");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(prevTemperatures[i], 2);
        Serial.print(" -> ");
        Serial.println(temperatures[i], 2);
      } else {
        temperatureChanged[i] = false;
      }
    }
    
    // Update previous temperature value
    prevTemperatures[i] = temperatures[i];
  }
}

// Check sensor connection status at startup
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

// Read temperature values from all sensors
void readAllSensors() {
  // Initiate temperature conversion for all sensors
  sensors.requestTemperatures();
  
  // Read and validate temperature data
  for (int i = 0; i < 5; i++) {
    temperatures[i] = sensors.getTempC(sensorsAddress[i]);
    
    // Handle read errors
    if (temperatures[i] == DEVICE_DISCONNECTED_C) {
      Serial.print("Error: Could not read sensor ");
      Serial.println(i + 1);
      temperatures[i] = -999; // Mark as invalid value
    }
  }
}

// Print temperature readings to serial monitor
void printAllTemperatures() {
  Serial.println("Current Temperature Readings (°C):");
  
  for (int i = 0; i < 5; i++) {
    Serial.print("Sensor ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(temperatures[i], 2);
    
    // Indicate temperature changes
    if (temperatureChanged[i]) {
      Serial.print(" *Changed*");
    }
    
    Serial.println();
  }
  
  Serial.println("------------------------");
}