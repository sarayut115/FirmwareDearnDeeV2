#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <M5Stack.h>
#include <PubSubClient.h>
#include <esp32-hal-timer.h>
#include <BLEDevice.h>
#include "Free_Fonts.h"
#define TEXT1 "DEARN DEE"
#define TEXT2 "V2"

#define DAC_PIN 26  // กำหนดขา DAC เป็นขา 26
#define G5_PIN 5    // กำหนดขา G5 เป็นขา 5
#define G13_PIN 13  // กำหนดขา G13 เป็นขา 13
#define G12_PIN 12
#define G15_PIN 15


extern const unsigned char m5stack_startup_music[];

bool showkey = false;
char keymap[2][4][10] = { { { 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p' }, { 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', '╝' }, { 'z', 'x', 'c', 'v', 'b', 'n', 'm', '^', '<', ' ' }, { '1', '2', '3', '4', '5', '6', '7', '8', '9', '0' } }, { { 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P' }, { 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', '╝' }, { 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '^', '<', ' ' }, { '<', '>', ',', '.', '-', '_', '+', '*', '/', '\\' } } };
int shift = 0;
int csel = 0;
int rsel = 0;
int ocsel = -1;
int orsel = -1;
int selecttt = 1;
String keystring;
String okeystring;

bool showkey2 = false;
String keystring2;
String okeystring2;

const char* ssid = keystring.c_str();
const char* password = keystring2.c_str();
const char* mqtt_server = "34.73.215.67";
const int mqtt_port = 1883;
const char* mqtt_username = "";
const char* mqtt_password = "";
const char* mqtt_client_id = "";
const char* topic_inten = "s7connection";
const char* topic_freq = "s7connection/frequency";
const char* topic_ramp = "s7connection/rampup";
const char* topic_pulse = "s7connection/pulse";
const char* topic_batSTM = "BATT_STIM";
const char* topic_online = "onOf";
const char* topic_IMU = "IMU";
const char* topic_batSEN = "BATT_SENSOR";
const char* topic_trigger = "TRIGGER";

WiFiClient espClient;
PubSubClient client(espClient);

int8_t getBatteryLevel() {
  Wire.beginTransmission(0x75);
  Wire.write(0x78);
  if (Wire.endTransmission(false) == 0
      && Wire.requestFrom(0x75, 1)) {
    switch (Wire.read() & 0xF0) {
      case 0xE0: return 25;
      case 0xC0: return 50;
      case 0x80: return 75;
      case 0x00: return 100;
      default: return 0;
    }
  }
  return -1;
}
int8_t previousBatteryLevel = -1;

static BLEUUID serviceUUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");  //กำหนด UUID ที่จะเชื่อมต่อ
static BLEUUID charUUID("d28a4899-0858-4622-9b4d-eb2acc823a25");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLEAdvertisedDevice* myDevice;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static boolean needRescan = false;
uint8_t prevData = 0;
String currentData;

int previousInten = 0;  // เก็บค่าก่อนหน้า
int previousFreq = 0;
int previousRamp = 0;
int previousPulse = 0;
int Multiplier = 0;
hw_timer_t* timer = NULL;
uint64_t lasttime = 0;
int micsec = 0;
int ramp = 0;
int status = 0;
int choose = 1;

int inten = 1;
int freq = 20;
int pulse = 200;
int rampup = 5;
int count = 1;
int Boost = 0;
int check = 1;
static int upRamp = 255;
int prevForceValue = -1;      // ใช้ตัวแปรเพื่อเก็บค่า previousForceValue ที่เคยรับมาก่อนหน้า
int prevBatteryLevel = -1.0;  // ใช้ตัวแปรเพื่อเก็บค่า previousBatteryLevel ที่เคยรับมาก่อนหน้า

String previousBatSEN;
String previousFSR;
String FSR;
String batSEN;

int intFSR = FSR.toInt();
int screen = 1;
int screen_key = 0;
bool isOnline = false;
int dacOut;
int previousScreen = 0;

//---------------------------------- WIFI --------------------------------

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    M5.Lcd.setTextColor(RED);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.println("Connecting to WiFi...");
    delay(500);
  }
  if (WiFi.status() == WL_CONNECTED) {
    status = 1;
  }
  setLCD();
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

