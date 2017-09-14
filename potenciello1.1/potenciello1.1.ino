/////////////////////////////////
// POTENCIELLO 1.1
// Created: December 8, 2014
// Updated: April 4, 2017
// Creator: Pong Trairatvorakul
/////////////////////////////////

//== IMPORT RELEVANT LIBRARIES ===========================

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <LiquidCrystal.h>
#include <Encoder.h>

//== Generate Synthesis Modules & Connections ============
AudioSynthWaveformDc     ampEnv;            //xy=182,301
AudioSynthWaveformDc     ampEnv2;            //xy=182,301
AudioSynthWaveformDc     ampEnv0;            //xy=182,301
AudioSynthWaveform       waveform1;      //xy=189,217
AudioSynthWaveform       waveform2;      //xy=189,217
AudioEffectMultiply      multiply1;      //xy=388,296
AudioEffectMultiply      multiply2;      //xy=388,296
AudioEffectMultiply      multiply0;      //xy=388,296
AudioSynthWaveformDc     filtEnv;            //xy=394,388
AudioFilterStateVariable filter1;        //xy=561,341
AudioSynthWaveformDc     filtEnv2;            //xy=394,388
AudioFilterStateVariable filter2;        //xy=561,341
AudioSynthWaveformDc     filtEnv0;            //xy=394,388
AudioFilterStateVariable filter0;        //xy=561,341
AudioSynthWaveformDc     filtEnv3;            //xy=394,388
AudioFilterStateVariable filter3;        //xy=561,341
AudioSynthWaveformDc     filtEnv4;            //xy=394,388
AudioFilterStateVariable filter4;        //xy=561,341
AudioOutputAnalog        dac1;           //xy=764,330
AudioMixer4              mixer;
//AudioConnection          patchCord1(ampEnv, 0, multiply1, 1);
AudioConnection          patchCord2(waveform1, 0, multiply1, 0);
AudioConnection          patchCord3(multiply1, 0, filter1, 0);
AudioConnection          patchCord4(filtEnv, 0, filter1, 1);

//AudioConnection          patchCord5(ampEnv2, 0, multiply2, 1);
AudioConnection          patchCord6(waveform2, 0, multiply2, 0);
AudioConnection          patchCord7(multiply2, 0, filter2, 0);
AudioConnection          patchCord8(filtEnv2, 0, filter2, 1);

AudioConnection          patchCord9(waveform1, 0, mixer, 0);
AudioConnection          patchCord10(waveform2, 0, mixer, 1);

AudioConnection          patchCord14(ampEnv0, 0, multiply0, 1);
AudioConnection          patchCord15(mixer, 0, multiply0, 0);
AudioConnection          patchCord11(multiply0, 0, filter0, 0);
AudioConnection          patchCord13(filtEnv0, 0, filter0, 1);
//AudioConnection          patchCord18(filter0, dac1);

//filter final signal (hi-pass)
AudioConnection          patchCord16(filter0, 0, filter3, 0);
AudioConnection          patchCord17(filtEnv3, 0, filter3, 1);
AudioConnection          patchCord18(filter3, 2, dac1, 0);


//== Declare Globals ==========================================
// for smoothing
#define BUFFSIZE 10       // size of buffer to take average of
int motrBuff[BUFFSIZE];   // buffer for motor
int acclBuff[BUFFSIZE];   // buffer for accelerometer
int motrSum = 0;          // sum of motor inputs
int acclSum = 0;          // sum of accelerometer inputs

int i, j;              // loop counters or demo

// for use in scale
volatile boolean fromScale;
volatile int lastScaleEncoder;
volatile boolean firstTimeInScale = true;

// for notes
volatile int base = 57;          // lowest note
volatile boolean fromOpenString; // true if last mode was open string
volatile int lastStringEncoder;  // value from rotary encoder
volatile boolean firstTimeOpenString = true;

volatile float refPitch = 440.;  // reference 'A' pitch
volatile boolean firstTimeFinetune = true; // true if first time in finetuning
volatile int lastFinetuneEncoder; // store last finetune value
volatile boolean fromFinetune;    // flag for leaving finetuning mode

volatile boolean firstTimeWaveform = true; // choose waveform mode
volatile boolean fromWaveform;             // flag for leave waveform mode
volatile int lastWaveformEncoder;          // store last waveform

// parameters in the scroll menu
String parameters[] = {
  "OPEN STRING     ",
  "SCALE           ",
  "FINETUNE        ",
  "WAVEFORM        ",
};


