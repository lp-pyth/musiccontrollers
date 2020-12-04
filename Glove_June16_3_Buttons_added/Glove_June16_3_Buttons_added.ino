#include <Bounce.h>

#include <autotune.h>
#include <microsmooth.h>

#include <MIDI.h>
#include <EEPROM.h>
#include <Wire.h>

#include "SmoothPin.h"

#define NUM 3
#define MIDI_CHAN 1
#define DEBUG 1

#define CTRL_TRACKSTART 104       //start of ctrl values for track on/off buttons (number up in forloop) FOOTPEDAL VALUES
unsigned long millisTime = 0;
unsigned long playTime = 0;

// control number offsets
#define MODE0_CONTROL 0
#define MODE1_CONTROL MODE0_CONTROL + 20
#define MODE2_CONTROL MODE1_CONTROL + 1

#define BLUE1 10
#define GREEN1 6
#define RED1 9
#define BLUE2 5
#define GREEN2 3
#define RED2 4
int ledTime = 0;
unsigned long  redsmashTime = 0;
unsigned long  redcounterTime = 0;
unsigned long  whitesmashTime = 0;
unsigned long  whitecounterTime = 0;
int whitesmashled = 255;
int redsmashled = 255;
bool redSmash = false;
bool whiteSmash = false;

//sensor values
#define FINGER_N 8
int16_t accel_x, accel_y, accel_z;
SmoothPin finger_pins[FINGER_N];
int fingers[FINGER_N];
int ir; //TODO
#define DIFF_THRESH 270
int bendPins[] = {A14, A0, A1, A2, A3, /*A6,*/ A7, A8}; // the pins for each of the finger resistors

uint16_t fingerMin[FINGER_N];
uint16_t fingerMax[FINGER_N];
// the eeprom address for the calibration
#define CALIB_ADDR 16


/*
 * Button setup: define pins for all of the action buttons
 * buttons are pins 6, 7, 10, 11
 */
#define FINGERPIN2 2
#define FINGERPIN1 1
#define BUTTON3 11
#define BUTTON4 12
#define BUTTONA 8
#define BUTTONB 13
#define BUTTONC 7
Bounce XYLockButton = Bounce(BUTTON3, 10);
Bounce ModeButton = Bounce(FINGERPIN1, 10);
Bounce CalibButton = Bounce(BUTTON4, 10);
Bounce MuteButton = Bounce(FINGERPIN2, 10);

Bounce AButton = Bounce(BUTTONA, 10);
Bounce BButton = Bounce(BUTTONB, 10);
Bounce CButton = Bounce(BUTTONC, 10);

bool AButtonState = false;
bool BButtonState = false;
bool CButtonState = false;
bool ABButtonState = false;
bool BCButtonState = false;
bool ACButtonState = false;
bool ABCButtonState = false;

uint8_t xy_lock = 0;  // Store whether or not we're in XY lock.
// This is so we can connect with Ableton
// 0 = send both, 1 = send X, 2 = send y

//accel init
#define accel_module (0x53)    //ACCELEROMETER SETUP
byte values [6];
char output [512];

//vibration motor
const int motorPin = 23;

//Which mode we are in: 0 is gesture
//mode 1 is accel -> note
#define MODES 3
int mode = 0;

bool in_calibration_mode = false;
int calib_on_time = -1; // You have to hold down the calibration buton. Track that.
bool muted = false;

/*
 * current note
 */
int current_note = -1;


// timing details: we want to send midi at a 100hz clock
// that means every 10 ms
int lastMidiSend = 0;
#define MIDI_INTERVAL 20

/*
 * Arpeggiator!
 * The number of midi clock ticks we've received. There
 * are 24 per quarter note.
 */
int midi_clock = 1;
bool send_arpegg = false;
uint8_t arpegg_interval = 0; // number of ticks between on and off (and on, and off...)

//unsigned long ctrlmidiTime = 0;                                           //CTRL MIDI BUFFER
//#define CTRLMIDI_TIME 50


