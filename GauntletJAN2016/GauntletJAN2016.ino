
//OR SHIFT? OR 2nd BUTTON FOR CONTROL/NOTE CHANGE ON LONG SENSORS??
//OCTAVE CHANGE WOULD BE NICE... either with buttons switch or fsr combinations..
//other idea is to send ctrl values with long sensors, to change sounds directly while playing, then another bank to switch through those..
//hardware add push buttons to gauntlet.. (for octave changes, ctrl changes 

#include <Bounce.h>
#include <MIDI.h>
#define MIDI_CHAN 2

#define GREEN1 5                      //LEDS
#define BLUE1 4
#define RED1 3
#define GREEN2 9
#define BLUE2 10
#define RED2 6

#define FSRLONG_N 6                  //FSR AND POS SENSOR VARIABLES AND THRESHOLD LEVELS
#define POSLONG_N 6
#define FSRSHORT_N 8 
#define THRESH_SHORT 850
#define THRESH_LONG 900

int fsrlongPins[] = {A0, A1, A2, A3, A4, A5}; //the pins for the long fsr sensors
int poslongPins[] = {A6, A7, A8, A9, A10, A11}; //the pins for the pos sensors
int fsrshortPins[] = {A12, A13, A16, A19, A14, A15, A17, A18}; //the pins for the short fsr sensors
int z[FSRSHORT_N] = {1,1,1,1,1,1,1,1}; //switch variable   for midi toggle
int fsrlongReadings[FSRLONG_N];
int poslongReadings[POSLONG_N];
int fsrshortReadings[FSRSHORT_N];
int poslongMidi[POSLONG_N];
int poslongMidiold[POSLONG_N] = {0,0,0,0,0,0};
int poslongPitch [POSLONG_N];
int posnoteLevel[POSLONG_N];
int posnotePos [POSLONG_N];
int posChange [POSLONG_N];
int fsrshortmidiVol[FSRSHORT_N];
bool fsrshortPress[FSRSHORT_N];
bool fsrlongPress [FSRLONG_N];

int controlVal = 31;                        //smallest control value - 1 (make sure it doesnt conflict with the other midi inst)

//int scalelong[] =  {60, 62, 64,     67, 69,     72};
//int scaleshort[] = {60, 62, 64, 65, 67, 69, 71, 72};   
//int scalelong[] =  {36, 38, 40,     43, 45,     48};
//int scaleshort[] = {36, 38, 40, 41, 43, 45, 47, 48};   //C MAJOR SCALE
int scalelong[] =  {48, 50, 52,     55, 57,     60};
int scaleshort[] = {48, 50, 52, 53, 55, 57, 59, 60};   //C MAJOR SCALE

#define BUTTON1 1                           //BUTTONS AND BOUNCE
#define BUTTON2 0
Bounce ButtonUp = Bounce(BUTTON1, 10);
Bounce ButtonDown = Bounce(BUTTON2, 10);
int bank = 1;
int scalebank = 0;

unsigned long millisTime;  
unsigned long fsrshortstartTime[FSRSHORT_N]; 
bool fsrshortBounce[FSRSHORT_N];
int ctrlbounceTime=300;       
int notebounceTime=30;              

IntervalTimer myTimer;                      //interrupt counter
int counter=0;

void setup() {

  Serial.begin(9600);                       // use the serial port


  myTimer.begin(heartbeat, 10000000);  // Time in microseconds (10s)

// functions called by IntervalTimer should be short, run as quickly as
// possible, and should avoid calling other functions if possible.
  
  Serial.print(" START ");   

  initFsrlongs(); 
  initPoslongs();
  initFsrshorts();
  initButtons();
  initLeds();                               //might not need the init for analogWrites
  bankLeds();

  for(int i=0; i<FSRSHORT_N; i++){ 
    fsrshortPress[i]=false;
    fsrshortBounce[i]=true;
  }
  for(int i=0; i<FSRLONG_N; i++){ 
    fsrlongPress[i]=false; 
  }
  for (int i=0; i<FSRSHORT_N; i++){
    usbMIDI.sendNoteOff(scaleshort[i], 127, MIDI_CHAN);  
  }
    for (int i=0; i<FSRLONG_N; i++){
    usbMIDI.sendNoteOff(scalelong[i], 127, MIDI_CHAN);  
  }
}

