#include "Tlc5940.h"
#include <Bounce.h>
#include <Wire.h>
#include <MIDI.h>   
#include <usb_keyboard.h>   
             
#define MIDI_CHAN 3               
                                    
#define HIGHTHRESH 540              //the frs threshold that plays the note                                
#define LOWTHRESH 900               //to be able to play again the fsr needs to jump back above this value                                           
#define FSR_N 9                                                 
int fsrPins[] = {15, 17, 40, 20, 21, 23, 22, 16, 14};
int fsrReadings[FSR_N];
bool fsrPress[FSR_N];
int fsrmidiVol[FSR_N];

int scale0[] = {37, 38, 39, 40, 41, 42, 43, 44, 36};     //a chromatic scale, use this only for drum pads
int scale1[] = {53, 54, 55, 56, 57, 58, 59, 60, 52};     // the last pin is the first note, to have the big pad as the start of the standard ableton drumrack setup

      //for now, scale0 for bank0, scale for bank1, if other functions, use banks
    
#define BUTTON1 0                                        //BUTTONS AND BOUNCE
#define BUTTON2 1  
#define BUTTON3 2
Bounce ButtonUp = Bounce(BUTTON1, 10);
Bounce ButtonDown = Bounce(BUTTON2, 10);
Bounce ButtonLow = Bounce(BUTTON3, 10);

