#include <ESP32Servo.h>
#include <Wire.h>
#include <SoftwareSerial.h>

#define CORE_0 0
#define CORE_1 1

// =======================コード①(anndo_togo1)の変数=======================
const int VOL_PIN = 32;  //flytePin
const int Buzzer = 14;

Servo myServo;
Servo myServo1;
const int SERVO_PIN = 25;
const int SERVO_PIN1 = 13;

uint8_t a;
uint8_t b;
uint8_t c;
int32_t d;
double stock_data_e[5] = { 0 };    //データをストックする配列.{0} は、配列の全要素を 0 で初期化することを意味します。
double e;                          //気圧のデータ(最新)
double stock_data_ave[5] = { 0 };  //移動平均を格納する
double hikaku;

double ave1;  //連続した移動平均の差を格納する変数
double ave2;
double ave3;
double ave4;

int count = 0;    //移動平均検知回数.
int count10 = 0;  //気圧開傘されたか.
int timer10 = 0;  //タイマー開傘されたか.
int fly = 0;      //フライトピンが抜けてるか.
uint32_t isrCount = 0, isrTime = 0;
//isrCountは割り込みサービスルーチン（ISR：通常のプログラムの実行を一時停止し、特定の「割り込み」イベントが発生したときに実行される特別な関数です。割り込みは、外部デバイスや内部タイマーなどによって引き起こされるイベントです。）が呼び出された回数をカウントするための変数です。
//isrTime は、最後に割り込みが発生した時刻を記録するための変数です。
//timer
int value;
#define BTN_STOP_ALARM 0  //恐らくスピーカーのピン番号
hw_timer_t* timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
volatile uint32_t isrCounter = 0;
volatile uint32_t lastIsrAt = 0;


void IRAM_ATTR onTimer() {  //割り込み関数の設定
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  isrCounter++;
  lastIsrAt = millis();
  portEXIT_CRITICAL_ISR(&timerMux);
  // Give a semaphore that we can check in the loop
  xSemaphoreGiveFromISR(timerSemaphore, NULL);
  // It is safe to use digitalRead/Write here if you want to toggle an output
}
//timer
//const int VOL_PIN = 5;   5は適当

double moving_average2(double hikaku) {


  for (int ii = 0; ii < 4; ii += 1) {
    stock_data_ave[ii] = stock_data_ave[ii + 1];
  }

  stock_data_ave[4] = hikaku;  //平均の最新のデータ
  //Serial.print("new=");
  //Serial.println(hikaku);
  Serial.println("-------------");
  ave1 = stock_data_ave[1] - stock_data_ave[0];
  Serial.print("移動平均前後の差1=");
  Serial.println(ave1, 5);
  ave2 = stock_data_ave[2] - stock_data_ave[1];
  Serial.print("移動平均前後の差2=");
  Serial.println(ave2, 5);
  ave3 = stock_data_ave[3] - stock_data_ave[2];
  Serial.print("移動平均前後の差3=");
  Serial.println(ave3, 5);
  ave4 = stock_data_ave[4] - stock_data_ave[3];
  Serial.print("移動平均前後の差4=");
  Serial.println(ave4, 5);
  //↑　4つの気圧平均比較

  if (ave1 > 0 && ave2 > 0 && ave3 > 0 && ave4 > 0) {
    if (400 <= isrCounter) {
      if (fly = 1) {
        count = count + 1;
      }
    }
  }


  Serial.print("count=");
  Serial.println(count);




  if (count > 10) {
    if (timer10 != 1) {
      count10 = 1;
      myServo.write(0);
      delay(1000);
      myServo.write(160);
      delay(1000);
    }
  }
  return ave1;
}


double moving_average(double konndou) {
  //number ++;
  //if(number>5){
  //number=0;
  //}


  for (int i = 0; i < 4; i += 1) {
    stock_data_e[i] = stock_data_e[i + 1];
    //Serial.println(stock_data_e[i]);
    //Serial.print(stock_data_e[i],stock_data_e[i+1]);
  }

  stock_data_e[4] = konndou;
  //Serial.print("new=");
  //Serial.println(konndou);
  double result = 0;  //初期化
  for (int i = 0; i < 5; i += 1) {

    //result +=stock_data_e[i];//合計
    result += stock_data_e[i];  //合計
  }

  result = result / 5.00;  //平均算出

  return result;
}

