///--------------------------------------------------------------
/// Motorola & Shenkar - MotoUnit
/// MotoHQ code that will handle all comunication via USB port
/// With RF wireless comunication with CSMA/CA
///
/// By : Michael Lachower 032673584
///--------------------------------------------------------------
#include <Thread.h>
#include <ThreadController.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

/// Protocol enum defines
/// Public enums used for all units and HQ for protocol consistency
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



/// RF24 variable for radio comunication
RF24 radio(9, 10);

/// RF Radio comunication addresses
/// Address 000001 : Global communication address
/// Address 000002 : Local cluster communication address
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
/// used to queue incoming messeges
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

/// ThreadController will run the application with multithreading
/// 4 simultaneous threads are needed for :
/// duplex communication via RF
/// LED controll
/// Button responsiveness
ThreadController controll = ThreadController();

/// inputButtonThread - is used for continuous listening to MotoUnit Buttons
/// to not miss button presses
/// will run function : chackInputButtons()
Thread inputButtonThread = Thread();

/// outputButtonThread - is used for continuous handling of LED
/// MotoUnit output
/// will run function : blinkLed()
Thread outputButtonThread = Thread();

/// rf24Sender - is used for sending RF messges when needed without
/// missing any incoming USB or RF communication
/// will run function : SendRfWhenPossible()
Thread rf24Sender = Thread();

/// Moto Unit Variables

/// Activation status of the device
/// if false device will only be able to
/// send activation beacon messeges until
/// unit ID is given as a response by MotoHQ
bool wasActivated = false;

/// Activation beacon messeges status
/// if true device has sent Activation beacon messege
/// and is waiting for an answer
bool wasRequestIdSent = false;

/// Specific MotoUnit Address will be changed as soon as device is initiated
uint8_t motounitAdress = 0;

/// Specific MotoUnit Cluster ID will be changed as soon as device is initiated
uint8_t motounitClusterID = 0;

/// Specific MotoUnit Type determined by the hardware of the device
/// can be set to : Commander / Soldier
Sender_Type unitType = Soldier;

/// Setup is called once every time the device is turend on
void setup() {

  /// Random Seed is generated every boot and uses analog true random generation
  randomSeed(analogRead(0));

  /// Serial communication initialized at 9600 baud rate
  Serial.begin(9600);

  /// RF communication initialized
  radio.begin();
  /// set RF communication power level is set to max for maximum distance
  radio.setPALevel(RF24_PA_MAX);
  /// set Auto acknowledgment of RF communication
  radio.setAutoAck (true ) ;
  /// set RF Radio to listening by default
  ReturnToListen();

  /// Chack if the RF chip is connected and write to Serial
  /// can be used for Serial port debuging
  Serial.print("is Chip Connected = " );
  Serial.println(radio.isChipConnected());

  /// Set all button and LED Arduino pins
  pinMode(ledG, OUTPUT);
  pinMode(ledR, OUTPUT);
  pinMode(ledB, OUTPUT);
  pinMode(button1Pin, INPUT);
  pinMode(button2Pin, INPUT);
  pinMode(button3Pin, INPUT);



  /// Set activation state to false and  device connected LED to show that
  wasActivated = false;
  digitalWrite(ledR, HIGH);


  // Configure application threading functions and timers
  rf24Sender.onRun(SendRfWhenPossible);
  rf24Sender.setInterval(0);

  inputButtonThread.onRun(chackInputButtons);
  inputButtonThread.setInterval(0);

  outputButtonThread.onRun(blinkLed);
  outputButtonThread.setInterval(0);

  // Adds all threads to the controller
  controll.add(&rf24Sender);
  controll.add(&inputButtonThread);
  controll.add(&outputButtonThread); // & to pass the pointer to it

  // Setup LCD for Moto Commander
  if (unitType == Commander) {
    initializeLCD();
    writeToDisplay("Waiting For ID", "Press button 1");
  }
}



