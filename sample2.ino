// ---------------- Bluetooth (RemoteXY) ----------------
#define REMOTEXY_MODE__SOFTSERIAL
#include <SoftwareSerial.h>

#define REMOTEXY_SERIAL_RX A1
#define REMOTEXY_SERIAL_TX A2
#define REMOTEXY_SERIAL_SPEED 9600

// RemoteXY connection settings 
#define REMOTEXY_SERIAL_RX 2
#define REMOTEXY_SERIAL_TX 3
#define REMOTEXY_SERIAL_SPEED 9600

#include <RemoteXY.h>

// RemoteXY GUI configuration  
#pragma pack(push, 1)  
uint8_t const PROGMEM RemoteXY_CONF_PROGMEM[] =   // 46 bytes V19 
  { 255,1,0,11,0,39,0,19,0,0,0,0,27,1,106,200,1,1,2,0,
  67,23,141,66,11,68,64,26,11,10,41,59,24,24,48,4,26,31,79,78,
  0,31,79,70,70,0 };
  
struct {

    // input variables
  uint8_t button_01; // =1 if state is ON, else =0, from 0 to 1

    // output variables
  char value_01[11]; // string UTF8 end zero

    // other variable
  uint8_t connect_flag;  // =1 if wire connected, else =0

} RemoteXY;   
#pragma pack(pop)
 

// ---------------- LIBRARIES ----------------
#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------------- PINS ----------------
#define SS_PIN 10
#define RST_PIN 9
#define LOCK_BUZZER_PIN A0

// ---------------- RFID ----------------
MFRC522 rfid(SS_PIN, RST_PIN);
byte allowedUID[] = {0xDE, 0xAD, 0xBE, 0xEF};
const byte uidSize = 4;

// ---------------- LCD ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------------- KEYPAD ----------------
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = 
{
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {4, 3, 2, A3};
byte colPins[COLS] = {8, 7, 6, 5};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ---------------- SECURITY ----------------
String correctPIN = "1234";
String inputPIN = "";
String lastOpenedBy = "None";

bool unlocked = false;

// ---------------- FUNCTIONS ----------------
void lockDoor()
{
  digitalWrite(LOCK_BUZZER_PIN, LOW);
  unlocked = false;
  lcd.clear();
  lcd.print("Door Locked");
}

void unlockDoor(String source) 
{
  digitalWrite(LOCK_BUZZER_PIN, HIGH);
  unlocked = true;

  lastOpenedBy = source;

  lcd.clear();
  lcd.print("Opened by:");
  lcd.setCursor(0, 1);
  lcd.print(source);

  // Bluetooth app
  lastOpenedBy.toCharArray(RemoteXY.value_01, 11);

  delay(5000);

  lockDoor();
}

// ---------------- RFID CHECK ----------------
bool checkRFID() {
  if (!rfid.PICC_IsNewCardPresent()) return false;
  if (!rfid.PICC_ReadCardSerial()) return false;

  if (rfid.uid.size != uidSize) return false;

  for (byte i = 0; i < uidSize; i++) {
    if (rfid.uid.uidByte[i] != allowedUID[i]) {
      rfid.PICC_HaltA();
      return false;
    }
  }

  rfid.PICC_HaltA();
  return true;
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();

  pinMode(LOCK_BUZZER_PIN, OUTPUT);
  lockDoor();

  lcd.init();
  lcd.backlight();
  lcd.print("Ready");

  RemoteXY_Init();
  strcpy(RemoteXY.value_01, "None");
}

// ---------------- LOOP ----------------
void loop() {

  RemoteXY_Handler();

  // ---- BLUETOOTH ----
  if (RemoteXY.connect_flag && RemoteXY.button_01 == 1) 
  {
    Serial.print(1);
    unlockDoor("Bluetooth");
  }
  else
  {
    Serial.print(0);
  }

  // ---- RFID ----
  if (!unlocked && checkRFID()) {
    unlockDoor("RFID");
  }

  // ---- KEYPAD ----
  char key = keypad.getKey();

  if (key) {
    lcd.clear();

    if (key == '#') {
      if (inputPIN == correctPIN) {
        unlockDoor("PIN");
      } else {
        lcd.print("Wrong PIN");
        delay(800);
        lockDoor();
      }
      inputPIN = "";
    }
    else if (key == '*') {
      inputPIN = "";
    }
    else {
      inputPIN += key;
    }

    lcd.setCursor(0, 1);
    lcd.print(inputPIN);
  }
}
