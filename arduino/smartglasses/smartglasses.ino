#include <Arduino.h>
#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include "esp_wifi.h"

#include <Wire.h>
#include <Adafruit_VL53L1X.h>
#include <Adafruit_DRV2605.h>

// ========= YOUR HOME/HOTSPOT WIFI (STA MODE) =========
// (Use your own SSID/PASS here)
#define STA_SSID "My S24"
#define STA_PASS "freewifianywhereyougo"

// ========= FALLBACK SOFTAP (used only if STA fails) =========
static const char* AP_SSID = "SmartGlassesCam";
static const char* AP_PASS = "seeed123";   // ≥ 8 chars

// ======= Seeed XIAO ESP32S3 Sense (OV2640) pin map =======
#define PWDN_GPIO_NUM   -1
#define RESET_GPIO_NUM  -1
#define XCLK_GPIO_NUM   10
#define SIOD_GPIO_NUM   40
#define SIOC_GPIO_NUM   39
#define Y9_GPIO_NUM     48
#define Y8_GPIO_NUM     11
#define Y7_GPIO_NUM     12
#define Y6_GPIO_NUM     14
#define Y5_GPIO_NUM     16
#define Y4_GPIO_NUM     18
#define Y3_GPIO_NUM     17
#define Y2_GPIO_NUM     15
#define VSYNC_GPIO_NUM  38
#define HREF_GPIO_NUM   47
#define PCLK_GPIO_NUM   13
// =========================================================

// ------------ I2C pins for TOF + DRV2605 -----------------
// On the XIAO ESP32S3, D4/D5 are the header pins labelled SDA/SCL.
#define I2C_SDA D4
#define I2C_SCL D5

// ------------- TOF + HAPTIC GLOBALS ----------------------
Adafruit_VL53L1X tof = Adafruit_VL53L1X();
Adafruit_DRV2605 drv;

bool tof_ok = false;
bool drv_ok = false;

// latest distance & obstacle state used by /tof and haptics
int16_t last_distance_mm = -1;
bool obstacle_now = false;

// how close before we start buzzing (in mm)
const int OBSTACLE_MM = 800;   // ~0.8 m, tweak as you like

// timing
unsigned long last_tof_ms   = 0;
const unsigned long TOF_INTERVAL_MS = 60;  // check ~16 times/sec

unsigned long last_buzz_ms  = 0;

// ------------------ HTTP server --------------------------
WebServer server(80);

// ------------- *NEW* MJPEG Constants ---------------------
#define PART_BOUNDARY "123456789000000000000987654321"

static const char* _STREAM_CONTENT_TYPE =
    "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;

static const char* _STREAM_BOUNDARY =
    "\r\n--" PART_BOUNDARY "\r\n";

static const char* _STREAM_PART =
    "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

// =========================================================
//                  CAMERA SETUP
// =========================================================
bool init_camera() {
  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;

  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;

  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  config.frame_size   = FRAMESIZE_SVGA;   // 800x600
  config.jpeg_quality = 10;
  config.fb_count     = 2;               // single buffer

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%X\n", err);
    return false;
  }

  sensor_t* s = esp_camera_sensor_get();
  if (s) {
    s->set_framesize(s, config.frame_size);
    s->set_sharpness(s, 1);
    s->set_brightness(s, 0);
    s->set_contrast(s, 0);
    s->set_hmirror(s, 1);
    s->set_vflip(s, 1);
  }
  return true;
}

// ---------- *NEW* Multiple JPEG capture ---------------
 void handleStream() {
 sensor_t* s = esp_camera_sensor_get();
  if (s) {
    s->set_framesize(s, FRAMESIZE_QVGA); // 320x240
    s->set_quality(s, 12);               // faster, lighter
  }
//
  WiFiClient client = server.client();

  server.sendContent("HTTP/1.1 200 OK\r\n");
  server.sendContent("Content-Type: ");
  server.sendContent(_STREAM_CONTENT_TYPE);
  server.sendContent("\r\n");
  server.sendContent("Access-Control-Allow-Origin: *\r\n");
  server.sendContent("\r\n");
//
  while (client.connected()) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      break;
    }

    server.sendContent(_STREAM_BOUNDARY);

    char header[64];
    snprintf(header, sizeof(header), _STREAM_PART, fb->len);
    server.sendContent(header);

    client.write(fb->buf, fb->len);
    esp_camera_fb_return(fb);

    delay(30); // ~30 FPS max, adjust as needed
  }
}

// ---------- Single JPEG capture (header-safe) ----------
void handle_capture() {
  sensor_t* s = esp_camera_sensor_get();
     if (s) {
         s->set_framesize(s, FRAMESIZE_SVGA); // back to high-res
         s->set_quality(s, 10);               // your chosen quality
     }


  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    server.send(500, "text/plain", "Camera capture failed");
    return;
  }

  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "0");
  server.send_P(200, "image/jpeg",
                reinterpret_cast<const char*>(fb->buf),
                fb->len);
  esp_camera_fb_return(fb);
}

