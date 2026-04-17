// ---------------- Bluetooth (RemoteXY) ----------------
#define REMOTEXY_MODE__SOFTSERIAL
#include <SoftwareSerial.h>

#define REMOTEXY_SERIAL_RX A1
#define REMOTEXY_SERIAL_TX A2
#define REMOTEXY_SERIAL_SPEED 9600

#include <RemoteXY.h>

#pragma pack(push, 1)
uint8_t const PROGMEM RemoteXY_CONF_PROGMEM[] =
{ 255,1,0,11,0,31,0,19,0,0,0,0,27,1,106,200,1,1,2,0,
  67,23,141,66,11,68,64,26,11,1,36,54,34,34,1,2,64,0 };
#pragma pack(pop)

struct {
  uint8_t button_01;
  char value_01[11];
  uint8_t connect_flag;
} RemoteXY;

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
byte allowedUID[] = {0x00, 0xEC, 0x75, 0x1B};
const byte uidSize = 4;

// ---------------- LCD ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------------- KEYPAD ----------------
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
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

// ---------------- functions ----------------
bool unlocked = false;
unsigned long unlockTime = 0;
const unsigned long unlockDuration = 5000;

// ---------------- LOCK ----------------
void lockDoor()
{
  digitalWrite(LOCK_BUZZER_PIN, LOW);
  unlocked = false;

  lcd.clear();
  lcd.print("Door Locked");
}

// ---------------- UNLOCK ----------------
void unlockDoor(String source)
{
  digitalWrite(LOCK_BUZZER_PIN, HIGH);
  unlocked = true;

  lastOpenedBy = source;
  lastOpenedBy.toCharArray(RemoteXY.value_01, 11);

  lcd.clear();
  lcd.print("Opened by:");
  lcd.setCursor(0, 1);
  lcd.print(source);

  unlockTime = millis();
}

// ---------------- RFID CHECK ----------------
bool checkRFID()
{
  if (!rfid.PICC_IsNewCardPresent()) return false;
  if (!rfid.PICC_ReadCardSerial()) return false;

  if (rfid.uid.size != uidSize) return false;

  byte allowedUID[4] = {0x00, 0xEC, 0x75, 0x1B};

  for (byte i = 0; i < rfid.uid.size; i++)
  {
    if (rfid.uid.uidByte[i] != allowedUID[i])
    {
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
      return false;
    }
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  return true;
}
// ---------------- SETUP ----------------
void setup()
{
  Serial.begin(9600);

  SPI.begin();
  rfid.PCD_Init();

  pinMode(LOCK_BUZZER_PIN, OUTPUT);

  lcd.init();
  lcd.backlight();

  lockDoor();

  RemoteXY_Init();
  strcpy(RemoteXY.value_01, "None");
}

// ---------------- LOOP ----------------
void loop()
{
  RemoteXY_Handler();

  // ---------------- AUTO LOCK (5 seconds) ----------------
  if (unlocked && millis() - unlockTime >= unlockDuration)
  {
    lockDoor();
  }

  // ---------------- BLUETOOTH  ----------------
  if (RemoteXY.connect_flag && RemoteXY.button_01)
  {
    unlockDoor("Bluetooth");
    RemoteXY.button_01 = 0;   
  }

  // ---------------- RFID ----------------
  if (!unlocked && checkRFID())
  {
    unlockDoor("RFID");
  }

  // ---------------- KEYPAD ----------------
  char key = keypad.getKey();

  if (key)
  {
    lcd.clear();

    if (key == '#')
    {
      if (inputPIN == correctPIN)
      {
        unlockDoor("PIN");
      }
      else
      {
        lcd.print("Wrong PIN");
        delay(800);
        lockDoor();
      }
      inputPIN = "";
    }
    else if (key == '*')
    {
      inputPIN = "";
    }
    else
    {
      inputPIN += key;
    }

    lcd.setCursor(0, 1);
    lcd.print(inputPIN);
  }
}
