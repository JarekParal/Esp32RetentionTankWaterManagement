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

// ROM addresses of the 10 temperature sensors
DeviceAddress sensorsAddress[10] = {
  {0x28, 0x48, 0x1E, 0x6C, 0x00, 0x00, 0x00, 0x96},  // Sensor 1
  {0x28, 0x34, 0x03, 0x6B, 0x00, 0x00, 0x00, 0x65},  // Sensor 2
  {0x28, 0xB4, 0xF5, 0x6A, 0x00, 0x00, 0x00, 0x3A},  // Sensor 3
  {0x28, 0xF4, 0xD2, 0x69, 0x00, 0x00, 0x00, 0x6E},  // Sensor 4
  {0x28, 0xAC, 0x35, 0x6B, 0x00, 0x00, 0x00, 0x6D},  // Sensor 5
  {0x28, 0x42, 0x83, 0x6A, 0x00, 0x00, 0x00, 0x0B},  // Sensor 6
  {0x28, 0x52, 0x58, 0x6B, 0x00, 0x00, 0x00, 0xF1},  // Sensor 7
  {0x28, 0x81, 0xB9, 0x6A, 0x00, 0x00, 0x00, 0xF4},  // Sensor 8
  {0x28, 0x11, 0xC1, 0x6B, 0x00, 0x00, 0x00, 0x99},  // Sensor 9
  {0x28, 0x8D, 0x21, 0x6B, 0x00, 0x00, 0x00, 0x8F}   // Sensor 10
};

// Arrays to store temperature data
float temperatures[10];         // Current temperature readings
float prevTemperatures[10];     // Previous temperature readings
float lastReportedTemp[10];     // Last temperature reported to web client
bool temperatureChanged[10] = {false};  // Track temperature changes
bool anyChangeDetected = false; // Flag to track if any sensor changed

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println("ESP32 DS18B20 Temperature Monitoring System Started");
  
  // Initialize temperature sensor library
  sensors.begin();
  
  // Set sensor resolution to 12-bit (highest precision)
  for (int i = 0; i < 10; i++) {
    sensors.setResolution(sensorsAddress[i], 10);
    prevTemperatures[i] = -999; // Initialize with invalid value
    lastReportedTemp[i] = -999; // Initialize reported values
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
  delay(500);
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
  delay(500); // Reduced sampling interval for faster response
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
    html += "body { font-family: Arial; margin:0px auto; padding: 10px; }";
    html += ".container { display: flex; flex-wrap: wrap; justify-content: space-between; max-width: 1200px; margin: 0 auto; }";
    html += ".column { width: 48%; min-width: 300px; }";
    html += ".sensor { margin: 10px; padding: 15px; border-radius: 10px; box-shadow: 0 4px 8px 0 rgba(0,0,0,0.2); }";
    html += ".changed { background-color: #ffcccc; }";
    html += "h1 { color: #444; text-align: center; }";
    html += ".temp { font-size: 24px; font-weight: bold; }";
    html += "@media (max-width: 768px) { .column { width: 100%; } }";
    html += "</style>";
    html += "</head><body>";
    html += "<h1>ESP32 DS18B20 Temperature Monitor</h1>";
    
    // Left column with sensors 1-5
    html += "<div class='container'>";
    html += "<div class='column'>";
    html += "<h2>Sensors 1-5</h2>";
    for (int i = 0; i < 5; i++) {
      String sensorClass = temperatureChanged[i] ? "sensor changed" : "sensor";
      html += "<div class='" + sensorClass + "'>";
      html += "<h3>Sensor " + String(i + 1) + "(Celsius)</h3>";
      html += "<p class='temp'>" + String(temperatures[i], 2) + "</p>";
      html += "</div>";
    }
    html += "</div>";
    
    // Right column with sensors 6-10
    html += "<div class='column'>";
    html += "<h2>Sensors 6-10</h2>";
    for (int i = 5; i < 10; i++) {
      String sensorClass = temperatureChanged[i] ? "sensor changed" : "sensor";
      html += "<div class='" + sensorClass + "'>";
      html += "<h3>Sensor " + String(i + 1) + "(Celsius)</h3>";
      html += "<p class='temp'>" + String(temperatures[i], 2) + "</p>";
      html += "</div>";
    }
    html += "</div>";
    html += "</div>";
    
    // Add JavaScript for real-time updates
    html += "<script>";
    html += "function updateData() {";
    html += "  fetch('/data')";
    html += "    .then(response => response.json())";
    html += "    .then(data => {";
    html += "      if (data.length > 0) {"; // Only update when there's new data
    html += "        data.forEach((sensor, index) => {";
    html += "          const sensorIndex = sensor.id - 1;";
    html += "          const columnIndex = sensorIndex < 5 ? 0 : 1;";
    html += "          const sensorDivIndex = sensorIndex % 5;";
    html += "          const sensorDiv = document.querySelectorAll('.column')[columnIndex].querySelectorAll('.sensor')[sensorDivIndex];";
    html += "          const tempElement = sensorDiv.querySelector('.temp');";
    html += "          tempElement.textContent = sensor.temp;";
    html += "          if (sensor.changed) {";
    html += "            sensorDiv.classList.add('changed');";
    html += "            setTimeout(() => sensorDiv.classList.remove('changed'), 2000);";
    html += "          }";
    html += "        });";
    html += "      }";
    html += "    });";
    html += "}";
    html += "setInterval(updateData, 1000);"; // Check for updates every second
    html += "updateData(); // Initial update on page load";
    html += "</script>";
    html += "</body></html>";
    
    // Send HTML response
    server.send(200, "text/html", html);
  });
  
  // Data API route handler - Only send data when changes detected
  server.on("/data", HTTP_GET, []() {
    if (!anyChangeDetected) {
      server.send(200, "application/json", "[]"); // Return empty array if no changes
      return;
    }
    
    String json = "[";
    // Generate JSON data for all sensors that changed
    bool first = true;
    for (int i = 0; i < 10; i++) {
      if (temperatureChanged[i]) {
        if (!first) json += ",";
        json += "{\"id\":" + String(i + 1) + ",\"temp\":" + String(temperatures[i], 2) + ",\"changed\":" + String(temperatureChanged[i] ? "true" : "false") + "}";
        first = false;
        
        // Update last reported temperature for this sensor
        lastReportedTemp[i] = temperatures[i];
      }
    }
    json += "]";
    
    // Reset change flag after sending data
    anyChangeDetected = false;
    
    // Send JSON response
    server.send(200, "application/json", json);
  });
}

