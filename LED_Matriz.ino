/*
 * ESP8266 WiFi Clock - Toggle Version (Final)
 * 專案名稱: 雙區顯示時鐘 (自動切換內容)
 * 硬體架構: ESP-12F + 10x MAX7219 (FC16)
 * 佈局邏輯: 
 * - 下排 (Zone 0): 0-4 -> 顯示 日期 (1s) / 時間 (1s) 循環
 * - 上排 (Zone 1): 5-9 -> 顯示 "Marlon" (1s) / "$$$$$" (1s) 循環
 * * 功能更新: 
 * - 增加狀態機邏輯處理每秒切換
 * - Font_Data.h 已增加 "$" (ASCII 36)
 */

#include <ESP8266WiFi.h>
#include <time.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

// 引入自定義檔案
// 請確保 Font_Data.h 已經包含 $, !, 你, 好 的定義
#include "Font_Data.h" 
#include "Quotes.h"    

// --- WiFi 設定 ---
const char* ssid     = "IT-AP13";
const char* password = "greentea80342958";

// --- 時間設定 ---
#define NTP_SERVER1 "pool.ntp.org"
#define NTP_SERVER2 "time.nist.gov"

// --- 硬體設定 ---
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES   10     // 總模組數
#define CS_PIN        15     // GPIO 15 (D8)

// 建立 Parola 物件
MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// --- Zone 定義 ---
#define ZONE_LOWER  0 // 下排 (時間/日期)
#define ZONE_UPPER  1 // 上排 (Marlon/$$$$$)

// --- 全域變數 ---
char timeBuffer[15];     // 存放 "HH:MM:SS"
char dateBuffer[12];     // 存放 "MM/DD"
unsigned long lastDisplayUpdate = 0; // 上次切換顯示內容的時間

// 顯示狀態: 0=狀態A, 1=狀態B
bool displayState = false; 

// 函式宣告
void connectWiFi();
void syncTime();
void updateDisplay(); // 整合的顯示更新函式

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n[System] Starting Toggle Version...");

  // 1. 初始化螢幕
  P.begin(2); 
  P.setIntensity(1); 

  // 設定分區範圍
  // 下排: 0(右下) -> 4(左下)
  P.setZone(ZONE_LOWER, 0, 4);
  // 上排: 5(右上) -> 9(左上)
  P.setZone(ZONE_UPPER, 5, 9);

  // *** 關鍵修正：解決文字顛倒問題 ***
  // 必須分開呼叫 setZoneEffect，不可使用 | 運算符
  
  // 針對下排 (Zone 0) 進行 180 度翻轉
  P.setZoneEffect(ZONE_LOWER, true, PA_FLIP_UD);
  P.setZoneEffect(ZONE_LOWER, true, PA_FLIP_LR);
  
  // 針對上排 (Zone 1) 進行 180 度翻轉
  P.setZoneEffect(ZONE_UPPER, true, PA_FLIP_UD);
  P.setZoneEffect(ZONE_UPPER, true, PA_FLIP_LR);

  // 2. 連線階段 (先用預設字型顯示英文)
  P.setFont(ZONE_LOWER, nullptr); 
  P.setFont(ZONE_UPPER, nullptr);

  P.displayZoneText(ZONE_UPPER, "WiFi", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
  P.displayAnimate();
  connectWiFi();

  // 3. NTP 時間同步
  P.displayZoneText(ZONE_UPPER, "Sync", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
  P.displayAnimate();
  syncTime();

  // 4. 初始化顯示
  P.displayClear();
  updateDisplay(); // 立即顯示第一次內容
}

void loop() {
  // 1. 動畫核心
  P.displayAnimate();

  // 2. 每 1000ms (1秒) 切換一次顯示內容
  if (millis() - lastDisplayUpdate > 1000) {
    lastDisplayUpdate = millis();
    displayState = !displayState; // 切換狀態 (0 -> 1 -> 0)
    updateDisplay();
  }
}

// --- 顯示核心邏輯 ---
void updateDisplay() {
  time_t now = time(nullptr);
  struct tm* p_tm = localtime(&now);

  // -------------------------
  // 下排邏輯 (Zone 0): 日期 <-> 時間
  // -------------------------
  // 時間日期都用預設數字字型 (nullptr)
  P.setFont(ZONE_LOWER, nullptr); 

  if (displayState == 0) {
    // 狀態 0: 顯示 日期 (MM/DD)
    sprintf(dateBuffer, "%02d/%02d", p_tm->tm_mon + 1, p_tm->tm_mday);
    P.displayZoneText(ZONE_LOWER, dateBuffer, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
  } else {
    // 狀態 1: 顯示 時間 (HH:MM:SS)
    sprintf(timeBuffer, "%02d:%02d:%02d", p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec);
    P.displayZoneText(ZONE_LOWER, timeBuffer, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
  }

  // -------------------------
  // 上排邏輯 (Zone 1): Marlon <-> $$$$$
  // -------------------------
  
  if (displayState == 0) {
    // 狀態 0: 顯示 "Marlon"
    // 切換回預設字型以顯示英文字母
    P.setFont(ZONE_UPPER, nullptr); 
    P.displayZoneText(ZONE_UPPER, "Marlon", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
  } else {
    // 狀態 1: 顯示 "$$$$$"
    // 切換到自定義字型 (ChineseFont) 因為我們把 $ (ASCII 36) 定義在那裡
    P.setFont(ZONE_UPPER, ChineseFont); 
    P.displayZoneText(ZONE_UPPER, "$$$$$", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
  }

  // 強制刷新畫面 (針對 PA_PRINT 靜態顯示模式)
  P.displayReset(ZONE_LOWER);
  P.displayReset(ZONE_UPPER);
}

void connectWiFi() {
  Serial.print("[WiFi] Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 15000) {
    delay(500);
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n[WiFi] Failed");
  }
}

void syncTime() {
  // ESP8266 Core 2.3.0 相容寫法: UTC+8 = 28800秒
  configTime(28800, 0, NTP_SERVER1, NTP_SERVER2);

  Serial.print("[NTP] Syncing");
  time_t now = time(nullptr);
  unsigned long startAttempt = millis();
  while (now < 100000 && millis() - startAttempt < 20000) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  
  if (now > 100000) {
    Serial.println("\n[NTP] Synced");
    Serial.print("Current Time: ");
    Serial.println(ctime(&now));
  } else {
    Serial.println("\n[NTP] Failed");
  }
}