void setup() {
  Serial.begin(9600);
  // put your setup code here, to run once
  initFingers();
  initAccel();
  initButtons();

  usbMIDI.setHandleRealTimeSystem(handleMidiClock);

  readCalibration();

  pinMode(motorPin, OUTPUT);

  pinMode(RED1, OUTPUT);
  pinMode(GREEN1, OUTPUT);
  pinMode(BLUE1, OUTPUT);
  pinMode(RED2, OUTPUT);
  pinMode(GREEN2, OUTPUT);
  pinMode(BLUE2, OUTPUT);

  mode = EEPROM.read(1);

  Serial.println("Init done");

}

void loop() {

  // Read all sensors
  readAccel();
  readFingers();
  readButtons();
  //Serial.println("loop!");

  if (AButton.fallingEdge()) {AButtonState = true;}  
  if (BButton.fallingEdge()) {BButtonState = true;}  
  if (CButton.fallingEdge()) {CButtonState = true;}    
  if (AButton.risingEdge()) {AButtonState = false;}
  if (BButton.risingEdge()) {BButtonState = false;}  
  if (CButton.risingEdge()) {CButtonState = false;} 

  if (AButtonState == true && BButtonState == true) {ABButtonState == true; Serial.println("ABBUTTONSTATE TRUE");} 
  if (BButtonState == true && CButtonState == true) {BCButtonState == true; Serial.println("BCBUTTONSTATE TRUE");}
  if (AButtonState == true && CButtonState == true) {ACButtonState == true; Serial.println("ACBUTTONSTATE TRUE");}
  if (AButtonState == true && BButtonState == true && CButtonState == true) {ABCButtonState == true; Serial.println("ABCBUTTONSTATE TRUE");}
  if (AButtonState == false && BButtonState == false) {ABButtonState == false;}
  if (BButtonState == false && CButtonState == false) {BCButtonState == false;}
  if (AButtonState == false && CButtonState == false) {ACButtonState == false;}
  if (AButtonState == false && BButtonState == false && CButtonState == false) {ABCButtonState == false;}

  millisTime = millis();

  if (millisTime-ledTime>10 &&  muted==false && mode==0 && whiteSmash==false) {  //refresh leds every 10ms to make the flicker
    int ledval = map(accel_y, 0, 550, 100, 200);
    if (ledval >255) ledval = 255;
    analogWrite(BLUE1, ledval);
    analogWrite(BLUE2, ledval);
    ledval = map(accel_x, 0, 200, 100, 255);
    if (ledval>255) ledval = 255;
    ledval=ledval/8;
    analogWrite(RED1, ledval);
    analogWrite(RED2, ledval); 
    ledTime = millisTime;
  } 

  /*Serial.print ("Accel X: ");
  Serial.print (accel_x);
  Serial.print (" Accel Y: ");
  Serial.print (accel_y);
  Serial.print (" Accel Z: ");  
  Serial.println (accel_z);
  */
  //WHITE SMASH sends a midi note when arm is jerked on MIDI_CHAN 3 on percussion
  /*
   * still add noeoff in case of button switch
   */
  if (accel_x > 500 && millisTime - whitecounterTime > 40) {
    usbMIDI.sendNoteOn(35, 127, 3);
    whitecounterTime = millisTime;
    Serial.println ("WHITESMASH");
    whiteSmash=true; 
    
  }
  if (whiteSmash==true){
    if (millisTime - whitesmashTime > 2){
      digitalWrite(motorPin, HIGH);
      analogWrite(BLUE1, whitesmashled);
      analogWrite(BLUE2, whitesmashled);
      //whitesmashled=whitesmashled-5;
      analogWrite(GREEN1, whitesmashled);
      analogWrite(GREEN2, whitesmashled); 
      //whitesmashled=whitesmashled-10;
      analogWrite(RED1, whitesmashled);
      analogWrite(RED2, whitesmashled); 
      whitesmashled=whitesmashled-1;             //decrease ledval by 1 each 2ms
      if (whitesmashled<5) {
        whiteSmash=false;
        usbMIDI.sendNoteOff(35, 127, 3);
        digitalWrite(motorPin, LOW);
        whitesmashled=255;
        clearLeds();
      }
      whitesmashTime=millisTime;
    }  
  }
  
  //handgestures plus accel spike to end footpedal messages
  if (accel_y > 450 && millisTime - playTime > 100) {
    playTime = millisTime;
    if (gesture(fingers,  (const int[])  { 111, 480, 545, 487, 446, 477, 478, 0,} )) {             //FIST -start
      redSmash=true; 
      Serial.println ("REDSMASH TRACKS ON");
      for (int u = 0; u < 12; u++) {                                                                    //ALL LOOP TRACKS ON
        usbMIDI.sendControlChange(CTRL_TRACKSTART + u, 127, 5);                                         //we send a 127 track value to all tracks
        Serial.println(String(" ch= ") + 5 + (", Control = ") + (CTRL_TRACKSTART + u) + (" Velocity = 127 "));
        Serial.println("All Tracks On");
      }
    }
    else if (AButtonState == true && ABButtonState == false && ACButtonState == false && ABCButtonState == false) {   //stop
      redSmash=true; 
      Serial.println ("REDSMASH TRACKS OFF");
      for (int u = 0; u < 12; u++) {                                                                  //ALL LOOP TRACKS OFF
        usbMIDI.sendControlChange(CTRL_TRACKSTART + u, 0, 5);                                         //we send a 0 track value to all tracks except the pressed one
        Serial.println(String(" ch= ") + 5 + (", Control = ") + (CTRL_TRACKSTART + u) + (" Velocity = 0 "));
        Serial.println("All Tracks Off");
      }
    }
      /*for(int i=0; i<12; i++){                                                //maybe use with a different handpos
         usbMIDI.sendControlChange(CTRL_STOPSTART+i, 127, 5);                  //we send stop messages
        }*/
  }
    
  if (redSmash==true){
    if (millisTime - redsmashTime > 1){
      digitalWrite(motorPin, HIGH);
      analogWrite(BLUE1, 35);
      analogWrite(BLUE2, 35);
      analogWrite(GREEN1, 0);
      analogWrite(GREEN2, 0); 
      analogWrite(RED1, redsmashled);
      analogWrite(RED2, redsmashled); 
      redsmashled=redsmashled-1;
      if (redsmashled<5) {
        redSmash=false;
        digitalWrite(motorPin, LOW);
        redsmashled=255;
        clearLeds();
      }
      redsmashTime=millisTime;
    }  
  }
  
  if (ModeButton.fallingEdge()) {
    mode++;
    mode %= MODES;
    clearLeds();
    Serial.print("mode switch ");
    Serial.println(mode);
    /*
     * We've switched modes - clean some stuff up.
     */
    xy_lock = 0;
    clearOutput();

    //Save the current mode to EEPROM
    EEPROM.write(1, mode);
  }

  
   // Calibration: check to see if button is pushed  
  if (!in_calibration_mode && CalibButton.fallingEdge()) {
    Serial.println("Calib?");
    calib_on_time = millis();
  } else if (CalibButton.risingEdge()) {
    calib_on_time = -1;
  }
  if (!in_calibration_mode && calib_on_time > -1 && millis() - calib_on_time > 3000) {
    Serial.println("enter calib");
    clearOutput();
    in_calibration_mode = true;
    resetCalibration();
  }
  if (in_calibration_mode) {
    Serial.println("In Calibration");
    if (CalibButton.fallingEdge()) {
      in_calibration_mode = false;
      writeCalibration();
      return;
    }
    updateCalibration();
    return;
  }

  //Check for mute mode
  if (MuteButton.fallingEdge()) {
    muted = !muted;
    if (muted) {
      clearOutput();
      clearLeds();
    }
  }

  //Do MIDI communication
  int t = millis();
  if (t - lastMidiSend >= MIDI_INTERVAL) {
    if (!muted) {
      sendOutput();
    }
    lastMidiSend = t;
  }

  // clean up: clear midi buffer
  while (usbMIDI.read()) ; // read and discard any incoming MIDI messages
}

