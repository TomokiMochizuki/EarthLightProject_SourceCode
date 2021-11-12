#include <SD.h>
#include <SoftwareSerial.h>

#define X_IN 0
#define Y_IN 1
#define Z_IN 2

#define REPEAT 100
#define FILE_NAME "test_launch.txt"
File myFile;
const int sd_chip_select = 4;
long x, y, z;
String Buffer;

//The setup function is called once at startup of the sketch
void setup()
{
// Add your initialization code here
  Serial.begin(9600);
  while (!Serial); //Wait for user to open terminal
  Serial.print("Initializing SD card...");delay(5);
  if (!SD.begin(sd_chip_select)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");
}

// The loop function is called in an endless loop
void loop()
{
//Add your repeated code here

  x = 0;
  y = 0;
  z = 0;
  for(int i = 0 ; i < REPEAT ; i++){
    x += analogRead(X_IN);
    y += analogRead(Y_IN);
    z += analogRead(Z_IN);
  }

  x /= REPEAT;
  y /= REPEAT;
  z /= REPEAT;

  //Serial.print("X:");
  //Serial.print(9.8*(x-500)/(700-500));
  //Serial.print(" Y:");
  //Serial.print(9.8*(y-500)/(700-500));
  //Serial.print(" Z:");
  //Serial.println(9.8*(z-500)/(700-500));

  delay(20);
  Buffer.remove(0);
  Buffer.concat("X:");
  Buffer.concat(9.8*(x-500)/(700-500));
  Buffer.concat(" Y:");
  Buffer.concat(9.8*(y-500)/(700-500));
  Buffer.concat(" Z:");
  Buffer.concat(9.8*(z-500)/(700-500));
  Serial.println(Buffer);
  SDWriteData();
}

void SDWriteData(void) {
  
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  myFile = SD.open(FILE_NAME, FILE_WRITE);

  // if the file opened okay, write to it:
  if (myFile) {
    myFile.println(Buffer);
    myFile.close();
    //Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    //Serial.println("error opening test.txt");
  }
}
