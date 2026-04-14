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

bool unlocked = false;

// ---------------- TIMING  ----------------
unsigned long unlockTime = 0;
bool unlockActive = false;

// ---------------- BLUETOOTH EDGE DETECTION ----------------
bool lastButtonState = false;

// ---------------- LOCK FUNCTION ----------------
void lockDoor()
{
  digitalWrite(LOCK_BUZZER_PIN, LOW);
  unlocked = false;
  unlockActive = false;

  lcd.clear();
  lcd.print("Door Locked");
}

// ---------------- UNLOCK FUNCTION ----------------
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
  unlockActive = true;
}

// ---------------- RFID CHECK ----------------
bool checkRFID()
{
  if (!rfid.PICC_IsNewCardPresent()) return false;
  if (!rfid.PICC_ReadCardSerial()) return false;

  if (rfid.uid.size != uidSize) return false;

  for (byte i = 0; i < uidSize; i++)
  {
    if (rfid.uid.uidByte[i] != allowedUID[i])
    {
      rfid.PICC_HaltA();
      return false;
    }
  }

  rfid.PICC_HaltA();
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

  // ---------------- BLUETOOTH  ----------------
  bool currentButtonState = (RemoteXY.connect_flag && RemoteXY.button_01 != 0);

  if (currentButtonState && !lastButtonState)
  {
    unlockDoor("Bluetooth");
  }

  lastButtonState = currentButtonState;

  // ---------------- AUTO RE-LOCK  ----------------
  if (unlockActive && (millis() - unlockTime >= 5000))
  {
    lockDoor();
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