int tlcbluePins[] = {3,6,9,12,16,19,22,25,28,31};
int tlcgreenPins[] = {1,4,7,10,13,17,20,23,26,29};
int tlcredPins[] = {2,5,8,11,14,18,21,24,27,30};
int tlcallPins[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
#define TLCBLUEPIN_N 10             
#define TLCGREENPIN_N 10
#define TLCREDPIN_N 10
#define TLCALLPIN_N 30              //max value on chip is 3000/40000 
#define DELAY_LEDS 3                //the delay to make sure the chip can handle the data load..

#define BANKS_N 2                       //number of banks
int bank = 0;

int accel1Switch = 0;
int accel2Switch = 0;
#define CTRL_START 51
int16_t accel_x, accel_y, accel_z;                        //accelerometer
#define accel_module (0x53)                               //x-left/right -100 to 100 y back front -275 top, to -200 leanover in both directions. z not working...
byte values [6];
char output [512];

unsigned long startTime[FSR_N];   //millis bounce function
unsigned long millisTime;
bool fsrBounce[FSR_N];
int bounceTime = 100;
                
unsigned long stopTime;            
bool stopBool = false;
int stopHold = 2000;                         //time to hold down buttons to send del-msg

unsigned long ctrl1midiTime=0;
unsigned long ctrl2midiTime=0;
#define CTRLMIDI_TIME 50
int ctrl1var;
int ctrl1varold=0;
int ctrl2var;
int ctrl2varold=0;


IntervalTimer myTimer;                      //interrupt counter
int counter=0;


void setup(void) {
 
  Serial.begin(9600);                     //serial monitor start
  myTimer.begin(heartbeat, 10000000);     // Time in microseconds (10s)
  
  initAccel();
  Tlc.init();                             //inits fo all sensors and led chips
  initFsrs();
  initButtons();

  for(int i=0; i<FSR_N; i++){             //reset all press booleans
    fsrPress[i]=false;
  }
   for(int i=0; i<FSR_N; i++){             //reset all debounce startTimes
    startTime[i]=0;
    fsrBounce[i]=true;
  }
    
  
  clearLeds();                            //little led boot-up play
  for (int i=0; i<TLCBLUEPIN_N; i++)   {
    Tlc.set(tlcbluePins[i], 3000); 
    Tlc.update();
    delay(127);  
  }
  /*
  clearLeds();
  for (int i=0; i<TLCGREENPIN_N; i++)   {
    Tlc.set(tlcgreenPins[i], 3000); 
    Tlc.update();
    delay(50);  
  }
  clearLeds();
  for (int i=0; i<TLCREDPIN_N; i++)   {
    Tlc.set(tlcredPins[i], 3000); 
    Tlc.update();
    delay(50);  
  }
  clearLeds();   
  
  for (int i=0; i<TLCALLPIN_N; i++)   {
    Tlc.set(tlcallPins[i], 800); 
    Tlc.update();
    delay(25);  
  }
  */
  clearLeds();
}

void loop(void) {                                   //MAIN CODE 

  millisTime = millis();                  //time var running WHATS THE MAX MILLIS VALUE?

  while (usbMIDI.read()) ;                // read and discard any incoming MIDI messages

  readAccel();
  readFsrs();
  readButtons();

  if(ButtonUp.fallingEdge()) {              //3 banks and the buttons switch through them BLUE==MODE0, TURKOISE==MODE1, GREEN=MODE2
    bank++;                                 //Leds show bank on button switch when control or midi notes are being sent                                
    if (bank>=2) bank=BANKS_N-2;;
    bankLeds();   
    resetPress();                           //resets the press booleans on every bank change  
    stopTime=millisTime;
    stopBool=true;
  }
  if (ButtonUp.risingEdge()) {
    stopBool=false;
  }
  if (millisTime-stopTime>=stopHold && stopBool==true) {
    stopBool=false;
    bank=2;
    bankLeds();
  }
  if(ButtonLow.fallingEdge()) {
    accel1Switch++;
    if (accel1Switch>=2) accel1Switch = 0;
    accel1SwitchLeds();
  }
   if(ButtonDown.fallingEdge()) {
    accel2Switch++;
    if (accel2Switch>=2) {
      accel2Switch = 0;
   /*
      Keyboard.set_modifier(MODIFIERKEY_CTRL);
      Keyboard.set_key1(KEY_Z);
      Keyboard.send_now();
      Keyboard.set_modifier(0);
      Keyboard.set_key1(0);
      Keyboard.send_now();    
    */
    }
    accel2switchLeds();
                         
  } 
  //serial.println(bank); 
  //serial.println(accel1Switch);

  if (accel1Switch == 1 &&  millisTime-ctrl1midiTime>CTRLMIDI_TIME) {                                 //x-left/right -100 to 100 y back front -275 top, to -200 leanover in both directions. z not working...
    /*
    if (accel_x<-100) accel_x=-100;
    if (accel_x>100) accel_x=100;
    int xctrllevel = map (accel_x, -100, 100, 0, 127);  
    usbMIDI.sendControlChange(CTRL_START, xctrllevel, MIDI_CHAN); 
    Serial.print ("accel1_x ");
    Serial.println (accel_x);
    */
    if (accel_y<-275) accel_y=-275;
    if (accel_y>-140) accel_y=-140;
    ctrl1var = map (accel_y, -140, -275, 0, 127);  
    if (ctrl1var != ctrl1varold) {
      usbMIDI.sendControlChange(CTRL_START, ctrl1var, MIDI_CHAN); 
      ctrl1varold = ctrl1var;
      Serial.print ("CTRL: ");
      Serial.print (CTRL_START);
      Serial.print (" Channel: ");
      Serial.print (MIDI_CHAN);
      Serial.print (" Accel_y ");
      Serial.print (accel_y);
      Serial.print (" Val: ");
      Serial.println (ctrl1var);
    }

    ctrl1midiTime = millisTime;
  }
  if (accel2Switch == 1 &&  millisTime-ctrl2midiTime>CTRLMIDI_TIME) {                                 //x-left/right -100 to 100 y back front -275 top, to -200 leanover in both directions. z not working...
    /*if (accel_x<0) accel_x=accel_x*-1;
    if (accel_x>130) accel_x=130;
    int xctrllevel = map(accel_x, 0, 130, 127, 0);  
    usbMIDI.sendControlChange(CTRL_START+1, xctrllevel, MIDI_CHAN);
    Serial.print ("accel2_x ");
    Serial.println (accel_x);
    */
    if (accel_y<-275) accel_y=-275;
    if (accel_y>-140) accel_y=-140;
    int ctrl2var = map (accel_y, -140, -275, 0, 127);  
    if (ctrl2var != ctrl2varold) {
      usbMIDI.sendControlChange(36, ctrl2var, 2);                                                     //respective value on gauntlet
      //usbMIDI.sendControlChange(CTRL_START+1, yctrllevel, MIDI_CHAN); 
      ctrl2varold=ctrl2var;
      Serial.print ("CTRL: ");
      Serial.print ("36");
      Serial.print (" Channel: ");
      Serial.print ("2");
      Serial.print (" Accel_y: ");
      Serial.print (accel_y);
      Serial.print (" Val: ");
      Serial.println (ctrl2var);
    }
     
    ctrl2midiTime = millisTime;

  }
  
  if (bank!=2){
    for(int i=0; i<FSR_N; i++){                                                   //FSR Code 
      if (fsrBounce[i]==false && millisTime-startTime[i]>=bounceTime) {            //sets all bounces to true      
        fsrBounce[i]=true;
      }
      if (fsrReadings[i] <= HIGHTHRESH&&fsrBounce[i]==true){       // if reading goes below highthress and if bounce is ok
                                                                  // and if fsr is not pressed, end note, switch led on,etc.
      if (fsrPress[i]==false){                                                                   
        fsrmidiVol[i] = map (fsrReadings[i], 1024, 0, 0, 127);
        if (bank==0){
          usbMIDI.sendNoteOn(scale0[i], fsrmidiVol[i], MIDI_CHAN);
          Tlc.set(tlcbluePins[i], 3000);                      //switch respective led on
          if (fsrReadings[8]<=HIGHTHRESH) {                   //main fsr has an additional LED not covered by the main led function
            Tlc.set(tlcbluePins[9],3000);             
          } 
          Tlc.update();
          delay(DELAY_LEDS);                                           //tlc chip needs some extra time, without the delay msgs often dont arrive especially in forloops...
        }
        else if (bank==1) {
          usbMIDI.sendNoteOn(scale1[i], fsrmidiVol[i], MIDI_CHAN);
          Tlc.set(tlcgreenPins[i], 3000);                      //switch respective led on
          if (fsrReadings[8]<=HIGHTHRESH) {                   //main fsr has an additional LED not covered by the main led function
            Tlc.set(tlcgreenPins[9],3000);             
          } 
          Tlc.update();
          delay(DELAY_LEDS);                                           //tlc chip needs some extra time, without the delay msgs often dont arrive especially in forloops...
        }
        startTime[i]=millisTime;                            //if note is send, activate bounce
        fsrBounce[i]=false;                                 //fsr is not bounced                                 

        //serial.println(fsrPins[i]);
      }
      fsrPress[i]=true;                                 //fsr is  pressed
      }
    else if (fsrReadings[i] > LOWTHRESH) {
      if (fsrPress[i]==true){
      if (bank==0) {
        usbMIDI.sendNoteOff(scale0[i], 127, MIDI_CHAN);
        Tlc.set(tlcbluePins[i], 0);                      //switch respective led on
          if (fsrReadings[8]>HIGHTHRESH) {                   //main fsr has an additional LED not covered by the main led function
            Tlc.set(tlcbluePins[9],0);             
          } 
          Tlc.update();
          delay(DELAY_LEDS); 
      }
      
      else if (bank==1) {
        usbMIDI.sendNoteOff(scale1[i], 127, MIDI_CHAN);
        Tlc.set(tlcgreenPins[i], 0);                      //switch respective led on
          if (fsrReadings[8]>HIGHTHRESH) {                   //main fsr has an additional LED not covered by the main led function
            Tlc.set(tlcgreenPins[9],0);             
          } 
          Tlc.update();
          delay(DELAY_LEDS); 
        
      }
      //clearLeds();
      }
      fsrPress[i] = false;
    }
  } 
 }
}


void initAccel(){
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

void initFsrs(){
  pinMode(fsrPins[0], INPUT_PULLUP); 
  pinMode(fsrPins[1], INPUT_PULLUP);
  pinMode(fsrPins[2], INPUT_PULLUP);
  pinMode(fsrPins[3], INPUT_PULLUP);
  pinMode(fsrPins[4], INPUT_PULLUP);
  pinMode(fsrPins[5], INPUT_PULLUP);
  pinMode(fsrPins[6], INPUT_PULLUP);
  pinMode(fsrPins[7], INPUT_PULLUP);
  pinMode(fsrPins[8], INPUT_PULLUP);
}

void initButtons() {
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);
  pinMode(BUTTON3, INPUT_PULLUP);
} 

void readAccel() {                                        //read accel functions
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
   while(Wire.available()){
     values[i] = Wire.read();
     i++;
   }
   Wire.endTransmission();
   
   accel_x = (((int16_t)values[1]) << 8) | values [0];
   accel_y = (((int16_t)values[3]) << 8) | values [2];
   accel_z = (((int16_t)values[5]) << 8) | values [4];

  //serial.print (" X ");
  //serial.print(accel_x); 
  //serial.print (" Y ");
  //serial.print(accel_y);
  //serial.print (" Z ");
  //serial.print(accel_z);
  //serial.print(" ");
}

void readFsrs(){                          //reads and prints fsr values
  for(int i=0; i<FSR_N; i++){
    fsrReadings[i] = analogRead(fsrPins[i]);
  }
  for(int i=0;i<FSR_N; i++){
    Serial.print (fsrReadings[i]);
    Serial.print (" ");
  }
  Serial.println (" ");
}

void readButtons(){                       //reads buttons
  ButtonUp.update();
  ButtonDown.update();
  ButtonLow.update();
}

void bankLeds() {                       //the led functions to show the different banks
  if (bank==0) {                      //maybe change color for green and red later...
    clearLeds();
    resetScales();
    for (int i=0; i<TLCBLUEPIN_N; i++)   {
      Tlc.set(tlcbluePins[i], 400); 
      Tlc.update(); 
      delay (DELAY_LEDS);                       //otherwise the chip cant handle the speed 
    }
    //put millis delay here
    clearLeds();
  }
  else if (bank==1) {
    clearLeds();
    resetScales(); 
    for (int i=0; i<TLCGREENPIN_N; i++)   {
      Tlc.set(tlcgreenPins[i], 300); 
      Tlc.update();  
      delay (DELAY_LEDS);                    
    }
    //put millis delay here
    clearLeds();
  }
  else if (bank==2) {
    clearLeds();
    resetScales();  
    for (int i=0; i<TLCREDPIN_N; i++)   {
      Tlc.set(tlcredPins[i], 400); 
      Tlc.update(); 
      delay (DELAY_LEDS);                  
    }
  }
}

void accel1SwitchLeds() {
  if (accel1Switch==0) {               
    clearLeds();
    for (int i=0; i<TLCBLUEPIN_N; i++)   {
      Tlc.set(tlcbluePins[i], 400); 
      Tlc.update(); 
      delay (DELAY_LEDS);                  
    }
    for (int i=0; i<TLCGREENPIN_N; i++)   {
      Tlc.set(tlcgreenPins[i], 300); 
      Tlc.update();  
      delay (DELAY_LEDS);                    
    }
  }
  else if (accel1Switch==1) {
    clearLeds();
    for (int i=0; i<TLCALLPIN_N; i++) {
      Tlc.set(tlcallPins[i], 500); 
      Tlc.update();
      delay (DELAY_LEDS);  
    }
  }
}

void accel2switchLeds() {
  if (accel2Switch==0) {               
    clearLeds();
    for (int i=0; i<TLCBLUEPIN_N; i++)   {
      Tlc.set(tlcredPins[i], 400); 
      Tlc.update(); 
      delay (DELAY_LEDS);                  
    }
    for (int i=0; i<TLCGREENPIN_N; i++)   {
      Tlc.set(tlcgreenPins[i], 300); 
      Tlc.update();  
      delay (DELAY_LEDS);                    
    }
  }
  else if (accel2Switch==1) {
    clearLeds();
    for (int i=0; i<TLCALLPIN_N; i++) {
      Tlc.set(tlcallPins[i], 500); 
      Tlc.update();
      delay (DELAY_LEDS);  
    }
  }
}

void clearLeds() {                      
  Tlc.clear();
  Tlc.update();
}

void resetPress(){
  for(int i=0; i<FSR_N; i++){ 
    fsrPress[i]=false;
  }
}
void resetScales(){
  for(int i=0; i<FSR_N; i++){ 
    usbMIDI.sendNoteOff(scale0[i], 127, MIDI_CHAN);
    usbMIDI.sendNoteOff(scale1[i], 127, MIDI_CHAN);
  }
}

void heartbeat() {
  Serial.print("still alive ");
  Serial.println(counter);
  counter++;
}


    