void loop() {

  millisTime = millis();                    //time var running

  while (usbMIDI.read()) ;                  // read and discard any incoming MIDI messages
   
  readFsrlongs();
  readPoslongs();
  readFsrshorts();
  readButtons();

  //Serial.println(""); 

  for (int i=0; i<FSRSHORT_N; i++){
    //Serial.print(i);
    //Serial.print(" = "); 
    //Serial.print(z[i]);
    //Serial.print(" ");
  }

  //Serial.println(""); 

  if(ButtonUp.fallingEdge()) {              //3 banks and the buttons switch through them BLUE==MODE0, TURQUOISE==MODE1, RED=MODE2
    bank++;                                 //Leds show bank except when control or midi notes are being sent                                
    if (bank>=2) bank=0;
    bankLeds();   
    resetPress();                           //resets the press booleans on every bank change  
    resetScales();                          //sends midi off for both scales
    usbMIDI.sendPitchBend(8192, MIDI_CHAN); //reset Pitchbend to 0
    for (int i=0; i<FSRSHORT_N; i++){ 
      fsrshortPress[i]=false;
      fsrshortBounce[i]=true;
    }
  }
  if(ButtonDown.fallingEdge()) {
    scalebank++;
    if (scalebank>=2) scalebank=0;
    bankLeds(); 
    resetPress();                           //resets the press booleans on every bank change
    resetScales();                          //sends midi off for both scales
    usbMIDI.sendPitchBend(8192, MIDI_CHAN);     
    clearLeds();
    if(scalebank==0)analogWrite(BLUE1, 10);     
    if(scalebank==1)analogWrite(BLUE2, 10);  
  }

  //Serial.println(bank); 
  //Serial.println(scalebank);

  if (bank==0) {                            //MODE0 Turqouis  CTRL   long and short sensors both send ctrl messages 
                                         
    //Serial.println("MODE0");
    for(int i = 0; i< FSRLONG_N; i++){      //LONG SENSORS send ctrls, pos send 0-127 value
      if (fsrlongReadings[i] <= THRESH_LONG){
        poslongMidi[i] = poslongReadings[i] >> 3;
        if (poslongMidi[i]!=poslongMidiold[i]){ 
          usbMIDI.sendControlChange((i+controlVal), poslongMidi[i], MIDI_CHAN);
          /*
          Serial.print(" CTRL: ");
          Serial.print(i+controlVal);
          Serial.print(" Val: ");
          Serial.println(poslongMidi[i]);
          */
          poslongMidiold[i]=poslongMidi[i];
        }
        fsrlongPress[i]=true;
        //int ledcntrlblue = map (poslongReadings[i], 0, 1023, 0, 255);
        int ledcntrlblue = poslongReadings[i] >> 2;
        analogWrite(BLUE1, ledcntrlblue);                                       //led animations
        analogWrite(BLUE2, ledcntrlblue);
        Serial.print(" ledcntrlblue" );
        Serial.println(ledcntrlblue);
        int lednotegreen = map (poslongReadings[i], 1023, 0, 0, 255);
        analogWrite(GREEN1, lednotegreen);
        analogWrite(GREEN2, lednotegreen);
      }
      else if (fsrlongReadings[i] > THRESH_LONG && fsrlongPress[i]==true) {     //this function is only there to shut the Leds off
        fsrlongPress[i] = false;
        clearLeds();
        bankLeds();
      }
    }  
    for (int i=0; i<FSRSHORT_N; i++){                                           //SHORT SENSORS send control values
      if (fsrshortBounce[i]==false && millisTime-fsrshortstartTime[i]>=ctrlbounceTime) {                  
        fsrshortBounce[i]=true;
      }
      if (fsrshortReadings[i]<=THRESH_SHORT && fsrshortBounce[i]==true){
        if(!fsrshortPress[i]) {
          if (z[i]==0) { 
            usbMIDI.sendControlChange((i+FSRLONG_N+controlVal), 0, MIDI_CHAN);
          }
          else if (z[i]==1){
            usbMIDI.sendControlChange((i+FSRLONG_N+controlVal), 127, MIDI_CHAN);
          }
          z[i]=z[i]+1;
          if (z[i]>1) z[i]=0;
          Serial.println( z[i] );
          fsrshortstartTime[i]=millisTime;                                                    //if note is send, activate bounce
          fsrshortBounce[i]=false;   
        } 
        fsrshortPress[i]=true;
        if (z[i]==0) { 
            usbMIDI.sendControlChange((i+FSRLONG_N+controlVal), 0, MIDI_CHAN);
            analogWrite(BLUE1, 255);
            analogWrite(GREEN1, 255);
          }
          else if (z[i]==1){
            usbMIDI.sendControlChange((i+FSRLONG_N+controlVal), 127, MIDI_CHAN);
            analogWrite(BLUE2, 255);
            analogWrite(GREEN2, 255);
          }  
      }
      else if (fsrshortReadings[i] > THRESH_SHORT && fsrshortPress[i]==true) {
        fsrshortPress[i] = false;
        clearLeds();
        bankLeds();
        //Serial.println(String(" CTRL ")+(i+FSRLONG_N+controlVal)+(" LedOff "));
      }
    }
  }
  
  else if (bank==1) {                                                           //MODE0=1 BLUE send notes plus bend on pos sensors. fsr  shorts send notes
 
     //Serial.println("MODE1"); 
    for(int i=0; i<FSRSHORT_N; i++){                                            //SHORT SENSORS
      if (fsrshortBounce[i]==false && millisTime-fsrshortstartTime[i]>=notebounceTime) {                  
        fsrshortBounce[i]=true;
      }
        if (fsrshortReadings[i]<=THRESH_SHORT  && fsrshortBounce[i]==true){
          if(!fsrshortPress[i]) {
            usbMIDI.sendPitchBend(8192, MIDI_CHAN);                                                   //OR HERE??????
            fsrshortmidiVol[i] = map (fsrshortReadings[i], 1024, 0, 0, 127);
            usbMIDI.sendNoteOn(scaleshort[i]+(scalebank*12), fsrshortmidiVol[i], MIDI_CHAN);
            analogWrite(BLUE1, 255);
            analogWrite(BLUE2, 255);
            fsrshortBounce[i]=false;
            fsrshortstartTime[i]=millisTime;
          } 
          fsrshortPress[i]=true;
        }
        else if (fsrshortReadings[i] > THRESH_SHORT && fsrshortPress[i]==true && fsrshortBounce[i]==true) {
          fsrshortPress[i] = false;
          usbMIDI.sendNoteOff(scaleshort[i]+(scalebank*12), 127, MIDI_CHAN);
          fsrshortstartTime[i]=millisTime;                                                                                       //if note is send, activate bounce
          fsrshortBounce[i]=false;   
          clearLeds();
          bankLeds();
          //Serial.println(String(" Note ")+(scaleshort[i])+(" LedOff "));
        }
    }
      
    for(int i = 0; i< FSRLONG_N; i++){      //LONG SENSORS send ctrls, pos send 0-127 value
      if (fsrlongReadings[i] <= THRESH_LONG && i!=3){
        poslongMidi[i] = poslongReadings[i] >> 3;
        if (poslongMidi[i]!=poslongMidiold[i]){ 
          usbMIDI.sendControlChange((i+FSRLONG_N+FSRSHORT_N+controlVal), poslongMidi[i], MIDI_CHAN);
          /*
          Serial.print(" CTRL: ");
          Serial.print(i+controlVal);
          Serial.print(" Val: ");
          Serial.println(poslongMidi[i]);
          */
          poslongMidiold[i]=poslongMidi[i];
        }
        fsrlongPress[i]=true;
        //int ledcntrlblue = map (poslongReadings[i], 0, 1023, 0, 255);
        int ledcntrlblue = poslongReadings[i] >> 2;
        analogWrite(BLUE1, ledcntrlblue);                                       //led animations
        analogWrite(BLUE2, ledcntrlblue);
        Serial.print(" ledcntrlblue" );
        Serial.println(ledcntrlblue);
      }
      else if (fsrlongReadings[i] <= THRESH_LONG && i==3){                       //exception for pitch shift on fourth long sensor
        if (fsrlongPress[i]==false){   
        posnoteLevel[i] = map (fsrlongReadings[i], THRESH_LONG, 0, 0, 127); 
        posnotePos[i] = poslongReadings[i]; 
        analogWrite(BLUE1, 255);                                        
        analogWrite(BLUE2, 255);
        }
        fsrlongPress[i]=true;
        posChange[i] = (poslongReadings[i] - posnotePos[i]) * 16; 
        poslongPitch[i] = 8191 + posChange[i];
        poslongPitch[i] = constrain(poslongPitch[i],0,16383);
        usbMIDI.sendPitchBend(poslongPitch[i], MIDI_CHAN);
        Serial.print(" PosChange: ");
        Serial.print(posChange[i]);
        int lednotered = abs(posChange[i]>>6);      
        Serial.print(" Redvar: ");
        Serial.println(lednotered); 
        analogWrite(RED1, lednotered);                                        
        analogWrite(RED2, lednotered); 
      }
      else if (fsrlongReadings[i] > THRESH_LONG && fsrlongPress[i]==true && i!=3) {     //this function is only there to shut the Leds off
        fsrlongPress[i] = false;
        clearLeds();
        bankLeds();
      }
      else if (fsrlongReadings[i]>THRESH_LONG && fsrlongPress[i] == true && i==3) {
        fsrlongPress[i] = false;
        usbMIDI.sendPitchBend(8192, MIDI_CHAN);
        clearLeds();
        bankLeds();                                           
      }  
    }    
  }
  
    
}
  

