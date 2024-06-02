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
float lastWt = 0.0;  // 用于存储上一次的体重值

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

        if (httpResponseCode > 0) {
            M5.Lcd.println("Data posted to Mastodon");
        } else {
            M5.Lcd.printf("Error on sending POST: %d\n", httpResponseCode);
        }

        http.end();
    } else {
        M5.Lcd.println("WiFi not connected");
    }
}

void disp(int16_t x, int16_t y, int16_t textsize, uint16_t color, String msg) {
    M5.Lcd.setCursor(x, y);
    M5.Lcd.setTextSize(textsize);
    M5.Lcd.setTextColor(color);
    M5.Lcd.print(msg);
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

            M5.Lcd.fillScreen(BLACK);
            disp(0, 20, 2, WHITE, "Weights:");
            disp(0, 40, 2, WHITE, "TR: " + String(tr, 1) + " kg");
            disp(0, 60, 2, WHITE, "BR: " + String(br, 1) + " kg");
            disp(0, 80, 2, WHITE, "TL: " + String(tl, 1) + " kg");
            disp(0, 100, 2, WHITE, "BL: " + String(bl, 1) + " kg");
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
    delay(100);
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

    if (!cal) disp(0, 120, 2, WHITE, "Calibrating...");

    if (wt != lastWt) {
        lastWt = wt;
        String message = "Weight: " + String(wt) + " kg";
        postToMastodon(message);
    }

    delay(100);
}
