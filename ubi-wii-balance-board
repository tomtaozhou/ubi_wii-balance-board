#include <WiFi.h>
#include <HTTPClient.h>
#include "Wiimote.h"
#include <Wire.h>
#include <M5StickC.h>

// Wi-Fi配置
const char* ssid = "wifissid";
const char* password = "Wi-Fi密码";

// Mastodon配置
const char* mastodonHost = "https://「正确的域名」/api/v1/statuses";
const char* accessToken = "mastodon tonken";

#define LED        2

// 其他全局变量
Wiimote wiimote;
char w_kg[10];
float tr, br, tl, bl, total;
int button_A = 0;
float w_off = 0.0;
float wt;
int cal = 0;
float lastWt = 0.0;  // 用于存储上一次的体重值

// 连接Wi-Fi的函数
void connectToWiFi() {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
}

// 将数据发送到Mastodon的函数
void postToMastodon(String message) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(mastodonHost);
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        http.addHeader("Authorization", "Bearer " + String(accessToken));

        String httpRequestData = "status=" + message;
        int httpResponseCode = http.POST(httpRequestData);

        if (httpResponseCode > 0) {
            Serial.println("Data posted to Mastodon");
        }
        else {
            Serial.print("Error on sending POST: ");
            Serial.println(httpResponseCode);
        }

        http.end();
    }
    else {
        Serial.println("WiFi not connected");
    }
}

// 显示信息的函数
void disp(int16_t x, int16_t y, int16_t textsize, uint16_t color, String msg) {
    M5.Lcd.setCursor(x, y);
    M5.Lcd.setTextSize(textsize);
    M5.Lcd.setTextColor(color);
    M5.Lcd.print(msg);
}

void wiimote_callback(wiimote_event_type_t event_type, uint16_t handle, uint8_t *data, size_t len) {
  static int connection_count = 0;
  printf("wiimote handle=%04X len=%d ", handle, len);
  if(event_type == WIIMOTE_EVENT_DATA){
    if(data[1]==0x32){
      for (int i = 0; i < 4; i++) {
        printf("%02X ", data[i]);
      }
      // http://wiibrew.org/wiki/Wiimote/Extension_Controllers/Nunchuck
      uint8_t* ext = data+4;
      printf(" ... Nunchuk: sx=%3d sy=%3d c=%d z=%d\n",
        ext[0],
        ext[1],
        0==(ext[5]&0x02),
        0==(ext[5]&0x01)
      );
    }else if(data[1]==0x34){
      for (int i = 0; i < 4; i++) {
        printf("%02X ", data[i]);
      }
      // https://wiibrew.org/wiki/Wii_Balance_Board#Data_Format
      uint8_t* ext = data+4;
      /*printf(" ... Wii Balance Board: TopRight=%d BottomRight=%d TopLeft=%d BottomLeft=%d Temperature=%d BatteryLevel=0x%02x\n",
        ext[0] * 256 + ext[1],
        ext[2] * 256 + ext[3],
        ext[4] * 256 + ext[5],
        ext[6] * 256 + ext[7],
        ext[8],
        ext[10]
      );*/
      
      float weight[4];
      wiimote.get_balance_weight(data, weight);
      tr=weight[BALANCE_POSITION_TOP_RIGHT];
      br=weight[BALANCE_POSITION_BOTTOM_RIGHT];
      tl=weight[BALANCE_POSITION_TOP_LEFT];
      bl=weight[BALANCE_POSITION_BOTTOM_LEFT];
      total=tr+br+tl+bl;
      digitalWrite(LED, HIGH);
      printf(" ... Wii Balance Board: TopRight=%2.1f BottomRight=%2.1f TopLeft=%2.1f BottomLeft=%2.1f total=%3.2f\n",tr,br,tl,bl,total);
    }else{
      for (int i = 0; i < len; i++) {
        printf("%02X ", data[i]);
      }
      printf("\n");
    }

    bool wiimote_button_down  = (data[2] & 0x01) != 0;
    bool wiimote_button_up    = (data[2] & 0x02) != 0;
    bool wiimote_button_right = (data[2] & 0x04) != 0;
    bool wiimote_button_left  = (data[2] & 0x08) != 0;
    bool wiimote_button_plus  = (data[2] & 0x10) != 0;
    bool wiimote_button_2     = (data[3] & 0x01) != 0;
    bool wiimote_button_1     = (data[3] & 0x02) != 0;
    bool wiimote_button_B     = (data[3] & 0x04) != 0;
    bool wiimote_button_A     = (data[3] & 0x08) != 0;
    bool wiimote_button_minus = (data[3] & 0x10) != 0;
    bool wiimote_button_home  = (data[3] & 0x80) != 0;
    if (wiimote_button_A) button_A=1; else button_A=0;
    static bool rumble = false;
    if(wiimote_button_plus && !rumble){
      wiimote.set_rumble(handle, true);
      rumble = true;
    }
    if(wiimote_button_minus && rumble){
      wiimote.set_rumble(handle, false);
      rumble = false;
    }
  }else if(event_type == WIIMOTE_EVENT_INITIALIZE){
    printf("  event_type=WIIMOTE_EVENT_INITIALIZE\n");
    wiimote.scan(true);
  }else if(event_type == WIIMOTE_EVENT_SCAN_START){
    printf("  event_type=WIIMOTE_EVENT_SCAN_START\n");
  }else if(event_type == WIIMOTE_EVENT_SCAN_STOP){
    printf("  event_type=WIIMOTE_EVENT_SCAN_STOP\n");
    if(connection_count==0){
      wiimote.scan(true);
    }
  }else if(event_type == WIIMOTE_EVENT_CONNECT){
    printf("  event_type=WIIMOTE_EVENT_CONNECT\n");
    wiimote.set_led(handle, 1<<connection_count);
    connection_count++;
  }else if(event_type == WIIMOTE_EVENT_DISCONNECT){
    printf("  event_type=WIIMOTE_EVENT_DISCONNECT\n");
    connection_count--;
    wiimote.scan(true);
  }else{
    printf("  event_type=%d\n", event_type);
  }
  delay(100);
}

void setup() {
    // M5StickC初始化
    M5.begin();
    M5.Lcd.setRotation(1);

    pinMode(LED, OUTPUT);
    Serial.begin(115200);
    while (!Serial) delay(100);
    for (int i = 0; i < 5; i++) {
        digitalWrite(LED, HIGH);
        delay(300);
        digitalWrite(LED, LOW);
        delay(300);
    }
    wiimote.init(wiimote_callback);

    // 连接到Wi-Fi
    connectToWiFi();
}

void loop() {
  M5.update();
  wiimote.handle();

  if (button_A) {
    w_off = total;
    cal = 1;
  }

  wt = total - w_off;
  if (wt < 0.5) wt = 0.0;
  sprintf(w_kg, "%2.1f", wt);

  M5.Lcd.fillScreen(BLACK);   // 清空屏幕
  disp(1, 2, 1, GREEN, (String)"M5StickC & WiiBB");
  disp(8, 15, 3, CYAN, (char *)w_kg);
  disp(110, 15, 1, CYAN, (char *)"kg");
  if (!cal) disp(80, 30, 2, CYAN, (char *)"CAL");

  // 发送数据到Mastodon的条件判断
  if (wt != lastWt) {  // 如果当前体重和上一次体重不同
    lastWt = wt;  // 更新上一次的体重值
    String message = "体重: " + String(wt) + " kg";
    postToMastodon(message);
  }

  delay(100);
}
