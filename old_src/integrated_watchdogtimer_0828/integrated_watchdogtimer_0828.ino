#include <SoftwareSerial.h>
#include <Wire.h>
#include <MadgwickAHRS.h>
#include <SparkFunBME280.h>
#include <math.h>
#include <SPI.h>
#include <SD.h>
#include <avr/wdt.h> 

SoftwareSerial mySerial(12,13);// MegaGPS
//SoftwareSerial mySerial(2,3);
BME280 sensor;
Madgwick MadgwickFilter;


/********************************************
            プロトタイプ宣言
 ********************************************/
String NMEA2DMS(float val);
String NMEA2DM(float val);
String NMEA2DD(float val);
String UTC2GMT900(String str);
void GPSData(void);
void init_GPS(void);
//boolean GPS_data(void);
void init_BME280(void);
void setupBME280(void);
void BME280_data(void);
void init_BMX055(void);
void BMX055_Accl(void);
void BMX055_Gyro(void);
void BMX055_Mag(void);
void BMX055_data(void);
void SDWriteData(void);
void SDReadData(void);
void Serial_print(void);
/*******************************************
              変数の定義
 *******************************************/
#define Addr_Accl 0x19  // (JP1,JP2,JP3 = Open)
#define Addr_Gyro 0x69  // (JP1,JP2,JP3 = Open)
#define Addr_Mag 0x13   // (JP1,JP2,JP3 = Open)
#define GRYO 1
#define ACCL 2
#define MAG 3
#define XYZ_ROTATION 4
#define FILE_NAME "testb_8.txt"
File myFile;

float xAccl = 0.00;
float yAccl = 0.00;
float zAccl = 0.00;
float xGyro = 0.00;
float yGyro = 0.00;
float zGyro = 0.00;
float   xMag  = 0;
float   yMag  = 0;
float   zMag  = 0;
float  roll = 0  ;
float  pitch = 0 ;
float  yaw = 0 ;
float Temp;
float Humidity;
float Pressure;
String Latitude;
String Longitude;
String Height;
String Time;
 //GPS関連
 boolean GPSDataFlag=false;
 int d;
 int m;
 float s;
 int hh;
 String line="";
 int len;
 int i=0;
 int index=0;
 String str;
 String list[30];
 String preline="";
/********************************************
            main関数
 ********************************************/
void setup() {
  wdt_enable(WDTO_8S);
  //Wire(Arduino-I2C)
  Wire.begin();
  mySerial.begin(9600);
  // 57600bps 
  Serial.begin(57600);
  init_GPS(); 
  //delay(100);
  init_BME280();
  delay(10);
  setupBME280();
  delay(10);
  init_BMX055();
  delay(10);
  MadgwickFilter.begin(10);
  Serial.print("Initializing SD card...");
 delay(5);
  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");
}
void loop() {
  init_GPS(); 
  delay(5);  
  GPSData();
  if(GPSDataFlag){ //GPSが取得出来たら
    BME280_data();
    BMX055_data();
    Serial_print();
    SDWriteData();
    delay(2000);
  }
  GPSDataFlag=false;
  wdt_reset();
}


/****************************************************
  　　　　　　　プロトタイプ宣言した関数
 ****************************************************/

/****************GPS*************************/
void init_GPS(void){
  for (i = 0; i < 30; i++) {
      list[i] = "";
    }
}
// NMEAの緯度経度を「度分秒」(DMS)の文字列に変換する
String NMEA2DMS(float val) {
  d = val / 100;
  m = ((val / 100.0) - d) * 100.0;
  s = ((((val / 100.0) - d) * 100.0) - m) * 60;
  return String(d) + "度" + String(m) + "分" + String(s, 1) + "秒";
}
 
// (未使用)NMEAの緯度経度を「度分」(DM)の文字列に変換する
String NMEA2DM(float val) {
  d = val / 100;
  m = ((val / 100.0) - d) * 100.0;
  return String(d) + "度" + String(m, 4) + "分";
}
 
// NMEAの緯度経度を「度」(DD)の文字列に変換する
String NMEA2DD(float val) {
  d = val / 100;
  m = (((val / 100.0) - d) * 100.0) / 60;
  s = (((((val / 100.0) - d) * 100.0) - m) * 60) / (60 * 60);
  return String(d + m + s, 6);
}
 
// UTC時刻から日本の標準時刻に変換する(GMT+9:00)
String UTC2GMT900(String str) {
  hh = (str.substring(0,2).toInt()) + 9;
  if(hh > 24) hh = hh - 24;
 
  return String(hh,DEC) + ":" + str.substring(2,4) + ":" + str.substring(4,6);  
}