//---------------------------------- MQTT --------------------------------


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  // ตรวจสอบค่าที่ได้รับและเก็บไว้
  if (strcmp(topic, topic_inten) == 0) {
    int receivedInten = message.toInt();
    if (receivedInten != previousInten) {
      previousInten = receivedInten;
      Serial.print("Updated intensity value to: ");
      Serial.println(previousInten);
      M5.Lcd.fillRect(223, 60, 30, 23, BLACK);
    }
  } else if (strcmp(topic, topic_freq) == 0) {
    int receivedFreq = message.toInt();
    if (receivedFreq != previousFreq) {
      previousFreq = receivedFreq;
      Serial.print("Updated frequency value to: ");
      Serial.println(previousFreq);
      M5.Lcd.fillRect(223, 90, 30, 40, BLACK);
    }
  } else if (strcmp(topic, topic_ramp) == 0) {
    int receivedRamp = message.toInt();
    if (receivedRamp != previousRamp) {
      previousRamp = receivedRamp;
      Serial.print("Updated Ramp-up value to: ");
      Serial.println(previousRamp);
      M5.Lcd.fillRect(223, 150, 30, 30, BLACK);
    }
  } else if (strcmp(topic, topic_ramp) == 0) {
    int receivedPulse = message.toInt();
    if (receivedPulse != previousPulse) {
      previousPulse = receivedPulse;
      Serial.print("Updated Pulse value to: ");
      Serial.println(previousPulse);
      M5.Lcd.fillRect(223, 120, 30, 30, BLACK);
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(mqtt_client_id, mqtt_username, mqtt_password)) {
      Serial.println("connected");
      // ส่งค่าที่เก็บไว้กลับไปที่ broker เมื่อเชื่อมต่อสำเร็จ
      client.publish(topic_inten, String(previousInten).c_str());
      client.subscribe(topic_inten);
      client.publish(topic_freq, String(previousFreq).c_str());
      client.subscribe(topic_freq);
      client.publish(topic_ramp, String(previousRamp).c_str());
      client.subscribe(topic_ramp);
      client.publish(topic_pulse, String(previousPulse).c_str());
      client.subscribe(topic_pulse);
      client.publish(topic_online, String(status).c_str());
      client.publish(topic_batSTM, String(getBatteryLevel()).c_str());
      client.publish(topic_batSEN, batSEN.c_str());
      client.publish(topic_trigger, FSR.c_str());
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

//---------------------------------- Bluetooth --------------------------------

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient* pClient = BLEDevice::createClient();
  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback());

  // Connect กับ server BLE
  pClient->connect(myDevice);
  Serial.println(" - Connected to server");

  // รับข้อมูลจากเซิร์ฟเวอร์ BLE
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");

  // รับข้อมูลจากเซิร์ฟเวอร์ BLE
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }

  if (pRemoteCharacteristic->canNotify()) {
    pRemoteCharacteristic->registerForNotify(notifyCallback);  //เรียกใช้ Function notifyCallback
  }

  connected = true;
  return true;
}


void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {

  String message;


  if (length > 0) {
    if (pData[0] == 0 || pData[0] == 1) {
      FSR = String(pData[0]);
      message = "";
      for (int i = 1; i < length; i++) {
        message += (char)pData[i];
      }
    } else {
      batSEN = "";
      message = "";
      for (int i = 0; i < length; i++) {
        message += (char)pData[i];
      }
    }

    int commaIndex = message.indexOf(',');
    if (commaIndex != -1) {
      FSR = message.substring(0, commaIndex);
      batSEN = message.substring(commaIndex + 1);
    }

    Serial.print("FSR : ");
    Serial.println(FSR);
    Serial.print("batSEN : ");
    Serial.println(batSEN);

    // เช็คว่าค่า batSEN เปลี่ยนแปลงหรือไม่
    if (batSEN != previousBatSEN) {
      // ถ้าเปลี่ยนแปลง ส่งค่าใหม่ไปทาง MQTT
      client.publish(topic_batSEN, batSEN.c_str());
      // อัพเดทค่า batSEN ล่าสุด
      previousBatSEN = batSEN;
    }

    if (FSR != previousFSR) {
      // ถ้าเปลี่ยนแปลง ส่งค่าใหม่ไปทาง MQTT
      client.publish(topic_trigger, FSR.c_str());
      // อัพเดทค่า batSEN ล่าสุด
      previousFSR = FSR;
    }
  }
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      needRescan = false;  // ไม่ต้องการสแกนใหม่ต่อเนื่อง
    } else {
      needRescan = true;  // ต้องการสแกนใหม่เพื่อค้นหาเซอร์วิสอื่น ๆ
    }
  }
};


void setupBLE() {
  BLEDevice::init("");

  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}

//---------------------------------- TIMER --------------------------------

void IRAM_ATTR onTimer() {
  if (FSR == "0" && screen != 1 && screen != 4) {
    if (micsec == 1) {
      digitalWrite(G5_PIN, HIGH);
      digitalWrite(G13_PIN, LOW);
      // RampupOfline();

    } else if (micsec == 2) {
      digitalWrite(G5_PIN, LOW);
      digitalWrite(G13_PIN, HIGH);
      // RampupOfline();

    } else if (micsec == 3) {
      digitalWrite(G5_PIN, LOW);
      digitalWrite(G13_PIN, LOW);
    }

    micsec++;
    if (micsec >= Multiplier) {  // จำนวนรอบเพิ่มขึ้นเนื่องจากความถี่ลดลง กำหนดเป็น 20 เพื่อให้ความถี่เท่ากับ 50 Hz
      micsec = 1;
    }
  }
  // ตรวจสอบว่าต้องการเปลี่ยน dacOut หรือไม่
  // RampupOfline();
}

