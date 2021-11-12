char buff[255];
int counter = 0;
int LED = 13;

void setup(){
  Serial.begin(9600);
  pinMode(LED, OUTPUT);
  Serial.println("Start reading.");
}

void loop(){
  while(Serial.available()){
    char inChar = char(Serial.read());
    //Serial.print(inChar);
    buff[counter] = inChar;
    counter++;  
    if (inChar == '\0'){
      Serial.println(buff);
      //digitalWrite(LED, HIGH);
      counter = 0;
    }else{
      //digitalWrite(LED, LOW);
    }
  }
}