// different scales
int musicalScale[4][16] = {
  {base, base + 2, base + 4, base + 5, base + 7, base + 9, base + 11, base + 12, base + 14, base + 16, base + 17, base + 19, base + 21, base + 23, base + 24},
  {base, base + 2, base + 3, base + 5, base + 7, base + 8, base + 11, base + 12, base + 14, base + 15, base + 17, base + 19, base + 20, base + 23, base + 24},
  {base, base + 2, base + 3, base + 5, base + 7, base + 9, base + 11, base + 12, base + 14, base + 15, base + 17, base + 19, base + 21, base + 23, base + 24},
  {base, base + 2, base + 3, base + 5, base + 7, base + 8, base + 10, base + 12, base + 14, base + 15, base + 17, base + 19, base + 20, base + 22, base + 24},
};
// .. and their respective names
String scaleName[] = {
  "Major           ",
  "Harmonic min    ",
  "Melodic min     ",
  "Natural min     ",
  "Free            "
};
int scaleIndex = (sizeof(musicalScale) / sizeof(int) / 16);

// octave multipliers
float octave[] = {0.5, 1., 2.};

float touch_maximum = 0;


//LCD Initialize
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

//Lower Encoder Initialize
int encoderPin1 = 6;
int encoderPin2 = 7;
volatile int lastEncoded = 0;
volatile float encoderValue = 0;
long lastencoderValue = 0;
int lastMSB = 0;
int lastLSB = 0;

//Upper Encoder Initialize
int encoder2Pin1 = 8;
int encoder2Pin2 = 9;
volatile int lastEncoded2 = 0;
volatile int encoderValue2 = 0;
long lastencoderValue2 = 0;
int lastMSB2 = 0;
int lastLSB2 = 0;

void setup() {
  // set all vals in buffer to 0
  for (int i = 0; i < BUFFSIZE; i++) {
    motrBuff[i] = 0;
    acclBuff[i] = 0;
  }
  
  analogWriteResolution(12); // set the DAC to output 12bit audio

  // set waveform for upper string
  waveform1.begin(WAVEFORM_SAWTOOTH);
  waveform1.frequency(refPitch);
  waveform1.amplitude(1.0);

  // set amplitude for upper string
  ampEnv.amplitude(1.0);
  filtEnv.amplitude(0.0);
  filter1.frequency(100);
  filter1.octaveControl(5);

  // set waveform for upper string
  waveform2.begin(WAVEFORM_SAWTOOTH);
  waveform2.frequency(293.333);
  waveform2.amplitude(1.0);

  // set amplitude for lower stirng
  ampEnv2.amplitude(1.0);
  filtEnv2.amplitude(0.0);
  filter2.frequency(10000);
  filter2.octaveControl(5);

  // set overall amplitude
  ampEnv0.amplitude(1.0);
  filtEnv0.amplitude(0.0);
  filter0.frequency(10000);
  filter0.octaveControl(5);

  // set filters
  filtEnv3.amplitude(1.0);
  filter3.frequency(50);
  filter3.octaveControl(1);

  filtEnv4.amplitude(1.0);
  filter4.frequency(10000);
  filter4.octaveControl(1);

  // set mixer default
  mixer.gain(0, 0.4);
  mixer.gain(1, 0.4);

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Startup Screen
  lcd.setCursor(1, 0);
  lcd.print("POTENCIELLO");
  lcd.setCursor(5, 1);
  lcd.print("by Pong Tr.");
  delay(1000);
  lcd.clear();

  //Lower Encoder Setup
  pinMode(encoderPin1, INPUT);
  pinMode(encoderPin2, INPUT);

  digitalWrite(encoderPin1, HIGH); //turn pullup resistor on
  digitalWrite(encoderPin2, HIGH); //turn pullup resistor on

  //call updateEncoder() when any high/low changed seen
  //on interrupt 0 (pin 2), or interrupt 1 (pin 3)
  attachInterrupt(6, updateEncoder, CHANGE);
  attachInterrupt(7, updateEncoder, CHANGE);

  //Upper Encoder Setup
  pinMode(encoder2Pin1, INPUT);
  pinMode(encoder2Pin2, INPUT);

  digitalWrite(encoder2Pin1, HIGH); //turn pullup resistor on
  digitalWrite(encoder2Pin2, HIGH); //turn pullup resistor on

  //call updateEncoder() when any high/low changed seen
  attachInterrupt(8, updateEncoder2, CHANGE);
  attachInterrupt(9, updateEncoder2, CHANGE);

  // you have to allocate a certain number of memory blocks for the audio library to function - 20 is a guess...
  AudioMemory(20);

}