//=======================コード②(TWELITE+GPS+BMX055)の変数============================================
//BMX055の変数
#define Addr_Accl 0x19
#define Addr_Gyro 0x69
#define Addr_Mag 0x13
float xAccl = 0.00;
float yAccl = 0.00;
float zAccl = 0.00;
float xGyro = 0.00;
float yGyro = 0.00;
float zGyro = 0.00;
int16_t xMag = 0;
int16_t yMag = 0;
int16_t zMag = 0;
SoftwareSerial gpsSerial(26, 27);  //GNSSのシリアル名
SoftwareSerial tweSerial(16, 17);  //tweliteのシリアル名とUARTピン番号RXTX

//SDの設定
#include <SD.h>
#include <SPI.h>
File f;
void SDopen() {
  f = SD.open("/MKS_all_code_test_log02_20240829.csv", FILE_APPEND);
}

//LPS25HBの変数設定
uint8_t Core1a;
uint8_t Core1b;
uint8_t Core1c;
int32_t Core1d;
double Core1e;

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


// =====================スニペット①(Miyake code)のsetup関数==============================
void setupCore1() {
  Serial.begin(115200);
  Wire.begin();
  pinMode(BTN_STOP_ALARM, INPUT);  //Buzzerのピン番号ちゃうやろ
  pinMode(VOL_PIN, INPUT);

  pinMode(Buzzer, OUTPUT);  //Buzzer追加

  // Create semaphore to inform us when the timer has fired
  timerSemaphore = xSemaphoreCreateBinary();
  // Use 1st timer of 4 (counted from zero).
  // Set 80 divider for prescaler (see ESP32 Technical Reference Manual for more
  // info).
  timer = timerBegin(1000000);
  //ESP32はタイマーが4つあり、0-3までのタイマーを利用できます。
  //2つ目が何クロックでカウントをするかの数値になります。80の場合、80クロックで1カウントします。

  // Attach onTimer function to our timer.
  timerAttachInterrupt(timer, &onTimer);
  //最初の引数は設定するタイマー。2つ目は割り込み時に呼ばれる割り込み関数。3つ目が割り込み検知方法です。
  // Set alarm to call onTimer function every second (value in microseconds).
  // Repeat the alarm (third parameter)
  timerAlarm(timer, 10000, true, 0);
  //割り込みが発生したときの、トリガー条件を設定します。最初の引数は設定するタイマー。2つ目はカウント数。3つ目がautoreloadで、trueの場合には定期実行、falseの場合には1ショットの実行になります。
  //1000000で１秒間隔でカウント
  // Start an alarm
  timerStart(timer);
  //タイマー有効化
  //↑変更
  //timer

  myServo.attach(SERVO_PIN);
  myServo1.attach(SERVO_PIN1);
}

