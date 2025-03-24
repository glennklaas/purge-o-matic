#include <LiquidCrystal_I2C.h>
const int rPin1 = A7; //relay output pin, using analog pin as digital
const int switchPin = 3; //input switch pin
const int ptPin = A6; //pressure tranducer pin
const int ledPin = 2;
int lastSampleTime = 0;
int pressureHistory[4] = {0, 0, 0, 0};
const float MAX_RANGE = 1023.0;
const float MAX_PSI = 60.0;
float deltaPSI = 0.0;
int completedProgramLoops = 0;
bool valveRequest = false;
unsigned long int valveDelay = 0;
bool burpInterlock = 1;
bool onRequest = 0;
bool delayInterlock = false;
unsigned long int delayTime = 0;


//initialize running state to 0
//run state 0 = in standby
//run state 1 = purge cycle
//run state 2 = burp cycle
//run state 3 = completed cycle, ready to start next cycle
int runningState = 0;
int burpLoops = 5; //total number to be performed
int burpCounter = 0; //initialize to 0, sequential counter var
int initialValveClose = 0;
int startTime = 0;

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  //Setup serial comms
  Serial.begin(9600); 

  pinMode(switchPin, INPUT);
  pinMode(rPin1, OUTPUT);
  pinMode(ledPin, OUTPUT);

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
  
  //interrupt if switched off
	if(digitalRead(switchPin) == 0){
 		runningState = 0;
    onRequest = true;
    digitalWrite(rPin1,HIGH);
	}

  switch (runningState) {
          // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  
  case 0: // * * * * * I N I T I A L I Z E  R U N N I N G  S T A T E * * * * *
          // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
    //positive going transition to the on state, record start time
    if(digitalRead(switchPin) == HIGH && onRequest){
      startTime = millis();
      runningState = 2;
      //digitalWrite(rPin1,LOW);
      onRequest = false;
      digitalWrite(rPin1,HIGH);
      
    }
    break;
      
  // PURGE
           // * * * * * * * * * * * * * * * * * * * * *
  case 1:  // * * * * * P U R G E  S T A T E  * * * * *
           // * * * * * * * * * * * * * * * * * * * * *

    //purge running state
    if(initialValveClose == 0){
      valveRequest = false;
      initialValveClose = 1;
    }

      
    //if pressure isnt changing appreciably while valve is shut, open valve
    if((deltaPSI<.05 && digitalRead(rPin1) == LOW )){
      valveRequest = true;
    }
      
    //wait for vented pressure to go below 1psi with valve open
    if(analogRead(ptPin) < 17 && digitalRead(rPin1) == HIGH){
      runningState = 2; 
      initialValveClose = 0;
    }  
    //delay(500);
    break;
   
    
          // * * * * * * * * * * * * * * * * * * * *
  case 2: // * * * * * B U R P   S T A T E * * * * *
          // * * * * * * * * * * * * * * * * * * * * 
    // burp running state
    if(initialValveClose == 0){
      valveRequest = false;
      burpCounter = 0;
      initialValveClose = 1;
    }
    //close valve to begin burp
    if(burpCounter < burpLoops){
            
      //begin a burp

      //if pressure isnt changing appreciably while valve is shut, open valve
      if(deltaPSI<.02 && digitalRead(rPin1) == LOW && !valveRequest){

        if(delayInterlock == false){ //delay statements
          delayInterlock = true;
          delayTime = millis() + 10000; //set time for 10 seconds into future
        }
                
        if(delayTime < millis() && delayInterlock){
          valveRequest = true;
          delayInterlock = false;
        }//end delay statements

      }
      //wait for vented pressure to go below 1psi with valve open
      if(deltaPSI<.05 && digitalRead(rPin1) == HIGH && valveRequest){
        //runningState = 2;   
        burpInterlock = 0;

        if(delayInterlock == false){ //delay statements
          delayInterlock = true;
          delayTime = millis() + 1000; //set time for 1 seconds into future
        }
                
        if(delayTime < millis() && delayInterlock){
          valveRequest = false;
          delayInterlock = false;
        }//end delay statements

      } 

      if(burpInterlock == 0){
          burpCounter = burpCounter + 1;
          burpInterlock = 1; //to prevent burp counter looping continuously
        }
      

    }
    
     else if (burpCounter >= 5){

      burpCounter = 0;
      initialValveClose = 0;
      runningState = 3; //TODO change to 3 for looping run
      valveRequest = false;
    }             
    break;


          // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
  case 3: // * * * * * S T A T E  T R A N S I T I O N  L O G I C * * * * *
          // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
    //end, ready to start next round
    runningState = 0; //TODO change back to 1 for normal looping ops
    completedProgramLoops = completedProgramLoops + 1;
    break;
      
  default:
    break;
  } //end of switch


  if(lastSampleTime == 0){
    lastSampleTime = millis();
  }





  //every 250ms
  if(millis() - lastSampleTime > 250 && runningState != 0){
        
    //set new lastSampleTime
    lastSampleTime = millis();
    //shift last 3 values pressure history array
    pressureHistory[3] = pressureHistory[2];
    pressureHistory[2] = pressureHistory[1];
    pressureHistory[1] = pressureHistory[0];
    //set new pressure value to array
    pressureHistory[0] = analogRead(ptPin);
  
    //calc deltaPSI from average of 3 past pressures compared to current
    deltaPSI = (float((pressureHistory[1]+1 + pressureHistory[2]+1 + pressureHistory[3]+1))/(3 * pressureHistory[0]+1)) - 1;

    //calculate deltaPSI from first and last array values
    //deltaPSI = (float(pressureHistory[0])-float(pressureHistory[3]))/float(pressureHistory[3]+1);

    if(deltaPSI < 0){
      deltaPSI = deltaPSI * -1;
    }

    // *** Delta ***
    //lcd.setCursor(0,0);
    //lcd.print(String(deltaPSI, 2));
    //lcd.print(" PDelta");

    // *** Running State ***
   //lcd.setCursor(0,0);
    //}
  } //end of 250ms sampling

    //front panel LED showing if it is currently running
    if(runningState == 0){
      digitalWrite(ledPin, LOW);
    }
    else{
      digitalWrite(ledPin,HIGH);
    }

    //valve position request and cooldown control
    if(valveRequest == true && millis() > valveDelay && !digitalRead(rPin1) && runningState != 0){
      valveDelay = millis() + 1000;
      digitalWrite(rPin1,HIGH);
    }
    else if(valveRequest == false && millis() > valveDelay && digitalRead(rPin1) && runningState != 0){
      valveDelay = millis() + 1000;
      digitalWrite(rPin1,LOW);
    }

    // *** PSI ***
    lcd.setCursor(0,0);
    lcd.print(runningState);
    lcd.setCursor(3,0);
    lcd.print(burpCounter);
    lcd.setCursor(5,0);
    lcd.print(valveRequest);
    lcd.setCursor(0,1);
    lcd.print(deltaPSI);
}