void loop() {
  //Choose parameter
  int parameterIndex = encoderValue2 / 10 % (sizeof(parameters) / sizeof(int) / 4);
  String activeParameter = parameters[parameterIndex];
  lcd.setCursor(0, 0);
  lcd.print(activeParameter);

  //  Choose a scale
  if (activeParameter == "SCALE           ") {
    if (firstTimeInScale) {
      encoderValue = 10 * (sizeof(musicalScale) / sizeof(int) / 16);
      firstTimeInScale = false;
    }
    else if (fromScale == false) {
      encoderValue = lastScaleEncoder;
    }
    scaleIndex = (int)encoderValue / 10 % (sizeof(musicalScale) / sizeof(int) / 16 + 1);
    fromScale = true;
    //Print on screen
    lcd.setCursor(0, 1);
    lcd.print(scaleName[scaleIndex]);
    lcd.setCursor(15, 0);
    lcd.print(scaleIndex);
  }

  //  Choose a waveform
  if (activeParameter == "WAVEFORM        ") {
    String waveform;
    if (firstTimeWaveform) {
      encoderValue = 0;
      firstTimeWaveform = false;
    }
    else if (fromWaveform == false) {
      encoderValue = lastWaveformEncoder;
    }
    int waveIndex = (int)encoderValue / 10 % 4;
    if (waveIndex == 0) {
      waveform1.begin(WAVEFORM_SAWTOOTH);
      waveform = "Sawtooth    ";
    }
    if (waveIndex == 1) {
      waveform1.begin(WAVEFORM_SINE);
      waveform = "Sine            ";
    }
    if (waveIndex == 2) {
      waveform1.begin(WAVEFORM_SQUARE);
      waveform = "Square          ";
    }
    if (waveIndex == 3) {
      waveform1.begin(WAVEFORM_TRIANGLE);
      waveform = "Triangle        ";
    }
    fromWaveform = true;
    //Print on screen
    lcd.setCursor(0, 1);
    lcd.print(waveform);
  }

  //Choose Open String Note
  else if (activeParameter == "OPEN STRING     ") {
    lcd.setCursor(0, 1);
    lcd.print("MIDI NOTE:  ");
    lcd.setCursor(12, 1);
    lcd.print(base);
    if (firstTimeOpenString) {
      encoderValue = base * 10;
      firstTimeOpenString = false;
    }
    else if (fromOpenString == false) {
      encoderValue = lastStringEncoder;
    }
    base = encoderValue / 10;
    fromOpenString = true;
    lcd.setCursor(12, 1);
    lcd.print(base);
    lcd.print("    ");
  }

  //Finetune
  else if (activeParameter == "FINETUNE        ") {
    lcd.setCursor(0, 1);
    lcd.print("A4:  ");
    lcd.setCursor(5, 1);
    lcd.print(refPitch);
    lcd.print("Hz    ");
    if (firstTimeFinetune) {
      encoderValue = refPitch * 10;
      firstTimeFinetune = false;
    }
    else if (fromFinetune == false) {
      encoderValue = lastFinetuneEncoder;
    }
    refPitch = encoderValue / 10.;
    fromFinetune = true;
    lcd.setCursor(5, 1);
    lcd.print(refPitch);
    lcd.print("Hz    ");
  }

  // Calculate frequency for upper string
  float freq = freq;
  float in1 = analogRead(A0);
  in1 = in1 / 1023.; // normalize to 0.0 -> 1.0

  if (scaleIndex == (sizeof(musicalScale) / sizeof(int) / 16)) {
    float baseFreq = refPitch * pow(2, ((base - 69.66) / 12.));
    freq = baseFreq + in1 * baseFreq * 4;
  }
  else {
    int index = in1 * (sizeof(musicalScale[0]) / sizeof(int)); // map to the index values of your pitch array (0 to Array Length - 1)
    int pitch = musicalScale[scaleIndex][index]; // get the pitch number from the array
    freq = refPitch * pow(2, ((pitch - 69.) / 12.)); // calculate the frequency in Hz of the pitch
  }

  waveform1.frequency(freq);
  //  filter1.frequency(2*freq);

  //Calculate frequency for lower string
  float freq2 = freq;
  float in2 = analogRead(A1);
  in2 = in2 / 1023.; // normalize to 0.0 -> 1.0

  float baseFreq2 = refPitch * pow(2, ((base - 69.66) / 12.)) * (2. / 3.);
  freq2 = baseFreq2 + in2 * baseFreq2 * 4;
  waveform2.frequency(freq2);

  //map analog A2 (motor) to amplitude
  int in3 = analogRead(A2);
  float amp = smooth(in3, motrBuff, &motrSum);
  amp = amp / 500;
  ampEnv0.amplitude(amp, 100);

  //map analog A3 to filter cutoff frequency
  float in5 = analogRead(A3);
  //in2 = in2 / 1023.; // normalize to 0.0 -> 1.0
  //  filtEnv.amplitude(in2);
  in5 = (in5 / 51.4 + 2) * freq;
  if (in5 > 10000) {
    in5 = 10000;
    filter0.octaveControl(1);
  }
  filter0.frequency(in5);

  //map analog A4 (accelerometer) to octave
  int in4 = analogRead(A4);
  int accl = smooth(in4, acclBuff, &acclSum); 
  if (accl > 550) {
    mixer.gain(1, 0.6);
    mixer.gain(0, 0.);
  } else {
//  else if (in4 < 500) {
    mixer.gain(0, 0.6);
    mixer.gain(1, 0.);
  }
//  else {
//    mixer.gain(0, 0.4);
//    mixer.gain(1, 0.4);
//  }
  //  in4 = in4/1023;
  //  int octInd = in4 * (sizeof(octave)/sizeof(int) - 1);
  //  freq = freq * octave[octInd];


  //Send to Joystick
  Joystick.X(analogRead(A0));
  Joystick.Y(analogRead(A2));
  Joystick.Z(analogRead(A1));
  Joystick.Zrotate(analogRead(A3));
  //  Joystick.sliderLeft(analogRead(4));
  //  Joystick.sliderRight(analogRead(5));

  //Serial.printf("pot1: %3d\tpot2: %3d\tmotor: %3d\tpressure: %3d\taccel: %3d\n", analogRead(A0), analogRead(A1), analogRead(A2), analogRead(A3), analogRead(A4));
  Serial.printf("freq1: %.3f\tfreq2: %.3f\tamp: %.3f\tpress: %.3f\taccl: %d\n", freq, freq2, amp, in5, accl);
  delay(1);

}

