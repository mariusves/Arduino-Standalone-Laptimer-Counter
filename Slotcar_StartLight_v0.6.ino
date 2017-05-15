/* Slotcar Race Lights with Laptimer/counter v0.6
 Author: Rotzbouf
 Date: 12/05/17
 Description:
  * Complete Code rewritten in v0.5
    * Interrupt Routines are now as short as possible and only save the timestamp of activation 
  * When the Board is started up, it will show "READY!" in LCD
  * The starting phase will be initiated by pressing the START_RACE button. (Pin 48)
    * When start phase is initiated, "SET!" will be visible on LCD
    * Inverval between each LED is set random between 1s and 3s.
    * When lights turn to green, "GO!" will be visible on LCD
  * Timing functions are called via Interrupts (Pin 2 and Pin 3 on the Arduino)
  * Bestlap is recorded per lane and overall
  * Lapcount (L), Laptime (T), Besttime (B) is shown per lane on LCD
  * Best overall lap is visible on LCD
  * Stores Best overall laptime in EEPROM to save time on reboot/poweroff
  * Clear EEPROM memory by pressing RESET Button. (Pin 41)
    * A message will be displayed on LCD when clearing EEPROM.
  * Both pushbuttons (BTN_START_RACE and BTN_RESET_TR) are debounced
    * No delay() used to ensure that interrupts are recorded correctly
  * Race Start Sequence rewritten to remove delay(), this will ensure propper interrupt timings
  * Early Start Detection shows a message on LCD
*/

#include <LiquidCrystal.h>
#include <EEPROM.h>
#include "EEPROMAnything.h"

// Starting Light Pins
const int LED_GREEN = 53;
const int LED_YEL = 52;
const int LED_RED = 51;

// Button and Timer Pins
const int BTN_START_RACE = 50;
const int BTN_RESET_TR = 49; // BTN to clear EEPROM
const int TIMER_SLOT_1 = 2; // Pin 2 = Interrupt 0
const int TIMER_SLOT_2 = 3; // Pin 3 = Interrupt 1

// LCD Pin Setup
LiquidCrystal LCD(12, 11, 7, 6, 5, 4);

// Variables for Timer
float LAPTIME_SLOT1 = 99;
float LAPTIME_SLOT2 = 99;
float BESTLAP_SLOT1 = 99;
float BESTLAP_SLOT2 = 99;
float BESTLAP_ALL = 99;
unsigned long START_RACE_TIME = 999999;
unsigned long STORE_OLD_TIME_SLOT1 = 0;
unsigned long STORE_OLD_TIME_SLOT2 = 0;
unsigned long STORE_NEW_TIME_SLOT1 = 0;
unsigned long STORE_NEW_TIME_SLOT2 = 0;

// Variables to save temporary values used by ISRs
volatile unsigned long GET_LAPTIME1_TIME;
volatile unsigned long GET_LAPTIME2_TIME;
volatile boolean PROCESS_TIME1 = false;
volatile boolean PROCESS_TIME2 = false;

// Lapcounter
int LAPCOUNT_SLOT1 = 0;
int LAPCOUNT_SLOT2 = 0;

// MISC Variables
float TR_OLD = 99;  // Variable to store Track Record from EEPROM
boolean EARLY_START = false;

// BTN Debounce Variables
unsigned long BTN_START_DEBOUNCE_TIME = 0;
unsigned long BTN_RESET_DEBOUNCE_TIME = 0;
unsigned long BTN_DEBOUNCE_DELAY = 100;
int BTN_START_CURRENT;
int BTN_START_STATE;
int BTN_START_LAST_STATE = HIGH;
int BTN_RESET_CURRENT;
int BTN_RESET_STATE;
int BTN_RESET_LAST_STATE = HIGH;

// Start Race Sequence Variables
unsigned long START_RACE_CURRENT_MILLIS;
unsigned long START_RACE_PREV_MILLIS = 0;
int RAND = 0;
boolean RACE_STARTED = false;

// Pin Setup
void setup() {
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YEL, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(BTN_START_RACE, INPUT_PULLUP);
  pinMode(BTN_RESET_TR, INPUT_PULLUP);
  pinMode(TIMER_SLOT_1, INPUT);
  pinMode(TIMER_SLOT_2, INPUT);
  attachInterrupt(0, GET_LAPTIME1, FALLING);
  attachInterrupt(1, GET_LAPTIME2, FALLING);
  LCD.begin(20, 4);
  LCD.clear();
  LCD.setCursor(7, 2);
  LCD.print("READY!");
  Serial.begin(9600);  // remove in final code
}

void GET_LAPTIME1 () {
  GET_LAPTIME1_TIME = micros();
  PROCESS_TIME1 = true;
}

void GET_LAPTIME2 () {
  GET_LAPTIME2_TIME = micros();
  PROCESS_TIME2 = true;
}

