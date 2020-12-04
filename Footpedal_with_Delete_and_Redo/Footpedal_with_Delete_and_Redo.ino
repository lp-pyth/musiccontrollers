                                   //LCD & SHIELD Libraries
  #include <Wire.h>
  #include <Adafruit_MCP23017.h>
  #include <Adafruit_RGBLCDShield.h>
  Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();
  #include <usb_keyboard.h>
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
  
  #define BUTTONBANK 6         //bankswitch button
  #define BUTTONSTOP 11        //stop button (not part of bank)
  #define BUTTONPLAY 23        //play (not part of bank)
  #define CTRLBUTTONS_N 12     //ctrl buttons used to trigger clips
  #define NUM_BANKS 2
  #define CTRL_DEL
  #define CTRL_STOP 71        //begin of control values available to footpedal (71-128)
  #define CTRL_PLAY 72
  #define CTRL_START 73      //start of ctrl values for buttons (number up in forloop)
  int bank = 0;

  #define CTRLBUTTONS_N 12
  int buttonPins[] = {2,3,4,5,/*6,*/7,8,9,10,/*11,*/12,20,21,22,/*23*/};  //All button pins except bank, stop and play button
  // int buttons[CTRLBUTTONS_N];

  Bounce buttonBank = Bounce(BUTTONBANK, 50);                  
  Bounce buttonStop = Bounce(BUTTONSTOP, 50);
  Bounce buttonPlay = Bounce (BUTTONPLAY, 50);
  
  Bounce buttons[12] = {
    Bounce (buttonPins[0], 50), 
    Bounce (buttonPins[1], 50), 
    Bounce (buttonPins[2], 50), 
    Bounce (buttonPins[3], 50),
    Bounce (buttonPins[4], 50),
    Bounce (buttonPins[5], 50),
    Bounce (buttonPins[6], 50),
    Bounce (buttonPins[7], 50),
    Bounce (buttonPins[8], 50),
    Bounce (buttonPins[9], 50),
    Bounce (buttonPins[10], 50),
    Bounce (buttonPins[11], 50)
  };
  
  int bankadd = 0;


  
void setup(){  

delay (200);
      Keyboard.set_key1(0);
      Keyboard.send_now();
      Keyboard.set_key1(KEY_M);
      Keyboard.send_now();
      Keyboard.set_key1(0);
      Keyboard.send_now();
delay (200);
      Keyboard.set_key1(0);
      Keyboard.send_now();
      Keyboard.set_key1(KEY_C);
      Keyboard.send_now();
      Keyboard.set_key1(0);
      Keyboard.send_now();


delay (4000);
      Keyboard.set_key1(0);
      Keyboard.send_now();
      Keyboard.set_key1(KEY_M);
      Keyboard.send_now();
      Keyboard.set_key1(0);
      Keyboard.send_now();
delay (4000);
      Keyboard.set_key1(0);
      Keyboard.send_now();
      Keyboard.set_key1(KEY_C);
      Keyboard.send_now();
      Keyboard.set_key1(0);
      Keyboard.send_now();

void loop(){

      

  millisTime = millis();                  //time var running WHATS THE MAX MILLIS VALUE?
  
  while (usbMIDI.read());   

  readButtons();

  if(buttonBank.fallingEdge()) {     //3 banks, Button+/-, Print LCD and Serial 
    bank++;
    bank %= NUM_BANKS;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Bank ");
    lcd.print(bank);
    Serial.print("Bank ");
    Serial.println(bank);
    redoTime=millisTime;
    redoBool=true;
  }
  if (buttonBank.risingEdge()) {
      redoBool=false;
    }
  if (millisTime-redoTime>=redoHold && redoBool==true) {
      redoBool=false;
      Serial.println ("Redo ");
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Redo ");
      Keyboard.set_modifier(MODIFIERKEY_CTRL);
      Keyboard.set_key1(KEY_Z);
      Keyboard.send_now();
      Keyboard.set_modifier(0);
      Keyboard.set_key1(0);
      Keyboard.send_now();
      bank=bank-1;
    }

  bankadd = bank * CTRLBUTTONS_N;     //ATTENTION THAT HE READS 12 HERE


  if (buttonStop.fallingEdge()) {
    usbMIDI.sendControlChange((CTRL_STOP), 127, MIDI_CHAN);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Stop ");
    Serial.println(String("ch=") + MIDI_CHAN + (", Control=") + (CTRL_STOP) + ("Velocity=127"));
  }
  if (buttonPlay.fallingEdge()) {
    usbMIDI.sendControlChange((CTRL_PLAY), 127, MIDI_CHAN);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Play ");
    Serial.println(String("ch=") + MIDI_CHAN + (", Control=") + (CTRL_PLAY) + ("Velocity=127"));
  }



              
                     
  for(int i=0; i<CTRLBUTTONS_N; i++){   
    if (buttons[i].fallingEdge()) {
      usbMIDI.sendControlChange((CTRL_START+bankadd+i), 127, MIDI_CHAN);
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(" CTRL ");
      lcd.print (CTRL_START+bankadd+i);
      Serial.println(String(" ch= ") + MIDI_CHAN + (", Control = ") + (CTRL_START+bankadd+i) + (" Velocity = 127 "));
      delTime[i]=millisTime;
      delBool[i]=true;
    }
    if (buttons[i].risingEdge()) {                                  //if the button gets released before delHold, delbool is false and the delete function cannot be sent
      delBool[i]=false;
    }                                                             
    if (millisTime-delTime[i]>=delHold && delBool[i]==true) {        //for each button the possible delete function if delbool is true and pressure time longer than delHold (3s)
      delBool[i]=false;
      Serial.println ("DELETE ");
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Delete ");
      Keyboard.set_key1(KEY_DELETE);                      //send a keyboard DEL
      Keyboard.send_now();
      Keyboard.set_key1(0);
      Keyboard.send_now();
    }
  }
}


void readButtons(){
  buttonBank.update();
  buttonStop.update();
  buttonPlay.update();
  for(int i=0; i<CTRLBUTTONS_N; i++){   
    buttons[i].update();
  }
}


