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
bool valveOpenRequest = false;
int valveDelay = 0;

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
  
  //Serial.println(runningState);
  //interrupt if switched off
	if(digitalRead(switchPin) == 0){
 		runningState = 0;
    digitalWrite(rPin1,LOW);
	}

  switch (runningState) {
          // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  
  case 0: // * * * * * I N I T I A L I Z E  R U N N I N G  S T A T E * * * * *
          // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
    //positive going transition to the on state, record start time
    if(digitalRead(switchPin) == HIGH){
      startTime = millis();
      runningState = 1;
    }
    break;
      

           // * * * * * * * * * * * * * * * * * * * * *
  case 1:  // * * * * * P U R G E  S T A T E  * * * * *
           // * * * * * * * * * * * * * * * * * * * * *

    //purge running state
    if(initialValveClose == 0){
      valveOpenRequest = false;
      initialValveClose = 1;
    }

      
    //if pressure isnt changing appreciably while valve is shut, open valve
    if((deltaPSI<.05 && digitalRead(rPin1) == LOW )){
      valveOpenRequest = true;
    }
      
    //wait for vented pressure to go below 1psi with valve open
    if(analogRead(ptPin) < 17 && digitalRead(rPin1) == HIGH){
      runningState = 3; 
      initialValveClose = 0;
    }  
    //delay(500);
    break;
    
    
          // * * * * * * * * * * * * * * * * * * * *
  case 2: // * * * * * B U R P   S T A T E * * * * *
          // * * * * * * * * * * * * * * * * * * * * 
    // burp running state
    if(initialValveClose == 0){
      digitalWrite(rPin1,LOW);
      initialValveClose = 1;
    }

    //delay(500);  // TODO: Review the reason this delay is helpful.  Without it, we hit "steady state" and get stuck in this state.

    //close valve to begin burp
    if(burpCounter < burpLoops){
            
      //begin a burp
      digitalWrite(rPin1, LOW);
    
      //if pressure isnt changing appreciably while valve is shut, open valve
      if(deltaPSI<.05 && digitalRead(rPin1) == LOW){
        digitalWrite(rPin1,HIGH);
      } else {
        digitalWrite(rPin1,LOW);
      }
      
      //wait for vented pressure to go below 1psi with valve open
      if(analogRead(ptPin) < 17 && digitalRead(rPin1) == HIGH){
        runningState = 2;   
        initialValveClose = 0;
      } 

      burpCounter = burpCounter + 1;

    } else if (burpCounter >= 5){
      burpCounter = 0;
      runningState = 3;
    }             
    break;


          // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
  case 3: // * * * * * S T A T E  T R A N S I T I O N  L O G I C * * * * *
          // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
    //end, ready to start next round
    runningState = 1;
    completedProgramLoops = completedProgramLoops + 1;
    break;
      
  default:
    break;
  } //end of switch


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
    pressureHistory[0] = analogRead(ptPin);
  
    //calculate deltaPSI from first and last array values
    deltaPSI = abs((float(pressureHistory[0]-pressureHistory[3]))/float(pressureHistory[3]+1));
    Serial.println(deltaPSI);

    // *** Delta ***
    //Serial.println(deltaPSI);
    //lcd.setCursor(0,0);
    //lcd.print(String(deltaPSI, 2));
    //lcd.print(" PDelta");

    // *** Running State ***
   //lcd.setCursor(0,0);
    //switch(runningState) {
      //case 0: lcd.print("0-INIT"); Serial.print("0-INIT  "); break;
      //case 1: lcd.print("1-PURG"); Serial.print("1-PURG ");  break;
      //case 2: lcd.print("2-BURP"); Serial.print("2-BURP [" + String(burpCounter-1) + "]  ");break;
      //case 3: lcd.print("3-LOOP"); Serial.print("3-LOOP  ");break;
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
    if(valveOpenRequest == true && millis() > valveDelay){
      valveDelay = millis() + 5000;
      digitalWrite(rPin1,HIGH);
    }
    else if(valveOpenRequest == false && millis() > valveDelay){
      valveDelay = millis() + 5000;
      digitalWrite(rPin1,LOW);
    }





    // *** PSI ***
    lcd.setCursor(0,1);
    int sensorValue = analogRead(ptPin);
    lcd.print( (sensorValue / 1023.0) * 60.0);
    lcd.print(" PSI    ");
    //Serial.print( String(float(sensorValue/17.05), 2) + " PSI  ");

    // *** Delta ***
    //Serial.println(String(deltaPSI) + " DeltaP" );
  //Serial.println(deltaPSI);
    //float mynum = float(analogRead(ptPin)/17.05);
    
    //Serial.println(runningState);
    //Serial.println(analogRead(ptPin));
    //Serial.println(deltaPSI);
    //Serial.print(pressureHistory[0]);
    //Serial.print(", ");
    //Serial.print(pressureHistory[1]);
    //Serial.print(", ");
    //Serial.print(pressureHistory[2]);
    //Serial.print(", ");
    //Serial.println(pressureHistory[3]);

  
}