void loopCore1() {
  // スニペット①(Miyake code)のループコードをここに挿入
  value = digitalRead(VOL_PIN);

  Serial.print(isrCounter);
  Serial.print(" at ");
  Serial.print(isrTime);
  Serial.println(" ms");

  if (value == LOW) {
    fly = 0;
    timerWrite(timer, 0);
    isrCounter = 0;
    count = 0;
  }

  if (value == HIGH) {
    if (fly != 1) {
      Serial.println(" risyou");  //発射検知
      fly = 1;
    }


    //timer
    // If Timer has fired



    if (xSemaphoreTake(timerSemaphore, 0) == pdTRUE) {

      // Read the interrupt count and time
      //portENTER_CRITICAL(&timerMux);  いらない（多分）
      isrCount = isrCounter;
      isrTime = lastIsrAt;
      //portEXIT_CRITICAL(&timerMux);　いらない（多分）
      // Print it
      // Serial.print("onTimer no. ");
      // Serial.println("割込み!");

      //if(0<=isrTime&&isrTime<=20){
      //Serial.println("タイマスタート");
      //}
      //if(395<=isrTime&&isrTime<=405){
      //Serial.println("燃焼終了");
      //}

      //以下はタイマーによる割り込みらしい
      //-----------!!!! 一段目の縦開放のタイマーを設定すること !!!!-------------
      if (1000 <= isrCounter) {
        if (count10 != 1) {
          timer10 = 1;
          myServo.write(0);
          delay(1000);
          // myServo.write(90);
          // delay(1000);
          myServo.write(160);
          delay(1000);


          // myServo.write(90);
        }
        //10.050秒後にモータが動く
      }

      //-----------!!!! 一段目の縦開放のタイマーを設定すること !!!!-------------
      if (1105 <= isrCounter) {
        myServo1.write(0);
        delay(1000);
        myServo1.write(60);
        delay(1000);

        //  myServo1.write(0);
        // delay(1000);
        // myServo1.write(90);
        // delay(1000);

        delay(10000);
        for (;;) {
          digitalWrite(Buzzer, HIGH);
          delay(1000);
          //digitalWrite(Buzzer,LOW);
          //delay(1000);
        }
      }
    }
    // If button is pressed
    if (digitalRead(BTN_STOP_ALARM) == LOW) {
      // If timer is still running
      if (timer) {
        // Stop and free timer
        timerEnd(timer);
        timer = NULL;
      }
    }
  }
  //timer









  //volt
  //Serial.print(count);

  //↓　気圧の値
  Wire.beginTransmission(0x5c);
  Wire.write(0x20);
  Wire.write(0xc0);
  Wire.endTransmission();

  Wire.beginTransmission(0x5c);
  Wire.write(0x28);
  Wire.endTransmission();  //必要
  Wire.requestFrom(0x5c, 1);
  a = Wire.read();

  Wire.beginTransmission(0x5c);
  Wire.write(0x29);
  Wire.endTransmission();
  Wire.requestFrom(0x5c, 1);
  b = Wire.read();


  Wire.beginTransmission(0x5c);
  Wire.write(0x2A);
  Wire.endTransmission();
  Wire.requestFrom(0x5c, 1);
  c = Wire.read();

  d = c << 16 | b << 8 | a;

  e = d / 4096.00;  //.00つける　センサーの値
  //Serial.print("e=");
  //Serial.println(e,5);

  hikaku = moving_average(e);

  Serial.println(hikaku);  //最新の平均気圧

  Serial.println(moving_average2(hikaku));  //1,0の差
}

// =======================スニペット②(SD+TWELITE+GPS+BMX055)のsetup関数=======================
void setupCore0() {
  Wire.begin();
  tweSerial.begin(115200);  //115200bps固定　絶対に変えるな
  gpsSerial.begin(9600);
  Serial.begin(115200);
  BMX055_Init();
  delay(300);
}


//=======================コード②(SD+TWELITE+GPS+BMX055)のloop変数============================================

void loopCore0() {
  // スニペット②のループコードをここに挿入
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
        tweSerial.print(NMEA2DD(list[2].toFloat()));
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

// コア1のタスク
void taskCore1(void* pvParameters) {
  setupCore1();
  while (1) {
    loopCore1();
    vTaskDelay(10 / portTICK_PERIOD_MS);  // 他のタスクに時間を与える
  }
}

// コア0のタスク
void taskCore0(void* pvParameters) {
  setupCore0();
  while (1) {
    loopCore0();
    vTaskDelay(10 / portTICK_PERIOD_MS);  // 他のタスクに時間を与える
  }
}

void setup() {
  // コア1のタスクを作成
  xTaskCreatePinnedToCore(taskCore1, "Core1Task", 10000, NULL, 1, NULL, CORE_1);

  // コア0のタスクを作成
  xTaskCreatePinnedToCore(taskCore0, "Core0Task", 10000, NULL, 1, NULL, CORE_0);
}

void loop() {
  // ここでは何もしない
}
