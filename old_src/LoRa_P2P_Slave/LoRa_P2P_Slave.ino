#define DGHARDUINO_PIN_LORA_POWER (6)
#define DGHARDUINO_PIN_LORA_RESET (7)
#define DGHARDUINO_PIN_LORA_WAKEUP (8)
#define DGHARDUINO_PIN_CPU_LORA_UART_RX (9)
#define DGHARDUINO_PIN_CPU_LORA_UART_TX (10)
#define DGHARDUINO_PIN_CPU_LORA_SDA (A4)
#define DGHARDUINO_PIN_CPU_LORA_SCL (A5)

static const uint16_t u16Channel = 9212;/* チャンネル：921.2Mhz  */
static const uint16_t u16DR = 0;/* Date rate = 0 */

static const char* pFromAddr = "10:01";/* 自身デバイスアドレス 1001.*/
static const uint16_t u16ToAddr = 1000;/*相手デバイスアドレス 1000.*/

/*結果コード begin*/
static const char* respSleep[] ={"Sleep.","AT_ERROR"};
static const char* respOK[] ={"OK","AT_ERROR"};
static const char* respTxDone[] ={"+Finish.","AT_ERROR"};
static const char* respAC[] ={"1000AC","AT_ERROR"};
char* arrayResp[]={"OK","AT_ERROR"};
/*結果コード end*/

struct{
  bool bInitLora = true;
}myWorkarea;
String rfBuf;/* serial1 データ ストリング */
String checkCmd;/*　利用したコマンド ストリング */
char* atCmd;/*　現在コマンド　*/
uint16_t timeLimit = 60000;
bool sendFlg = false;
unsigned long timer = millis();
void setup() {
  // opens serial port, sets data rate to 9600 bp
  Serial.begin(9600);
  Serial1.begin(9600);
  pinMode(DGHARDUINO_PIN_LORA_POWER, OUTPUT);
  pinMode(DGHARDUINO_PIN_LORA_RESET, OUTPUT);
  pinMode(DGHARDUINO_PIN_LORA_WAKEUP, OUTPUT);
  loraPower(true);/*電源ピンをハイに設定する。*/
  loraReset(true);/*リセットピンをハイに設定する。*/
  loraWakeup(true);/*Wake upピンをハイに設定する。*/
}
void loop() {
  if(myWorkarea.bInitLora){
    loraSetup();
  }
 if(!checkResult(serial1Rcv())){/*エラー確認*/
    //return;
 }
}

/*****************************************************************************/
/*  名前: checkResult()                                                       */
/*  内容：この関数は、serial1Rcvからの文字列結果をチェックして何かを行います　　　　　     */
/*  例:                                                                       */
/*            スリープ                                             　　　       */
/*            処理ペイロードデータ                                               */
/*            ERROR確認...      　　                                           */
/*  引数:                                                                      */
/*           result:                                                          */
/*                   "payload:00000001"                                       */
/*                   "ATOK"                                                   */
/*  返し結果: true or false                                                     */
/******************************************************************************/
bool checkResult(String result){
  bool returnResult = true;
  if(result!="WAITTING"){
    if((result=="TIMEOUT")||((result.indexOf("AT_ERROR", 0)>=0)&&result.indexOf("at+cipher", 0)<0)){/*エラー確認*/
      checkCmd.remove(0);
      returnResult = false;
    }
    if(result.indexOf("AT+DADDRP", 0)>=0){/*LoRa P2P チャンネルとデバイスアドレス設定完了確認*/
      myWorkarea.bInitLora = false;
    }
    if(result.indexOf("PAYLOAD:", 0)>=0){/* パイロート受信確認*/
      Serial.println(result);
    }
  }
 return returnResult;
}

/********************************************************************************/
/*  名前: sendAT()                                                          　 　 */
/*  内容：この関数は、コマンドを送信し、引数で設定された制限時間までいくつかの確認応答を待ちます. */
/*  引数:                                                                        */
/*           cmd: "AT\r"                                                         */
/*           timelimit: 5000ms                                                   */
/*           resp[]: {"OK","AT_ERROR"}                                           */
/*********************************************************************************/
void sendAT(char* cmd,uint16_t timeNum,char* resp[]){
   if((sendFlg)&&(checkCmd.indexOf(cmd, 0) < 0)){
    atCmd=cmd;
    checkCmd.concat(cmd);
    Serial1.print(cmd);/*Serial1でAT Commandを送信する。*/
    Serial.println();
    Serial.println(atCmd);
    timeLimit = timeNum;
    for(int i = 0; i < sizeof(resp); i++){
      arrayResp[i]=resp[i];
    }
    sendFlg = false;
    timer = millis();
  }
}

