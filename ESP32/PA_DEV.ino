/*
==========================================================
SMART BELL SYSTEM - FAST REALTIME VERSION
BOARD : ESP32 DOIT DEVKIT V1
==========================================================

PIR      -> GPIO 19
HC-SR04  -> Trig 5, Echo 18
BUZZER   -> GPIO 4
LED      -> GPIO 2
LCD I2C  -> SDA 21, SCL 22

Serial2:
TX -> GPIO 17 -> RX ESP32-CAM
RX -> GPIO 16
==========================================================
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ==========================================================
// TAMBAHAN: INCLUDE WIFI & TELEGRAM
// ==========================================================
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ThingerESP32.h>

// ==========================================================
// TAMBAHAN: INCLUDE MQTT
// ==========================================================
#include <PubSubClient.h>

// ==========================================================
// LCD
// ==========================================================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ==========================================================
// PIN
// ==========================================================
const int pinPIR    = 19;
const int pinTrig   = 5;
const int pinEcho   = 18;
const int pinBuzzer = 4;
const int pinLED    = 2;
const int pinButton = 15;

// ==========================================================
// VARIABLE
// ==========================================================
unsigned long lastJepretTime = 0;
volatile bool manualCapture = false;
volatile bool needFlashUpdate = false;
volatile bool needJepret = false;unsigned long pirLcdClearTime = 0;
unsigned long lastBuzzerMillis = 0;
unsigned long lastPirDetect = 0;
bool toggleState = false;
String lastLine1 = "";
String lastLine2 = "";
bool muteTemporary = false;
unsigned long buttonPressTime = 0;
unsigned long objectGoneTime = 0;
int hcsrConfirmCount = 0;
const int HCSR_CONFIRM_THRESHOLD = 3;
long lastJarak = 999;
bool sudahJepret = false;
// ==========================================================
// TELEGRAM CONTROL FLAG
// ==========================================================
bool buzzerEnable = true;
bool ledEnable    = true;
bool lcdEnable    = true;
bool camEnable    = true;
bool flashEnable  = false;
bool pirEnable    = true;
bool hcsrEnable   = true;

// ==========================================================
// TAMBAHAN: WIFI & TELEGRAM VARIABLE
// ==========================================================
const char* ssid     = "Guik guik piyaw piyaw";
const char* password = "12345678";
String BOT_TOKEN = "8724613363:AAFafZ2MfEeztAUb0_9ZyDiTfPP6UgYqd9U";
String CHAT_ID = "1353775639";
long lastUpdateId = 0;
unsigned long lastTelegramPoll = 0;
const unsigned long TELEGRAM_INTERVAL = 300;

#include <UniversalTelegramBot.h>
WiFiClientSecure telegramSecure;
UniversalTelegramBot bot(BOT_TOKEN, telegramSecure);

#define USERNAME "rapip"
#define DEVICE_ID "iot_9"
#define DEVICE_CREDENTIAL "EpihFX2yDkb&V8l8"

ThingerESP32 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

// ==========================================================
// TAMBAHAN: MQTT VARIABLE
// ==========================================================
const char* MQTT_SERVER = "24bb092dd7274dcc963dd9b82f95928a.s1.eu.hivemq.cloud"; // ganti dengan URL cluster kamu
const int   MQTT_PORT   = 8883; // HiveMQ Cloud pakai TLS
const char* MQTT_CLIENT = "SmartBell_ESP32";
const char* MQTT_USER   = "kelompok9";       // username tadi
const char* MQTT_PASS   = "Smartbell123"; // password tadi

const char* TOPIC_PIR    = "smartbell/pir";
const char* TOPIC_STATUS = "smartbell/status";
const char* TOPIC_CMD    = "smartbell/cmd";
const char* TOPIC_STATUS_TAMU = "smartbell/tamu";

WiFiClientSecure mqttWifi;
PubSubClient mqtt(mqttWifi);

unsigned long lastMqttPublish = 0;
const unsigned long MQTT_PUBLISH_INTERVAL = 2000;

long bacaJarak();

// ==========================================================
// TAMBAHAN: MQTT CALLBACK
// ==========================================================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  msg.trim();
  Serial.print("MQTT CMD: ");
  Serial.println(msg);
  handleCommand("/" + msg);
}

// ==========================================================
// TAMBAHAN: MQTT RECONNECT
// ==========================================================
void mqttReconnect() {
  if (mqtt.connected()) return;
  if (WiFi.status() != WL_CONNECTED) return;

//  mqttWifi.setInsecure(); // tambah ini
  Serial.print("Connecting MQTT...");
  if (mqtt.connect(MQTT_CLIENT, MQTT_USER, MQTT_PASS)) { // tambah user & pass
    Serial.println("MQTT Connected!");
    mqtt.subscribe(TOPIC_CMD);
  } else {
    Serial.print("MQTT failed, rc=");
    Serial.println(mqtt.state());
  }
}

// ==========================================================
// TAMBAHAN: MQTT PUBLISH
// ==========================================================
void mqttPublish() {
  if (!mqtt.connected()) return;

  if (millis() - lastPirDetect < 5000) {
    mqtt.publish(TOPIC_PIR, "GERAKAN TERDETEKSI");
  } else {
    mqtt.publish(TOPIC_PIR, "TIDAK ADA GERAKAN");
  }

  if (lastJarak >= 2 && lastJarak <= 100) {
    mqtt.publish(TOPIC_STATUS_TAMU, "TAMU DATANG");
  } else {
    mqtt.publish(TOPIC_STATUS_TAMU, "AMAN");
  }

  String status = "JARAK:" + String(lastJarak) + "cm";
  status += " LED:" + String(ledEnable ? "ON" : "OFF");
  status += " CAM:" + String(camEnable ? "ON" : "OFF");
  status += " PIR:" + String(pirEnable ? "ON" : "OFF");
  status += " FLASH:" + String(flashEnable ? "ON" : "OFF");
  status += " HCSR:" + String(hcsrEnable ? "ON" : "OFF");
  status += " LCD:" + String(lcdEnable ? "ON" : "OFF");
  mqtt.publish(TOPIC_STATUS, status.c_str());
}

// ==========================================================
// UPDATE FLASH ESP32-CAM
// ==========================================================
void updateFlash() {

  if (flashEnable) {
    Serial2.write('F');
  }

  else {
    Serial2.write('f');
  }
}

// ==========================================================
// COMMAND TELEGRAM
// ==========================================================
void handleCommand(String text) {

  text.trim();

  if (text == "/buzzer_on") buzzerEnable = true;
  if (text == "/buzzer_off") buzzerEnable = false;

  if (text == "/led_on") ledEnable = true;
  if (text == "/led_off") ledEnable = false;

  if (text == "/lcd_on") lcdEnable = true;
  if (text == "/lcd_off") lcdEnable = false;

  if (text == "/cam_on") camEnable = true;
  if (text == "/cam_off") camEnable = false;

  if (text == "/pir_on") pirEnable = true;
  if (text == "/pir_off") pirEnable = false;

  if (text == "/hcsr_on") hcsrEnable = true;
  if (text == "/hcsr_off") hcsrEnable = false;

  if (text == "/flash_on") {
    flashEnable = true;
    needFlashUpdate = true;  // jangan Serial2 langsung dari Core 0
  }

  if (text == "/flash_off") {
    flashEnable = false;
    needFlashUpdate = true;
  }

  if (text == "/jepret") {
    if (!camEnable) {
      Serial.println("CAM DISABLED!");
    } else {
      needJepret = true;     // jangan Serial2 langsung dari Core 0
      Serial.println("JEPRET MANUAL!");
    }
  }

  Serial.print("COMMAND: ");
  Serial.println(text);
}

// ==========================================================
// TAMBAHAN: WIFI CONNECT
// ==========================================================
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Menghubungkan WiFi");
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
    retry++;
    if (retry >= 30) {
      Serial.println("\nReconnect WiFi...");
      WiFi.disconnect();
      delay(500);
      WiFi.begin(ssid, password);
      retry = 0;
    }
  }
  Serial.println();
  Serial.println("WiFi Connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// ==========================================================
// TAMBAHAN: POLLING TELEGRAM COMMAND
// ==========================================================
void checkTelegramCommand() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  
  for (int i = 0; i < numNewMessages; i++) {
    String text = bot.messages[i].text;
    
    Serial.print("TG CMD: ");
    Serial.println(text);
    
    handleCommand(text);
    
    // INI YANG PENTING!
    bot.last_message_received = bot.messages[i].message_id;
  }
}

// ==========================================================
// UPDATE LCD STABLE
// ==========================================================
void updateLCD(String line1, String line2) {

  if (!lcdEnable) return;

  if (line1 != lastLine1 || line2 != lastLine2) {

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print(line1);

    lcd.setCursor(0, 1);
    lcd.print(line2);

    lastLine1 = line1;
    lastLine2 = line2;
  }
}

void telegramTask(void *pvParameters) {
  for (;;) {
    if (millis() - lastTelegramPoll >= TELEGRAM_INTERVAL) {
      lastTelegramPoll = millis();
      checkTelegramCommand();
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// ==========================================================
// SETUP
// ==========================================================
void setup() {

  Serial.begin(115200);

  Serial2.begin(115200, SERIAL_8N1, 16, 17);

  // TAMBAHAN: CONNECT WIFI
  connectWiFi();
  thing.add_wifi(ssid, password);
  telegramSecure.setInsecure(); // ← TAMBAH INI SEKALI SAJA

  // ==========================================================
  // TAMBAHAN: SETUP MQTT
  // ==========================================================
  mqttWifi.setInsecure();           // ← Pindahkan ke sini (sekali saja)
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  mqttReconnect();

  xTaskCreatePinnedToCore(
    telegramTask,
    "TelegramTask",
    16384,  // ← stack lebih besar
    NULL,
    2,      // ← prioritas lebih tinggi dari loop
    NULL,
    0
  );
  updateFlash();

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("   SMART BELL ");

  lcd.setCursor(0, 1);
  lcd.print(" INITIALIZING ");

  pinMode(pinPIR, INPUT);
  pinMode(pinTrig, OUTPUT);
  pinMode(pinEcho, INPUT);

  pinMode(pinBuzzer, OUTPUT);
  pinMode(pinLED, OUTPUT);
  pinMode(pinButton, INPUT_PULLUP);

  if (buzzerEnable) digitalWrite(pinBuzzer, HIGH);
  if (ledEnable) digitalWrite(pinLED, HIGH);

  delay(200);

  digitalWrite(pinBuzzer, LOW);
  digitalWrite(pinLED, LOW);

  Serial.println();
  Serial.println("=================================");
  Serial.println("SMART BELL SYSTEM ACTIVE");
  Serial.println("=================================");

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("CALIBRATING");

  lcd.setCursor(0, 1);
  lcd.print("WAIT 5 SEC");

  delay(5000);

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print(" SMART BELL ");

  lcd.setCursor(0, 1);
  lcd.print(" SYSTEM READY");

  Serial.println("SYSTEM READY!");

  thing["monitor"] >> [](pson& out){
    long jarak = lastJarak;

    out["jarak_cm"] = jarak;

    if (jarak >= 2 && jarak <= 100)
      out["ultrasonic"] = "OBJEK TERDETEKSI";
    else
      out["ultrasonic"] = "AMAN";

    if (millis() - lastPirDetect < 5000)
      out["pir"] = "GERAKAN TERDETEKSI";
    else
      out["pir"] = "TIDAK ADA GERAKAN";

    out["led"] = digitalRead(pinLED);
    out["cam"] = camEnable;
    out["lcd_status"] = lcdEnable;
    out["camera_status"] = camEnable;
  };

  thing["led_manual"] << [](pson& in){
    if(in.is_empty()){
      in = ledEnable;
    } else {
      ledEnable = in;
    }
  };

  thing["buzzer_manual"] << [](pson& in){
    if(in.is_empty()){
      in = buzzerEnable;
    } else {
      buzzerEnable = in;
    }
  };

  thing["lcd_control"] << [](pson& in){
    if(in.is_empty()){
      in = lcdEnable;
    } else {
      lcdEnable = in;
    }
  };

  thing["flash_manual"] << [](pson& in){
    if(in.is_empty()){
      in = flashEnable;
    } else {
      flashEnable = in;
      updateFlash();
    }
  };

  thing["pir_control"] << [](pson& in){
    if(in.is_empty()){
      in = pirEnable;
    } else {
      pirEnable = in;
    }
  };

  thing["ultrasonic_control"] << [](pson& in){
    if(in.is_empty()){
      in = hcsrEnable;
    } else {
      hcsrEnable = in;
    }
  };

  thing["cam_control"] << [](pson& in){
    if(in.is_empty()){
      in = camEnable;
    } else {
      camEnable = in;
    }
  };

  delay(1000);

  lcd.clear();
}

// ==========================================================
// BACA JARAK HC-SR04
// ==========================================================
long bacaJarak() {

  if (!hcsrEnable) return 999;

  digitalWrite(pinTrig, LOW);
  delayMicroseconds(2);

  digitalWrite(pinTrig, HIGH);
  delayMicroseconds(10);

  digitalWrite(pinTrig, LOW);

  long duration = pulseIn(pinEcho, HIGH, 12000); // max ~200cm cukup
  
  if (duration == 0) {
    return 999;
  }

  long cm = duration * 0.034 / 2;

  if (cm <= 0 || cm > 400) {
    return 999;
  }

  return cm;
}

// ==========================================================
// LOOP - LCD SEPERTI YANG KAMU MAU
// ==========================================================
void loop() {

  // Serial2 & Manual Jepret
  if (needFlashUpdate) {
    needFlashUpdate = false;
    updateFlash();
  }
  if (needJepret) {
    needJepret = false;
    manualCapture = true;
    updateFlash();
    Serial2.write('S');
  }

  // Thinger & MQTT
  static unsigned long lastThingHandle = 0;
  if (millis() - lastThingHandle > 100) {
    thing.handle();
    lastThingHandle = millis();
  }

  if (!mqtt.connected()) {
    static unsigned long lastMqttRetry = 0;
    if (millis() - lastMqttRetry > 5000) {
      lastMqttRetry = millis();
      mqttReconnect();
    }
  }
  mqtt.loop();

  if (millis() - lastMqttPublish >= MQTT_PUBLISH_INTERVAL) {
    lastMqttPublish = millis();
    mqttPublish();
  }

  // Button
  if (!muteTemporary) {
    if (digitalRead(pinButton) == LOW) {
      if (buttonPressTime == 0) buttonPressTime = millis();
      if (millis() - buttonPressTime > 150) {
        muteTemporary = true;
        digitalWrite(pinBuzzer, LOW);
        digitalWrite(pinLED, LOW);
        updateLCD("   MODE DIAM   ", " TUNGGU PERGI  ");
        Serial.println("MODE DIAM AKTIF");
        buttonPressTime = 0;
      }
    } else buttonPressTime = 0;
  }

  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    handleCommand(cmd);
  }

  if (lcdEnable) lcd.backlight();
  else { lcd.noBacklight(); lcd.clear(); }

  int deteksiPIR = digitalRead(pinPIR);
  lastJarak = bacaJarak();
  long jarakTamu = lastJarak;

  // MUTE MODE
  if (muteTemporary) {
    if (jarakTamu > 100 || jarakTamu == 999) {
      if (objectGoneTime == 0) objectGoneTime = millis();
      if (millis() - objectGoneTime > 2000) {
        muteTemporary = false;
        objectGoneTime = 0;
        updateLCD(" SISTEM AKTIF ", " KEMBALI      ");
      }
    } else objectGoneTime = 0;
    delay(10);
    return;
  }

  // ====================== PIR - RISING EDGE (HANYA SEKALI SAAT GERAKAN BARU) ======================
  static int pirDebounce = 0;
  static unsigned long lastPirTrigger = 0;
  static bool previousMotion = false;

  bool currentMotion = false;

  // Debounce
  if (pirEnable && deteksiPIR == HIGH) {
    pirDebounce++;
    if (pirDebounce >= 6) currentMotion = true;
  } else {
    pirDebounce = 0;
    currentMotion = false;
  }

  // Hanya trigger saat gerakan BARU MULAI (Rising Edge)
  if (currentMotion && !previousMotion && 
      (millis() - lastPirTrigger > 6000)) {     // cooldown 6 detik

    lastPirTrigger = millis();
    lastPirDetect = millis();

    Serial.println("=== GERAKAN TERDETEKSI ===");

    if (lcdEnable) updateLCD(" MOTION DETECT ", " SENDING PHOTO ");
    
    if (buzzerEnable) digitalWrite(pinBuzzer, HIGH);
    if (ledEnable)    digitalWrite(pinLED, HIGH);

    if (camEnable) { 
      updateFlash();
      Serial2.write('S'); 
    }

    mqtt.publish(TOPIC_PIR, "GERAKAN TERDETEKSI");

    delay(300);
    digitalWrite(pinBuzzer, LOW);
    digitalWrite(pinLED, LOW);
    
    pirLcdClearTime = millis();
  }

  previousMotion = currentMotion;   // simpan state untuk next loop

  // Clear LCD setelah 3 detik
  if (pirLcdClearTime > 0 && millis() - pirLcdClearTime > 3000) {
    if (lcdEnable) {
      lcd.clear();
      lastLine1 = "";
      lastLine2 = "";
    }
    pirLcdClearTime = 0;
  }

  // ====================== ULTRASONIC ======================
  static unsigned long lastHCSR = 0;
  if (hcsrEnable && millis() - lastHCSR >= 120) {
    lastHCSR = millis();
    long bacaan = bacaJarak();
    if (bacaan != 999) {
      lastJarak = bacaan;
      jarakTamu = bacaan;
    }

    if (jarakTamu >= 2 && jarakTamu <= 100) {
      Serial.print("Jarak Tamu: ");
      Serial.print(jarakTamu);
      Serial.println(" cm");
    }
  }

  // ====================== MODE TAMU ======================
  if (hcsrEnable && jarakTamu >= 2 && jarakTamu <= 100) {
    hcsrConfirmCount++;

    if (hcsrConfirmCount >= HCSR_CONFIRM_THRESHOLD) {
      int interval = map(jarakTamu, 2, 100, 50, 600);

      if (millis() - lastBuzzerMillis >= interval) {
        lastBuzzerMillis = millis();
        toggleState = !toggleState;
        if (buzzerEnable) digitalWrite(pinBuzzer, toggleState);
        if (ledEnable)    digitalWrite(pinLED, toggleState);
      }

      if (lcdEnable) {
        static unsigned long lastTamuLCD = 0;
        if (millis() - lastTamuLCD >= 400) {
          updateLCD("  TAMU DATANG  ", " JARAK:" + String(jarakTamu) + "cm ");
          lastTamuLCD = millis();
        }
      }
      delay(10);
      return;               // JANGAN tampilkan SCANNING
    }
  } 
  else {
    hcsrConfirmCount = 0;
  }

  // ====================== STANDBY ======================
  digitalWrite(pinBuzzer, LOW);
  digitalWrite(pinLED, LOW);

  // Hanya tampilkan SCANNING kalau benar-benar tidak ada objek
  if (lcdEnable && !(lastJarak >= 2 && lastJarak <= 100)) {
    static unsigned long lastScanLCD = 0;
    if (millis() - lastScanLCD >= 900) {
      updateLCD("   SMART BELL ", "   SCANNING... ");
      lastScanLCD = millis();
    }
  }

  delay(5);
}