void loop() {
  // Debounce BTN_START_RACE
  BTN_START_CURRENT = digitalRead(BTN_START_RACE);
  if (BTN_START_CURRENT != BTN_START_LAST_STATE){
    BTN_START_DEBOUNCE_TIME = millis();
  }
  if ((millis() - BTN_START_DEBOUNCE_TIME) > BTN_DEBOUNCE_DELAY) {
    if (BTN_START_CURRENT != BTN_START_STATE) {
      BTN_START_STATE = BTN_START_CURRENT;
      if (BTN_START_STATE == LOW) {
        RACE_STARTED = false; // Set value to false when button is pushed
        EARLY_START = false;
        RAND = random(1000,3000);
        START_RACE_CURRENT_MILLIS = millis();
        START_RACE_PREV_MILLIS = START_RACE_CURRENT_MILLIS;
        LCD.clear();
        LCD.setCursor(8, 2);
        LCD.print("SET!");
        while (RACE_STARTED == false) {
          if (START_RACE_CURRENT_MILLIS >= START_RACE_PREV_MILLIS + RAND && digitalRead(LED_RED) == LOW && digitalRead(LED_YEL) == LOW && digitalRead(LED_GREEN) == LOW) {
            digitalWrite(LED_RED, HIGH);
            START_RACE_PREV_MILLIS = START_RACE_CURRENT_MILLIS;
          }
          if (START_RACE_CURRENT_MILLIS >= START_RACE_PREV_MILLIS + RAND && digitalRead(LED_RED) == HIGH && digitalRead(LED_YEL) == LOW && digitalRead(LED_GREEN) == LOW) {
            digitalWrite(LED_YEL, HIGH);
            START_RACE_PREV_MILLIS = START_RACE_CURRENT_MILLIS;
          }
          if (START_RACE_CURRENT_MILLIS >= START_RACE_PREV_MILLIS + RAND && digitalRead(LED_RED) == HIGH && digitalRead(LED_YEL) == HIGH && digitalRead(LED_GREEN) == LOW) {
            digitalWrite(LED_RED, LOW);
            START_RACE_PREV_MILLIS = START_RACE_CURRENT_MILLIS;
          }
          if (START_RACE_CURRENT_MILLIS >= START_RACE_PREV_MILLIS + RAND && digitalRead(LED_RED) == LOW && digitalRead(LED_YEL) == HIGH && digitalRead(LED_GREEN) == LOW) {
            digitalWrite(LED_YEL, LOW);
            LCD.clear();
            LCD.setCursor(9, 2);
            LCD.print("Go!");
            digitalWrite(LED_GREEN, HIGH);
            START_RACE_PREV_MILLIS = START_RACE_CURRENT_MILLIS;
            LAPCOUNT_SLOT1 = 0;
            LAPCOUNT_SLOT2 = 0;
            START_RACE_TIME = millis();
            LAPTIME_SLOT1 = 99;
            LAPTIME_SLOT2 = 99;
            BESTLAP_SLOT1 = 99;
            BESTLAP_SLOT2 = 99;
            BESTLAP_ALL = 99;
            STORE_OLD_TIME_SLOT1 = 0;
            STORE_OLD_TIME_SLOT2 = 0;
            STORE_NEW_TIME_SLOT1 = 0;
            STORE_NEW_TIME_SLOT2 = 0;
          }
          if (START_RACE_CURRENT_MILLIS >= START_RACE_PREV_MILLIS + RAND && digitalRead(LED_RED) == LOW && digitalRead(LED_YEL) == LOW && digitalRead(LED_GREEN) == HIGH) {
            digitalWrite(LED_GREEN, LOW);
            START_RACE_PREV_MILLIS = START_RACE_CURRENT_MILLIS;
            RACE_STARTED = true;
          }
          START_RACE_CURRENT_MILLIS = millis();
        }
      }
    }
  }
  BTN_START_LAST_STATE = BTN_START_CURRENT;
  
  // Debounce BTN_RESET_TR
  BTN_RESET_CURRENT = digitalRead(BTN_RESET_TR);
  if (BTN_RESET_CURRENT != BTN_RESET_LAST_STATE){
    BTN_RESET_DEBOUNCE_TIME = millis();
  }
  if ((millis() - BTN_RESET_DEBOUNCE_TIME) > BTN_DEBOUNCE_DELAY) {
    if (BTN_RESET_CURRENT != BTN_RESET_STATE) {
      BTN_RESET_STATE = BTN_RESET_CURRENT;
      if (BTN_RESET_STATE == LOW) {
        CLEAR_EEPROM(); // Call function to clear EEPROM
      }
    }
  }
  BTN_RESET_LAST_STATE = BTN_RESET_CURRENT;
  
  if (PROCESS_TIME1 == true) {
    GET_LAPTIME1_TIME = GET_LAPTIME1_TIME / 1000;
    if (GET_LAPTIME1_TIME < START_RACE_TIME) {
      EARLY_START = true;
    }
    else {
      CALCULATE_LAPTIME1();
    }
    PROCESS_TIME1 = false;
  }
  
  if (PROCESS_TIME2 == true) {
    GET_LAPTIME2_TIME = GET_LAPTIME2_TIME / 1000;
    if (GET_LAPTIME2_TIME < START_RACE_TIME) {
      EARLY_START = true;
    }
    else {
      CALCULATE_LAPTIME2();
    }
    PROCESS_TIME2 = false;
  }

  if (EARLY_START == true) {
    // Early Start Detected
    LCD.clear();
    LCD.setCursor(5, 0);
    LCD.print("Early Start");
    LCD.setCursor(6, 1);
    LCD.print("detected");
    LCD.setCursor(2, 3);
    LCD.print("Restart the Race");
  }
}