// =========================================================
//                 TOF + HAPTIC SETUP
// =========================================================
void init_tof_and_haptics() {
  // Start I2C
  Wire.begin(I2C_SDA, I2C_SCL);

  // -------- TOF --------
  Serial.println("Init VL53L1X...");
  if (!tof.begin(0x29, &Wire)) {
    Serial.println("VL53L1X NOT FOUND");
    tof_ok = false;
  } else {
    tof_ok = true;
    tof.setTimingBudget(50);   // this IS supported
    tof.startRanging();
    Serial.println("VL53L1X OK");
  }

  // -------- HAPTICS --------
  Serial.println("Init DRV2605...");
  if (!drv.begin()) {
    Serial.println("DRV2605 NOT FOUND");
    drv_ok = false;
  } else {
    drv_ok = true;
    drv.selectLibrary(1);
    drv.setMode(DRV2605_MODE_INTTRIG);
    drv.setWaveform(0, 1);   // strong click
    drv.setWaveform(1, 0);
    Serial.println("DRV2605 OK");
  }
}


// =========================================================
//          TOF MEASUREMENT + HAPTIC CONTROL
// =========================================================
void update_tof_and_haptics() {
  if (!tof_ok) return;

  unsigned long now = millis();
  if (now - last_tof_ms < TOF_INTERVAL_MS) return;
  last_tof_ms = now;

  // Non-blocking: only read when data ready
  if (!tof.dataReady()) return;

  int16_t dist = tof.distance();
  tof.clearInterrupt();

  last_distance_mm = dist;

  bool new_obstacle = (dist > 0 && dist < OBSTACLE_MM);
  obstacle_now = new_obstacle;

  if (!drv_ok) return;

  if (!new_obstacle) {
    // Nothing near: no buzzing
    return;
  }

  // Map distance to buzz interval: closer = faster buzz
  // Clamp distance between 100mm and OBSTACLE_MM
  int16_t d_clamped = dist;
  if (d_clamped < 100) d_clamped = 100;
  if (d_clamped > OBSTACLE_MM) d_clamped = OBSTACLE_MM;

  const unsigned long MIN_BUZZ_MS = 80;   // very close
  const unsigned long MAX_BUZZ_MS = 600;  // near threshold

  unsigned long interval = map(d_clamped,
                               100, OBSTACLE_MM,
                               MIN_BUZZ_MS, MAX_BUZZ_MS);

  if (now - last_buzz_ms >= interval) {
    drv.go();                // fire one haptic effect
    last_buzz_ms = now;
  }
}

// =========================================================
//                     HTTP HANDLERS
// =========================================================
void handle_root() {
  String html;
  html  = "SmartGlassesCam\r\n\r\nEndpoints:\r\n";
  html += "/capture  - single JPEG\r\n";
  html += "/stream   - multipart MJPEG\r\n";
  html += "/net      - mode & IP\r\n";
  html += "/tof      - current distance JSON\r\n";
  server.send(200, "text/plain", html);
}

void handle_net() {
  bool sta = (WiFi.getMode() & WIFI_MODE_STA) && (WiFi.status() == WL_CONNECTED);
  IPAddress ip = sta ? WiFi.localIP() : WiFi.softAPIP();
  String html = String("Mode: ") + (sta ? "STA" : "AP") + "\nIP: " + ip.toString();
  server.send(200, "text/plain", html);
}

void handle_tof_http() {
  if (!tof_ok) {
    server.send(500, "application/json",
                "{\"error\":\"tof_not_initialized\"}");
    return;
  }

  char buf[96];
  snprintf(buf, sizeof(buf),
           "{\"distance_mm\":%d,\"obstacle\":%s}",
           (int)last_distance_mm,
           obstacle_now ? "true" : "false");
  server.send(200, "application/json", buf);
}

// =========================================================
//                       SETUP / LOOP
// =========================================================
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\nStarting SmartGlassesCam…");

  if (!psramFound()) {
    Serial.println("WARNING: PSRAM not found. Enable OPI PSRAM in Tools menu!");
  }

  if (!init_camera()) {
    Serial.println("Fatal: camera init failed");
    while (true) delay(1000);
  }
  Serial.println("Camera init OK");

  // TOF + HAPTIC
  init_tof_and_haptics();

  // ---------- Prefer STA; fall back to AP ----------
  Serial.println("Connecting to Wi-Fi (STA)...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(STA_SSID, STA_PASS);

  unsigned long t0 = millis();
  const unsigned long TIMEOUT = 10000; // 10s
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < TIMEOUT) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("STA connected. ESP IP: ");
    Serial.println(WiFi.localIP());
    WiFi.setSleep(true);
    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    esp_wifi_set_max_tx_power(40); // ~10 dBm
  } else {
    Serial.println("STA connect failed → starting fallback AP.");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    IPAddress ip = WiFi.softAPIP();
    Serial.print("AP SSID: "); Serial.println(AP_SSID);
    Serial.print("AP IP:   "); Serial.println(ip);
  }

  // ---------- Routes ----------
  server.on("/", handle_root);
  server.on("/net", HTTP_GET, handle_net);
  server.on("/capture", HTTP_GET, handle_capture);
  // NEW
  server.on("/stream", HTTP_GET, handleStream);
  server.on("/tof", HTTP_GET, handle_tof_http);

  server.begin();

  Serial.println("HTTP server started");
  Serial.println("Use /capture for stills, /net for status, /tof for distance.");
}

void loop() {
  server.handleClient();
  update_tof_and_haptics();   // run frequently
}