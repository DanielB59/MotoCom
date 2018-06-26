#include <Thread.h>
#include <ThreadController.h>

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define BlinkNumber 3

enum Brodcast_Type { All = 0, Single = 1,
                     Control = 2, Distress = 3
                   };
enum Sender_Type { Soldier = 0, Commander = 1,
                   HQ = 2, Extrnal = 3
                 };
enum MessageData { Fire = 0, stopFire = 1,
                   Advance = 2, Reatrat = 3,
                   Ack = 4 , ReqestID = 5 , AssignID = 6,
                   IsConnected = 7, VerifayConnect = 8
                 };
enum EncodingOffset { SenderAdress = 0, ReciverAdress = 6,
                      BrodcastType = 12,  SenderType = 14,
                      Data = 16, ClusterId = 22,  EndMsg = 26
                    };
enum EncodingSize { AdressSize = 6 , BrodcastTypeSize = 2,
                    SenderTypeSize = 2, DataSize = 6, ClusterIdSize = 4
                  };

/// RF Radio public variables
RF24 radio(9, 10);
const byte addresses[][6] = {"00001", "00002"};
uint8_t motoUnitAdress = 0;

/// Leds public variables
int ledG = 3;
int ledB = 2;
int ledR = 4;
bool isGreenActive = false;
int redBlinkNumber = 0;
int blueBlinkNumber = 0;
int greenBlinkNumber = 0;

/// buttons public variables
const int button1Pin = 7;
int button1State = 0;
int button1Timer = 0;
bool wasButton1Pressed = false;
const int button2Pin = 6;
int button2State = 0;
int button2Timer = 0;
bool wasButton2Pressed = false;
const int button3Pin = 5;
int button3State = 0;
int button3Timer = 0;
bool wasButton3Pressed = false;

// ThreadController that will controll all threads with multi threhads
ThreadController controll = ThreadController();
Thread inputButtonThread = Thread();
Thread outputButtonThread = Thread();

/// Moto Unit Variables
bool isMotoComander = true;
bool isTestMode = false;
int counter = 0;
int mode = 0;

bool wasActivated = false;
bool wasRequestIdSent = false;
uint8_t motounitAdress = 0;
uint8_t motounitClusterID = 0;
Sender_Type unitType = HQ;

void setup() {
  Serial.begin(9600);
  radio.begin();

  Serial.print("is Chip Connected = " );
  Serial.println(radio.isChipConnected());

  uint32_t  messege = makeMessage(16, Single, IsConnected) ;
  handleMessage(messege);

  pinMode(ledG, OUTPUT);
  pinMode(ledR, OUTPUT);
  pinMode(ledB, OUTPUT);
  pinMode(button1Pin, INPUT);
  pinMode(button2Pin, INPUT);
  pinMode(button3Pin, INPUT);

  
  digitalWrite(ledR, HIGH);  

  // Configure Threads
  inputButtonThread.onRun(chackInputButtons);
  inputButtonThread.setInterval(50);

  outputButtonThread.onRun(blinkBlueLed);
  outputButtonThread.setInterval(400);

  // Adds both threads to the controller
  controll.add(&inputButtonThread);
  controll.add(&outputButtonThread); // & to pass the pointer to it
}

void writeToRadio(uint32_t messege) {
  radio.openWritingPipe(addresses[0]);
  radio.stopListening();
  radio.write(&messege, sizeof(messege));
  Serial.println(messege);
}

void loop() {

  controll.run();

  if (!wasActivated) {
    sendActivationBeacon();
    return;
  }

  /// Writing to pipe
  if (wasButton1Pressed && button1Timer <= 0) {
    uint32_t messege = makeMessage(0, All, Fire) ;
    writeToRadio(messege);
    wasButton1Pressed =  false;
    //  delay(50);
    // Serial.print("Reques ID Sent with : ");

  } else if (wasButton2Pressed && button2Timer <= 0) {
    uint32_t messege = makeMessage(0, All, stopFire) ;
    writeToRadio(messege);
    wasButton2Pressed =  false;
    // delay(50);
  }
  else if (wasButton3Pressed && button3Timer <= 0) {
    uint32_t messege  = makeMessage(0, All, Advance) ;
    writeToRadio(messege);
    wasButton3Pressed =  false;
    // delay(50);
  }

  /// Reading from pipe
  radio.openReadingPipe(1, addresses[0]);
  radio.startListening();
  if (radio.available())
  {
    uint32_t text = {0};
    radio.read(&text, sizeof(text));
    handleMessage(text);
    Serial.println(text);
  }
}

void sendActivationBeacon() {

  /// Writing to pipe
  if (wasButton1Pressed && button1Timer <= 0) {
    uint32_t messege  = makeMessage(0, Control, ReqestID) ;
    writeToRadio(messege);
    wasButton1Pressed =  false;
    // delay(50);
  }

  if (!wasRequestIdSent) {
    return;
  }
  /// Reading from pipe
  radio.openReadingPipe(1, addresses[0]);
  radio.startListening();
  if (radio.available())
  {
    handleHandShkae();
  }
}

void handleHandShkae() {
  uint32_t text = {0};
  radio.read(&text, sizeof(text));
  MessageData data = GetData( text);
  handleMessage(text);
  if (data == AssignID) {
    wasActivated = true;
    Serial.println("Connected");
    motounitAdress = GetReciverAdress(text);
    motounitClusterID = GetClusterId(text);
       digitalWrite(ledR, LOW);  
  }else{
    Serial.println("Not Connected");
    }

}

