//Multi-Access Door Lock Code by Yousif Atia, Tim Bureac, Isaac Carlson
//
// ---------------- Bluetooth (RemoteXY) ----------------
#define REMOTEXY_MODE__SOFTSERIAL
#include <SoftwareSerial.h>

#define REMOTEXY_SERIAL_RX A1
#define REMOTEXY_SERIAL_TX A2
#define REMOTEXY_SERIAL_SPEED 9600

#include <RemoteXY.h>

// RemoteXY GUI configuration  
#pragma pack(push, 1)  
uint8_t const PROGMEM RemoteXY_CONF_PROGMEM[] =   // 46 bytes V19 
  { 255,1,0,11,0,31,0,19,0,0,0,0,27,1,106,200,1,1,2,0,
  67,23,141,66,11,68,64,26,11,1,36,54,34,34,1,2,64,0
  };
  
struct {

    // input variables
  uint8_t button_01; // bluetooth unlock button

    // output variables
  char value_01[11]; // string for bluetooth security feature

    // other variable
  uint8_t connect_flag;  // =1 if wire connected, else =0

} RemoteXY;   
#pragma pack(pop)
 

// ---------------- LIBRARIES ----------------
#include <SPI.h> //RFID Library
#include <MFRC522.h> //RFID Library
#include <Keypad.h> //Keypad Library
#include <Wire.h> //LCD display Library
#include <LiquidCrystal_I2C.h> //LCD display Library

// ---------------- PINS ----------------
//rfid pins
#define SS_PIN 10 
#define RST_PIN 9
//lock pin
#define LOCK_BUZZER_PIN A0

// ---------------- RFID ----------------
MFRC522 rfid(SS_PIN, RST_PIN); 
byte allowedUID[4] = {0x00, 0xEC, 0x75, 0x1B};
const byte uidSize = 4;

// ---------------- LCD ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2); //lcd is 16 by 2

// ---------------- KEYPAD ----------------
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = //sets up keypad characters
{
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {4, 3, 2, A3}; //keypad pins
byte colPins[COLS] = {8, 7, 6, 5}; //keypad pins

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ---------------- SECURITY ----------------
String correctPIN = "1234"; //pin unlock code
String correctPIN2 = "0000"; //second unlock code
String inputPIN = ""; //variable for entered pin
String lastOpenedBy = "None"; //variable for who opened lock

// ---------------- Function ----------------
bool unlocked = false; //unlocked variable
unsigned long unlockTime = 0; //variable for replacement delay function
const unsigned long unlockDuration = 5000; //stays unlocked for 5 seconds

// ---------------- LOCK ----------------
void lockDoor()
{
  digitalWrite(LOCK_BUZZER_PIN, LOW); //locks door pin
  unlocked = false; //unlocked variable

  lcd.clear(); //clears LCD
  lcd.print("Door Locked");  //displays Door Locked on screen
}

// ---------------- UNLOCK ----------------
void unlockDoor(String source)
{
  digitalWrite(LOCK_BUZZER_PIN, HIGH); //unlocks door
  unlocked = true; //unlocked variable

  lastOpenedBy = source; //whoever unlocked lock
  lastOpenedBy.toCharArray(RemoteXY.value_01, 11); //displays who last unlocked door on remotexy

  lcd.clear(); //clears lcd
  lcd.print("Opened by:"); //prints Opened by: on lcd
  lcd.setCursor(0, 1); //sets cursor to next row
  lcd.print(source); //prints who last opened door

  unlockTime = millis();  // start timer
}

// ---------------- RFID CHECK ----------------
bool checkRFID()
{
  if (!rfid.PICC_IsNewCardPresent()) return false; 
  if (!rfid.PICC_ReadCardSerial()) return false;

  if (rfid.uid.size != uidSize) return false; //if rfid card does fit specified character length

  for (byte i = 0; i < uidSize; i++) //function compares rfid code byte by byte to passcode
    {
    if (rfid.uid.uidByte[i] != allowedUID[i])
    {
      rfid.PICC_HaltA();
      return false;
    }
  }

  rfid.PICC_HaltA(); //pauses rfid so it only reads card once
  return true; //if code is correct then it finally returns true
}

// ---------------- SETUP ----------------
void setup()
{
  Serial.begin(9600); //serial monitor set to 9600

  SPI.begin(); //rfid initializes
  rfid.PCD_Init(); //rfid initializes

  pinMode(LOCK_BUZZER_PIN, OUTPUT); //initialize lock pin

  lcd.init(); //initialize lcd
  lcd.backlight(); //initialize lcd 

  lockDoor(); // locks door

  RemoteXY_Init(); //initializes bluetooth
  strcpy(RemoteXY.value_01, "None"); //sets bluetooth module string to None (the security string)
}

// ---------------- LOOP ----------------
void loop()
{
  RemoteXY_Handler(); //starts Bluetooth

  // ---- AUTO LOCK (5 seconds) ----
  if (unlocked && millis() - unlockTime >= unlockDuration) //lock door if unlock time is above unlock duration
  {
    lockDoor();
  }

  // ---- BLUETOOTH ----
  if (RemoteXY.connect_flag && RemoteXY.button_01) //if bluetooth is connected and remotexy button is pressed, unlock door
  {
    unlockDoor("Yousif");
    RemoteXY.button_01 = 0;
  }

  // ---- RFID ----
  if (!unlocked && checkRFID()) //if door is not unlocked and checkrfid returns true, unlock door
  {
    unlockDoor("Isaac");
  }

  // ---- KEYPAD ----
  char key = keypad.getKey(); //sets up key variable

  if (key) //statement handles keypad logic. Unlocks/locks door and prints corrent key on display.
  {
    lcd.clear(); //clears lcd

    if (key == '#') //# key is like enter button. If statement checks if pin is correct.
    {
      if (inputPIN == correctPIN) //if pin is correct, unlock door
      {
        unlockDoor("Tim");
      }
      else if ((inputPIN == correctPIN2)
      {
        unlockDoor("Dog Walker")
      }
      else //if pin is incorrect, else statement prints Wrong PIN, waits 0.8 second, and the locks door
      {
        lcd.print("Wrong PIN"); //if pin is incorrect print Wrong PIN

        unsigned long wrongTime = millis();
        while (millis() - wrongTime < 800) //wait 0.8 seconds
        {
          RemoteXY_Handler();  // keep Bluetooth alive
        }

        lockDoor(); //lock door
      }
      inputPIN = "";
    }
    else if (key == '*') //if * is pressed clear variable which also clears screen
    {
      inputPIN = ""; //clears input variable
    }
    else
    {
      inputPIN += key; //adds number to inpnut pin
    }

    lcd.setCursor(0, 1); //sets cursor to second row
    lcd.print(inputPIN); //prints input pin after each press
  }
}
