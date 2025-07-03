#include <LiquidCrystal.h>
#include "HX711.h"

// === HX711 ===
#define DOUT_PIN   3
#define SCK_PIN    2
const float calibration_factor = 13594.0f;
HX711 scale;

// === LCD Keypad Shield (RS,E,D4,D5,D6,D7) ===
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// === Кнопки на A0 ===
#define BUTTON_PIN   A0
#define BTN_NONE     0
#define BTN_RIGHT    1   // TARE
#define BTN_UP       2   // START
#define BTN_DOWN     3   // STOP / BACK
#define BTN_LEFT     4   // режим 25 кг
#define BTN_SELECT   5   // режим 20 кг

// === POWER PIN ===
const int POWER_PIN = 11;

// === CGRAM-символи ===
byte chrB[8]  =       {0b11111,0b10000,0b11110,0b10001,0b10001,0b10001,0b11110,0b00000}; // Б
byte chrZ[8]  =       {0b10101,0b10101,0b01110,0b00100,0b01110,0b10101,0b10101,0b00000}; // Ж
byte chrR[8]  =       {0b11110,0b10001,0b10001,0b11110,0b10000,0b10000,0b10000,0b00000}; // Р
byte chrV[8]  =       {0b10001,0b10011,0b10011,0b10101,0b10101,0b11001,0b11001,0b00000}; // И
byte chrSoftSign[8] = {0b10000,0b10000,0b10000,0b11110,0b10001,0b10001,0b11110,0b00000}; // Ь
byte chrG[8] =        {0b11111,0b10000,0b10000,0b10000,0b10000,0b10000,0b10000,0b00000}; // Г
byte chrP[8] =        {0b11111,0b10001,0b10001,0b10001,0b10001,0b10001,0b10001,0b00000}; // П
byte chrCH[8] =       {0b10001,0b10001,0b10001,0b01111,0b00001,0b00001,0b00001,0b00000}; // Ч

// === Стани автомата ===
enum State { ST_MENU, ST_20KG, ST_25KG, ST_MANUAL };
State state = ST_MENU;

unsigned long lastBlinkTime = 0;
bool blinkOn = true;
float weight = 0.0;

int readButton() {
  int v = analogRead(BUTTON_PIN);
  if      (v <  50) return BTN_RIGHT;
  else if (v < 200) return BTN_UP;
  else if (v < 400) return BTN_DOWN;
  else if (v < 600) return BTN_LEFT;
  else if (v < 800) return BTN_SELECT;
  return BTN_NONE;
}

void displayMenu() {
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("O");     // O
  lcd.write(byte(0)); // Б
  lcd.print("EPIT");    // EPІТ
  lcd.write(byte(4)); // Ь
  lcd.print(" PE");     
  lcd.write(byte(1)); // Ж
  lcd.write(byte(3)); // И
  lcd.print("M:");

  lcd.setCursor(0,1);
  lcd.print("20L | 25L | PY");
  lcd.write(byte(7)); // Ч
  lcd.print("H");
}

void displayHeader() {
  lcd.clear();
  lcd.setCursor(2,0);
  lcd.write(byte(5));
  lcd.print("OTO"); 
  lcd.write(byte(7));
  lcd.print("HA BA");
  lcd.write(byte(6));
  lcd.print("A:   "); 
}

void setup() {
  Serial.begin(9600);
  scale.begin(DOUT_PIN, SCK_PIN);
  scale.set_scale(calibration_factor);

  lcd.begin(16,2);
  lcd.createChar(0, chrB);
  lcd.createChar(1, chrZ);
  lcd.createChar(2, chrR);
  lcd.createChar(3, chrV);
  lcd.createChar(4, chrSoftSign);
  lcd.createChar(5, chrP);
  lcd.createChar(6, chrG);
  lcd.createChar(7, chrCH);

  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, LOW);

  displayMenu();
}

void loop() {
  int btn = readButton();

  if (btn == BTN_DOWN) {
    digitalWrite(POWER_PIN, LOW);
    state = ST_MENU;
    displayMenu();
    delay(100);
    return;
  }

  else if (btn == BTN_RIGHT) {
    scale.tare();
    lcd.clear();
    lcd.setCursor(3,0);
    lcd.print("TAPA - OK!");
    delay(600);
    (state == ST_MENU ? displayMenu() : displayHeader());
    delay(50);
  }

  if (state == ST_MENU) {
    if (btn == BTN_SELECT) {
      state = ST_20KG;
      scale.tare();
      displayHeader();
      digitalWrite(POWER_PIN, HIGH);
      delay(200);
    } else if (btn == BTN_LEFT) {
      state = ST_25KG;
      scale.tare();
      displayHeader();
      digitalWrite(POWER_PIN, HIGH);
      delay(200);
    } else if (btn == BTN_UP) {
      state = ST_MANUAL;
      scale.tare();
      displayHeader();
      digitalWrite(POWER_PIN, HIGH);
      delay(200);
    }

    // --- Блимання останнього символу верхнього рядка ---
    if (millis() - lastBlinkTime > 500) {
      lastBlinkTime = millis();
      blinkOn = !blinkOn;
      lcd.setCursor(15, 0);
      lcd.print(blinkOn ? "*" : " ");
      lcd.setCursor(0, 0);
      lcd.print(blinkOn ? "*" : " ");
    }

    return;
  }

  weight = scale.get_units(10);
  if (weight < 0.0f) weight = 0.0f;
  lcd.setCursor(0,1);
  lcd.print("                ");
  lcd.setCursor(0,1);
  lcd.print(weight,2);
  lcd.print(" kg");

  if ((state == ST_20KG && weight >= 19.5) || (state == ST_25KG && weight >= 24.5)) {
    digitalWrite(POWER_PIN, LOW);
    state = ST_MENU;
    displayMenu();
  }

  delay(20);
}