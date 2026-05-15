// ======================================================
// ESP32-CAM TELEGRAM REALTIME FAST VERSION
// FLASH DIMATIKAN
// REALTIME: Hapus delay blocking
// ======================================================

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// ======================================================
// WIFI
// ======================================================
const char* ssid     = "Guik guik piyaw piyaw";
const char* password = "12345678";

// ======================================================
// TELEGRAM
// ======================================================
String BOT_TOKEN = "8724613363:AAFafZ2MfEeztAUb0_9ZyDiTfPP6UgYqd9U";
String CHAT_ID   = "1353775639";
long lastUpdateId = 0;
unsigned long lastTelegramPoll = 0;
const unsigned long TELEGRAM_INTERVAL = 1500;
// ======================================================
// AI THINKER ESP32-CAM PIN
// ======================================================
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5

#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define RED_LED           33
#define FLASH_LED          4

WiFiClientSecure client;

// ======================================================
// WIFI CONNECT
// ======================================================
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

// ======================================================
// CAMERA INIT
// ======================================================
bool initCamera() {

  camera_config_t config;

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;

  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;

  config.pin_xclk = XCLK_GPIO_NUM;

  config.pin_pclk  = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href  = HREF_GPIO_NUM;

  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;

  config.pin_pwdn  = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // ======================================================
  // FAST REALTIME SETTINGS
  // ======================================================
  config.frame_size   = FRAMESIZE_QVGA;
  config.jpeg_quality = 18;
  config.fb_count     = 2;
  config.grab_mode    = CAMERA_GRAB_LATEST;

  esp_err_t err = esp_camera_init(&config);

  if (err != ESP_OK) {

    Serial.print("Camera Init Failed: ");
    Serial.println(err);

    return false;
  }

  sensor_t * s = esp_camera_sensor_get();

  s->set_framesize(s, FRAMESIZE_QVGA);

  s->set_vflip(s, 0);
  s->set_hmirror(s, 0);

  s->set_brightness(s, 0);
  s->set_contrast(s, 0);
  s->set_saturation(s, 0);

  Serial.println("Camera Ready!");

  return true;
}

// ======================================================
// SEND PHOTO TELEGRAM
// ======================================================
void sendPhotoTelegram() {

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println("WiFi Disconnect!");
    connectWiFi();
  }

  // ======================================================
  // AMBIL FOTO LANGSUNG TANPA FLASH
  // ======================================================
  camera_fb_t * fb = esp_camera_fb_get();

  if (!fb) {

    Serial.println("Gagal mengambil foto!");
    return;
  }

  Serial.println("Foto berhasil diambil");

  digitalWrite(RED_LED, LOW);

  client.stop();
  client.setInsecure();

  Serial.println("Connect Telegram...");

  if (!client.connect("api.telegram.org", 443)) {

    Serial.println("Gagal connect Telegram!");

    esp_camera_fb_return(fb);

    digitalWrite(RED_LED, HIGH);
    digitalWrite(FLASH_LED, LOW);
    return;
  }

  Serial.println("Telegram Connected!");

  // ======================================================
  // FORM DATA
  // ======================================================
  String head = "--Boundary\r\n";

  head += "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n";
  head += CHAT_ID + "\r\n";

  head += "--Boundary\r\n";
  head += "Content-Disposition: form-data; name=\"caption\"\r\n\r\n";
  head += "GERAKAN TERDETEKSI!\r\n";

  head += "--Boundary\r\n";
  head += "Content-Disposition: form-data; name=\"photo\"; filename=\"esp32cam.jpg\"\r\n";
  head += "Content-Type: image/jpeg\r\n\r\n";

  String tail = "\r\n--Boundary--\r\n";

  uint32_t totalLen = head.length() + fb->len + tail.length();

  // ======================================================
  // HTTP REQUEST
  // ======================================================
  client.println("POST /bot" + BOT_TOKEN + "/sendPhoto HTTP/1.1");
  client.println("Host: api.telegram.org");
  client.println("Content-Length: " + String(totalLen));
  client.println("Content-Type: multipart/form-data; boundary=Boundary");
  client.println("Connection: close");
  client.println();

  // HEAD
  client.print(head);

  // ======================================================
  // SEND IMAGE FAST
  // ======================================================
  uint8_t *fbBuf = fb->buf;
  size_t fbLen = fb->len;

  Serial.println("Mengirim foto...");

  for (size_t n = 0; n < fbLen; n += 2048) {

    size_t chunkSize;

    if ((n + 2048) < fbLen) {
      chunkSize = 2048;
    } else {
      chunkSize = fbLen - n;
    }

    client.write(fbBuf + n, chunkSize);
  }

  // TAIL
  client.print(tail);

  // ======================================================
  // RESPONSE
  // ======================================================
  String response = "";

  long timeout = millis();

  while ((millis() - timeout) < 8000) {

    while (client.available()) {

      char c = client.read();

      response += c;
    }

    if (!client.connected()) {
      break;
    }

    delay(1);
  }

  Serial.println("================ RESPONSE ================");
  Serial.println(response);
  Serial.println("==========================================");

  if (response.indexOf("\"ok\":true") >= 0) {

    Serial.println("FOTO BERHASIL DIKIRIM!");

  } else {

    Serial.println("FOTO GAGAL DIKIRIM!");
  }

  // ======================================================
  // RELEASE BUFFER
  // ======================================================
  esp_camera_fb_return(fb);

  client.stop();

  digitalWrite(RED_LED, HIGH);

  Serial.println("Selesai.");
}