void loop() {
  controll.run();
  /// Chack if unit is active and connected to HQ
  if (!wasActivated) {
    /// send activation beacon messeges until
    /// unit ID is given as a response by MotoHQ
    sendActivationBeacon();
    return;
  }
  decraseButtonTimers();
  /// Test if any button is pressed and if so
  /// handle writing message to RF Radio
  if (wasButton1Pressed && button1Timer <= 0) {
    uint32_t messege = 0;
    /// set message caaording to unit type
    if (unitType == Commander) {
      messege = makeMessage(0, All, Fire) ;
    } else {
      messege = makeMessage(0, All, Distres) ;
    }
    /// broadcast message
    SendMsg(messege);
    wasButton1Pressed =  false;
  }
  else if (wasButton2Pressed && button2Timer <= 0) {
    uint32_t messege = 0;
    /// set message caaording to unit type
    if (unitType == Commander) {
      messege = makeMessage(0, All, stopFire) ;
    } else {
      messege = makeMessage(0, All, Distres) ;
    }
    /// broadcast message
    SendMsg(messege);
    wasButton2Pressed =  false;
  }
  else if (wasButton3Pressed && button3Timer <= 0) {
    uint32_t messege = 0;
    /// set message caaording to unit type
    if (unitType == Commander) {
      messege = makeMessage(0, All, Advance) ;
    } else {
      messege = makeMessage(0, All, Distres) ;
    }

    /// broadcast message
    SendMsg(messege);
    wasButton3Pressed =  false;
  }

  /// tests of there is messages waiting at RF Radio for the unit
  /// Chack if data is avilable
  if (radio.available())
  {
    /// Read message
    uint32_t text = {0};
    radio.read(&text, sizeof(text));

    ///Send acknowledgment
    radio.writeAckPayload(0, text, sizeof(text));

    /// handle output via LEDs to unit
    handleMessage(text);

  }
}

///  listener function used to detect the need for sending of a RF message
void SendRfWhenPossible() {
  if (outMessageAvilable) {
    writeToRadio(globalMessege);
  }
}

///  function by the button thread to initialize sending of a RF message
///  @param : uint32_t msg - the message to send
void SendMsg(uint32_t msg) {
  caResendLeft = caMaxResend;
  globalMessege = msg;
  outMessageAvilable = true;
}

///  function used to send a 32 bit message Via RF Radio
///  using CSMA/CA to verify the communication
///  @param : uint32_t messege - the message to send
void writeToRadio(uint32_t messege) {

  Serial.println("Sending Message");
  /// Set maximum CS tries
  csResendsLeft = csMaxResends;
  /// Set backoff ranodm time
  long randBackoff = random(BeckoffRandomMin, BeckoffRandomMax);
  /// perform CS on RF channel
  while (!radio.testCarrier() && radio.testRPD () && csResendsLeft <= 0) {
    /// channel is occupied perform random backoff
    delay(randBackoff);
    Serial.println("Channel Occupied");
    csResendsLeft--;
  }
  /// test is CS max tries has been exceeded
  if (csResendsLeft > 0) {
    Serial.println("Channel free for Sending");
  }
  else {
    /// sending falied chaneel not free
    Serial.println("Exited via Max Timer : Sending failed");
    delay(randBackoff);
    outMessageAvilable = true;
    return;
  }
  /// set writing RF pipe
  radio.openWritingPipe(addresses[0]);
  radio.stopListening();
  /// send data on RF
  bool writeRslt = radio.write(&messege, sizeof(messege));
  /// test if massage was acknowledged for CA
  if (!writeRslt) {
    /// test if massage was not acknowledgeed more then CA resends left
    if (caResendLeft > 0) {
      /// acknowledgment not recived perform random backoff
      Serial.print("Ack Not Recived -> resending for time : ");
      Serial.println(caResendLeft);
      Serial.print("Waiting For : ");
      Serial.println(randBackoff);
      delay(randBackoff);
      outMessageAvilable = true;
      caResendLeft--;
    } else {
      /// sending falied acknowledgment not recived return to listening
      outMessageAvilable = false;
      Serial.println("Ack Not Recived 20 times Msg Failed");
      ReturnToListen();
    }

  } else {
    /// Message sent return to listening
    outMessageAvilable = false;
    Serial.println("Ack Recived");
    ReturnToListen();
    Serial.println(messege);
  }
}

/// Returns RF to lisening mode
void ReturnToListen() {
  radio.openReadingPipe(0, addresses[0]);
  radio.startListening();
}


