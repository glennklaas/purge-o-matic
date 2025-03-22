#include <LiquidCrystal_I2C.h>
const int rPin1 = 3;
const int switchPin = 2;
int lastSampleTime = 0;
int pressureHistory[4] = {0, 0, 0, 0};
const float MAX_RANGE = 1023.0;
const float MAX_PSI = 60.0;
float deltaPSI = 0.0;
int completedProgramLoops = 0;


//initialize running state to 0
//run state 0 = in standby
//run state 1 = purge cycle
//run state 2 = burp cycle
//run state 3 = completed cycle, ready to start next cycle
int runningState = 0;
int burpLoops = 2; //total number to be performed
int burpCounter = 0; //initialize to 0, sequential counter var
int initialValveClose = 0;
int startTime = 0;

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  //Setup serial comms
  Serial.begin(9600); 

  pinMode(switchPin, INPUT);
  pinMode(rPin1, OUTPUT);

  lcd.init();
  lcd.backlight();
  welcome_screen();
  digitalWrite(rPin1, LOW);
  //digitalWrite(rPin1, HIGH);
}

void welcome_screen() {
  //clear screen
  lcd.clear();
    
  //Initialize the Display and display splash screen
  lcd.begin(16,2);
  lcd.setCursor(0,0);
  lcd.print(" Purge-O-Matic");
  lcd.setCursor(0,1);
  lcd.print("  Version 1.0");
  delay(500);
  
  //clear screen
  lcd.clear();
}

void loop() {
  
  //Serial.println(runningState);
  	//interrupt if switched off
	if(digitalRead(switchPin) == 0){
 		runningState = 0;
      	digitalWrite(rPin1,LOW);
	}

  
  	switch (runningState) {
  case 0:
      //positive going transition to the on state, record start time
    if(digitalRead(switchPin) == HIGH){
      startTime = millis();
      runningState = 1;
    }
    break;
      
  case 1:
      //purge running state
      if(initialValveClose == 0){
        digitalWrite(rPin1,LOW);
        initialValveClose = 1;
      }
        //delay(500);
      	//if pressure isnt changing appreciably while valve is shut, open valve
        if(deltaPSI<.05 && digitalRead(rPin1) == LOW){
        digitalWrite(rPin1,HIGH);
      	}
      
        //wait for vented pressure to go below 1psi with valve open
        if(analogRead(A0) < 17 && digitalRead(rPin1) == HIGH){
        runningState = 2; 
        initialValveClose = 0;
        } 
      
    break;
  case 2:
    // burp running state
      
      if(initialValveClose == 0){
      digitalWrite(rPin1,LOW);
        initialValveClose = 1;
      }
      //delay(500);
      //close valve to begin burp
      if(burpCounter < burpLoops){
        //begin a burp
      digitalWrite(rPin1, LOW);
      
        //if pressure isnt changing appreciably while valve is shut, open valve
        if(deltaPSI<.05 && digitalRead(rPin1) == LOW){
        digitalWrite(rPin1,HIGH);
      	}
        else{
        digitalWrite(rPin1,LOW);
        }
        
        //wait for vented pressure to go below 1psi with valve open
        if(analogRead(A0) < 17 && digitalRead(rPin1) == HIGH){
        runningState = 2;   
        initialValveClose = 0;
        } 
      burpCounter = burpCounter + 1;
      }
      else if (burpCounter >= 5){
        burpCounter = 0;
        runningState = 3;
      }             
    break;
      
  case 3:
    //end, ready to start next round
    runningState = 1;
    completedProgramLoops = completedProgramLoops + 1;
    break;
      
  default:
    break;

      

  }//end of switch


    if(lastSampleTime == 0){
    lastSampleTime = millis();
    }
      
    //every 250ms
    if(millis() - lastSampleTime > 100){
        
     	//set new lastSampleTime
      //Serial.print(millis()-lastSampleTime);
      lastSampleTime = millis();
   		//shift last 3 values pressure history array
   		pressureHistory[3] = pressureHistory[2];
   		pressureHistory[2] = pressureHistory[1];
   		pressureHistory[1] = pressureHistory[0];
   		//set new pressure value to array
   		pressureHistory[0] = analogRead(A0);
  	
   		//calculate deltaPSI from first and last array values
   		deltaPSI = abs((float(pressureHistory[0]-pressureHistory[3]))/float(pressureHistory[3]+1));
      
    }//end of 250ms sampling





//update display
lcd.setCursor(0,0);
lcd.print(deltaPSI);
Serial.println(deltaPSI);
lcd.setCursor(0,1);
lcd.print(runningState);
lcd.setCursor(5,1);
lcd.print(float(analogRead(A0)/17.05));
//Serial.println(runningState);
//Serial.println(analogRead(A0));
//Serial.println(deltaPSI);
//Serial.print(pressureHistory[0]);
//Serial.print(", ");
//Serial.print(pressureHistory[1]);
//Serial.print(", ");
//Serial.print(pressureHistory[2]);
//Serial.print(", ");
//Serial.println(pressureHistory[3]);



}
