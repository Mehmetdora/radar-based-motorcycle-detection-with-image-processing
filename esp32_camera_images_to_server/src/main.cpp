#include "Arduino.h"
#include "esp_camera.h"
#include <WiFi.h>

#define CAMERA_MODEL_XIAO_ESP32S3
const char* ssid     = "Dora";
const char* password = "mehmetdora";    
const char* host     = "172.20.10.2";   // server ip adresi
const uint16_t port  = 5001;

#define MAX_FRAMES          60
#define FRAME_INTERVAL_MS   45      // kareler arası bekleme (ms)
#define CAPTURE_MS          3000    // toplam süre



#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  10
#define SIOD_GPIO_NUM  40
#define SIOC_GPIO_NUM  39
#define Y9_GPIO_NUM    48
#define Y8_GPIO_NUM    11
#define Y7_GPIO_NUM    12
#define Y6_GPIO_NUM    14
#define Y5_GPIO_NUM    16
#define Y4_GPIO_NUM    18
#define Y3_GPIO_NUM    17
#define Y2_GPIO_NUM    15
#define VSYNC_GPIO_NUM 38
#define HREF_GPIO_NUM  47
#define PCLK_GPIO_NUM  13

// PSRAM'de frame buffer yapısı
struct Frame {
    uint8_t* buf;
    size_t   len;
};

Frame frames[MAX_FRAMES];
int frameCount = 0;

void initCamera() {
    camera_config_t config;
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
    config.frame_size   = FRAMESIZE_VGA;  // 640x480
    config.jpeg_quality = 12;
    config.fb_count     = 2;
    config.fb_location  = CAMERA_FB_IN_PSRAM;
    config.grab_mode    = CAMERA_GRAB_LATEST;

    if (esp_camera_init(&config) != ESP_OK) {
        Serial.println("Kamera başlatılamadı!");
        return;
    }
    Serial.println("Kamera hazır.");
}

void freeAllFrames() {
    for (int i = 0; i < frameCount; i++) {
        if (frames[i].buf) {
            free(frames[i].buf);
            frames[i].buf = nullptr;
            frames[i].len = 0;
        }
    }
    frameCount = 0;
}

void captureFrames() {
    freeAllFrames();

    // Önce buffer'ı temizle (eski kareler)
    for (int i = 0; i < 3; i++) {
        camera_fb_t* fb = esp_camera_fb_get();
        if (fb) esp_camera_fb_return(fb);
        delay(30);
    }

    Serial.println("Çekim başlıyor...");
    unsigned long start = millis();

    while ((millis() - start) < CAPTURE_MS && frameCount < MAX_FRAMES) {
        unsigned long frameStart = millis();

        camera_fb_t* fb = esp_camera_fb_get();
        if (!fb) continue;

        uint8_t* copy = (uint8_t*)ps_malloc(fb->len);
        if (copy) {
            memcpy(copy, fb->buf, fb->len);
            frames[frameCount].buf = copy;
            frames[frameCount].len = fb->len;
            frameCount++;
            Serial.printf("  Kare %d çekildi (%lu ms)\n", frameCount, millis() - start);
        }
        esp_camera_fb_return(fb);

        // Bir sonraki kareye kadar bekleniyor çünkü kareler arasında belli bir
        // zaman farkı olmalı , böylece 3 sn boyunca tam olarak aynı zaman aralıkları
        // ile resimler çekilecek
        long remaining = FRAME_INTERVAL_MS - (long)(millis() - frameStart);
        if (remaining > 0) delay(remaining);
    }

    Serial.printf("Çekim bitti: %d kare, %lu ms\n", frameCount, millis() - start);
}

void sendAllFrames() {
    if (frameCount == 0) {
        Serial.println("Gönderilecek kare yok.");
        return;
    }

    WiFiClient client;
    if (!client.connect(host, port)) {
        Serial.println("PC'ye bağlanılamadı!");
        return;
    }

    Serial.printf("%d kare gönderiliyor...\n", frameCount);

    // Toplam kare sayısını önce gönder
    uint8_t countByte = (uint8_t)frameCount;
    client.write(&countByte, 1);

    for (int i = 0; i < frameCount; i++) {
        // 4 byte boyut
        uint32_t size = frames[i].len;
        uint8_t header[4] = {
            (uint8_t)(size >> 24),
            (uint8_t)(size >> 16),
            (uint8_t)(size >> 8),
            (uint8_t)(size)
        };
        client.write(header, 4);

        // JPEG verisi
        size_t sent = 0;
        while (sent < frames[i].len) {
            size_t chunk = min((size_t)1024, frames[i].len - sent);
            client.write(frames[i].buf + sent, chunk);
            sent += chunk;
        }

        Serial.printf("  [%d/%d] %d byte gönderildi\n", i+1, frameCount, frames[i].len);
    }

    client.flush();
    client.stop();
    Serial.println("Tüm kareler gönderildi.");
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    WiFi.begin(ssid, password);
    Serial.print("WiFi bağlanıyor");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nBağlandı: " + WiFi.localIP().toString());

    initCamera();
    Serial.println("Hazır. manual tetikleme için serial'den c yaz.");
}

void loop() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        if (cmd == "c") {
            captureFrames();   // önceki buffer temizlenir, sonra 3 sn çekilir, bellekte tut
            sendAllFrames();   // sonra gönder,
        }
    }
}