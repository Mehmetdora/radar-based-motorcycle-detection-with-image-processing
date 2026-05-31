#include "Arduino.h"
#include "esp_camera.h"
#include "HardwareSerial.h"
#include <WiFi.h>

#define CAMERA_MODEL_XIAO_ESP32S3
const char* ssid     = "Dora";
const char* password = "mehmetdora";    
const char* host     = "172.20.10.2";   // server ip adresi
const uint16_t port  = 5001;

#define MAX_FRAMES          60
#define FRAME_INTERVAL_MS   45      // kareler arası bekleme (ms)
#define CAPTURE_MS          3000    // toplam süre

#define RADAR_UART_BAUD     115200
#define RADAR_UART_RX_PIN   44      // STM32 USART2 TX (PA2) bu pine bağlanmalı
#define RADAR_UART_TX_PIN   43      // Gerekirse STM32 RX tarafına bağlanabilir
#define RADAR_LINE_MAX_LEN  128

static const char* IMAGE_PROTOCOL_MAGIC = "RDR1";

HardwareSerial RadarSerial(1);


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

struct RadarMetadata {
    String rawLine;
    String objectClass;
    bool motionDetected;
    float signalPower;
    float peakFrequencyHz;
    float estimatedSpeedKmh;
};

char radarLineBuffer[RADAR_LINE_MAX_LEN];
uint8_t radarLineIndex = 0;

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

String jsonEscape(const String& value) {
    String escaped;
    escaped.reserve(value.length() + 8);

    for (size_t i = 0; i < value.length(); i++) {
        char c = value.charAt(i);
        switch (c) {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                if ((uint8_t)c < 0x20) {
                    char encoded[7];
                    snprintf(encoded, sizeof(encoded), "\\u%04X", (uint8_t)c);
                    escaped += encoded;
                } else {
                    escaped += c;
                }
                break;
        }
    }

    return escaped;
}

String buildRadarMetadataJson(const RadarMetadata& radar) {
    String json;
    json.reserve(220);

    json += "{";
    json += "\"source\":\"STM32_HB100\",";
    json += "\"raw\":\"" + jsonEscape(radar.rawLine) + "\",";
    json += "\"obj\":\"" + jsonEscape(radar.objectClass) + "\",";
    json += "\"motion\":";
    json += radar.motionDetected ? "1" : "0";
    json += ",\"power\":";
    json += String(radar.signalPower, 2);
    json += ",\"freq_hz\":";
    json += String(radar.peakFrequencyHz, 2);
    json += ",\"speed_kmh\":";
    json += String(radar.estimatedSpeedKmh, 2);
    json += "}";

    return json;
}

void writeUint16BE(WiFiClient& client, uint16_t value) {
    uint8_t bytes[2] = {
        (uint8_t)(value >> 8),
        (uint8_t)(value)
    };
    client.write(bytes, sizeof(bytes));
}

void sendAllFrames(const RadarMetadata& radar) {
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

    String radarJson = buildRadarMetadataJson(radar);
    if (radarJson.length() > UINT16_MAX) {
        Serial.println("Radar metadata çok uzun, gönderim iptal edildi.");
        client.stop();
        return;
    }

    client.write((const uint8_t*)IMAGE_PROTOCOL_MAGIC, 4);
    writeUint16BE(client, (uint16_t)radarJson.length());
    client.write((const uint8_t*)radarJson.c_str(), radarJson.length());

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

bool parseRadarMessage(String line, RadarMetadata& radar) {
    line.trim();
    if (!line.startsWith("HB100")) {
        return false;
    }

    radar.rawLine = line;
    radar.objectClass = "";
    radar.motionDetected = false;
    radar.signalPower = 0.0f;
    radar.peakFrequencyHz = 0.0f;
    radar.estimatedSpeedKmh = 0.0f;

    bool hasObjectClass = false;
    bool hasMotion = false;

    int start = 0;
    while (start < line.length()) {
        int end = line.indexOf(',', start);
        if (end < 0) {
            end = line.length();
        }

        String token = line.substring(start, end);
        token.trim();

        int separator = token.indexOf('=');
        if (separator > 0) {
            String key = token.substring(0, separator);
            String value = token.substring(separator + 1);
            key.trim();
            value.trim();

            if (key == "OBJ") {
                radar.objectClass = value;
                hasObjectClass = true;
            } else if (key == "MOTION") {
                radar.motionDetected = (value.toInt() != 0);
                hasMotion = true;
            } else if (key == "POWER") {
                radar.signalPower = value.toFloat();
            } else if (key == "FREQ_HZ") {
                radar.peakFrequencyHz = value.toFloat();
            } else if (key == "SPEED_KMH") {
                radar.estimatedSpeedKmh = value.toFloat();
            }
        }

        start = end + 1;
    }

    return hasObjectClass && hasMotion;
}

void handleRadarLine(const char* line) {
    RadarMetadata radar;

    if (!parseRadarMessage(String(line), radar)) {
        Serial.print("Geçersiz radar UART mesajı: ");
        Serial.println(line);
        return;
    }

    if (!radar.motionDetected) {
        Serial.print("Hareket yok, çekim başlatılmadı: ");
        Serial.println(radar.rawLine);
        return;
    }

    Serial.print("Radar tetikledi: ");
    Serial.println(radar.rawLine);

    captureFrames();
    sendAllFrames(radar);
}

void pollRadarUart() {
    while (RadarSerial.available()) {
        char c = (char)RadarSerial.read();

        if (c == '\r') {
            continue;
        }

        if (c == '\n') {
            if (radarLineIndex > 0) {
                radarLineBuffer[radarLineIndex] = '\0';
                handleRadarLine(radarLineBuffer);
                radarLineIndex = 0;
            }
            continue;
        }

        if (radarLineIndex < (RADAR_LINE_MAX_LEN - 1)) {
            radarLineBuffer[radarLineIndex++] = c;
        } else {
            radarLineIndex = 0;
            Serial.println("Radar UART satırı çok uzun, buffer temizlendi.");
        }
    }
}

void setup() {
    Serial.begin(115200);
    RadarSerial.begin(RADAR_UART_BAUD, SERIAL_8N1, RADAR_UART_RX_PIN, RADAR_UART_TX_PIN);
    delay(1000);

    WiFi.begin(ssid, password);
    Serial.print("WiFi bağlanıyor");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nBağlandı: " + WiFi.localIP().toString());

    initCamera();
    Serial.println("Hazır. STM32 HB100 UART mesajı bekleniyor.");
}

void loop() {
    pollRadarUart();
}
