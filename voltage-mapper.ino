//*********************************************************
//*********************************************************
//**                                                     **
//**                 Voltage Mapper                      **
//**                                                     **
//**                  Alpha Version                      **
//**                   2014-12-28                        **
//**                                                     **
//**                   Developers:                       **
//**                   JHurd                             **
//**                   Dronus4x4                         **
//**                                                     **
//*********************************************************
//*********************************************************

// =========   PURPOSE:   ==================================
// Read voltage from air flow sensor, apply a bias (voltage offset) for a particular voltage range (load point), then send it to the ECU
// =========================================================

// =========   PROCESS:   ==================================
// Initalization first reads the parameters in the selected profile (0-XX) file from the SD card. Once all parameters are loaded into an
// array variable (bias[] the mapping of voltage will begin.

// =========   REQUIRED FILES:   ============================
// readProfile.ino
// ioInterrupt.ino
// ==========================================================

// -- The magic of F() for strings in Serial.print --
// The F() macro tells the compiler to keep your strings in PROGMEM. This helps free up dynamic memory

// ========= THINGS TO DO =======
// 1. Need to add code to make sure we have valid math (no negative voltage) for example you'd only want a negative bias at loadpoint 255 (5v input)
//    Possibly a good idea to limit the bias value to something reasonable. What would be the maximum voltage difference required?

//  ========    Load Associated libraries   ========
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <TimerOne.h>
#include <LiquidCrystal_I2C.h> // There are a couple versions of examples for the version of I2C board connected to the LCD


// ========= Configurable via (future) Interface ===================
// =================================================================
boolean debug = true; // Enable debugging Serial info
int profileSelect = 0; // Profile number to be used (profileX.txt)
float limitVoltage = 4.2; // Output voltage limit (can not go beyond maxVoltageRAW, any higher will be ignored)
float AREF = 4.97; // The reference voltage that the analog inputs use.(voltage on aref pin). This may differ when we use powering from another supply. If this number is off, the inputVoltage value will be inaccurate. ***** This also represents the maximum voltage that the Arduino will see for an input ********** anaglogReference(INTERNAL) uses the system voltage which can be dragged down as more and more stuff is powered by the uno
// ==================================================================
// ==================================================================


// ========= Pin Configurations =====================================
// This is currently setup for the UNO with no display
//  * SD card attached to SPI bus as follows:
// ** MOSI - pin 11 on Arduino Uno
// ** MISO - pin 12 on Arduino Uno
// ** CLK - pin 13 on Arduino Uno
// ** CS - depends on your SD card shield or module. Pin 3 Used here
const int sdCardPIN = 10; // TimerOne library uses Uno's pins 9, 10 so I changed this to 3 so it wont conflict with the interrupt (but it doesn't appear to conflict at all.. interesting)
const int outputVoltagePIN = 9; // Set arduino pin number to variable
char inputVoltagePIN = A0; // Set arduino pin number to variable
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address, pin configuration (from I2C to LCD). In my case 0x27. I have added I2C Scanner sketch to the library examples to find addresses of I2C connected devices
// ==================================================================


// =======   Global Variables    ====================================
boolean paramLoaded = false; //Set to 1 if config has been loaded
int loadPoint; // Voltage range. This is calculated from the input voltage, then rounded to nearest, hence a float variable.
signed int bias[176]; // A place to store all parameters from the profileX.txt (number of loadpoints plus one)
int idleVoltageRAW1023 = 1023;
int limitVoltageRAW1023;
float inputVoltageREAL; //Real Voltage = (Raw * (AREF / MaxRawValue1023)
int inputVoltageRAW1023;
int outputVoltageRAW1023;
float outputVoltageREAL; //Real Voltage = (Raw * (AREF / MaxRawValue1023)
float maxVoltageREAL = 4.5;
int maxVoltageRAW1023; //Absolute00 maximum output voltage to send out, 4.5v / (5/1023) = 921 (do not chage as load points are based on this number)
float minVoltageREAL = 1.0;
int minVoltageRAW1023 = 205; //Absolute minimum voltage to scale loadpoints from, 1.0v / (5/1023) = 205. Maybe replaced with lowest idle voltage (do not change as load points are boased on this number)
unsigned long scanTimeINT;
char tmpString[10]; // To temperarely store string values for serial display
// ==================================================================


void setup() {
  Serial.begin(9600); // Set baud rate for serial
  pinMode(outputVoltagePIN, OUTPUT); // Set the arduino pin to output

  if (debug) {
    Serial.println(F(""));
    Serial.println(F("=======  DEBUG MODE ENABLED  ======="));
  }

  SPI.setClockDivider(4); // Sets the SPI clock. 4 = one quarter of the system clock or 4Mhz

  Timer1.initialize(500); // set a timer of length 100000 microseconds (or 0.1 sec - or 10Hz)
  Timer1.attachInterrupt( ioInterrupt ); // attach the service routine here

  lcd.begin(20, 4);  // initialize the lcd for 20 chars 4 lines
  lcd.backlight(); // Turn on backlight (lcd.noBacklight(); to turn off)
  
  lcd.setCursor(3, 0);
  lcd.print(F("ZedDee Voltage"));
  lcd.setCursor(6, 1);
  lcd.print(F("Modifier"));
  lcd.setCursor(8, 2);
  lcd.print(F(" "));
  lcd.setCursor(5, 3);
  lcd.print(F("DRIVE SAFE!"));
  
  maxVoltageRAW1023 = maxVoltageREAL/(AREF/1023);
  minVoltageRAW1023 = minVoltageREAL/(AREF/1023);
} 


