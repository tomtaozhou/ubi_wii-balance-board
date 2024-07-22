#include <M5Stack.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "Wiimote.h"

// Wi-Fi配置
const char* ssid = "wifissid";
const char* password = "Wi-Fi密码";

// Mastodon配置
const char* mastodonHost = "https://example.com/api/v1/statuses";
const char* accessToken = "mastodon token";

#define LED_PIN 2

Wiimote wiimote;
float tr, br, tl, bl, total;
char w_kg[10];
int button_A = 0;
float w_off = 0.0, wt = 0.0;
int cal = 0;
float lastWt = 0.0;

void connectToWiFi() {
    M5.Lcd.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        M5.Lcd.print(".");
    }
    M5.Lcd.println("\nConnected to WiFi");
}

void postToMastodon(String message) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(mastodonHost);
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        http.addHeader("Authorization", "Bearer " + String(accessToken));

        String httpRequestData = "status=" + message;
        int httpResponseCode = http.POST(httpRequestData);

        http.end();
    }
}

void wiimote_callback(wiimote_event_type_t event_type, uint16_t handle, uint8_t *data, size_t len) {
    static int connection_count = 0;
    Serial.printf("wiimote handle=%04X len=%d\n", handle, len);
    if (event_type == WIIMOTE_EVENT_DATA) {
        if (data[1] == 0x34) {
            float weight[4];
            wiimote.get_balance_weight(data, weight);
            tr = weight[BALANCE_POSITION_TOP_RIGHT];
            br = weight[BALANCE_POSITION_BOTTOM_RIGHT];
            tl = weight[BALANCE_POSITION_TOP_LEFT];
            bl = weight[BALANCE_POSITION_BOTTOM_LEFT];
            total = tr + br + tl + bl;
        }
    } else if (event_type == WIIMOTE_EVENT_INITIALIZE) {
        Serial.println("Wiimote initializing...");
        wiimote.scan(true);
    } else if (event_type == WIIMOTE_EVENT_SCAN_START) {
        Serial.println("Wiimote scan started");
        M5.Lcd.println("Scanning...");
    } else if (event_type == WIIMOTE_EVENT_SCAN_STOP) {
        Serial.println("Wiimote scan stopped");
        if (connection_count == 0) {
            wiimote.scan(true);
        }
    } else if (event_type == WIIMOTE_EVENT_CONNECT) {
        Serial.println("Wiimote connected");
        M5.Lcd.println("Connected!");
        digitalWrite(LED_PIN, HIGH);
        wiimote.set_led(handle, 1 << connection_count);
        connection_count++;
    } else if (event_type == WIIMOTE_EVENT_DISCONNECT) {
        Serial.println("Wiimote disconnected");
        M5.Lcd.println("Disconnected");
        digitalWrite(LED_PIN, LOW);
        connection_count--;
        wiimote.scan(true);
    } else {
        Serial.printf("Unknown event: %d\n", event_type);
    }
}

void drawWeightPoint(int x, int y, float weight, float maxWeight, uint16_t color, const char* label) {
    int maxRadius = 30;
    int minRadius = 5;
    int radius = map(weight, 0, maxWeight, minRadius, maxRadius);
    M5.Lcd.fillCircle(x, y, radius, color);
    
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(x - 40, y + 35);
    M5.Lcd.printf("%s:%.1f", label, weight);
}

void drawWeightDistribution() {
    M5.Lcd.fillScreen(BLACK);
    
    int centerX = M5.Lcd.width() / 2;
    int centerY = M5.Lcd.height() / 2;
    
    // 绘制十字线
    M5.Lcd.drawLine(centerX, 0, centerX, M5.Lcd.height(), WHITE);
    M5.Lcd.drawLine(0, centerY, M5.Lcd.width(), centerY, WHITE);
    
    float maxWeight = max(max(tr, br), max(tl, bl));
    
    // 绘制四个点的重量
    drawWeightPoint(centerX/2, centerY/2, tl, maxWeight, TFT_GREEN, "TL");
    drawWeightPoint(centerX+centerX/2, centerY/2, tr, maxWeight, TFT_RED, "TR");
    drawWeightPoint(centerX/2, centerY+centerY/2, bl, maxWeight, TFT_BLUE, "BL");
    drawWeightPoint(centerX+centerX/2, centerY+centerY/2, br, maxWeight, TFT_YELLOW, "BR");
    
    // 显示总重量
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(centerX - 50, centerY - 10);
    M5.Lcd.printf("Total: %.1f", total);
}

void setup() {
    M5.begin();
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.setTextSize(2);

    pinMode(LED_PIN, OUTPUT);
    Serial.begin(115200);
    while (!Serial) delay(100);

    M5.Lcd.println("Initializing Wiimote...");
    wiimote.init(wiimote_callback);

    connectToWiFi();
}

void loop() {
    M5.update();
    wiimote.handle();

    if (M5.BtnA.wasPressed()) {
        w_off = total;
        cal = 1;
    }

    wt = total - w_off;
    if (wt < 0.5) wt = 0.0;
    sprintf(w_kg, "%2.1f", wt);

    if (!cal) {
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setTextSize(2);
        M5.Lcd.setTextColor(WHITE);
        M5.Lcd.setCursor(0, 120);
        M5.Lcd.print("Calibrating...");
    } else {
        drawWeightDistribution();
    }

    if (wt != lastWt) {
        lastWt = wt;
        String message = "Weight: " + String(wt) + " kg";
        postToMastodon(message);
    }

    delay(100);
}
