#include "esp_camera.h"
#include <WiFi.h>

// Define the SSID and password for the ESP32 access point
const char* ssid = "ESP32-Camera";  // Name of the private network
const char* password = "12345678";  // Password for the private network

// Camera Pin Configuration for OV2640
#define PWDN_GPIO_NUM    32
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM    0
#define SIOD_GPIO_NUM    26
#define SIOC_GPIO_NUM    27
#define Y9_GPIO_NUM      35
#define Y8_GPIO_NUM      34
#define Y7_GPIO_NUM      39
#define Y6_GPIO_NUM      36
#define Y5_GPIO_NUM      21
#define Y4_GPIO_NUM      19
#define Y3_GPIO_NUM      18
#define Y2_GPIO_NUM      5
#define VSYNC_GPIO_NUM   25
#define HREF_GPIO_NUM    23
#define PCLK_GPIO_NUM    22

void startCameraServer();

void setup() {
  Serial.begin(115200);

  // Initialize ESP32 as Access Point
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("ESP32 IP Address: ");
  Serial.println(IP);

  // Configure camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Frame settings
  if(psramFound()){
    config.frame_size = FRAMESIZE_VGA; // 640x480
    config.jpeg_quality = 10; //10-63 (lower means better quality)
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_CIF; // 320x240
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Start the camera server
  startCameraServer();
  
  Serial.println("Camera ready! Connect to the ESP32's Wi-Fi network and go to the IP address in the browser.");
}

void loop() {
  delay(10000); // Nothing to do in the loop
}

// Serve MJPEG stream
void startCameraServer(){
  WiFiServer server(80);

  server.begin();
  while(true) {
    WiFiClient client = server.available();
    if (client) {
      Serial.println("New client connected");
      String header = "HTTP/1.1 200 OK\r\nContent-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
      client.print(header);

      while (client.connected()) {
        camera_fb_t * fb = esp_camera_fb_get();
        if (!fb) {
          Serial.println("Camera capture failed");
          continue;
        }

        client.printf("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %d\r\n\r\n", fb->len);
        client.write(fb->buf, fb->len);
        client.printf("\r\n");

        esp_camera_fb_return(fb);
        delay(30); // Control frame rate
      }
      client.stop();
      Serial.println("Client disconnected");
    }
  }
}