void loop() {
  analogReference(DEFAULT); //This is the Uno's Default but I figured I would leave this here in case we'd use an EXTERNAL voltage reference
  unsigned long scanStart = millis();
  if (paramLoaded == false) { // Run the function to load profile parameters if it hasn't already
    readProfile(); // Make sure the readProfile.ino file is in the same folder as Voltage_Mapper.ino or you will experience compiling errors
    if (limitVoltage) {  // If limitVoltage value is set
      maxVoltageRAW1023 = (int)round((limitVoltage / (AREF / 1023.0))); // Set a new maxVoltage value to limitVoltage
    }
    lcd.clear();
  } else {
    // This is for serial display purposes
    outputVoltageREAL = outputVoltageRAW1023 * (AREF / 1023.0);
    inputVoltageREAL = inputVoltageRAW1023 * (AREF / 1023.0); // Convert Raw input values, max 1023, to voltage, max 5 volts

    Serial.print(F("LP: "));
    Serial.print(loadPoint);
    Serial.print(F("\t"));

    lcd.setCursor(11, 0); //Start at character, then line
    lcd.print(F("LP: "));
    lcd.print(F("    ")); // Clear the numbers
    lcd.setCursor(15, 0); // Then start again
    lcd.print(loadPoint);

    if (debug) {
      Serial.print(F("LP Window: "));
      Serial.print(dtostrf((maxVoltageRAW1023 - minVoltageRAW1023) * (AREF / 1023.00) / 175.00, 5, 3, tmpString));  // dtostrf() converts float to strings as arduino serial is unable to display beyond two decimal places
      Serial.print(F("V"));
      Serial.print(F("\t"));
    }
    Serial.print(F("Bias: "));
    Serial.print(bias[loadPoint]);
    Serial.print(F("\t"));

    lcd.setCursor(0, 1);
    lcd.print(F("Bias Value: "));
    lcd.print(F("         ")); // Clear the numbers
    lcd.setCursor(12, 1); // Then start again
    lcd.print(bias[loadPoint]);

    if (debug) {

      Serial.print(F("Min Bias Step: "));
      Serial.print(dtostrf(AREF / 1023.0, 5, 4, tmpString));
      Serial.print(F("V"));
      Serial.print(F("\t"));

      Serial.print(F("Bias Offset: "));
      Serial.print(dtostrf(bias[loadPoint] * (AREF / 1023.0), 5, 3, tmpString));
      Serial.print(F("V"));
      Serial.print(F("\t"));
    }

    lcd.setCursor(0, 2);
    lcd.print(F("Bias Off: "));
    lcd.print(F("        "));
    lcd.setCursor(10, 2);
    lcd.print(dtostrf(bias[loadPoint] * (AREF / 1023.0), 5, 3, tmpString));
    lcd.print(F("V"));

    Serial.print(F("IN: "));
    Serial.print(dtostrf(inputVoltageREAL, 5, 3, tmpString));
    Serial.print(F("V"));
    Serial.print(F("\t"));

    lcd.setCursor(0, 0);
    lcd.print(F("IN: "));
    lcd.print(dtostrf(inputVoltageREAL, 5, 3, tmpString));
    lcd.print(F("V"));

    if (debug) {
      Serial.print(F("Diff: "));
      Serial.print(dtostrf(outputVoltageREAL - inputVoltageREAL, 5, 3, tmpString));
      Serial.print(F("V"));
      Serial.print(F("\t"));
    }

    Serial.print(F("OUT: "));
    Serial.print(dtostrf(outputVoltageREAL, 5, 3, tmpString));
    Serial.print(F("V"));

    lcd.setCursor(0, 3);
    lcd.print(F("OUT: "));
    lcd.print(dtostrf(outputVoltageREAL, 5, 3, tmpString));
    lcd.print(F("V"));

      if ((outputVoltageRAW1023 >= maxVoltageRAW1023) || (outputVoltageRAW1023 <= minVoltageRAW1023)) {
      Serial.print(F("(L)"));
      lcd.print(F("(L)"));
    } else {
      lcd.print(F("   ")); //If not limit, overwrite the three characters
    }

    if (debug) {
      Serial.print(F("\t"));
      unsigned long scanEnd = millis();
      unsigned long scanTime = scanEnd - scanStart;
      Serial.print(F("Loop: "));
      Serial.print(scanTime);
      Serial.print(F("ms\t"));

      Serial.print(F("Inter: "));
      Serial.print(scanTimeINT);
      Serial.print(F("\xb5s")); // this produces the micro symbol on the LCD display
    }
    Serial.println(F(""));
  }
}
