#include <ESP32Servo.h>
#include<Wire.h>
const int VOL_PIN = 32;

Servo myServo;
Servo myServo1;
const int SERVO_PIN=25;
const int SERVO_PIN1=13;

uint8_t a;
uint8_t b;
uint8_t c;
int32_t d;

//int term=2;//標本数

//int number=0;//データ番号
double stock_data_e[5]={0}; //データをストックする配列
double e;//気圧のデータ(最新)
double stock_data_ave[5]={0};
double hikaku;
double ave1;
double ave2;
double ave3;
double ave4;
int count=0;//移動平均検知回数.
int count10=0;//気圧開傘されたか.
int timer10=0;//タイマー開傘されたか.
int fly=0;//フライトピンが抜けてるか.
 uint32_t isrCount = 0, isrTime = 0;
//timer

int value;

#define BTN_STOP_ALARM    0
hw_timer_t * timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
volatile uint32_t isrCounter = 0;
volatile uint32_t lastIsrAt = 0;
void IRAM_ATTR onTimer(){
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

double moving_average2(double hikaku){
 

  for(int ii=0;ii<4;ii+=1){
    stock_data_ave[ii]=stock_data_ave[ii+1];
    
  }

  stock_data_ave[4]=hikaku; //平均の最新のデータ
  //Serial.print("new=");
  //Serial.println(hikaku);
  Serial.println("-------------");
  ave1=stock_data_ave[1]-stock_data_ave[0];
  Serial.print("移動平均前後の差1=");
    Serial.println(ave1,5);
  ave2=stock_data_ave[2]-stock_data_ave[1];
  Serial.print("移動平均前後の差2=");
  Serial.println(ave2,5);
  ave3=stock_data_ave[3]-stock_data_ave[2];
  Serial.print("移動平均前後の差3=");
  Serial.println(ave3,5);
  ave4=stock_data_ave[4]-stock_data_ave[3];
  Serial.print("移動平均前後の差4=");
  Serial.println(ave4,5);
  //↑　4つの気圧平均比較

  if(ave1>0 && ave2>0 && ave3>0 && ave4>0){
    if(400<=isrCounter){
      if(fly=1){
        count=count+1;
      }
    }
  }

  
  Serial.print("count=");
  Serial.println(count);
  
  


  if(count>10){
    if(timer10!=1){
      count10=1;
      myServo.write(0);
      delay(1000);
      myServo.write(160);
      delay(1000);
    }
  }
  return ave1;

}


double moving_average(double konndou){
//number ++;
  //if(number>5){
    //number=0;
  //}


  for(int i=0;i<4;i+=1){
    stock_data_e[i]=stock_data_e[i+1];
    //Serial.println(stock_data_e[i]);
   //Serial.print(stock_data_e[i],stock_data_e[i+1]);
  }

  stock_data_e[4]=konndou; 
  //Serial.print("new=");
  //Serial.println(konndou);
  double result=0;//初期化
  for(int i=0;i<5;i+=1){

    //result +=stock_data_e[i];//合計
    result +=stock_data_e[i];//合計

  }

  result=result/5.00;//平均算出

  return result;

}

void setup()
{
  Serial.begin(9600);
  Wire.begin();
  //timer
  pinMode(BTN_STOP_ALARM, INPUT);
  pinMode(VOL_PIN, INPUT);
  // Create semaphore to inform us when the timer has fired
  timerSemaphore = xSemaphoreCreateBinary();
  // Use 1st timer of 4 (counted from zero).
  // Set 80 divider for prescaler (see ESP32 Technical Reference Manual for more
  // info).
  //timer = timerBegin(1000000);
 // 川村変更点
 //タイマーの初期化
  timer = timerBegin(0, 80, true); // タイマーインデックス 0、プリスケーラ 80 (1usごとに1カウント)、カウンタは増加

  //ESP32はタイマーが4つあり、0-3までのタイマーを利用できます。
  //2つ目が何クロックでカウントをするかの数値になります。80の場合、80クロックで1カウントします。

  // Attach onTimer function to our timer.
  //timerAttachInterrupt(timer, &onTimer);
 timerAttachInterrupt(timer, &onTimer, true); // 割り込みを割り当てる
  //最初の引数は設定するタイマー。2つ目は割り込み時に呼ばれる割り込み関数。3つ目が割り込み検知方法です。
  // Set alarm to call onTimer function every second (value in microseconds).
  // Repeat the alarm (third parameter)
  //timerAlarm(timer,10000,true,0);
 // 10,000カウント後にアラームが発動 (10000us = 10ms)
  timerAlarmWrite(timer, 10000000, true);
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

void loop() {


  value = digitalRead( VOL_PIN );
  Serial.print(isrCounter);
  Serial.print(" at ");
  Serial.print(isrTime);
  Serial.println(" ms");

  if(value ==LOW){
    fly=0;
    timerWrite(timer, 0);
    isrCounter = 0;
    count = 0;
  }

  if(value == HIGH){
    if(fly !=1){
      Serial.println(" risyou");
      fly =1;
    }
    

    //timer
    // If Timer has fired



    if (xSemaphoreTake(timerSemaphore, 0) == pdTRUE){
    
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

      if(1000<=isrCounter ){
        if(count10 !=1){
          timer10=1;
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


      if(1105<=isrCounter){
        myServo1.write(0);
        delay(1000);
        myServo1.write(60);
        delay(1000);

       
      for (;;) {
              digitalWrite(Buzzer, HIGH);
              delay(1000);
              //digitalWrite(Buzzer,LOW);
              //delay(1000);
            }
        //  myServo1.write(0);
        // delay(1000);
        // myServo1.write(90);
        // delay(1000);
      }



    }
    // If button is pressed
    if (digitalRead(BTN_STOP_ALARM) == LOW){
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
  Wire.endTransmission();//必要
  Wire.requestFrom(0x5c,1);
  a=Wire.read();
 
  Wire.beginTransmission(0x5c);
  Wire.write(0x29);
  Wire.endTransmission();
  Wire.requestFrom(0x5c,1);
  b=Wire.read();
 

  Wire.beginTransmission(0x5c);
  Wire.write(0x2A);
  Wire.endTransmission();
  Wire.requestFrom(0x5c,1);
  c=Wire.read();

  d=c<<16|b<<8|a;

  e=d/4096.00;//.00つける　センサーの値
  //Serial.print("e=");
  //Serial.println(e,5);

  hikaku=moving_average(e);

  Serial.println(hikaku);//最新の平均気圧

  Serial.println(moving_average2(hikaku));//1,0の差

}
