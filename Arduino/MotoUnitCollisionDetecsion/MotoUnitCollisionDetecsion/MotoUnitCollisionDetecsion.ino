#include <Thread.h>
#include <ThreadController.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

/// Public enums used for all units and HQ
enum Brodcast_Type { All = 0, Single = 1,
                     Control = 2, Distress = 3   };
enum Sender_Type { Soldier = 0, Commander = 1,
                   HQ = 2, Extrnal = 3   };
enum MessageData { Fire = 0, stopFire = 1,
                   Advance = 2, Reatrat = 3,
                   Ack = 4 , ReqestID = 5 , AssignID = 6,
                   IsConnected = 7, VerifayConnect = 8, Distres = 9 };
enum EncodingOffset { SenderAdress = 0, ReciverAdress = 6,
                      BrodcastType = 12,  SenderType = 14,
                      Data = 16, ClusterId = 22,  EndMsg = 26};
enum EncodingSize { AdressSize = 6 , BrodcastTypeSize = 2,
                    SenderTypeSize = 2, DataSize = 6, ClusterIdSize = 4};
                    


/// RF Radio public variables
RF24 radio(9, 10);
const byte addresses[][6] = {"00001", "00002"};

/// CSMA/CA variables              
// Collision Avoidence send tries made befor aborting
const int caMaxResend = 100;
int caResendLeft = 0;
// Maximum number of carier scence made before aborting send
const int csMaxResends = 20;
int csResendsLeft = 0;
/// Minimum and maximum for random backoff delay if channel is not clear or sending failed (Ack not recived)
const int BeckoffRandomMax = 100;
const int BeckoffRandomMin = 10;
/// Global variables to syncronize internal comunication between sending and reciving thereds
uint32_t globalMessege = 0;
bool outMessageAvilable = false;

/// Commander OLED screen variables
SoftwareSerial LCD(0, 8);

/// Leds public variables
#define BlinkNumber 1
/// LED connection Pins
int ledG = 3;
int ledB = 2;
int ledR = 4;
/// LED global comunication variables
int redBlinkNumber = 0;
int blueBlinkNumber = 0;
int greenBlinkNumber = 0;
int purpleBlinkNumber = 0;

/// buttons public variables
/// Minimum time for buttons to be pressed again
const int buttonCooldownTimer  = 5000;
/// Button connection Pins
const int button1Pin = 7;
const int button2Pin = 6;
const int button3Pin = 5;
/// buttons global comunication variables
int button1State = 0;
int button1Timer = 0;
bool wasButton1Pressed = false;
int button2State = 0;
int button2Timer = 0;
bool wasButton2Pressed = false;
int button3State = 0;
int button3Timer = 0;
bool wasButton3Pressed = false;

// ThreadController that will controll all threads with multi threhads
ThreadController controll = ThreadController();
Thread inputButtonThread = Thread();
Thread outputButtonThread = Thread();
Thread rf24Sender = Thread();

/// Moto Unit Variables
bool isMotoComander = true;
bool isTestMode = false;
int counter = 0;
int mode = 0;

bool wasActivated = false;
bool wasRequestIdSent = false;
uint8_t motounitAdress = 0;
uint8_t motounitClusterID = 0;
Sender_Type unitType = Soldier;

void setup() {

  randomSeed(analogRead(0));
  Serial.begin(9600);

  radio.begin();
  radio.setPALevel(RF24_PA_MAX);
  radio.setAutoAck (true ) ;
  ReturnToListen();

  Serial.print("is Chip Connected = " );
  Serial.println(radio.isChipConnected());

  pinMode(ledG, OUTPUT);
  pinMode(ledR, OUTPUT);
  pinMode(ledB, OUTPUT);
  pinMode(button1Pin, INPUT);
  pinMode(button2Pin, INPUT);
  pinMode(button3Pin, INPUT);
  
  // digitalWrite(ledR, HIGH);

  rf24Sender.onRun(SendRfWhenPossible);
  rf24Sender.setInterval(0);

  // Configure Threads
  inputButtonThread.onRun(chackInputButtons);
  inputButtonThread.setInterval(0);

  outputButtonThread.onRun(blinkBlueLed);
  outputButtonThread.setInterval(0);

  // Adds both threads to the controller
  controll.add(&rf24Sender);
  controll.add(&inputButtonThread);
  controll.add(&outputButtonThread); // & to pass the pointer to it

  // Setup for moto Commander
  if (unitType == Commander) {
    initializeLCD();
    writeToDisplay("Waiting For ID", "Press button 1");
  }
}



