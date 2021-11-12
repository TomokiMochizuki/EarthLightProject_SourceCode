\m#include <avr/wdt.h>

#define DGHARDUINO_PIN_LORA_POWER (6)
#define DGHARDUINO_PIN_LORA_RESET (7)
#define DGHARDUINO_PIN_LORA_WAKEUP (8)
#define DGHARDUINO_PIN_CPU_LORA_UART_RX (9)
#define DGHARDUINO_PIN_CPU_LORA_UART_TX (10)
#define DGHARDUINO_PIN_CPU_LORA_SDA (A4)
#define DGHARDUINO_PIN_CPU_LORA_SCL (A5)


static const uint16_t u16Channel = 9212;/* 921.2Mhz DR0（SF12）で設定する */
static const uint16_t u16DR = 5;/* Date rate = 5 */

/*NODE A　設定*/
//static const char* pFromAddr = "10:00";/* NODE Aのデバイスアドレス 1000.*/
//static const uint16_t u16ToAddr = 1001;/* NODE Bのデバイスアドレス 1001.*/

/*NODE B　設定*/
static const char* pFromAddr = "10:01";
static const uint16_t u16ToAddr = 1000;


/*結果コード begin*/
static const char* respOK[] = {"OK", "AT_ERROR"};
static const char* respTxDone[] = {"+Finish.", "AT_ERROR"};
const char* respAck[] = {"®", "ERROR"};
char* arrayResp[] = {"OK", "AT_ERROR"};
/*結果コード end*/

struct {
  bool bInitLora = true;
} myWorkarea;
String rfBuf;/* serial1 データ ストリング */
String dataBuf;/* serial データ ストリング */
String tempBuf;/* serial テンポラリデータ ストリング */
uint16_t getData = 0;/* serial1 データ バイト*/
String checkCmd;/*　利用したコマンド ストリング */
char* atCmd;/*　現在コマンド　*/
uint16_t sendDataLength = 0;
bool sendFlg = false;
bool setupLora = false;
unsigned long timer = millis();
uint16_t timeLimit = 60000;
unsigned long timerSendData = millis();
bool sendAckFlg = false;
unsigned long timerAck;
unsigned long resetTimer;

void setup() {
  // opens serial port, sets data rate to 9600 bps
  Serial.begin(38400);
  Serial1.begin(9600);
  pinMode(DGHARDUINO_PIN_LORA_POWER, OUTPUT);
  pinMode(DGHARDUINO_PIN_LORA_RESET, OUTPUT);
  loraPower(true);/*電源ピンをハイに設定する。*/
  loraReset(true);/*リセットピンをハイに設定する。*/
  pinMode(2, OUTPUT);
  wdt_enable(WDTO_8S);
}
void loop() {
  if (myWorkarea.bInitLora) {
    loraSetup();
  } else {
    sendData();
    digitalWrite(2, LOW);
  }
  wdt_reset();
  if (!checkResult(serial1Rcv())) { /*エラー確認*/
    return;
  }
  wdt_reset();
  if (millis() - resetTimer >= 60000) {
    software_reset();
    resetTimer = millis();
  }
}


