#include <OneWire.h>
#include <DallasTemperature.h>

// 定义连接 DS18B20 传感器的 GPIO 引脚
#define ONE_WIRE_BUS 15  

// 创建 OneWire 通信实例
OneWire oneWire(ONE_WIRE_BUS); 
// 创建 DallasTemperature 传感器实例，传入 OneWire 引用
DallasTemperature sensors(&oneWire); 

// 10 个传感器的 ROM 地址
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

// 存储温度读数的数组，对应 10 个传感器
float temperatures[10];  

void setup() {
  // 初始化串口通信，波特率 115200
  Serial.begin(115200); 
  Serial.println("ESP32 DS18B20 Temperature Monitoring System Started");
  
  // 初始化温度传感器库
  sensors.begin(); 
  
  // 将每个传感器的分辨率设置为 12 位（最高精度）
  for (int i = 0; i < 10; i++) { 
    sensors.setResolution(sensorsAddress[i], 12);
  }
  
  // 检查所有传感器的连接状态
  checkSensorsConnection(); 
  
  // 延迟 2 秒，准备开始读取温度
  delay(2000); 
}

void loop() {
  // 读取所有 10 个传感器的温度
  readAllSensors(); 
  // 将所有温度读数打印到串口监视器
  printAllTemperatures(); 
  // 每次读取间隔 2 秒
  delay(2000); 
}

// 检查所有传感器的连接状态函数
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

// 读取所有传感器温度的函数
void readAllSensors() {
  // 触发所有传感器开始温度转换
  sensors.requestTemperatures(); 
  
  for (int i = 0; i < 10; i++) { 
    temperatures[i] = sensors.getTempC(sensorsAddress[i]);
    
    // 检查温度读取是否成功
    if (temperatures[i] == DEVICE_DISCONNECTED_C) { 
      Serial.print("Error: Could not read sensor ");
      Serial.println(i + 1);
      temperatures[i] = -999; // 标记为错误值
    }
  }
}

// 将所有温度读数打印到串口监视器的函数
void printAllTemperatures() {
  Serial.println("Current Temperature Readings (°C):");
  
  for (int i = 0; i < 10; i++) { 
    Serial.print("Sensor ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(temperatures[i], 2); // 保留 2 位小数显示温度
  }
  
  Serial.println("------------------------");
}