void updateEncoder() { //Lower
  int MSB = digitalRead(encoderPin1); //MSB = most significant bit
  int LSB = digitalRead(encoderPin2); //LSB = least significant bit

  int encoded = (MSB << 1) | LSB; //converting the 2 pin value to single number
  int sum  = (lastEncoded << 2) | encoded; //adding it to the previous encoded value

  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) encoderValue ++;
  if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) encoderValue --;

  lastEncoded = encoded; //store this value for next time
}

void updateEncoder2() { //Upper
  int MSB2 = digitalRead(encoder2Pin1); //MSB = most significant bit
  int LSB2 = digitalRead(encoder2Pin2); //LSB = least significant bit

  int encoded2 = (MSB2 << 1) | LSB2; //converting the 2 pin value to single number
  int sum2  = (lastEncoded2 << 2) | encoded2; //adding it to the previous encoded value

  if (sum2 == 0b1101 || sum2 == 0b0100 || sum2 == 0b0010 || sum2 == 0b1011) encoderValue2 ++;
  if (sum2 == 0b1110 || sum2 == 0b0111 || sum2 == 0b0001 || sum2 == 0b1000) encoderValue2 --;

  if (fromScale) {
    lastScaleEncoder = encoderValue;
    fromScale = false;
  }
  if (fromOpenString) {
    lastStringEncoder = encoderValue;
    fromOpenString = false;
  }
  if (fromFinetune) {
    lastFinetuneEncoder = encoderValue;
    fromFinetune = false;
  }
  if (fromWaveform) {
    lastWaveformEncoder = encoderValue;
    fromWaveform = false;
  }

  lastEncoded2 = encoded2; //store this value for next time
}

//Smooth
int smooth(int data, int buff[], int *sum) {
  int first = buff[0];
  *sum = *sum - first + data;
  for (int i = 0; i < BUFFSIZE - 1; i++) 
    buff[i] = buff[i+1];
  buff[BUFFSIZE - 1] = data;
  int smoothedVal = *sum/BUFFSIZE;

  return smoothedVal;
}
