#include <Bounce.h>


/*
 * This is where you keep track of your button
 */

// The pin the button is connected to
#define SHIFTPIN 1
Bounce shiftButton = Bounce(SHIFTPIN, 10);
#define NUM_BANKS 3
int bank = 0;
// End of shift button variables.

void setup() {
  // put your setup code here, to run once:
  pinMode(SHIFTPIN, INPUT_PULLUP);

  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  shiftButton.update();
  if( shiftButton.fallingEdge()) {
    bank++;
    bank %= NUM_BANKS;
  }
  Serial.print("Bank is ");
  Serial.println(bank);
}
