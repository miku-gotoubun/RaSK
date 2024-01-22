#include <SoftwareSerial.h>

// rxPin = 2  txPin = 3
SoftwareSerial mySerial(16,17);

// NMEAの緯度経度を「度分秒」(DMS)の文字列に変換する
String NMEA2DMS(float val) {
  int d = val / 100;
  int m = ((val / 100.0) - d) * 100.0;
  float s = ((((val / 100.0) - d) * 100.0) - m) * 60;
  return String(d) + "度" + String(m) + "分" + String(s, 1) + "秒";
}

// (未使用)NMEAの緯度経度を「度分」(DM)の文字列に変換する
String NMEA2DM(float val) {
  int d = val / 100;
  float m = ((val / 100.0) - d) * 100.0;
  return String(d) + "度" + String(m, 4) + "分";
}

// NMEAの緯度経度を「度」(DD)の文字列に変換する
String NMEA2DD(float val) {
  int d = val / 100;
  int m = (((val / 100.0) - d) * 100.0) / 60;
  float s = (((((val / 100.0) - d) * 100.0) - m) * 60) / (60 * 60);
  return String(d + m + s, 6);
}

// UTC時刻から日本の標準時刻に変換する(GMT+9:00)
String UTC2GMT900(String str) {
  int hh = (str.substring(0, 2).toInt()) + 9;
  if (hh > 24) hh = hh - 24;

  return String(hh, DEC) + ":" + str.substring(2, 4) + ":" + str.substring(4, 6);
}

void setup() {
  mySerial.begin(9600);
  Serial.begin(9600);
}
void loop() {
  // 1つのセンテンスを読み込む
  String line = mySerial.readStringUntil('\n');

  if (line != "") {
    int i, index = 0, len = line.length();
    String str = "";

    // StringListの生成(簡易)
    String list[30];
    for (i = 0; i < 30; i++) {
      list[i] = "";
    }

    // 「,」を区切り文字として文字列を配列にする
    for (i = 0; i < len; i++) {
      if (line[i] == ',') {
        list[index++] = str;
        str = "";
        continue;
      }
      str += line[i];
    }

    // $GPGGAセンテンスのみ読み込む
    if (list[0] == "$GPGGA") {

      // ステータス
      if (list[6] != "0") {
        // 現在時刻
        Serial.print(UTC2GMT900(list[1]));

        // 緯度
        Serial.print(" 緯度:");
        Serial.print(NMEA2DMS(list[2].toFloat()));
        Serial.print("(");
        Serial.print(NMEA2DD(list[2].toFloat()));
        Serial.print(")");

        // 経度
        Serial.print(" 経度:");
        Serial.print(NMEA2DMS(list[4].toFloat()));
        Serial.print("(");
        Serial.print(NMEA2DD(list[4].toFloat()));
        Serial.print(")");

        // 海抜
        Serial.print(" 海抜:");
        Serial.print(list[9]);
        list[10].toLowerCase();
        Serial.print(list[10]);
      } else {
        Serial.print("測位できませんでした。");
      }

      Serial.println("");
    }
  }
}
