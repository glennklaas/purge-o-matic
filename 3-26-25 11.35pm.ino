#include <LiquidCrystal_I2C.h>
const int rPin1 = A7; //relay output pin, using analog pin as digital
const int switchPin = 3; //input switch pin
const int ptPin = A6; //pressure tranducer pin
const int ledPin = 2;

//initialize running state to 0
//run state 0 = in standby
//run state 1 = purge cycle
//run state 2 = burp cycle
//run state 3 = completed cycle, ready to start next cycle
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
unsigned long int Timeout = 0;
int runningState = 0;
int burpLoops = 5; //total number to be performed
int burpCounter = 0; //initialize to 0, sequential counter var
int initialValveClose = 0;
unsigned long int startTime = 0;
bool displayPurgeTimer = false;
int tempCountdown = 0;

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
  lcd.setCursor(2,0);
  lcd.print("PSI");
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

void resetPSIHistory() {
  pressureHistory[0] = 1023;
  pressureHistory[1] = 0;
  pressureHistory[2] = 0;
  pressureHistory[3] = 0;
  deltaPSI = 1.0;
}

void loop() {
  //interrupt if switched off
  if(digitalRead(switchPin) == 0){
 	  runningState = 0;
    onRequest = true;
    digitalWrite(rPin1,HIGH);

    //initialize running state to 0
    //run state 0 = in standby
    //run state 1 = purge cycle
    //run state 2 = burp cycle
    //run state 3 = completed cycle, ready to start next cycle
    runningState = 0;
    //burpCounter = 0; //initialize to 0, sequential counter var
    initialValveClose = 0;
    startTime = 0UL;
	}

  switch (runningState) {
          // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  
  case 0: // * * * * * I N I T I A L I Z E  R U N N I N G  S T A T E * * * * *
          // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
    //positive going transition to the on state, record start time
    if(digitalRead(switchPin) == HIGH && onRequest){
      startTime = millis();
      runningState = 1;
      onRequest = false;
      digitalWrite(rPin1,HIGH);
      initialValveClose = 1;
      lcd.setCursor(0,0);
      lcd.setCursor(0,1);
      lcd.print("                "); //clear bottom line
      lcd.setCursor(5,0);
      lcd.print("                "); //clear remainder of top line to the right of pressure reading
      lcd.setCursor(7,0);
      lcd.print("Purge: --");
      lcd.setCursor(7,1);
      lcd.print("Time:");
      displayPurgeTimer = false;
      lcd.setCursor(14,0);
      lcd.print("--");
    }

    lastSampleTime = 0;
    //int completedProgramLoops = 0;
    valveRequest = false;
    valveDelay = 0;
    burpInterlock = 1;
    //onRequest = 0;
    delayInterlock = false;
    delayTime = 0UL;
    Timeout = 0;
    //runningState = 0;
    burpLoops = 5; //total number to be performed
    burpCounter = 0; //initialize to 0, sequential counter var
    initialValveClose = 0;
    //startTime = 0;
    displayPurgeTimer = false;
    completedProgramLoops = 0;

    break;
      
  // PURGE
           // * * * * * * * * * * * * * * * * * * * * *
  case 1:  // * * * * * P U R G E  S T A T E  * * * * *
           // * * * * * * * * * * * * * * * * * * * * *
    //purge running state

    if(initialValveClose == 0){
      valveRequest = false;
      burpCounter = 0;
      initialValveClose = 1;
      Timeout = millis() + 60000; //60s timeout safety
      delayInterlock = false;
      lcd.setCursor(0,1);
      lcd.print(" #");
      lcd.print(completedProgramLoops);
      lcd.setCursor(7,0);
      lcd.print("Purge:");
      resetPSIHistory();
    }
    tempCountdown = int((delayTime - millis())/1000 ) + 1;
    if(displayPurgeTimer){
      if(tempCountdown >= 10 ){
        lcd.setCursor(14,0);
        lcd.print(tempCountdown);
      }
      else if(tempCountdown <= 0 ){
              displayPurgeTimer = false;
              lcd.setCursor(14,0);
              lcd.print("--");
      }
      else if(tempCountdown < 10){
              lcd.setCursor(14,0);
              lcd.print(" ");
              lcd.setCursor(15,0);
              lcd.print(tempCountdown );
      }
    }

      //begin purge

      //if pressure isnt changing appreciably while valve is shut, open valve
      if(deltaPSI<.03 && digitalRead(rPin1) == LOW && !valveRequest){ //removed:  || millis() > Timeout
        
        deltaPSI = 1.0;
        Timeout = millis() + 60000;//60s timeout
        if(delayInterlock == false){ //delay statements
          delayInterlock = true;
          delayTime = millis() + 10000; //set time for 10 seconds into future
          displayPurgeTimer = true;
        }
                
        if(delayTime < millis() && delayInterlock){
          valveRequest = true;
          delayInterlock = false;
          resetPSIHistory();
        }//end delay statements

      }
      //wait for vented pressure to reach steady with valve open
      if(deltaPSI<.05 && digitalRead(rPin1) == HIGH && valveRequest || analogRead(ptPin) < 5 && digitalRead(rPin1) == HIGH && valveRequest){    //removed || millis() > Timeout  
        Timeout = millis() + 60000;//60s timeout
        if(delayInterlock == false){ //delay statements
          delayInterlock = true;
          burpInterlock = 0;//allow entry into burp sequential counter statement
          delayTime = millis() + 1000; //set time for 1 seconds into future
          lcd.setCursor(14,0);
          lcd.print("--");
        }
                
        if(delayTime < millis() && delayInterlock){
          valveRequest = false;
          delayInterlock = false;
          initialValveClose = 0;
          runningState = 2;
          lcd.setCursor(7,0);
          lcd.print(" Burp:   ");
          displayPurgeTimer = false;
          resetPSIHistory();
        }//end delay statements


    }             
    break;
   
    
          // * * * * * * * * * * * * * * * * * * * *
  case 2: // * * * * * B U R P   S T A T E * * * * *
          // * * * * * * * * * * * * * * * * * * * * 
    
    // burp running state
    if(initialValveClose == 0){
      valveRequest = false;
      burpCounter = 0;
      initialValveClose = 1;
      Timeout = millis() + 60000; //60s timeout safety
      resetPSIHistory();
    }

    //close valve to begin burp
    if(burpCounter < burpLoops){
      //begin a burp

      //if pressure isnt changing appreciably while valve is shut, open valve
      if(deltaPSI<.03 && digitalRead(rPin1) == LOW && !valveRequest){//removed  || millis() > Timeout

        Timeout = millis() + 60000;//60s timeout
        if(delayInterlock == false){ //delay statements
          delayInterlock = true;
          delayTime = millis() + 2000; //set time for 2 seconds into future
        }
                
        if(delayTime < millis() && delayInterlock){
          valveRequest = true;
          delayInterlock = false;
          resetPSIHistory();
        }//end delay statements

      }
      //wait for vented pressure to go below 1psi with valve open
      if(deltaPSI<.05 && digitalRead(rPin1) == HIGH && valveRequest || analogRead(ptPin) < 5 && digitalRead(rPin1) == HIGH && valveRequest){//removed  || millis() > Timeout
                
        Timeout = millis() + 60000;//60s timeout
        if(delayInterlock == false){ //delay statements
          delayInterlock = true;
          burpInterlock = 0;//allow entry into burp sequential counter statement
          delayTime = millis() + 1000; //set time for 1 seconds into future
        }
                
        if(delayTime < millis() && delayInterlock){
          valveRequest = false;
          delayInterlock = false;
          resetPSIHistory();
        }//end delay statements

      } 

      if(burpInterlock == 0){
          lcd.setCursor(14,0);
          lcd.print(burpLoops - burpCounter); //run time
          burpCounter = burpCounter + 1;
          resetPSIHistory();
          burpInterlock = 1; //to prevent burp counter looping continuously
      }
      

    }
    
    else if (burpCounter >= burpLoops){

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
    runningState = 1; //TODO change back to 1 for normal looping ops
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
        Serial.println(deltaPSI);
    // *** Delta ***
    //lcd.setCursor(0,1);
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

  //display per running states
  switch(runningState){

    case 0:
      lcd.setCursor(0,1);
      lcd.print("                "); //clear bottom line
      lcd.setCursor(5,0);
      lcd.print("                "); //clear remainder of top line to the right of pressure reading
      lcd.setCursor(8,0);
      lcd.print("Ready");
    break;

    case 1:
      lcd.setCursor(13,1);
      lcd.print(int((millis() - startTime)/1000)); //run time
    break;

    case 2:
      lcd.setCursor(13,1);
      lcd.print(int((millis() - startTime)/1000)); //run time
    break;
  }



      //display pressure in first two digits of display
      
      int tempPress = int(analogRead(ptPin)/17.05);
      if(tempPress >= 10){
         lcd.setCursor(0,0);
         lcd.print(tempPress);
      }
      else{
        lcd.setCursor(0,0);
        lcd.print(" ");
        lcd.setCursor(1,0);
        lcd.print(tempPress);
      }
      //moved up to void setup to prevent blinking
}
