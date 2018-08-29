///--------------------------------------------------------------
/// Motorola & Shenkar - MotoUnit
/// MotoHQ code that will handle all comunication via USB port
/// With RF wireless comunication with CSMA/CA
///
/// By : Michael Lachower 032673584
///--------------------------------------------------------------
#include <Thread.h>
#include <ThreadController.h>
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


/// ThreadController will run the application with multithreading
/// 4 simultaneous threads are needed for :
/// duplex communication via serial and RF simultaneously
ThreadController controll = ThreadController();

/// radioListenerThread - is used for continuous listening to RF radio
/// to not miss any incoming RF communication
/// will run function : chackRadioForInput()
Thread radioListenerThread = Thread();

/// computerListenerThread - is used for continuous listening to Serial port
/// to not miss any incoming USB communication
/// will run function : chackComputerForInput()
Thread computerListenerThread = Thread();

/// rf24Sender - is used for sending RF messges when needed without
/// missing any incoming USB or RF communication
/// will run function : SendRfWhenPossible()
Thread rf24Sender = Thread();

/// Activation status of the device
/// if false device will not be able to comunicate Via RF
bool wasActivated = false;

/// LED connection Pins
int ledG = 4;
int ledB = 2;
int ledR = 3;

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
  radio.setAutoAck (true) ;
  /// set RF Radio to listening by default
  ReturnToListen();

  /// Set all LED Arduino pins
  pinMode(ledG, OUTPUT);
  pinMode(ledR, OUTPUT);
  pinMode(ledB, OUTPUT);


  // Configure application threading functions and timers
  radioListenerThread.onRun(chackRadioForInput);
  radioListenerThread.setInterval(50);

  computerListenerThread.onRun(chackComputerForInput);
  computerListenerThread.setInterval(50);

  rf24Sender.onRun(SendRfWhenPossible);
  rf24Sender.setInterval(0);

  // Adds all threads to the controller
  controll.add(&radioListenerThread);
  controll.add(&computerListenerThread);
  controll.add(&rf24Sender);
}

/// tests of there is messages waiting at RF Radio
void chackRadioForInput() {
  /// Chack if motoHQ is active and connected to HQ PC
  if (!wasActivated) {
    return;
  }
  /// Open RF Reading from RF chip
  radio.openReadingPipe(1, addresses[0]);
  radio.startListening();
  /// Chack if data is avilable
  if (radio.available())
  {
    /// Turn on Received message LED
    digitalWrite(ledG, HIGH);

    /// Read message
    uint32_t text = {0};
    radio.read(&text, sizeof(text));

    ///Send acknowledgment
    radio.writeAckPayload(0, text, sizeof(text));

    ///Send message to MotoHQ Via Serial connection
    sendMsgToComputer(text);
    /// Turn off Received message LED
    delay(200);
    digitalWrite(ledG, LOW);
  }
}


void chackComputerForInput() {
  /// test if unit is active and connected to HQ
  if (!wasActivated) {
    chackComputerActivation();
    return;
  }
  /// tests of there is messages waiting at serial port for the unit
  /// Chack if data is avilable
  if (Serial.available()) {
    /// Turn on Received message LED
    digitalWrite(ledG, HIGH);
    uint32_t msg = getMsgFromSerial();
    /// Send message to RF
    SendMsg(msg);
    delay(200);
    digitalWrite(ledG, LOW);

  }
}

/// test for activation messeges from serial
void chackComputerActivation() {
  /// tests of there is messages waiting at serial port for the unit
  /// Chack if data is avilable
  if (Serial.available()) {
    uint32_t msg = getMsgFromSerial();
    /// test for activation messeges has the correct format
    if (0x7F000 == msg) {
      wasActivated = true;
      /// Turn on Received message LED
      digitalWrite(ledG, HIGH);
      /// confirm activation messeges
      sendMsgToComputer(msg);
      delay(200);
      digitalWrite(ledG, LOW);
    }
  }
}

///  function for reciving Serial messages from the MotoHQ
///  @param : uint32_t msg - the message to send
uint32_t getMsgFromSerial() {
  byte buffer[sizeof(uint32_t)] = {0};
  uint32_t count = 0, msg = 0;
  while (0 < Serial.available()) buffer[count++] = Serial.read();
  msg = (*(uint32_t*)buffer);
  return msg;
}

///  function for sending Serial messages to the MotoHQ
///  @param : uint32_t msg - the message to send
void sendMsgToComputer(uint32_t msg) {
  Serial.write((byte*)&msg, sizeof(msg));
}

///  listener function used to detect the need for sending of a RF message
void SendRfWhenPossible() {
  if (outMessageAvilable) {
    writeToRadio(globalMessege);
  }
}

///  function for the button thread to initialize sending of a RF message
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
  /// Set maximum CS tries
  csResendsLeft = csMaxResends;
  /// Set backoff ranodm time
  long randBackoff = random(BeckoffRandomMin, BeckoffRandomMax);
  /// perform CS on RF channel
  while (!radio.testCarrier() && radio.testRPD () && csResendsLeft <= 0) {
    /// channel is occupied perform random backoff
    delay(randBackoff);
    csResendsLeft--;
  }
  /// test is CS max tries has been exceeded
  if (csResendsLeft > 0) { }
  else {
    /// sending falied chaneel not free
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
      delay(randBackoff);
      outMessageAvilable = true;
      caResendLeft--;
    } else {
      /// sending falied acknowledgment not recived return to listening
      outMessageAvilable = false;
      ReturnToListen();
    }

  } else {
    /// Message sent return to listening
    outMessageAvilable = false;
    ReturnToListen();
  }
}

///  function used to return RF Radio to listening
void ReturnToListen() {
  radio.openReadingPipe(0, addresses[0]);
  radio.startListening();
}

/// Main loop nothing to do here , just calling thred controll
// handeller to run all thredes
void loop() {
  controll.run();
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


