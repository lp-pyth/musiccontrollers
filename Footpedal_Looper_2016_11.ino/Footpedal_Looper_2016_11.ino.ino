                                   //LCD & SHIELD Libraries
  #include <Wire.h>
  #include <Adafruit_MCP23017.h>
  #include <Adafruit_RGBLCDShield.h>
  Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();
  //#include <usb_keyboard.h>
                                    // LCD backlight color
  #define RED 0x1
  #define YELLOW 0x3
  #define GREEN 0x2
  #define TEAL 0x6
  #define BLUE 0x4
  #define VIOLET 0x5
  #define WHITE 0x7
                                    //Pushbuttons Debounce Library
  #include <Bounce.h>
                                    //Midi Library
  #include <MIDI.h>
  const int MIDI_CHAN = 5;

                                    //buttons on following pins, left to right, top to bottom: {2,3,4,5,/*6,*/7,8,9,10,/*11,*/12,20,21,22,/*23*/}
  #define BUTTONBANK 6              //bankswitch button
  #define BUTTONSTOP 11             //stop button (not part of bank)
  #define BUTTONPLAY 23             //play (not part of bank)
  int buttonaPins[] =   {  2,  4,  7,  9,   12,   21};  //button group a, responsible for sending start go and stop ctrl msgs
  int buttonbPins[] =   {  3,  5,  8,  10,   20,   22};  //button group b, responsible for sending clear and del msgs
  #define BOUNCE_T 80               //bounce time in ms

  #define CTRLBUTTONSA_N 6          //ctrl A buttons used to trigger loopers
  #define CTRLBUTTONSB_N 6          //ctrl B buttons used to redo/clear
  #define CTRL_PLAY 65              //begin of control values available to footpedal (71-128)
  #define CTRL_STOP 66              
  #define CTRL_RACK 67              //CTRL value for instrument rack changes
  #define CTRL_RECSTART 68          //start of ctrl values for rec buttons (number up in forloop)
  #define CTRL_STOPSTART 80         //start of ctrl values for stop buttons (number up in forloop)
  #define CTRL_CLEARSTART 92        //start of ctrl values for clear buttons (number up in forloop)
  #define CTRL_TRACKSTART 104       //start of ctrl values for track on/off buttons (number up in forloop)
  #define CTRL_UNDOSTART 116        //start of ctrl values for undo buttons (number up in forloop)


  
  #define NUM_CTRLBANKS 2               //bank variables
  #define NUM_RACKBANKS 2   
  int ctrlbank = 0;                     //two banks, 0 and 1
  int rackbank = 0;
  int ctrlbankadd = 0;                  //add variable, bankadd = bank * CTRLBUTTONS_N
  int rackbankadd = 0;
  bool bankBool = false;
  int holdTime = 1500;
  int doubleTime = 300;
  int rackvar = 0;
  bool racktimeBool = false;
   
  unsigned long millisTime=0;         //time counter
  unsigned long bankTime=0;  
  unsigned long clearallTime=0;
  bool stoppressBool=false; 
  bool clearallBool=false;   
  unsigned long alltracksonTime=0;
  bool playpressBool=false; 
  bool alltracksonBool=false;   
         
  bool recaBool[CTRLBUTTONSA_N] = {true, true, true, true, true, true};
  bool recbBool[CTRLBUTTONSA_N] = {true, true, true, true, true, true};
  bool recapressBool[CTRLBUTTONSA_N] = {false, false, false, false, false, false};
  bool recbpressBool[CTRLBUTTONSA_N] = {false, false, false, false, false, false};
  
  bool doubleaBool[CTRLBUTTONSA_N] = {false, false, false, false, false, false};
  bool doublebBool[CTRLBUTTONSB_N] = {false, false, false, false, false, false};
  bool doubleapressBool[CTRLBUTTONSA_N] = {false, false, false, false, false, false};
  bool doublebpressBool[CTRLBUTTONSB_N] = {false, false, false, false, false, false};
  bool doubleacountBool[CTRLBUTTONSA_N] = {false, false, false, false, false, false};
  bool doublebcountBool[CTRLBUTTONSB_N] = {false, false, false, false, false, false};
  unsigned long doubleaTime[CTRLBUTTONSA_N]={0,0,0,0,0,0};
  unsigned long doublebTime[CTRLBUTTONSB_N]={0,0,0,0,0,0};   
  int x=0;
  
  bool holdaBool[CTRLBUTTONSA_N] = {false, false, false, false, false, false};
  bool holdbBool[CTRLBUTTONSB_N] = {false, false, false, false, false, false};
  bool holdapressBool[CTRLBUTTONSA_N] = {false, false, false, false, false, false};
  bool holdbpressBool[CTRLBUTTONSB_N] = {false, false, false, false, false, false};
  unsigned long holdaTime[CTRLBUTTONSA_N]={0,0,0,0,0,0};
  unsigned long holdbTime[CTRLBUTTONSB_N]={0,0,0,0,0,0};  

  IntervalTimer myTimer;
  int counter=0;
  
  Bounce buttonBank = Bounce(BUTTONBANK, BOUNCE_T);                  
  Bounce buttonStop = Bounce(BUTTONSTOP, BOUNCE_T);
  Bounce buttonPlay = Bounce (BUTTONPLAY, BOUNCE_T);
  
  Bounce buttonsA[6] = {
    Bounce (buttonaPins[0], BOUNCE_T), 
    Bounce (buttonaPins[1], BOUNCE_T), 
    Bounce (buttonaPins[2], BOUNCE_T), 
    Bounce (buttonaPins[3], BOUNCE_T),
    Bounce (buttonaPins[4], BOUNCE_T),
    Bounce (buttonaPins[5], BOUNCE_T),
  };

  Bounce buttonsB[6] = {
    Bounce (buttonbPins[0], BOUNCE_T), 
    Bounce (buttonbPins[1], BOUNCE_T), 
    Bounce (buttonbPins[2], BOUNCE_T), 
    Bounce (buttonbPins[3], BOUNCE_T),
    Bounce (buttonbPins[4], BOUNCE_T),
    Bounce (buttonbPins[5], BOUNCE_T),
  };
  