void clearOutput() {
  send_arpegg = false;
  if (current_note != -1) {
    usbMIDI.sendNoteOff(current_note, 99, MIDI_CHAN);
    current_note = -1;
  }

  /*
  analogWrite(BLUE1, 0);
  analogWrite(BLUE2, 0);
  analogWrite(RED1, 0);
  analogWrite(RED2, 0);
  analogWrite(GREEN1, 0);
  analogWrite(GREEN2, 0);
  */
}

int gestureDebounce = 0;
int lastxcontrol = -1;
int lastycontrol = -1;
void sendOutput() {
  // timing code: if it's been more than 20ms since our last
  // send, then send again
  Serial.println("Mode " + (String) mode + (String) xy_lock);

  if (XYLockButton.update() && XYLockButton.fallingEdge()) {
    xy_lock++;
  }


  /*
   * If there are no gestures, send nothing.
   * If there is a thumb gesture, send a note.
   * If there are other gestures, send control changes
   */
  if (mode == 0) {
    // calibration mode: xy lock
    xy_lock %= 3;


    // Which control number we should send.
    // This code will read the states of all of the fingers, and decide
    // which, if any, controls should be sent
    int xcontrol = -1;
    int ycontrol = -1;
    bool send_note = false;

    Serial.print("{ ");
    for (int i = 0; i < FINGER_N; i++) {
      Serial.print(fingers[i]);
      Serial.print(", ");
    }
    Serial.print("}");
    Serial.println();

    /*
     * Todo: debounce this somehow! ouch.
     */
        
    if (gesture(fingers, (const int[])          { 260, 4, 127, 460, 430, 426, 435, 438, }         )) {xcontrol = 21;  //pistol
                                                                                                  ycontrol = 22;}
    else if ( gesture(fingers,  (const int[])   { 111, 480, 545, 487, 446, 477, 478, 0, }        )) {xcontrol = 3;  //fist
                                                                                                  ycontrol = 4;}
    else if ( gesture(fingers,  (const int[])   { -6, -21, -88, 12, -116, 11, -6, 0, }            ))  {send_note = true;  //flat
                                                                                                  ycontrol = 5;}
    else if ( gesture(fingers,  (const int[])   { -12, 382, -189, 420, -472, 125, 188, 0, }      )) {xcontrol = 7;  //tiger
                                                                                                  ycontrol = 8;}
    else if ( gesture(fingers,  (const int[])   { 333, 7, 189, 14, 453, 17, 453, 452, }         )) {xcontrol = 9;  //duck
                                                                                                  ycontrol = 10;}
    else if ( gesture(fingers,  (const int[])   { 396, 22, 777, 64, 511, 49, 350, 473, }        )) {xcontrol = 11;  //wolf
                                                                                                 ycontrol = 12;}
    else if ( gesture(fingers,  (const int[])   { 605, 13, 561, 33, 359, 463, 214, 424, }       )) {xcontrol = 29;  //3 fingers up
                                                                                                  ycontrol = 30;}
    else if ( gesture(fingers,  (const int[])   { 211, -8, 39, -7, 216, 417, 278, 491, }        )) {xcontrol = 27;  // gun
                                                                                                  ycontrol = 27;}                                                      
    
    if (ABCButtonState == true) {xcontrol = 13; ycontrol = 14; Serial.println("ABCBUTTON SEND");
            analogWrite(RED1, 255);
            analogWrite(RED2, 255);
            analogWrite(GREEN1, 255);
            analogWrite(GREEN2, 255);}   
    if (ABButtonState == true) {xcontrol = 15; ycontrol = 16; Serial.println("ABBUTTON SEND");
            analogWrite(RED1, 255);
            analogWrite(RED2, 255);}
    if (BCButtonState == true) {xcontrol = 25; ycontrol = 26; Serial.println("BCBUTTON SEND");
            analogWrite(RED1, 255);
            analogWrite(RED2, 255);}
    if (ACButtonState == true) {xcontrol = 27; ycontrol = 28; Serial.println("ACBUTTON SEND");
            analogWrite(RED1, 255);
            analogWrite(RED2, 255);}                                                               
    else if (AButtonState == true && ABButtonState == false && ACButtonState == false && ABCButtonState == false) {xcontrol = 17; ycontrol = 18; Serial.println("ABUTTON SEND");}
    else if (BButtonState == true && BCButtonState == false && ABButtonState == false && ABCButtonState == false) {xcontrol = 19; ycontrol = 20; Serial.println("BBUTTON SEND");}
    else if (CButtonState == true) {xcontrol = 1; ycontrol = 2; Serial.println("CBUTTON SEND");}   

    bool in_gesture = (xcontrol != -1 || ycontrol != -1);

    if (xcontrol != lastxcontrol  || ycontrol != lastycontrol) {
      if (gestureDebounce == 0) {
        gestureDebounce = millis();
        return;
      }

      if (millis() - gestureDebounce < 40) {
        return;
      }
    }

    gestureDebounce = 0;
    lastxcontrol = xcontrol;
    lastycontrol = ycontrol;


    if (send_note && xy_lock != 2) {
      noteOn();
      analogWrite(GREEN1, 255);
      analogWrite(GREEN2, 255);
      analogWrite(BLUE1, 255);
      analogWrite(BLUE2, 255);

    } else {
      noteOff();
      analogWrite(GREEN1, 0);
      analogWrite(GREEN2, 0);
      analogWrite(BLUE1, 0);
      analogWrite(BLUE2, 0);
    }

    if (in_gesture) {
      Serial.println("In gesture");
      int xmidi = 0;
      int ymidi = 0;
      if (xcontrol != -1 && xy_lock != 2) {
        xmidi = map (accel_x, -130, 220, 0, 127);
        sendControl(xcontrol + MODE0_CONTROL, xmidi);
      }
      if (ycontrol != -1 && xy_lock != 1) {
        ymidi = map (accel_y, 150, -245, 0, 127);
        sendControl(ycontrol + MODE0_CONTROL, ymidi);
      }

      //Send a hit
      digitalWrite(motorPin, HIGH);
      delay(7);
      digitalWrite(motorPin, LOW);
      delay(3);

      
      //Show the LED: calibrate the midi values to a LED value
      int ledval = map(ymidi, 0, 127, 100, 255);
      analogWrite(BLUE1, ledval);
      analogWrite(BLUE2, ledval);

      ledval = map(xmidi, 0, 127, 100, 255);
      analogWrite(BLUE1, ledval);
      analogWrite(BLUE2, ledval);

    }
    else {
     
      /*
      analogWrite(BLUE1, 0);
      analogWrite(BLUE2, 0);
      analogWrite(RED1, 0);
      analogWrite(RED2, 0);
      analogWrite(GREEN1, 0);
      analogWrite(GREEN2, 0);
      */
    }
  }

  /*
   * Accel X is the note value.
   * Accel Y is the speed
   * If the fingers are in a fist, then send a note.
   */

  else if (mode == 1 ) {
    bool in_fist = false;

    if ( gesture(fingers,  (const int[])  { -39, -31, -38, -15, -20, -11, 2, -15, }          )) {
      in_fist = true; //USE GESTURE TO TRIGGER ARPEGG
    }


    if ( in_fist) {
      send_arpegg = true;
    } else {
      send_arpegg = false;
      noteOff();
    }

    //Map the Y accel to the midi interval
    arpegg_interval =  map (accel_y, 250, -170, 8, 1) * 2;
    if ( arpegg_interval < 1) {
      arpegg_interval = 1;
    }
    Serial.print("Arpeggio interval: ");
    Serial.println(arpegg_interval);
  }

  
  //All of the fingers (except thumb) are controls
  //Fingers are 21-29, X is 29, and Y is 30
  
  else if (mode == 2) {
    // calibration mode: xy lock
    xy_lock %= (FINGER_N + 2);

    /* Program mode - obey xy_lock */
    if (xy_lock != 0) {
      int val = 0;
      if (xy_lock < FINGER_N - 1) {
        val = map(fingers[xy_lock], 600, 0, 0, 127);
      } else if (xy_lock == FINGER_N - 1) {
        val = map (accel_x, -130, 220, 0, 127);
      } else if (xy_lock == FINGER_N ) {
        val = map (accel_y, 100, -245, 0, 127);
      }
      sendControl(xy_lock + MODE2_CONTROL, val);
      analogWrite(BLUE1, 0);
      analogWrite(BLUE2, 0);
      analogWrite(GREEN1, 0);
      analogWrite(GREEN2, 0);
    }
    else {
      for (int i = 0; i < FINGER_N - 1; i++) {
        int val = map(fingers[i + 1], 600, 0, 0, 127);
        sendControl(i + MODE2_CONTROL, val);
      }
      int val = map (accel_x, -130, 220, 0, 127);
      sendControl(FINGER_N - 1 + MODE2_CONTROL, val);
      val = map (accel_y, 100, -245, 0, 127);
      sendControl(FINGER_N + MODE2_CONTROL, val);

      int fingerssum = 0;
      for (int i = 0; i < FINGER_N; i++) {
        fingerssum += fingers[i];
      }
      int fingersavrg = fingerssum / FINGER_N;
      int ledavrg = map (fingersavrg, 50, 400, 0, 255);
      Serial.print(fingersavrg); Serial.print (" "); Serial.println(ledavrg);
      if (ledavrg > 255) ledavrg = 255;
      analogWrite(BLUE1, ledavrg);
      analogWrite(BLUE2, ledavrg);

    }
  }
}

