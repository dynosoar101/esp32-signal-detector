#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>


const char* ssid     = "SpectrumSetup-5305";
const char* password = "suchnation402";
const char* udpAddress = "192.168.1.144";
const int udpPort = 5005;

const int yellowPin = 2;
const int greenPin = 1;
const int redPin = 0;

WiFiUDP udp;
BLEScan* pBLEScan;

//timeout for connecting to wifi
const unsigned long wifiTimeout = 10000; // 10 seconds



void setup() {
  pinMode(yellowPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(redPin, OUTPUT);
  Serial.begin(115200);
  digitalWrite(redPin, HIGH);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    //timeout if it takes too long to connect
    if (millis() > wifiTimeout) {
      Serial.println("Failed to connect to WiFi. Restarting...");
      ESP.restart();
    }
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  digitalWrite(redPin, LOW);
  digitalWrite(yellowPin, HIGH); //yellow means wifi but no gui connection

  udp.begin(udpPort);

  // Initialize BLE
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setActiveScan(true);
  
  // FIXED: Set window equal to interval for a 100% continuous reception scan cycle
  pBLEScan->setInterval(200);
  pBLEScan->setWindow(200);
}

void loop() {
  // 1. Regular tracking loop data dispatch
  udp.beginPacket(udpAddress, udpPort);
  if(rand() % 2 == 0) {
    udp.print("GREEN");
  } else {
    udp.print("RED");  
  }
  udp.endPacket();

}