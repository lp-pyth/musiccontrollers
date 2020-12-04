



unsigned long startTime = 0; // when did we begin pressing?
unsigned long millisTime;
unsigned long bounceTime = 1000;
int i=0;
void setup() {

  Serial.begin(9600); 


}

void loop() {
     
    millisTime = millis(); 
    if (millisTime-startTime>=bounceTime) {
      startTime=millisTime;
      Serial.print ("Pow");  
      Serial.println (i);
      i++;
    }
}