void sendControl(int num, int val) {
  if (val < 0) {
    val = 0;
  } else if (val > 127) {
    val = 127;
  }

  //if(millisTime-ctrlmidiTime>CTRLMIDI_TIME){                                    //MIDI CTRL BUFFER
  usbMIDI.sendControlChange(num, val, MIDI_CHAN);
  // ctrlmidiTime=millisTime;
  //}

#ifdef DEBUG
  Serial.print("sending ");
  Serial.print(num);
  Serial.print(" ");
  Serial.print(val);
  Serial.println();
#endif
}


/*
 * Send a note, based on accel_x, that is mapped
 * to a one-octave C-major scale.
 *
 * This needs to be debounced
 */
int noteDebounce = 0;
void noteOn() {
#define SCALE_SIZE 8
  int note = map (accel_x, -130, 220, 0, SCALE_SIZE);
  if (note >= SCALE_SIZE ) note = SCALE_SIZE - 1;
  if (note <= 0) note = 0;

  int scale[] = {60, 62, 64, 65, 67, 69, 71, 72};                                           //mapped scale 1
  note = scale[note];

  if (current_note == note) {
    noteDebounce = 0;
    return;
  }

  if (current_note != -1) {
    if (noteDebounce == 0) {
      noteDebounce = millis();
      return;
    }
    if ((millis() - noteDebounce) < 50) {
      return;
    }
    usbMIDI.sendNoteOff(current_note, 99, MIDI_CHAN);
  }

  noteDebounce = 0;
  usbMIDI.sendNoteOn(note, 99, MIDI_CHAN);
  current_note = note;
}

