#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

const char* ssid     = "-";
const char* password = "";
const char* udpAddress = "1";
const int udpPort = 5005;

// Pins updated per color request
const int yellowPin = 2;
const int bluePin = 1;   // Changed name from greenPin to bluePin
const int redPin = 0;

WiFiUDP udp;

// States
const int STATE_NO_WIFI = 0;
const int STATE_WIFI_NO_GUI = 1;
const int STATE_WIFI_AND_GUI = 2;
const int STATE_SCANNING = 3;
const int STATE_LOCATING = 4;

int currentState = STATE_NO_WIFI;
const unsigned long wifiTimeout = 10000; // 10 seconds

void handleLEDs(int state);

void setup() {
  pinMode(yellowPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(redPin, OUTPUT);
  Serial.begin(115200);
  handleLEDs(currentState);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() > wifiTimeout) {
      Serial.println("Failed to connect to WiFi. Restarting...");
      ESP.restart();
    }
    Serial.println("Connecting to WiFi...");
    delay(500);
  }
  
  currentState = STATE_WIFI_NO_GUI;
  Serial.println("Connected to WiFi");
  handleLEDs(currentState);

  udp.begin(udpPort);
}

void loop() {
  // Emergency block if WiFi gets disconnected
  if (WiFi.status() != WL_CONNECTED && currentState != STATE_NO_WIFI) {
    currentState = STATE_NO_WIFI;
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
      if (millis() - startTime > wifiTimeout) {
        Serial.println("Failed to connect to WiFi. Restarting...");
        ESP.restart();
      }
      delay(500);
    }
    currentState = STATE_WIFI_NO_GUI;
  }

  // Track the last time we heard from the GUI to handle disconnects
  static unsigned long lastGuiHeartbeat = 0;
  const unsigned long guiTimeoutWindow = 3000; // 3 seconds without a packet = GUI closed

  switch(currentState) {
    case STATE_NO_WIFI: {
      handleLEDs(currentState);
      break;
    }

    case STATE_WIFI_NO_GUI: {
      handleLEDs(currentState);
      
      // Ping Pygame to look for it
      udp.beginPacket(udpAddress, udpPort);
      udp.print("YELLOW"); 
      udp.endPacket();
      
      unsigned long startCheck = millis();
      while (millis() - startCheck < 1000) { 
        if (udp.parsePacket()) {
          String message = udp.readString();
          if (message == "GUI_ACTIVE") { 
            currentState = STATE_WIFI_AND_GUI;
            lastGuiHeartbeat = millis(); // Initialize heartbeat timer
            Serial.println("Handshake confirmed! Moving to STATE_WIFI_AND_GUI.");
            break;
          }
        }
        delay(20);
      }
      break;
    }

    case STATE_WIFI_AND_GUI: {
      handleLEDs(currentState);
      
      // Check if the GUI has stopped talking to us (GUI closed)
      if (millis() - lastGuiHeartbeat > guiTimeoutWindow) {
        Serial.println("GUI connection lost (Timeout). Switching back to Yellow.");
        currentState = STATE_WIFI_NO_GUI;
        break;
      }

      if (udp.parsePacket()) {
        String message = udp.readString();
        
        // Refresh our heartbeat timer because we received valid traffic
        lastGuiHeartbeat = millis();

        if (message == "GET_DEVICES") {
          currentState = STATE_SCANNING;
          Serial.println("GUI requested a scan event...");
        }
        // Even if Pygame sends an address check or state sync message, 
        // it keeps our connection alive here.
      }
      break;
    }

    case STATE_SCANNING: {
      // TODO: Handle your custom BLE scanning loop implementation here!
      Serial.println("Inside custom scanning todo block.");
      
      // Reset the heartbeat right after finishing your work so it doesn't immediately timeout
      lastGuiHeartbeat = millis(); 
      currentState = STATE_WIFI_AND_GUI;
      break;
    }
  }
}

void handleLEDs(int state) {
  switch(state) {
    case STATE_NO_WIFI:
      digitalWrite(redPin, HIGH);  
      digitalWrite(yellowPin, LOW);  
      digitalWrite(bluePin, LOW);
      break;
    case STATE_WIFI_NO_GUI:
      digitalWrite(redPin, LOW);    
      digitalWrite(yellowPin, HIGH); 
      digitalWrite(bluePin, LOW);
      break;
    case STATE_WIFI_AND_GUI:
      digitalWrite(redPin, LOW);    
      digitalWrite(yellowPin, LOW);  
      digitalWrite(bluePin, HIGH); // Blue Indicator Active
      break;
    default:
      break;
  }
}