void loop() {
  controll.run();

  if (!wasActivated) {
    sendActivationBeacon();
    return;
  }
  decraseButtonTimers();
  /// Writing to pipe
  if (wasButton1Pressed && button1Timer <= 0) {
    uint32_t messege = 0;
    if (unitType == Commander) {
      messege = makeMessage(0, All, Fire) ;
    } else {
      messege = makeMessage(0, All, Distres) ;
    }
    SendMsg(messege);
    wasButton1Pressed =  false;
  }
  else if (wasButton2Pressed && button2Timer <= 0) {
    uint32_t messege = 0;
    if (unitType == Commander) {
      messege = makeMessage(0, All, stopFire) ;
    } else {
      messege = makeMessage(0, All, Distres) ;
    }
    SendMsg(messege);
    wasButton2Pressed =  false;
  }
  else if (wasButton3Pressed && button3Timer <= 0) {
    uint32_t messege = 0;
    if (unitType == Commander) {
      messege = makeMessage(0, All, Advance) ;
    } else {
      messege = makeMessage(0, All, Distres) ;
    }
    SendMsg(messege);
    wasButton3Pressed =  false;
  }

/// Chack if there is a message for the unit
  if (radio.available())
  {
    Serial.println("--------------------------------------");
    uint32_t text = {0};
    radio.read(&text, sizeof(text));
    radio.writeAckPayload(0, text, sizeof(text));
    handleMessage(text);
  //  Serial.println(text);
  }
}

void SendRfWhenPossible() {
  if (outMessageAvilable) {
    writeToRadio(globalMessege);
  }
}

void SendMsg(uint32_t msg) {
  caResendLeft = caMaxResend;
  globalMessege = msg;
  outMessageAvilable = true;
}


void writeToRadio(uint32_t messege) {
  Serial.println("--------------------------------------");
  Serial.println("Sending Message");
  csResendsLeft = csMaxResends;
  long randBackoff = random(BeckoffRandomMin, BeckoffRandomMax);
  while (!radio.testCarrier() && radio.testRPD () && csResendsLeft <= 0) {
    delay(randBackoff);
    Serial.println("RPD Ocupied");
    csResendsLeft--;
  }
  if (csResendsLeft > 0) {
    Serial.println("Channel free for Sending");
  }
  else {
    Serial.println("Exited via Max Timer : Sending failed");
    //delay(randBackoff);
    outMessageAvilable = true;
    return;
  }
  radio.openWritingPipe(addresses[0]);
  radio.stopListening();
  bool writeRslt = radio.write(&messege, sizeof(messege));

  if (!writeRslt) {
    if (caResendLeft > 0) {
      Serial.print("Ack Not Recived -> resending for time : ");
      Serial.println(caResendLeft);
      Serial.print("Waiting For : ");
      Serial.println(randBackoff);
      delay(randBackoff);
      outMessageAvilable = true;
      caResendLeft--;
    } else {
      outMessageAvilable = false;
      Serial.println("Ack Not Recived 20 times Msg Failed");
      ReturnToListen();
    }

  } else {
    outMessageAvilable = false;
    Serial.println("Ack Recived");
    ReturnToListen();
    Serial.println(messege);
  }
}