void GPSData(void) {
  // 1つのセンテンスを読み込む
  line = mySerial.readStringUntil('\n');
  delay(5);
  if(line != "" && line!= preline){
    preline=line;
    index = 0;
    len = line.length();delay(5);
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
      
      // ステータス
      if(list[6] != "0"){      
        // 現在時刻
        //Serial.print(UTC2GMT900(list[1]));//  22:59:08
        delay(5);
        Time=UTC2GMT900(list[1]);delay(5);
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
        delay(5);GPSDataFlag= true;
      }else{
        Serial.print("測位できませんでした。");
        GPSDataFlag= false;
      }
      
      //Serial.println("");
    }
    
   //return true;
  }
  else{
  GPSDataFlag= false;}
}
/****************温湿度・気圧センサ*********************/
void init_BME280(void) { //I2C通信で制御する場合のコード
  //Serial.begin(115200);// 115200bps
  //Wire.begin();
  sensor.beginI2C();  // Wire を用いて I2C 接続開始
}

void setupBME280(void) {
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

void BME280_data(void){
  Temp=sensor.readTempC(); //°C 
  delay(5);
  Humidity=sensor.readFloatHumidity(); //%
  delay(5);
  Pressure=sensor.readFloatPressure()/100; //hPa
  delay(5);
  }
/****************9軸センサ*********************/
void init_BMX055(void) {
  //------------------------------------------------------------//
  Wire.beginTransmission(Addr_Accl);
  Wire.write(0x0F); // Select PMU_Range register
  Wire.write(0x03);   // Range = +/- 2g
  Wire.endTransmission();
  delay(10);
  //------------------------------------------------------------//
  Wire.beginTransmission(Addr_Accl);
  Wire.write(0x10);  // Select PMU_BW register
  Wire.write(0x08);  // Bandwidth = 7.81 Hz
  Wire.endTransmission();
  delay(10);
  //------------------------------------------------------------//
  Wire.beginTransmission(Addr_Accl);
  Wire.write(0x11);  // Select PMU_LPW register
  Wire.write(0x00);  // Normal mode, Sleep duration = 0.5ms
  Wire.endTransmission();
  delay(10);
  //------------------------------------------------------------//
  Wire.beginTransmission(Addr_Gyro);
  Wire.write(0x0F);  // Select Range register
  Wire.write(0x04);  // Full scale = +/- 125 degree/s
  Wire.endTransmission();
  delay(10);
  //------------------------------------------------------------//
  Wire.beginTransmission(Addr_Gyro);
  Wire.write(0x10);  // Select Bandwidth register
  Wire.write(0x07);  // ODR = 100 Hz
  Wire.endTransmission();
  delay(10);
  //------------------------------------------------------------//
  Wire.beginTransmission(Addr_Gyro);
  Wire.write(0x11);  // Select LPM1 register
  Wire.write(0x00);  // Normal mode, Sleep duration = 2ms
  Wire.endTransmission();
  delay(10);
  //------------------------------------------------------------//
  Wire.beginTransmission(Addr_Mag);
  Wire.write(0x4B);  // Select Mag register
  Wire.write(0x83);  // Soft reset
  Wire.endTransmission();
  delay(10);
  //------------------------------------------------------------//
  Wire.beginTransmission(Addr_Mag);
  Wire.write(0x4B);  // Select Mag register
  Wire.write(0x01);  // Soft reset
  Wire.endTransmission();
  delay(10);
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
//=====================================================================================//
void BMX055_Accl(void) {
  int data[6];
  for (int i = 0; i < 6; i++)
  {
    Wire.beginTransmission(Addr_Accl);
    Wire.write((2 + i));// Select data register
    Wire.endTransmission();
    Wire.requestFrom(Addr_Accl, 1);// Request 1 byte of data
    // Read 6 bytes of data
    // xAccl lsb, xAccl msb, yAccl lsb, yAccl msb, zAccl lsb, zAccl msb
    if (Wire.available() == 1)
      data[i] = Wire.read();
  }
  // Convert the data to 12-bits
  xAccl = ((data[1] * 256) + (data[0] & 0xF0)) / 16;
  if (xAccl > 2047)  xAccl -= 4096;
  yAccl = ((data[3] * 256) + (data[2] & 0xF0)) / 16;
  if (yAccl > 2047)  yAccl -= 4096;
  zAccl = ((data[5] * 256) + (data[4] & 0xF0)) / 16;
  if (zAccl > 2047)  zAccl -= 4096;
  xAccl = xAccl * 0.0098; // renge +-2g
  yAccl = yAccl * 0.0098; // renge +-2g
  zAccl = zAccl * 0.0098; // renge +-2g
}
//=====================================================================================//
void BMX055_Gyro(void) {
  int data[6];
  for (int i = 0; i < 6; i++)
  {
    Wire.beginTransmission(Addr_Gyro);
    Wire.write((2 + i));    // Select data register
    Wire.endTransmission();
    Wire.requestFrom(Addr_Gyro, 1);    // Request 1 byte of data
    // Read 6 bytes of data
    // xGyro lsb, xGyro msb, yGyro lsb, yGyro msb, zGyro lsb, zGyro msb
    if (Wire.available() == 1)
      data[i] = Wire.read();
  }
  // Convert the data
  xGyro = (data[1] * 256) + data[0];
  if (xGyro > 32767)  xGyro -= 65536;
  yGyro = (data[3] * 256) + data[2];
  if (yGyro > 32767)  yGyro -= 65536;
  zGyro = (data[5] * 256) + data[4];
  if (zGyro > 32767)  zGyro -= 65536;
  xGyro = xGyro * 0.0038; //  Full scale = +/- 125 degree/s
  yGyro = yGyro * 0.0038; //  Full scale = +/- 125 degree/s
  zGyro = zGyro * 0.0038; //  Full scale = +/- 125 degree/s
}
//=====================================================================================//
void BMX055_Mag(void) {
  int data[8];
  for (int i = 0; i < 8; i++)
  {
    Wire.beginTransmission(Addr_Mag);
    Wire.write((0x42 + i));    // Select data register
    Wire.endTransmission();
    Wire.requestFrom(Addr_Mag, 1);    // Request 1 byte of data
    // Read 6 bytes of data
    // xMag lsb, xMag msb, yMag lsb, yMag msb, zMag lsb, zMag msb
    if (Wire.available() == 1)
      data[i] = Wire.read();
  }
  // Convert the data
  xMag = ((data[1] << 8) | (data[0] >> 3));
  if (xMag > 4095)  xMag -= 8192;
  yMag = ((data[3] << 8) | (data[2] >> 3));
  if (yMag > 4095)  yMag -= 8192;
  zMag = ((data[5] << 8) | (data[4] >> 3));
  if (zMag > 16383)  zMag -= 32768;
}
void BMX055_data(void) {
  BMX055_Gyro();
  BMX055_Accl();
  BMX055_Mag();
  MadgwickFilter.update(xGyro, yGyro, zGyro, xAccl, yAccl, zAccl, xMag, yMag, zMag);
  roll  = MadgwickFilter.getRoll();
  pitch = MadgwickFilter.getPitch();
  yaw   = MadgwickFilter.getYaw();
}


void SDWriteData(void) {
  
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  myFile = SD.open(FILE_NAME, FILE_WRITE);

  // if the file opened okay, write to it:
  if (myFile) {
   // Serial.print("Writing to test.txt...");delay(5);
    myFile.println("Time");
    myFile.println(Time); delay(5);
    myFile.println("Temp,Humidity,Pressure"); 
    myFile.print(Temp);
    myFile.print(",");  delay(5);
    //myFile.print("Humidity");   
    myFile.print(Humidity);//delay(5);
    myFile.print(",");delay(5);
    //myFile.print("Pressure");   
    myFile.println(Pressure); //delay(5);
    delay(5);
    //myFile.print("XYZ_ROTATION");  
    //myFile.println(XYZ_ROTATION);
    myFile.println("roll,pitch,yaw");     delay(5);
    myFile.print(roll);
    myFile.print(",");delay(5);
    //myFile.print("pitch");   
    myFile.print(pitch);
    myFile.print(","); delay(5);
    //myFile.print("yaw");   
    myFile.println(yaw);
    delay(5);
    myFile.println("Latitude,Longitude,Height");  delay(5);   
    myFile.print(Latitude);
    myFile.print(",");delay(5);
    //myFile.print("Longitude");   
    myFile.print(Longitude);
    myFile.print(",");delay(5);
    //myFile.print("Height");   
    myFile.println(Height);
    delay(5);
    // close the file:
    myFile.close();
   // Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
   // Serial.println("error opening test.txt");
  }
}
void SDReadData(void) {
  // re-open the file for reading:
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
   // Serial.println("error opening test.txt");
  }
}
void Serial_print(void){
    delay(5);
    //Serial.println("This is Serial print");
    Serial.println(F("Time"));delay(5);
    Serial.println(Time);delay(5);
    Serial.println(F("Temp,Humidity,Pressure")); 
    Serial.print(Temp);delay(5);
    Serial.print(F(","));  delay(5);
    //myFile.print(F("Humidity"));   
    Serial.print(Humidity); delay(5);
    Serial.print(F(","));delay(5);
    //myFile.print(F("Pressure"));   
    Serial.println(Pressure);  delay(5);
    delay(5);
    //myFile.print("XYZ_ROTATION");  
    //myFile.println(XYZ_ROTATION);
    Serial.println(F("roll,pitch,yaw"));  delay(5);   
    Serial.print(roll);delay(5);
    Serial.print(F(","));  delay(5);
    //myFile.print("pitch");   
    Serial.print(pitch);delay(5);
    Serial.print(F(","));delay(5);
    //myFile.print("yaw");   
    Serial.println(yaw);delay(5);
    
    Serial.println(F("Latitude,Longitude,Height"));     
    Serial.print(Latitude);delay(5);
    Serial.print(F(","));  delay(5);
    //myFile.print("Longitude");   
    Serial.print(Longitude);
    Serial.print(F(","));  delay(5);
    //myFile.print("Height");   
    Serial.println(Height); 
    delay(5);
}
