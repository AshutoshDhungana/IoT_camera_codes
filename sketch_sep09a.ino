#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h> 


// Define the SSID and password for the ESP32 access point
const char* ssid = "ESP32-Camera";
const char* password = "12345678";
const char* targetIP = "http://<TARGET_IP_ADDRESS>/upload";  // Change this to your target IP and endpoint


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

// Declare the server globally
WiFiServer server(80);

void startCameraServer();

void setup() {
  Serial.begin(115200);
  
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("ESP32 IP Address: ");
  Serial.println(IP);

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

  // Optimize frame size and quality
  config.frame_size = FRAMESIZE_SXGA;
  config.jpeg_quality = 12;
  config.fb_count = 2;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Apply grayscale effect and other optimizations
  sensor_t * s = esp_camera_sensor_get();
  s->set_special_effect(s, 2);
  s->set_framesize(s, FRAMESIZE_SXGA);
  s->set_quality(s, 12);
  s->set_brightness(s, 0);
  s->set_contrast(s, 0);
  s->set_saturation(s, -2);
  s->set_whitebal(s, 1);
  s->set_awb_gain(s, 1);
  s->set_wb_mode(s, 0);
  s->set_exposure_ctrl(s, 1);
  s->set_aec2(s, 0);
  s->set_gain_ctrl(s, 1);
  s->set_agc_gain(s, 0);
  s->set_gainceiling(s, (gainceiling_t)0);
  s->set_bpc(s, 0);
  s->set_wpc(s, 1);
  s->set_raw_gma(s, 1);
  s->set_lenc(s, 1);
  s->set_hmirror(s, 0);
  s->set_vflip(s, 0);
  s->set_dcw(s, 1);
  s->set_colorbar(s, 0);

  startCameraServer();
  
  Serial.println("Camera ready! Connect to the ESP32's Wi-Fi network and go to the IP address in the browser.");
}

void loop() {
    // Handle client connections for video streaming and capturing
    WiFiClient client = server.available();
    
    if (client) {
        String request = client.readStringUntil('\r\n');
        Serial.println(request);
        
        // Check if the request is to capture an image
        if (request.indexOf("/capture") != -1) {
            camera_fb_t * fb = esp_camera_fb_get();
            if (!fb) {
                Serial.println("Camera capture failed");
                client.println("HTTP/1.1 500 Internal Server Error");
                client.println("Content-Type: text/plain");
                client.println("Content-Length: 0");
                client.println();
            } else {
                client.println("HTTP/1.1 200 OK");
                client.println("Content-Type: image/jpeg");
                client.println("Content-Length: " + String(fb->len));
                client.println();
                client.write(fb->buf, fb->len);
                
                esp_camera_fb_return(fb);
            }
        } 
        // Otherwise, serve the video stream
        else {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
            client.println();
            
            while (client.connected()) {
                camera_fb_t * fb = esp_camera_fb_get();
                if (!fb) {
                    Serial.println("Camera capture failed");
                    break;
                }
                
                client.println("--frame");
                client.println("Content-Type: image/jpeg");
                client.println("Content-Length: " + String(fb->len));
                client.println();
                client.write(fb->buf, fb->len);
                client.println();
                
                esp_camera_fb_return(fb);
                
                delay(10); // Adjust to control frame rate
            }
        }
        
        client.stop();
        Serial.println("Client disconnected");
    }
}

void startCameraServer() {
    server.begin();  // Use the globally declared server
  
    Serial.println("Camera server started");
}
