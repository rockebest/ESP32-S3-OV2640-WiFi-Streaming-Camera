#include <WiFi.h>
#include <WiFiUdp.h>
#include "esp_camera.h"

// =====================================================
// Wi-Fi settings
// Use a 2.4 GHz Wi-Fi network.
// Do NOT commit your real Wi-Fi password to a public repository.
// =====================================================
const char* ssid = "YOUR_2_4GHz_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// =====================================================
// Network ports
// =====================================================
const int CAMERA_PORT = 3333;
const int DISCOVERY_PORT = 4210;

const char* DISCOVERY_MESSAGE = "DISCOVER_ESP32_CAM";

WiFiServer server(CAMERA_PORT);
WiFiUDP udp;

// =====================================================
// ESP32-S3 WROOM CAM + OV2640 pin map
// This pin map is commonly used on Freenove ESP32-S3 WROOM CAM boards.
// If your board is different, adjust the pin definitions.
// =====================================================
#define CAM_PIN_PWDN   -1
#define CAM_PIN_RESET  -1

#define CAM_PIN_XCLK   15
#define CAM_PIN_SIOD   4
#define CAM_PIN_SIOC   5

#define CAM_PIN_D0     11   // Y2
#define CAM_PIN_D1     9    // Y3
#define CAM_PIN_D2     8    // Y4
#define CAM_PIN_D3     10   // Y5
#define CAM_PIN_D4     12   // Y6
#define CAM_PIN_D5     18   // Y7
#define CAM_PIN_D6     17   // Y8
#define CAM_PIN_D7     16   // Y9

#define CAM_PIN_VSYNC  6
#define CAM_PIN_HREF   7
#define CAM_PIN_PCLK   13

// =====================================================
// Camera initialization
// =====================================================
void initCamera() {
  camera_config_t config;
  memset(&config, 0, sizeof(config));

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;

  config.pin_pwdn     = CAM_PIN_PWDN;
  config.pin_reset    = CAM_PIN_RESET;
  config.pin_xclk     = CAM_PIN_XCLK;
  config.pin_sccb_sda = CAM_PIN_SIOD;
  config.pin_sccb_scl = CAM_PIN_SIOC;

  config.pin_d0       = CAM_PIN_D0;
  config.pin_d1       = CAM_PIN_D1;
  config.pin_d2       = CAM_PIN_D2;
  config.pin_d3       = CAM_PIN_D3;
  config.pin_d4       = CAM_PIN_D4;
  config.pin_d5       = CAM_PIN_D5;
  config.pin_d6       = CAM_PIN_D6;
  config.pin_d7       = CAM_PIN_D7;

  config.pin_vsync    = CAM_PIN_VSYNC;
  config.pin_href     = CAM_PIN_HREF;
  config.pin_pclk     = CAM_PIN_PCLK;

  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    Serial.println("PSRAM found.");

    config.frame_size   = FRAMESIZE_QVGA;   // 320x240
    config.jpeg_quality = 10;               // lower = better quality
    config.fb_count     = 2;
    config.fb_location  = CAMERA_FB_IN_PSRAM;
    config.grab_mode    = CAMERA_GRAB_LATEST;
  } else {
    Serial.println("PSRAM not found.");

    config.frame_size   = FRAMESIZE_QQVGA;  // 160x120
    config.jpeg_quality = 14;
    config.fb_count     = 1;
    config.fb_location  = CAMERA_FB_IN_DRAM;
    config.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;
  }

  esp_err_t err = esp_camera_init(&config);

  if (err != ESP_OK) {
    Serial.print("Camera init failed: 0x");
    Serial.println(err, HEX);

    while (true) {
      delay(1000);
    }
  }

  Serial.println("Camera init OK.");

  sensor_t *s = esp_camera_sensor_get();

  if (s) {
    s->set_vflip(s, 0);
    s->set_hmirror(s, 0);

    // Basic image tuning
    s->set_brightness(s, 1);   // -2 to 2
    s->set_contrast(s, 1);     // -2 to 2
    s->set_saturation(s, 1);   // -2 to 2

    s->set_whitebal(s, 1);
    s->set_awb_gain(s, 1);
    s->set_exposure_ctrl(s, 1);
    s->set_gain_ctrl(s, 1);
    s->set_lenc(s, 1);
  }
}