void CALCULATE_LAPTIME1() {
  STORE_NEW_TIME_SLOT1 = GET_LAPTIME1_TIME;
  if (LAPCOUNT_SLOT1 == 0) {
    STORE_OLD_TIME_SLOT1 = START_RACE_TIME;
  }  
  LAPTIME_SLOT1 = STORE_NEW_TIME_SLOT1 - STORE_OLD_TIME_SLOT1;
  LAPTIME_SLOT1 = LAPTIME_SLOT1 / 1000;  
  STORE_OLD_TIME_SLOT1 = STORE_NEW_TIME_SLOT1;  
  if (LAPTIME_SLOT1 > 1) {
    LAPCOUNT_SLOT1 = LAPCOUNT_SLOT1 + 1;
    if (BESTLAP_SLOT1 > LAPTIME_SLOT1) {
      BESTLAP_SLOT1 = LAPTIME_SLOT1;
    }
    if (BESTLAP_ALL > BESTLAP_SLOT1) {
      BESTLAP_ALL = BESTLAP_SLOT1;
    }
    UPDATE_TR();
    LCDPrint();
  }
}

void CALCULATE_LAPTIME2() {
  STORE_NEW_TIME_SLOT2 = GET_LAPTIME2_TIME;
  if (LAPCOUNT_SLOT2 == 0) {
    STORE_OLD_TIME_SLOT2 = START_RACE_TIME;
  }  
  LAPTIME_SLOT2 = STORE_NEW_TIME_SLOT2 - STORE_OLD_TIME_SLOT2;
  LAPTIME_SLOT2 = LAPTIME_SLOT2 / 1000;  
  STORE_OLD_TIME_SLOT2 = STORE_NEW_TIME_SLOT2;  
  if (LAPTIME_SLOT2 > 1) {
    LAPCOUNT_SLOT2 = LAPCOUNT_SLOT2 + 1;
    if (BESTLAP_SLOT2 > LAPTIME_SLOT2) {
      BESTLAP_SLOT2 = LAPTIME_SLOT2;
    }
    if (BESTLAP_ALL > BESTLAP_SLOT2) {
      BESTLAP_ALL = BESTLAP_SLOT2;
    }
    UPDATE_TR();
    LCDPrint();
  }
}

void LCDPrint() {
  LCD.clear();
  LCD.setCursor(0, 0);
  LCD.print("L: ");
  LCD.print(LAPCOUNT_SLOT1);
  LCD.setCursor(10, 0);
  LCD.print("L: ");
  LCD.print(LAPCOUNT_SLOT2);
  LCD.setCursor(0, 1);
  LCD.print("T: ");
  LCD.print(LAPTIME_SLOT1,3);
  LCD.setCursor(10, 1);
  LCD.print("T: ");
  LCD.print(LAPTIME_SLOT2,3);
  LCD.setCursor(0, 2);
  LCD.print("B: ");
  LCD.print(BESTLAP_SLOT1,3);
  LCD.setCursor(10, 2);
  LCD.print("B: ");
  LCD.print(BESTLAP_SLOT2,3);
  LCD.setCursor(0, 3);
  LCD.print("Track Record: ");
  LCD.print(BESTLAP_ALL,3);  
}

void UPDATE_TR() {
  EEPROM_readAnything(0, TR_OLD); // Read saved data from EEPROM
  if (TR_OLD < 1) {
    TR_OLD = 99;  // if time is lower than 1s, set Record to 99s
  }
  if (BESTLAP_ALL < TR_OLD){
    EEPROM_writeAnything(0, BESTLAP_ALL);  // Write new Track record to EEPROM
  }
  else {
    BESTLAP_ALL = TR_OLD;
  }
}

void CLEAR_EEPROM(){
  LCD.clear();
  LCD.setCursor(4, 1);
  LCD.print("Clearing");
  LCD.setCursor(4, 2);
  LCD.print("EEPROM");
  for (int i = 0 ; i < sizeof(BESTLAP_ALL) ; i++) {
    EEPROM.write(i, 0);
  }
  LCD.print(" done");
}
