/*HANKERILA  HKL-EA8  WEB CONTROL RELAYS*/
#include <WiFi.h>
#include <WebServer.h>
#include <PCF8574.h>

const char *ssid = "HANKER";        // input your ssid
const char *password = "a12345678"; // input your password

WebServer server(80);

PCF8574 pcf8574_re(0x24, 4, 5);

void setup()
{
  Serial.begin(115200);

  // connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());

  for (int i = 0; i < 8; i++)
  {
    pcf8574_re.pinMode(i, OUTPUT);
  }
  // init PCF8574
  pcf8574_re.begin();
  for (int i = 0; i < 8; i++)
  {
    pcf8574_re.digitalWrite(i, 0);
  }
  // start the web server
  server.on("/", handleRoot);
  server.on("/SW", handleSW);
  server.begin();
  Serial.println("Web server started");
}

void handleRoot()
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
</body>\
</html>";
  server.send(200, "text/html", HTML);
}

void handleSW()
{
  String state = server.arg("LED");
  if (state == "on1")
  {
    pcf8574_re.digitalWrite(0, 1);
  }
  else if (state == "off1")
  {
    pcf8574_re.digitalWrite(0, 0);
  }

  if (state == "on2")
  {
    pcf8574_re.digitalWrite(1, 1);
  }
  else if (state == "off2")
  {
    pcf8574_re.digitalWrite(1, 0);
  }

  if (state == "on3")
  {
    pcf8574_re.digitalWrite(2, 1);
  }
  else if (state == "off3")
  {
    pcf8574_re.digitalWrite(2, 0);
  }

  if (state == "on4")
  {
    pcf8574_re.digitalWrite(3, 1);
  }
  else if (state == "off4")
  {
    pcf8574_re.digitalWrite(3, 0);
  }
  if (state == "on5")
  {
    pcf8574_re.digitalWrite(4, 1);
  }
  else if (state == "off5")
  {
    pcf8574_re.digitalWrite(4, 0);
  }
  if (state == "on6")
  {
    pcf8574_re.digitalWrite(5, 1);
  }
  else if (state == "off6")
  {
    pcf8574_re.digitalWrite(5, 0);
  }
  if (state == "on7")
  {
    pcf8574_re.digitalWrite(6, 1);
  }
  else if (state == "off7")
  {
    pcf8574_re.digitalWrite(6, 0);
  }
  if (state == "on8")
  {
    pcf8574_re.digitalWrite(7, 1);
  }
  else if (state == "off8")
  {
    pcf8574_re.digitalWrite(7, 0);
  }

  server.send(200, "text/html", "LED IS <b>" + state + "</b>.");
}

void loop()
{
  server.handleClient();
}