//ADDITIONAL FUNCTIONS

void initFsrlongs(){
  pinMode(A0, INPUT_PULLUP); 
  pinMode(A1, INPUT_PULLUP); 
  pinMode(A2, INPUT_PULLUP); 
  pinMode(A3, INPUT_PULLUP); 
  pinMode(A4, INPUT_PULLUP); 
  pinMode(A5, INPUT_PULLUP); 
}

void initPoslongs(){                                                            //there are external pull_up resistors on the remaining A pins
  pinMode(A6, INPUT_PULLUP); 
  pinMode(A7, INPUT_PULLUP); 
  pinMode(A8, INPUT_PULLUP); 
  pinMode(A9, INPUT_PULLUP); 
}

void initFsrshorts(){
  pinMode(A15, INPUT_PULLUP); 
  pinMode(A16, INPUT_PULLUP); 
  pinMode(A17, INPUT_PULLUP); 
  pinMode(A18, INPUT_PULLUP); 
  pinMode(A19, INPUT_PULLUP); 
  pinMode(A20, INPUT_PULLUP); 
}

void initButtons() {
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);
} 

void initLeds() {
  pinMode(BLUE1, OUTPUT);
  pinMode(GREEN1, OUTPUT);
  pinMode(RED1, OUTPUT);
  pinMode(BLUE2, OUTPUT);
  pinMode(GREEN2, OUTPUT);
  pinMode(RED2, OUTPUT);
}

