// Thông tin Blynk cho ESP32
#define BLYNK_TEMPLATE_ID "TMPL6313yDT8u"  // ID mẫu Blynk
#define BLYNK_TEMPLATE_NAME "DHT11"        // Tên mẫu Blynk
#define BLYNK_AUTH_TOKEN "XxobOw9z_Wv18jHcSdT2GDUBovu0_E7E" // Token xác thực Blynk

#define BLYNK_PRINT Serial  // Hiển thị thông tin debug từ Blynk trên Serial Monitor

// Thư viện cần thiết
#include <WiFi.h>              // Thư viện Wi-Fi cho ESP32
#include <WiFiClient.h>        // Quản lý client Wi-Fi
#include <BlynkSimpleEsp32.h>  // Thư viện Blynk dành cho ESP32
#include <uRTCLib.h>           // Thư viện cho RTC DS3231
#include <Wire.h>              // Giao tiếp I2C
#include <Adafruit_GFX.h>      // Đồ họa cho OLED
#include <Adafruit_SSD1306.h>  // Thư viện cho màn hình OLED SSD1306
#include <RTClib.h>            // Quản lý thời gian thực
#include <DHT.h>               // Cảm biến nhiệt độ & độ ẩm DHT

// *** Kết nối Wi-Fi ***
char ssid[] = "haidang";       // Tên Wi-Fi
char pass[] = "123456781";     // Mật khẩu Wi-Fi

// *** Chân điều khiển thiết bị ***
const int fanPin = 26;         // Chân quạt chính
const int fanPin2 = 23;        // Chân quạt phụ
const int ledPin1 = 4;         // Chân LED 1
const int ledPin2 = 18;        // Chân LED 2
const int ledPin3 = 19;        // Chân LED 3
const int ledPin4 = 2;         // Chân LED 4
const int switchLedPin1 = 5;   // Công tắc LED 1
const int switchLedPin2 = 13;  // Công tắc LED 2
const int switchLedPin3 = 14;  // Công tắc LED 3
const int switchLedPin4 = 25;  // Công tắc LED 4
const int switchFanPin = 32;   // Công tắc quạt
const int gasSensorPin = 35;   // Cảm biến khí gas
const int buzzerPin = 17;      // Chuông báo động

// *** Biến trạng thái thiết bị ***
bool ledState1 = false;        // Trạng thái LED 1
bool ledState2 = false;        // Trạng thái LED 2
bool ledState3 = false;        // Trạng thái LED 3
bool ledState4 = false;        // Trạng thái LED 4
bool fanState = false;         // Trạng thái quạt chính
bool alarmActive = false;      // Trạng thái báo động

// Lưu trạng thái công tắc lần cuối (cho chống rung)
bool lastSwitchLedState1 = HIGH;
bool lastSwitchLedState2 = HIGH;
bool lastSwitchLedState3 = HIGH;
bool lastSwitchLedState4 = HIGH;
bool lastSwitchFanState = HIGH;

// *** Ngưỡng an toàn khí gas ***
const float dangerThreshold = 20.0;

// *** Cài đặt màn hình OLED ***
#define SCREEN_WIDTH 128       // Chiều rộng màn hình OLED
#define SCREEN_HEIGHT 64       // Chiều cao màn hình OLED
#define OLED_RESET -1          // Chân reset màn hình OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// *** Cài đặt RTC và cảm biến DHT ***
RTC_DS3231 rtc;                // Đối tượng RTC DS3231
#define DHTPIN 27              // Chân kết nối DHT11
#define DHTTYPE DHT11          // Loại cảm biến
DHT dht(DHTPIN, DHTTYPE);      // Khởi tạo cảm biến DHT

