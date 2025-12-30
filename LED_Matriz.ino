/*
 * ESP8266 WiFi Clock - Scroll Version (Final)
 * 專案名稱: 雙區顯示時鐘 (滑動切換內容)
 * 硬體架構: ESP-12F + 10x MAX7219 (FC16)
 * 佈局邏輯: 
 * - 下排 (Zone 0): 0-4 -> 滑動顯示 日期 <-> 時間
 * - 上排 (Zone 1): 5-9 -> 滑動顯示 "Marlon" <-> "$$$$$"
 * * 功能更新: 
 * - 改用 PA_SCROLL_LEFT (向左滑動)
 * - 使用 getZoneStatus() 判斷動畫結束才切換內容
 */

#include <ESP8266WiFi.h>
#include <time.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

// 引入自定義檔案
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
#define MAX_DEVICES   10     
#define CS_PIN        15     

MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// --- Zone 定義 ---
#define ZONE_LOWER  0 // 下排
#define ZONE_UPPER  1 // 上排

// --- 全域變數 ---
char timeBuffer[15];     // 存放 "HH:MM:SS"
char dateBuffer[12];     // 存放 "MM/DD"

// 狀態變數: false = 顯示第一種內容, true = 顯示第二種內容
bool lowerState = false; // false=日期, true=時間
bool upperState = false; // false=Marlon, true=$$$$$

// --- 動畫設定 ---
#define SCROLL_SPEED  40   // 捲動速度 (越小越快)
#define SCROLL_PAUSE  1500 // 停留在中間的時間 (毫秒)，這裡設為 1.5秒

void connectWiFi();
void syncTime();
void handleLowerZone();
void handleUpperZone();

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n[System] Starting Scroll Version...");

  // 1. 初始化螢幕
  P.begin(2); 
  P.setIntensity(1); 

  P.setZone(ZONE_LOWER, 0, 4);
  P.setZone(ZONE_UPPER, 5, 9);

  // 翻轉修正
  P.setZoneEffect(ZONE_LOWER, true, PA_FLIP_UD);
  P.setZoneEffect(ZONE_LOWER, true, PA_FLIP_LR);
  
  P.setZoneEffect(ZONE_UPPER, true, PA_FLIP_UD);
  P.setZoneEffect(ZONE_UPPER, true, PA_FLIP_LR);

  // 2. 連線階段 (靜態顯示)
  P.setFont(ZONE_LOWER, nullptr); 
  P.setFont(ZONE_UPPER, nullptr);
  
  // 連線時使用 PRINT 模式，比較清楚
  P.displayZoneText(ZONE_UPPER, "WiFi", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
  P.displayAnimate();
  connectWiFi();

  P.displayZoneText(ZONE_UPPER, "Sync", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
  P.displayAnimate();
  syncTime();

  // 3. 初始化正式顯示 (設定為 SCROLL 模式)
  P.displayClear();

  // 設定初始文字與動畫模式
  // 參數: Text, Align, Speed, Pause, EffectIn, EffectOut
  
  // 下排初始: 日期
  P.displayZoneText(ZONE_LOWER, dateBuffer, PA_CENTER, SCROLL_SPEED, SCROLL_PAUSE, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  
  // 上排初始: Marlon
  P.displayZoneText(ZONE_UPPER, "Marlon", PA_CENTER, SCROLL_SPEED, SCROLL_PAUSE, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
}

void loop() {
  // 1. 動畫核心 (必須持續呼叫)
  // 當動畫完成時 (EffectOut 結束)，displayAnimate() 會返回 true
  P.displayAnimate();

  // 2. 檢查各 Zone 狀態，如果動畫結束，就切換內容
  if (P.getZoneStatus(ZONE_LOWER)) {
    handleLowerZone();
  }

  if (P.getZoneStatus(ZONE_UPPER)) {
    handleUpperZone();
  }
}

// --- 下排邏輯 (日期 <-> 時間) ---
void handleLowerZone() {
  time_t now = time(nullptr);
  struct tm* p_tm = localtime(&now);
  
  // 切換狀態
  lowerState = !lowerState;
  
  // 數字都使用預設字型
  P.setFont(ZONE_LOWER, nullptr);

  if (lowerState == false) {
    // 顯示 日期
    sprintf(dateBuffer, "%02d/%02d", p_tm->tm_mon + 1, p_tm->tm_mday);
    P.setTextBuffer(ZONE_LOWER, dateBuffer);
  } else {
    // 顯示 時間
    sprintf(timeBuffer, "%02d:%02d:%02d", p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec);
    P.setTextBuffer(ZONE_LOWER, timeBuffer);
  }

  // 重置動畫，讓新文字開始滑動
  P.displayReset(ZONE_LOWER);
}

// --- 上排邏輯 (Marlon <-> $$$$$) ---
void handleUpperZone() {
  // 切換狀態
  upperState = !upperState;

  if (upperState == false) {
    // 顯示 "Marlon" (英文使用預設字型)
    P.setFont(ZONE_UPPER, nullptr);
    P.setTextBuffer(ZONE_UPPER, "Marlon");
  } else {
    // 顯示 "$$$$$" (符號在自定義字型中)
    P.setFont(ZONE_UPPER, ChineseFont);
    P.setTextBuffer(ZONE_UPPER, "$$$$$");
  }

  // 重置動畫
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
  configTime(28800, 0, NTP_SERVER1, NTP_SERVER2);

  Serial.print("[NTP] Syncing");
  time_t now = time(nullptr);
  unsigned long startAttempt = millis();
  while (now < 100000 && millis() - startAttempt < 20000) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println(now > 100000 ? "\n[NTP] Synced" : "\n[NTP] Failed");
}