void setup(){  
                             
  Serial.begin(9600);

  myTimer.begin(heartbeat, 10000000);  // Time in microseconds (10s)

// functions called by IntervalTimer should be short, run as quickly as
// possible, and should avoid calling other functions if possible.

  
  pinMode (BUTTONBANK, INPUT_PULLUP);
  pinMode (BUTTONSTOP, INPUT_PULLUP);
  pinMode (BUTTONPLAY, INPUT_PULLUP);
  
  pinMode(buttonaPins[0], INPUT_PULLUP);
  pinMode(buttonaPins[1], INPUT_PULLUP);
  pinMode(buttonaPins[2], INPUT_PULLUP);
  pinMode(buttonaPins[3], INPUT_PULLUP);
  pinMode(buttonaPins[4], INPUT_PULLUP);
  pinMode(buttonaPins[5], INPUT_PULLUP);
  
  pinMode(buttonbPins[0], INPUT_PULLUP);
  pinMode(buttonbPins[1], INPUT_PULLUP);
  pinMode(buttonbPins[2], INPUT_PULLUP);
  pinMode(buttonbPins[3], INPUT_PULLUP);
  pinMode(buttonbPins[4], INPUT_PULLUP);
  pinMode(buttonbPins[5], INPUT_PULLUP);
                          
  lcd.begin(16, 2);                           //LCD START
 
  lcd.clear();            
  lcd.setCursor(0, 0);
  lcd.print("NODE STATION");
 
  Serial.println("serial start"); 
}

