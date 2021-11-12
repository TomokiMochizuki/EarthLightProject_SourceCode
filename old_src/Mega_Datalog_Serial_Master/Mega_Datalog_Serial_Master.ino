//データロガー＆シリアル通信master
/**
   @file balloon_datalog.ino
   @brief 温湿度・気圧とGPS情報を取得する
*/
#include <SoftwareSerial.h>
#include <Wire.h>
#include <SparkFunBME280.h>
#include <math.h>
#include <SPI.h>
#include <SD.h>
BME280 sensor;

/*******************************************
              変数の定義
 *******************************************/
//温湿度・気圧センサ関連
float Temp;
float Humidity;
float Pressure;

//GPS関連
boolean GPSDataFlag = false;
int d;
int m;
float s;
int hh;
String line = "";
int len;
int i = 0;
int index = 0;
String str;
String list[30];
String preline = "";

/**************** GPSモジュール *************************/
// ソフトウェアシリアルを使用
#define MY_SERIAL_BAUD 9600
// RX = 10, TX = 11
SoftwareSerial mySerial(10, 11);

String time;      // JST時刻
String Latitude;  // 緯度
String Longitude; // 経度
String Height;    // 海抜高さ

void init_GPS() {
  for (i = 0; i < 30; i++) {
    list[i] = "";
  }
}

/**
   @brief UTC時刻から日本の標準時刻に変換する(GMT+9:00)

   @param time hhmmss.ss形式のUTC時刻
*/
String UTC2GMT900(String str) {
  hh = (str.substring(0, 2).toInt()) + 9;
  if (hh > 24) hh = hh - 24;

  return String(hh, DEC) + ":" + str.substring(2, 4) + ":" + str.substring(4, 6);
}

/**
   @brief NMEAの緯度経度を「度分秒」(DMS)の文字列に変換する

   @param val dddmm.mmmm表記
*/
String NMEA2DMS(float val) {
  d = val / 100;
  m = ((val / 100.0) - d) * 100.0;
  s = ((((val / 100.0) - d) * 100.0) - m) * 60;
  return String(d) + "度" + String(m) + "分" + String(s, 1) + "秒";
}

/**
   @brief NMEAの緯度経度を「度分」(DM)の文字列に変換する

   @param val dddmm.mmmm表記
*/
String NMEA2DM(float val) {
  d = val / 100;
  m = ((val / 100.0) - d) * 100.0;
  return String(d) + "度" + String(m, 4) + "分";
}

/**
   @brief NMEAの緯度経度を「度」(DD)の文字列に変換する

   @param val dddmm.mmmm表記
*/
String NMEA2DD(float val) {
  d = val / 100;
  m = (((val / 100.0) - d) * 100.0) / 60;
  s = (((((val / 100.0) - d) * 100.0) - m) * 60) / (60 * 60);
  return String(d + m + s, 6);
}

/**
   @brief GPSデータを出力する

*/
void GPSData() {
  // センテンスを読み込む
  line = mySerial.readStringUntil('\n');
  delay(5);
  if (line != "" && line != preline) {
    preline = line;
    index = 0;
    len = line.length();
    delay(5);
    str = "";

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
      // 位置特定品質が0(位置特定できない)以外の時
      if (list[6] != "0") {
        // 現在時刻
        //Serial.print(UTC2GMT900(list[1]));
        time = UTC2GMT900(list[1]);
        delay(5);

        // 緯度
        //Serial.print(" 緯度:");
        //Serial.print(NMEA2DMS(list[2].toFloat()));
        //Serial.print("(");
        //Serial.print(NMEA2DD(list[2].toFloat()));
        //Serial.print(")");
        Latitude = NMEA2DD(list[2].toFloat());
        delay(5);

        // 経度
        //Serial.print(" 経度:");
        //Serial.print(NMEA2DMS(list[4].toFloat()));
        //Serial.print("(");
        //Serial.print(NMEA2DD(list[4].toFloat()));
        //Serial.print(")");
        Longitude = NMEA2DD(list[4].toFloat());
        delay(5);

        // 海抜
        //Serial.print(" 海抜:");
        //Serial.print(list[9]);
        //list[10].toLowerCase();
        //Serial.print(list[10]);
        Height = list[9];
        delay(5);
        GPSDataFlag = true;
      } else {
        Serial.print("測位できませんでした。");
        GPSDataFlag = false;
      }

      //Serial.println("");
    }

    //return true;
  }
  else {
    GPSDataFlag = false;
  }
}

