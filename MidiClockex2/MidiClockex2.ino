byte counter; 
byte CLOCK = 248; 
byte START = 250; 
byte CONTINUE = 251; 
byte STOP = 252; 

void setup() { 
Serial.begin(31250); 
Serial1.begin(31250);
pinMode(13, OUTPUT); 
digitalWrite(13, HIGH); 
usbMIDI.setHandleRealTimeSystem(RealTimeSystem); 
} 



void loop() { 
usbMIDI.read(); 

} 

void RealTimeSystem(byte realtimebyte) { 
if(realtimebyte == 248) { 
  Serial1.write(248);
counter++; 
if(counter == 24) { 
counter = 0; 
digitalWrite(13, HIGH);
} 

if(counter == 12) { 
digitalWrite(13, LOW);
} 
} 

if(realtimebyte == START || realtimebyte == CONTINUE) { 
counter = 0; 
digitalWrite(13, HIGH); 
} 

if(realtimebyte == STOP) { 
digitalWrite(13, LOW); 
} 

}
