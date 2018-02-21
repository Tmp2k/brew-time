/*
  AnalogReadSerial
  Reads an analog input on pin 0, prints the result to the serial monitor.
  Graphical representation is available using serial plotter (Tools > Serial Plotter menu)
  Attach the center pin of a potentiometer to pin A0, and the outside pins to +5V and ground.

  This example code is in the public domain.
*/


#define DISPLAY_VOLT 500 //This correlates ot the voltage read at the pin. 400 = LED ON   750 = LED OFF
#define ON_DELAY 6000 //This is the delay in ms before another turnOn command will be executed as multiple quick button presses set the temperature
#define OFF_DELAY 1000 //This is the minimum time after turning on that you can turn back off.
#define PIN_DISPLAY A0
#define PIN_HEAT 4
#define PIN_HEAT_BTN 2
#define PIN_POWER_BTN 3

long lastOn = -ON_DELAY;


// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);

  pinMode(PIN_HEAT_BTN,OUTPUT);
  digitalWrite(PIN_HEAT_BTN,HIGH);
  pinMode(PIN_POWER_BTN,OUTPUT);
  digitalWrite(PIN_POWER_BTN,HIGH);
  
}

// the loop routine runs over and over again forever:
void loop() {

  

  if (isOn()) {
    Serial.print("ON  ");
  } else {
    Serial.print("OFF ");
  }
  if (isPowerOn()) {
    Serial.print("PWR: ON  ");
  } else {
    Serial.print("PWR: OFF ");
  }
  if (isHeatOn()) {
    Serial.print("HEAT: ON  ");
  } else {
    Serial.print("HEAT: OFF ");
  }

  Serial.println("");
  delay(100);        // delay in between reads for stability
}


void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();

    if (inChar == 'n') {
      turnOn();
    }
    if (inChar == 'f') {
      turnOff();
    }
  }
  
}


bool isOn() {
  return millis() - lastOn < ON_DELAY || isHeatOn();
}

bool turnOn() {
  if(isOn()) return false;
  lastOn = millis();
  if(!isPowerOn()) {
    digitalWrite(PIN_POWER_BTN,LOW);
    delay(100);
    digitalWrite(PIN_POWER_BTN,HIGH);
  }

  if(!isHeatOn()) {
    digitalWrite(PIN_HEAT_BTN,LOW);
    delay(100);
    digitalWrite(PIN_HEAT_BTN,HIGH);
  }

  return true;
  
}

bool turnOff() {
  if(millis() - lastOn < OFF_DELAY) return false;
  lastOn = -ON_DELAY;
  if(isPowerOn()) {    
    digitalWrite(PIN_POWER_BTN,LOW);
    delay(100);
    digitalWrite(PIN_POWER_BTN,HIGH);
  }

  return true;
}

bool isPowerOn() {
  int sensorValue = analogRead(PIN_DISPLAY);
  return (sensorValue < DISPLAY_VOLT);
}

bool isHeatOn() {
  int sensorValue = digitalRead(PIN_HEAT);
  return (sensorValue == HIGH);
}