// *** Biểu tượng trên OLED ***
const unsigned char temp_icon[] PROGMEM = {0x18, 0x3C, 0x24, 0x24, 0x7E, 0x7E, 0x24, 0x24}; // Nhiệt độ
const unsigned char humidity_icon[] PROGMEM = {0x18, 0x18, 0x3C, 0x3C, 0x5A, 0x5A, 0xA5, 0xA5}; // Độ ẩm
const unsigned char clock_icon[] PROGMEM = {0x3C, 0x42, 0xA9, 0x85, 0x85, 0xA9, 0x42, 0x3C}; // Đồng hồ
const unsigned char calendar_icon[] PROGMEM = {0x7E, 0x5A, 0x5A, 0x7E, 0x42, 0x42, 0x42, 0x7E}; // Lịch

// *** Biến thời gian ***
unsigned long previousDisplayUpdateTime = 0;  // Lần cập nhật OLED cuối
const long displayUpdateInterval = 1000;      // Tần suất cập nhật OLED (ms)
unsigned long previousGasCheckTime = 0;       // Lần kiểm tra khí cuối
const long gasCheckInterval = 500;            // Tần suất kiểm tra khí (ms)

void setup() {
  // *** Khởi động Serial ***
  Serial.begin(115200);

  // *** Cài đặt chân điều khiển ***
  pinMode(fanPin, OUTPUT);
  pinMode(fanPin2, OUTPUT);
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(ledPin3, OUTPUT);
  pinMode(ledPin4, OUTPUT);
  pinMode(switchLedPin1, INPUT_PULLUP);
  pinMode(switchLedPin2, INPUT_PULLUP);
  pinMode(switchLedPin3, INPUT_PULLUP);
  pinMode(switchLedPin4, INPUT_PULLUP);
  pinMode(switchFanPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);

  // *** Khởi động màn hình OLED ***
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED not found"));
    while (1);
  }
  display.clearDisplay();

  // *** Khởi động RTC ***
  if (!rtc.begin()) {
    Serial.println("RTC DS3231 not found, check connections!");
    while (1);
  }

  // *** Khởi động DHT ***
  dht.begin();

  // *** Kết nối Wi-Fi ***
  WiFi.begin(ssid, pass);
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");

  // *** Kết nối Blynk ***
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}

// *** Xử lý dữ liệu từ Blynk ***
BLYNK_WRITE(V0) {
  ledState1 = param.asInt();
  digitalWrite(ledPin1, ledState1);
}
BLYNK_WRITE(V1) {
  ledState2 = param.asInt();
  digitalWrite(ledPin2, ledState2);
}
BLYNK_WRITE(V2) {
  ledState3 = param.asInt();
  digitalWrite(ledPin3, ledState3);
}
BLYNK_WRITE(V3) {
  ledState4 = param.asInt();
  digitalWrite(ledPin4, ledState4);
}
BLYNK_WRITE(V4) {
  fanState = param.asInt();
  digitalWrite(fanPin, fanState);
}

void loop() {
  // *** Chạy Blynk ***
  Blynk.run();

  // *** Cập nhật hiển thị OLED ***
  unsigned long currentMillis = millis();
  if (currentMillis - previousDisplayUpdateTime >= displayUpdateInterval) {
    previousDisplayUpdateTime = currentMillis;
    updateDisplay();
  }

  // *** Kiểm tra cảm biến khí gas ***
  if (currentMillis - previousGasCheckTime >= gasCheckInterval) {
    previousGasCheckTime = currentMillis;
    checkGasSensor();
  }

  // *** Kiểm tra công tắc LED và quạt ***
  checkSwitches();
}

// *** Kiểm tra cảm biến khí gas ***
void checkGasSensor() {
  int gasValue = analogRead(gasSensorPin);
  float gasConcentration = (gasValue / 4095.0) * 100; // Tính nồng độ khí gas

  if (gasConcentration > dangerThreshold && !alarmActive) {
    digitalWrite(buzzerPin, HIGH);
    digitalWrite(fanPin2, HIGH);
    Blynk.logEvent("nguy_hiem", "Nguy_hiem!"); // Gửi cảnh báo Blynk
    alarmActive = true;
  } else if (gasConcentration <= dangerThreshold && alarmActive) {
    digitalWrite(buzzerPin,