/// send activation beacon messeges until
/// unit ID is given as a response by MotoHQ
void sendActivationBeacon() {

  /// Reset Button Timers if needed
  decraseButtonTimers();
  /// if button 1 is pressed send activation beacon
  if (wasButton1Pressed && button1Timer <= 0) {
    /// set message to activation beacon
    uint32_t messege  = makeMessage(0, Control, ReqestID) ;
    /// broadcast message
    SendMsg(messege);
    /// Reset Button
    wasButton1Pressed =  false;
    /// Update request ID sent
    wasRequestIdSent = true;

  }
  /// if request ID is not sent dont continue
  if (!wasRequestIdSent) {
    return;
  }
  /// Reading from RF to see if there is a response
  if (radio.available())
  {
    /// if there is a response send the result to be verified
    handleHandShkae();
  }
}

/// verify response from MotoHQ and recive ID & cluster ID
void handleHandShkae() {
  uint32_t text = {0};
  /// Read verification message
  radio.read(&text, sizeof(text));
  ///Send acknowledgment
  radio.writeAckPayload(0, text, sizeof(text));
  ///  extract message data
  MessageData data = GetData( text);
  ///  output visual acknowledgment
  handleMessage(text);
  /// verify message data
  if (data == AssignID) {
    /// Activate Device
    wasActivated = true;
    Serial.println("Connected");
    ///  extract device ID from message
    motounitAdress = GetReciverAdress(text);
    Serial.print("motounitAdress = ");
    Serial.println(motounitAdress);
    ///  extract cluster ID from message
    motounitClusterID = GetClusterId(text);
    ///  turn off not device connected LED
    digitalWrite(ledR, LOW);
    if (unitType == Commander) {
      ///  write message to Commander LCD screen display
      char unitAdressString[8];
      writeToDisplay("Connected with ID:", itoa(motounitAdress, unitAdressString, 10));
    }

  } else {
    Serial.println("Not Connected");
  }

}