void noteOff() {
  if (current_note != -1) {
    // send note up
    usbMIDI.sendNoteOff(current_note, 99, MIDI_CHAN);
    current_note = -1;
  }
}


//Called every time there's a midi tick

void handleMidiTick() {
  // We're just using this for arpeggiator
  if (! send_arpegg) {
    return;
  }
  int val = midi_clock % (arpegg_interval * 2);

  if (val == 0) {
    noteOn();
    analogWrite(RED1, 100);
    analogWrite(RED2, 100);
    analogWrite(GREEN1, 100);
    analogWrite(GREEN2, 100);
  } else if (val == arpegg_interval) {
    noteOff();
    analogWrite(RED1, 0);
    analogWrite(RED2, 0);
    analogWrite(GREEN1, 0);
    analogWrite(GREEN2, 0);
  }
}

void initAccel() {
  Wire.begin();
  Wire.beginTransmission(accel_module);
  Wire.write(0x2D);
  Wire.write(0);
  Wire.endTransmission();
  Wire.beginTransmission(accel_module);
  Wire.write(0x2D);
  Wire.write(16);
  Wire.endTransmission();
  Wire.beginTransmission(accel_module);
  Wire.write(0x2D);
  Wire.write(8);
  Wire.endTransmission();
}


void readAccel() {
  int16_t xyzregister = 0x32;

  Wire.beginTransmission(accel_module);
  Wire.write(xyzregister);
  Wire.endTransmission();
  Wire.beginTransmission(accel_module);
  Wire.write(xyzregister);
  Wire.endTransmission();
  Wire.beginTransmission(accel_module);
  Wire.requestFrom(accel_module, 6);
  int16_t i = 0;
  while (Wire.available()) {
    values[i] = Wire.read();
    i++;
  }
  Wire.endTransmission();

  accel_x = (((int16_t)values[1]) << 8) | values [0];
  accel_y = (((int16_t)values[3]) << 8) | values [2];
  accel_z = (((int16_t)values[5]) << 8) | values [4];
}

