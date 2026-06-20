#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// WiFi credentials
// const char* ssid     = "test_id";
// const char* password = "test_password";
// const char* udpAddress = "xxx.xxx.xxx.xxx"; // Replace with the IP address of your GUI application
// const int udpPort = xxxx;



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

//current state for traversal
int currentState = STATE_NO_WIFI;

//timeout for connecting to wifi
const unsigned long wifiTimeout = 10000; // 10 seconds

void setup() {
  pinMode(yellowPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(redPin, OUTPUT);
  Serial.begin(115200);
  handleLEDs(currentState);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    //timeout if it takes too long to connect
    if (millis() > wifiTimeout) {
      Serial.println("Failed to connect to WiFi. Restarting...");
      ESP.restart();
    }
    Serial.println("Connecting to WiFi...");
  }
  currentState = STATE_WIFI_NO_GUI;
  Serial.println("Connected to WiFi");
  handleLEDs(currentState);

  udp.begin(udpPort);

  // Initialize BLE
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setActiveScan(true);
  
}


void loop() {
  //emergency block if wifi gets disconnected
  if (WiFi.status() != WL_CONNECTED && currentState != STATE_NO_WIFI) {
    currentState = STATE_NO_WIFI;
    //give the esp time to reconnect otherwise timeout and restart
    unsigned long startTime = millis();
    if (millis() - startTime > wifiTimeout) {
        Serial.println("Failed to reconnect to WiFi. Restarting...");
        ESP.restart();
      }
      delay(500);
    
  }
  switch(currentState){
    case STATE_NO_WIFI:
      handleLEDs(currentState);
      //do nothing, wait for wifi to reconnect
      break;

    case STATE_WIFI_NO_GUI:
      handleLEDs(currentState);
      //send a message to the gui to see if it is connected
      udp.beginPacket(udpAddress, udpPort);
      udp.print("transmitting");
      udp.endPacket();
      if (udp.parsePacket()) {
        String message = udp.readString();
        if (message == "ACK") {
          currentState = STATE_WIFI_AND_GUI;
          Serial.println("ACK received from GUI, starting BLE scan...");
        }
      }
      else{
        currentState = STATE_WIFI_NO_GUI;
        Serial.println("No ACK received from GUI, waiting...");
      }
      case STATE_WIFI_AND_GUI:
      handleLEDs(currentState);
      //to do: wait for user input to start scanning
      if (udp.parsePacket()) {
        String message = udp.readString();
        if (message == "GET_DEVICES") {
          currentState = STATE_SCANNING;
          Serial.println("Starting BLE scan...");
        }
      }
    
  }



  // //if ack receieved from gui
  // udp.beginPacket(udpAddress, udpPort);
  // udp.print("transmitting");
  // udp.endPacket();
  // if (udp.parsePacket()) {
  //   String message = udp.readString();
  //   if (message == "ACK") {
  //     digitalWrite(yellowPin, LOW); //green means wifi and gui connection
  //     digitalWrite(greenPin, HIGH);
  //     Serial.println("ACK received from GUI, starting BLE scan...");
  //   }
  //   if (message == "GET_DEVICES") {
  //     //todo
  //   }
  // }
  // else{
  //   digitalWrite(yellowPin, HIGH); //yellow means wifi but no gui connection
  //   digitalWrite(greenPin, LOW);
  //   }
  

}

void handleLEDs(int state){
  switch(state){
    case STATE_NO_WIFI:
      digitalWrite(redPin, HIGH);
      digitalWrite(yellowPin, LOW);
      digitalWrite(greenPin, LOW);
      break;
    case STATE_WIFI_NO_GUI:
      digitalWrite(redPin, LOW);
      digitalWrite(yellowPin, HIGH);
      digitalWrite(greenPin, LOW);
      break;
    case STATE_WIFI_AND_GUI:
      digitalWrite(redPin, LOW);
      digitalWrite(yellowPin, LOW);
      digitalWrite(greenPin, HIGH);
      break;
  //default
    default:
      digitalWrite(redPin, LOW);
      digitalWrite(yellowPin, LOW);
      digitalWrite(greenPin, LOW);
      break;
  }
}