void clearLeds() {
  analogWrite(BLUE1, 0); 
  analogWrite(BLUE2, 0);
  analogWrite(RED1, 0);
  analogWrite(RED2, 0);
  analogWrite(GREEN1, 0);
  analogWrite(GREEN2, 0);
}

void bankLeds() {
    if (bank==0) {
      clearLeds();
      analogWrite(BLUE1, 7);
      analogWrite(BLUE2, 7);
      analogWrite(GREEN1,7);
      analogWrite(GREEN2,7);
    }
    else if (bank==1) {
      clearLeds();
      analogWrite(BLUE1, 10);
      analogWrite(BLUE2, 10);

    }
}

void readFsrlongs(){
  for(int i = 0; i< FSRLONG_N; i++){
    fsrlongReadings[i] = analogRead(fsrlongPins[i]);
  }
  for(int i = 0; i< FSRLONG_N; i++){
    //Serial.print (fsrlongReadings[i]);
    //Serial.print (" ");
  }
}

void readPoslongs(){
  for(int i = 0; i< POSLONG_N; i++){
    poslongReadings[i] = analogRead(poslongPins[i]);
  }
  for(int i = 0; i< POSLONG_N; i++){
    //Serial.print (poslongReadings[i]);
    //Serial.print (" ");
  }
}

