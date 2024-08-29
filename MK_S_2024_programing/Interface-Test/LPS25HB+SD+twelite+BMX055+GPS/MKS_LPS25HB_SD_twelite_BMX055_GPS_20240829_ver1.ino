#include <SoftwareSerial.h>
#include <Wire.h>

//SDの設定
#include <SD.h>
#include <SPI.h>
File f;
void SDopen() {
  f = SD.open("/MKS_log05_test_20240829.csv", FILE_APPEND);
}

//LPS25HBの変数設定
uint8_t Core1a;
uint8_t Core1b;
uint8_t Core1c;
int32_t Core1d;
double Core1e;

// BMX055 加速度センサのI2Cアドレス
#define Addr_Accl 0x19  // (JP1,JP2,JP3 = Openの時)
// BMX055 ジャイロセンサのI2Cアドレス
#define Addr_Gyro 0x69  // (JP1,JP2,JP3 = Openの時)
// BMX055 磁気センサのI2Cアドレス
#define Addr_Mag 0x13  // (JP1,JP2,JP3 = Openの時)

// センサーの値を保存するグローバル変数
float xAccl = 0.00;
float yAccl = 0.00;
float zAccl = 0.00;
float xGyro = 0.00;
float yGyro = 0.00;
float zGyro = 0.00;
int16_t xMag = 0;
int16_t yMag = 0;
int16_t zMag = 0;

