#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>


const char* ssid     = "-";
const char* password = "";
const char* udpAddress = "1";
const int udpPort = 5005;

const int yellowPin = 2;
const int greenPin = 1;
const int redPin = 0;

WiFiUDP udp;
BLEScan* pBLEScan;

//states
const int STATE_NO_WIFI = 0;
const int STATE_WIFI_NO_GUI = 1;
const int STATE_WIFI_AND_GUI = 2;
const int STATE_SCANNING = 3;
const int STATE_LOCATING = 4;


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
  
}


void loop() {
  //if ack receieved from gui
  udp.beginPacket(udpAddress, udpPort);
  udp.print("transmitting");
  udp.endPacket();
  if (udp.parsePacket()) {
    String message = udp.readString();
    if (message == "ACK") {
      digitalWrite(yellowPin, LOW); //green means wifi and gui connection
      digitalWrite(greenPin, HIGH);
      Serial.println("ACK received from GUI, starting BLE scan...");
    }
    if (message == "GET_DEVICES") {
      //todo
    }
  }
  else{
    digitalWrite(yellowPin, HIGH); //yellow means wifi but no gui connection
    digitalWrite(greenPin, LOW);
    }
  

}
