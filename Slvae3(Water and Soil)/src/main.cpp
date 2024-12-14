#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// Define GPIO pins
#define SOIL_SENSOR_PIN 34
#define WATER_LEVEL_SENSOR_PIN 35
#define RELAY_PLANT_WATERING_PIN 25
#define RELAY_REFILL_PIN 26

// Define thresholds
#define SOIL_MOISTURE_THRESHOLD 1900 // Adjust as per sensor calibration
#define WATER_LEVEL_LOW_THRESHOLD 1300   // Adjust as per sensor calibration
#define WATER_LEVEL_FULL_THRESHOLD 1550 // Adjust as per sensor calibration

// Timing variables
unsigned long previousWateringTime = 0;
unsigned long wateringInterval = 10000;  // Adjust to your actual desired interval
bool isRefilling = false; // Tracks if refill pump is active

// Master's MAC Address (Replace with actual MAC)
uint8_t masterMAC[] = {0xFC, 0xE8, 0xC0, 0x74, 0x50, 0x14}; // Replace with master MAC

// Get Wi-Fi channel for the specified SSID
const char* wifi_network_ssid = "josip";  // Wi-Fi network SSID
const char* wifi_network_password = "12345678"; // Wi-Fi network password


// Structure for ESP-NOW data
typedef struct struct_message {
  int soilMoistureValue;
  char soilStatus[10];    // Soil status: Dry or Moist
  char pumpStatus[20];    // Pump status: Watering or Off
  int waterLevelValue;
  char refillStatus[20];  // Refill status: Refilling or Full
  unsigned long remainingCooldown; // Add remaining cooldown to the structure
} struct_message;

struct_message myData;

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
  int slave_channel = get_wifi_channel(wifi_network_ssid);

  if (slave_channel == 0) {
    Serial.println("SSID not found!");
  } else {
    Serial.printf("SSID found. Channel: %d\n", slave_channel);
    if (WiFi.channel() != slave_channel) {
      esp_wifi_set_promiscuous(true);
      esp_wifi_set_channel(slave_channel, WIFI_SECOND_CHAN_NONE);
      esp_wifi_set_promiscuous(false);
    }
  }
  Serial.printf("Current Wi-Fi channel: %d\n", WiFi.channel());
}

// Callback for send status
void OnDataSent(const uint8_t *mac, esp_now_send_status_t status) {
  // Handle send status here if needed
}

// Initialize ESP-NOW
void initESPNow() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Set the MAC address of the master device
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, masterMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add master as a peer");
    return;
  }
  Serial.println("Master added as a peer!");
}

// Send data to master
void sendDataToMaster() {
  esp_err_t result = esp_now_send(masterMAC, (uint8_t *)&myData, sizeof(myData));
  if (result != ESP_OK) {
    Serial.println("Error sending data to master");
  }
}

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Set relay pins as outputs
  pinMode(RELAY_PLANT_WATERING_PIN, OUTPUT);
  pinMode(RELAY_REFILL_PIN, OUTPUT);

  // Set sensor pins as inputs
  pinMode(SOIL_SENSOR_PIN, INPUT);
  pinMode(WATER_LEVEL_SENSOR_PIN, INPUT);

  // Initialize relays to OFF state
  digitalWrite(RELAY_PLANT_WATERING_PIN, HIGH); // Ensure relay is OFF
  digitalWrite(RELAY_REFILL_PIN, HIGH); // Ensure relay is OFF

  // Setup WiFi (required for ESP-NOW)
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(WIFI_PS_NONE);
  WiFi.channel(1);  // Set initial channel

  // Set Wi-Fi channel based on the master's Wi-Fi network
  scan_and_set_wifi_channel();

  // Initialize ESP-NOW
  initESPNow();

  // Register ESP-NOW send callback
  esp_now_register_send_cb(OnDataSent);
}

void loop() {
  // Read soil moisture sensor value
  int soilMoistureValue = analogRead(SOIL_SENSOR_PIN);
  // Read water level sensor value
  int waterLevelValue = analogRead(WATER_LEVEL_SENSOR_PIN);

  // Water level logic (no change)
  if (waterLevelValue < WATER_LEVEL_LOW_THRESHOLD) {
    if (!isRefilling) {
      digitalWrite(RELAY_REFILL_PIN, LOW); // Turn refill pump ON
      strcpy(myData.refillStatus, "Refilling");
      isRefilling = true;
    }
  } else if (waterLevelValue >= WATER_LEVEL_FULL_THRESHOLD) {
    if (isRefilling) {
      digitalWrite(RELAY_REFILL_PIN, HIGH); // Turn refill pump OFF
      strcpy(myData.refillStatus, "Full");
      isRefilling = false;
    }
  }

  // Timing variables for watering cooldown (6 hours)
  unsigned long currentTime = millis();    // Get current time

  // Soil moisture logic
  if (soilMoistureValue > SOIL_MOISTURE_THRESHOLD) {  // Soil is dry
    strcpy(myData.soilStatus, "Dry");
    
    if (currentTime - previousWateringTime > wateringInterval) { // Cooldown period is over
      Serial.println("Soil is dry. Watering the plant...");
      digitalWrite(RELAY_PLANT_WATERING_PIN, LOW); // Turn watering pump ON
      delay(5000); // Water for 5 seconds
      digitalWrite(RELAY_PLANT_WATERING_PIN, HIGH); // Turn watering pump OFF
      previousWateringTime = currentTime; // Update last watering time
      strcpy(myData.pumpStatus, "Watering"); // Update pump status to "Watering"
      myData.remainingCooldown = 0; // Reset cooldown since watering just occurred
    } else {
      // Cooldown period not over, hold off watering
      Serial.println("Soil is dry, but watering is on hold until cooldown interval expires.");
      unsigned long remainingCooldown = wateringInterval - (currentTime - previousWateringTime);
      Serial.print("Remaining cooldown: ");
      Serial.println(remainingCooldown);  // Display remaining cooldown in milliseconds
      strcpy(myData.pumpStatus, "Off"); // Pump is not running
      myData.remainingCooldown = remainingCooldown; // Send cooldown to master
    }
  } else {  // Soil is moist
    strcpy(myData.soilStatus, "Moist");
    Serial.println("Soil is moist. No watering needed.");
    digitalWrite(RELAY_PLANT_WATERING_PIN, HIGH); // Ensure watering pump is OFF
    strcpy(myData.pumpStatus, "Off"); // Pump is off since soil is moist
    myData.remainingCooldown = 0; // No cooldown needed since no watering happened
  }

  // Populate the structure with sensor data
  myData.soilMoistureValue = soilMoistureValue;
  myData.waterLevelValue = waterLevelValue;

  // Send the data to master via ESP-NOW
  sendDataToMaster();

  // Add a small delay to avoid flooding the serial monitor
  delay(1000);
}