void checkTelegramCommand() {

  client.stop();
  client.setInsecure();

  if (!client.connect("api.telegram.org", 443)) return;

  String url = "/bot" + BOT_TOKEN +
               "/getUpdates?offset=" +
               String(lastUpdateId + 1) +
               "&limit=1";

  client.println("GET " + url + " HTTP/1.1");
  client.println("Host: api.telegram.org");
  client.println("Connection: close");
  client.println();

  String response = "";

  unsigned long timeout = millis();

  while (millis() - timeout < 1500) {

    while (client.available()) {
      response += (char)client.read();
    }

    if (!client.connected()) break;

    yield();
  }

  client.stop();

  if (response.indexOf("\"result\":[]") != -1 ||
      response.length() == 0) return;

  int idx = response.indexOf("\"update_id\":");

  if (idx != -1) {
    int start = idx + 12;
    int end = response.indexOf(",", start);

    if (end != -1)
      lastUpdateId = response.substring(start, end).toInt();
  }

  if (response.indexOf("/buzzer_on") != -1) Serial.println("/buzzer_on");
  else if (response.indexOf("/buzzer_off") != -1) Serial.println("/buzzer_off");

  else if (response.indexOf("/led_on") != -1) Serial.println("/led_on");
  else if (response.indexOf("/led_off") != -1) Serial.println("/led_off");

  else if (response.indexOf("/lcd_on") != -1) Serial.println("/lcd_on");
  else if (response.indexOf("/lcd_off") != -1) Serial.println("/lcd_off");

  else if (response.indexOf("/cam_on") != -1) Serial.println("/cam_on");
  else if (response.indexOf("/cam_off") != -1) Serial.println("/cam_off");

  else if (response.indexOf("/flash_on") != -1) Serial.println("F");
  else if (response.indexOf("/flash_off") != -1) Serial.println("f");

  else if (response.indexOf("/pir_on") != -1) Serial.println("/pir_on");
  else if (response.indexOf("/pir_off") != -1) Serial.println("/pir_off");

  else if (response.indexOf("/hcsr_on") != -1) Serial.println("/hcsr_on");
  else if (response.indexOf("/hcsr_off") != -1) Serial.println("/hcsr_off");
}
// ======================================================
// SETUP
// ======================================================
void setup() {

  // MATIKAN BROWNOUT
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);

  pinMode(RED_LED, OUTPUT);
  pinMode(FLASH_LED, OUTPUT);

  // ACTIVE LOW
  digitalWrite(RED_LED, HIGH);

  delay(1000);

  Serial.println();
  Serial.println("BOOTING ESP32-CAM");

  // ======================================================
  // CAMERA
  // ====================================================== 
  if (!initCamera()) {

    Serial.println("Camera Error!");
    ESP.restart();
  }

  // ======================================================
  // WIFI
  // ======================================================
  connectWiFi();

  Serial.println("SYSTEM READY!");
}
// ======================================================
// LOOP — REALTIME + TELEGRAM COMMAND
// ======================================================
void loop() {

  // ===============================
  // CEK COMMAND TELEGRAM
  // ===============================
  static unsigned long lastTelegramPoll = 0;

  if (millis() - lastTelegramPoll >= 1500) {
    lastTelegramPoll = millis();
    checkTelegramCommand();
  }

  // ===============================
  // TERIMA DATA DARI ESP32 DEVKIT
  // ===============================
  if (Serial.available()) {

    char data = Serial.read();

    if (data == 'S') {

      Serial.println();
      Serial.println("=== DETEKSI GERAKAN ===");

      sendPhotoTelegram();
    }

    else if (data == 'F') {

      digitalWrite(FLASH_LED, HIGH);
      Serial.println("FLASH ON");
    }

    else if (data == 'f') {

      digitalWrite(FLASH_LED, LOW);
      Serial.println("FLASH OFF");
    }
  }

  yield();
}