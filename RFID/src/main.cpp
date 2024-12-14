#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>

// RFID Module Pins
#define RST_PIN 22 // Reset pin for the RFID module
#define SS_PIN 5   // Slave select pin for the RFID module

// Servo Motor Pin
int servoPin = 13; // Pin connected to the servo motor

// Create instances for RFID and servo
MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance
Servo myServo;                   // Create a servo object

// Target UID (replace with your RFID card's UID)
byte targetUID[] = {0x82, 0x44, 0xF4, 0x51};

// Function prototype
bool isUIDMatch(byte* uid, byte uidSize, byte* target);

void setup() {
  Serial.begin(115200);   // Start Serial communication
  SPI.begin();            // Initialize SPI bus
  mfrc522.PCD_Init();     // Initialize the RFID module
  Serial.println("Scan RFID card...");

  // Attach the servo to the specified GPIO pin
  myServo.attach(servoPin);

  // Test servo movement at startup
  Serial.println("Testing servo movement...");
  myServo.write(190);  // Move servo to 90 degrees
  delay(2000);        // Wait for 2 seconds
  myServo.write(0);   // Move servo back to 0 degrees
  delay(2000);        // Wait for 2 seconds
}

void loop() {
  // Look for new RFID cards
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    delay(50); // Short delay before trying again
    return;
  }

  // Print detected UID in hexadecimal format
  Serial.print("Card UID: ");
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    // Print each byte of the UID in hexadecimal with leading zero if needed
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println();

  // Check if the UID matches the target UID
  if (isUIDMatch(mfrc522.uid.uidByte, mfrc522.uid.size, targetUID)) {
    Serial.println("Authorized UID detected!");
    myServo.write(90);  // Rotate servo to 90 degrees
    Serial.println("Door opened");
    delay(3000);        // Hold position for 3 seconds
    myServo.write(0);   // Return servo to 0 degrees
    Serial.println("Servo returned to 0 degrees");
  } else {
    Serial.println("Unauthorized UID!");
    myServo.write(0);  // Rotate servo to 180 degrees
    Serial.println("Door closed");
    delay(3000);         // Hold position for 3 seconds
    myServo.write(0);    // Return servo to 0 degrees
    Serial.println("Servo returned to 0 degrees");
  }

  // Halt the RFID card to avoid repeated readings
  mfrc522.PICC_HaltA();
}

// Function to compare the detected UID with the target UID
bool isUIDMatch(byte* uid, byte uidSize, byte* target) {
  for (byte i = 0; i < uidSize; i++) {
    if (uid[i] != target[i]) {
      return false; // Mismatch found
    }
  }
  return true; // All bytes matched
}