/// handle message according to unit type and message type
void handleMessage(uint32_t text) {

  /// print to serial all message data for debug
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

  // test if the message is addresed to Same Cluster ID as unit
  if (clusterId == motounitClusterID) {
    // test if this unit is a commander
    if (unitType == Commander) {
      // output message to unit
      outputMessageData(data);
      return;
    }
    // test if sending unit is a commander
    if (senderType == Commander) {
      // output message to unit
      outputMessageData(data);
      return;
    }
    // test if sending unit is HQ
    if (senderType == HQ) {
      /// if the maggae is a global broadcast message
      if (brodcastType == All || brodcastType == Distress) {
        // output message to unit
        outputMessageData(data);
        return;
      }
      /// if the maggae is addresed to unit
      if (brodcastType == Single && motounitAdress == recAdress) {
        // output message to unit
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


/// thread function fro reciving live button input
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


/// Blink Leds if message was recived in a diffrent thread
void blinkLed() {
  /// test what LED is to be blinked
  if (redBlinkNumber >= 1) {
    digitalWrite(ledR, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(200);                 // wait for a second
    digitalWrite(ledR, LOW);    // turn the LED off by making the voltage LOW
    delay(200);
    redBlinkNumber--;
  }
  /// test what LED is to be blinked
  else if (blueBlinkNumber >= 1) {
    digitalWrite(ledB, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(200);                 // wait for a second
    digitalWrite(ledB, LOW);    // turn the LED off by making the voltage LOW
    delay(200);
    blueBlinkNumber--;
  }
  /// test what LED is to be blinked
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


///  Clears Commander LCD screen display
void clearLCD() {
  LCD.write(0xFE);
  LCD.write(0x01);
}
///  initialize Commander LCD screen display
void initializeLCD() {
  LCD.begin(9600);
  clearLCD();
}
///  write message in 2 string rows Commander LCD screen display
///  @param : const char *row1 - the 1 row to write to display
///  @param : const char *row2 - the 2 row to write to display
void writeToDisplay(const char *row1, const char *row2) {
  clearLCD();
  LCD.write(row1);
  LCD.write(254);
  LCD.write(192);
  LCD.write(row2);
}
/// Reset Button Timers if needed has to be called every loop iteration
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
///  Helper function used to make m message using the global enums
///  @param : uint8_t reciver - the 8 bit reciver address
///  @param : Brodcast_Type type - the BrodcastType of the message
///  @param :  MessageData data - the MessageData of the message
///  #return : uint32_t - the final message created
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

///  Helper function used to set up to 8 bits in a 32 bit message indepentely
///  @param : uint32_t messege - the message to change bits in
///  @param : uint8_t adress - the bits to change to
///  @param : int startPoint - the bit location to start the change at
///  @param : int endPoint - the bit location to end the change at
///  #return : uint32_t - the changed message
uint32_t setBits(uint32_t messege, uint8_t adress, int startPoint, int endPoint) {
  int count = 0;
  for (int i = startPoint; i < endPoint; i++) {
    bitWrite(messege, i, bitRead(adress, count++));
  }
  return messege;

}

///  Helper function used to get a 8 bit sender address from a 32 bit message
///  @param : uint32_t messege - the 32 bit message
///  #return : uint8_t - the 8 bit sender address
uint8_t GetSenderAdress(uint32_t messege) {
  return GetAdress(messege, SenderAdress);
}
///  Helper function used to get a 8 bit reciver address from a 32 bit message
///  @param : uint32_t messege - the 32 bit message
///  #return : uint8_t - the 8 bit sender address
uint8_t GetReciverAdress(uint32_t messege) {
  return GetAdress(messege, ReciverAdress);
}

///  Helper function used to get a 8 bit address from a 32 bit message
///  @param : uint32_t messege - the 32 bit message
///  @param : int offset - the offset of the wanted address
///  #return : uint8_t - the 8 bit address
uint8_t GetAdress(uint32_t messege, int offset) {
  uint8_t add = 0;
  int count = 0;
  for (int i = offset; i < offset + AdressSize; i++) {
    bitWrite(add, count++, bitRead(messege, i));
  }
  return add;
}

///  Helper function used to get a 8 bit cluster ID from a 32 bit message
///  @param : uint32_t messege - the 32 bit message
///  #return : uint8_t - the 8 bit cluster ID
uint8_t GetClusterId(uint32_t messege) {
  int offset = ClusterId;
  uint8_t add = 0;
  int count = 0;
  for (int i = offset; i < offset + ClusterIdSize; i++) {
    bitWrite(add, count++, bitRead(messege, i));
  }
  return add;
}

///  Helper function used to get message data from a 32 bit message
///  @param : uint32_t messege - the 32 bit message
///  #return : MessageData - the MessageData enum
MessageData GetData(uint32_t messege) {
  int offset = Data;
  uint8_t add = 0;
  int count = 0;
  for (int i = offset; i < offset + DataSize; i++) {
    bitWrite(add, count++, bitRead(messege, i));
  }
  return add;
}


///  Helper function used to get BrodcastType from a 32 bit message
///  @param : uint32_t messege - the 32 bit message
///  #return : BrodcastType - the BrodcastType enum
Brodcast_Type GetBrodcastType(uint32_t messege) {
  int offset = BrodcastType;
  uint8_t add = 0;
  int count = 0;
  for (int i = offset; i < offset + BrodcastTypeSize; i++) {
    bitWrite(add, count++, bitRead(messege, i));
  }
  return add;
}

///  Helper function used to get Sender Type from a 32 bit message
///  @param : uint32_t messege - the 32 bit message
///  #return : Sender_Type - the Sender_Type enum
Sender_Type GetSenderType(uint32_t messege) {
  int offset = SenderType;
  uint8_t add = 0;
  int count = 0;
  for (int i = offset; i < offset + SenderTypeSize; i++) {
    bitWrite(add, count++, bitRead(messege, i));
  }
  return add;
}

///  Helper function used to print a 32 bit message in binary format
///  @param : uint32_t messege - the 32 bit message
void printMessageBits(uint32_t messege) {
  for (int i = 0; i < 32; i++) {

    Serial.print(bitRead(messege, i));
    if ((i + 1) % 4 == 0) {
      Serial.print(" ");
    }
  }
  Serial.println(" ");
}




