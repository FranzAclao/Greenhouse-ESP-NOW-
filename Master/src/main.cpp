#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ESPAsyncWebServer.h>
#include "pageindex.h" // Include the HTML file
#include <esp_wifi.h>

// Network Credentials
const char* wifi_network_ssid = "josip";  // Wi-Fi network SSID
const char* wifi_network_password = "12345678"; // Wi-Fi network password

// Access Point Configuration
const char* soft_ap_ssid = "ESP32_WS";  // AP SSID
const char* soft_ap_password = "helloesp32WS"; // AP Password
IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// LED Pins
#define LED1_PIN 13
#define LED2_PIN 18

// ESP-NOW Communication
uint8_t slave1MAC[] = {0x88, 0x13, 0xBF, 0x0C, 0x42, 0x94}; // slave1 MAC: LDR slave
uint8_t slave2MAC[] = {0xCC, 0x7B, 0x5C, 0x35, 0x48, 0xFC}; // "2slave" MAC: DHT slave
uint8_t slave3MAC[] = {0xAC, 0x15, 0x18, 0xD4, 0xA6, 0xD4}; // slave3 MAC: Soil and Watering

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Data Structure for ESP-NOW
typedef struct struct_message1 {
  int ldrValue;
  char lightStatus[20];
} struct_message1;

typedef struct struct_message2 {
  float temperature;
  char fanStatus[20];
} struct_message2;

typedef struct struct_message3 {
  int soilMoistureValue;
  char soilStatus[10];    // Soil status: Dry or Moist
  char pumpStatus[20];    // Pump status: Watering or Off
  int waterLevelValue;
  char refillStatus[20];  // Refill status: Refilling or Full
  unsigned long remainingCooldown; // Add remaining cooldown to the structure
} struct_message3;

struct_message1 receivedDataSlave1; // Data received from slave 1 (LDR slave)
struct_message2 receivedDataSlave2; // Data received from 2slave (DHT slave)
struct_message3 receivedDataSlave3; // Data received from slave 3 (Soil and Watering)


// Get Wi-Fi channel for the specified SSID
int32_t get_wifi_channel(const char* ssid) {
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i) {
    if (String(ssid) == WiFi.SSID(i)) {
      return WiFi.channel(i);
    }
  }
  return 0;
}

// Set Wi-Fi channel for ESP32
void scan_and_set_wifi_channel() {
  Serial.printf("\nScanning for SSID: %s\n", wifi_network_ssid);
  int master_channel = get_wifi_channel(wifi_network_ssid);

  if (master_channel == 0) {
    Serial.println("SSID not found!");
  } else {
    Serial.printf("SSID found. Channel: %d\n", master_channel);

    if (WiFi.channel() != master_channel) {
      esp_wifi_set_promiscuous(true);
      esp_wifi_set_channel(master_channel, WIFI_SECOND_CHAN_NONE);
      esp_wifi_set_promiscuous(false);
    }
  }
  Serial.printf("Current Wi-Fi channel: %d\n", WiFi.channel());
}

// Unified ESP-NOW Receive Callback
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  Serial.print("Data received from: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", mac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();

  // Identify the sender based on the MAC address
  if (memcmp(mac, slave1MAC, 6) == 0) {
    // Data from Slave 1 (LDR slave)
    memcpy(&receivedDataSlave1, incomingData, sizeof(receivedDataSlave1));
    Serial.printf("LDR Value: %d, Light Status: %s\n", receivedDataSlave1.ldrValue, receivedDataSlave1.lightStatus);
  } else if (memcmp(mac, slave2MAC, 6) == 0) {
    // Data from 2slave (DHT slave)
    memcpy(&receivedDataSlave2, incomingData, sizeof(receivedDataSlave2));
    Serial.printf("Temperature: %.2f, Fan Status: %s\n", receivedDataSlave2.temperature, receivedDataSlave2.fanStatus);
  } else if (memcmp(mac, slave3MAC, 6) == 0) {
    // Data from Slave 3 (Soil and Watering)
    memcpy(&receivedDataSlave3, incomingData, sizeof(receivedDataSlave3));

    // Adjust the display format for Slave 3 (Soil and Watering) data
    String soilStatus = (receivedDataSlave3.soilMoistureValue > 1900) ? "Dry" : "Moist";
    String refillStatus = String(receivedDataSlave3.refillStatus);
    String pumpStatus = String(receivedDataSlave3.pumpStatus);
    unsigned long remainingCooldown = receivedDataSlave3.remainingCooldown; // Get remaining cooldown

    // Update the display format
    Serial.printf("Water Level: %d\n", receivedDataSlave3.waterLevelValue);
    Serial.printf("Refill Status: %s\n", receivedDataSlave3.refillStatus);
    Serial.printf("Soil Status: %s\n", receivedDataSlave3.soilStatus);
    Serial.printf("Pump Status: %s\n", receivedDataSlave3.pumpStatus);
    

    // Only display the remaining cooldown if it's not 0 (i.e., soil is dry and recently watered)
    if (remainingCooldown > 0) {
      Serial.printf("Remaining Cooldown: %lu ms\n", remainingCooldown);  // Display remaining cooldown
    } else {
      Serial.println("No cooldown, soil is moist.");
    }

  } else {
    Serial.println("Unknown MAC address");
  }
}


