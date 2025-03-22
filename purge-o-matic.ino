#include <LiquidCrystal_I2C.h>

const float MAX_RANGE = 1023.0;
const float MAX_PSI = 60.0;
const int switchPin = 2;
const int relayPin = 3;
const int freqPin = 4;
float prevPSI = 0.0;
float curPSI = 0.0;
float deltaPSI = 0.0;

int loops = 0;
bool running = 0;
bool needToProcessStop = false;
bool ventingState = false;

unsigned long int switched_on_millis=0UL;
unsigned long int lastDeltaCheck=0UL;

enum stage {STOPPED, RUNNING_PURGE, RUNNING_BURP};

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  //Setup serial comms
  Serial.begin(9600); 

  pinMode(switchPin, INPUT);
  pinMode(relayPin, OUTPUT);
  pinMode(freqPin, OUTPUT);

  lcd.init();
  lcd.backlight();
  welcome_screen();
  set_display_labels();
  running = digitalRead(switchPin);
  digitalWrite(freqPin, LOW);
  update_state_label(running);
}

void set_display_labels(){
  lcd.setCursor(12,0);
  lcd.print(":---");
  lcd.setCursor(0,0);
  lcd.print("PSI:--");
  lcd.setCursor(9,1);
  lcd.print("LOOP:");
  lcd.setCursor(0,1);
  lcd.print("MINS:---");
}

void update_switch_status(int switchState) {
  
  if(switchState == 1){
    running = 1;
    switched_on_millis = millis();
  }
  else {
    running = 0;
    switched_on_millis = 0UL;
  }
}

void update_state_label(int state) {
  
  lcd.setCursor(7,0);
  switch(state){
    case 0:
      lcd.print(" STOP");
      break;
    case 1:
      lcd.print("PURGE");
      break;
    case 2:
      lcd.print(" BURP");
      break;
    default:
      lcd.print("ERROR");
      break;
  }

}

void delay_with_updates(int milliseconds){
  
  unsigned long previousMillis = millis();
  unsigned long currentMillis = previousMillis;
  unsigned long interval = milliseconds;
  
  while(true) {
    
    // Check if run switch has been turned off
    if(digitalRead(switchPin) == 0 ) {
      needToProcessStop = true;
      break;
    }

    //Update Cycle secs
    lcd.setCursor(13,0);
    lcd.print((currentMillis - previousMillis)/1000);
    //Serial.println((currentMillis - previousMillis)/1000);
    //Serial.println(deltaPSI);
    if(currentMillis - previousMillis > interval) { 
    //if(deltaPSI < 0.05){
      break;
    }

   update_display();

    currentMillis = millis();
  }
}

void update_psi(){
  if(running==1){
    digitalWrite(freqPin, !digitalRead(freqPin)); //scope freq
    //comparison
    curPSI = analogRead(A0); //rewrite current data
    

    //Delta processing 
    if((millis() - lastDeltaCheck > 1000) && ventingState == false) {
      deltaPSI = abs((prevPSI-curPSI)/(curPSI+1)); //compare old to new, percentage change return
      lastDeltaCheck = millis();
      prevPSI = curPSI;
      Serial.println(deltaPSI);
    }

  
    int psi = (curPSI / MAX_RANGE) * MAX_PSI; //math to convert 0-1023 to PSI
    

    lcd.setCursor(4, 0);
    if(psi < 10)
      lcd.print(" ");    
    lcd.print(psi);
  } else {
    clear_psi();
  }
}

void clear_psi(){
  lcd.setCursor(0,0);
  lcd.print("PSI:--");  
}

void update_mins(){
  lcd.setCursor(0,1);
  lcd.print("000");
}

void clear_mins(){
  lcd.setCursor(0,1);
  lcd.print("MINS:---");
}

void clear_secs(){
  lcd.setCursor(13,0);
  lcd.print("---");
}

void loop() {
  loops = loops + 1;

  if(running == 1) {

    update_display();
    update_switch_status(digitalRead(switchPin));
    
    ventingState = false;
    digitalWrite(relayPin, LOW); //close valve for 10
    update_state_label(running);
    delay_with_updates(10000);
    ventingState = true;
    digitalWrite(relayPin, HIGH); //open valve for .5 seconds
    delay_with_updates(500);

    if(needToProcessStop){
      needToProcessStop=false;
    }else{
      
      //Burp
      update_state_label(running + 1);
      delay_with_updates(3000);
    }


  } else {
    loops=0;
    clear_psi();
    clear_secs();

    update_state_label(running);
    update_switch_status(digitalRead(switchPin));
    digitalWrite(relayPin, LOW); //close valve

  }
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
  delay(3000);
  
  //clear screen
  lcd.clear();
}

void update_display () {
  //PSI
  update_psi();

  //"STAGE"

  //MINS
  lcd.setCursor(5 ,1);
  unsigned long mins = (millis() - switched_on_millis) / 60000 ;
  lcd.print(mins);

  //LOOP
  lcd.setCursor(14, 1);
  
  if(loops == 0){
    lcd.print("--");
  } else {
    if(loops<10)
      lcd.print(" ");
    lcd.print(loops);
  }

}
