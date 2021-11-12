/*
  Reading lat, long and UTC time via UBX binary commands - no more NMEA parsing!
  By: Paul Clark and Nathan Seidle
  Using the library modifications provided by @blazczak and @geeksville

  SparkFun Electronics
  Date: June 16th, 2020
  License: MIT. See license file for more information but you can
  basically do whatever you want with this code.
  This example shows how to query a Ublox module for its lat/long/altitude. We also
  turn off the NMEA output on the I2C port. This decreases the amount of I2C traffic
  dramatically.
  Note: Long/lat are large numbers because they are * 10^7. To convert lat/long
  to something google maps understands simply divide the numbers by 10,000,000. We
  do this so that we don't have to use floating point numbers.
  Leave NMEA parsing behind. Now you can simply ask the module for the datums you want!
  Feel like supporting open source hardware?
  Buy a board from SparkFun!
  ZED-F9P RTK2: https://www.sparkfun.com/products/15136
  NEO-M8P RTK: https://www.sparkfun.com/products/15005
  SAM-M8Q: https://www.sparkfun.com/products/15106
  Hardware Connections:
  Plug a Qwiic cable into the GPS and a BlackBoard
  If you don't have a platform with a Qwiic connection use the SparkFun Qwiic Breadboard Jumper (https://www.sparkfun.com/products/14425)
  Open the serial monitor at 115200 baud to see the output
*/

#include <Wire.h> //Needed for I2C to GPS


#include <SoftwareSerial.h>
#include <SparkFunBME280.h>
#include <math.h>
#include <SPI.h>
#include <SD.h>
#include <SparkFun_Ublox_Arduino_Library.h>
SFE_UBLOX_GPS myGPS;
BME280 sensor_inner;
BME280 sensor_outer;
#include <avr/wdt.h> 

void init_GPS(void);
void GPS_data(void);
void init_BME280_inner(void);
void init_BME280_outer(void);
void setupBME280_inner(void);
void setupBME280_outer(void);
void BME280_data_inner(void);
void BME280_data_outer(void);
void Serial_print(void);
void Create_Main_Buffer(void);
void SDWriteData(void);

float Temp_inner,Temp_outer;
float Humidity_inner,Humidity_outer;
float Pressure_inner,Pressure_outer;
long lastTime = 0; //Simple local timer. Limits amount if I2C traffic to Ublox module.
long SerialLastTime = 0;
long ResetTime = 0;
long waitTime;
long latitude;
long longitude;
long altitude;
byte Hour;
byte Minute;
byte Second;
String Main_Buffer;
String Data_Buffer;
String Send_Buffer;

#define TrueOrFalse 0
#define FILE_NAME "sample10.txt"
File myFile;

void setup()
{
  wdt_enable(WDTO_8S);
  Serial.begin(38400);
  while (!Serial); //Wait for user to open terminal
  Wire.begin();
  init_GPS();
  delay(100);
  init_BME280_inner();
  delay(10);
  setupBME280_inner();
  delay(10);
  init_BME280_outer();
  delay(10);
  setupBME280_outer();
  delay(10);
  Serial.print("Initializing SD card...");delay(5);
  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");
  pinMode(12, OUTPUT);
  pinMode(13, INPUT);
  digitalWrite(12, HIGH);
}

void loop()
{
  
  GPS_data();
  delay(50);
  BME280_data_inner();
  delay(10);
  BME280_data_outer();
  delay(10);
  Create_Main_Buffer();
  SDWriteData();
  //Serial_print();
  wdt_reset();
  GPS_data();
  delay(50);
  BME280_data_inner();
  delay(10);
  BME280_data_outer();
  delay(10);
  Create_Main_Buffer();
  SDWriteData();
  Serial_print();
  wdt_reset();
  if (millis() - ResetTime >= 60000){
    digitalWrite(12, LOW);
    ResetTime = millis();
    delay(10);
    digitalWrite(12, HIGH);
  }
}


/****************************************************
  　　　　　　　プロトタイプ宣言した関数
 ****************************************************/

/****************GPS*************************/

void init_GPS(void) {
  Serial.println("SparkFun Ublox Example");
  //myGPS.enableDebugging(); // Uncomment this line to enable debug messages
  
  if (myGPS.begin() == false) //Connect to the Ublox module using Wire port
  {
    Serial.println(F("Ublox GPS not detected at default I2C address. Please check wiring. Freezing."));
    while (1);
  }

  myGPS.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)
  myGPS.saveConfiguration(); //Save the current settings to flash and BBR
}

void GPS_data(void) {
  //Query module only every second. Doing it more often will just cause I2C traffic.
  //The module only responds when a new position is available
  waitTime = 1250 - (millis()-lastTime);
  if (waitTime >= 0){
    delay(waitTime);
  }
  lastTime = millis(); //Update the timer
  latitude = myGPS.getLatitude(); delay(5);
  #if TrueOrFalse
  Serial.print(F("Lat: "));
  Serial.print(latitude);
  #endif
  longitude = myGPS.getLongitude(); delay(5);
  #if TrueOrFalse
  Serial.print(F(" Long: "));
  Serial.print(longitude);
  Serial.print(F(" (degrees * 10^-7)"));
  #endif
  altitude = myGPS.getAltitude(); delay(5);
  #if TrueOrFalse
  Serial.print(F(" Alt: "));
  Serial.print(altitude);
  Serial.print(F(" (mm)"));

  Serial.print(F(" Time: "));
  #endif
  Hour = myGPS.getHour(); delay(5);
  #if TrueOrFalse
  if (Hour < 10)
  {
    Serial.print(F("0"));
  }
  Serial.print(Hour);
  Serial.print(F(":"));
  #endif
  Minute = myGPS.getMinute(); delay(5);
  #if TrueOrFalse
  if (Minute < 10)
  {
    Serial.print(F("0"));
  }
  Serial.print(Minute);
  Serial.print(F(":"));
  #endif
  Second = myGPS.getSecond(); delay(5);
  #if TrueOrFalse
  if (Second < 10)
  {
    Serial.print(F("0"));
  }
  Serial.print(Second);

  Serial.println();
  #endif
}