void loop(){

  millisTime = millis();                      //time var running WHATS THE MAX MILLIS VALUE?
  
  while (usbMIDI.read());   

  readButtons();
  
  if(buttonBank.fallingEdge() && racktimeBool==false) {     //bank button pressed and racktimeBool is false
    ctrlbank++;                                             //ctrl back goes up
    ctrlbank %= NUM_CTRLBANKS;                              //until it reaches the max amount of ctrlbanknumbers, then it starts from 0
    ctrlbankadd = ctrlbank * CTRLBUTTONSA_N;                //to send the right MIDI CTRL value, cotrlbankadd is used
    bankTime=millisTime;                                    //bankTime starts counting (to possibly go from ctrl to rack mode if holdTime is reacher (1500ms))
    bankBool=true;
    Serial.print("CTRL Bank ");
    Serial.println(ctrlbank);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("CTRL Bank ");
    lcd.print(ctrlbank);
    lcd.setCursor(15, 1);
    lcd.print(ctrlbank);
  }
  if (buttonBank.risingEdge()) {
      bankBool=false;
    }
  if (millisTime-bankTime>=holdTime && bankBool==true && racktimeBool==false) {       //RACK MODE
      bankBool=false;
      ctrlbank=ctrlbank-1;
      if (ctrlbank<0) ctrlbank=1;
      ctrlbankadd = ctrlbank * CTRLBUTTONSA_N;
      racktimeBool = true;
      Serial.println ("Rack Time");
                            lcd.clear();
                            lcd.setCursor(0,0);
                            lcd.print("RACKTIME BABY");
      rackbank=0;
    }
    
  if (racktimeBool==true){                          //RACKTTIME BABY time to choose your rack
    if(buttonBank.fallingEdge()) {    
      rackbank++;
      rackbank %= NUM_RACKBANKS;
      Serial.print("Rack Bank ");
      Serial.println(rackbank);
                            lcd.clear();
                            lcd.setCursor(0, 0);
                            lcd.print("Rack Bank ");
                            lcd.print(rackbank);
      rackbankadd = rackbank * (CTRLBUTTONSA_N+CTRLBUTTONSB_N);
      bankTime=millisTime;
      bankBool=true;
    }
    if (buttonBank.risingEdge()) {
        bankBool=false;
      }
    if (millisTime-bankTime>=holdTime && bankBool==true && racktimeBool==true) {
        bankBool=false;
        racktimeBool = false;
        rackbank=rackbank-1;
        Serial.println ("Control Time");
                            lcd.clear();
                            lcd.setCursor(0,0);
                            lcd.print("YOU GOT CONTROL!");
                            lcd.setCursor(15, 1);                     //always show bank in bottom right corner of LCD
                            lcd.print(ctrlbank);
    }
    if (buttonStop.fallingEdge()) {                                //in rackmode the play and stop button control the rack up or down movement
      rackvar = rackvar+1;
      if (rackvar>23) rackvar=0;
      sendrackVar();
    }
    if (buttonPlay.fallingEdge()) {
      rackvar = rackvar-1;
      if (rackvar<0) rackvar=23;
      sendrackVar();
    }
    for(int i=0; i<CTRLBUTTONSA_N; i++){   
      if (buttonsA[i].fallingEdge()) {
      rackvar=(i*2)+rackbankadd;   
      sendrackVar();  
      }
    }
    for(int i=0; i<CTRLBUTTONSB_N; i++){ 
      if (buttonsB[i].fallingEdge()) {
      rackvar=(i*2)+rackbankadd+1;
      sendrackVar();
      }
    }  
  }

  else if (racktimeBool==false){                    //CONTROL TIME time to send looper controls 











  
    if (buttonStop.fallingEdge() && stoppressBool==false) {     //if press is registered and button is not pressed
      clearallTime=millisTime;                                  //start clear all counter
      stoppressBool=true;                                       //button is pressed
      clearallBool=true;                                        //expect clear all
    }
    if (buttonStop.risingEdge() && stoppressBool==true && clearallBool==true) {       //if button is raised and it was pressed before and clearall was expected
      // DONT SEND STOP MESSAGE TRY OUT BE ABLE TO STOP ALL LOOPERS BUT NEVER THE CLICK THE CLICK CLICKS FOREVER
      
      //usbMIDI.sendControlChange((CTRL_STOP), 127, MIDI_CHAN);                         //send a stop message
      //usbMIDI.sendControlChange((CTRL_STOP), 0, MIDI_CHAN);
      
      for(int i=0; i<12; i++){   
        usbMIDI.sendControlChange(CTRL_STOPSTART+i, 127, MIDI_CHAN);                  //we send stop messages to all tracks
      }
      
      stoppressBool=false;                                                            //but is not pressed anymore
      clearallBool=false;                                                             //we do not exect a clearall
                          lcd.clear();
                          lcd.setCursor(0,0);
                          lcd.print("Stop ");
                          Serial.println(String("ch=") + MIDI_CHAN + (", Control=") + (CTRL_STOP) + ("Velocity=127"));
                          Serial.println(String("ch=") + MIDI_CHAN + (", Control=") + (CTRL_STOP) + ("Velocity=0")); 
                          lcd.setCursor(15, 1);                                       //always show bank in bottom right corner of LCD
                          lcd.print(ctrlbank);
    }
    if (millisTime-clearallTime>=holdTime && clearallBool==true && stoppressBool==true) {  //if time has run out and we still expect a clearall and the button is pressed
      for(int i=0; i<12; i++){   
        usbMIDI.sendControlChange(CTRL_CLEARSTART+i, 127, MIDI_CHAN);                  //we send clearall messages
      }
            clearallBool=false;                                                        //we do not expect a clearall
                          lcd.clear();
                          lcd.setCursor(0,0);
                          lcd.print("Loopers cleared");
                          lcd.setCursor(15, 1);                     //always show bank in bottom right corner of LCD
                          lcd.print(ctrlbank);
    }
    if (buttonStop.risingEdge() && stoppressBool==true && clearallBool==false) {       //if button is released and was pressed and we do not expect a clearall
      stoppressBool=false;                                                             //button is not pressed anymore
      for(int i=0; i<12; i++){   
      usbMIDI.sendControlChange(CTRL_CLEARSTART+i, 0, MIDI_CHAN);
      delay(3);               
      }
    }





      
    
    if (buttonPlay.fallingEdge() && playpressBool==false) {
      alltracksonTime=millisTime;
      playpressBool=true;
      alltracksonBool=true;
    }
    if (buttonPlay.fallingEdge() && playpressBool==true && alltracksonBool==true) {
      //usbMIDI.sendControlChange((CTRL_PLAY), 127, MIDI_CHAN);
      //usbMIDI.sendControlChange((CTRL_PLAY), 0, MIDI_CHAN);
      for (int u = 0; u < 12; u++) {                                                                    //ALL LOOP TRACKS ON
        usbMIDI.sendControlChange(CTRL_TRACKSTART + u, 127, MIDI_CHAN);                                 //we send a 127 track value to all tracks
        Serial.println(String(" ch= ") + 5 + (", Control = ") + (CTRL_TRACKSTART + u) + (" Velocity = 127 "));
        Serial.println("All Tracks On");
      }
                          lcd.clear();
                          lcd.setCursor(0,0);
                          lcd.print("Tracks On ");
                          lcd.setCursor(15, 1);                     //always show bank in bottom right corner of LCD
                          lcd.print(ctrlbank);
      Serial.println(String("ch=") + MIDI_CHAN + (", Control=") + (CTRL_PLAY) + ("Velocity=127"));
    }    
    //if (buttonPlay.risingEdge()) {
    //  usbMIDI.sendControlChange((CTRL_PLAY), 0, MIDI_CHAN);  
    //  Serial.println(String("ch=") + MIDI_CHAN + (", Control=") + (CTRL_PLAY) + ("Velocity=0"));
    //}





/*
 * 
 */





    
    for(int i=0; i<CTRLBUTTONSA_N; i++){                          //BUTTON GROUP A
      if (buttonsA[i].fallingEdge() && recaBool[i]==true) {                           //if ctrlabutton is pressed and if we are waiting for a rec 127
        usbMIDI.sendControlChange((CTRL_RECSTART+ctrlbankadd+i), 127, MIDI_CHAN);     //we send a rec ctrl message
        recaBool[i]=false;                                                            //we are not waiting for a rec 127
        recapressBool[i]=true;                                                        //we are ready to send the 0 value for rec
        holdaTime[i]=millisTime;                                                      //hold time is starting
        holdaBool[i]=true;                                                            //we are waiting for a hold

                          Serial.println(String(" ch= ") + MIDI_CHAN + (", Control = ") + (CTRL_RECSTART+ctrlbankadd+i) + (" Velocity = 127 "));
                          lcd.clear();
                          lcd.setCursor(0,0);
                          lcd.print("REC CTRL ");
                          lcd.print(CTRL_RECSTART+ctrlbankadd+i);
                          lcd.setCursor(0,1);
                          lcd.print("127");
                          lcd.setCursor(15, 1);                     //always show bank in bottom right corner of LCD
                          lcd.print(ctrlbank);
      }
      if (buttonsA[i].risingEdge() && recapressBool[i]==true) {                      //if the button gets released and if we were ready for a 0 rec value
        usbMIDI.sendControlChange(CTRL_RECSTART+ctrlbankadd+i, 0, MIDI_CHAN);        //we send the 0 rec value
        recapressBool[i]=false;                                                      //we are not ready for a rec 0
        doubleaBool[i]=true;                                                         //we are ready for a double 127
        doubleaTime[i]=millisTime;                                                   //double press time is starting
        doubleacountBool[i]=true;
        holdaBool[i]=false;                                                          //we are not ready for a hold 127
                          Serial.println(String(" ch= ") + MIDI_CHAN + (", Control = ") + (CTRL_RECSTART+ctrlbankadd+i) + (" Velocity = 0 MOMENTARY "));
                          lcd.clear();
                          lcd.setCursor(0,0);
                          lcd.print("REC CTRL ");
                          lcd.print (CTRL_RECSTART+ctrlbankadd+i);
                          lcd.setCursor(0,1);
                          lcd.print("0");
                          lcd.setCursor(15, 1);                     //always show bank in bottom right corner of LCD
                          lcd.print(ctrlbank);
      }
      if (millisTime-doubleaTime[i]>doubleTime && doubleacountBool[i]==true){                                        //if double press time has run out  
        recaBool[i]=true;                                                               //we are ready for a rec 127  
        doubleaBool[i]=false;                                                           //we are not ready for a double 127
        doubleacountBool[i]=false;
      }
      else if (buttonsA[i].fallingEdge() && millisTime-doubleaTime[i]<=doubleTime && doubleaBool[i]==true){     //if double press has not run out and we are waiting for a double 127
        usbMIDI.sendControlChange(CTRL_STOPSTART+ctrlbankadd+i, 127, MIDI_CHAN);                                //we send a double press stop ctrl value
        doubleaBool[i]=false;                                                                                   //we are not ready for a double 127
        doubleapressBool[i]=true;                                                                               //we are ready for a double 0 value
        holdaTime[i]=millisTime;                                                                                //we are again starting the hold clock
        holdaBool[i]=true;                                                                                      //we are waiting for a hold 127
                          Serial.println(String(" ch= ") + MIDI_CHAN + (", Control = ") + (CTRL_STOPSTART+ctrlbankadd+i) + (" Velocity = 127 "));
                          lcd.clear();
                          lcd.setCursor(0,0);
                          lcd.print("STOP CTRL ");
                          lcd.print(CTRL_STOPSTART+ctrlbankadd+i);
                          lcd.setCursor(0,1);
                          lcd.print("127");
                          lcd.setCursor(15, 1);                     //always show bank in bottom right corner of LCD
                          lcd.print(ctrlbank);
      }
      if (buttonsA[i].risingEdge()&& doubleapressBool[i]==true){                                                //if we are ready for a double0 and it happens
        usbMIDI.sendControlChange(CTRL_STOPSTART+ctrlbankadd+i, 0, MIDI_CHAN);                                    //we send a 0 stop value
        recaBool[i]=true;                                                                                         //we are ready for a rec 127  
        doubleapressBool[i]=false;                                                                                //we are not ready for a double 0
        holdaBool[i]=false;                                                                                       //we are not anymore waiting for a hold  
                          Serial.println(String(" ch= ") + MIDI_CHAN + (", Control = ") + (CTRL_STOPSTART+ctrlbankadd+i) + (" Velocity = 0 "));
                          lcd.clear();
                          lcd.setCursor(0,0);
                          lcd.print("STOP CTRL ");
                          lcd.print(CTRL_STOPSTART+ctrlbankadd+i);
                          lcd.setCursor(0,1);
                          lcd.print("0");  
                          lcd.setCursor(15, 1);                     //always show bank in bottom right corner of LCD
                          lcd.print(ctrlbank);        
      }                                                        
      if (millisTime-holdaTime[i]>=holdTime && holdaBool[i]==true) {                                             //if we are ready for a hold 127 and it happens
        usbMIDI.sendControlChange(CTRL_STOPSTART+ctrlbankadd+i, 0, MIDI_CHAN);                                   //we send a 0 stop value 
        delay (4);
        usbMIDI.sendControlChange(CTRL_RECSTART+ctrlbankadd+i, 0, MIDI_CHAN);                                    //we send the 0 rec value
        delay (4);
        usbMIDI.sendControlChange(CTRL_CLEARSTART+ctrlbankadd+i, 127, MIDI_CHAN);                                //we send the clear message
        holdaBool[i]=false;                                                                                      //we are not anymore waiting for a hold127
        holdapressBool[i]=true;                                                                                  //we are ready for a hold0
                          Serial.println(String(" ch= ") + MIDI_CHAN + (", Control = ") + (CTRL_CLEARSTART+ctrlbankadd+i) + (" Velocity = 127"));
                          lcd.clear();
                          lcd.setCursor(0,0);
                          lcd.print("CLEAR CTRL ");
                          lcd.print(CTRL_CLEARSTART+ctrlbankadd+i);
                          lcd.setCursor(0,1);
                          lcd.print("127");  
     }
      if (buttonsA[i].risingEdge()&& holdapressBool[i]==true) {                                                   //if we are ready for a hold0 and the buttons rises
         usbMIDI.sendControlChange(CTRL_CLEARSTART+ctrlbankadd+i, 0, MIDI_CHAN);                                  //we send the 0 clear message
         recaBool[i]=true;
         doubleaBool[i]=false;
         holdapressBool[i]=false;
                          Serial.println(String(" ch= ") + MIDI_CHAN + (", Control = ") + (CTRL_CLEARSTART+ctrlbankadd+i) + (" Velocity = 0 "));
                          lcd.clear();
                          lcd.setCursor(0,0);
                          lcd.print("CLEAR CTRL ");
                          lcd.print(CTRL_CLEARSTART+ctrlbankadd+i);
                          lcd.setCursor(0,1);
                          lcd.print("0");  
                          lcd.setCursor(15, 1);                     //always show bank in bottom right corner of LCD
                          lcd.print(ctrlbank);
      }         
    }

    
    for(int i=0; i<CTRLBUTTONSB_N; i++){                          //BUTTON GROUP B  TRACK ON/OFF and UNDO
      if (buttonsB[i].fallingEdge() && recbBool[i]==true) {                           //if ctrlbbutton is pressed and if we are waiting for a rec 127
        recbBool[i]=false;                                                            //we are not waiting for a rec 127
        recbpressBool[i]=true;                                                        //we are ready to send the 0 value for rec
        holdbTime[i]=millisTime;                                                      //hold time is starting
        holdbBool[i]=true;                                                            //we are waiting for a hold
        
      }
      if (buttonsB[i].risingEdge() && recbpressBool[i]==true) {                      //if the button gets release        
        recapressBool[i]=false;                                                      //we are not ready for a rec 0
        doublebBool[i]=true;                                                         //we are ready for a double 127
        doublebTime[i]=millisTime;                                                   //double press time is starting
        doublebcountBool[i]=true;
        holdbBool[i]=false;                                                          //we are not ready for a hold 127
      }
      if (millisTime-doublebTime[i]>doubleTime && doublebcountBool[i]==true){        //if double press time has run out  
        recbBool[i]=true;                                                            //we are ready for a rec 127  
        doublebBool[i]=false;                                                        //we are not ready for a double 127
        doublebcountBool[i]=false;
                          lcd.clear();
                          lcd.setCursor(0,0);
                          lcd.print("HAA, TOO SLOW!!!");
                          lcd.setCursor(15, 1);                     //always show bank in bottom right corner of LCD
                          lcd.print(ctrlbank);
      }
      else if (buttonsB[i].fallingEdge() && millisTime-doublebTime[i]<=doubleTime && doublebBool[i]==true){     //STOP if double press has not run out and we are waiting for a double 127
        //usbMIDI.sendControlChange(CTRL_TRACKSTART+ctrlbankadd+i, 127, MIDI_CHAN);                               //we send a double press stop ctrl value
        doublebBool[i]=false;                                                                                   //we are not ready for a double 127
        doublebpressBool[i]=true;                                                                               //we are ready for a double 0 value
        holdbTime[i]=millisTime;                                                                                //we are again starting the hold clock
        holdbBool[i]=true;                                                                                      //we are waiting for a hold 127
        doublebcountBool[i]=false;
                          /*Serial.println(String(" ch= ") + MIDI_CHAN + (", Control = ") + (CTRL_TRACKSTART+ctrlbankadd+i) + (" Velocity = 127 "));
                          lcd.clear();
                          lcd.setCursor(0,0);
                          lcd.print("TRACK CTRL ");
                          lcd.print(CTRL_TRACKSTART+ctrlbankadd+i);
                          lcd.setCursor(0,1);
                          lcd.print("127");
                          lcd.setCursor(15, 1);                     //always show bank in bottom right corner of LCD
                          lcd.print(ctrlbank);*/
      }
      if (buttonsB[i].risingEdge()&& doublebpressBool[i]==true){                                                  //if we are ready for a double0 and it happens
        for(int u=0; u<(CTRLBUTTONSA_N*2); u++){
          if (u!=(i+ctrlbankadd)){
            usbMIDI.sendControlChange(CTRL_TRACKSTART+u, 0, MIDI_CHAN);                                   //we send a 0 track value to all tracks except the pressed one
                                                                                                          //(tracks can only all be activated with glove)
                      Serial.println(String(" ch= ") + MIDI_CHAN + (", Control = ") + (CTRL_TRACKSTART+u) + (" Velocity = 0 "));           
          }                        
        }
        //usbMIDI.sendControlChange(CTRL_TRACKSTART+ctrlbankadd+i, 0, MIDI_CHAN);                                   //we send a 0 track value
                        Serial.println(String(" ch= ") + MIDI_CHAN + (", Control = ") + (CTRL_TRACKSTART+ctrlbankadd+i) + (" Velocity = 0 "));
                        lcd.clear();
                        lcd.setCursor(0,0);
                        lcd.print("SOLO CTRL ");
                        lcd.print(CTRL_TRACKSTART+ctrlbankadd+i);
                        lcd.setCursor(0,1);
                        lcd.print("0"); 
                        lcd.setCursor(15, 1);                     //always show bank in bottom right corner of LCD
                        lcd.print(ctrlbank);     
        recbBool[i]=true;                                                                                         //we are ready for a rec 127  
        doublebpressBool[i]=false;                                                                                //we are not ready for a double 0
        holdbBool[i]=false;                                                                                       //we are not anymore waiting for a hold 
        doublebcountBool[i]=false;     
      }                                                        
      if (millisTime-holdbTime[i]>=holdTime && holdbBool[i]==true) {                                             //if we are ready for a hold 127 and it happens
        
        usbMIDI.sendControlChange(CTRL_UNDOSTART+ctrlbankadd+i, 127, MIDI_CHAN);                                 //we send the undo message
        holdbBool[i]=false;                                                                                      //we are not anymore waiting for a hold127
        holdbpressBool[i]=true;                                                                                  //we are ready for a hold0
        doublebcountBool[i]=false;
                          Serial.println(String(" ch= ") + MIDI_CHAN + (", Control = ") + (CTRL_UNDOSTART+ctrlbankadd+i) + (" Velocity = 127"));
                          lcd.clear();
                          lcd.setCursor(0,0);
                          lcd.print("UNDO CTRL ");
                          lcd.print(CTRL_UNDOSTART+ctrlbankadd+i);
                          lcd.setCursor(0,1);
                          lcd.print("127");
                          lcd.setCursor(15, 1);                     //always show bank in bottom right corner of LCD
                          lcd.print(ctrlbank);  
     }
      if (buttonsB[i].risingEdge()&& holdbpressBool[i]==true) {                                                   //if we are ready for a hold0 and the buttons rises
         usbMIDI.sendControlChange(CTRL_UNDOSTART+ctrlbankadd+i, 0, MIDI_CHAN);                                   //we send the 0 clear message
         recbBool[i]=true;
         doublebBool[i]=false;
         holdbpressBool[i]=false;
         doublebcountBool[i]=false;
                          Serial.println(String(" ch= ") + MIDI_CHAN + (", Control = ") + (CTRL_UNDOSTART+ctrlbankadd+i) + (" Velocity = 0 "));
                          lcd.clear();
                          lcd.setCursor(0,0);
                          lcd.print("UNDO CTRL ");
                          lcd.print(CTRL_STOPSTART+ctrlbankadd+i);
                          lcd.setCursor(0,1);
                          lcd.print("0"); 
                          lcd.setCursor(15, 1);                     //always show bank in bottom right corner of LCD
                          lcd.print(ctrlbank); 
      }         
    }    
  }
}


void readButtons(){
  buttonBank.update();
  buttonStop.update();
  buttonPlay.update();
  for(int i=0; i<CTRLBUTTONSA_N; i++){   
    buttonsA[i].update();
  }
  for(int i=0; i<CTRLBUTTONSB_N; i++){   
    buttonsB[i].update();
  }
}

void sendrackVar(){
  usbMIDI.sendControlChange(CTRL_RACK, rackvar, MIDI_CHAN);
  Serial.print(" Rack Nr ");
  Serial.println(rackvar);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Rack Nr: ");
  lcd.print(rackvar);
  lcd.setCursor(15, 1);                     //always show bank in bottom right corner of LCD
  lcd.print(ctrlbank);
}


void heartbeat() {
  Serial.print("still alive ");
  Serial.println(counter);
  counter++;
}