// Add ESP-NOW Peers
void addESPNowPeers() {
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));

  // Add Slave 1
  memcpy(peerInfo.peer_addr, slave1MAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add Slave 1");
  }

  // Add 2slave (DHT slave)
  memcpy(peerInfo.peer_addr, slave2MAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add 2slave");
  }

  // Add Slave 3 (Soil and Watering)
  memcpy(peerInfo.peer_addr, slave3MAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add Slave 3");
  }
}

// Setup Function
void setup() {
  Serial.begin(115200);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);

  // Set Wi-Fi mode to AP+STA
  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(WIFI_PS_NONE);
  WiFi.channel(1);  // Set initial channel

  // Configure Access Point
  WiFi.softAP(soft_ap_ssid, soft_ap_password);
  delay(1000);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Connect to Wi-Fi network (STA)
  WiFi.begin(wifi_network_ssid, wifi_network_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi!");
  Serial.print("STA IP Address: ");
  Serial.println(WiFi.localIP());

  // Set Wi-Fi channel based on the master Wi-Fi network
  scan_and_set_wifi_channel();

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register Unified ESP-NOW Receive Callback
  esp_now_register_recv_cb(OnDataRecv);

  // Add peers
  addESPNowPeers();

  // Setup Web Server
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    String soilStatus;
    String waterForPlant;
    String waterContainer = String(receivedDataSlave3.refillStatus);  // Water container status
    String cooldownMessage = "";

    if (receivedDataSlave3.soilMoistureValue > 1900) {  // Soil is dry
      soilStatus = "Dry";
      if (receivedDataSlave3.pumpStatus == "Watering") {
        waterForPlant = "Soil is dry. Watering the plant...";
      } else {
        waterForPlant = "Soil is dry, but watering is on hold until cooldown interval expires.";
        cooldownMessage = "Remaining cooldown: " + String(receivedDataSlave3.remainingCooldown / 1000) + " seconds";
      }
    } else {  // Soil is moist
      soilStatus = "Moist";
      waterForPlant = "Soil is moist. No watering needed.";
      cooldownMessage = "";  // No cooldown message needed
    }

    // Build the status string
    String status = "Slave1_Light_Status: " + String(receivedDataSlave1.lightStatus) + ", ";
    status += "Slave2_Temperature: " + String(receivedDataSlave2.temperature) + ", ";
    status += "Slave2_Fan_Status: " + String(receivedDataSlave2.fanStatus) + ", ";
    status += "Slave3_Water_Level: " + String(receivedDataSlave3.waterLevelValue) + ", ";
    status += "Slave3_Water_Container: " + waterContainer + ", ";
    status += "Slave3_Soil_Status: " + soilStatus + ", ";
    status += "Slave3_Water_For_Plant: " + waterForPlant;

    if (!cooldownMessage.isEmpty()) {
      status += ", " + cooldownMessage;  // Append cooldown message if applicable
    }

    request->send(200, "text/plain", status); // Send formatted status
  });

  // Route to serve the HTML page with updated Slave 3 data
server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
  String soilStatus;
  String waterForPlant;  // This will be used for slave3PumpStatus
  String cooldownMessage = "";

  if (receivedDataSlave3.soilMoistureValue > 1900) {  // Soil is dry
    soilStatus = "Dry";
    if (receivedDataSlave3.pumpStatus == "Watering") {
      waterForPlant = "Watering";  // Set the pump status to "Watering"
    } else {
      waterForPlant = "Waiting for cooldown";  // Set the pump status to "Waiting for cooldown"
      cooldownMessage = "Remaining cooldown: " + String(receivedDataSlave3.remainingCooldown / 1000) + " seconds";
    }
  } else {  // Soil is moist
    soilStatus = "Moist";
    waterForPlant = "No watering needed";  // Set the pump status to indicate no watering needed
    cooldownMessage = "";  // No cooldown message needed
  }

  // Generate the HTML page
  String html = PAGEINDEX;
  html.replace("%STATUS%", "Slave 1: " + String(receivedDataSlave1.lightStatus) + ", Slave 2: " + String(receivedDataSlave2.fanStatus) + ", Slave 3: " + waterForPlant);
  html.replace("%SOIL_STATUS%", soilStatus);
  html.replace("%WATER_LEVEL%", String(receivedDataSlave3.waterLevelValue));
  html.replace("%COOLDOWN%", cooldownMessage);  // Add the cooldown message to the page

  // Pass the updated waterForPlant status into the placeholder
  html.replace("%WATER_FOR_PLANT%", waterForPlant);

  request->send(200, "text/html", html);  // Send the HTML page with updated content
});



  // Start server
  server.begin();
}

// Loop Function
void loop() {
  // Blink LEDs to show the system is running
  digitalWrite(LED1_PIN, HIGH);
  delay(500);
  digitalWrite(LED1_PIN, LOW);
  delay(500);
  digitalWrite(LED2_PIN, HIGH);
  delay(500);
  digitalWrite(LED2_PIN, LOW);
  delay(500);
}