#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <DHT.h>

// Pin definitions
#define RELAY_PIN 2       // GPIO pin connected to the relay
#define DHTPIN 4          // Pin where the DHT11 is connected
#define DHTTYPE DHT11     // Define sensor type (DHT11)
#define THRESHOLD_TEMP 32 // Example temperature threshold

// DHT sensor setup
DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor

// Master's MAC Address (Replace with actual MAC)
uint8_t masterMAC[] = {0xFC, 0xE8, 0xC0, 0x74, 0x50, 0x14}; // Replace with master MAC

// Wi-Fi Network SSID (Define the SSID you're connecting to)
const char* wifi_network_ssid = "josip";  // Wi-Fi network SSID
const char* wifi_network_password = "12345678"; // Wi-Fi network password

// Structure for ESP-NOW data
typedef struct struct_message {
  float temperature;
  char fanStatus[20];
} struct_message;

struct_message myData;

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

  Serial.print("Sent Temperature: ");
  Serial.print(myData.temperature);
  Serial.print(" °C, Fan Status: ");
  Serial.println(myData.fanStatus);
}

void setup() {
  Serial.begin(115200);

  // Setup relay pin
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Ensure relay is off initially

  // Setup DHT sensor
  dht.begin();

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
  float temperature = dht.readTemperature(); // Reads temperature in Celsius

  // Check if the reading was successful
  if (isnan(temperature)) {
    Serial.println("Failed to read temperature from DHT sensor!");
    return;
  }

  // Check the temperature and control the relay
  if (temperature > THRESHOLD_TEMP) {
    digitalWrite(RELAY_PIN, HIGH); // Turn on the fan
    strcpy(myData.fanStatus, "ON");
  } else {
    digitalWrite(RELAY_PIN, LOW); // Turn off the fan
    strcpy(myData.fanStatus, "OFF");
  }

  // Populate the structure with temperature and fan status
  myData.temperature = temperature;

  // Send data to master
  sendDataToMaster();

  // Delay for stability
  delay(1000);
}