// =====================================================
// Write all bytes to WiFiClient
// =====================================================
bool writeAll(WiFiClient &client, const uint8_t *data, size_t len) {
  size_t sent = 0;

  while (sent < len) {
    if (!client.connected()) {
      return false;
    }

    size_t n = client.write(data + sent, len - sent);

    if (n == 0) {
      delay(1);
      continue;
    }

    sent += n;
  }

  return true;
}

// =====================================================
// Send one JPEG frame
// Stream format:
// "FRAM" + 4-byte little-endian JPEG size + JPEG binary data
// =====================================================
bool sendFrame(WiFiClient &client) {
  camera_fb_t *fb = esp_camera_fb_get();

  if (!fb) {
    Serial.println("Camera capture failed.");
    return false;
  }

  if (fb->format != PIXFORMAT_JPEG) {
    Serial.println("Frame is not JPEG.");
    esp_camera_fb_return(fb);
    return false;
  }

  uint32_t len = fb->len;
  bool ok = true;

  ok &= writeAll(client, (const uint8_t *)"FRAM", 4);
  ok &= writeAll(client, (const uint8_t *)&len, 4);
  ok &= writeAll(client, fb->buf, fb->len);

  esp_camera_fb_return(fb);

  return ok;
}

// =====================================================
// UDP Discovery
// Python sends: "DISCOVER_ESP32_CAM"
// ESP32 replies: "ESP32CAM,<ip>,<port>"
// =====================================================
void handleDiscovery() {
  int packetSize = udp.parsePacket();

  if (packetSize <= 0) {
    return;
  }

  char packet[128];
  int len = udp.read(packet, sizeof(packet) - 1);

  if (len <= 0) {
    return;
  }

  packet[len] = '\0';

  String message = String(packet);
  message.trim();

  Serial.print("UDP packet received: ");
  Serial.println(message);

  if (message == DISCOVERY_MESSAGE) {
    String response = "ESP32CAM,";
    response += WiFi.localIP().toString();
    response += ",";
    response += String(CAMERA_PORT);

    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.print(response);
    udp.endPacket();

    Serial.print("Discovery response sent: ");
    Serial.println(response);
  }
}

// =====================================================
// Wi-Fi connection
// =====================================================
void connectWiFi() {
  WiFi.mode(WIFI_STA);

  // Disable Wi-Fi sleep for more stable streaming
  WiFi.setSleep(false);

  WiFi.begin(ssid, password);

  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Wi-Fi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// =====================================================
// setup
// =====================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("ESP32-S3 OV2640 Wi-Fi Camera Starting...");

  initCamera();
  connectWiFi();

  server.begin();
  udp.begin(DISCOVERY_PORT);

  Serial.println("Camera TCP server started.");
  Serial.print("Camera TCP port: ");
  Serial.println(CAMERA_PORT);

  Serial.println("UDP discovery started.");
  Serial.print("Discovery UDP port: ");
  Serial.println(DISCOVERY_PORT);

  Serial.println("Ready.");
}

// =====================================================
// loop
// =====================================================
void loop() {
  handleDiscovery();

  WiFiClient client = server.available();

  if (client) {
    Serial.println("Python client connected.");

    while (client.connected()) {
      handleDiscovery();

      bool ok = sendFrame(client);

      if (!ok) {
        Serial.println("Frame send failed.");
        break;
      }

      // FPS control. Smaller delay = faster but more network load.
      delay(10);
    }

    client.stop();
    Serial.println("Python client disconnected.");
  }
}