void Set_time() {
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 500, true);  // ปรับค่าระยะเวลาให้สัญญาณเกิดขึ้นทุก 10,000 ไมโครวินาที
  timerAlarmEnable(timer);
}

//---------------------------------- SET PARAMETER online --------------------------------

void changeFreq() {
  switch (previousFreq) {
    case 20:
      Multiplier = 100;

      break;
    case 30:
      Multiplier = 67;

      break;
    case 40:
      Multiplier = 50;

      break;
    case 50:
      Multiplier = 40;

      break;
    case 60:
      Multiplier = 33;
  }
}

void updateDAC() {
  switch (previousInten) {
    case 0:
      dacOut = 0;

      break;
    case 1:
      dacOut = 240;

      break;
    case 2:
      dacOut = 190;

      break;
    case 3:
      dacOut = 150;

      break;
    case 4:
      dacOut = 120;

      break;
    case 5:
      dacOut = 110;

      break;
  }
}

// void updateRamp() {
//   switch (previousRamp) {
//     case 1:
//       upRamp = (upRamp + 241) % 256;
//       if (upRamp < dacOut) {
//         upRamp = 255;
//       }
//       break;
//     case 2:
//       upRamp = (upRamp + 244) % 256;
//       if (upRamp < dacOut) {
//         upRamp = 255;
//       }
//       break;
//     case 3:
//       upRamp = (upRamp + 246) % 256;
//       if (upRamp < dacOut) {
//         upRamp = 255;
//       }
//       break;
//     case 4:
//       upRamp = (upRamp + 251) % 256;
//       if (upRamp < dacOut) {
//         upRamp = 255;
//       }
//       break;
//     case 5:
//       upRamp = (upRamp + 254) % 256;
//       if (upRamp < dacOut) {
//         upRamp = 255;
//       }
//       break;
//   }
// }



//---------------------------------- SET PARAMETER ofline --------------------------------

void updateDAC_ofline() {
  switch (inten) {
    case 0:
      dacOut = 0;

      break;
    case 1:
      dacOut = 240;

      break;
    case 2:
      dacOut = 190;

      break;
    case 3:
      dacOut = 150;

      break;
    case 4:
      dacOut = 120;

      break;
    case 5:
      dacOut = 110;

      break;
  }
}

void changeFreq_ofline() {
  switch (freq) {
    case 20:
      Multiplier = 100;

      break;
    case 30:
      Multiplier = 67;

      break;
    case 40:
      Multiplier = 50;

      break;
    case 50:
      Multiplier = 40;

      break;
    case 60:
      Multiplier = 33;
  }
}

// void RampupOfline() {
//   switch (rampup) {
//     case 1:
//       upRamp = (upRamp + 241) % 256;
//       if (upRamp < dacOut) {
//         upRamp = 255;
//       }
//       break;
//     case 2:
//       upRamp = (upRamp + 244) % 256;
//       if (upRamp < dacOut) {
//         upRamp = 255;
//       }
//       break;
//     case 3:
//       upRamp = (upRamp + 246) % 256;
//       if (upRamp < dacOut) {
//         upRamp = 255;
//       }
//       break;
//     case 4:
//       upRamp = (upRamp + 251) % 256;
//       if (upRamp < dacOut) {
//         upRamp = 255;
//       }
//       break;
//     case 5:
//       upRamp = (upRamp + 254) % 256;
//       if (upRamp < dacOut) {
//         upRamp = 255;
//       }
//       break;
//   }
// }

//---------------------------------- setIMU --------------------------------


void IMUreading() {
  float accX = 0.0F;
  float accY = 0.0F;
  float accZ = 0.0F;

  float gyroX = 0.0F;
  float gyroY = 0.0F;
  float gyroZ = 0.0F;

  float pitch = 0.0F;
  float roll = 0.0F;
  float yaw = 0.0F;

  M5.IMU.getGyroData(&gyroX, &gyroY, &gyroZ);
  M5.IMU.getAhrsData(&pitch, &roll, &yaw);

  // Create payload string
  String payload = String(gyroX) + "," + String(gyroY) + "," + String(gyroZ) + "," + String(pitch) + "," + String(roll) + "," + String(yaw);


  if (FSR == "0") {
    client.publish(topic_IMU, payload.c_str());
  }
}

