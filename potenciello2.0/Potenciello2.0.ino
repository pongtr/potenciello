/////////////////////////////////
// POTENCIELLO 2.0
// Created: March 8, 2017 7:30pm
// Creator: Pong Trairatvorakul
/////////////////////////////////

//================ DEBUG ============================
#define DEBUG true
float maxMotr = 0.50;
float maxAmp = 0.00;

//================ IMPORT LIBRARIES =================
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <LiquidCrystal.h>

//================ IO DEFINITIONS ===================

// INPUTS
#define POT1 A0
#define POT2 A1
#define MOTR A2
#define PRES A3
#define ACCL A4

// Encoders
#define EN1_P1 6
#define EN1_P2 7
#define EN2_P1 8
#define EN2_P2 9

// OUTPUTS
// LCD
#define LCD_RS  12
#define LCD_EN  11
#define LCD_D4  5
#define LCD_D5  4
#define LCD_D6  3
#define LCD_D7  2
// initialize library with interface pins
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D4);

//================ AUDIO CONNECTIONS ===================
// Components
AudioSynthWaveform       W1;
AudioSynthWaveform       W2;
AudioFilterStateVariable F1;
AudioFilterStateVariable F2;
AudioMixer4              MX;
AudioSynthWaveformDc     AM;
AudioEffectMultiply      MUL;
AudioOutputAnalog        DAC;

// Connections
AudioConnection   patchCord0(W1, 0, F1,  0);
AudioConnection   patchCord1(W2, 0, F2,  0);
AudioConnection   patchCord2(F1, 0, MX,  0);
AudioConnection   patchCord3(F2, 0, MX,  1);
AudioConnection   patchCord4(MX, 0, MUL, 0);
AudioConnection   patchCord5(AM, 0, MUL, 1);
AudioConnection   patchCord6(MUL, 0, DAC, 0);

//================ GLOBAL VARIABLES ===================
// sensor values
float pot1, pot2, motr, pres, accl;

// sound values
float refFreq = 440.0;  // reference frequency
float f1, f2;           // freqs of two stirngs
float bf1, bf2;         // freqs of open strings
float amp;              // overall amplitude
float fc1, fc2;         // cutoff frequencies for the two different filters

//================ SETUP ===================
void sensor_setup() {    // set up music sensors
  pinMode(POT1, INPUT);
  pinMode(POT2, INPUT);
  pinMode(MOTR, INPUT);
  pinMode(PRES, INPUT);
  pinMode(ACCL, INPUT);
}

void encoder_setup() {   // set up rotary encoders
  // encoder 1
  pinMode(EN1_P1, INPUT);
  pinMode(EN1_P2, INPUT);
  digitalWrite(EN1_P1, HIGH);
  digitalWrite(EN1_P2, HIGH);
  //attachInterrupt(EN1_P1, updateEncoder1, CHANGE);
  //attachInterrupt(EN1_P2, updateEncoder1, CHANGE);

  // encoder 2
  pinMode(EN2_P1, INPUT);
  pinMode(EN2_P2, INPUT);
  digitalWrite(EN2_P1, HIGH);
  digitalWrite(EN2_P2, HIGH);
  //attachInterrupt(EN2_P1, updateEncoder2, CHANGE);
  //attachInterrupt(EN2_P2, updateEncoder2, CHANGE);
}

void lcd_setup() {       // set up LCD and display splash
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Startup Screen
  lcd.setCursor(1, 0);
  lcd.print("POTENCIELLO 2.0");
  lcd.setCursor(5, 1);
  lcd.print("by Pong Tr.");
  lcd.clear();
}

void serial_setup() {
  Serial.begin(9600);
  Serial.println("POTENCIELLO 2.0\n   by Pong Tr.\n"); 
}

void audio_setup() {
  bf1 = refFreq;
  bf2 = bf1 * 2.0 / 3.0;
  f1 = bf1; f2 = bf2;
  
  // waveform 1
  W1.begin(WAVEFORM_SAWTOOTH);
  W1.frequency(f1);
  W1.amplitude(0.5);

  // waveform 2
  W2.begin(WAVEFORM_SAWTOOTH);
  W2.frequency(f2);
  W2.amplitude(0.5);

  // filters
  F1.frequency(500); // placeholder value
  F2.frequency(340); // placeholder value

  // amplitude envelope
  AM.amplitude(0.0);

  // allocate 20 blocks of memory for audio
  AudioMemory(20);
}

void setup() {
  lcd_setup();
  sensor_setup();
  encoder_setup();
  serial_setup();
  audio_setup();
}

//================ READ SENSORS ===================
// read, store, print sensor vals
void read_vals() {
  // read values and scale
  pot1 = max(0, analogRead(POT1) - 20) / 1023.;
  pot2 = max(0, analogRead(POT2) - 20) /1023.;
  motr = max(0, analogRead(MOTR) -  5) /1023.;
  if (motr > maxMotr) maxMotr = motr;
  motr /= maxMotr;
  pres = analogRead(PRES)/1023.;
  accl = analogRead(ACCL)/1023.;
  // print to help debug
  //if (motr > maxMotr) Serial.println(maxMotr = motr);
  //if (DEBUG) Serial.printf("pot1: %.3f\tpot2: %.3f\tmotr: %.3f\tpres: %.3f\taccl: %.3f\n", pot1, pot2, motr, pres, accl);
  //Serial.printf("Amp: %.3f\n", amp);
}

//================ UPDATE SOUND ===================
// use sensor vals to update audio parameters
void update_sound() {
  // W1 frequency
  f1 = bf1 + pot1 * bf1 * 4;
  W1.frequency(f1);
  
  // W2 frequency
  f2 = bf2 + pot2 * bf2 * 4;
  W2.frequency(f2);

  // Amplitude envelope
  amp = 0.8 * motr;
  if (amp > maxAmp) Serial.printf("Amp: %.3f\n", maxAmp = amp);
  AM.amplitude(amp, 100);

  // F1 cutoff freq
  fc1 = f1 * 3. * pres + f1 * 1.10;
  F1.frequency(fc1);

  // F2 cutoff freq
  fc2 = f2 * 3. * pres + f2 * 1.10;
  F2.frequency(fc2);

  // Mixer proportion from each string
  // NEEDS CALIBRATION
  if (accl > 0.650) {
    MX.gain(1, 0.6);
    MX.gain(0, 0.0);
  } else if (accl < 0.550) {
    MX.gain(0, 0.6);
    MX.gain(1, 0.0);
  } else {
    MX.gain(0, 0.4);
    MX.gain(1, 0.4);
  }
}

//================ LOOP ===================
void loop() {
  read_vals();
  update_sound();
  
}
