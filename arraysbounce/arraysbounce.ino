#include <Bounce.h>

// This code turns a led on/off through debounced buttons
// Buttons wired one side to pin the other 330oms to ground

//#define B0 4// i have a feeling there is
//#define B1 5// an injection issue with these first two

#define B2 6
#define B3 7

byte buttons[]={6,7,4,5};  //seperate array from definitions to set up the pins
#define NUMBUTTONS sizeof(buttons) //gives size of array *helps for adding buttons

#define LED 13
int ledValue = LOW;

// I really dont see getting around doing this manually
Bounce bouncer[] = {//would guess thats what the fuss is about
  Bounce(4,5),
  Bounce(5,5),
  Bounce(B2,5),
  Bounce(B3,5)
};

void setup() {
   for (byte set=0;set<NUMBUTTONS;set++){//sets the button pins
     pinMode(buttons[set],INPUT);
     digitalWrite(buttons[set],HIGH);//<-comment out this line if not using internal pull ups
   }//-----------------------------------and change read()==to high if your set up requires
  pinMode(LED,OUTPUT);//------------------otherwise event will occure on release
}

void loop() {
// test turning on a led with any of the debounced buttons
for(int num=0;num<NUMBUTTONS;num++){
   if ( bouncer[num].update()) {
     if ( bouncer[num].read() == LOW) {
       if ( ledValue == LOW ) {
         ledValue = HIGH;
       } else {
         ledValue = LOW;
       }
       digitalWrite(LED,ledValue);
     }
   }
   
}
}