//---------------------------------- SETTING UI  --------------------------------
void Splash() {
  static uint8_t brightness, pre_brightness;
  uint32_t length = strlen((char*)m5stack_startup_music);
  M5.Lcd.setBrightness(0);
  M5.Lcd.setFreeFont(FF8);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.drawString(TEXT1, 10, 100, GFXFF);
  M5.Lcd.setTextColor(TFT_ORANGE);
  M5.Lcd.drawString(TEXT2, 260, 100, GFXFF);
  for (int i = 0; i < length; i++) {
    brightness = (i / 157);
    if (pre_brightness != brightness) {
      pre_brightness = brightness;
      M5.Lcd.setBrightness(brightness);
    }
  }
}

void choose_Mode_UI() {
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(10, 20);
  M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.println("-- Choose Mode --");
  M5.Lcd.setTextColor(TFT_DARKGREY);
  M5.Lcd.setCursor(120, 80);
  M5.Lcd.println("WiFi");
  M5.Lcd.setCursor(90, 120);
  M5.Lcd.println("Offline");
  M5.Lcd.setCursor(90, 160);
  M5.Lcd.println("Setting");
}

void setLCD() {

  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(40, 90);
  M5.Lcd.print("Set up your device on");
  M5.Lcd.setCursor(100, 128);
  M5.Lcd.print("Application");
  delay(5000);
  M5.Lcd.fillScreen(BLACK);
}

void UIOfline() {
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.setCursor(10, 20);
  M5.Lcd.print("---- Offline ----");
  M5.Lcd.setTextColor(TFT_DARKGREY);
  M5.Lcd.setCursor(10, 60);
  M5.Lcd.println("Intensity : " + String(inten));
  M5.Lcd.setCursor(10, 90);
  M5.Lcd.println("Frequency : " + String(freq));
  M5.Lcd.setCursor(10, 120);
  M5.Lcd.println("pulse     : " + String(pulse));
  M5.Lcd.setCursor(10, 150);
  M5.Lcd.println("RampUp    : " + String(rampup));
}

void Set_input() {
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(10, 20);
  M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.println("-- Change WiFi --");
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_DARKGREY);
  M5.Lcd.setCursor(10, 90);
  M5.Lcd.println("ssid :");
  M5.Lcd.setCursor(10, 150);
  M5.Lcd.println("pasword :");
}

//---------------------------------- KEYBOARD1 --------------------------------

void setShowkey() {
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
  M5.Lcd.setBrightness(100);
}

void keyboard() {
  setShowkey();
  if ((showkey) && ((csel != ocsel) || (rsel != orsel) || (keystring != okeystring))) {
    // M5.Speaker.tone(561, 40); //frequency 3000, with a duration of 200ms
    M5.Lcd.drawString("                                       ", 0, 80, 4);
    M5.Lcd.drawString(String(keystring), 0, 80, 4);

    int x, y;
    M5.Lcd.fillRect(0, 112, 320, 240, BLACK);
    for (int r = 0; r < 4; r++) {
      for (int c = 0; c < 10; c++) {
        x = (c * 32);
        y = (112 + (r * 32));
        if ((csel == c) && (rsel == r)) {
          M5.Lcd.setTextColor(TFT_BLACK, TFT_WHITE);
          M5.Lcd.fillRect(x, y, 32, 32, WHITE);
        } else {
          M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
          M5.Lcd.drawRect(x, y, 32, 32, RED);
        }
        M5.Lcd.drawString(String(keymap[shift][r][c]), x + 10, y + 7, 2);
      }
    }
    ocsel = csel;
    orsel = rsel;
  }
}

void LoopKey() {
  setShowkey();
  if (showkey) {
    if (M5.BtnA.wasReleased()) {
      csel = csel + 1;
      if (csel > 9) {
        csel = 0;
      }
    }
    if (M5.BtnC.wasReleased()) {
      rsel = rsel + 1;
      if (rsel > 3) {
        rsel = 0;
      }
    }
    if (M5.BtnB.pressedFor(700)) {
      showkey = false;
      M5.Lcd.clear();
      screen_key = screen_key - 1;
      Set_input();
    } else if (M5.BtnB.wasReleased()) {
      if ((rsel == 2) && (csel == 7)) {
        shift++;
        if (shift > 1)
          shift = 0;

        csel = 0;
        rsel = 0;
      } else if ((rsel == 2) && (csel == 8)) {
        int len = keystring.length();
        keystring = keystring.substring(0, len - 1);
      } else
        keystring += keymap[shift][rsel][csel];
    }
  }
}

//---------------------------------- KEYBOARD2 --------------------------------