void software_reset() {
  wdt_disable();
  wdt_enable(WDTO_15MS);
  while (1) {}
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
bool checkResult(String result) {
  bool returnResult = true;
  if (result != "WAITTING") {
    //Serial.println(result);
    if ((result == "TIMEOUT") || ((result.indexOf("ERROR", 0) >= 0) && result.indexOf("at+cipher", 0) < 0)) { /*エラー確認*/
      checkCmd.remove(0);
      returnResult = false;
    }
    if (result.indexOf("AT+DADDRP", 0) >= 0) { /*LoRa P2P チャンネルとデバイスアドレス設定完了確認*/
      myWorkarea.bInitLora = false;
      Serial.println();
    }
    if (result.indexOf("txDone", 0) >= 0) { /*通信完了確認*/
      checkCmd.remove(0);
    }
    if (result.indexOf((char)169, 0) >= 0) { /*通信完了確認*/
      checkCmd.remove(0);
      dataBuf.remove(0, sendDataLength);
    }
  }
  return returnResult;
}

/********************************************************************************/
/*  名前: sendData()                                                     　　    */
/*  内容：この関数は、serial受信よりデータを送ります。1s以内に受信しないと通信します、または */
/*  　　　ペイロードサイズをバッファデータ量(50 Byte)が超過したら、即時送信する。          */
/********************************************************************************/
void sendData() {
  if (Serial.available() > 0) {
    digitalWrite(2, HIGH);
    int inByte = Serial.read();
    Serial.write(inByte);
    if ((inByte >= 0x20) && (inByte <= 0x7f)) {
      dataBuf.concat((char)inByte);
    }
    if ((inByte == 0x0D) || (inByte == 0x0A)) {
      Serial.println();
      dataBuf.concat((char)175);
    }
    timerSendData = millis();
  }

  String termString;
  char buf[200];
  char payLoad[200] = {};
  if (dataBuf.length() >= 400) {
    Serial.println("\n\r");
    Serial.println("BUFFER_OVER_FLOW");
    Serial.println("buffer over flow has been deleted.");
    Serial.print("[[ ");
    Serial.print(dataBuf);
    Serial.println(" ]]");
    Serial.print("\n\r");
    dataBuf.remove(sendDataLength, dataBuf.length());
  }

  if ((unsigned long) (millis() - timer) > 1000) {
    if (sendFlg && dataBuf.indexOf("DONE") >= 0) {
      //while (dataBuf.indexOf("DONE")<=20){
      //  dataBuf.remove(0,dataBuf.indexOf("DONE")+3);
      //}
      termString = dataBuf.substring(dataBuf.indexOf("Time"), dataBuf.indexOf("DONE"));
      termString.toCharArray(payLoad, 200);
      sendDataLength = termString.length();
      sprintf(buf, "AT+SENDP=%u:%s\r", u16ToAddr, payLoad);
      Serial.println(dataBuf);
      //Serial.println("----------");
      Serial.println(payLoad);
      //Serial.println(dataBuf.indexOf("Time"));
      //Serial.println(dataBuf.indexOf("DONE"));
      sendAT(buf, 2000, respAck);
      dataBuf.remove(0);
    }
    if (sendFlg && dataBuf.length() >= 50) {
      termString = dataBuf.substring(0, 50);
      termString.toCharArray(payLoad, 200);
      sendDataLength = termString.length();
      sprintf(buf, "AT+SENDP=%u:%s\r", u16ToAddr, payLoad);
      Serial.println(dataBuf);
      //Serial.println(payLoad);
      Serial.println(dataBuf.indexOf("Time"));
      //Serial.println(dataBuf.indexOf("DONE"));
      //sendAT(buf,3000,respAck);
    }
    if (sendFlg && dataBuf.length() > 0 && (unsigned long) (millis() - timerSendData) > 1000) {
      dataBuf.toCharArray(payLoad, 200);
      sendDataLength = dataBuf.length();
      sprintf(buf, "AT+SENDP=%u:%s\r", u16ToAddr, payLoad);
      //sendAT(buf,4000,respAck);
    }
  }
}

/*********************************************************************************/
/*  名前: sendAT()                                                          　 　  */
/*  内容：この関数は、コマンドを送信し、引数で設定された制限時間までいくつかの確認応答を待ちます.*/
/*  引数:                                                                         */
/*           cmd: "AT\r"                                                         */
/*           timelimit: 5000ms                                                   */
/*           resp[]: {"OK","AT_ERROR"}                                           */
/*********************************************************************************/
void sendAT(char* cmd, uint16_t timeNum, char* resp[]) {
  if ((sendFlg) && (checkCmd.indexOf(cmd, 0) < 0)) {
    atCmd = cmd;
    checkCmd.concat(cmd);
    Serial1.print(cmd);/*Serial1でAT Commandを送信する。*/
    if (myWorkarea.bInitLora) {
      Serial.println();
      Serial.println(cmd);
    }
    timeLimit = timeNum;
    for (int i = 0; i < sizeof(resp); i++) {
      arrayResp[i] = resp[i];
    }
    sendFlg = false;
    timer = millis();
  }
}
/******************************************************************************/
/*  名前: convertHex2Dec()                                                     */
/*  内容：この関数は、hexadecimalからdecimal                                      */
/*       　表示します。　                                                        */
/******************************************************************************/
int convertHex2Dec(int data) {
  if (data < 65) {
    data -= 48;
  } else {
    data -= 55;
  }
  return data;
}
/*****************************************************************************/
/*  名前: serial1Rcv()                                                        */
/*  内容:この関数は、シリアルポートからデータを取得し、結果をチェックして、返します。 　     */
/*        または受信するときにデータを表示します。                                   */
/*  結果例:                                                                    */
/*           payload:00000001                                                 */
/*           atCmd + get data(ATOK, AT+JOINAT_ERROR,AT+DADDR=?00:11:22:33OK)  */
/*           .....                                                            */
/*****************************************************************************/
String serial1Rcv(void) {
  String result = "WAITTING";
  if (Serial1.available()) {
    int inByte = Serial1.read();
    if (myWorkarea.bInitLora) {
      Serial.write(inByte);
    }
    if ((inByte >= 0x20) && (inByte <= 0x7f)) {
      rfBuf.concat((char)inByte);
    }
    if ((rfBuf.indexOf("PAYLOAD:", 0) >= 0) && (rfBuf.indexOf(",HOP:", 0) < 0)) {
      if ((0x30 <= inByte && inByte <= 0x39) || (0x41 <= inByte && inByte <= 0x46)) {
        if (getData < 16) {
          getData = 16 * convertHex2Dec(inByte);
        } else {
          getData = getData + convertHex2Dec(inByte);
          if (getData != 169) {
            if (getData == 175) {
              Serial.println();
            } else {
              Serial.print((char)getData);
            }
          } else {
            rfBuf.concat((char)getData);
          }
          getData = 0;
        }
      }
    }
    if ((inByte == 0x0D) || (inByte == 0x0A)) {
      if (rfBuf.indexOf("RECVP", 0) >= 0) { /*受信確認*/
        if (rfBuf.indexOf((char)169, 0) < 0) {
          sendAckFlg = true;
          timerAck = millis();
        } else {
          result = atCmd;
          result.concat(rfBuf);
          rfBuf.remove(0);
          sendFlg = true;
          timer = millis();
        }
      }
      if (rfBuf.indexOf("+Rejoin OK.+Finish.", 0) >= 0) { /*LoRaWanの設定完了を確認する*/
        result = "Ready.";
        sendFlg = true;
        rfBuf.remove(0);
        delay(100);
      }
      for (int i = 0; i < sizeof(arrayResp); i++) { /*結果コード確認*/
        if ((rfBuf.indexOf(arrayResp[i], 0) >= 0) && (rfBuf.indexOf("+Rejoin OK.", 0) < 0)) {
          result = atCmd;
          result.concat(rfBuf);
          sendFlg = true;
          rfBuf.remove(0);
        }
      }
    }
  }
  if ((!sendFlg) && (unsigned long) (millis() - timer) > timeLimit) { /*タイムアウトチェック*/
    result = "TIMEOUT";
    rfBuf.remove(0);
    sendFlg = true;
  }
  if ((sendAckFlg) && (unsigned long) (millis() - timerAck) > 1000) {
    if (!sendFlg) {
      sendFlg = true;
    }
    char ack[100] = {(char)169};
    char buf[100];
    int ackAddr = rfBuf.substring(rfBuf.indexOf("SRC:", 0) + 4, rfBuf.indexOf(",DST:", 0)).toInt();
    sprintf(buf, "AT+SENDP=%u:%s\r", ackAddr, ack);
    sendAT(buf, 10000, respTxDone);
    rfBuf.remove(0);
    sendAckFlg = false;
  }
  return result;
}
/*****************************************************************************/
/*  名前: loraPower()                                                      　 */
/*  内容：電源ピンを設定する。                                                    */
/*****************************************************************************/
void loraPower(bool bOn) {
  digitalWrite(DGHARDUINO_PIN_LORA_POWER, bOn);
}
/*****************************************************************************/
/*  名前: loraPower()                                                      　 */
/*  内容：電源ピンを設定する。                                                    */
/*****************************************************************************/
void loraReset(bool bOn) {
  digitalWrite(DGHARDUINO_PIN_LORA_RESET, bOn);
}
/******************************************************************************/
/*  名前: loraSetup()                                                          */
/*  内容：LoRa P2P チャンネルとデバイスアドレス設定する。                               */
/******************************************************************************/
void loraSetup() {
  sendAT("at+cipher=0\r", 5000, respOK); /* cipher key: OFF で設定する */
  char channel[100];
  sprintf(channel, "AT+CHANNELS=P:%u:%u\r", u16Channel, u16DR);
  sendAT(channel, 5000, respOK); /* チャンネル: 921.2Mhz DR0（SF12）で設定する */
  char addr[100];
  sprintf(addr, "AT+DADDRP=%s\r", pFromAddr);
  sendAT(addr, 5000, respOK); /* デバイスアドレス: 0x1000で設定する */
}
