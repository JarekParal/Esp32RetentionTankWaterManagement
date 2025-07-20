/*HANKERILA  HKL-EA8 Ethernet WEB CONTROL RELAYS*/
/*Select the ESP32 version 2.0.17 for the development board. If the latest version is used, the compilation will not pass */
/*set your IP address and input the IP can open the webpage to control the relays*/
#include <ETH.h>
#include <WebServer.h>
#include <PCF8574.h>

constexpr int ULTRASOUND_TRIGER_PIN = 15; // RX on connector
constexpr int ULTRASOUND_ECHO_PIN = 16;   // TX on connector

// Define the Ethernet address
#define ETH_ADDR 0               // Define the Ethernet power pin
#define ETH_POWER_PIN -1         // Define the Ethernet MDC pin
#define ETH_MDC_PIN 23           // Define the Ethernet MDIO pin
#define ETH_MDIO_PIN 18          // Define the Ethernet type
#define ETH_TYPE ETH_PHY_LAN8720 // Define the Ethernet clock mode
#define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT

IPAddress local_ip(uint32_t(0)); // 0 => Use DNS IP address
// IPAddress local_ip(192, 168, 1, 200); // Fixed IP address for local
IPAddress gateway(192, 168, 1, 1);  // IP address for gateway
IPAddress subnet(255, 255, 255, 0); // IP address for subnet
IPAddress dns(192, 168, 1, 1);      // IP address for DNS

WebServer server(80);

PCF8574 pcf8574_re(0x24, 4, 5);
void server_handle_root();
void server_handle_sw();
float get_distance_cm_from_ultrasound_sensor();

void setup()
{
  // Configure pins for ultrasonic sensor
  pinMode(ULTRASOUND_TRIGER_PIN, OUTPUT);
  pinMode(ULTRASOUND_ECHO_PIN, INPUT);

  Serial.begin(115200);
  delay(1000); // Allow time for Serial to initialize

  Serial.println("Starting Ethernet...");
  ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE); // Initialize the Ethernet
  if (ETH.config(local_ip, gateway, subnet, dns, dns) == false)
  {                                                  // If Ethernet configuration fails
    Serial.println("LAN8720 Configuration failed."); // Print configuration failed message
  }
  else
  { // Else print configuration success message
    Serial.println("LAN8720 Configuration success.");
  }

  delay(1000);                     // Wait for the Ethernet DNS initialization
  Serial.println("Connected");     // Print connected message
  Serial.print("ETH IP Address:"); // Print the local IP address
  Serial.println(ETH.localIP());

  for (int i = 0; i < 8; i++)
  {
    pcf8574_re.pinMode(i, OUTPUT);
  }
  // init PCF8574
  pcf8574_re.begin();
  for (int i = 0; i < 8; i++)
  {
    // Set all relays to OFF state (1 = OFF, 0 = ON)
    pcf8574_re.digitalWrite(i, 1);
  }
  // start the web server
  server.on("/", server_handle_root);
  server.on("/SW", server_handle_sw);
  server.begin();
  Serial.println("Web server started");
}

void loop()
{
  server.handleClient();

  // const float distanceCm = get_distance_cm_from_ultrasound_sensor();

  // // Print distance to Serial Monitor
  // Serial.print("Distance (cm): ");
  // Serial.println(distanceCm);
}

void server_handle_root()
{
  String HTML = "<!DOCTYPE html>\
<html>\
<head><meta charset='utf-8'></head>\
<title>ESP32 WEB CONTROL</title>\
<body>\
  <script> var xhttp = new XMLHttpRequest();\
            function sw(arg){\
              xhttp.open('GET','/SW?LED='+ arg ,true);\
              xhttp.send();}\
  </script>\
  <table border='1' width=50% >\
  <tr>\
  <td  colspan='8'  style='background-color:#FFA500;text-align:center' >\
  <h1>MODEL:EA8 relay control </h1>\
  </td>\
  </tr>\
  <tr>\
  <td> <button onmousedown=sw('on1') style='background-color: red; padding:1px 9.66px ; vertical-align=center'> ON1</button>  </td>\
  <td> <button onmousedown=sw('on2') style='background-color: red; padding:1px 9.66px ; vertical-align=center'> ON2</button>  </td>\
  <td> <button onmousedown=sw('on3') style='background-color: red; padding:1px 9.66px ; vertical-align=center'> ON3</button>  </td>\
  <td> <button onmousedown=sw('on4') style='background-color: red; padding:1px 9.66px ; vertical-align=center'> ON4</button>  </td>\
  <td> <button onmousedown=sw('on5') style='background-color: red; padding:1px 9.66px ; vertical-align=center'> ON5</button>  </td>\
  <td> <button onmousedown=sw('on6') style='background-color: red; padding:1px 9.66px ; vertical-align=center'> ON6</button>  </td>\
  <td> <button onmousedown=sw('on7') style='background-color: red; padding:1px 9.66px ; vertical-align=center'> ON7</button>  </td>\
  <td> <button onmousedown=sw('on8') style='background-color: red; padding:1px 9.66px ; vertical-align=center'> ON8</button>  </td>\
   </tr>\
  <tr>\
   <td width:50px text-align:center> <button onmousedown=sw('off1') style='background-color: green'>OFF1</button> </td>\
   <td width:50px text-align:center> <button onmousedown=sw('off2') style='background-color: green'>OFF2</button> </td>\
   <td width:50px text-align:center> <button onmousedown=sw('off3') style='background-color: green'>OFF3</button> </td>\
   <td width:50px text-align:center> <button onmousedown=sw('off4') style='background-color: green'>OFF4</button> </td>\
   <td width:50px text-align:center> <button onmousedown=sw('off5') style='background-color: green'>OFF5</button> </td>\
   <td width:50px text-align:center> <button onmousedown=sw('off6') style='background-color: green'>OFF6</button> </td>\
   <td width:50px text-align:center> <button onmousedown=sw('off7') style='background-color: green'>OFF7</button> </td>\
   <td width:50px text-align:center> <button onmousedown=sw('off8') style='background-color: green'>OFF8</button> </td>\
   </tr>\
  </table>\
  ";
  HTML += "<h2>Ultrasonic Sensor Distance: " + String(get_distance_cm_from_ultrasound_sensor()) + " cm</h2>";
  HTML += "\
  </body>\
  </html>";
  server.send(200, "text/html", HTML);
}

void server_handle_sw()
{
  String state = server.arg("LED");
  for (int i = 0; i < 8; ++i)
  {
    if (state == "on" + String(i + 1))
    {
      pcf8574_re.digitalWrite(i, 0); // Turn ON the relay (0 = ON, 1 = OFF)
    }
    else if (state == "off" + String(i + 1))
    {
      pcf8574_re.digitalWrite(i, 1); // Turn OFF the relay (1 = OFF, 0 = ON)
    }
  }

  server.send(200, "text/html", "LED IS <b>" + state + "</b>.");
}

float get_distance_cm_from_ultrasound_sensor()
{
  // Define sound speed in cm/uS
  constexpr float SOUND_SPEED = 0.034;

  // Clear the trigPin
  digitalWrite(ULTRASOUND_TRIGER_PIN, LOW);
  delayMicroseconds(5);

  // Trigger ultrasonic pulse
  digitalWrite(ULTRASOUND_TRIGER_PIN, HIGH);
  delayMicroseconds(20);
  digitalWrite(ULTRASOUND_TRIGER_PIN, LOW);

  // Measure pulse duration
  long duration = pulseIn(ULTRASOUND_ECHO_PIN, HIGH);

  // Calculate distance in cm
  return (duration * SOUND_SPEED) / 2;
}
