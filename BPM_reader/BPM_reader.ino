
#include <MIDI.h>
#include <EEPROM.h>
  
#define NUM 3
#define MIDI_CHAN 1
#define DEBUG 1


// the eeprom address for the calibration
#define CALIB_ADDR 16

int mode = 0; // sth to save into eeprom

/*
 * current note
 */
int current_note = -1;


// timing details: we want to send midi at a 100hz clock
// that means every 10 ms
int lastMidiSend = 0;
#define MIDI_INTERVAL 20

 
int midi_clock = 1;
bool send_arpegg = false;
uint8_t arpegg_interval = 0; // number of ticks between on and off (and on, and off...)

void setup() {
  
  Serial.begin(9600);                                                 
 
  usbMIDI.setHandleRealTimeSystem(handleMidiClock);

  mode = EEPROM.read(1);
  
  Serial.println("Init done");
  
}

void loop() { 
  
  millisTime = millis();  

    //Save the current mode to EEPROM
  EEPROM.write(1, mode);
  
  
  //Do MIDI communication
  int t = millis();
  if (t - lastMidiSend >= MIDI_INTERVAL) {
    if(!muted){
      sendOutput();
    }
    lastMidiSend = t;
  }  
 
  // clean up: clear midi buffer
  while (usbMIDI.read()) ; // read and discard any incoming MIDI messages
}

void clearOutput() {
  send_arpegg = false;
  if (current_note != -1){
    usbMIDI.sendNoteOff(current_note, 99, MIDI_CHAN);
    current_note = -1;
  }

}

int gestureDebounce = 0;
int lastxcontrol = -1;
int lastycontrol = -1;
void sendOutput() {
   // timing code: if it's been more than 20ms since our last
  // send, then send again
  Serial.println("Mode " + (String) mode + (String) xy_lock);

  if (XYLockButton.update() && XYLockButton.fallingEdge()){
      xy_lock++;
  }


  /*
   * Accel X is the note value.
   * Accel Y is the speed
   * If the fingers are in a fist, then send a note. 
   */
   
  else if (mode == 1 ) {
         
    if( in_fist) {
      send_arpegg = true;
    } else {
      send_arpegg = false;
      noteOff();
    }

    /*
     * Map the Y accel to the midi interval
     */
    arpegg_interval =  4;
    if( arpegg_interval < 1){ arpegg_interval = 1; }
    Serial.print("Arpeggio interval: ");
    Serial.println(arpegg_interval);
  }
}


/*
 * Send a note, based on accel_x, that is mapped
 * to a one-octave C-major scale.
 * 
 * This needs to be debounced
 */
int noteDebounce = 0;



void noteOff(){
  if (current_note != -1){
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

  if (val == 0){
    //noteOn();
  } else if (val == arpegg_interval) {
    noteOff();
  }
}






// read the calibration values from eeprom
 

uint16_t readShort(int addr){
  uint16_t val = EEPROM.read(addr);
  val *= 256;
  val += EEPROM.read(addr+1);
  return val;
}

void writeShort(int addr, uint16_t val){
  EEPROM.write(addr, val / 256);
  EEPROM.write(addr + 1, val % 256);
}




/*
 * Called by the midi library, whenever a tick comes in. 
 */

void handleMidiClock(byte message){
  if (message == 248){ // tick
    midi_clock++;
    handleMidiTick();
  }

  else if (message == 250 || message == 251){
    //MIDI START / MIDI CONTINUE
    midi_clock = 0;
  }
  else if (message == 252){
    // MIDI STOP
    if (send_arpegg){
      noteOff();
    }
  }
}


