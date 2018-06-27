#include <Thread.h>
#include <ThreadController.h>

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

enum Brodcast_Type { All = 0, Single = 1,
                     Control = 2, Distress = 3
                   };
enum MessageData { Fire = 0, stopFire = 1,
                   Advance = 2, Reatrat = 3,
                   Ack = 4 , ReqestID = 5 , AssignID = 6,
                   IsConnected = 7, VerifayConnect = 8 , Distres =9
                 };
enum EncodingOffset { SenderAdress = 0, ReciverAdress = 6,
                      BrodcastType = 12,  SenderType = 14,
                      Data = 16
                    };
RF24 radio(9, 10);
const byte addresses[][6] = {"00001", "00002"};
uint8_t unitAdress = 0;

// ThreadController that will controll all threads with multi threhads
ThreadController controll = ThreadController();
Thread radioListenerThread = Thread();
Thread computerListenerThread = Thread();

bool wasActivated = false;

void setup() {
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  Serial.begin(9600);
  radio.begin();
  // Configure Threads
  radioListenerThread.onRun(chackRadioForInput);
 // radioListenerThread.setInterval(50);

  computerListenerThread.onRun(chackComputerForInput);
 // computerListenerThread.setInterval(50);

  // Adds both threads to the controller
  controll.add(&radioListenerThread);
  controll.add(&computerListenerThread);
}
void chackRadioForInput() {
  /// Chack if motoHQ is active and connected to HQ PC
  if (!wasActivated) {
    return;
  }
  /// Reading from pipe
  radio.openReadingPipe(1, addresses[0]);
  radio.startListening();
  if (radio.available())
  {
    digitalWrite(4, HIGH);
    uint32_t text = {0};
    radio.read(&text, sizeof(text));
    sendMsgToComputer(text);
    delay(200);
    digitalWrite(4, LOW);
  }
}
void chackComputerForInput() {
  if (!wasActivated) {
    chackComputerActivation();
    return;
  }
  if (Serial.available()) {

    digitalWrite(2, HIGH);
    uint32_t msg = getMsgFromSerial();;
    sendMsgToRadio(msg);
    delay(200);
    digitalWrite(2, LOW);

  }
}

void chackComputerActivation() {

  if (Serial.available()) {
    uint32_t msg = getMsgFromSerial();

    if (0x7F000 == msg) {
      wasActivated = true;
      digitalWrite(3, HIGH);
      sendMsgToComputer(msg);
      delay(200);
      digitalWrite(3, LOW);
    }
  }
}

uint32_t getMsgFromSerial() {
  byte buffer[sizeof(uint32_t)] = {0};
  uint32_t count = 0, msg = 0;
  while (0 < Serial.available()) buffer[count++] = Serial.read();
  msg = (*(uint32_t*)buffer);
  return msg;
}

/// To Fill Daniel
void sendMsgToComputer(uint32_t msg) {
  Serial.write((byte*)&msg, sizeof(msg));
//  Serial.flush();
}
/// To Fill by michael
void sendMsgToRadio(uint32_t msg) {
  radio.openWritingPipe(addresses[0]);
  radio.stopListening();
  radio.write(&msg, sizeof(msg));
}


/// nothing to do here yet, just calling thred
// handeller to run all thredes
void loop() {
  controll.run();
}

/// Helper functions from moto unit code

uint32_t makeMessage(uint8_t reciver, Brodcast_Type type, MessageData data) {
  uint32_t messege = 0;
  messege = setBits(messege, 0, SenderAdress);
  messege = setBits(messege, reciver, ReciverAdress);
  messege = setBits(messege, type, BrodcastType);
  messege = setBits(messege, 0, SenderType);
  messege = setBits(messege, data, Data);
  return messege;
}

// Used in makeMessage(uint8_t reciver, Brodcast_Type type, MessageData data)
// To set bits indepentely
uint32_t setBits(uint32_t messege, uint8_t adress, int startPoint) {
  int count = 0;
  for (int i = startPoint; i < startPoint + 8; i++) {
    bitWrite(messege, i, bitRead(adress, count++));
  }
  return messege;

}

uint8_t GetAdress(uint32_t messege, int offset) {
  uint8_t add = 0;
  int count = 0;
  for (int i = offset; i < offset + 8; i++) {
    bitWrite(add, count++, bitRead(messege, i));
  }
  return add;
}

MessageData GetData(uint32_t messege) {
  int offset = Data;
  uint8_t add = 0;
  int count = 0;
  for (int i = offset; i < offset + 4; i++) {
    bitWrite(add, count++, bitRead(messege, i));
  }
  return add;
}

Brodcast_Type GetBrodcastType(uint32_t messege) {
  int offset = BrodcastType;
  uint8_t add = 0;
  int count = 0;
  for (int i = offset; i < offset + 4; i++) {
    bitWrite(add, count++, bitRead(messege, i));
  }
  return add;
}

void printMessageBits(uint32_t messege) {
  for (int i = 0; i < 32; i++) {

    Serial.print(bitRead(messege, i));
    if ((i + 1) % 4 == 0) {
      Serial.print(" ");
    }
  }
  Serial.println(" ");


}