void initFingers() {
  for (int i = 0; i < FINGER_N; i++) {
    finger_pins[i].init(bendPins[i], true);
  }
}

void initButtons() {
  //XYLockButton = Bounce(7, 10);
  pinMode(FINGERPIN1, INPUT_PULLUP);
  pinMode(FINGERPIN2, INPUT_PULLUP);
  pinMode(BUTTON3, INPUT_PULLUP);
  pinMode(BUTTON4, INPUT_PULLUP);
  pinMode(BUTTONA, INPUT_PULLUP);
  pinMode(BUTTONB, INPUT_PULLUP);
  pinMode(BUTTONC, INPUT_PULLUP);
}

void readButtons() {
  //XYLockButton.update();
  ModeButton.update();
  CalibButton.update();
  MuteButton.update();
  AButton.update();
  BButton.update();
  CButton.update();
}

void clearLeds() {
  analogWrite(BLUE1, 0);
  analogWrite(BLUE2, 0);
  analogWrite(RED1, 0);
  analogWrite(RED2, 0);
  analogWrite(GREEN1, 0);
  analogWrite(GREEN2, 0);
}


void readFingers() {
  for (int i = 0; i < FINGER_N; i++) {
    SmoothPin pin = finger_pins[i];
    pin.read();

    // Apply the calibration values to the raw finger result
    fingers[i] = map(pin.value, fingerMin[i], fingerMax[i], 0, 500);
    //fingers[i] = pin.value;
    //Serial.println(pin.value);
  }
}