/*****************************************************************************/
/*  名前: serial1Rcv()                                                        */
/*  内容:この関数は、シリアルポートからデータを取得し、結果をチェックして、返します。  　   */
/*  結果例:                                                                   */
/*           payload:00000001                                                */
/*           atCmd + get data(ATOK, AT+JOINAT_ERROR,AT+DADDR=?00:11:22:33OK) */
/*           .....                                                           */
/*****************************************************************************/
String serial1Rcv(void){
  String result = "WAITTING";
  if (Serial.available()){
    int inByte = Serial.read();
    if((inByte >= 0x20) && (inByte <= 0x7f)){
      Serial1.print((char)inByte);
    }
    if((inByte == 0x0D) || (inByte == 0x0A)){
      Serial1.print("\r"); 
    }
    timeLimit=60000;
    timer = millis();
    rfBuf.remove(0);
  }
  if (Serial1.available()){
    int inByte = Serial1.read(); /* Receive binary data as a byte or series of bytes*/
    Serial.write(inByte); /* Writes binary data to the serial port */
    if((inByte >= 0x20) && (inByte <= 0x7f)){
      rfBuf.concat((char)inByte); /* change binary data into string and then put it in rfBuf */
    }
    if((inByte == 0x0D) || (inByte == 0x0A)){
        if(rfBuf.indexOf("RECVP", 0) >= 0){/*受信確認*/
          /* ⇓⇓　+RECVP=SRC:1000,~~SNR:26という文字列の中からPAYLOADの値(00000001)のみを取り出す作業を行う */
          result = rfBuf.substring(rfBuf.indexOf("PAYLOAD:", 0)+8,rfBuf.indexOf(",HOP:", 0));
          Serial.println();
          Serial.print("PAYLOAD:");
          Serial.println(result);
          rfBuf.remove(0);
        }
        if(rfBuf.indexOf("+Rejoin OK.+Finish.", 0) >= 0){/*LoRaWanの設定完了を確認する*/
          result = "Ready.";
          sendFlg = true;
          rfBuf.remove(0);
          delay(10);
        }
        for(int i = 0; i < sizeof(arrayResp); i++){/*結果コード確認*/
          if((rfBuf.indexOf(arrayResp[i], 0) >= 0)&&(rfBuf.indexOf("+Rejoin OK.", 0) < 0)){
              result = atCmd;
              result.concat(rfBuf);
              sendFlg = true;
              rfBuf.remove(0);
            }
          }
    }
  }
  if((!sendFlg)&&(unsigned long) (millis() - timer) > timeLimit){/*タイムアウトチェック*/
    Serial.println("TIMEOUT");
    result = "TIMEOUT";
    Serial.println("\r\r");
    rfBuf.remove(0);
    sendFlg = true;
  }
  return result;
}

/*****************************************************************************/
/*  名前: loraPower()                                                      　 */
/*  内容：電源ピンを設定する。                                                    */
/*****************************************************************************/
void loraPower(bool bOn){
  digitalWrite(DGHARDUINO_PIN_LORA_POWER,bOn);
}

/*****************************************************************************/
/*  名前: loraPower()                                                      　 */
/*  内容：電源ピンを設定する。                                                    */
/*****************************************************************************/
void loraReset(bool bOn){
  digitalWrite(DGHARDUINO_PIN_LORA_RESET,bOn);
}
/******************************************************************************/
/*  名前: loraWakeup()                                                         */
/*  内容：Wake upピンを設定する。                                                 */
/******************************************************************************/
void loraWakeup(bool bOn){
  digitalWrite(DGHARDUINO_PIN_LORA_WAKEUP,bOn);
}
/******************************************************************************/
/*  名前: loraSetup()                                                          */
/*  内容：LoRa P2P チャンネルとデバイスアドレス設定する。                               */
/******************************************************************************/
void loraSetup(){
  sendAT("at+cipher=0\r",5000,respOK);/* cipher key: OFF で設定する */
  char channel[100];
  sprintf(channel, "AT+CHANNELS=P:%u:%u\r", u16Channel, u16DR);
  sendAT(channel,5000,respOK);/* チャンネル: 921.2Mhz DR0（SF12）で設定する */
  char addr[100];
  sprintf(addr, "AT+DADDRP=%s\r", pFromAddr);
  sendAT(addr,5000,respOK);/* デバイスアドレス: 0x1001で設定する */
}
