// originally, Teensyduino MIDI Beat Clock Example 
// by Sebastian Tomczak 
// 29 August 2011 

byte counter; 
byte CLOCK = 248; 
byte START = 250; 
byte CONTINUE = 251; 
byte STOP = 252; 
int x = -1;
unsigned long millisTime=0;
unsigned long  click1Time=0;  
unsigned long  click2Time=0;  
volatile bool clickTime;
unsigned long beatTime=0;  

void setup() {  
Serial.begin(31250); 
pinMode(13, OUTPUT); 
digitalWrite(13, HIGH); 
usbMIDI.setHandleRealTimeSystem(RealTimeSystem); 
} 

void loop() { 
  
delay(50);
usbMIDI.read(); 


if(clickTime){Serial.println("HIGH");}

if(clickTime){
  Serial.println(" HIGH ");
  click2Time=millisTime;
  Serial.println(click2Time);
  usbMIDI.sendNoteOn(61, 100, 1);
  usbMIDI.sendNoteOff(61, 100, 1);
}

millisTime = millis();  

if (clickTime==true) click1Time=millisTime;
if (clickTime==false) {
   click2Time=millisTime;
   beatTime=click2Time-click1Time;
   Serial.print(" click1Time: ");
   Serial.print(click1Time);
   Serial.print(" click2Time: ");
   Serial.print(click2Time);
   Serial.print(" beatTime: ");
   Serial.print(beatTime);
   Serial.print(" millis Time: ");
   Serial.println(millisTime);
}
} 

void RealTimeSystem(byte realtimebyte) { 
  
unsigned long millisTime;


if(realtimebyte == 248) { 
counter++; 

if(counter == 23) { 
digitalWrite(13, LOW); 
clickTime=false;

/*click2Time=millisTime;
   beatTime=click2Time-click1Time;
   Serial.print(" click1Time: ");
   Serial.print(click1Time);
   Serial.print(" click2Time: ");
   Serial.print(click2Time);
   Serial.print(" beatTime: ");
   Serial.print(beatTime);
   Serial.print(" millis Time: ");
   Serial.println(millisTime); */
} 
if(counter == 24) { 
counter = 0; 
} 
if(counter == 12) {  
} 

if(counter == 11) {  
digitalWrite(13, HIGH); 
Serial.println("HIGH");
clickTime=true;
//click1Time=millisTime;
} 
} 

if(realtimebyte == START || realtimebyte == CONTINUE) { 
counter = 0; 
digitalWrite(13, HIGH); 
Serial.println("START");
} 

if(realtimebyte == STOP) { 
digitalWrite(13, LOW); 
Serial.println("STOP");
  
} 
} 