void keyboard2() {
  setShowkey();
  if ((showkey2) && ((csel != ocsel) || (rsel != orsel) || (keystring2 != okeystring2))) {
    // M5.Speaker.tone(561, 40); //frequency 3000, with a duration of 200ms
    M5.Lcd.drawString("                                       ", 0, 80, 4);
    M5.Lcd.drawString(String(keystring2), 0, 80, 4);

    int x, y;
    M5.Lcd.fillRect(0, 112, 320, 240, BLACK);
    for (int r = 0; r < 4; r++) {
      for (int c = 0; c < 10; c++) {
        x = (c * 32);
        y = (112 + (r * 32));
        if ((csel == c) && (rsel == r)) {
          M5.Lcd.setTextColor(TFT_BLACK, TFT_WHITE);
          M5.Lcd.fillRect(x, y, 32, 32, WHITE);
        } else {
          M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
          M5.Lcd.drawRect(x, y, 32, 32, RED);
        }
        M5.Lcd.drawString(String(keymap[shift][r][c]), x + 10, y + 7, 2);
      }
    }
    ocsel = csel;
    orsel = rsel;
  }
}

void LoopKey2() {
  setShowkey();
  if (showkey2) {
    if (M5.BtnA.wasReleased()) {
      csel = csel + 1;
      if (csel > 9) {
        csel = 0;
      }
    }
    if (M5.BtnC.wasReleased()) {
      rsel = rsel + 1;
      if (rsel > 3) {
        rsel = 0;
      }
    }
    if (M5.BtnB.pressedFor(700)) {
      showkey2 = false;
      M5.Lcd.clear();
      screen_key = screen_key - 2;
      Set_input();
    } else if (M5.BtnB.wasReleased()) {
      if ((rsel == 2) && (csel == 7)) {
        shift++;
        if (shift > 1)
          shift = 0;

        csel = 0;
        rsel = 0;
      } else if ((rsel == 2) && (csel == 8)) {
        int len = keystring.length();
        keystring2 = keystring.substring(0, len - 1);
      } else
        keystring2 += keymap[shift][rsel][csel];
    }
  }
}

//---------------------------------- KEYBOARD LOOP --------------------------------

void loop_input() {
  if (M5.BtnB.wasReleased()) {
    selecttt++;
    if (selecttt > 2) {
      selecttt = 1;
    }
  }
  switch (selecttt) {
    case 1:
      M5.Lcd.setTextSize(2);
      M5.Lcd.setTextColor(TFT_DARKGREY);
      M5.Lcd.setCursor(10, 150);
      M5.Lcd.println("pasword : " + keystring2);
      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.setCursor(10, 90);
      M5.Lcd.println("ssid : " + keystring);

      if (M5.BtnC.pressedFor(700)) {
        showkey = !showkey;
        M5.Speaker.tone(661, 120);  //frequency 3000, with a duration of 200ms
        screen_key = screen_key + 1;
      }
      if (M5.BtnA.pressedFor(700)) {
        M5.Speaker.tone(661, 120);
        screen = screen - 3;
        choose_Mode_UI();
        Boost = 0;
        M5.Lcd.fillScreen(BLACK);
        Serial.println(screen);
      }
      break;

    case 2:
      M5.Lcd.setTextSize(2);
      M5.Lcd.setTextColor(TFT_DARKGREY);
      M5.Lcd.setCursor(10, 90);
      M5.Lcd.println("ssid : " + keystring);
      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.setCursor(10, 150);
      M5.Lcd.println("pasword : " + keystring2);
      if (M5.BtnC.pressedFor(700)) {
        showkey2 = !showkey2;
        M5.Speaker.tone(661, 120);  //frequency 3000, with a duration of 200ms
        screen_key = screen_key + 2;
      }
      if (M5.BtnA.pressedFor(700)) {
        M5.Speaker.tone(661, 120);
        screen = screen - 3;
        choose_Mode_UI();
        Boost = 0;
        M5.Lcd.fillScreen(BLACK);
        Serial.println(screen);
      }

      break;
  }
}

//------------------------------------ KEEP DATA FORM KEYBOARD ------------------------------



//---------------------------------- LOOP UI  --------------------------------


void battery() {
  int8_t currentBatteryLevel = getBatteryLevel();  // ดึงค่าปัจจุบันของระดับแบตเตอรี่

  if (currentBatteryLevel != previousBatteryLevel) {  // ตรวจสอบว่ามีการเปลี่ยนแปลงหรือไม่
    // ส่งค่าระดับแบตเตอรี่ที่เปลี่ยนแปลงไปยัง MQTT
    client.publish(topic_batSTM, String(currentBatteryLevel).c_str());

    // ปรับค่า previousBatteryLevel เป็นค่าปัจจุบัน
    previousBatteryLevel = currentBatteryLevel;
    M5.Lcd.fillRect(260, 6, 150, 10, BLACK);
  }
  if (currentBatteryLevel == 100) {
    M5.Lcd.setTextColor(TFT_GREEN);
  } else if (currentBatteryLevel == 75) {
    M5.Lcd.setTextColor(TFT_YELLOW);
  } else if (currentBatteryLevel == 50) {
    M5.Lcd.setTextColor(TFT_ORANGE);
  } else if (currentBatteryLevel == 25) {
    M5.Lcd.setTextColor(TFT_RED);
  }
  M5.Lcd.progressBar(290, 5, 20, 10, currentBatteryLevel);
  M5.Lcd.setCursor(260, 6);
  M5.Lcd.setTextSize(1);
  M5.Lcd.print(currentBatteryLevel);
  M5.Lcd.print("%");
  if (M5.Power.isCharging()) {
    M5.Lcd.setTextColor(TFT_YELLOW);
    M5.Lcd.setCursor(298, 6);
    M5.Lcd.setTextSize(1);
    M5.Lcd.printf("c");
  }
}