// Check for temperature changes between readings
void checkTemperatureChanges() {
  anyChangeDetected = false;
  
  for (int i = 0; i < 10; i++) {
    // Only compare valid temperature values
    if (temperatures[i] != -999 && prevTemperatures[i] != -999) {
      float change = abs(temperatures[i] - prevTemperatures[i]);
      
      // Detect significant temperature change (threshold lowered to 0.1°C)
      if (change > 0.1) {
        temperatureChanged[i] = true;
        anyChangeDetected = true; // Set global change flag
        Serial.print("Temperature change detected on Sensor ");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(prevTemperatures[i], 2);
        Serial.print(" -> ");
        Serial.println(temperatures[i], 2);
      } else if (lastReportedTemp[i] != -999) {
        // Check cumulative change since last report
        float cumulativeChange = abs(temperatures[i] - lastReportedTemp[i]);
        if (cumulativeChange > 0.1) {
          temperatureChanged[i] = true;
          anyChangeDetected = true;
          Serial.print("Cumulative temperature change detected on Sensor ");
          Serial.print(i + 1);
          Serial.print(": ");
          Serial.print(lastReportedTemp[i], 2);
          Serial.print(" -> ");
          Serial.println(temperatures[i], 2);
        } else {
          temperatureChanged[i] = false;
        }
      } else {
        temperatureChanged[i] = false;
      }
    } else if (temperatures[i] != -999 && prevTemperatures[i] == -999) {
      // First valid reading
      temperatureChanged[i] = true;
      anyChangeDetected = true;
    } else {
      temperatureChanged[i] = false;
    }
    
    // Update previous temperature value
    prevTemperatures[i] = temperatures[i];
  }
}

// Check sensor connection status at startup
void checkSensorsConnection() {
  Serial.println("Checking sensor connections:");
  
  for (int i = 0; i < 10; i++) {
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
  for (int i = 0; i < 10; i++) {
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
  
  for (int i = 0; i < 10; i++) {
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