/****************温湿度・気圧センサ*********************/
void init_BME280_inner(void) { //I2C通信で制御する場合のコード
  //Serial.begin(115200);// 115200bps
  //Wire.begin();
  sensor_inner.beginI2C();  // Wire を用いて I2C 接続開始
}

void init_BME280_outer(void) { //I2C通信で制御する場合のコード
  //Serial.begin(115200);// 115200bps
  //Wire.begin();
  sensor_outer.beginI2C();  // Wire を用いて I2C 接続開始
}

void setupBME280_inner(void) {
  Wire.begin();
  sensor_inner.setI2CAddress(0x76);
  if (sensor_inner.beginI2C()) {
    Serial.println("I2C address: 0x76");
    return;
  }
  Serial.println("sensor_inner connect failed");
  //while(1) { }
}

void setupBME280_outer(void) {
  Wire.begin();
  sensor_outer.setI2CAddress(0x77);
  if (sensor_outer.beginI2C()) {
    Serial.println("I2C address: 0x77");
    return;
  }
  Serial.println("sensor_outer connect failed");
  //while(1) { }
}

void BME280_data_inner(void) {
  Temp_inner = sensor_inner.readTempC(); //°C
  delay(5);
  Humidity_inner = sensor_inner.readFloatHumidity(); //%
  delay(5);
  Pressure_inner = sensor_inner.readFloatPressure() / 100; //hPa
  delay(5);
  #if TrueOrFalse
  Serial.println(F("Temp,Humidity,Pressure"));
  Serial.print(Temp_inner); delay(5);
  Serial.print(F(","));  delay(5);
  //myFile.print(F("Humidity"));
  Serial.print(Humidity_inner); delay(5);
  Serial.print(F(",")); delay(5);
  //myFile.print(F("Pressure"));
  Serial.println(Pressure_inner);  delay(5);
  delay(5);
  #endif
}

void BME280_data_outer(void) {
  Temp_outer = sensor_outer.readTempC(); //°C
  delay(5);
  Humidity_outer = sensor_outer.readFloatHumidity(); //%
  delay(5);
  Pressure_outer = sensor_outer.readFloatPressure() / 100; //hPa
  delay(5);
  #if TrueOrFalse
  Serial.println(F("Temp,Humidity,Pressure"));
  Serial.print(Temp_outer); delay(5);
  Serial.print(F(","));  delay(5);
  //myFile.print(F("Humidity"));
  Serial.print(Humidity_outer); delay(5);
  Serial.print(F(",")); delay(5);
  //myFile.print(F("Pressure"));
  Serial.println(Pressure_outer);  delay(5);
  delay(5);
  #endif
}

void Serial_print(void){
  //String result = Main_Buffer.substring(0);
  //Serial.println(result);
 //while(digitalRead(13));
 Send_Buffer = Main_Buffer;
 Send_Buffer.concat("\nDONE\n");
 Serial.println(Send_Buffer); 
}

void Create_Main_Buffer(void){
  Main_Buffer.remove(0);
  Main_Buffer.concat("Time: \n");
  if (Hour < 10){
    Main_Buffer.concat("0");
  }
  Main_Buffer.concat(Hour);
  Main_Buffer.concat(F(":"));
  if (Minute < 10){
    Main_Buffer.concat("0");
  }
  Main_Buffer.concat(Minute);
  Main_Buffer.concat(":");
  if (Second < 10){
    Main_Buffer.concat("0");
  }
  Main_Buffer.concat(Second);
  Main_Buffer.concat("\n");

  Main_Buffer.concat("Temp,Humidity,Pressure\n");
  Main_Buffer.concat(Temp_outer);
  Main_Buffer.concat(","); 
  Main_Buffer.concat(Humidity_outer);
  Main_Buffer.concat(",");
  Main_Buffer.concat(Pressure_outer);
  Main_Buffer.concat("\n");
  
  Main_Buffer.concat("Latitude,Longitude,Height\n"); 
  Main_Buffer.concat(latitude/10000000);
  Main_Buffer.concat(".");
  Main_Buffer.concat(latitude%10000000);
  Main_Buffer.concat(",");
  Main_Buffer.concat(longitude/10000000);
  Main_Buffer.concat(".");
  Main_Buffer.concat(longitude%10000000);
  Main_Buffer.concat(",");
  Main_Buffer.concat(altitude/1000);
  Main_Buffer.concat(".");
  Main_Buffer.concat(altitude%1000);
}

void SDWriteData(void) {
  
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  myFile = SD.open(FILE_NAME, FILE_WRITE);

  Data_Buffer = Main_Buffer;
  Data_Buffer.concat("\n");

  Data_Buffer.concat("Temp_inner,Humidity_inner,Pressure_inner\n");
  Data_Buffer.concat(Temp_inner);
  Data_Buffer.concat(","); 
  Data_Buffer.concat(Humidity_inner);
  Data_Buffer.concat(",");
  Data_Buffer.concat(Pressure_inner);
  // if the file opened okay, write to it:
  if (myFile) {
    myFile.println(Data_Buffer);
    myFile.close();
   // Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
   // Serial.println("error opening test.txt");
  }
}