void showTrigger() {
  if (FSR == "0") {
    M5.Lcd.setTextSize(2);
    M5.Lcd.fillRect(140, 200, 150, 23, BLACK);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(140, 200);
    M5.Lcd.print("    Stimulating");
  } else {
    M5.Lcd.setTextSize(2);
    M5.Lcd.fillRect(140, 200, 150, 23, BLACK);
    M5.Lcd.setTextColor(TFT_DARKGREY);
    M5.Lcd.setCursor(140, 200);
    M5.Lcd.print("Not Stimulating");
  }
}

void setLoopLCD_ofline() {

  if (M5.BtnB.wasReleased()) {
    count++;
    if (count > 4) {
      count = 1;
    }
  }

  M5.Lcd.setTextSize(3);
  switch (count) {
    case 1:
      M5.Lcd.setTextColor(TFT_DARKGREY);
      M5.Lcd.setCursor(10, 90);
      M5.Lcd.print("Frequency : ");
      M5.Lcd.println(freq);
      M5.Lcd.setCursor(10, 120);
      M5.Lcd.print("pulse     : ");
      M5.Lcd.println(pulse);
      M5.Lcd.setCursor(10, 150);
      M5.Lcd.print("RampUp    : ");
      M5.Lcd.println(rampup);

      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.setCursor(10, 60);
      M5.Lcd.print("Intensity : ");
      M5.Lcd.println(inten);

      if (M5.BtnA.wasReleased()) {
        inten = inten - 1;
        if (inten <= 0) {
          inten = 0;
        }
        M5.Lcd.fillRect(223, 60, 30, 23, BLACK);
        M5.Lcd.setCursor(226, 60);
        M5.Lcd.println(inten);
      } else if (M5.BtnC.wasReleased()) {

        inten += 1;
        if (inten > 5) {
          inten = 5;
        }

        M5.Lcd.fillRect(223, 60, 30, 23, BLACK);
        M5.Lcd.setCursor(226, 60);
        M5.Lcd.println(inten);
      }
      if (M5.BtnA.pressedFor(1000)) {
        M5.Speaker.tone(661, 120);
        screen = screen - 2;
        choose_Mode_UI();
        Boost = 0;
        M5.Lcd.fillScreen(BLACK);
        Serial.println(screen);
      }
      break;
    case 2:
      M5.Lcd.setTextColor(TFT_DARKGREY);
      M5.Lcd.setCursor(10, 60);
      M5.Lcd.print("Intensity : ");
      M5.Lcd.println(inten);
      M5.Lcd.setCursor(10, 120);
      M5.Lcd.print("pulse     : ");
      M5.Lcd.println(pulse);
      M5.Lcd.setCursor(10, 150);
      M5.Lcd.print("RampUp    : ");
      M5.Lcd.println(rampup);

      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.setCursor(10, 90);
      M5.Lcd.print("Frequency : ");
      M5.Lcd.println(freq);


      if (M5.BtnA.wasReleased()) {
        freq = freq - 20;
        if (freq <= 20) {
          freq = 20;
        }
        M5.Lcd.fillRect(223, 90, 30, 23, BLACK);
        M5.Lcd.setCursor(226, 90);
        M5.Lcd.println(freq);
      } else if (M5.BtnC.wasReleased()) {
        freq += 10;
        if (freq >= 60) {
          freq = 60;
        }
        M5.Lcd.fillRect(223, 90, 30, 23, BLACK);
        M5.Lcd.setCursor(226, 90);
        M5.Lcd.println(freq);
      }
      if (M5.BtnA.pressedFor(1000)) {
        M5.Speaker.tone(661, 120);

        screen = screen - 2;
        choose_Mode_UI();
        Boost = 0;
        M5.Lcd.fillScreen(BLACK);
        Serial.println(screen);
      }
      break;
    case 3:
      M5.Lcd.setTextColor(TFT_DARKGREY);
      M5.Lcd.setCursor(10, 60);
      M5.Lcd.print("Intensity : ");
      M5.Lcd.println(inten);
      M5.Lcd.setCursor(10, 120);
      M5.Lcd.setCursor(10, 90);
      M5.Lcd.print("Frequency : ");
      M5.Lcd.println(freq);
      M5.Lcd.setCursor(10, 150);
      M5.Lcd.print("RampUp    : ");
      M5.Lcd.println(rampup);

      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.setCursor(10, 120);
      M5.Lcd.print("pulse     : ");
      M5.Lcd.println(pulse);

      if (M5.BtnA.wasReleased()) {
        pulse = pulse - 100;
        if (pulse <= 200) {
          pulse = 200;
        }
        M5.Lcd.fillRect(223, 120, 30, 23, BLACK);
        M5.Lcd.setCursor(226, 120);
        M5.Lcd.println(pulse);
      } else if (M5.BtnC.wasReleased()) {
        pulse += 100;
        if (pulse > 500) {
          pulse = 500;
        }
        M5.Lcd.fillRect(223, 120, 30, 23, BLACK);
        M5.Lcd.setCursor(226, 120);
        M5.Lcd.println(pulse);
      }
      if (M5.BtnA.pressedFor(1000)) {
        M5.Speaker.tone(661, 120);
        screen = screen - 2;
        choose_Mode_UI();
        Boost = 0;
        M5.Lcd.fillScreen(BLACK);
        Serial.println(screen);
      }
      break;
    case 4:
      M5.Lcd.setTextColor(TFT_DARKGREY);
      M5.Lcd.setCursor(10, 60);
      M5.Lcd.print("Intensity : ");
      M5.Lcd.println(inten);
      M5.Lcd.setCursor(10, 90);
      M5.Lcd.print("Frequency : ");
      M5.Lcd.println(freq);
      M5.Lcd.setCursor(10, 120);
      M5.Lcd.print("pulse     : ");
      M5.Lcd.println(pulse);

      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.setCursor(10, 150);
      M5.Lcd.print("RampUp    : ");
      M5.Lcd.println(rampup);

      if (M5.BtnA.wasReleased()) {
        rampup = rampup - 1;
        if (rampup <= 0) {
          rampup = 1;
        }
        M5.Lcd.fillRect(223, 150, 30, 23, BLACK);
        M5.Lcd.setCursor(226, 150);
        M5.Lcd.println(rampup);
      } else if (M5.BtnC.wasReleased()) {
        rampup += 1;
        if (rampup > 5) {
          rampup = 5;
        }
        M5.Lcd.fillRect(223, 150, 30, 23, BLACK);
        M5.Lcd.setCursor(226, 150);
        M5.Lcd.println(rampup);
      }
      if (M5.BtnA.pressedFor(1000)) {
        M5.Speaker.tone(661, 120);

        screen = screen - 2;
        choose_Mode_UI();
        Boost = 0;
        M5.Lcd.fillScreen(BLACK);
        Serial.println(screen);
      }
      break;
  }
}