bool gesture(int fv[], const int def[]) {
  for (int i = 0; i < FINGER_N; i++) {
    if (abs(fv[i] - def[i]) > DIFF_THRESH)
      return false;
  }
  return true;
}


void resetCalibration() {
  for (int i = 0; i < FINGER_N; i++) {
    fingerMin[i] = 32767;
    fingerMax[i] = 0;
  }
}

void updateCalibration() {
  for (int i = 0; i < FINGER_N; i++) {
    SmoothPin pin = finger_pins[i];
    int val = pin.read();
    if (val > fingerMax[i]) {
      fingerMax[i] = val;
    }
    if (val < fingerMin[i]) {
      fingerMin[i] = val;
    }
    char buf[128];
    sprintf(buf, "%d (%d): %d, %d ", i, val, fingerMin[i], fingerMax[i]);
    Serial.println(buf);
  }
}


//read the calibration values from eeprom
void readCalibration() {
  Serial.println("Read calibraiton");
  for (int i = 0; i < FINGER_N; i++) {
    fingerMin[i] = readShort(CALIB_ADDR + 2 * i);
    fingerMax[i] = readShort(CALIB_ADDR + 2 * FINGER_N + 2 * i);
    char buf[30];
    sprintf(buf, "%d: %d, %d ", i, fingerMin[i], fingerMax[i]);
    Serial.println(buf);
  }
}

uint16_t readShort(int addr) {
  uint16_t val = EEPROM.read(addr);
  val *= 256;
  val += EEPROM.read(addr + 1);
  return val;
}

void writeShort(int addr, uint16_t val) {
  EEPROM.write(addr, val / 256);
  EEPROM.write(addr + 1, val % 256);
}

void writeCalibration() {
  for (int i = 0; i < FINGER_N; i++) {
    writeShort(CALIB_ADDR + 2 * i, fingerMin[i]);
    writeShort(CALIB_ADDR + 2 * FINGER_N + 2 * i, fingerMax[i]);
  }
}


//Called by the midi library, whenever a tick comes in.
void handleMidiClock(byte message) {
  if (message == 248) { // tick
    midi_clock++;
    handleMidiTick();
  }

  else if (message == 250 || message == 251) {
    //MIDI START / MIDI CONTINUE
    midi_clock = 0;
  }
  else if (message == 252) {
    // MIDI STOP
    if (send_arpegg) {
      analogWrite(RED1, 0);
      analogWrite(RED2, 0);
      analogWrite(GREEN1, 0);
      analogWrite(GREEN2, 0);
      noteOff();
    }
  }
}


