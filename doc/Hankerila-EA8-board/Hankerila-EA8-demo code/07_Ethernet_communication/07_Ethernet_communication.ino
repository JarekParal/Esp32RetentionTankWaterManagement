//hankerila official store----www.aliexpress.com/store/1103673574
#include <ETH.h> 
#include <WiFiUdp.h> 

// Define the Ethernet address
#define ETH_ADDR        0  // Define the Ethernet power pin
#define ETH_POWER_PIN  -1  // Define the Ethernet MDC pin
#define ETH_MDC_PIN    23  // Define the Ethernet MDIO pin
#define ETH_MDIO_PIN   18  // Define the Ethernet type
#define ETH_TYPE       ETH_PHY_LAN8720 // Define the Ethernet clock mode
#define ETH_CLK_MODE   ETH_CLOCK_GPIO17_OUT


WiFiUDP Udp;                     

unsigned int localUdpPort = 4196; // Unsigned integer for local UDP port
IPAddress local_ip(192, 168, 50, 200); // IP address for local 
IPAddress gateway(192, 168, 50, 1);// IP address for gateway
IPAddress subnet(255, 255, 255, 0);// IP address for subnet
IPAddress dns(192, 168, 50, 1);// IP address for DNS

void setup()
{
  
  Serial.begin(115200); // Start serial communication at 115200 baud rate
  Serial.println();
   

  ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);    // Initialize the Ethernet
  if (ETH.config(local_ip, gateway, subnet, dns, dns) == false) {   // If Ethernet configuration fails
   Serial.println("LAN8720 Configuration failed."); // Print configuration failed message
  }else{// Else print configuration success message
    Serial.println("LAN8720 Configuration success.");}
  Serial.println("Connected");// Print connected message
  Serial.print("IP Address:"); // Print the local IP address
  Serial.println(ETH.localIP());
  Udp.begin(localUdpPort);   // Start the UDP
}

void loop()
{
  // Get the size of the received packet
  int packetSize = Udp.parsePacket(); 
  // If there is a packet received
  if (packetSize)                     
  {
    char buf[packetSize];  // Create a buffer with the packet size
    Udp.read(buf, packetSize); // Read the packet into the buffer
    Serial.println();  // Print a new line
    Serial.print("Received: "); // Print the received message
    Serial.println(buf);
    Serial.print("From IP: ");  // Print the IP address from which the packet was received
    Serial.println(Udp.remoteIP());
    Serial.print("From Port: ");// Print the port from which the packet was received
    Serial.println(Udp.remotePort());

    
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort()); // Begin to prepare a packet to send back
    Udp.print("Received: ");    // Print the received message in the sending packet
    Udp.write((const uint8_t*)buf, packetSize); // Write the buffer data to the sending packet
    Udp.endPacket();           // End the sending packet  
  }
}  