void setLoopLCD_online() {
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(20, 20);
  M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.println("---- Online ----");
  M5.Lcd.setTextColor(TFT_DARKGREY);
  M5.Lcd.setCursor(10, 60);
  M5.Lcd.print("Intensity : ");
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.println(previousInten);
  M5.Lcd.setTextColor(TFT_DARKGREY);
  M5.Lcd.setCursor(10, 90);
  M5.Lcd.print("Frequency : ");
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.println(previousFreq);
  M5.Lcd.setCursor(10, 120);
  M5.Lcd.setTextColor(TFT_DARKGREY);
  M5.Lcd.print("pulse     : ");
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.println(previousPulse);
  M5.Lcd.setCursor(10, 150);
  M5.Lcd.setTextColor(TFT_DARKGREY);
  M5.Lcd.print("RampUp    : ");
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.println(previousRamp);
  if (M5.BtnA.pressedFor(1000)) {
    status = 0;
    client.publish(topic_online, String(status).c_str());
    delay(3000);
    screen = screen - 1;
    Boost = 0;
    choose_Mode_UI();
  }
}



void Choose_Mode() {
  if (M5.BtnB.wasReleased()) {
    choose++;
    if (choose > 3) {
      choose = 1;
    }
  }
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(10, 20);
  M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.println("-- Choose Mode --");
  switch (choose) {
    case 1:
      M5.Lcd.setTextColor(TFT_DARKGREY);
      M5.Lcd.setCursor(90, 120);
      M5.Lcd.println("Offline");
      M5.Lcd.fillRect(65, 120, 25, 23, BLACK);
      M5.Lcd.fillRect(65, 160, 25, 23, BLACK);
      M5.Lcd.setCursor(90, 160);
      M5.Lcd.println("Setting");
      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.setCursor(90, 80);
      M5.Lcd.println("> ");
      M5.Lcd.setCursor(120, 80);
      M5.Lcd.println("WiFi");
      if (M5.BtnC.pressedFor(700)) {
        if (okeystring.isEmpty()) {
          M5.Lcd.fillScreen(BLACK);
          M5.Lcd.setTextSize(2);
          M5.Lcd.setTextColor(RED);
          M5.Lcd.setCursor(10, 100);
          M5.Lcd.println("Fill your SSID");
          M5.Lcd.setCursor(5, 120);
          M5.Lcd.println("and WiFi password");
          delay(3000);
          M5.Lcd.fillScreen(BLACK);
          choose_Mode_UI();
        } else {
          //M5.Speaker.tone(661, 120);
          M5.Lcd.fillScreen(BLACK);
          screen = screen + 1;
          setup_wifi();
        }
      }
      break;

    case 2:
      M5.Lcd.setTextColor(TFT_DARKGREY);
      M5.Lcd.setCursor(120, 80);
      M5.Lcd.println("WiFi");
      M5.Lcd.setCursor(90, 160);
      M5.Lcd.println("Setting");
      M5.Lcd.fillRect(65, 160, 25, 23, BLACK);
      M5.Lcd.fillRect(90, 80, 30, 23, BLACK);
      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.setCursor(65, 120);
      M5.Lcd.println("> ");
      M5.Lcd.setCursor(90, 120);
      M5.Lcd.println("Offline");
      if (M5.BtnC.pressedFor(700)) {
        M5.Speaker.tone(661, 120);
        M5.Lcd.fillScreen(BLACK);
        screen = screen + 2;
        UIOfline();
      }

      break;

    case 3:
      M5.Lcd.setTextColor(TFT_DARKGREY);
      M5.Lcd.setCursor(120, 80);
      M5.Lcd.println("WiFi");
      M5.Lcd.setCursor(90, 120);
      M5.Lcd.println("Offline");
      M5.Lcd.fillRect(65, 120, 25, 23, BLACK);
      M5.Lcd.fillRect(90, 80, 30, 23, BLACK);
      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.setCursor(65, 160);
      M5.Lcd.println("> ");
      M5.Lcd.setCursor(90, 160);
      M5.Lcd.println("Setting");
      if (M5.BtnC.pressedFor(700)) {
        M5.Speaker.tone(661, 120);
        M5.Lcd.fillScreen(BLACK);
        screen = screen + 3;
        Set_input();
      }

      break;
  }
}