void ReturnToListen() {
  radio.openReadingPipe(0, addresses[0]);
  radio.startListening();
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
  else if (blueBlinkNumber >= 1) {
    digitalWrite(ledB, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(200);                 // wait for a second
    digitalWrite(ledB, LOW);    // turn the LED off by making the voltage LOW
    delay(200);
    blueBlinkNumber--;
  }
  else if (greenBlinkNumber >= 1) {
    digitalWrite(ledG, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(200);                 // wait for a second
    digitalWrite(ledG, LOW);    // turn the LED off by making the voltage LOW
    delay(200);
    greenBlinkNumber--;
  }
  else if (purpleBlinkNumber >= 1) {
    digitalWrite(ledR, HIGH);
    digitalWrite(ledG, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(200);                 // wait for a second
    digitalWrite(ledR, LOW);    // turn the LED off by making the voltage LOW
    digitalWrite(ledG, LOW);
    delay(200);
    purpleBlinkNumber--;
  }


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

void clearLCD() {
  LCD.write(0xFE);
  LCD.write(0x01);
}

void initializeLCD() {
  LCD.begin(9600);
  clearLCD();
}

void writeToDisplay(const char *row1, const char *row2) {
  clearLCD();
  LCD.write(row1);
  LCD.write(254);
  LCD.write(192);
  LCD.write(row2);
}

void decraseButtonTimers() {
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

void sendActivationBeacon() {

  /// Reset Button Timers if needed
  decraseButtonTimers();
  /// Writing to pipe
  if (wasButton1Pressed && button1Timer <= 0) {
    uint32_t messege  = makeMessage(0, Control, ReqestID) ;
    writeToRadio(messege);

    wasButton1Pressed =  false;
    wasRequestIdSent = true;
    // delay(50);
  }

  if (!wasRequestIdSent) {
    return;
  }
  /// Reading from pipe
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
    Serial.print("motounitAdress = ");
    Serial.println(motounitAdress);
    motounitClusterID = GetClusterId(text);
    digitalWrite(ledR, LOW);
    if (unitType == Commander) {
      char unitAdressString[8];
      writeToDisplay("Connected with ID:", itoa(motounitAdress, unitAdressString, 10));
    }

  } else {
    Serial.println("Not Connected");
  }

}


void chackInputButtons() {
  /// will only send messages when buttons are pressed
  button1State = digitalRead(button1Pin);
  if (button1State == HIGH) {
    if (!wasButton1Pressed) {
      wasButton1Pressed = true;
      button1Timer = buttonCooldownTimer;
    }
  }
  button2State = digitalRead(button2Pin);
  if (button2State == HIGH ) {
    if (!wasButton2Pressed) {
      wasButton2Pressed = true;
      button2Timer = buttonCooldownTimer;
    }
  }
  button3State = digitalRead(button3Pin);
  if (button3State == HIGH ) {
    if (!wasButton3Pressed) {
      wasButton3Pressed = true;
      button3Timer = buttonCooldownTimer;
    }
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

  Brodcast_Type brodcastType = GetBrodcastType( text);
  Serial.print("Bordcast Type = " );
  Serial.println(brodcastType);

  Sender_Type senderType = GetSenderType( text);
  Serial.print("Sender Type = " );
  Serial.println(senderType);

  MessageData data = GetData( text);
  Serial.print("data = " );
  Serial.println(data);

  uint8_t clusterId = GetClusterId(text);
  Serial.print("Cluster ID = " );
  Serial.println(clusterId);

  if (clusterId == motounitClusterID) {
    //    Serial.println("Same Cluster ID" );
    if (unitType == Commander) {
      //  Serial.println("Recived becouse i am commander" );
      outputMessageData(data);
      return;
    }
    if (senderType == Commander) {
      // Serial.println("Recived and i got if from commander" );
      outputMessageData(data);
      return;
    }
    if (senderType == HQ) {
      Serial.println("Recived and i got if from HQ" );
      if (brodcastType == All || brodcastType == Distress) {
        Serial.println("brodcastType == All || brodcastType == Distress" );
        outputMessageData(data);
        return;
      }
      Serial.print("brodcastType = " );
      Serial.println(brodcastType);
      Serial.print("motoUnit Adress = ");
      Serial.println( motounitAdress);
      Serial.print("reciver Adress = " );
      Serial.println( recAdress);
      if (brodcastType == Single && motounitAdress == recAdress) {
        Serial.println("brodcastType == Single && motoUnitAdress == recAdress" );
        outputMessageData(data);
        return;
      }
    }

  }



}

/// Blink Leds if message was recived in a diffrent thread
void outputMessageData(MessageData data) {
  switch (data) {
    case Fire: {
        if (unitType == Commander) {
          writeToDisplay("Message Recived:", "Fire");
        }
        blueBlinkNumber += BlinkNumber;
        break;
      }

    case stopFire: {
        if (unitType == Commander) {
          writeToDisplay("Message Recived:", "Stop Fire");
        }
        greenBlinkNumber += BlinkNumber;
        break;
      }
    case Advance: {
        if (unitType == Commander) {
          writeToDisplay("Message Recived:", "Advance");
        }
        redBlinkNumber += BlinkNumber;
        break;
      }
    case Reatrat: {
        if (unitType == Commander) {
          writeToDisplay("Message Recived:", "Reatrat");
        }
        purpleBlinkNumber += BlinkNumber;
        break;
      }
    case Ack: {
        if (unitType == Commander) {
          writeToDisplay("Message Recived:", "Ack");
        }
        break;
      }
    case ReqestID: {
        if (unitType == Commander) {
          writeToDisplay("Message Recived:", "Request ID");
        }
        break;
      }
    case AssignID: {
        if (unitType == Commander) {
          writeToDisplay("Message Recived:", "Assign ID");
        }
        break;
      }
    case Distres: {
        if (unitType == Commander) {
          writeToDisplay("Message Recived:", "Distress");
          redBlinkNumber += BlinkNumber;
        }
        break;
      }

    default: {
        if (unitType == Commander) {
          writeToDisplay("Message Recived:", "Other Message");
        }
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
  messege = setBits(messege, data, Data, ClusterId);
  messege = setBits(messege, motounitClusterID, ClusterId, EndMsg);
  return messege;
}



