#include <Thread.h>
#include <ThreadController.h>
#include <SoftwareSerial.h>


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
                   IsConnected = 7, VerifayConnect = 8, Distres = 9
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
SoftwareSerial LCD(0, 8);
const byte addresses[][6] = {"00001", "00002"};


/// Leds public variables
int ledG = 3;
int ledB = 2;
int ledR = 4;
bool isGreenActive = false;
int redBlinkNumber = 0;
int blueBlinkNumber = 0;
int greenBlinkNumber = 0;
int purpleBlinkNumber = 0;

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
Sender_Type unitType = Commander;

void setup() {


  Serial.begin(9600);

  radio.begin();

  Serial.print("is Chip Connected = " );
  Serial.println(radio.isChipConnected());

  //uint32_t  messege = makeMessage(16, Single, IsConnected) ;
  //handleMessage(messege);

  pinMode(ledG, OUTPUT);
  pinMode(ledR, OUTPUT);
  pinMode(ledB, OUTPUT);
  pinMode(button1Pin, INPUT);
  pinMode(button2Pin, INPUT);
  pinMode(button3Pin, INPUT);


  digitalWrite(ledR, HIGH);


  // Configure Threads
  inputButtonThread.onRun(chackInputButtons);
  inputButtonThread.setInterval(0);

  outputButtonThread.onRun(blinkBlueLed);
  outputButtonThread.setInterval(0);

  // Adds both threads to the controller
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
    writeToRadio(messege);
    wasButton1Pressed =  false;
  }
  else if (wasButton2Pressed && button2Timer <= 0) {
    uint32_t messege = 0;
    if (unitType == Commander) {
      messege = makeMessage(0, All, stopFire) ;
    } else {
      messege = makeMessage(0, All, Distres) ;
    }
    writeToRadio(messege);
    wasButton2Pressed =  false;
  }
  else if (wasButton3Pressed && button3Timer <= 0) {
    uint32_t messege = 0;
    if (unitType == Commander) {
      messege = makeMessage(0, All, Advance) ;
    } else {
      messege = makeMessage(0, All, Distres) ;
    }
    writeToRadio(messege);
    wasButton3Pressed =  false;
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

int startTimer  = 5000;
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
      Serial.println("Same Cluster ID" );
    if (unitType == Commander) {
      Serial.println("Recived becouse i am commander" );
      outputMessageData(data);
      return;
    }
    if (senderType == Commander) {
      Serial.println("Recived and i got if from commander" );
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
        blueBlinkNumber = BlinkNumber;
        break;
      }

    case stopFire: {
        if (unitType == Commander) {
          writeToDisplay("Message Recived:", "Stop Fire");
        }
        greenBlinkNumber = BlinkNumber;
        break;
      }
    case Advance: {
        if (unitType == Commander) {
          writeToDisplay("Message Recived:", "Advance");
        }
        redBlinkNumber = BlinkNumber;
        break;
      }
    case Reatrat: {
        if (unitType == Commander) {
          writeToDisplay("Message Recived:", "Reatrat");
        }
        purpleBlinkNumber = BlinkNumber;
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
          redBlinkNumber = BlinkNumber;
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

void writeToRadio(uint32_t messege) {
  radio.openWritingPipe(addresses[0]);
  radio.stopListening();
  radio.write(&messege, sizeof(messege));
  Serial.println(messege);
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


