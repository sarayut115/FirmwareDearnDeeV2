#include <M5StickCPlus.h>
#include "BLEDevice.h"

const int forceResistorPin = 36;  // Assuming the force resistor is connected to GPIO 36
float batteryLow = 3.0;

#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"  //ตั้ง UUID ของ M5Stick
#define CHARACTERISTIC_UUID "d28a4899-0858-4622-9b4d-eb2acc823a25"

BLEServer* pServer;
BLEService* pService;
BLECharacteristic* pCharacteristic;

int previousForceValue = 0;  // เก็บค่า forceResistorPin ที่เคยอ่านไว้ล่าสุด

void setup() {
  Serial.begin(115200);
  M5.begin();
  M5.Lcd.setRotation(3);
  pinMode(forceResistorPin, INPUT);

  // Create the BLE Server
  BLEDevice::init("M5StickCPlus");
  pServer = BLEDevice::createServer();
  pService = pServer->createService(SERVICE_UUID);  // กำหนด UUID ของบริการ

  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

  // Set the initial value for the characteristic
  pCharacteristic->setValue("Hello, Client!");

  // Start the service
  pService->start();

  // Advertise the service
  pServer->getAdvertising()->addServiceUUID(pService->getUUID());
  pServer->getAdvertising()->setScanResponse(true);
  pServer->getAdvertising()->start();
  Serial.println("Server started");
}

void loop() {
  M5.update();

  // Read the value from the force resistor
  int forceValue = analogRead(forceResistorPin);
  int displayValue = map(forceValue, 0, 4095, 0, 100);  // Assuming 4095 is the maximum value the sensor can read

  // ตรวจสอบว่า forceResistorPin มีการกดหรือไม่
  if (forceValue > 0) {
    displayValue = 1;
  } else {
    displayValue = 0;
  }

  // // Map the force value to a range suitable for display


  // Display the force value on the screen
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.print("Force: ");
  M5.Lcd.fillRect(115, 0, 30, 23, BLACK);
  M5.Lcd.setCursor(115, 0);
  M5.Lcd.println(displayValue);

  float batteryVoltage = M5.Axp.GetBatVoltage();                                  // อ่านค่าแรงดันแบตเตอรี่ (เป็น mV)
  float batteryLevel = (batteryVoltage - batteryLow) / (4.2 - batteryLow) * 100;  // คำนวณระดับแบตเตอรี่ (%)


if (batteryLevel >= 0 && batteryLevel <= 100) {
  int bat = int(batteryLevel); // แปลง float เป็น int

  String message = String(displayValue) + "," + String(bat);

  // Set the value of the characteristic to the constructed message
  pCharacteristic->setValue(message.c_str());
  pCharacteristic->notify();

  M5.Lcd.fillRect(115, 30, 70, 23, BLACK);
  M5.Lcd.println(" ");
  M5.Lcd.printf("Battery : %d%%\n", bat); // แสดงผลของแบตเตอรี่เป็น integer
  delay(50);
} else {
  M5.Lcd.setCursor(115, 30);
  // แสดงข้อความเมื่อค่าแบตเตอรี่ไม่อยู่ในช่วงที่ถูกต้อง
  M5.Lcd.println("Invalid battery level");
}
}
