
// ---------------- Bluetooth ----------------
#define REMOTEXY_MODE__SOFTSERIAL
#include <SoftwareSerial.h>
#define REMOTEXY_SERIAL_RX A2
#define REMOTEXY_SERIAL_TX A1
#define REMOTEXY_SERIAL_SPEED 9600
#include <RemoteXY.h>

// RemoteXY GUI configuration  
#pragma pack(push, 1)  
uint8_t const PROGMEM RemoteXY_CONF_PROGMEM[] =   // 38 bytes V19 
  { 255,1,0,11,0,31,0,19,0,0,0,0,27,1,106,200,1,1,2,0,
  67,23,141,66,11,68,64,26,11,1,36,54,34,34,1,2,64,0 };
  
// this structure defines all the variables and events of your control interface 
struct {

    // input variables
  uint8_t button_01; // =1 if button pressed, else =0, from 0 to 1

    // output variables
  char value_01[11]; // string UTF8 end zero

    // other variable
  uint8_t connect_flag;  // =1 if wire connected, else =0

} RemoteXY;   
#pragma pack(pop)
 
/////////////////////////////////////////////
//           END RemoteXY include          //
/////////////////////////////////////////////

#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

// ---------------- PINS ----------------
#define SS_PIN 10
#define RST_PIN 9
#define LOCK_BUZZER_PIN A0

#define LCD_ADDR 0x27
#define LCD_COLS 16
#define LCD_ROWS 2


#define BT_RX_PIN A2
#define BT_TX_PIN A1

// ---------------- RFID ----------------
MFRC522 rfid(SS_PIN, RST_PIN);

// ---------------- LCD ----------------
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);

// ---------------- KEYPAD ----------------
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};



// Keypad pins
byte rowPins[ROWS] = {4, 3, 2, A3};
byte colPins[COLS] = {8, 7, 6, 5};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ---------------- VARIABLES ----------------
String correctPIN = "1234";
String inputPIN = "";
int inputNumber = 0;
byte allowedUID[4] = {0xDE, 0xAD, 0xBE, 0xEF};

int button_01 = 0;
// ---------------- FUNCTIONS ----------------
void lockDoor() {
  digitalWrite(LOCK_BUZZER_PIN, LOW);
  lcd.clear();
  lcd.print("Door Locked");
}

void unlockDoor() {
  digitalWrite(LOCK_BUZZER_PIN, HIGH);
  lcd.clear();
  lcd.print("Door Unlocked");
  delay(5000);
  digitalWrite(LOCK_BUZZER_PIN, LOW);
  lcd.clear();
  lcd.print("Ready");

  
}

// 
bool checkRFID() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial())
    return false;

  for (byte i = 0; i < 4; i++) {
    if (rfid.uid.uidByte[i] != allowedUID[i])
      return false;
  }
  return true;
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();

  pinMode(LOCK_BUZZER_PIN, OUTPUT);
  lockDoor(); // starts locked

  lcd.init();
  lcd.backlight();

  RemoteXY_Init ();
  lcd.setCursor(0, 0);
  lcd.print("Ready");
  
  
}

// ---------------- LOOP ----------------
void loop() {
    // ----- BLUETOOTH -----
  RemoteXYEngine.handler ();
 if (RemoteXY.button_01 = 0)
 {
  unlockDoor();
 }
  
    
    
    
  
  // ----- RFID -----
  if (checkRFID()) {
    unlockDoor();
  }

  // ----- KEYPAD -----
  char key = keypad.getKey();
  if (key) {
    lcd.clear();
    if (key == '#') {
      if (inputPIN == correctPIN) {
        unlockDoor();
      } else {
        // wrong PIN
        delay(300);
        digitalWrite(LOCK_BUZZER_PIN, LOW);
        lcd.clear();

      }
      inputPIN = "";
    } else if (key == '*') {
      inputPIN = "";
      lcd.clear();
    } else {
      inputPIN += key;
    }
    lcd.setCursor(0, 0);
    lcd.print(inputPIN);
  }
  //Serial.print(key);

}
