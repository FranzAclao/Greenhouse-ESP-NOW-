#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// Definitions
#define LED_PIN 13
#define LDR_PIN 34  // LDR Pin
#define LDR_THRESHOLD_DIM 500
#define LDR_THRESHOLD_FULL 1000

// PWM settings
const int PWM_freq = 5000;
const int PWM_channel = 0;
const int PWM_resolution = 8;

// Modes
enum LightMode {OFF, DIM, FULL_ON};
LightMode currentMode = OFF;

// Structure for ESP-NOW data
typedef struct struct_message {
  int ldrValue;
  char lightStatus[20];
} struct_message;

struct_message myData;

// Master's MAC Address (Replace with actual MAC)
uint8_t masterMAC[] = {0xfc, 0xe8, 0xc0, 0x74, 0x50, 0x14}; // Replace with master MAC

// Wi-Fi Network SSID (Define the SSID you're connecting to)
const char* wifi_network_ssid = "josip";  // Wi-Fi network SSID
const char* wifi_network_password = "12345678"; // Wi-Fi network password

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
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// Setup PWM
void setupPWM() {
  ledcSetup(PWM_channel, PWM_freq, PWM_resolution);
  ledcAttachPin(LED_PIN, PWM_channel);
}

// Read LDR value
int readLDR() {
  return analogRead(LDR_PIN);
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
  if (result == ESP_OK) {
    Serial.println("Data sent successfully!");
  } else {
    Serial.println("Error sending data");
  }

  Serial.print("Sent LDR Value: ");
  Serial.print(myData.ldrValue);
  Serial.print(", Light Status: ");
  Serial.println(myData.lightStatus);
}

void setup() {
  Serial.begin(115200);

  // Setup LED and LDR pin
  pinMode(LED_PIN, OUTPUT);

  // Setup PWM
  setupPWM();

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
  int ldrValue = readLDR();

  // Determine the light mode based on LDR value
  if (ldrValue > LDR_THRESHOLD_FULL) {
    currentMode = FULL_ON;
    strcpy(myData.lightStatus, "ON");
  } else if (ldrValue > LDR_THRESHOLD_DIM) {
    currentMode = DIM;
    strcpy(myData.lightStatus, "DIM");
  } else {
    currentMode = OFF;
    strcpy(myData.lightStatus, "OFF");
  }

  // Apply the current mode
  int pwmValueDim = map(ldrValue, LDR_THRESHOLD_DIM, LDR_THRESHOLD_FULL, 50, 200);
  switch (currentMode) {
    case OFF:
      ledcWrite(PWM_channel, 0);  // LED OFF
      break;
    case DIM:
      ledcWrite(PWM_channel, pwmValueDim);  // Dim LED
      break;
    case FULL_ON:
      ledcWrite(PWM_channel, 255);  // Fully ON LED
      break;
  }

  // Populate the structure with LDR value and status
  myData.ldrValue = ldrValue;

  // Send data to master
  sendDataToMaster();

  delay(1000); // Delay before sending the next message
}