// rxPin = 2  txPin = 3
SoftwareSerial gpsSerial(26, 27);  //GNSSのシリアル名
SoftwareSerial tweSerial(16, 17);  //tweliteのUARTピン番号RXTX

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
  Wire.begin();
  tweSerial.begin(115200);  //TWELITEのセットアップ
  gpsSerial.begin(9600);    //GPSの出力更新レート。115200bpsで毎秒10回取得になるが動かなくなる
  Serial.begin(115200);
  BMX055_Init();
  delay(300);
}
void BMX055_Init() {
  //------------------------------------------------------------//
  Wire.beginTransmission(Addr_Accl);
  Wire.write(0x0F);  // Select PMU_Range register
  Wire.write(0x03);  // Range = +/- 2g
  Wire.endTransmission();
  delay(100);
  //------------------------------------------------------------//
  Wire.beginTransmission(Addr_Accl);
  Wire.write(0x10);  // Select PMU_BW register
  Wire.write(0x08);  // Bandwidth = 7.81 Hz
  Wire.endTransmission();
  delay(100);
  //------------------------------------------------------------//
  Wire.beginTransmission(Addr_Accl);
  Wire.write(0x11);  // Select PMU_LPW register
  Wire.write(0x00);  // Normal mode, Sleep duration = 0.5ms
  Wire.endTransmission();
  delay(100);
  //------------------------------------------------------------//
  Wire.beginTransmission(Addr_Gyro);
  Wire.write(0x0F);  // Select Range register
  Wire.write(0x04);  // Full scale = +/- 125 degree/s
  Wire.endTransmission();
  delay(100);
  //------------------------------------------------------------//
  Wire.beginTransmission(Addr_Gyro);
  Wire.write(0x10);  // Select Bandwidth register
  Wire.write(0x07);  // ODR = 100 Hz
  Wire.endTransmission();
  delay(100);
  //------------------------------------------------------------//
  Wire.beginTransmission(Addr_Gyro);
  Wire.write(0x11);  // Select LPM1 register
  Wire.write(0x00);  // Normal mode, Sleep duration = 2ms
  Wire.endTransmission();
  delay(100);
  //------------------------------------------------------------//
  Wire.beginTransmission(Addr_Mag);
  Wire.write(0x4B);  // Select Mag register
  Wire.write(0x83);  // Soft reset
  Wire.endTransmission();
  delay(100);
  //------------------------------------------------------------//
  Wire.beginTransmission(Addr_Mag);
  Wire.write(0x4B);  // Select Mag register
  Wire.write(0x01);  // Soft reset
  Wire.endTransmission();
  delay(100);
  //------------------------------------------------------------//
  Wire.beginTransmission(Addr_Mag);
  Wire.write(0x4C);  // Select Mag register
  Wire.write(0x00);  // Normal Mode, ODR = 10 Hz
  Wire.endTransmission();
  //------------------------------------------------------------//
  Wire.beginTransmission(Addr_Mag);
  Wire.write(0x4E);  // Select Mag register
  Wire.write(0x84);  // X, Y, Z-Axis enabled
  Wire.endTransmission();
  //------------------------------------------------------------//
  Wire.beginTransmission(Addr_Mag);
  Wire.write(0x51);  // Select Mag register
  Wire.write(0x04);  // No. of Repetitions for X-Y Axis = 9
  Wire.endTransmission();
  //------------------------------------------------------------//
  Wire.beginTransmission(Addr_Mag);
  Wire.write(0x52);  // Select Mag register
  Wire.write(0x16);  // No. of Repetitions for Z-Axis = 15
  Wire.endTransmission();
}
void BMX055_Accl() {
  unsigned int data[6];
  for (int i = 0; i < 6; i++) {
    Wire.beginTransmission(Addr_Accl);
    Wire.write((2 + i));  // Select data register
    Wire.endTransmission();
    Wire.requestFrom(Addr_Accl, 1);  // Request 1 byte of data
    // Read 6 bytes of data
    // xAccl lsb, xAccl msb, yAccl lsb, yAccl msb, zAccl lsb, zAccl msb
    if (Wire.available() == 1)
      data[i] = Wire.read();
  }
  // Convert the data to 12-bits
  xAccl = ((data[1] * 256) + (data[0] & 0xF0)) / 16;
  if (xAccl > 2047) xAccl -= 4096;
  yAccl = ((data[3] * 256) + (data[2] & 0xF0)) / 16;
  if (yAccl > 2047) yAccl -= 4096;
  zAccl = ((data[5] * 256) + (data[4] & 0xF0)) / 16;
  if (zAccl > 2047) zAccl -= 4096;
  xAccl = xAccl * 0.0098;  // range = +/-2g
  yAccl = yAccl * 0.0098;  // range = +/-2g
  zAccl = zAccl * 0.0098;  // range = +/-2g
}
//=====================================================================================//
void BMX055_Gyro() {
  unsigned int data[6];
  for (int i = 0; i < 6; i++) {
    Wire.beginTransmission(Addr_Gyro);
    Wire.write((2 + i));  // Select data register
    Wire.endTransmission();
    Wire.requestFrom(Addr_Gyro, 1);  // Request 1 byte of data
    // Read 6 bytes of data
    // xGyro lsb, xGyro msb, yGyro lsb, yGyro msb, zGyro lsb, zGyro msb
    if (Wire.available() == 1)
      data[i] = Wire.read();
  }
  // Convert the data
  xGyro = (data[1] * 256) + data[0];
  if (xGyro > 32767) xGyro -= 65536;
  yGyro = (data[3] * 256) + data[2];
  if (yGyro > 32767) yGyro -= 65536;
  zGyro = (data[5] * 256) + data[4];
  if (zGyro > 32767) zGyro -= 65536;

  xGyro = xGyro * 0.0038;  //  Full scale = +/- 125 degree/s
  yGyro = yGyro * 0.0038;  //  Full scale = +/- 125 degree/s
  zGyro = zGyro * 0.0038;  //  Full scale = +/- 125 degree/s
}
//=====================================================================================//
void BMX055_Mag() {
  unsigned int data[8];
  for (int i = 0; i < 8; i++) {
    Wire.beginTransmission(Addr_Mag);
    Wire.write((0x42 + i));  // Select data register
    Wire.endTransmission();
    Wire.requestFrom(Addr_Mag, 1);  // Request 1 byte of data
    // Read 6 bytes of data
    // xMag lsb, xMag msb, yMag lsb, yMag msb, zMag lsb, zMag msb
    if (Wire.available() == 1)
      data[i] = Wire.read();
  }
  // Convert the data
  xMag = ((data[1] << 5) | (data[0] >> 3));
  if (xMag > 4095) xMag -= 8192;
  yMag = ((data[3] << 5) | (data[2] >> 3));
  if (yMag > 4095) yMag -= 8192;
  zMag = ((data[5] << 7) | (data[4] >> 1));
  if (zMag > 16383) zMag -= 32768;
}