//---------------------------------- MAIN --------------------------------


void setup() {
  pinMode(G5_PIN, OUTPUT);
  pinMode(G13_PIN, OUTPUT);
  pinMode(G15_PIN, OUTPUT);  /////////////////
  digitalWrite(G5_PIN, LOW);
  digitalWrite(G13_PIN, LOW);
  digitalWrite(G15_PIN, HIGH);

  M5.begin();
  M5.Power.begin();
  M5.IMU.Init();
  Wire.begin();
  Splash();
  setupBLE();
  M5.Lcd.setFreeFont(NULL);
  M5.Lcd.setBrightness(100);

  choose_Mode_UI();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);


  // เริ่มต้นใช้งาน Timer
  Set_time();
}



void loop() {
  M5.update();
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("Connected to the BLE Server.");
    } else {
      Serial.println("Failed to connect to the server.");
    }
    doConnect = false;
  }

  while (needRescan) {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(RED);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.println("Connecting to Bluetooth...");
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->start(5, false);  // Start scanning again
    delay(1000);
    M5.Lcd.clear();

    break;
  }


  if (screen == 1) {
    battery();
    Choose_Mode();
  } else if (screen == 2) {
    battery();
    showTrigger();
    if (WiFi.status() != WL_CONNECTED) {
      setup_wifi();  // ตรวจสอบสถานะ WiFi และเชื่อมต่อหากไม่ได้เชื่อมต่อ
    }
    if (!client.connected()) {
      reconnect();  // ตรวจสอบและเชื่อมต่อ MQTT หากยังไม่ได้เชื่อมต่อ
    }
    client.loop();

    changeFreq();
    IMUreading();
    setLoopLCD_online();
    updateDAC();
  } else if (screen == 3) {
    battery();
    showTrigger();
    setLoopLCD_ofline();
    changeFreq_ofline();
    updateDAC_ofline();
  } else if (screen == 4) {
    battery();
    if (screen_key == 0) {
      loop_input();
    } else if (screen_key == 1) {
      keyboard();
      okeystring = keystring;
      LoopKey();
    } else if (screen_key == 2) {
      keyboard2();
      okeystring2 = keystring2;
      LoopKey2();
    }
  }
  static int previousdacOut = dacOut;  // เก็บค่า dacOut ก่อนหน้าไว้
  if (previousdacOut != dacOut) {      // ถ้ามีการเปลี่ยนแปลง
    dacWrite(DAC_PIN, dacOut);         // ใช้ dacOut ใหม่
    previousdacOut = dacOut;           // ปรับปรุงค่า previousDacOut
  }
}