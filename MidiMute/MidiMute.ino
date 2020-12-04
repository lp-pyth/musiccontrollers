#include <Bounce.h>


/*
 * Midi information
 */
// This is the midi channel
#define MIDI_CHAN 1
bool note_states[127];



/*
 * This is where you keep track of your button
 */

// The pin the button is connected to
#define MUTEPIN 1
Bounce muteButton = Bounce(MUTEPIN, 10);
bool muted = false;
// End of shift button variables.

void setup() {
  // put your setup code here, to run once:
  pinMode(MUTEPIN, INPUT_PULLUP);
  Serial.begin(9600);

  for(uint8_t i = 0; i< 128; i++){
    note_states[i] = false;
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  /*
   * Check for mute button pressing
   */
  muteButton.update();
  if( muteButton.fallingEdge()) {
    muted = !muted;
    if (muted){
      allNotesOff();
    }
  }

  while (usbMIDI.read()) ;


  /*
   * Make your music here.
   */
}

void noteOn(uint8_t note, uint8_t velocity) {
  if (muted){
    return;
  }
  note %= 128;
  note_states[note] = true;
  usbMIDI.sendNoteOn(note, velocity, MIDI_CHAN);
}

void noteOff(uint8_t note, uint8_t velocity){
  if (muted) {
    return;
  }
  note %= 128;
  note_states[note] = false;
  usbMIDI.sendNoteOff(note, velocity, MIDI_CHAN);
}

void allNotesOff(){
  for(uint8_t i = 0; i < 128; i++){
    if (note_states[i]){
      note_states[i] = false;
      usbMIDI.sendNoteOff(i, 99, MIDI_CHAN);
    }
  }
}

