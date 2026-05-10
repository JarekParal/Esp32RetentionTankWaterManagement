#include <WiFi.h>

// Define the SSID and password of the WiFi network
const char* ssid = "HANKER";
const char* password = "a12345678";

// Setup function, runs once when the device starts
void setup() {
    // Initialize serial communication at a baud rate of 115200
    Serial.begin(115200);
    // Start the connection to the specified WiFi network
    WiFi.begin(ssid, password);

    // Record the start time of the connection attempt
    unsigned long startAttemptTime = millis();
    // Wait for up to 5 seconds for the WiFi connection to be established
    while (WiFi.status() != WL_CONNECTED && (millis() - startAttemptTime) < 5000) {
        // Wait for half a second
        delay(500);
        // Print a dot to indicate the waiting process
        Serial.print(".");
    }

    // Check if the WiFi connection is successful
    if (WiFi.status() == WL_CONNECTED) {
        // Print a new line
        Serial.println("");
        // Print the connection status
        Serial.println("WiFi connected");
        // Print the label for the IP address
        Serial.println("IP address: ");
        // Print the local IP address of the device
        Serial.println(WiFi.localIP());
    } 
}

// Loop function, runs repeatedly after setup
void loop() {
    // Check if the device is still connected to the WiFi network
    if (WiFi.status() == WL_CONNECTED) {
        // Get the current Received Signal Strength Indicator (RSSI)
        int rssi = WiFi.RSSI();
        // Print the label for the signal strength
        Serial.print("Signal Strength (RSSI): ");
        // Print the actual RSSI value
        Serial.print(rssi);
        // Print the unit of RSSI
        Serial.print(" dBm - ");

        // Evaluate the signal quality based on the RSSI value
        if (rssi >= -50) {
            // If RSSI is greater than or equal to -50 dBm, the signal is excellent
            Serial.println("Signal is excellent");
        } else if (rssi >= -70) {
            // If RSSI is between -50 dBm and -70 dBm, the signal is good
            Serial.println("Signal is good");
        } else if (rssi >= -80) {
            // If RSSI is between -70 dBm and -80 dBm, the signal is fair
            Serial.println("Signal is fair");
        } else {
            // If RSSI is less than -80 dBm, the signal is poor
            Serial.println("Signal is poor");
        }
    }
    // Wait for 2 seconds before the next check
    delay(2000);
}    