int startTimer  = 1000;
// callback for inputButtonThread
void chackInputButtons() {

  /// Normal Mode
  /// will only send messages when buttons are pressed
  button1State = digitalRead(button1Pin);
  if (button1State == HIGH) {
    if (!wasButton1Pressed) {
      wasButton1Pressed = true;
      button1Timer = startTimer;
    }
  }
  button2State = digitalRead(button2Pin);
  if (button2State == HIGH ) {
    if (!wasButton2Pressed) {
      wasButton2Pressed = true;
      button2Timer = startTimer;
    }
  }
  button3State = digitalRead(button3Pin);
  if (button3State == HIGH ) {
    if (!wasButton3Pressed) {
      wasButton3Pressed = true;
      button3Timer = startTimer;
    }
  }

  /// Reset Button Timers if needed
  if (button1Timer > 0) {
    button1Timer--;
  }
  if (button2Timer > 0) {
    button2Timer--;
  }
  if (button3Timer > 0) {
    button3Timer--;
  }

}

// callback for hisThread
void blinkBlueLed() {
  /// Blink Leds if message was recived in a diffrent thread
  if (redBlinkNumber >= 1) {
    digitalWrite(ledR, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(200);                 // wait for a second
    digitalWrite(ledR, LOW);    // turn the LED off by making the voltage LOW
    delay(200);
    redBlinkNumber--;
  }
  if (blueBlinkNumber >= 1) {
    digitalWrite(ledB, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(200);                 // wait for a second
    digitalWrite(ledB, LOW);    // turn the LED off by making the voltage LOW
    delay(200);
    blueBlinkNumber--;
  }
  if (greenBlinkNumber >= 1) {
    digitalWrite(ledG, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(200);                 // wait for a second
    digitalWrite(ledG, LOW);    // turn the LED off by making the voltage LOW
    delay(200);
    greenBlinkNumber--;
  }


}

void handleMessage(uint32_t text) {
  printMessageBits(text);

  uint8_t sendAdress = GetSenderAdress(text);
  Serial.print("Sender Adress = " );
  Serial.println(sendAdress);

  uint8_t recAdress = GetReciverAdress(text);
  Serial.print("Reciver Adress = " );
  Serial.println(recAdress);

  Brodcast_Type type = GetBrodcastType( text);
  Serial.print("Bordcast Type = " );
  Serial.println(type);

  Sender_Type senderType = GetSenderType( text);
  Serial.print("Sender Type = " );
  Serial.println(senderType);

  MessageData data = GetData( text);
  Serial.print("data = " );
  Serial.println(data);

  uint8_t clusterId = GetClusterId(text);
  Serial.print("Cluster ID = " );
  Serial.println(clusterId);

  if (type == All) {
    outputMessageData(data);
  }
  else if (type == Single && motoUnitAdress == recAdress) {
    outputMessageData(data);

  }

}

/// Blink Leds if message was recived in a diffrent thread
void outputMessageData(MessageData data) {
  switch (data) {
    case Fire: {
       // redBlinkNumber = BlinkNumber;
         blueBlinkNumber = BlinkNumber;
        break;
      }

    case stopFire: {
        blueBlinkNumber = BlinkNumber;
        break;
      }
    case Ack: {
        greenBlinkNumber = BlinkNumber;
        break;
      }
    default: {
        greenBlinkNumber = BlinkNumber;
        break;
      }

  }

}

uint32_t makeMessage(uint8_t reciver, Brodcast_Type type, MessageData data) {
  uint32_t messege = 0;
  messege = setBits(messege, motounitAdress, SenderAdress, ReciverAdress);
  messege = setBits(messege, reciver, ReciverAdress, BrodcastType);
  messege = setBits(messege, type, BrodcastType, SenderType);
  messege = setBits(messege, unitType, SenderType, Data);
  messege = setBits(messege, data, Data, EndMsg);
  return messege;
}


// To set bits indepentely
uint32_t setBits(uint32_t messege, uint8_t adress, int startPoint, int endPoint) {
  int count = 0;
  for (int i = startPoint; i < endPoint; i++) {
    bitWrite(messege, i, bitRead(adress, count++));
  }
  return messege;

}

uint8_t GetSenderAdress(uint32_t messege) {
  return GetAdress(messege, SenderAdress);
}

uint8_t GetReciverAdress(uint32_t messege) {
  return GetAdress(messege, ReciverAdress);
}

uint8_t GetAdress(uint32_t messege, int offset) {
  uint8_t add = 0;
  int count = 0;
  for (int i = offset; i < offset + AdressSize; i++) {
    bitWrite(add, count++, bitRead(messege, i));
  }
  return add;
}

uint8_t GetClusterId(uint32_t messege) {
  int offset = ClusterId;
  uint8_t add = 0;
  int count = 0;
  for (int i = offset; i < offset + ClusterIdSize; i++) {
    bitWrite(add, count++, bitRead(messege, i));
  }
  return add;
}

MessageData GetData(uint32_t messege) {
  int offset = Data;
  uint8_t add = 0;
  int count = 0;
  for (int i = offset; i < offset + DataSize; i++) {
    bitWrite(add, count++, bitRead(messege, i));
  }
  return add;
}

Brodcast_Type GetBrodcastType(uint32_t messege) {
  int offset = BrodcastType;
  uint8_t add = 0;
  int count = 0;
  for (int i = offset; i < offset + BrodcastTypeSize; i++) {
    bitWrite(add, count++, bitRead(messege, i));
  }
  return add;
}

Sender_Type GetSenderType(uint32_t messege) {
  int offset = SenderType;
  uint8_t add = 0;
  int count = 0;
  for (int i = offset; i < offset + SenderTypeSize; i++) {
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
