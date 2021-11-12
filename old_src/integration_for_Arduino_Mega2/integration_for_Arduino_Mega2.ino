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
BME280 sensor;
#include <avr/wdt.h> 

void init_GPS(void);
void GPS_data(void);
void init_BME280(void);
void setupBME280(void);
void BME280_data(void);
void Serial_print(void);
void Create_Buffer(void);
void SDWriteData(void);

float Temp;
float Humidity;
float Pressure;
long lastTime = 0; //Simple local timer. Limits amount if I2C traffic to Ublox module.
long SerialLastTime = 0;
long waitTime;
long latitude;
long longitude;
long altitude;
byte Hour;
byte Minute;
byte Second;
String Buffer;

#define TrueOrFalse 0
#define FILE_NAME "sample9.txt"
File myFile;

void setup()
{
  wdt_enable(WDTO_8S);
  Serial.begin(38400);
  while (!Serial); //Wait for user to open terminal
  Wire.begin();
  init_GPS();
  delay(100);
  init_BME280();
  delay(10);
  setupBME280();
  delay(10);
  Serial.print("Initializing SD card...");delay(5);
  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");
  pinMode(13, INPUT); 
}

void loop()
{
  
  GPS_data();
  delay(50);
  BME280_data();
  delay(10);
  Create_Buffer();
  SDWriteData();
  //Serial_print();
  wdt_reset();
  GPS_data();
  delay(50);
  BME280_data();
  delay(10);
  Create_Buffer();
  SDWriteData();
  Serial_print();
  wdt_reset();
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
  waitTime = 1000 - (millis()-lastTime);
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

void BME280_data(void) {
  Temp = sensor.readTempC(); //°C
  delay(5);
  Humidity = sensor.readFloatHumidity(); //%
  delay(5);
  Pressure = sensor.readFloatPressure() / 100; //hPa
  delay(5);
  #if TrueOrFalse
  Serial.println(F("Temp,Humidity,Pressure"));
  Serial.print(Temp); delay(5);
  Serial.print(F(","));  delay(5);
  //myFile.print(F("Humidity"));
  Serial.print(Humidity); delay(5);
  Serial.print(F(",")); delay(5);
  //myFile.print(F("Pressure"));
  Serial.println(Pressure);  delay(5);
  delay(5);
  #endif
}
/*
void Serial_print(void){
  delay(5);
  Serial.println(F("Time: "));delay(5);
  if (Hour < 10){
    Serial.print(F("0"));
  }
  Serial.print(Hour);delay(5);
  Serial.print(F(":"));
  if (Minute < 10){
    Serial.print(F("0"));
  }
  Serial.print(Minute);delay(5);
  Serial.print(F(":"));
  if (Second < 10){
    Serial.print(F("0"));
  }
  Serial.print(Second);delay(5);
  Serial.println();

  Serial.println(F("Temp,Humidity,Pressure"));
  Serial.print(Temp); delay(5);
  Serial.print(F(","));  delay(5);
  //myFile.print(F("Humidity"));
  Serial.print(Humidity); delay(5);
  Serial.print(F(",")); delay(5);
  //myFile.print(F("Pressure"));
  Serial.println(Pressure);  delay(5);
  delay(5);
  Serial.println(F("Latitude,Longitude,Height")); 
  Serial.print(latitude/10000000);
  Serial.print(F("."));
  Serial.print(latitude%10000000);
  delay(5);
  Serial.print(F(","));delay(5);
  Serial.print(longitude/10000000);
  Serial.print(F("."));
  Serial.print(longitude%10000000);
  Serial.print(F(","));delay(5);
  Serial.print(altitude/1000);
  Serial.print(F("."));
  Serial.println(altitude%1000);
}
*/
void Serial_print(void){
  //String result = Buffer.substring(0);
  //Serial.println(result);
 //while(digitalRead(13));
 Buffer.concat("\nDONE\n");
 Serial.println(Buffer); 
}

void Create_Buffer(void){
  Buffer.remove(0);
  Buffer.concat("Time: \n");
  if (Hour < 10){
    Buffer.concat("0");
  }
  Buffer.concat(Hour);
  Buffer.concat(F(":"));
  if (Minute < 10){
    Buffer.concat("0");
  }
  Buffer.concat(Minute);
  Buffer.concat(":");
  if (Second < 10){
    Buffer.concat("0");
  }
  Buffer.concat(Second);
  Buffer.concat("\n");

  Buffer.concat("Temp,Humidity,Pressure\n");
  Buffer.concat(Temp);
  Buffer.concat(","); 
  Buffer.concat(Humidity);
  Buffer.concat(",");
  Buffer.concat(Pressure);
  Buffer.concat("\n");
  
  Buffer.concat("Latitude,Longitude,Height\n"); 
  Buffer.concat(latitude/10000000);
  Buffer.concat(".");
  Buffer.concat(latitude%10000000);
  Buffer.concat(",");
  Buffer.concat(longitude/10000000);
  Buffer.concat(".");
  Buffer.concat(longitude%10000000);
  Buffer.concat(",");
  Buffer.concat(altitude/1000);
  Buffer.concat(".");
  Buffer.concat(altitude%1000);
}

void SDWriteData(void) {
  
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  myFile = SD.open(FILE_NAME, FILE_WRITE);

  // if the file opened okay, write to it:
  if (myFile) {
    myFile.println(Buffer);
    myFile.close();
   // Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
   // Serial.println("error opening test.txt");
  }
}
