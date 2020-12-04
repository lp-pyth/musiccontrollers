#include "Tlc5940.h"
#include <Bounce.h>
#include <Wire.h>
#include <MIDI.h>   
#include <usb_keyboard.h>   
             
#define MIDI_CHAN 3   

int tlcbluePins[] = {3,6,9,12,16,19,22,25,28,31};
int tlcgreenPins[] = {1,4,7,10,13,17,20,23,26,29};
int tlcredPins[] = {2,5,8,11,14,18,21,24,27,30};
int tlcallPins[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
#define TLCBLUEPIN_N 10             
#define TLCGREENPIN_N 10
#define TLCREDPIN_N 10
#define TLCALLPIN_N 30                                                                //max value on chip is 3000/40000 
#define DELAY_LEDS 3                                                                  //the delay to make sure the chip can handle the data load..
int lowledval = 1000;
int lowledvalCounter=0;
int highledval = 3000;
                                      
#define HIGHTHRESH 540                                                                //the frs threshold that plays the note                                
#define LOWTHRESH 900                                                                 //to be able to play again the fsr needs to jump back above this value                                           
#define FSR_N 9                                                 
int fsrPins[] = {15, 17, 40, 20, 21, 23, 22, 16, 14};
int fsrReadings[FSR_N];
bool fsrPress[FSR_N];
int fsrmidiVol[FSR_N];

int scale0[] = {37, 38, 39, 40, 41, 42, 43, 44, 36};                                  //drum scale
int scale1[] = {36, 38, 40, 41, 43, 45, 47, 48, 34};                                  //chestins scale

#define BUTTON1 0                                        //BUTTONS AND BOUNCE
#define BUTTON2 1  
#define BUTTON3 2
Bounce ButtonUp = Bounce(BUTTON1, 10);
Bounce ButtonDown = Bounce(BUTTON2, 10);
Bounce ButtonShift = Bounce(BUTTON3, 10);

#define PADMODE_N 2                                                                   //number of padmodes (3, namely Drumkit=0, ChestIns=1)
int padmode = 0;
bool shiftmode = false;
bool shiftPad[FSR_N];
bool shiftpadCounter[FSR_N];
int shiftcounter = 0;

int accel1Switch = 0;
int accel2Switch = 0;
//#define CTRL_START 51
int ctrlchange[] = {0,1,2,3,4,5,6,7,8};
int16_t accel_x, accel_y, accel_z;                                                    //accelerometer
#define accel_module (0x53)                                                           //x-left/right -100 to 100 y back front -275 top, to -200 leanover in both directions. z not working...
byte values [6];
char output [512];

unsigned long startTime[FSR_N];   //millis bounce function
unsigned long millisTime;
unsigned long shiftcounterTime;
bool fsrBounce[FSR_N];
int bounceTime = 100;
bool shiftcounterBool = false;
               
unsigned long ctrlmidiTime[FSR_N]={0,0,0,0,0,0,0,0,0};
#define CTRLMIDI_TIME 100
int ctrlyvar;
int ctrlyvarold=0;  
int ctrlxvar;
int ctrlxvarold=0;

IntervalTimer myTimer;                                                                //interrupt counter
int counter=0;

void setup(void) {
 
  Serial.begin(9600);                                                                 //serial monitor start
  myTimer.begin(heartbeat, 10000000);                                                 // Time in microseconds (10s)
  
  initAccel();
  Tlc.init();                                                                         //inits fo all sensors and led chips
  initFsrs();
  initButtons();
  
  for(int i=0; i<FSR_N; i++){                                                         //reset all debounce startTimes
    startTime[i]=0;
    fsrBounce[i]=true;
    shiftPad[i]=false;                                                                //puts all ctrl pads off
    shiftpadCounter[i]=false;
    fsrPress[i]=false;                                                                //reset all press booleans
  }
  
  resetScales();
 
  /*clearLeds();                                                                        //little led boot-up play
  for (int i=0; i<TLCBLUEPIN_N; i++)   {
    Tlc.set(tlcbluePins[i], lowledval); 
    Tlc.update();
    delay(127);  
  } 
 
  clearLeds();

 
  for (int i=0; i<TLCGREENPIN_N; i++)   {
    Tlc.set(tlcgreenPins[i], highledval); 
    Tlc.update();
    delay(50);  
  }
  clearLeds();
  for (int i=0; i<TLCREDPIN_N; i++)   {
    Tlc.set(tlcredPins[i], highledval); 
    Tlc.update();
    delay(50);  
  }
  clearLeds();   
  
  for (int i=0; i<TLCALLPIN_N; i++)   {
    Tlc.set(tlcallPins[i], 800); 
    Tlc.update();
    delay(25);  
  }
  
  clearLeds(); */
}

void loop(void) {                                //MAIN CODE 

  millisTime = millis();                                                              //time var running WHATS THE MAX MILLIS VALUE?

  while (usbMIDI.read()) ;                                                            //read and discard any incoming MIDI messages

  readAccel();
  readFsrs();
  readButtons();
 
  if(ButtonShift.fallingEdge()) {                                                     //if we press the shift button and keep it pressed we are in control change mode
    Serial.println("CTRL SEND READY");                                                //could use the whole drum pad here to send ctrl values, actually no need to use pushbuttons
    shiftmode=true; 
    ctrlLeds();
    shiftcounterBool=true;
    shiftcounterTime=millisTime;
    Serial.println("shiftcounterbool=true");
  }
  
 /* if(millisTime-shiftcounterTime>=2000 && shiftcounterBool==true){
    lowledvalCounter++;
    if (lowledvalCounter>=2){
      lowledvalCounter=0;
    }
    if (lowledvalCounter==1){
    lowledval= 0;
    Serial.println ("lowledval=0");
    }
    if (lowledvalCounter==0){
    lowledval= 1000;
    Serial.println ("lowledval=1000");
    }
  } */

  if(ButtonShift.risingEdge()) {
    Serial.println("PLAYSEND READY");
    shiftcounterBool==false;
    Serial.println("shiftcounterbool=false");
    shiftmode=false;
    padmodeLeds();
  }

  if(shiftmode==true){                       //SHIFT ON CTRL MODE SENDS CTRL VALUES ON BUTTONS AND SOMETHING ELSE ON PADS
    if(ButtonUp.fallingEdge()) {    
      for(int i=0; i<FSR_N; i++){                                                     //all ctrls on
        shiftPad[i]=true;                     
      }          
    }
    if(ButtonDown.fallingEdge()) {
      for(int i=0; i<FSR_N; i++){                                                     //all ctrls off
        shiftPad[i]=false;                    
      }  
    }
    for(int i=0; i<FSR_N; i++){                                                       //if shift button is pressed send ctrl values on drumpads
      if (fsrBounce[i]==false && millisTime-startTime[i]>=bounceTime) {               //sets all bounces to true, ok to go once bounce time has been reached     
        fsrBounce[i]=true;
      }
      if (fsrReadings[i]<=HIGHTHRESH && fsrBounce[i]==true){                          //if reading goes below highthress and if bounce is ok and if fsr is not pressed, end note, switch led on,etc.
        if (fsrPress[i]==false){                                                                   
          if (shiftpadCounter[i]==false){
            shiftPad[i]=true;
            shiftpadCounter[i]=true;
          }
          else if (shiftpadCounter[i]==true){
            shiftPad[i]=false;
            shiftpadCounter[i]=false;       
          }
         
          startTime[i]=millisTime;                                                    //if note is send, activate bounce
          fsrBounce[i]=false;                                                         //fsr is not bounced                                 
          //serial.println(fsrPins[i]);
        }
        fsrPress[i]=true;                                                             //fsr is  pressed
      }
      else if (fsrReadings[i] > LOWTHRESH) {
        if (fsrPress[i]==true){
        // nothing here
        }
        fsrPress[i] = false;
      }
    }
    for(int i=0; i<FSR_N; i++){                   //LEDS FUNCTION IN SHIFTMODE
      if (shiftPad[i]==true){
        Tlc.set(tlcbluePins[i], highledval);                                          //switch respective led on
        if (shiftPad[8]==true) {                                                      //main fsr has an additional LED not covered by the main led function
          Tlc.set(tlcbluePins[9],highledval);             
        } 
        Tlc.set(tlcgreenPins[i], highledval);                                         //switch respective led on
        if (shiftPad[8]==true) {                                                      //main fsr has an additional LED not covered by the main led function
          Tlc.set(tlcgreenPins[9],highledval);             
        } 
        Tlc.set(tlcredPins[i], highledval);                                           //switch respective led on
        if (shiftPad[8]==true) {                                                      //main fsr has an additional LED not covered by the main led function
          Tlc.set(tlcredPins[9],highledval);             
        } 
        Tlc.update();
        //delay(DELAY_LEDS);  
      }
      if (shiftPad[i]==false){
        Tlc.set(tlcbluePins[i], 0);                                                   //switch respective led on
        Tlc.set(tlcredPins[i], lowledval); 
        Tlc.set(tlcgreenPins[i], lowledval);
        if (shiftPad[8]==false) {                                                     //main fsr has an additional LED not covered by the main led function
          Tlc.set(tlcbluePins[9], 0);                                                 //switch respective led on
          Tlc.set(tlcredPins[9], lowledval); 
          Tlc.set(tlcgreenPins[9], lowledval);             
        } 
        Tlc.update();
         //delay(DELAY_LEDS);  
      }
    }
  }
                                                    
                                              //CTRL SENDS FUNCTION INDEPENDENT OF SHIFTMODE
  if (accel_y<-275) accel_y=-275;                                                   //x-left/right -100 to 100 y back front -275 top, to -200 leanover in both directions. z not working...
  if (accel_y>-140) accel_y=-140;
  ctrlyvar = map (accel_y, -140, -275, 0, 127);
                                                 
  if (ctrlyvar != ctrlyvarold) {                                                    //only send ctrl changes
    for(int i=0; i<FSR_N; i++){                      
      if (shiftPad[i]==true && millisTime-ctrlmidiTime[i]>=CTRLMIDI_TIME){          //send ctrl change only every 50                                                    
          usbMIDI.sendControlChange(ctrlchange[i], ctrlyvar, MIDI_CHAN); 
          ctrlyvarold = ctrlyvar;    
          Serial.print (" Sending "); 
          Serial.print (ctrlchange[i]);                              
          ctrlmidiTime[i] = millisTime;
      }
    }   
  }  

  if(shiftmode==false){                         //SHIFT OFF PLAY MODE SENDS MIDI MESSAGES
    if(ButtonUp.fallingEdge()) {     
      if (padmode==0) resetScale0(); 
      if (padmode==1) resetScale1();
      padmode++;                                                                      //Leds show bank on button switch when control or midi notes are being sent                                
      if (padmode>1) padmode=0;
      padmodeLeds();   
      resetPress();       
      Serial.print("padmode ");
      Serial.println(padmode);                  
    }
    if(ButtonDown.fallingEdge()) {
      if (padmode==0) resetScale0(); 
      if (padmode==1) resetScale1();
      padmode=padmode-1;                                                              //Leds show bank on button switch when control or midi notes are being sent                                
      if (padmode<0) padmode=1;
      padmodeLeds();   
      resetPress(); 
      Serial.print("padmode ");
      Serial.println(padmode);     
    }

    for(int i=0; i<FSR_N; i++){                                                       //FSR Code 
      if (fsrBounce[i]==false && millisTime-startTime[i]>=bounceTime) {               //sets all bounces to true      
        fsrBounce[i]=true;
      }
      if (fsrReadings[i] <= HIGHTHRESH&&fsrBounce[i]==true){                          //if reading goes below highthress and if bounce is ok and if fsr is not pressed, end note, switch led on,etc.
        if (fsrPress[i]==false){                                                                   
          fsrmidiVol[i] = map (fsrReadings[i], 1024, 0, 0, 127);
          if (padmode==0){
            usbMIDI.sendNoteOn(scale0[i], fsrmidiVol[i], MIDI_CHAN);
            Tlc.set(tlcbluePins[i], highledval);                                      //switch respective led on
            if (fsrReadings[8]<=HIGHTHRESH) {                                         //main fsr has an additional LED not covered by the main led function
              Tlc.set(tlcbluePins[9],highledval);           
            } 
            Tlc.update();
            //delay(DELAY_LEDS);                                                      //tlc chip needs some extra time, without the delay msgs often dont arrive especially in forloops...
          }
          else if (padmode==1) {
            usbMIDI.sendNoteOn(scale1[i], fsrmidiVol[i], 6);
            Tlc.set(tlcgreenPins[i], highledval);                                     //switch respective led on
            if (fsrReadings[8]<=HIGHTHRESH) {                                         //main fsr has an additional LED not covered by the main led function
              Tlc.set(tlcgreenPins[9],highledval);             
            } 
            Tlc.set(tlcbluePins[i], highledval);                                      //switch respective led on
            if (fsrReadings[8]<=HIGHTHRESH) {                                         //main fsr has an additional LED not covered by the main led function
              Tlc.set(tlcbluePins[9],highledval);             
            } 
            Tlc.update();
            //delay(DELAY_LEDS);                                                      //tlc chip needs some extra time, without the delay msgs often dont arrive especially in forloops...
          }
          startTime[i]=millisTime;                                                    //if note is send, activate bounce
          fsrBounce[i]=false;                                                         //fsr is not bounced                                 
          //serial.println(fsrPins[i]); 
        }
        fsrPress[i]=true;                                                             //fsr is  pressed
      }
      else if (fsrReadings[i] > LOWTHRESH) {
        if (fsrPress[i]==true){
          if (padmode==0) {
            usbMIDI.sendNoteOff(scale0[i], 127, MIDI_CHAN);
            Tlc.set(tlcbluePins[i], lowledval);                                       //switch respective led on
            if (fsrReadings[8]>HIGHTHRESH) {                                          //main fsr has an additional LED not covered by the main led function
              Tlc.set(tlcbluePins[9],lowledval);             
            } 
            Tlc.update();
            delay(DELAY_LEDS); 
          }
          else if (padmode==1) {
            usbMIDI.sendNoteOff(scale1[i], 127, 6);
            Tlc.set(tlcbluePins[i], lowledval);                                       //switch respective led on
            if (fsrReadings[8]>HIGHTHRESH) {                                          //main fsr has an additional LED not covered by the main led function
              Tlc.set(tlcbluePins[9],lowledval);             
            } 
            Tlc.set(tlcgreenPins[i], lowledval);                                      //switch respective led on
            if (fsrReadings[8]>HIGHTHRESH) {                                          //main fsr has an additional LED not covered by the main led function
              Tlc.set(tlcgreenPins[9],lowledval);             
            } 
            Tlc.update();
            delay(DELAY_LEDS); 
          }
        }
        fsrPress[i] = false;
      }
    }
  }
}
                                                //XTRA VOIDS
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

void readAccel() {                                                                    //read accel functions
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

void readFsrs(){                                                                      //reads and prints fsr values
  for(int i=0; i<FSR_N; i++){
    fsrReadings[i] = analogRead(fsrPins[i]);
  }
                                      /*for(int i=0;i<FSR_N; i++){
                                        Serial.print (fsrReadings[i]);
                                        Serial.print (" ");
                                      }
                                      Serial.println (" ");*/
}

void readButtons(){                                                                   //reads buttons
  ButtonUp.update();
  ButtonDown.update();
  ButtonShift.update();
}

void padmodeLeds() {                                                                  //the led functions to show the different padmodes
  if (padmode==0) {                                                                   //maybe change color for green and red later...
    clearLeds();
    resetScales();
    for (int i=0; i<TLCBLUEPIN_N; i++)   {
      Tlc.set(tlcbluePins[i], lowledval); 
      Tlc.update(); 
      delay (DELAY_LEDS);                                                             //otherwise the chip cant handle the speed 
    }
  }
  else if (padmode==1) {
    clearLeds();
    resetScales(); 
    for (int i=0; i<TLCGREENPIN_N; i++)   {
      Tlc.set(tlcgreenPins[i], lowledval); 
      Tlc.update();  
      delay (DELAY_LEDS);                    
    }
    for (int i=0; i<TLCBLUEPIN_N; i++)   {
      Tlc.set(tlcbluePins[i], lowledval); 
      Tlc.update(); 
      delay (DELAY_LEDS);                                                             //otherwise the chip cant handle the speed 
    }
  }
}

void clearLeds() {                      
  Tlc.clear();
  Tlc.update();
}

void ctrlLeds() {
  clearLeds();
    for (int i=0; i<TLCBLUEPIN_N; i++)   {
      Tlc.set(tlcredPins[i], lowledval); 
      Tlc.set(tlcgreenPins[i], lowledval);
      Tlc.update(); 
      delay (DELAY_LEDS);                  
    }
}

void resetPress(){
  for(int i=0; i<FSR_N; i++){ 
    fsrPress[i]=false;
  }
}

void resetScale0(){
  for(int i=0; i<FSR_N; i++){ 
    usbMIDI.sendNoteOff(scale0[i], 127, MIDI_CHAN);
  }
}
void resetScale1(){
  for(int i=0; i<FSR_N; i++){ 
    usbMIDI.sendNoteOff(scale1[i], 127, 6);
  }
}
void resetScales(){
  for(int i=0; i<FSR_N; i++){ 
    usbMIDI.sendNoteOff(scale0[i], 127, MIDI_CHAN);
    usbMIDI.sendNoteOff(scale1[i], 127, 6);
  }
}

void heartbeat() {
  Serial.print("still alive ");
  Serial.println(counter);
  counter++;
}


    