/****************温湿度・気圧センサモジュール*********************/
void init_BME280() { //I2C通信で制御する場合のコード
  sensor.beginI2C();  // Wire を用いて I2C 接続開始
}

void setupBME280() {
  Wire.begin();
  sensor.setI2CAddress(0x76);
  if (sensor.beginI2C()) {
    Serial.println("I2C address: 0x76");
    return;
  }
  sensor.setI2CAddress(0x77);
  if (sensor.beginI2C()) {
    Serial.println("I2C address: 0x77");
    return;
  }
  Serial.println("Sensor connect failed");
  //while(1) { }
}

void BME280_data() {
  Temp = sensor.readTempC(); //°C
  delay(5);
  Humidity = sensor.readFloatHumidity(); //%
  delay(5);
  Pressure = sensor.readFloatPressure() / 100; //hPa
  delay(5);
}


/**************** SDカード *********************/
#define FILE_NAME "TESTC.TXT"
File myFile;

/**
   @brief SDカードに書き込む
*/

void SDWriteData() {
  myFile = SD.open(FILE_NAME, FILE_WRITE);

  // if the file opened okay, write to it:
  if (myFile) {
    myFile.println("Time");
    myFile.println(time);
    delay(5);
    myFile.println("Temp,Humidity,Pressure");
    myFile.print(Temp);
    myFile.print(",");
    delay(5);
    //myFile.print("Humidity");
    myFile.print(Humidity);
    myFile.print(",");
    delay(5);
    //myFile.print("Pressure");
    myFile.println(Pressure);
    delay(5);
    myFile.println("Latitude,Longitude,Height");
    delay(5);
    myFile.print(Latitude);
    myFile.print(",");
    delay(5);
    //myFile.print("Longitude");
    myFile.print(Longitude);
    myFile.print(",");
    delay(5);
    //myFile.print("Height");
    myFile.println(Height);
    delay(5);
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }
}

/**
   @brief SDカードから読み込む
*/
void SDReadData() {
  myFile = SD.open(FILE_NAME);
  if (myFile) {
    // Serial.println("test.txt:");

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }
}

/**************** 表示用 *********************/
void Serial_print() {
  //Serial.println("This is Serial print");
  Serial.println(F("Time"));
  Serial.println(time);
//  Serial.println(F("Temp,Humidity,Pressure"));
//  Serial.print(Temp);
//  Serial.print(F(","));
//  Serial.print(Humidity);
//  Serial.print(F(","));
//  Serial.println(Pressure);

  Serial.println(F("Latitude,Longitude,Height"));
  Serial.print(Latitude);
  Serial.print(F(","));
  Serial.print(Longitude);
  Serial.print(F(","));
  Serial.println(Height);
}


/****************************Main*****************************/
void setup() {
  //GPS
  Wire.begin();
  mySerial.begin(MY_SERIAL_BAUD);
  while (!mySerial) {
  }
  Serial.begin(9600);
  init_GPS();

  //BME
//  init_BME280();
//  delay(10);
//  setupBME280();
//  delay(10);

  //SD
  Serial.print("Initializing SD card...");
  delay(5);
  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    while (1) {
    };
  }
  Serial.println("initialization done.");
}

void loop() {
  //GPS
  GPSData();

  //BME
  //BME280_data();
  
  Serial_print();
  SDWriteData();
  delay(100);

 //シリアル通信
  while(!Serial.available()){} // 受信バファーにArduino Bから送信要求がないうちは何もせずに待つ
  String dummy;
  dummy = Serial.readString(); //受信バファーから送信要求を読み捨てる
}
