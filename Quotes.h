#ifndef QUOTES_H
#define QUOTES_H

/*
 * Quotes.h
 * 存放 LED 矩陣顯示的金句編碼資料
 * * 說明：
 * - 這些 hex code 對應 Font_Data.h 中的中文字型定義。
 * - 目前主程式(Final Version) 暫時只用 Font_Data.h 來顯示 "你好"，
 * 這個檔案是為了保留給之後如果要切換回「跑馬燈模式」使用。
 */

const char* quoteStrings[] = {
  "\x8B\x8A\x85\x93\x83\x8A\x8D\x8C", // 沒有奇蹟只有累積
  "\x95\x96\x92\x91\x92\x84\x89\x97\x92\x8F\x92\x86", // 金錢越賺越多時 間越花越少
  "\x90\x94\x98\x99", // 講重點！
  "\x80\x8E\x88\x81\x87\x82\x84\x86\x80", // 人脈是你幫助多少人
};

// 自動計算金句數量
const int QUOTE_COUNT = sizeof(quoteStrings) / sizeof(quoteStrings[0]);

#endif