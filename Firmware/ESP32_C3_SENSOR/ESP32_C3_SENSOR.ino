#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <Adafruit_MLX90640.h>

const char* ssid = "PRO-THERMAL-CAM";
const char* password = "masterofespressif";
const char* udpAddress = "192.168.4.1";

#define I2C_SDA 8
#define I2C_SCL 9
#define BAT_PIN 0 

Adafruit_MLX90640 mlx;
WiFiUDP udp;

struct __attribute__((packed)) ThermalPacket {
  float minTemp;
  float maxTemp;
  uint32_t frameId;
  uint8_t battery;
  int8_t rssi;
  uint8_t pixels[768]; 
} packet;

float frameBuffer[768];
uint32_t frameCounter = 0;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(ssid, password);
  
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(1000000); 

  if (!mlx.begin(MLX90640_I2CADDR_DEFAULT, &Wire)) {
    while(1) delay(100);
  }
  mlx.setMode(MLX90640_CHESS);
  mlx.setRefreshRate(MLX90640_16_HZ);

  while (WiFi.status() != WL_CONNECTED) delay(100);
  udp.begin(12345);
}

void loop() {
  if (mlx.getFrame(frameBuffer) == 0) {
    float minT = 1000, maxT = -1000;
    for (int i = 0; i < 768; i++) {
      if (isnan(frameBuffer[i])) frameBuffer[i] = 25.0;
      minT = min(minT, frameBuffer[i]);
      maxT = max(maxT, frameBuffer[i]);
    }
    
    packet.minTemp = minT;
    packet.maxTemp = maxT;
    packet.frameId = frameCounter++;
    packet.battery = constrain(map(analogReadMilliVolts(BAT_PIN) * 2, 3300, 4150, 0, 100), 0, 100);
    packet.rssi = (int8_t)WiFi.RSSI();

    float range = (maxT - minT > 0.1) ? (maxT - minT) : 0.1;
    for (int i = 0; i < 768; i++) {
      packet.pixels[i] = (uint8_t)((frameBuffer[i] - minT) / range * 255.0);
    }

    udp.beginPacket(udpAddress, 4444);
    udp.write((const uint8_t*)&packet, sizeof(packet));
    udp.endPacket();
  }
}