void readFsrshorts(){
  for(int i=0; i<FSRSHORT_N; i++){
    fsrshortReadings[i] = analogRead(fsrshortPins[i]);
  }
  for(int i=0;i<FSRSHORT_N; i++){
    //Serial.print (fsrshortReadings[i]);
    //Serial.print (" ");
  }
}

void readButtons(){
  ButtonUp.update();
  ButtonDown.update();
}

void resetPress(){
  for(int i=0; i<FSRSHORT_N+12; i++){ 
    fsrshortPress[i]=false;
  }
  for(int i=0; i<FSRLONG_N+12; i++){ 
    fsrlongPress[i]=false;
  }
}

void resetScales(){
    for (int i=0; i<FSRSHORT_N; i++){
    usbMIDI.sendNoteOff(scaleshort[i], 127, MIDI_CHAN);  
    delay(5);
  }
      for (int i=0; i<FSRSHORT_N; i++){
    usbMIDI.sendNoteOff(scaleshort[i]+12, 127, MIDI_CHAN); 
    delay(5); 
  }
    for (int i=0; i<FSRLONG_N; i++){
    usbMIDI.sendNoteOff(scalelong[i], 127, MIDI_CHAN);  
    delay(5);
  }
    for (int i=0; i<FSRLONG_N; i++){
    usbMIDI.sendNoteOff(scalelong[i]+12, 127, MIDI_CHAN); 
    delay(5); 
  }

}


void heartbeat() {
  Serial.print("still alive ");
  Serial.println(counter);
  counter++;
}





    /*
    for(int i=0; i<FSRLONG_N; i++) {                                           //sends pitch bend along right away, makes playing quite difficult...
      if (fsrlongReadings[i]<=THRESH_LONG) { 
        if (fsrlongPress[i]==false){   
          delay(20);
          poslongReadings[i] = analogRead(poslongPins[i]);
          posnoteLevel[i] = map (fsrlongReadings[i], THRESH_LONG, 0, 0, 127); 
          usbMIDI.sendNoteOn(scalelong[i], posnoteLevel[i], MIDI_CHAN);     
          }
        fsrlongPress[i] = true;
        poslongPitch[i] = map (poslongReadings[i], 0, 1023, 0, 16383); 
        usbMIDI.sendPitchBend(poslongPitch[i], MIDI_CHAN);
        int lednoteblue = map (poslongReadings[i], 0, 1023, 255, 0);
        int lednotegreen = map (poslongReadings[i], 0, 1023, 0, 255);
        analogWrite(BLUE1, lednoteblue);                                       //led animations
        analogWrite(BLUE2, lednoteblue);
        analogWrite(GREEN1, lednotegreen);
        analogWrite(GREEN2, lednotegreen);
      } 
      else if (fsrlongReadings[i]>THRESH_LONG && fsrlongPress[i] == true) {
        usbMIDI.sendNoteOff(scalelong[i], 127, MIDI_CHAN);   
        usbMIDI.sendPitchBend(8192, MIDI_CHAN); 
        fsrlongPress[i] = false;
        clearLeds();
        bankLeds();      
      }      
    }
    */