void loop() {
  // 1つのセンテンスを読み込む
  String line = gpsSerial.readStringUntil('\n');

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
        Serial.println(list[10]);

        //BMX055のデータ
        //BMX055 加速度の読み取り
        BMX055_Accl();
        Serial.print("Accl= ");
        Serial.print(xAccl);
        Serial.print(",");
        Serial.print(yAccl);
        Serial.print(",");
        Serial.print(zAccl);
        Serial.println("");

        //BMX055 ジャイロの読み取り
        BMX055_Gyro();
        Serial.print("Gyro= ");
        Serial.print(xGyro);
        Serial.print(",");
        Serial.print(yGyro);
        Serial.print(",");
        Serial.print(zGyro);
        Serial.println("");

        //BMX055 磁気の読み取り
        BMX055_Mag();
        Serial.print("Mag= ");
        Serial.print(xMag);
        Serial.print(",");
        Serial.print(yMag);
        Serial.print(",");
        Serial.print(zMag);
        Serial.println("");

        //↓　気圧の値
        Wire.beginTransmission(0x5c);
        Wire.write(0x20);
        Wire.write(0xc0);
        Wire.endTransmission();

        Wire.beginTransmission(0x5c);
        Wire.write(0x28);
        Wire.endTransmission();  //必要
        Wire.requestFrom(0x5c, 1);
        Core1a = Wire.read();

        Wire.beginTransmission(0x5c);
        Wire.write(0x29);
        Wire.endTransmission();
        Wire.requestFrom(0x5c, 1);
        Core1b = Wire.read();


        Wire.beginTransmission(0x5c);
        Wire.write(0x2A);
        Wire.endTransmission();
        Wire.requestFrom(0x5c, 1);
        Core1c = Wire.read();

        Core1d = Core1c << 16 | Core1b << 8 | Core1a;

        Core1e = Core1d / 4096.00;  //.00つける　センサーの値
        Serial.print("Press=");
        Serial.println(Core1e);

        //SD保存
        //f = SD.open("/MKS_log01_20240830.csv", FILE_APPEND);
        SDopen();
        if (SD.begin()) {
          if (f) {
            //time
            f.print(UTC2GMT900(list[1]));
            f.print(",");
            //気圧
            f.print("Press=,");
            f.print(Core1e);
            //f.print(",");
            f.print(",LAT,");
            //f.print(NMEA2DMS(list[2].toFloat())); //緯度(度分秒)
            //f.print(",");
            f.print(NMEA2DD(list[2].toFloat()));  //緯度(度)
            f.print(",");
            f.print("LOT");
            f.print(",");
            //f.print(NMEA2DMS(list[4].toFloat()));//経度(度分秒)
            //f.print(",");
            f.print(NMEA2DD(list[4].toFloat()));  //経度(度)
            f.print(",");
            f.print("Altitude above sea level");
            f.print(",");
            f.print(list[9]);
            //f.print(",");
            f.print(list[10]);
            f.print(",");

            //BMX055
            f.print("Accl, ");
            f.print(xAccl);
            f.print(",");
            f.print(yAccl);
            f.print(",");
            f.print(zAccl);
            f.print(",");
            f.print("Gyro, ");
            f.print(xGyro);
            f.print(",");
            f.print(yGyro);
            f.print(",");
            f.print(zGyro);
            f.print(",");
            f.print("Mag, ");
            f.print(xMag);
            f.print(",");
            f.print(yMag);
            f.print(",");
            f.print(zMag);
            f.println(",");


            f.close();
            Serial.println("write OK");
            tweSerial.println(" write OK");
          }
        } else {
          Serial.println("write failed");
          tweSerial.println(" write failed");
        }

        //TWELITE送信
        // 現在時刻
        tweSerial.print(UTC2GMT900(list[1]));

        //LPS25HB
        tweSerial.print("  Press=");
        tweSerial.print(Core1e);

        // 緯度
        tweSerial.print(" LAT:");
        //tweSerial.print( NMEA2DMS(list[2].toFloat()));
        //tweSerial.print(" (");
        tweSerial.print( NMEA2DD(list[2].toFloat()));
        //tweSerial.print(" )");
        tweSerial.print(" ,");

        // 経度
        tweSerial.print(" LOT:");
        //tweSerial.print( NMEA2DMS(list[4].toFloat()));
        //tweSerial.print(" (");
        tweSerial.print(NMEA2DD(list[4].toFloat()));
        //tweSerial.print(" )");
        tweSerial.print(" , ");

        // 海抜
        tweSerial.print(" Sea Level:");
        tweSerial.print(list[9]);
        list[10].toLowerCase();
        tweSerial.println(list[10]);

        //BMX055のデータ送信
        //BMX055 加速度
        tweSerial.print(" Accl=");
        tweSerial.print(xAccl);
        tweSerial.print(" ,");
        tweSerial.print(yAccl);
        tweSerial.print(" ,");
        tweSerial.print(zAccl);
        tweSerial.println(" ");

        //BMX055 ジャイロ
        tweSerial.print(" Gyro= ");
        tweSerial.print(xGyro);
        tweSerial.print(" ,");
        tweSerial.print(yGyro);
        tweSerial.print(" ,");
        tweSerial.print(zGyro);
        tweSerial.println(" ");

        //BMX055 磁気
        tweSerial.print(" Mag= ");
        tweSerial.print(xMag);
        tweSerial.print(" ,");
        tweSerial.print(yMag);
        tweSerial.print(" ,");
        tweSerial.print(zMag);
        tweSerial.println(" ");

      } else {
        Serial.println("測位できませんでした。");

        //BMX055 加速度の読み取り
        BMX055_Accl();
        Serial.print("Accl= ");
        Serial.print(xAccl);
        Serial.print(",");
        Serial.print(yAccl);
        Serial.print(",");
        Serial.print(zAccl);
        Serial.println("");

        //BMX055 ジャイロの読み取り
        BMX055_Gyro();
        Serial.print("Gyro= ");
        Serial.print(xGyro);
        Serial.print(",");
        Serial.print(yGyro);
        Serial.print(",");
        Serial.print(zGyro);
        Serial.println("");

        //BMX055 磁気の読み取り
        BMX055_Mag();
        Serial.print("Mag= ");
        Serial.print(xMag);
        Serial.print(",");
        Serial.print(yMag);
        Serial.print(",");
        Serial.print(zMag);
        Serial.println("");
        //LPS25HB
        //↓　気圧の値
        Wire.beginTransmission(0x5c);
        Wire.write(0x20);
        Wire.write(0xc0);
        Wire.endTransmission();

        Wire.beginTransmission(0x5c);
        Wire.write(0x28);
        Wire.endTransmission();  //必要
        Wire.requestFrom(0x5c, 1);
        Core1a = Wire.read();

        Wire.beginTransmission(0x5c);
        Wire.write(0x29);
        Wire.endTransmission();
        Wire.requestFrom(0x5c, 1);
        Core1b = Wire.read();


        Wire.beginTransmission(0x5c);
        Wire.write(0x2A);
        Wire.endTransmission();
        Wire.requestFrom(0x5c, 1);
        Core1c = Wire.read();

        Core1d = Core1c << 16 | Core1b << 8 | Core1a;

        Core1e = Core1d / 4096.00;  //.00つける　センサーの値
        Serial.print("Press=");
        Serial.println(Core1e);

        //SD保存
        //f = SD.open("/MKS_log01_20240830.csv", FILE_APPEND);
        SDopen();
        if (SD.begin()) {
          if (f) {
            f.print("No TIME");
            //気圧
            f.print(",Press=,");
            f.print(Core1e);
            f.print(",LAT,");
            //f.print(NMEA2DMS(list[2].toFloat())); //緯度(度分秒)
            //f.print(",");
            f.print("No LAT");  //緯度(度)
            f.print(",");
            f.print("LOT");
            f.print(",");
            //f.print(NMEA2DMS(list[4].toFloat()));//経度(度分秒)
            //f.print(",");
            f.print("No Lot");  //経度(度)
            f.print(",");
            f.print("Altitude above sea level");
            f.print(",");
            f.print("No sea level");
            f.print(",");
            //f.print(list[10]);

            //BMX055
            f.print("Accl, ");
            f.print(xAccl);
            f.print(",");
            f.print(yAccl);
            f.print(",");
            f.print(zAccl);
            f.print(",");
            f.print("Gyro, ");
            f.print(xGyro);
            f.print(",");
            f.print(yGyro);
            f.print(",");
            f.print(zGyro);
            f.print(",");
            f.print("Mag, ");
            f.print(xMag);
            f.print(",");
            f.print(yMag);
            f.print(",");
            f.print(zMag);
            f.println(",");




            f.close();
            Serial.println("write OK");
            tweSerial.println(" write OK");
          }
        } else {
          Serial.println("write failed");
          tweSerial.println(" write failed");
        }

        //twelite送信

        //GPS
        tweSerial.println("  GPS positioning is not possible.");  //測位できませんでした。

        //LPS25HB
        tweSerial.print("Press=");
        tweSerial.println(Core1e);

        //BMX055のデータ送信
        //BMX055 加速度
        tweSerial.print(" Accl=");
        tweSerial.print(xAccl);
        tweSerial.print(" ,");
        tweSerial.print(yAccl);
        tweSerial.print(" ,");
        tweSerial.print(zAccl);
        tweSerial.println(" ");

        //BMX055 ジャイロ
        tweSerial.print(" Gyro= ");
        tweSerial.print(xGyro);
        tweSerial.print(" ,");
        tweSerial.print(yGyro);
        tweSerial.print(" ,");
        tweSerial.print(zGyro);
        tweSerial.println(" ");

        //BMX055 磁気
        tweSerial.print(" Mag= ");
        tweSerial.print(xMag);
        tweSerial.print(" ,");
        tweSerial.print(yMag);
        tweSerial.print(" ,");
        tweSerial.print(zMag);
        tweSerial.println(" ");
      }

      Serial.println("");
    }
  }
}